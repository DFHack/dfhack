#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "PluginLua.h"

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

DFhackCExport command_result plugin_init (color_ostream &out, vector<PluginCommand> &commands) {
    return CR_OK;
}

/////////////////////////////////////////////////////
// Lua API
//

struct Expected {
private:
    int32_t dim_x, dim_y, dim_z;
    size_t size;
    df::tile_occupancy * occ_buf;
    df::building ** bld_buf;
    std::unordered_map<df::map_block *, std::set<int32_t>> block_items_buf;

public:
    Expected() {
        Maps::getTileSize(dim_x, dim_y, dim_z);
        size = dim_x * dim_y * dim_z;
        occ_buf = (df::tile_occupancy *)calloc(size, sizeof(*occ_buf));
        bld_buf = (df::building **)calloc(size, sizeof(*bld_buf));
    }

    ~Expected() {
        free(occ_buf);
        free(bld_buf);
    }

    size_t get_size() const {
        return size;
    }

    df::tile_occupancy * occ(int32_t x, int32_t y, int32_t z) {
        size_t off = get_offset(x, y, z);
        if (off < size)
            return &occ_buf[off];
        return nullptr;
    }
    df::tile_occupancy * occ(const df::coord & pos) {
        return occ(pos.x, pos.y, pos.z);
    }

    df::building ** bld(int32_t x, int32_t y, int32_t z) {
        size_t off = get_offset(x, y, z);
        if (off < size)
            return &bld_buf[off];
        return nullptr;
    }
    df::building ** bld(const df::coord & pos) {
        return bld(pos.x, pos.y, pos.z);
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

private:
    size_t get_offset(int32_t x, int32_t y, int32_t z) {
        return (dim_x * dim_y * z) + (dim_x * y) + x;
    }
};

static void scan_building(color_ostream &out, df::building * bld, Expected & expected) {
    for (int y = bld->y1; y <= bld->y2; ++y) {
        for (int x = bld->x1; x <= bld->x2; ++x) {
            if (!Buildings::containsTile(bld, df::coord2d(x, y)))
                continue;
            auto expected_bld = expected.bld(x, y, bld->z);
            if (bld->isSettingOccupancy() && expected_bld) {
                if (*expected_bld) {
                    WARN(log,out).print("Buildings overlap at (%d, %d, %d); please manually remove overlapping building."
                        " Run ':lua dfhack.gui.revealInDwarfmodeMap(%d, %d, %d, true, true)' to zoom to the tile.\n",
                        x, y, bld->z, x, y, bld->z);
                }
                *expected_bld = bld;
            }
            if (auto expected_occ = expected.occ(x, y, bld->z)) {
                auto bld_occ = df::tile_building_occ::Impassable;
                if (auto block_occ = Maps::getTileOccupancy(x, y, bld->z))
                    bld_occ = block_occ->bits.building;
                expected_occ->bits.building = bld_occ;
            }
        }
    }
}

static void scan_unit(df::unit * unit, Expected & expected) {
    if (unit->flags1.bits.caged || unit->flags1.bits.inactive || unit->flags1.bits.rider)
        return;
    if (auto craw = df::creature_raw::find(unit->race); craw && craw->flags.is_set(df::creature_raw_flags::EQUIPMENT_WAGON)) {
        for (int y = unit->pos.y - 1; y <= unit->pos.y + 1; ++y) {
            for (int x = unit->pos.x - 1; x <= unit->pos.x + 1; ++x) {
                if (auto expected_occ = expected.occ(x, y, unit->pos.z)) {
                    expected_occ->bits.unit = expected_occ->bits.unit || !unit->flags1.bits.on_ground;
                    expected_occ->bits.unit_grounded = expected_occ->bits.unit_grounded || unit->flags1.bits.on_ground;
                }
            }
        }
    } else if (auto expected_occ = expected.occ(unit->pos)) {
        expected_occ->bits.unit = expected_occ->bits.unit || !unit->flags1.bits.on_ground;
        expected_occ->bits.unit_grounded = expected_occ->bits.unit_grounded || unit->flags1.bits.on_ground;
    }
}

static void scan_item(df::item * item, Expected & expected) {
    if (!item->flags.bits.on_ground)
        return;
    auto pos = Items::getPosition(item);
    if (auto block_items = expected.block_items(pos))
        block_items->emplace(item->id);
    if (auto expected_occ = expected.occ(pos))
        expected_occ->bits.item = true;
}

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

static void reconcile_map_tile(color_ostream &out, df::building * bld, const df::tile_occupancy & expected_occ,
    df::tile_occupancy & block_occ, bool dry_run, int x, int y, int z)
{
    // clear building occupancy if there is no building there
    if (expected_occ.bits.building == df::tile_building_occ::None && block_occ.bits.building != df::tile_building_occ::None) {
        INFO(log,out).print("%s building occupancy at (%d, %d, %d)\n",
            dry_run ? "would fix" : "fixing", x, y, z);
        if (!dry_run)
            block_occ.bits.building = df::tile_building_occ::None;
    }

    // recalculate bulding occupancy if there *is* a building there
    if (bld) {
        auto prev_occ = block_occ.bits.building;
        bld->updateOccupancy(x, y);
        // if this resets the occupancy to Dynamic, trust the original value
        if (block_occ.bits.building == df::tile_building_occ::Dynamic)
            block_occ.bits.building = prev_occ;
        else if (prev_occ != block_occ.bits.building) {
            INFO(log,out).print("%s building occupancy at (%d, %d, %d)\n",
                dry_run ? "would fix" : "fixing", x, y, z);
            if (dry_run)
                block_occ.bits.building = prev_occ;
        }
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

static void fix_tile(color_ostream &out, df::coord pos, bool dry_run) {
    auto occ = Maps::getTileOccupancy(pos);
    auto block = Maps::getTileBlock(pos);
    if (!occ || !block) {
        WARN(log,out).print("invalid tile: (%d, %d, %d)\n", pos.x, pos.y, pos.z);
        return;
    }

    Expected expected;

    // building occupancy (scan all buildings since we can't depend on Buildings::findAtTile)
    size_t num_buildings = 0;
    for (auto bld : world->buildings.all) {
        if (Buildings::containsTile(bld, pos)) {
            ++num_buildings;
            scan_building(out, bld, expected);
        }
    }

    // unit occupancy (make sure we pick up wagons that might be overlapping the tile)
    vector<df::unit *> units;
    Units::getUnitsInBox(units, pos.x-1, pos.y-1, pos.z, pos.x+1, pos.y+1, pos.z);
    size_t num_units = units.size();
    for (auto unit : units)
        scan_unit(unit, expected);

    // check/fix order of map block item vector
    normalize_item_vector(out, block, dry_run);

    // scan for items at that position in the map block
    // this won't catch issues with items that are members of an incorrect map block. use fix_map for that.
    size_t num_items = 0;
    for (auto item_id : block->items) {
        if (auto item = df::item::find(item_id)) {
            ++num_items;
            scan_item(item, expected);
        }
    }

    // check/fix occupancy
    auto expected_occ = expected.occ(pos);
    auto expected_bld = expected.bld(pos);
    if (expected_bld && expected_occ)
        reconcile_map_tile(out, *expected_bld, *expected_occ, block->occupancy[pos.x&15][pos.y&15], dry_run, pos.x, pos.y, pos.z);

    INFO(log,out).print("verified %zd building(s), %zd unit(s), %zd item(s), 1 map block(s), and 1 map tile(s)\n",
        num_buildings, num_units, num_items);
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
    for (auto bld : world->buildings.all)
        scan_building(out, bld, expected);

    // set expected unit occupancy
    for (auto unit : world->units.active)
        scan_unit(unit, expected);

    // set expected item occupancy
    for (auto item : world->items.other.IN_PLAY)
        scan_item(item, expected);

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
                auto expected_bld = expected.bld(x, y, z);
                if (!expected_occ || !expected_bld) {
                    TRACE(log,out).print("pos out of bounds (%d, %d, %d)\n", x, y, z);
                    continue;
                }
                df::tile_occupancy &block_occ = block->occupancy[xoff][yoff];
                if (*expected_bld || (expected_occ->whole & occ_mask) != (block_occ.whole & occ_mask)) {
                    DEBUG(log,out).print("reconciling occupancy at (%d, %d, %d) (bld=%p, 0x%x ?= 0x%x)\n",
                        x, y, z, *expected_bld, expected_occ->whole & occ_mask, block_occ.whole & occ_mask);
                    reconcile_map_tile(out, *expected_bld, *expected_occ, block_occ, dry_run, x, y, z);
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
