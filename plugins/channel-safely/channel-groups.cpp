#include <channel-groups.h>
#include <tile-cache.h>
#include <inlines.h>
#include <modules/Maps.h>
#include <df/block_square_event_designation_priorityst.h>

#include <random>

// iterates the DF job list and adds channel jobs to the `jobs` container
void ChannelJobs::load_channel_jobs() {
    locations.clear();
    df::job_list_link* node = df::global::world->jobs.list.next;
    while (node) {
        df::job* job = node->item;
        node = node->next;
        if (is_channel_job(job)) {
            locations.emplace(job->pos);
            jobs.emplace(job->pos, job);
        }
    }
}

bool ChannelJobs::has_cavein_conditions(const df::coord &map_pos) {
    auto p = map_pos;
    auto ttype = *Maps::getTileType(p);
    if (!DFHack::isOpenTerrain(ttype)) {
        // check shared neighbour for cave-in conditions
        df::coord neighbours[4];
        get_connected_neighbours(map_pos, neighbours);
        int connectedness = 4;
        for (auto n: neighbours) {
            if (active.count(n) || DFHack::isOpenTerrain(*Maps::getTileType(n))) {
                connectedness--;
            }
        }
        if (!connectedness) {
            // do what?
            p.z--;
            ttype = *Maps::getTileType(p);
            if (DFHack::isOpenTerrain(ttype) || DFHack::isFloorTerrain(ttype)) {
                return true;
            }
        }
    }
    return false;
}

bool ChannelJobs::possible_cavein(const df::coord &job_pos) {
    for (auto iter : active) {
        if (iter == job_pos) continue;
        if (calc_distance(job_pos, iter) <= 2) {
            // find neighbours
            df::coord n1[8];
            df::coord n2[8];
            get_neighbours(job_pos, n1);
            get_neighbours(iter, n2);
            // find shared neighbours
            for (int i = 0; i < 7; ++i) {
                for (int j = i + 1; j < 8; ++j) {
                    if (n1[i] == n2[j]) {
                        if (has_cavein_conditions(n1[i])) {
                            WARN(jobs).print("Channel-Safely::jobs: Cave-in conditions detected at (" COORD ")\n", COORDARGS(n1[i]));
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

// adds map_pos to a group if an adjacent one exists, or creates one if none exist... if multiple exist they're merged into the first found
void ChannelGroups::add(const df::coord &map_pos) {
    // if we've already added this, we don't need to do it again
    if (groups_map.count(map_pos)) {
        return;
    }
    /* We need to add map_pos to an existing group if possible...
     * So what we do is we look at neighbours to see if they belong to one or more existing groups
     * If there is more than one group, we'll be merging them
     */
    df::coord neighbors[8];
    get_neighbours(map_pos, neighbors);
    Group* group = nullptr;
    int group_index = -1;

    DEBUG(groups).print("    add(" COORD ")\n", COORDARGS(map_pos));
    // and so we begin iterating the neighbours
    for (auto &neighbour: neighbors) {
        // go to the next neighbour if this one doesn't have a group
        if (!groups_map.count(neighbour)) {
            TRACE(groups).print(" -> neighbour is not designated\n");
            continue;
        }
        // get the group, since at least one exists... then merge any additional into that one
        if (!group){
            TRACE(groups).print(" -> group* has no valid state yet\n");
            group_index = groups_map.find(neighbour)->second;
            group = &groups.at(group_index);
        } else {
            TRACE(groups).print(" -> group* has an existing state\n");

            // we don't do anything if the found group is the same as the existing group
            auto index2 = groups_map.find(neighbour)->second;
            if (group_index != index2) {
                // we already have group "prime" if you will, so we're going to merge the new find into prime
                Group &group2 = groups.at(index2);
                // merge
                TRACE(groups).print(" -> merging two groups. group 1 size: %zu. group 2 size: %zu\n", group->size(),
                                    group2.size());
                for (auto pos2: group2) {
                    group->emplace(pos2);
                    groups_map[pos2] = group_index;
                }
                group2.clear();
                free_spots.emplace(index2);
                TRACE(groups).print("    merged size: %zu\n", group->size());
            }
        }
    }
    // if we haven't found at least one group by now we need to create/get one
    if (!group) {
        TRACE(groups).print(" -> no merging took place\n");
        // first we check if we can re-use a group that's been freed
        if (!free_spots.empty()) {
            TRACE(groups).print(" -> use recycled old group\n");
            // first element in a set is always the lowest value, so we re-use from the front of the vector
            group_index = *free_spots.begin();
            group = &groups[group_index];
            free_spots.erase(free_spots.begin());
        } else {
            TRACE(groups).print(" -> brand new group\n");
            // we create a brand-new group to use
            group_index = groups.size();
            groups.emplace_back();
            group = &groups[group_index];
        }
    }
    // puts the "add" in "ChannelGroups::add"
    group->emplace(map_pos);
    DEBUG(groups).print(" = group[%d] of (" COORD ") is size: %zu\n", group_index, COORDARGS(map_pos), group->size());

    // we may have performed a merge, so we update all the `coord -> group index` mappings
    for (auto &wpos: *group) {
        groups_map[wpos] = group_index;
    }
    DEBUG(groups).print(" <- add() exits, there are %zu mappings\n", groups_map.size());
}

// scans a single tile for channel designations
void ChannelGroups::scan_one(const df::coord &map_pos) {
    df::map_block* block = Maps::getTileBlock(map_pos);
    int16_t lx = map_pos.x % 16;
    int16_t ly = map_pos.y % 16;
    if (is_dig_designation(block->designation[lx][ly])) {
        for (df::block_square_event* event: block->block_events) {
            if (auto evT = virtual_cast<df::block_square_event_designation_priorityst>(event)) {
                // we want to let the user keep some designations free of being managed
                if (evT->priority[lx][ly] < 1000 * config.ignore_threshold) {
                    TRACE(groups).print("   adding (" COORD ")\n", COORDARGS(map_pos));
                    add(map_pos);
                }
            }
        }
    } else if (TileCache::Get().hasChanged(map_pos, block->tiletype[lx][ly])) {
        TileCache::Get().uncache(map_pos);
        remove(map_pos);
    }
}

// builds groupings of adjacent channel designations
void ChannelGroups::scan(bool full_scan) {
    static std::default_random_engine RNG(0);
    static std::bernoulli_distribution sometimes_scanFULLY(0.15);
    if (!full_scan) {
        full_scan = sometimes_scanFULLY(RNG);
    }

    // save current jobs, then clear and load the current jobs
    std::set<df::coord> last_jobs;
    for (auto &pos : jobs) {
        last_jobs.emplace(pos);
    }
    jobs.load_channel_jobs();
    // transpose channel jobs to
    std::set<df::coord> new_jobs;
    std::set<df::coord> gone_jobs;
    set_difference(last_jobs, jobs, gone_jobs);
    set_difference(jobs, last_jobs, new_jobs);
    INFO(groups).print("gone jobs: %zd\nnew jobs: %zd\n",gone_jobs.size(), new_jobs.size());
    for (auto &pos : new_jobs) {
        add(pos);
    }
    for (auto &pos : gone_jobs){
        remove(pos);
    }

    DEBUG(groups).print("  scan()\n");
    // foreach block
    for (int32_t z = mapz - 1; z >= 0; --z) {
        for (int32_t by = 0; by < mapy; ++by) {
            for (int32_t bx = 0; bx < mapx; ++bx) {
                // the block
                if (df::map_block* block = Maps::getBlock(bx, by, z)) {
                    // skip this block?
                    if (!full_scan && !block->flags.bits.designated) {
                        continue;
                    }
                    df::map_block* block_above = Maps::getBlock(bx, by, z+1);
                    // foreach tile
                    bool empty_group = true;
                    for (int16_t lx = 0; lx < 16; ++lx) {
                        for (int16_t ly = 0; ly < 16; ++ly) {
                            // the tile, check if it has a channel designation
                            df::coord map_pos((bx * 16) + lx, (by * 16) + ly, z);
                            if (TileCache::Get().hasChanged(map_pos, block->tiletype[lx][ly])) {
                                TileCache::Get().uncache(map_pos);
                                remove(map_pos);
                                if (jobs.count(map_pos)) {
                                    jobs.erase(map_pos);
                                }
                                block->designation[lx][ly].bits.dig = df::tile_dig_designation::No;
                            } else if (is_dig_designation(block->designation[lx][ly]) || block->occupancy[lx][ly].bits.dig_marked ) {
                                // We have a dig designated, or marked. Some of these will not need intervention.
                                if (block_above &&
                                    !is_channel_designation(block->designation[lx][ly]) &&
                                    !is_channel_designation(block_above->designation[lx][ly])) {
                                    // if this tile isn't a channel designation, and doesn't have a channel designation above it.. we can skip it
                                    continue;
                                }
                                for (df::block_square_event* event: block->block_events) {
                                    if (auto evT = virtual_cast<df::block_square_event_designation_priorityst>(event)) {
                                        // we want to let the user keep some designations free of being managed
                                        TRACE(groups).print("   tile designation priority: %d\n", evT->priority[lx][ly]);
                                        if (evT->priority[lx][ly] < 1000 * config.ignore_threshold) {
                                            if (empty_group) {
                                                group_blocks.emplace(block);
                                                empty_group = false;
                                            }
                                            TRACE(groups).print("   adding (" COORD ")\n", COORDARGS(map_pos));
                                            add(map_pos);
                                        } else if (groups_map.count(map_pos)) {
                                            remove(map_pos);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    // erase the block if we didn't find anything iterating through it
                    if (empty_group) {
                        group_blocks.erase(block);
                    }
                }
            }
        }
    }
    INFO(groups).print("scan() exits\n");
}

// clears out the containers for unloading maps or disabling the plugin
void ChannelGroups::clear() {
    debug_map();
    WARN(groups).print(" <- clearing groups\n");
    jobs.clear();
    group_blocks.clear();
    free_spots.clear();
    groups_map.clear();
    for(size_t i = 0; i < groups.size(); ++i) {
        groups[i].clear();
        free_spots.emplace(i);
    }
}

// erases map_pos from its group, and deletes mappings IFF the group is empty
void ChannelGroups::remove(const df::coord &map_pos) {
    // we don't need to do anything if the position isn't in a group (granted, that should never be the case)
    INFO(groups).print(" remove()\n");
    if (groups_map.count(map_pos)) {
        INFO(groups).print(" -> found group\n");
        // get the group, and map_pos' block*
        int group_index = groups_map.find(map_pos)->second;
        Group &group = groups[group_index];
        // erase map_pos from the group
        INFO(groups).print(" -> erase(" COORD ")\n", COORDARGS(map_pos));
        group.erase(map_pos);
        groups_map.erase(map_pos);
        // clean up if the group is empty
        if (group.empty()) {
            WARN(groups).print(" -> group is empty\n");
            // erase `coord -> group group_index` mappings
            for (auto iter = groups_map.begin(); iter != groups_map.end();) {
                if (group_index == iter->second) {
                    iter = groups_map.erase(iter);
                    continue;
                }
                ++iter;
            }
            // flag the `groups` group_index as available
            free_spots.insert(group_index);
        }
    }
    INFO(groups).print(" remove() exits\n");
}

// finds a group corresponding to a map position if one exists
Groups::const_iterator ChannelGroups::find(const df::coord &map_pos) const {
    const auto iter = groups_map.find(map_pos);
    if (iter != groups_map.end()) {
        return groups.begin() + iter->second;
    }
    return groups.end();
}

// returns an iterator to the first element stored
Groups::const_iterator ChannelGroups::begin() const {
    return groups.begin();
}

// returns an iterator to after the last element stored
Groups::const_iterator ChannelGroups::end() const {
    return groups.end();
}

// returns a count of 0 or 1 depending on whether map_pos is mapped to a group
size_t ChannelGroups::count(const df::coord &map_pos) const {
    return groups_map.count(map_pos);
}

// prints debug info about the groups stored, and their members
void ChannelGroups::debug_groups() {
    if (DFHack::debug_groups.isEnabled(DebugCategory::LDEBUG)) {
        int idx = 0;
        DEBUG(groups).print(" debugging group data\n");
        for (auto &group: groups) {
            DEBUG(groups).print("  group %d (size: %zu)\n", idx, group.size());
            for (auto &pos: group) {
                DEBUG(groups).print("   (%d,%d,%d)\n", pos.x, pos.y, pos.z);
            }
            idx++;
        }
    }
}

// prints debug info group mappings
void ChannelGroups::debug_map() {
    if (DFHack::debug_groups.isEnabled(DebugCategory::LTRACE)) {
        INFO(groups).print("Group Mappings: %zu\n", groups_map.size());
        for (auto &pair: groups_map) {
            TRACE(groups).print(" map[" COORD "] = %d\n", COORDARGS(pair.first), pair.second);
        }
    }
}
