#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"

#include "modules/Buildings.h"
#include "modules/Maps.h"
#include "modules/Units.h"

#include "df/building.h"
#include "df/creature_raw.h"
#include "df/item.h"
#include "df/map_block.h"
#include "df/tile_occupancy.h"
#include "df/unit.h"
#include "df/world.h"

namespace DFHack
{
    DBG_DECLARE(fixoccupancy, log, DebugCategory::LINFO);
}

using namespace DFHack;
using std::string;
using std::vector;

DFHACK_PLUGIN("fix-occupancy");

REQUIRE_GLOBAL(world);

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    CoreSuspender suspend;

    bool show_help = false;
    if (!Lua::CallLuaModuleFunction(out, "plugins.fix-occupancy", "parse_commandline", std::make_tuple(parameters),
            1, [&](lua_State *L) {
                show_help = !lua_toboolean(L, -1);
            })) {
        return CR_FAILURE;
    }

    return show_help ? CR_WRONG_USAGE : CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream &out, vector<PluginCommand> &commands) {
    DEBUG(log, out).print("initializing %s\n", plugin_name);

    commands.push_back(PluginCommand(
        "fix/occupancy",
        "Fix phantom occupancy issues.",
        do_command));

    return CR_OK;
}

/////////////////////////////////////////////////////
// Lua API
//

static void normalize_item_vector(color_ostream &out, df::map_block *block, bool dry_run) {
    bool needs_sorting = false;
    int32_t prev_id = -1;
    for (auto item_id : block->items) {
        // do we need to handle duplicates as well?
        if (item_id < prev_id) {
            needs_sorting = true;
            break;
        }
        prev_id = item_id;
    }
    if (needs_sorting) {
        INFO(log,out).print("%s item list for map block at (%d, %d, %d)\n",
            dry_run ? "would fix" : "fixing", block->map_pos.x, block->map_pos.y, block->map_pos.z);
        if (!dry_run)
            std::sort(block->items.begin(), block->items.end());
    }
}

static void fix_tile(color_ostream &out, df::coord pos, bool dry_run) {
    auto occ = Maps::getTileOccupancy(pos);
    auto block = Maps::getTileBlock(pos);
    if (!occ || !block) {
        WARN(log,out).print("invalid tile: (%d, %d, %d)\n", pos.x, pos.y, pos.z);
        return;
    }

    // building occupancy
    size_t num_buildings = 0;
    if (occ->bits.building != df::tile_building_occ::None && !Buildings::findAtTile(pos)) {
        INFO(log,out).print("%s building occupancy at (%d, %d, %d)\n",
            dry_run ? "would fix" : "fixing", pos.x, pos.y, pos.z);
        ++num_buildings;
        if (!dry_run)
            occ->bits.building = df::tile_building_occ::None;
    }

    // check/fix unit occupancy
    size_t num_units = 0;
    if (occ->bits.unit || occ->bits.unit_grounded) {
        vector<df::unit *> units;
        Units::getUnitsInBox(units, pos.x, pos.y, pos.z, pos.x, pos.y, pos.z);
        bool found_standing_unit = false;
        bool found_grounded_unit = false;
        for (auto unit : units) {
            if (unit->flags1.bits.caged)
                continue;
            ++num_units;
            if (unit->flags1.bits.on_ground)
                found_grounded_unit = true;
            else
                found_standing_unit = true;
        }
        if (occ->bits.unit != found_standing_unit) {
            INFO(log,out).print("%s standing unit occupancy at (%d, %d, %d)\n",
                dry_run ? "would fix" : "fixing", pos.x, pos.y, pos.z);
            if (!dry_run)
                occ->bits.unit = found_standing_unit;
        }
        if (occ->bits.unit_grounded != found_grounded_unit) {
            INFO(log,out).print("%s grounded unit occupancy at (%d, %d, %d)\n",
                dry_run ? "would fix" : "fixing", pos.x, pos.y, pos.z);
            if (!dry_run)
                occ->bits.unit_grounded = found_grounded_unit;
        }
    }

    // check/fix order of map block item vector
    normalize_item_vector(out, block, dry_run);

    // scan for items at that position in the map block
    // this won't catch issues with items that are members of an incorrect map block. use fix_map for that.
    size_t num_items = 0;
    bool found_item = false;
    for (auto item_id : block->items) {
        auto item = df::item::find(item_id);
        if (!item)
            continue;
        if (!item->flags.bits.on_ground || item->pos != pos)
            continue;
        ++num_items;
        found_item = true;
        break;
    }

    // check/fix block occupancy
    if (occ->bits.item != found_item) {
        INFO(log,out).print("%s item occupancy at (%d, %d, %d)\n",
            dry_run ? "would fix" : "fixing", pos.x, pos.y, pos.z);
        if (!dry_run)
            occ->bits.item = found_item;
    }

    INFO(log,out).print("verified %zd building(s), %zd unit(s), %zd item(s), 1 map block(s), and 1 map tile(s)\n",
        num_buildings, num_units, num_items);
}

struct Expected {
private:
    int32_t dim_x, dim_y, dim_z;
    size_t size;
    df::tile_occupancy * occ_buf;
    std::unordered_map<df::map_block *, std::set<int32_t>> block_items_buf;

public:
    Expected() {
        Maps::getTileSize(dim_x, dim_y, dim_z);
        size = dim_x * dim_y * dim_z;
        occ_buf = (df::tile_occupancy *)calloc(size, sizeof(*occ_buf));
    }

    ~Expected() {
        free(occ_buf);
    }

    size_t get_size() const {
        return size;
    }

    df::tile_occupancy * occ(int32_t x, int32_t y, int32_t z) {
        size_t off = (dim_x * dim_y * z) + (dim_x * y) + x;
        if (off < size)
            return &occ_buf[off];
        return nullptr;
    }
    df::tile_occupancy * occ(const df::coord & pos) {
        return occ(pos.x, pos.y, pos.z);
    }

    std::set<int32_t> * block_items(df::map_block * block) {
        if (!block)
            return nullptr;
        return &block_items_buf[block];
    }
    std::set<int32_t> * block_items(int32_t x, int32_t y, int32_t z) {
        return block_items(Maps::getTileBlock(x, y, z));
    }
    std::set<int32_t> * block_items(const df::coord & pos) {
        return block_items(pos.x, pos.y, pos.z);
    }
};

static void reconcile_map_tile(color_ostream &out, df::tile_occupancy & expected_occ, df::tile_occupancy & block_occ,
    bool dry_run, int x, int y, int z)
{
    // clear building occupancy if there is no building there
    if (expected_occ.bits.building == df::tile_building_occ::None && block_occ.bits.building != df::tile_building_occ::None) {
        INFO(log,out).print("%s building occupancy at (%d, %d, %d)\n",
            dry_run ? "would fix" : "fixing", x, y, z);
        if (!dry_run)
            block_occ.bits.building = df::tile_building_occ::None;
    }

    // clear unit occupancy if there are no units there
    if (!expected_occ.bits.unit && block_occ.bits.unit) {
        INFO(log,out).print("%s standing unit occupancy at (%d, %d, %d)\n",
            dry_run ? "would fix" : "fixing", x, y, z);
        if (!dry_run)
            block_occ.bits.unit = false;
    }
    if (!expected_occ.bits.unit_grounded && block_occ.bits.unit_grounded) {
        INFO(log,out).print("%s grounded unit occupancy at (%d, %d, %d)\n",
            dry_run ? "would fix" : "fixing", x, y, z);
        if (!dry_run)
            block_occ.bits.unit_grounded = false;
    }

    // clear item occupancy if there are no items there
    if (!expected_occ.bits.item && block_occ.bits.item) {
        INFO(log,out).print("%s item occupancy at (%d, %d, %d)\n",
            dry_run ? "would fix" : "fixing", x, y, z);
        if (!dry_run)
            block_occ.bits.item = false;
    }
}

static void reconcile_block_items(color_ostream &out, std::set<int32_t> * expected_items, df::map_block * block, bool dry_run) {
    vector<int32_t> & block_items = block->items;

    if (!expected_items) {
        if (block_items.size()) {
            INFO(log,out).print("%s stale item references in map block at (%d, %d, %d)\n",
                dry_run ? "would fix" : "fixing", block->map_pos.x, block->map_pos.y, block->map_pos.z);
            if (!dry_run)
                block_items.resize(0);
        }
        return;
    }

    if (!std::equal(expected_items->begin(), expected_items->end(), block_items.begin(), block_items.end())) {
        INFO(log,out).print("%s stale item references in map block at (%d, %d, %d)\n",
            dry_run ? "would fix" : "fixing", block->map_pos.x, block->map_pos.y, block->map_pos.z);
        if (!dry_run) {
            block_items.resize(expected_items->size());
            std::copy(expected_items->begin(), expected_items->end(), block_items.begin());
        }
    }
}

static void fix_map(color_ostream &out, bool dry_run) {
    static const uint32_t occ_mask = df::tile_occupancy::mask_building | df::tile_occupancy::mask_unit |
        df::tile_occupancy::mask_unit_grounded | df::tile_occupancy::mask_item;

    Expected expected;

    // set expected building occupancy
    for (auto bld : world->buildings.all) {
        for (int y = bld->y1; y <= bld->y2; ++y) {
            for (int x = bld->x1; x <= bld->x2; ++x) {
                if (!Buildings::containsTile(bld, df::coord2d(x, y)))
                    continue;
                if (auto expected_occ = expected.occ(x, y, bld->z)) {
                    auto bld_occ = df::tile_building_occ::Impassable;
                    if (auto block_occ = Maps::getTileOccupancy(x, y, bld->z))
                        bld_occ = block_occ->bits.building;
                    expected_occ->bits.building = bld_occ;
                }
            }
        }
    }

    // set expected unit occupancy
    for (auto unit : world->units.active) {
        if (unit->flags1.bits.caged || unit->flags1.bits.inactive || unit->flags1.bits.rider)
            continue;
        if (auto craw = df::creature_raw::find(unit->race); craw && craw->flags.is_set(df::creature_raw_flags::EQUIPMENT_WAGON)) {
            for (int y = unit->pos.y - 1; y <= unit->pos.y + 1; ++y) {
                for (int x = unit->pos.x - 1; x <= unit->pos.x + 1; ++x) {
                    if (auto expected_occ = expected.occ(x, y, unit->pos.z)) {
                        expected_occ->bits.unit = true;
                    }
                }
            }
        }
        if (auto expected_occ = expected.occ(unit->pos)) {
            expected_occ->bits.unit = expected_occ->bits.unit || !unit->flags1.bits.on_ground;
            expected_occ->bits.unit_grounded = expected_occ->bits.unit_grounded || unit->flags1.bits.on_ground;
        }
    }

    // set expected item occupancy
    for (auto item : world->items.other.IN_PLAY) {
        if (!item->flags.bits.on_ground)
            continue;
        auto pos = Items::getPosition(item);
        if (auto block_items = expected.block_items(pos))
            block_items->emplace(item->id);
        if (auto expected_occ = expected.occ(pos))
            expected_occ->bits.item = true;
    }

    // check against expected values and fix
    for (auto block : world->map.map_blocks) {
        // check/fix order of map block item vector
        normalize_item_vector(out, block, dry_run);
        reconcile_block_items(out, expected.block_items(block), block, dry_run);

        // reconcile occupancy of each tile against expected values
        int z = block->map_pos.z;
        for (int yoff = 0; yoff < 16; ++yoff) {
            int y = block->map_pos.y + yoff;
            for (int xoff = 0; xoff < 16; ++xoff) {
                int x = block->map_pos.x + xoff;
                auto expected_occ = expected.occ(x, y, z);
                if (!expected_occ) {
                    TRACE(log,out).print("pos out of bounds (%d, %d, %d)\n", x, y, z);
                    continue;
                }
                df::tile_occupancy &block_occ = block->occupancy[xoff][yoff];
                if ((expected_occ->whole & occ_mask) != (block_occ.whole & occ_mask)) {
                    DEBUG(log,out).print("reconciling occupancy at (%d, %d, %d) (0x%x != 0x%x)\n",
                        x, y, z, expected_occ->whole & occ_mask, block_occ.whole & occ_mask);
                    reconcile_map_tile(out, *expected_occ, block_occ, dry_run, x, y, z);
                }
            }
        }
    }

    INFO(log,out).print("verified %zd buildings, %zd units, %zd items, %zd map blocks, and %zd map tiles\n",
        world->buildings.all.size(), world->units.active.size(), world->items.other.IN_PLAY.size(),
        world->map.map_blocks.size(), expected.get_size());
}

DFHACK_PLUGIN_LUA_FUNCTIONS{
    DFHACK_LUA_FUNCTION(fix_tile),
    DFHACK_LUA_FUNCTION(fix_map),
    DFHACK_LUA_END
};
