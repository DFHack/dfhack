#include <channel-manager.h>
#include <tile-cache.h>
#include <inlines.h>

#include <modules/EventManager.h> //hash function for df::coord
#include <df/block_square_event_designation_priorityst.h>


// sets mark flags as necessary, for all designations
void ChannelManager::manage_groups() {
    INFO(manager).print("manage_groups()\n");
    // make sure we've got a fort map to analyze
    if (World::isFortressMode() && Maps::IsValid()) {
        // iterate the groups we built/updated
        for (const auto &group: groups) {
            manage_group(group, true, has_any_groups_above(groups, group));
        }
    }
}

void ChannelManager::manage_group(const df::coord &map_pos, bool set_marker_mode, bool marker_mode) {
    INFO(manager).print("manage_group(" COORD ")\n ", COORDARGS(map_pos));
    if (!groups.count(map_pos)) {
        groups.scan_one(map_pos);
    }
    auto iter = groups.find(map_pos);
    if (iter != groups.end()) {
        manage_group(*iter, set_marker_mode, marker_mode);
    }
    INFO(manager).print("manage_group() is done\n");
}

void ChannelManager::manage_group(const Group &group, bool set_marker_mode, bool marker_mode) {
    INFO(manager).print("manage_group()\n");
    if (!set_marker_mode) {
        if (has_any_groups_above(groups, group)) {
            marker_mode = true;
        } else {
            marker_mode = false;
        }
    }
    for (auto &designation: group) {
        manage_one(group, designation, true, marker_mode);
    }
    INFO(manager).print("manage_group() is done\n");
}

bool ChannelManager::manage_one(const Group &group, const df::coord &map_pos, bool set_marker_mode, bool marker_mode) {
    if (Maps::isValidTilePos(map_pos)) {
        INFO(manager).print("manage_one(" COORD ")\n", COORDARGS(map_pos));
        df::map_block* block = Maps::getTileBlock(map_pos);
        // we calculate the position inside the block*
        df::coord local(map_pos);
        local.x = local.x % 16;
        local.y = local.y % 16;
        df::tile_occupancy &tile_occupancy = block->occupancy[Coord(local)];
        // ensure that we aren't on the top-most layers
        if (map_pos.z < mapz - 3) {
            // do we already know whether to set marker mode?
            if (set_marker_mode) {
                DEBUG(manager).print("  -> marker_mode\n");
                // if enabling marker mode, just do it
                if (marker_mode) {
                    tile_occupancy.bits.dig_marked = marker_mode;
                    return true;
                }
                // if activating designation, check if it is safe to dig or not a channel designation
                if (!is_channel_designation(block->designation[Coord(local)]) || is_safe_to_dig_down(map_pos)) {
                    if (!block->flags.bits.designated) {
                        block->flags.bits.designated = true;
                    }
                    tile_occupancy.bits.dig_marked = false;
                    TileCache::Get().cache(map_pos, block->tiletype[Coord(local)]);
                }
                return false;

            } else {
                // next search for the designation priority
                DEBUG(manager).print(" if(has_groups_above())\n");
                // check that the group has no incomplete groups directly above it
                if (has_group_above(groups, map_pos) || !is_safe_to_dig_down(map_pos)) {
                    DEBUG(manager).print("  has_groups_above: setting marker mode\n");
                    tile_occupancy.bits.dig_marked = true;
                    if (jobs.count(map_pos)) {
                        jobs.erase(map_pos);
                    }
                    WARN(manager).print(" <- manage_one() exits normally\n");
                    return true;
                }
            }
        } else {
            // if we are though, it should be totally safe to dig
            tile_occupancy.bits.dig_marked = false;
        }
        WARN(manager).print(" <- manage_one() exits normally\n");
    }
    return false;
}

void ChannelManager::mark_done(const df::coord &map_pos) {
    groups.remove(map_pos);
    jobs.erase(map_pos);
    CSP::dignow_queue.erase(map_pos);
    TileCache::Get().uncache(map_pos);
}
