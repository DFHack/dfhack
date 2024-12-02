#include "Debug.h"
#include "Error.h"
#include "PluginManager.h"
#include "TileTypes.h"

#include "df/building_type.h"
#include "df/building_hatchst.h"
#include "df/building_bars_floorst.h"
#include "df/building_grate_floorst.h"
#include "df/tile_building_occ.h"
#include "modules/Buildings.h"
#include "modules/Gui.h"
#include "modules/Maps.h"
#include "modules/Screen.h"
#include "modules/Textures.h"

#include "df/building_tradedepotst.h"
#include "df/init.h"
#include "df/plotinfost.h"
#include "df/world.h"

using namespace DFHack;
using std::stack;
using std::unordered_set;

DFHACK_PLUGIN("pathable");

REQUIRE_GLOBAL(init);
REQUIRE_GLOBAL(plotinfo);
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

struct PaintCtx {
    bool use_graphics;
    int selected_tile_texpos;
    long pathable_tile_texpos;
    long unpathable_tile_texpos;

    PaintCtx() {
        use_graphics = Screen::inGraphicsMode();
        selected_tile_texpos = 0;
        Screen::findGraphicsTile("CURSORS", 4, 3, &selected_tile_texpos);
        pathable_tile_texpos = init->load_bar_texpos[1];
        unpathable_tile_texpos = init->load_bar_texpos[4];
        long on_off_texpos = Textures::getTexposByHandle(textures[0]);
        if (on_off_texpos > 0) {
            pathable_tile_texpos = on_off_texpos;
            unpathable_tile_texpos = Textures::getTexposByHandle(textures[1]);
        }
    }
};

static void paint_screen(const PaintCtx & ctx, const unordered_set<df::coord> & targets,
    bool show_hidden, std::function<bool(const df::coord & pos)> get_can_walk)
{
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

            bool can_walk = get_can_walk(map_pos);
            DEBUG(log).print("tile is %swalkable at offset %d, %d\n",
                             can_walk ? "" : "not ", x, y);

            if (ctx.use_graphics) {
                if (targets.contains(map_pos)) {
                    cur_tile.tile = ctx.selected_tile_texpos;
                } else{
                    cur_tile.tile = can_walk ?
                            ctx.pathable_tile_texpos : ctx.unpathable_tile_texpos;
                }
            } else {
                int color = can_walk ? COLOR_GREEN : COLOR_RED;
                if (targets.contains(map_pos))
                    color = COLOR_CYAN;
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

static void paintScreenPathable(df::coord target, bool show_hidden = false) {
    DEBUG(log).print("entering paintScreen\n");

    PaintCtx ctx;
    unordered_set<df::coord> targets;
    targets.emplace(target);
    paint_screen(ctx, targets, show_hidden, [&](const df::coord & pos){
        return Maps::canWalkBetween(target, pos);
    });
}

static bool get_depot_coords(color_ostream &out, unordered_set<df::coord> * depot_coords) {
    CHECK_NULL_POINTER(depot_coords);

    depot_coords->clear();
    for (auto bld : world->buildings.other.TRADE_DEPOT){
        DEBUG(log,out).print("found depot at (%d, %d, %d)\n", bld->centerx, bld->centery, bld->z);
        depot_coords->emplace(bld->centerx, bld->centery, bld->z);
    }

    return !depot_coords->empty();
}

static bool get_pathability_groups(color_ostream &out, unordered_set<uint16_t> * depot_pathability_groups,
    const unordered_set<df::coord> & depot_coords)
{
    CHECK_NULL_POINTER(depot_pathability_groups);

    for (auto pos : depot_coords) {
        auto wgroup = Maps::getWalkableGroup(pos);
        if (wgroup) {
            DEBUG(log,out).print("walkability group at (%d, %d, %d) is %d\n", pos.x, pos.y, pos.z, wgroup);
            depot_pathability_groups->emplace(wgroup);
        }
    }
    return !depot_pathability_groups->empty();
}

// if entry_tiles is given, it is filled with the surface edge tiles that match one of the given
// depot pathability groups. If entry_tiles is NULL, just returns true if such a tile is found.
// returns false if no tiles are found
static bool get_entry_tiles(unordered_set<df::coord> * entry_tiles, const unordered_set<uint16_t> & depot_pathability_groups) {
    auto & edge = plotinfo->map_edge;
    size_t num_edge_tiles = edge.surface_x.size();
    bool found = false;
    for (size_t idx = 0; idx < num_edge_tiles; ++idx) {
        df::coord pos(edge.surface_x[idx], edge.surface_y[idx], edge.surface_z[idx]);
        auto wgroup = Maps::getWalkableGroup(pos);
        if (depot_pathability_groups.contains(wgroup)) {
            found = true;
            if (!entry_tiles)
                break;
            entry_tiles->emplace(pos);
        }
    }
    return found;
}

static bool getDepotAccessibleByAnimals(color_ostream &out) {
    unordered_set<df::coord> depot_coords;
    if (!get_depot_coords(out, &depot_coords))
        return false;
    unordered_set<uint16_t> depot_pathability_groups;
    if (!get_pathability_groups(out, &depot_pathability_groups, depot_coords))
        return false;
    return get_entry_tiles(NULL, depot_pathability_groups);
}

struct FloodCtx {
    uint16_t wgroup;
    unordered_set<df::coord> & wagon_path;
    unordered_set<df::coord> seen; // contains tiles that should not be added to search_edge
    stack<df::coord> search_edge;  // contains tiles that can be successfully moved into
    const unordered_set<df::coord> & entry_tiles;

    FloodCtx(uint16_t wgroup, unordered_set<df::coord> & wagon_path, const unordered_set<df::coord> & entry_tiles)
        : wgroup(wgroup), wagon_path(wagon_path), entry_tiles(entry_tiles) {}
};

static bool is_wagon_dynamic_traversible(df::tiletype_shape shape, const df::coord & pos) {
    auto bld = Buildings::findAtTile(pos);
    if (!bld) return false;

    auto btype = bld->getType();
    // open hatch should be inaccessible regardless of the tile it sits on
    if (btype == df::building_type::Hatch) {
        if (shape == df::tiletype_shape::RAMP_TOP)
            return false;

        auto& hatch = *static_cast<df::building_hatchst*>(bld);
        if (hatch.door_flags.bits.closed)
            return true;
    // open floor grate/bar should be inaccessible regardless of the tile it sits on
    } else if (btype == df::building_type::GrateFloor) {
        auto& b = *static_cast<df::building_grate_floorst*>(bld);
        if (b.gate_flags.bits.closed)
            return true;
    } else if (btype == df::building_type::BarsFloor) {
        auto& b = *static_cast<df::building_bars_floorst*>(bld);
        if (b.gate_flags.bits.closed)
            return true;
    }
    // Doors, traps..etc
    return false;
}

// NOTE: When i.e. tracks, stairs have a bridge over them, the tile will have
// an occupancy of floored.
static bool is_wagon_tile_traversible(df::tiletype& tt) {
    auto shape = tileShape(tt);
    auto special = tileSpecial(tt);
    auto material = tileMaterial(tt);

    // Allow murky pool ramps
    if (shape == df::tiletype_shape::RAMP_TOP)
        return true;
    // NOTE: smoothing a boulder turns it into a smoothed floor
    else if (shape == df::tiletype_shape::STAIR_UP || shape == df::tiletype_shape::STAIR_DOWN ||
            shape == df::tiletype_shape::STAIR_UPDOWN || shape == df::tiletype_shape::BOULDER ||
            shape == df::tiletype_shape::EMPTY || shape == df::tiletype_shape::NONE)
        return false;
    else if (special == df::tiletype_special::TRACK)
        return false;
    // Fires seem to have their own path group, and group for lava is 0
    // According to wiki, the wagon won't path thru pool and river tiles, but ramps are ok
    else if (material == df::tiletype_material::POOL || material == df::tiletype_material::RIVER)
        return false;

    return true;
}

static bool is_wagon_traversible(FloodCtx & ctx, const df::coord & pos, const df::coord & prev_pos) {
    auto tt = Maps::getTileType(pos);
    if (!tt)
        return false;

    auto shape = tileShape(*tt);
    auto& occ = *Maps::getTileOccupancy(pos);
    switch (occ.bits.building) {
        case tile_building_occ::Obstacle: // Statues, windmills (middle tile)
            //FALLTHROUGH
        case tile_building_occ::Well:
            //FALLTHROUGH
        case tile_building_occ::Impassable: // Raised bridges
            return false;

        case tile_building_occ::Dynamic:
            // doors(block), levers (block), traps (block), hatches (OK, but block on down ramp)
            // closed floor grates (OK), closed floor bars (OK)
            if (is_wagon_dynamic_traversible(shape, pos) == false)
                return false;
            break;

        case tile_building_occ::None: // Not occupied by a building
            //FALLTHROUGH
        case tile_building_occ::Planned:
            //FALLTHROUGH
        case tile_building_occ::Passable:
            // Any tile with no building or a passable building including
            // beds, supports, rollers, armor/weapon stands, cages (not traps),
            // open wall grate/vertical bars, retracted bridges, open floodgates,
            // workshops (tiles with open space are handled by the tile check)
            if (is_wagon_tile_traversible(*tt) == false)
                return false;
            break;
        case tile_building_occ::Floored:
            // depot, lowered bridges or retractable bridges, forbidden hatches
            break;
        default:
            //NOTREACHED
            break;
    }

    if (ctx.wgroup == Maps::getWalkableGroup(pos))
        return true;

    // RAMP_TOP is assigned a walkability group if that commit is accepted
    // so this test, I think, would be useless.
    if (shape == df::tiletype_shape::RAMP_TOP ) {
        df::coord pos_below = pos + df::coord(0, 0, -1);
        if (Maps::getWalkableGroup(pos_below)) {
            ctx.search_edge.emplace(pos_below);
            return true;
        }
    } else if (shape == df::tiletype_shape::WALL) {
        auto prev_tt = Maps::getTileType(prev_pos);
        if (prev_tt && tileShape(*prev_tt) == df::tiletype_shape::RAMP) {
            df::coord pos_above = pos + df::coord(0, 0, 1);
            if (Maps::getWalkableGroup(pos_above)) {
                ctx.search_edge.emplace(pos_above);
                return true;
            }
        }
    }

    return false;
}

static bool is_wagon_traversible_by_ramp(FloodCtx & ctx, const df::coord & pos) {
    if ((is_wagon_traversible(ctx, pos+df::coord(-1, -1, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord( 0, -1, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord( 1, -1, 0), pos)) || // Top

            (is_wagon_traversible(ctx, pos+df::coord(-1,  0, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord( 0,  0, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord( 1,  0, 0), pos)) || // Middle

            (is_wagon_traversible(ctx, pos+df::coord(-1,  1, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord( 0,  1, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord( 1,  1, 0), pos)) || // Bottom

            (is_wagon_traversible(ctx, pos+df::coord(-1, -1, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord(-1,  0, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord(-1,  1, 0), pos)) || // Left

            (is_wagon_traversible(ctx, pos+df::coord( 1, -1, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord( 1,  0, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord( 1,  1, 0), pos))) { // Right
        return true;
    }
    return false;
}

static void check_wagon_tile(FloodCtx & ctx, const df::coord & pos) {
    if (ctx.seen.contains(pos))
        return;

    ctx.seen.emplace(pos);

    if (ctx.entry_tiles.contains(pos)) {
        ctx.wagon_path.emplace(pos); // Is this needed?
        ctx.search_edge.emplace(pos);
        return;
    }

    auto tt = Maps::getTileType(pos);
    if (!tt)
        return;

    auto shape = tileShape(*tt);
    if ((shape != df::tiletype_shape::RAMP_TOP &&
            (is_wagon_traversible(ctx, pos+df::coord(-1, -1, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord( 0, -1, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord( 1, -1, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord(-1,  0, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord( 1,  0, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord(-1,  1, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord( 0,  1, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord( 1,  1, 0), pos))) ||
            (shape == df::tiletype_shape::RAMP_TOP && is_wagon_traversible_by_ramp(ctx, pos))) {
        ctx.wagon_path.emplace(pos);
        ctx.search_edge.emplace(pos);
    }

#if 0
    // Use this if we don't mind red Xs on down ramps
    if ((is_wagon_traversible(ctx, pos+df::coord(-1, -1, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord( 0, -1, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord( 1, -1, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord(-1,  0, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord( 1,  0, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord(-1,  1, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord( 0,  1, 0), pos) &&
            is_wagon_traversible(ctx, pos+df::coord( 1,  1, 0), pos))) {
        ctx.wagon_path.emplace(pos);
        ctx.search_edge.emplace(pos);
    }
#endif

}

// returns true if a continuous 3-wide path can be found to an entry tile
// assumes:
// - wagons can only move in orthogonal directions
// - if three adjacent tiles are in the same pathability group, then they are traversible by a wagon
// - a wagon needs a single ramp to move elevations as long as the adjacent tiles are walkable
// TODO: cannot traverse doors, up stairs, or up/down stairs
static bool wagon_flood(color_ostream &out, unordered_set<df::coord> * wagon_path, const df::coord & depot_pos,
    const unordered_set<df::coord> & entry_tiles)
{
    unordered_set<df::coord> temp_wagon_path;
    FloodCtx ctx(Maps::getWalkableGroup(depot_pos), wagon_path ? *wagon_path : temp_wagon_path, entry_tiles);

    if (!ctx.wgroup)
        return false;

    bool found = false;
    ctx.wagon_path.emplace(depot_pos);
    ctx.seen.emplace(depot_pos);
    ctx.search_edge.emplace(depot_pos);
    while (!ctx.search_edge.empty()) {
        df::coord pos = ctx.search_edge.top();
        ctx.search_edge.pop();

        TRACE(log,out).print("checking tile: (%d, %d, %d); pathability group: %d\n", pos.x, pos.y, pos.z,
            Maps::getWalkableGroup(pos));

        // Ensure our wagon flood end points lands on an edge tile.
        // When there is no path to the depot entry_tiles will not
        // contain any edge tiles.
        if ((pos.x == 0 || pos.y == 0) && entry_tiles.contains(pos)) {
            found = true;
            if (!wagon_path)
                break;
            continue;
        }

        check_wagon_tile(ctx, pos+df::coord( 0, -1, 0));
        check_wagon_tile(ctx, pos+df::coord( 0,  1, 0));
        check_wagon_tile(ctx, pos+df::coord(-1,  0, 0));
        check_wagon_tile(ctx, pos+df::coord( 1,  0, 0));
    }

    return found;
}

static unordered_set<df::coord> wagon_path;
static unordered_set<df::coord> entry_tiles;

static bool getDepotAccessibleByWagons(color_ostream &out, bool cache_scan_for_painting) {
    if (cache_scan_for_painting) {
        entry_tiles.clear();
        wagon_path.clear();
    }
    unordered_set<df::coord> depot_coords;
    if (!get_depot_coords(out, &depot_coords))
        return false;
    unordered_set<uint16_t> depot_pathability_groups;
    if (!get_pathability_groups(out, &depot_pathability_groups, depot_coords))
        return false;
    if (!get_entry_tiles(&entry_tiles, depot_pathability_groups))
        return false;
    bool found_edge = false;
    for (auto depot_pos : depot_coords) {
        if (wagon_flood(out, cache_scan_for_painting ? &wagon_path : NULL, depot_pos, entry_tiles)) {
            found_edge = true;
            if (!cache_scan_for_painting)
                break;
        }
    }
    return found_edge;
}

static void paintScreenDepotAccess() {
    PaintCtx ctx;
    paint_screen(ctx, entry_tiles, false, [&](const df::coord & pos){
        return wagon_path.contains(pos);
    });
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(paintScreenPathable),
    DFHACK_LUA_FUNCTION(getDepotAccessibleByAnimals),
    DFHACK_LUA_FUNCTION(getDepotAccessibleByWagons),
    DFHACK_LUA_FUNCTION(paintScreenDepotAccess),
    DFHACK_LUA_END
};
