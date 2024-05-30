#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"

#include "modules/Buildings.h"
#include "modules/Maps.h"
#include "modules/Units.h"

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
    if (occ->bits.building != df::tile_building_occ::None && !Buildings::findAtTile(pos)) {
        INFO(log,out).print("%s building occupancy at (%d, %d, %d)\n",
            dry_run ? "would fix" : "fixing", pos.x, pos.y, pos.z);
        if (!dry_run)
            occ->bits.building = df::tile_building_occ::None;
    }

    // check/fix unit occupancy
    if (occ->bits.unit || occ->bits.unit_grounded) {
        vector<df::unit *> units;
        Units::getUnitsInBox(units, pos.x, pos.y, pos.z, pos.x, pos.y, pos.z);
        bool found_standing_unit = false;
        bool found_grounded_unit = false;
        for (auto unit : units) {
            if (unit->flags1.bits.caged)
                continue;
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
    bool found_item = false;
    for (auto item_id : block->items) {
        auto item = df::item::find(item_id);
        if (!item)
            continue;
        if (!item->flags.bits.on_ground || item->pos != pos)
            continue;
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
}

static void fix_map_items(color_ostream &out, bool dry_run) {
    /*
        local cnt = 0
    local icnt = 0
    local found = {}
    local found_somewhere = {}

    local should_fix = false
    local can_fix = true

    for _,block in ipairs(df.global.world.map.map_blocks) do
        local itable = {}
        local bx,by,bz = pos2xyz(block.map_pos)

        -- Scan the block item vector
        local last_id = nil
        local resort = false

        for _,id in ipairs(block.items) do
            local item = df.item.find(id)
            local ix,iy,iz = pos2xyz(item.pos)
            local dx,dy,dz = ix-bx,iy-by,iz-bz

            -- Check sorted order
            if last_id and last_id >= id then
                print(bx,by,bz,last_id,id,'block items not sorted')
                resort = true
            else
                last_id = id
            end

            -- Check valid coordinates and flags
            if not item.flags.on_ground then
                print(bx,by,bz,id,dx,dy,'in block & not on ground')
            elseif dx < 0 or dx >= 16 or dy < 0 or dy >= 16 or dz ~= 0 then
                found_somewhere[id] = true
                print(bx,by,bz,id,dx,dy,dz,'invalid pos')
                can_fix = false
            else
                found[id] = true
                itable[dx + dy*16] = true;

                -- Check missing occupancy
                if not block.occupancy[dx][dy].item then
                    print(bx,by,bz,dx,dy,'item & not occupied')
                    if fix then
                        block.occupancy[dx][dy].item = true
                    else
                        should_fix = true
                    end
                end
            end
        end

        -- Sort the vector if needed
        if resort then
            if fix then
                utils.sort_vector(block.items)
            else
                should_fix = true
            end
        end

        icnt = icnt + #block.items

        -- Scan occupancy for spurious marks
        for x=0,15 do
            local ocx = block.occupancy[x]
            for y=0,15 do
                if ocx[y].item and not itable[x + y*16] then
                    print(bx,by,bz,x,y,'occupied & no item')
                    if fix then
                        ocx[y].item = false
                    else
                        should_fix = true
                    end
                end
            end
        end

        cnt = cnt + 256
    end

    -- Check if any items are missing from blocks
    for _,item in ipairs(df.global.world.items.other.IN_PLAY) do
        if item.flags.on_ground and not found[item.id] then
            can_fix = false
            if not found_somewhere[item.id] then
                print(item.id,item.pos.x,item.pos.y,item.pos.z,'on ground & not in block')
            end
        end
    end

    -- Report
    print(cnt.." tiles and "..icnt.." items checked.")

    if should_fix and can_fix then
        print("Use 'fix/item-occupancy --fix' to fix the listed problems.")
    elseif should_fix then
        print("The problems are too severe to be fixed by this script.")
    end
    */
}

struct OccBuf {
private:
    int32_t dim_x, dim_y, dim_z;
    size_t size;
    df::tile_occupancy * buf;

public:
    OccBuf() {
        Maps::getTileSize(dim_x, dim_y, dim_z);
        size = dim_x * dim_y * dim_z;
        buf = (df::tile_occupancy *)calloc(size, sizeof(*buf));
    }

    ~OccBuf() {
        free(buf);
    }

    df::tile_occupancy * occ(int32_t x, int32_t y, int32_t z) {
        size_t off = (dim_x * dim_y * z) + (dim_x * y) + x;
        if (off < size)
            return &buf[off];
        return nullptr;
    }
};

static void reconcile_map_tile(color_ostream &out, df::tile_occupancy & expected_occ, df::tile_occupancy & block_occ, bool dry_run) {
    // clear building occupancy if there is no building there
    if (expected_occ.bits.building == df::tile_building_occ::None && block_occ.bits.building != df::tile_building_occ::None) {
        INFO(log,out).print("%s building occupancy at (%d, %d, %d)\n",
            dry_run ? "would fix" : "fixing", x, y, z);
        if (!dry_run)
            block_occ.bits.building = df::tile_building_occ::None;
    }

    // check/fix unit occupancy
    if (occ->bits.unit || occ->bits.unit_grounded) {
        vector<df::unit *> units;
        Units::getUnitsInBox(units, pos.x, pos.y, pos.z, pos.x, pos.y, pos.z);
        bool found_standing_unit = false;
        bool found_grounded_unit = false;
        for (auto unit : units) {
            if (unit->flags1.bits.caged)
                continue;
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

    // check/fix block occupancy
    if (occ->bits.item != found_item) {
        INFO(log,out).print("%s item occupancy at (%d, %d, %d)\n",
            dry_run ? "would fix" : "fixing", pos.x, pos.y, pos.z);
        if (!dry_run)
            occ->bits.item = found_item;
    }

}

static void fix_map(color_ostream &out, bool dry_run) {
    OccBuf occ_buf;

    // set expected building occupancy
    for (auto bld : world->buildings.all) {

    }

    // set expected unit occupancy
    for (auto unit : world->units.active) {

    }

    // set expected item occupancy
    for (auto item : world->items.other.IN_PLAY) {

    }

    // check against expected occupancy and fix
    for (auto block : world->map.map_blocks) {
        // check/fix order of map block item vector
        normalize_item_vector(out, block, dry_run);

        // reconcile occupancy of each tile against expected values
        int z = block->map_pos.z;
        for (int yoff = 0; yoff < 16; ++yoff) {
            int y = block->map_pos.y + yoff;
            for (int xoff = 0; xoff < 16; ++xoff) {
                int x = block->map_pos.x + xoff;
                auto expected_occ = occ_buf.occ(x, y, z);
                if (!expected_occ) {
                    TRACE(log,out).print("pos out of bounds (%d, %d, %d)\n", x, y, z);
                    continue;
                }
                reconcile_map_tile(out, *expected_occ, block->occupancy[x][y], dry_run);
            }
        }
    }

    // check/fix item membership in block item vector
    bool found_item = false;
    for (auto item : world->items.other.IN_PLAY) {
        if (!item->flags.bits.on_ground || item->pos != pos)
            continue;
        found_item = true;
        if (!dry_run) {
            bool inserted = false;
            insert_into_vector(block->items, item->id, &inserted);
            if (inserted) {
                INFO(log,out).print("fixing item membership in map block item list at (%d, %d, %d)\n",
                    pos.x, pos.y, pos.z);
            }
        } else if (!vector_contains(block->items, item->id)) {
            INFO(log,out).print("would fix item membership in map block item list at (%d, %d, %d)\n",
                pos.x, pos.y, pos.z);
        }
    }

}

DFHACK_PLUGIN_LUA_FUNCTIONS{
    DFHACK_LUA_FUNCTION(fix_tile),
    DFHACK_LUA_FUNCTION(fix_map),
    DFHACK_LUA_END
};

/*

#include "modules/Maps.h"
#include "modules/Units.h"
#include "modules/Translation.h"
#include "modules/World.h"

#include "df/creature_raw.h"
#include "df/map_block.h"
#include "df/unit.h"
#include "df/world.h"

unsigned fix_unit_occupancy (color_ostream &out, uo_opts &opts)
{
    if (!Core::getInstance().isMapLoaded())
        return 0;

    if (!World::isFortressMode() && !opts.use_cursor)
    {
        out.printerr("Can only scan entire map in fortress mode\n");
        return 0;
    }

    if (opts.use_cursor && cursor->x < 0)
    {
        out.printerr("No cursor\n");
        return 0;
    }

    uo_buffer.resize();
    unsigned count = 0;

    float time1 = getClock();
    for (size_t i = 0; i < world->map.map_blocks.size(); i++)
    {
        df::map_block *block = world->map.map_blocks[i];
        int map_z = block->map_pos.z;
        if (opts.use_cursor && (map_z != cursor->z || block->map_pos.y != (cursor->y / 16) * 16 || block->map_pos.x != (cursor->x / 16) * 16))
            continue;
        for (int x = 0; x < 16; x++)
        {
            int map_x = x + block->map_pos.x;
            for (int y = 0; y < 16; y++)
            {
                if (block->designation[x][y].bits.hidden)
                    continue;
                int map_y = y + block->map_pos.y;
                if (opts.use_cursor && (map_x != cursor->x || map_y != cursor->y))
                    continue;
                if (block->occupancy[x][y].bits.unit)
                    uo_buffer.set(map_x, map_y, map_z, 1);
            }
        }
    }

    for (auto it = world->units.active.begin(); it != world->units.active.end(); ++it)
    {
        df::unit *u = *it;
        if (!u || u->flags1.bits.caged || u->pos.x < 0)
            continue;
        df::creature_raw *craw = df::creature_raw::find(u->race);
        int unit_extents = (craw && craw->flags.is_set(df::creature_raw_flags::EQUIPMENT_WAGON)) ? 1 : 0;
        for (int16_t x = u->pos.x - unit_extents; x <= u->pos.x + unit_extents; ++x)
        {
            for (int16_t y = u->pos.y - unit_extents; y <= u->pos.y + unit_extents; ++y)
            {
                uo_buffer.set(x, y, u->pos.z, 0);
            }
        }
    }

    for (size_t i = 0; i < uo_buffer.size; i++)
    {
        if (uo_buffer.buf[i])
        {
            uint32_t x, y, z;
            uo_buffer.get_coords(i, x, y, z);
            out.print("(%u, %u, %u) - no unit found\n", x, y, z);
            ++count;
            if (!opts.dry_run)
            {
                df::map_block *b = Maps::getTileBlock(x, y, z);
                b->occupancy[x % 16][y % 16].bits.unit = false;
            }
        }
    }

    float time2 = getClock();
    std::cerr << "fix-unit-occupancy: elapsed time: " << time2 - time1 << " secs" << endl;
    if (count)
        out << (opts.dry_run ? "[dry run] " : "") << "Fixed occupancy of " << count << " tiles [fix-unit-occupancy]" << endl;
    return count;
}

*/