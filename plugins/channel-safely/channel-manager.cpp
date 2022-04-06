#include "channel-manager.h"
#include "inlines.h"
#include <df/block_square_event_designation_priorityst.h>
#include <LuaTools.h>
#include <LuaWrapper.h>


extern color_ostream* debug_out;
extern bool cheat_mode;

// executes dig designations for the specified tile coordinates
static bool dig_now_tile(color_ostream &out, const df::coord &map_pos) {
    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!lua_checkstack(L, 2) ||
        !Lua::PushModulePublic(out, L, "plugins.dig-now", "dig_now_tile"))
        return false;

    Lua::Push(L, map_pos);

    if (!Lua::SafeCall(out, L, 1, 1))
        return false;

    return lua_toboolean(L, -1);

}

// sets mark flags as necessary, for all designations
void ChannelManager::manage_all(color_ostream &out) {
    if (debug_out) debug_out->print("manage_all()\n");
    // make sure we've got a fort map to analyze
    if (World::isFortressMode() && Maps::IsValid()) {
        // read map / analyze designations
        build_groups();
        // iterate the groups we built/updated
        for (const auto &group : groups) {
            if (debug_out) debug_out->print("foreach group\n");
            // iterate the members of each group (designated tiles)
            for (auto &designation : group) {
                if (debug_out) debug_out->print("foreach tile\n");
                // each tile has a position and a block*
                const df::coord &tile_pos = designation.first;
                df::map_block* block = designation.second;
                // check the safety
                manage_one(out, group, tile_pos, block);
            }
        }
        if (debug_out) debug_out->print("done managing designations\n");
    }
}

// sets mark flags as necessary, for a single designation
void ChannelManager::manage_one(color_ostream &out, const df::coord &map_pos, df::map_block* block, bool manage_neighbours) {
    if (!groups.count(map_pos)) {
        build_groups();
    }
    auto iter = groups.find(map_pos);
    if (iter != groups.end()) {
        manage_one(out, *iter, map_pos, block);
        // only if asked, and if map_pos was found mapped to a group do we want to manage neighbours
        if (manage_neighbours) {
            // todo: how necessary is this?
            this->manage_neighbours(out, map_pos);
        }
    }
}
void ChannelManager::manage_one(color_ostream &out, const Group &group, const df::coord &map_pos, df::map_block* block) {
    // we calculate the position inside the block*
    df::coord local(map_pos);
    local.x = local.x % 16;
    local.y = local.y % 16;
    df::tile_occupancy &tile_occupancy = block->occupancy[local.x][local.y];
    // ensure that we aren't on the top-most layer
    if(map_pos.z < mapz - 1) {
        // next search for the designation priority
        for (df::block_square_event* event : block->block_events) {
            if (auto evT = virtual_cast<df::block_square_event_designation_priorityst>(event)) {
                // todo: should we revise this check and disregard priority 1/2 or some such?
                // we want to let the user keep some designations free of being managed
                if (evT->priority[local.x][local.y] < 6000) {
                    if (debug_out) debug_out->print("if(has_groups_above())\n");
                    // check that the group has no incomplete groups directly above it
                    if (!has_groups_above(groups, group)) {
                        tile_occupancy.bits.dig_marked = false;
                        block->flags.bits.designated = true;
                    } else {
                        tile_occupancy.bits.dig_marked = true;
                        jobs.erase_and_cancel(map_pos); //cancels job if designation is an open/active job
                        // is it permanently unsafe? and is it safe to instantly dig the group of tiles
                        if (cheat_mode && !is_safe_to_dig_down(map_pos) && !is_group_occupied(groups, group)) {
                            tile_occupancy.bits.dig_marked = false;
                            dig_now_tile(out, map_pos);
                        }
                    }
                }
            }
        }
    } else {
        // if we are though, it should be totally safe to dig
        tile_occupancy.bits.dig_marked = false;
    }
}

// sets mark flags as necessary, for a neighbourhood
void ChannelManager::manage_neighbours(color_ostream &out, const df::coord &map_pos) {
    // todo: when do we actually need to manage neighbours
    df::coord neighbours[8];
    get_neighbours(map_pos, neighbours);
    // for all the neighbours to our tile
    for (auto &position : neighbours) {
        // we need to ensure the position is still within the bounds of the map
        if (Maps::isValidTilePos(position)) {
            manage_one(out, position, Maps::getTileBlock(position));
        }
    }
}
