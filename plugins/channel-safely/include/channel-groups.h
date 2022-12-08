#pragma once
#include "plugin.h"
#include "channel-jobs.h"

#include <df/map_block.h>
#include <df/coord.h>
#include <modules/EventManager.h> //hash functions (they should probably get moved at this point, the ones that aren't specifically for EM anyway)

#include <vector>
#include <unordered_map>
#include <unordered_set>

using namespace DFHack;

using Group = std::set<df::coord>;
using Groups = std::vector<Group>;

/* Used to build groups of adjacent channel designations/jobs
 * groups_map: maps coordinates to a group index in `groups`
 * groups: list of Groups
 * Group: used to track designations which are connected through adjacency to one another (a group cannot span Z)
 *     Note: a designation plan may become unsafe if the jobs aren't completed in a specific order;
 *           the easiest way to programmatically ensure safety is to..
 *           lock overlapping groups directly adjacent across Z until the above groups are complete, or no longer overlap
 *           groups may no longer overlap if the adjacent designations are completed, but requires a rebuild of groups
 * jobs: list of coordinates with channel jobs associated to them
 */
class ChannelGroups {
private:
    using GroupBlocks = std::unordered_set<df::map_block*>;
    using GroupsMap = std::unordered_map<df::coord, int>;
    GroupBlocks group_blocks;
    GroupsMap groups_map;
    Groups groups;
    ChannelJobs &jobs;
    std::set<int> free_spots;
protected:
    void add(const df::coord &map_pos);
public:
    int debugGIndex(const df::coord &map_pos) const {
        if (groups_map.count(map_pos)) {
            return groups_map.find(map_pos)->second;
        }
        return -1;
    }
    explicit ChannelGroups(ChannelJobs &jobs) : jobs(jobs) { groups.reserve(200); }
    void scan_one(const df::coord &map_pos);
    void scan(bool full_scan = false);
    void clear();
    void remove(const df::coord &map_pos);
    Groups::const_iterator find(const df::coord &map_pos) const;
    Groups::const_iterator begin() const;
    Groups::const_iterator end() const;
    size_t count(const df::coord &map_pos) const;
    void debug_groups();
    void debug_map();
};
