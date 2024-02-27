#include "Debug.h"
#include "MemAccess.h"
#include "PluginManager.h"
#include "TileTypes.h"

#include "modules/EventManager.h"
#include "modules/Gui.h"
#include "modules/Job.h"
#include "modules/Maps.h"
#include "modules/Screen.h"
#include "modules/Textures.h"

#include "df/block_square_event_designation_priorityst.h"
#include "df/init.h"
#include "df/job_list_link.h"
#include "df/map_block.h"
#include "df/tile_designation.h"
#include "df/world.h"

#include <functional>

using namespace DFHack;

DFHACK_PLUGIN("pathable");

REQUIRE_GLOBAL(init);
REQUIRE_GLOBAL(window_x);
REQUIRE_GLOBAL(window_y);
REQUIRE_GLOBAL(window_z);
REQUIRE_GLOBAL(world);

namespace DFHack {
    DBG_DECLARE(pathable, log, DebugCategory::LINFO);
}

static std::vector<TexposHandle> textures;

DFhackCExport command_result plugin_init(color_ostream &out, std::vector<PluginCommand> &commands) {
    textures = Textures::loadTileset("hack/data/art/pathable.png", 32, 32, true);
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    return CR_OK;
}

static void paintScreenPathable(df::coord target, bool show_hidden = false) {
    DEBUG(log).print("entering paintScreen\n");

    bool use_graphics = Screen::inGraphicsMode();

    int selected_tile_texpos = 0;
    Screen::findGraphicsTile("CURSORS", 4, 3, &selected_tile_texpos);

    long pathable_tile_texpos = init->load_bar_texpos[1];
    long unpathable_tile_texpos = init->load_bar_texpos[4];
    long on_off_texpos = Textures::getTexposByHandle(textures[0]);
    if (on_off_texpos > 0) {
        pathable_tile_texpos = on_off_texpos;
        unpathable_tile_texpos = Textures::getTexposByHandle(textures[1]);
    }

    auto dims = Gui::getDwarfmodeViewDims().map();
    for (int y = dims.first.y; y <= dims.second.y; ++y) {
        for (int x = dims.first.x; x <= dims.second.x; ++x) {
            df::coord map_pos(*window_x + x, *window_y + y, *window_z);

            if (!Maps::isValidTilePos(map_pos))
                continue;

            // don't overwrite the target tile
            if (!use_graphics && map_pos == target) {
                TRACE(log).print("skipping target tile\n");
                continue;
            }

            if (!show_hidden && !Maps::isTileVisible(map_pos)) {
                TRACE(log).print("skipping hidden tile\n");
                continue;
            }

            DEBUG(log).print("scanning map tile at offset %d, %d\n", x, y);
            Screen::Pen cur_tile = Screen::readTile(x, y, true);
            DEBUG(log).print("tile data: ch=%d, fg=%d, bg=%d, bold=%s\n",
                    cur_tile.ch, cur_tile.fg, cur_tile.bg, cur_tile.bold ? "true" : "false");
            DEBUG(log).print("tile data: tile=%d, tile_mode=%d, tile_fg=%d, tile_bg=%d\n",
                    cur_tile.tile, cur_tile.tile_mode, cur_tile.tile_fg, cur_tile.tile_bg);

            if (!cur_tile.valid()) {
                DEBUG(log).print("cannot read tile at offset %d, %d\n", x, y);
                continue;
            }

            bool can_walk = Maps::canWalkBetween(target, map_pos);
            DEBUG(log).print("tile is %swalkable at offset %d, %d\n",
                             can_walk ? "" : "not ", x, y);

            if (use_graphics) {
                if (map_pos == target) {
                    cur_tile.tile = selected_tile_texpos;
                } else{
                    cur_tile.tile = can_walk ?
                            pathable_tile_texpos : unpathable_tile_texpos;
                }
            } else {
                int color = can_walk ? COLOR_GREEN : COLOR_RED;
                if (cur_tile.fg && cur_tile.ch != ' ') {
                    cur_tile.fg = color;
                    cur_tile.bg = 0;
                } else {
                    cur_tile.fg = 0;
                    cur_tile.bg = color;
                }

                cur_tile.bold = false;

                if (cur_tile.tile)
                    cur_tile.tile_mode = Screen::Pen::CharColor;
            }

            Screen::paintTile(cur_tile, x, y, true);
        }
    }
}

static bool is_warm(const df::coord &pos) {
    auto block = Maps::getTileBlock(pos);
    if (!block)
        return false;
    return block->temperature_1[pos.x&15][pos.y&15] >= 10075;
}

static bool is_rough_wall(int16_t x, int16_t y, int16_t z) {
    df::tiletype *tt = Maps::getTileType(x, y, z);
    if (!tt)
        return false;

    return tileShape(*tt) == df::tiletype_shape::WALL &&
        tileSpecial(*tt) != df::tiletype_special::SMOOTH;
}

static bool will_leak(int16_t x, int16_t y, int16_t z) {
    auto des = Maps::getTileDesignation(x, y, z);
    if (!des)
        return false;
    if (des->bits.liquid_type == df::tile_liquid::Water && des->bits.flow_size >= 1)
        return true;
    if (des->bits.water_table && is_rough_wall(x, y, z))
        return true;
    return false;
}

static bool is_damp(const df::coord &pos) {
    return will_leak(pos.x-1, pos.y-1, pos.z) ||
        will_leak(pos.x, pos.y-1, pos.z) ||
        will_leak(pos.x+1, pos.y-1, pos.z) ||
        will_leak(pos.x-1, pos.y, pos.z) ||
        will_leak(pos.x+1, pos.y, pos.z) ||
        will_leak(pos.x-1, pos.y+1, pos.z) ||
        will_leak(pos.x, pos.y+1, pos.z) ||
        will_leak(pos.x+1, pos.y+1, pos.z) ||
        will_leak(pos.x, pos.y, pos.z+1);
}

static void paintScreenWarmDamp(bool show_hidden = false) {
    DEBUG(log).print("entering paintScreenDampWarm\n");

    if (Screen::inGraphicsMode())
        return;

    auto dims = Gui::getDwarfmodeViewDims().map();
    for (int y = dims.first.y; y <= dims.second.y; ++y) {
        for (int x = dims.first.x; x <= dims.second.x; ++x) {
            df::coord map_pos(*window_x + x, *window_y + y, *window_z);

            if (!Maps::isValidTilePos(map_pos))
                continue;

            if (!show_hidden && !Maps::isTileVisible(map_pos)) {
                TRACE(log).print("skipping hidden tile\n");
                continue;
            }

            TRACE(log).print("scanning map tile at (%d, %d, %d) screen offset (%d, %d)\n",
                map_pos.x, map_pos.y, map_pos.z, x, y);

            Screen::Pen cur_tile = Screen::readTile(x, y, true);
            if (!cur_tile.valid()) {
                DEBUG(log).print("cannot read tile at offset %d, %d\n", x, y);
                continue;
            }

            int color = is_warm(map_pos) ? COLOR_RED : is_damp(map_pos) ? COLOR_BLUE : COLOR_BLACK;
            if (color == COLOR_BLACK) {
                TRACE(log).print("skipping non-warm, non-damp tile\n");
                continue;
            }

            // this will also change the color of the cursor or any selection box that overlaps
            // the tile. this is intentional since it makes the UI more readable
            if (cur_tile.fg && cur_tile.ch != ' ') {
                cur_tile.fg = color;
                cur_tile.bg = 0;
            } else {
                cur_tile.fg = 0;
                cur_tile.bg = color;
            }

            cur_tile.bold = false;

            if (cur_tile.tile)
                cur_tile.tile_mode = Screen::Pen::CharColor;

            Screen::paintTile(cur_tile, x, y, true);
        }
    }
}

struct designation{
    df::coord pos;
    df::tile_designation td;
    df::tile_occupancy to;
    designation() = default;
    designation(const df::coord &c, const df::tile_designation &td, const df::tile_occupancy &to) : pos(c), td(td), to(to) {}

    bool operator==(const designation &rhs) const {
        return pos == rhs.pos;
    }

    bool operator!=(const designation &rhs) const {
        return !(rhs == *this);
    }
};

namespace std {
    template<>
    struct hash<designation> {
        std::size_t operator()(const designation &c) const {
            std::hash<df::coord> hash_coord;
            return hash_coord(c.pos);
        }
    };
}

class Designations {
private:
    std::unordered_map<df::coord, designation> designations;
public:
    Designations() {
        df::job_list_link *link = world->jobs.list.next;
        for (; link; link = link->next) {
            df::job *job = link->item;

            if(!job || !Maps::isValidTilePos(job->pos))
                continue;

            df::tile_designation td;
            df::tile_occupancy to;
            bool keep_if_taken = false;

            switch (job->job_type) {
                case df::job_type::SmoothWall:
                case df::job_type::SmoothFloor:
                    keep_if_taken = true;
                    // fallthrough
                case df::job_type::CarveFortification:
                    td.bits.smooth = 1;
                    break;
                case df::job_type::DetailWall:
                case df::job_type::DetailFloor:
                    td.bits.smooth = 2;
                    break;
                case job_type::CarveTrack:
                    to.bits.carve_track_north = (job->item_category.whole >> 18) & 1;
                    to.bits.carve_track_south = (job->item_category.whole >> 19) & 1;
                    to.bits.carve_track_west = (job->item_category.whole >> 20) & 1;
                    to.bits.carve_track_east = (job->item_category.whole >> 21) & 1;
                    break;
                default:
                    continue;
            }
            if (keep_if_taken || !Job::getWorker(job))
                designations.emplace(job->pos, designation(job->pos, td, to));
        }
    }

    // get from job; if no job, then fall back to querying map
    designation get(const df::coord &pos) const {
        if (designations.count(pos)) {
            return designations.at(pos);
        }
        auto pdes = Maps::getTileDesignation(pos);
        auto pocc = Maps::getTileOccupancy(pos);
        if (!pdes || !pocc)
            return {};
        return designation(pos, *pdes, *pocc);
    }
};

static bool is_designated_for_smoothing(const designation &designation) {
    return designation.td.bits.smooth == 1;
}

static bool is_designated_for_engraving(const designation &designation) {
    return designation.td.bits.smooth == 2;
}

static bool is_designated_for_track_carving(const designation &designation) {
    const df::tile_occupancy &occ = designation.to;
    return occ.bits.carve_track_east || occ.bits.carve_track_north || occ.bits.carve_track_south || occ.bits.carve_track_west;
}

static char get_track_char(const designation &designation) {
    const df::tile_occupancy &occ = designation.to;
    if (occ.bits.carve_track_east && occ.bits.carve_track_north && occ.bits.carve_track_south && occ.bits.carve_track_west)
        return (char)0xCE; // NSEW
    if (occ.bits.carve_track_east && occ.bits.carve_track_north && occ.bits.carve_track_south)
        return (char)0xCC; // NSE
    if (occ.bits.carve_track_east && occ.bits.carve_track_north && occ.bits.carve_track_west)
        return (char)0xCA; // NEW
    if (occ.bits.carve_track_east && occ.bits.carve_track_south && occ.bits.carve_track_west)
        return (char)0xCB; // SEW
    if (occ.bits.carve_track_north && occ.bits.carve_track_south && occ.bits.carve_track_west)
        return (char)0xB9; // NSW
    if (occ.bits.carve_track_north && occ.bits.carve_track_south)
        return (char)0xBA; // NS
    if (occ.bits.carve_track_east && occ.bits.carve_track_west)
        return (char)0xCD; // EW
    if (occ.bits.carve_track_east && occ.bits.carve_track_north)
        return (char)0xC8; // NE
    if (occ.bits.carve_track_north && occ.bits.carve_track_west)
        return (char)0xBC; // NW
    if (occ.bits.carve_track_east && occ.bits.carve_track_south)
        return (char)0xC9; // SE
    if (occ.bits.carve_track_south && occ.bits.carve_track_west)
        return (char)0xBB; // SW
    if (occ.bits.carve_track_north)
        return (char)0xD0; // N
    if (occ.bits.carve_track_south)
        return (char)0xD2; // S
    if (occ.bits.carve_track_east)
        return (char)0xC6; // E
    if (occ.bits.carve_track_west)
        return (char)0xB5; // W
    return (char)0xC5; // single line cross; should never happen
}

static bool is_smooth_wall(const df::coord &pos) {
    df::tiletype *tt = Maps::getTileType(pos);
    return tt && tileSpecial(*tt) == df::tiletype_special::SMOOTH
                && tileShape(*tt) == df::tiletype_shape::WALL;
}

static bool blink(int delay) {
    return (Core::getInstance().p->getTickCount()/delay) % 2 == 0;
}

static char get_tile_char(const df::coord &pos, char desig_char, bool draw_priority) {
    if (!draw_priority)
        return desig_char;

    std::vector<df::block_square_event_designation_priorityst *> priorities;
    Maps::SortBlockEvents(Maps::getTileBlock(pos), NULL, NULL, NULL, NULL, NULL, NULL, NULL, &priorities);
    if (priorities.empty())
        return desig_char;
    switch (priorities[0]->priority[pos.x % 16][pos.y % 16] / 1000) {
    case 1: return '1';
    case 2: return '2';
    case 3: return '3';
    case 4: return '4';
    case 5: return '5';
    case 6: return '6';
    case 7: return '7';
    default:
        return '4';
    }
}

static void paintScreenCarve() {
    DEBUG(log).print("entering paintScreenCarve\n");

    if (Screen::inGraphicsMode() || blink(500))
        return;

    Designations designations;
    bool draw_priority = blink(1000);

    auto dims = Gui::getDwarfmodeViewDims().map();
    for (int y = dims.first.y; y <= dims.second.y; ++y) {
        for (int x = dims.first.x; x <= dims.second.x; ++x) {
            df::coord map_pos(*window_x + x, *window_y + y, *window_z);

            if (!Maps::isValidTilePos(map_pos))
                continue;

            if (!Maps::isTileVisible(map_pos)) {
                TRACE(log).print("skipping hidden tile\n");
                continue;
            }

            TRACE(log).print("scanning map tile at (%d, %d, %d) screen offset (%d, %d)\n",
                map_pos.x, map_pos.y, map_pos.z, x, y);

            Screen::Pen cur_tile;
            cur_tile.fg = COLOR_DARKGREY;

            auto des = designations.get(map_pos);

            if (is_designated_for_smoothing(des)) {
                if (is_smooth_wall(map_pos))
                    cur_tile.ch = get_tile_char(map_pos, (char)206, draw_priority); // hash, indicating a fortification designation
                else
                    cur_tile.ch = get_tile_char(map_pos, (char)219, draw_priority); // solid block, indicating a smoothing designation
            }
            else if (is_designated_for_engraving(des)) {
                cur_tile.ch = get_tile_char(map_pos, (char)10, draw_priority); // solid block with a circle on it
            }
            else if (is_designated_for_track_carving(des)) {
                cur_tile.ch = get_tile_char(map_pos, get_track_char(des), draw_priority); // directional track
            }
            else {
                TRACE(log).print("skipping tile with no carving designation\n");
                continue;
            }

            Screen::paintTile(cur_tile, x, y, true);
        }
    }
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(paintScreenPathable),
    DFHACK_LUA_FUNCTION(paintScreenWarmDamp),
    DFHACK_LUA_FUNCTION(paintScreenCarve),
    DFHACK_LUA_END
};
