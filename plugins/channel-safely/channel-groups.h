#pragma once
#include "channel-jobs.h"
#include <df/map_block.h>
#include <df/coord.h>
#include <vector>
#include <map>
#include <set>

using namespace DFHack;

extern int32_t mapx,mapy,mapz;
using Group = std::set<std::pair<df::coord, df::map_block*>>;
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
 * scan_map:
 * add:
 * read:
 * mark_done:
 */
class ChannelGroups {
private:
    using GroupsMap = std::map<df::coord, int>;
    GroupsMap groups_map;
    Groups groups;
    ChannelJobs &jobs;
    std::set<int> free_spots;
protected:
    void scan_map();
    void add(const df::coord &map_pos, df::map_block* block);
public:
    explicit ChannelGroups(ChannelJobs &jobs) : jobs(jobs) { groups.reserve(200); }
    void build();
    void clear();
    void mark_done(const df::coord &map_pos);
    Groups::const_iterator find(const df::coord &map_pos) const;
    Groups::const_iterator begin() const;
    Groups::const_iterator end() const;
    size_t count(const df::coord &map_pos) const;
    void debug();
};
