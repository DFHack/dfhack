#include "channel-groups.h"
#include "inlines.h"
#include <modules/Maps.h>
//#include <df/world.h>
#include <df/job.h>

extern color_ostream* debug_out;

// scans the map for channel designations
void ChannelGroups::scan_map() {
    // foreach block
    for (int32_t bx = 0; bx < mapx; ++bx) {
        for (int32_t by = 0; by < mapy; ++by) {
            for (int32_t z = mapz - 1; z >= 0; --z) {
                // the block
                if (df::map_block* block = Maps::getBlock(bx, by, z)) {
                    // foreach tile
                    for (int16_t lx = 0; lx < 16; ++lx) {
                        for (int16_t ly = 0; ly < 16; ++ly) {
                            // the tile, check if it has a channel designation
                            if (is_channel_designation(block->designation[lx][ly])) {
                                df::coord map_pos((bx * 16) + lx, (by * 16) + ly, z);
                                add(map_pos, block);
                            }
                        }
                    }
                }
            }
        }
    }
}

// adds map_pos to a group if an adjacent one exists, or creates one if none exist... if multiple exist they're merged into the first found
void ChannelGroups::add(const df::coord &map_pos, df::map_block* block) {
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

    // and so we begin iterating the neighbours
    for (auto &neighbour: neighbors) {
        // go to the next neighbour if this one doesn't have a group
        if (!groups_map.count(neighbour)) {
            continue;
        }
        // get the group, since at least one exists... then merge any additional into that one
        if (!group){
            group_index = groups_map.find(neighbour)->second;
            group = &groups.at(group_index);
        } else {
            // we already have group "prime" if you will, so we're going to merge the new find into prime
            auto index = groups_map.find(neighbour)->second;
            Group &group2 = groups.at(index);
            free_spots.emplace(index);

            if (debug_out) debug_out->print("found adjacent group. host size: %zu. group size: %zu\n", group->size(), group2.size());
            // merge
            group->insert(group2.begin(), group2.end());
            group2.clear();
            if (debug_out) debug_out->print("merged size: %zu\n", group->size());
        }
    }
    // if we haven't found at least one group by now we need to create/get one
    if (!group) {
        if (debug_out) debug_out->print("brand new group\n");
        // first we check if we can re-use a group that's been freed
        if (!free_spots.empty()) {
            if (debug_out) debug_out->print("being clever and re-using old merged positions\n");
            // first element in a set is always the lowest value, so we re-use from the front of the vector
            group = &groups[*free_spots.begin()];
            free_spots.erase(free_spots.begin());
        } else {
            // we create a brand-new group to use
            group_index = groups.size();
            groups.push_back(Group());
            group = &groups[group_index];
        }
    }
    // puts the add in ChannelGroups::add
    group->emplace(std::make_pair(map_pos, block));
    if (debug_out) debug_out->print("final size: %zu\n", group->size());

    // we may have performed a merge, so we update all the `coord -> group index` mappings
    for (auto &pair: *group) {
        const df::coord &wpos = pair.first;
        groups_map.erase(wpos);
        groups_map.emplace(wpos, group_index);
    }
    if (debug_out) debug_out->print("group index: %d\n", group_index);
    //debug();
}

// builds groupings of adjacent channel designations
void ChannelGroups::build() {
    // iterate over each map block, finding and adding channel designations
    scan_map();
    // iterate over each job, finding channel jobs
    jobs.load_channel_jobs();
    // transpose channel jobs to
    for (auto &map_entry : jobs) {
        df::job* job = map_entry.second;
        add(map_entry.first, Maps::getTileBlock(job->pos));
    }
}

// clears out the containers for unloading maps or disabling the plugin
void ChannelGroups::clear() {
    free_spots.clear();
    groups.clear();
    groups_map.clear();
}

// erases map_pos from its group, and deletes mappings IFF the group is empty
void ChannelGroups::mark_done(const df::coord &map_pos) {
    // we don't need to do anything if the position isn't in a group (granted, that should never be the case)
    if (groups_map.count(map_pos)) {
        // get the group, and map_pos' block*
        int index = groups_map.find(map_pos)->second;
        Group &group = groups[index];
        df::map_block* block = Maps::getTileBlock(map_pos);
        // erase map_pos from the group
        group.erase(std::make_pair(map_pos, block));
        // clean up if the group is empty
        if (group.empty()) {
            // erase `coord -> group index` mappings
            for (auto iter = groups_map.begin(); iter != groups_map.end();) {
                if (index == iter->second) {
                    iter = groups_map.erase(iter);
                    continue;
                }
                ++iter;
            }
            // flag the `groups` index as available
            free_spots.insert(index);
        }
    }
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
void ChannelGroups::debug() {
    int idx = 0;
    if (debug_out) debug_out->print("debugging group data\n");
    for (auto &group : groups) {
        if (debug_out) debug_out->print("group %d (size: %zu)\n", idx++, group.size());
        for (auto &pair : group) {
            if (debug_out) debug_out->print(" (%d,%d,%d)\n", pair.first.x, pair.first.y, pair.first.z);
        }
        idx++;
    }
}
