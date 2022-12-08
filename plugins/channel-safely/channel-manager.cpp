#include <channel-manager.h>
#include <tile-cache.h>
#include <inlines.h>

#include <modules/EventManager.h> //hash function for df::coord
#include <df/block_square_event_designation_priorityst.h>

#include <algorithm>
#include <random>

df::unit* find_dwarf(const df::coord &map_pos) {
    df::unit* nearest = nullptr;
    uint32_t distance;
    for (auto unit : df::global::world->units.active) {
        if (!nearest) {
            nearest = unit;
            distance = calc_distance(unit->pos, map_pos);
        } else if (unit->status.labors[df::unit_labor::MINE]) {
            uint32_t d = calc_distance(unit->pos, map_pos);
            if (d < distance) {
                nearest = unit;
                distance = d;
            } else if (Maps::canWalkBetween(unit->pos, map_pos)) {
                return unit;
            }
        }
    }
    return nearest;
}

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
        marker_mode = has_any_groups_above(groups, group);
    }
    // cavein prevention
    bool cavein_possible = false;
    uint8_t least_access = 100;
    std::unordered_map<df::coord, uint8_t> cavein_candidates;
    if (!marker_mode) {
        /* To prevent cave-ins we're looking at accessibility of tiles with open space below them
         * If it has space below, it has somewhere to fall
         * Accessibility tells us how close to a cave-in a tile is, low values are at risk of cave-ins
         * To count access, we find a random miner dwarf and count how many tile neighbours they can path to
         * */
        // find a dwarf to path from
        df::coord miner_pos = find_dwarf(*group.begin())->pos;

        // Analyze designations
        for (const auto &pos: group) {
            df::coord below(pos);
            below.z--;
            const auto below_ttype = *Maps::getTileType(below);
            // we can skip designations already queued for insta-digging
            if (CSP::dignow_queue.count(pos)) continue;
            if (DFHack::isOpenTerrain(below_ttype) || DFHack::isFloorTerrain(below_ttype)) {
                // the tile below is not solid earth
                // we're counting accessibility then dealing with 0 access
                DEBUG(manager).print("analysis: cave-in condition found\n");
                INFO(manager).print("(%d) checking accessibility of (" COORD ") from (" COORD ")\n", cavein_possible,COORDARGS(pos),COORDARGS(miner_pos));
                auto access = count_accessibility(miner_pos, pos);
                if (access == 0) {
                    TRACE(groups).print(" has 0 access\n");
                    if (config.insta_dig) {
                        manage_one(pos, true, false);
                        dig_now(DFHack::Core::getInstance().getConsole(), pos);
                        mark_done(pos);
                    } else {
                        // todo: engage dig management, swap channel designations for dig
                    }
                } else {
                    WARN(manager).print(" has %d access\n", access);
                    cavein_possible = config.riskaverse;
                    cavein_candidates.emplace(pos, access);
                    least_access = min(access, least_access);
                }
            } else if (config.insta_dig && isEntombed(miner_pos, pos)) {
                manage_one(pos, true, false);
                dig_now(DFHack::Core::getInstance().getConsole(), pos);
                mark_done(pos);
            }
        }
        DEBUG(manager).print("cavein possible(%d)\n"
                           "%zu candidates\n"
                           "least access %d\n", cavein_possible, cavein_candidates.size(), least_access);
    }
    // managing designations
    if (!group.empty()) {
        DEBUG(manager).print("managing group #%d\n", groups.debugGIndex(*group.begin()));
    }
    for (auto &pos: group) {
        // if no cave-in is possible [or we don't check for], we'll just execute normally and move on
        if (!cavein_possible) {
            TRACE(manager).print("cave-in evaluated false\n");
            assert(manage_one(pos, true, marker_mode));
            continue;
        }
        // cavein is only possible if marker_mode is false
        const static uint8_t MAX = 84; //arbitrary number that indicates the value has changed
        if (CSP::dignow_queue.count(pos) || (cavein_candidates.count(pos) &&
            least_access < MAX && cavein_candidates[pos] <= least_access+2)) {

            TRACE(manager).print("cave-in evaluated true and either of dignow or (%d <= %d)\n", cavein_candidates[pos], least_access+2);
            // we want to dig the cavein candidates first
            df::coord local(pos);
            local.x %= 16;
            local.y %= 16;
            auto block = Maps::ensureTileBlock(pos);
            for (df::block_square_event* event: block->block_events) {
                if (auto evT = virtual_cast<df::block_square_event_designation_priorityst>(event)) {
                    // we want to let the user keep some designations free of being managed
                    auto b = max(0, cavein_candidates[pos] - least_access);
                    auto v = 1000 + (b * 1700);
                    DEBUG(manager).print("(" COORD ") 1000+1000(%d) -> %d {least-access: %d}\n",COORDARGS(pos), b, v, least_access);
                    evT->priority[Coord(local)] = v;
                }
            }
            assert(manage_one(pos, true, false));
            continue;
        }
        if (cavein_candidates.count(pos)) {
            DEBUG(manager).print("cave-in evaluated true and no dignow and (%d > %d)\n", cavein_candidates[pos], least_access + 2);
        } else {
            DEBUG(manager).print("cave-in evaluated true and no dignow and pos is not a candidate\n");
        }
        assert(manage_one(pos, true, true));
    }
    INFO(manager).print("manage_group() is done\n");
}

bool ChannelManager::manage_one(const df::coord &map_pos, bool set_marker_mode, bool marker_mode) {
    if (Maps::isValidTilePos(map_pos)) {
        TRACE(manager).print("manage_one((" COORD "), %d, %d)\n", COORDARGS(map_pos), set_marker_mode, marker_mode);
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
                TRACE(manager).print("  -> setting marker mode\n");
                // if enabling marker mode, just do it
                if (marker_mode) {
                    goto markerMode;
                }
                // otherwise we check for some safety
                if (!is_channel_designation(block->designation[Coord(local)]) || is_safe_to_dig_down(map_pos)) {
                    // not a channel designation, or it is safe to dig down if it is
                    if (!block->flags.bits.designated) {
                        block->flags.bits.designated = true;
                    }
                    // we need to cache the tile now that we're activating the designation
                    TileCache::Get().cache(map_pos, block->tiletype[Coord(local)]);
                    TRACE(manager).print("marker mode DISABLED\n");
                    tile_occupancy.bits.dig_marked = false;
                } else {
                    // we want to activate the designation, that's not what we get
                    goto markerMode;
                }
            } else {
                // not set_marker_mode
                TRACE(manager).print(" if(has_groups_above())\n");
                // check that the group has no incomplete groups directly above it
                if (has_group_above(groups, map_pos) || !is_safe_to_dig_down(map_pos)) {

                    markerMode:

                    TRACE(manager).print("marker mode ENABLED\n");
                    tile_occupancy.bits.dig_marked = true;
                    if (jobs.count(map_pos)) {
                        cancel_job(map_pos);
                    }
                }
            }
        } else {
            // if we are though, it should be totally safe to dig
            tile_occupancy.bits.dig_marked = false;
        }
        TRACE(manager).print(" <- manage_one() exits normally\n");
        return true;
    }
    return false;
}

void ChannelManager::mark_done(const df::coord &map_pos) {
    groups.remove(map_pos);
    jobs.erase(map_pos);
    CSP::dignow_queue.erase(map_pos);
    TileCache::Get().uncache(map_pos);
}
