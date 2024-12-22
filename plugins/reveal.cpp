#include "PluginManager.h"
#include "TileTypes.h"

#include "modules/EventManager.h"
#include "modules/Gui.h"
#include "modules/Maps.h"
#include "modules/Screen.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/deep_vein_hollow.h"
#include "df/divine_treasure.h"
#include "df/encased_horror.h"
#include "df/gamest.h"
#include "df/map_block.h"
#include "df/world.h"

#include <unordered_set>

using std::string;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("reveal");
DFHACK_PLUGIN_IS_ENABLED(is_active);

REQUIRE_GLOBAL(game);
REQUIRE_GLOBAL(world);

struct hideblock {
    df::coord c;
    uint8_t hiddens[16][16];
};

std::unordered_set<df::coord> trigger_cache;
static vector<hideblock> hidesaved;

enum revealstate {
    NOT_REVEALED,
    REVEALED,
    SAFE_REVEALED,
    DEMON_REVEALED
};

static revealstate revealed = NOT_REVEALED;
static int32_t cycle_timestamp = 0;  // world->frame_counter at last reveal

static void reset_state() {
    is_active = false;
    trigger_cache.clear();
    hidesaved.clear();
    revealed = NOT_REVEALED;
    cycle_timestamp = 0;
}

/*
 * Anything that might reveal Hell or trigger gemstone pillar events is unsafe.
 */
bool isSafe(df::coord c) {
    // convert to block coordinates
    c.x >>= 4;
    c.y >>= 4;

    // Don't reveal blocks that contain trigger events
    if (trigger_cache.contains(c))
        return false;

    t_feature local_feature;
    t_feature global_feature;
    // get features of block
    // error -> obviously not safe to manipulate
    if(!Maps::ReadFeatures(c.x, c.y, c.z, &local_feature, &global_feature))
        return false;

    // Adamantine tubes and temples lead to Hell
    if (local_feature.type == df::feature_type::deep_special_tube ||
            local_feature.type == df::feature_type::deep_surface_portal)
        return false;
    // And Hell *is* Hell.
    if (global_feature.type == df::feature_type::underworld_from_layer)
        return false;
    // otherwise it's safe.
    return true;
}

static void update_minimap() {
    game->minimap.update = 1;
    game->minimap.mustmake = 1;
}

static void do_reveal_adventure(color_ostream &out, bool no_hell, bool incremental);
command_result reveal(color_ostream &out, vector<string> & params);
command_result unreveal(color_ostream &out, vector<string> & params);
command_result revtoggle(color_ostream &out, vector<string> & params);
command_result revflood(color_ostream &out, vector<string> & params);
command_result revforget(color_ostream &out, vector<string> & params);

DFhackCExport command_result plugin_init(color_ostream &out, vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand(
        "reveal",
        "Reveal the map.",
        reveal));
    commands.push_back(PluginCommand(
        "unreveal",
        "Revert a revealed map to its unrevealed state.",
        unreveal));
    commands.push_back(PluginCommand(
        "revtoggle",
        "Switch betwen reveal and unreveal.",
        revtoggle));
    commands.push_back(PluginCommand(
        "revflood",
        "Hide everything, then reveal tiles with a path to a unit.",
        revflood));
    commands.push_back(PluginCommand(
        "revforget",
        "Discard records about what was visible before revealing the map.",
        revforget));
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (World::isAdventureMode()) {
        if (cycle_timestamp < world->frame_counter) {
            cycle_timestamp = world->frame_counter;
            do_reveal_adventure(out, revealed == SAFE_REVEALED, true);
        }
    } else {
        World::SetPauseState(true);
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    reset_state();
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == DFHack::SC_WORLD_UNLOADED)
        reset_state();
    return CR_OK;
}

static void cache_tiles(const df::coord_path & tiles) {
    size_t num_tiles = tiles.size();
    for (size_t idx = 0; idx < num_tiles; ++idx) {
        df::coord pos = tiles[idx];
        pos.x >>= 4;
        pos.y >>= 4;
        trigger_cache.insert(pos);
    }
}

static void initialize_trigger_cache() {
    for (auto & horror : world->event.encased_horrors)
        cache_tiles(horror->tiles);
    for (auto & hollow : world->event.deep_vein_hollows)
        cache_tiles(hollow->tiles);
    for (auto & treasure : world->event.divine_treasures)
        cache_tiles(treasure->tiles);
}

// identify blocks that have tiles within the box that contains the unit's
// 26 tile radius vision cylinder
static bool is_vision_block(const df::coord & upos, const df::coord & bpos, int offset = 0) {
    const int radius = 27 + offset;

    int ux1, ux2, uy1, uy2;
    ux1 = upos.x - radius;
    ux2 = upos.x + radius;
    uy1 = upos.y - radius;
    uy2 = upos.y + radius;

    int bx1, bx2, by1, by2;
    bx1 = bpos.x;
    bx2 = bpos.x + 15;
    by1 = bpos.y;
    by2 = bpos.y + 15;

    int clip_x1, clip_x2, clip_y1, clip_y2;
    clip_x1 = std::max(ux1, bx1);
    clip_y1 = std::max(uy1, by1);
    clip_x2 = std::min(ux2, bx2);
    clip_y2 = std::min(uy2, by2);

    return clip_x1 <= clip_x2 && clip_y1 <= clip_y2;
}

static void do_reveal_adventure(color_ostream &out, bool no_hell, bool incremental = false) {
    df::coord pos;
    if (auto unit = World::getAdventurer())
        pos = Units::getPosition(unit);
    for (auto & block : world->map.map_blocks) {
        if (incremental && !is_vision_block(pos, block->map_pos))
            continue;
        if (no_hell && !isSafe(block->map_pos))
            continue;
        designations40d & designations = block->designation;
        for (uint32_t x = 0; x < 16; x++) for (uint32_t y = 0; y < 16; y++) {
            designations[x][y].bits.hidden = 0;
            // set "lit" property (named "pile" because of how it is used in fort mode)
            designations[x][y].bits.pile = 1;
        }
    }
}

static void do_reveal_fort(color_ostream &out, bool no_hell) {
    hidesaved.reserve(world->map.map_blocks.size());
    for (auto & block : world->map.map_blocks) {
        if (no_hell && !isSafe(block->map_pos))
            continue;
        hidesaved.emplace_back();
        hideblock *hb = &hidesaved.back();
        hb->c = block->map_pos;
        designations40d & designations = block->designation;
        for (uint32_t x = 0; x < 16; x++) for (uint32_t y = 0; y < 16; y++) {
            // save state of tile and set to revealed
            hb->hiddens[x][y] = designations[x][y].bits.hidden;
            designations[x][y].bits.hidden = 0;
        }
    }
    update_minimap();
}

command_result reveal(color_ostream &out, vector<string> & params) {
    bool no_hell = true;
    bool pause = false;
    for (auto & param : params) {
        if (param == "hell") {
            no_hell = false;
            pause = true;
        } else if (param == "demon") {
            out.printerr("`reveal demon` is currently disabled to prevent a hang due to a bug in the base game\n");
            return CR_FAILURE;
            //no_hell = false;
            //pause = false;
        }
        else if (param == "help" || param == "?")
            return CR_WRONG_USAGE;
    }

    if (revealed != NOT_REVEALED) {
        out.printerr("Map is already revealed.\n");
        return CR_FAILURE;
    }

    if (!Maps::IsValid()) {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    if (no_hell) {
        size_t initial_buckets = 2 * (world->event.encased_horrors.size() + world->event.divine_treasures.size() + world->event.deep_vein_hollows.size());
        trigger_cache.reserve(initial_buckets);
        initialize_trigger_cache();
    }

    out.print("Map revealed.\n\n");
    if (World::isAdventureMode()) {
        do_reveal_adventure(out, no_hell);
        is_active = true;
    } else if (World::isFortressMode()) {
        do_reveal_fort(out, no_hell);

        if (Screen::inGraphicsMode()) {
            out.print("Note that in graphics mode, tiles that are not adjacent to open\n"
                    "space will not render but can still be examined by hovering over\n"
                    "them with the mouse. Switching to text mode (in the game settings)\n"
                    "will allow the display of the revealed tiles.\n\n");
        }

        if (pause)
            out.print("Unpausing can unleash the forces of hell, so it has been temporarily disabled.\n\n");
        out.print("Run 'unreveal' to revert to previous state.\n");
    } else {
        out.printerr("Can only reveal in adventure or fortress mode.\n");
        return CR_FAILURE;
    }

    if (no_hell)
        revealed = SAFE_REVEALED;
    else {
        if (pause) {
            revealed = REVEALED;
            is_active = true;
            if (World::isFortressMode())
                World::SetPauseState(true);
        }
        else
            revealed = DEMON_REVEALED;
    }

    return CR_OK;
}

command_result unreveal(color_ostream &out, vector<string> & params) {
    for (auto & param : params) {
        if (param == "help" || param == "?")
            return CR_WRONG_USAGE;
    }

    if (!revealed) {
        out.printerr("There's nothing to revert!\n");
        return CR_FAILURE;
    }

    if (!Maps::IsValid()) {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    if (World::isAdventureMode()) {
        auto unit = World::getAdventurer();
        df::coord upos;
        if (unit)
            upos = Units::getPosition(unit);
        for (auto & block : world->map.map_blocks) {
            if (unit && is_vision_block(upos, block->map_pos, -1) && upos.z == block->map_pos.z)
                continue;
            designations40d & designations = block->designation;
            for (uint32_t x = 0; x < 16; x++) for (uint32_t y = 0; y < 16; y++) {
                designations[x][y].bits.hidden = 1;
                designations[x][y].bits.pile = 0;
            }
        }
    } else {
        for (auto & hb : hidesaved) {
            df::map_block * b = Maps::getTileBlock(hb.c.x, hb.c.y, hb.c.z);
            for (uint32_t x = 0; x < 16; x++) for (uint32_t y = 0; y < 16; y++) {
                b->designation[x][y].bits.hidden = hb.hiddens[x][y];
            }
        }
        update_minimap();
    }

    reset_state();

    out.print("Map hidden!\n");
    return CR_OK;
}

command_result revtoggle(color_ostream &out, vector<string> & params) {
    if (revealed)
        return unreveal(out, params);
    else
        return reveal(out, params);
}

// Unhides map tiles according to visibility, starting from the given
// coordinates. This algorithm only processes adjacent hidden tiles, so it must
// start on a hidden tile and it will not reveal hidden sections separated by
// already-unhidden tiles.
static void unhideFlood_internal(const df::coord &xy) {
    typedef std::pair <df::coord, bool> PosAndBelow;
    std::stack<PosAndBelow> flood;
    flood.emplace(xy, false);

    while (!flood.empty()) {
        PosAndBelow tile = flood.top();
        df::coord & current = tile.first;
        bool & from_below = tile.second;
        flood.pop();

        if(!Maps::isValidTilePos(current))
            continue;
        df::tile_designation *des = Maps::getTileDesignation(current);
        if(!des || !des->bits.hidden)
            continue;

        // we don't want constructions or ice to restrict vision (to avoid bug #1871)
        df::tiletype *tt = Maps::getTileType(current);
        if (!tt)
            continue;

        bool below = false;
        bool above = false;
        bool sides = false;
        bool unhide = true;
        // By tile shape, determine behavior and action
        switch (tileShape(*tt)) {
        // Walls
        case tiletype_shape::WALL:
            if (from_below)
                unhide = false;
            else if (tileMaterial(*tt) == tiletype_material::CONSTRUCTION ||
                tileMaterial(*tt) == tiletype_material::FROZEN_LIQUID)
            {
                // treat as a floor
                above = sides = true;
            }
            break;
        // Open space
        case tiletype_shape::NONE:
        case tiletype_shape::EMPTY:
        case tiletype_shape::RAMP_TOP:
        case tiletype_shape::STAIR_UPDOWN:
        case tiletype_shape::STAIR_DOWN:
        case tiletype_shape::BROOK_TOP:
            above = below = sides = true;
            break;
        // Floors
        case tiletype_shape::FORTIFICATION:
        case tiletype_shape::STAIR_UP:
        case tiletype_shape::RAMP:
        case tiletype_shape::FLOOR:
        case tiletype_shape::BRANCH:
        case tiletype_shape::TRUNK_BRANCH:
        case tiletype_shape::TWIG:
        case tiletype_shape::SAPLING:
        case tiletype_shape::SHRUB:
        case tiletype_shape::BOULDER:
        case tiletype_shape::PEBBLES:
        case tiletype_shape::BROOK_BED:
        case tiletype_shape::ENDLESS_PIT:
            if (from_below)
                unhide = false;
            else
                above = sides = true;
            break;
        }

        // Special case for trees - always reveal them as if they were floor tiles
        if (tileMaterial(*tt) == tiletype_material::PLANT ||
            tileMaterial(*tt) == tiletype_material::MUSHROOM)
        {
            if (from_below)
                unhide = false;
            else
                above = sides = true;
        }

        if (unhide) {
            des->bits.hidden = false;
        }
        if (sides) {
            // Scan adjacent tiles clockwise, starting toward east
            flood.emplace(df::coord(current.x + 1, current.y    , current.z), false);
            flood.emplace(df::coord(current.x + 1, current.y + 1, current.z), false);
            flood.emplace(df::coord(current.x    , current.y + 1, current.z), false);
            flood.emplace(df::coord(current.x - 1, current.y + 1, current.z), false);
            flood.emplace(df::coord(current.x - 1, current.y    , current.z), false);
            flood.emplace(df::coord(current.x - 1, current.y - 1, current.z), false);
            flood.emplace(df::coord(current.x    , current.y - 1, current.z), false);
            flood.emplace(df::coord(current.x + 1, current.y - 1, current.z), false);
        }
        if (above)
            flood.emplace(df::coord(current.x, current.y, current.z + 1), true);
        if (below)
            flood.emplace(df::coord(current.x, current.y, current.z - 1), false);
    }

    update_minimap();
}

// Lua entrypoint for unhideFlood_internal
static void unhideFlood(df::coord pos) {
    // no environment or bounds checking needed. if anything is invalid,
    // unhideFlood_internal will just exit immeditately
    unhideFlood_internal(pos);
}

command_result revflood(color_ostream &out, vector<string> & params) {
    for (auto & param : params) {
        if (param == "help" || param == "?")
            return CR_WRONG_USAGE;
    }

    if (!Maps::IsValid()) {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    if (revealed != NOT_REVEALED) {
        out.printerr("This is only safe to use with non-revealed map.\n");
        return CR_FAILURE;
    }

    if(!World::isFortressMode()) {
        out.printerr("Only in proper dwarf mode.\n");
        return CR_FAILURE;
    }

    df::coord pos;
    Gui::getCursorCoords(pos);
    if (!pos.isValid()) {
        df::unit *unit = Gui::getSelectedUnit(out, true);
        if (unit)
            pos = Units::getPosition(unit);
    }
    if (!pos.isValid()) {
        for (auto unit : Units::citizensRange(world->units.active)) {
            pos = Units::getPosition(unit);
            break;
        }
    }
    if (!pos.isValid()) {
        out.printerr("Please select a unit or place the keyboard cursor at some empty space you want to be unhidden.\n");
        return CR_FAILURE;
    }

    df::tiletype *tt = Maps::getTileType(pos);
    if (!tt || isWallTerrain(*tt)) {
        out.printerr("Please select a unit or place the keyboard cursor at some empty space you want to be unhidden.\n");
        return CR_FAILURE;
    }

    for (auto & b : world->map.map_blocks) {
        // change the hidden flag to 0
        for (uint32_t x = 0; x < 16; x++) for (uint32_t y = 0; y < 16; y++)
        {
            b->designation[x][y].bits.hidden = 1;
        }
    }

    unhideFlood_internal(pos);

    return CR_OK;
}

command_result revforget(color_ostream &out, vector<string> & params) {
    for (auto & param : params) {
        if (param == "help" || param == "?")
            return CR_WRONG_USAGE;
    }

    if (!revealed) {
        out.printerr("There's nothing to forget!\n");
        return CR_FAILURE;
    }

    reset_state();
    out.print("Reveal data forgotten!\n");
    return CR_OK;
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(unhideFlood),
    DFHACK_LUA_END
};
