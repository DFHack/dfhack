//
// Created by josh on 6/15/21.
//

#pragma once
#include <PluginManager.h>
#include <modules/World.h>
#include <modules/Maps.h>
#include <modules/Job.h>
#include <df/map_block.h>
#include <df/world.h>
#include <df/block_square_event_designation_priorityst.h>
#include <map>

using namespace DFHack;

/* Used to read/store/iterate channel digging jobs
 * jobs: list of coordinates with channel jobs associated to them
 * read: world->jobs.list to find channel jobs and emplaces them into the `jobs` map
 * cancel_job: finds a job corresponding to a coord, removes the mapping in jobs, and calls Job::removeJob
 */
class DigJobs {
private:
    friend class GroupData;

    using Jobs = std::map<df::coord, df::job*>; // job* will exist until it is complete, and likely beyond
    Jobs jobs;
public:
    void read();
    void clear() { jobs.clear(); }
    typename Jobs::iterator erase_and_cancel(const df::coord &pos);
    Jobs::const_iterator find(const df::coord &pos) const { return jobs.find(pos); }
    Jobs::const_iterator begin() const { return jobs.begin(); }
    Jobs::const_iterator end() const { return jobs.end(); }
};

// Used to build groups of adjacent channel designations/jobs
class GroupData {
public:
    using Group = std::set<std::pair<df::coord, df::map_block*>>;
private:
    using Groups = std::vector<Group>;
    using GroupsMap = std::map<df::coord, int>;
    GroupsMap groups_map;
    Groups groups;
    DigJobs &jobs;
    std::set<int> free_spots;
protected:
    void foreach_block();
    void foreach_tile(df::map_block* block, int z);
    void add(df::coord world_pos, df::map_block* block);
public:
    GroupData(DigJobs &jobs) : jobs(jobs) { groups.reserve(200); }
    void read();
    void cancel_job(const df::coord &pos) { jobs.cancel_job(pos); }
    void clear() { // to allow deleting group data when the plugin isn't enabled
        free_spots.clear();
        groups.clear();
        groups_map.clear();
    }
    void mark_done(const df::coord &tile) {
        auto iter = groups_map.find(tile);
        int group_index = iter->second;
        if (iter != groups_map.end()) {
            Group &group = groups[group_index];
            df::map_block* block = Maps::getTileBlock(tile);
            group.erase(std::make_pair(tile, block));
            if (group.empty()) {
                for (auto iter = groups_map.begin(); iter != groups_map.end();) {
                    if (group_index == iter->second) {
                        iter = groups_map.erase(iter);
                    } else {
                        ++iter;
                    }
                }
                free_spots.insert(group_index);
            }
        }
    }
    Groups::const_iterator find(const df::coord &pos) const {
        const auto iter = groups_map.find(pos);
        if (iter != groups_map.end()) {
            return groups.begin() + iter->second;
        }
        return groups.end();
    }
    Groups::const_iterator begin() const { return groups.begin(); }
    Groups::const_iterator end() const { return groups.end(); }
    void debug();
};

// Uses GroupData to detect an unsafe work environment
class ChannelManager {
private:
    DigJobs jobs;
    GroupData groups = GroupData(jobs);
    ChannelManager(){}
protected:
    void build_groups() { groups.read(); }
public:
    static ChannelManager Get(){
        static ChannelManager instance;
        return instance;
    }
    void manage_designations(color_ostream &out);
    void manage_safety(color_ostream &out, df::map_block* block, const df::coord &local, const df::coord &tile, const df::coord &tile_above);
    void delete_groups() { groups.clear(); }
    void mark_done(const df::coord &tile) { groups.mark_done(tile); }
    void debug() {
        groups.debug();
    }
};

extern color_ostream* debug_out;
extern bool cheat_mode;
inline void getNeighbours(const df::coord &tile, df::coord(&neighbours)[8]) {
    neighbours[0] = tile;
    neighbours[1] = tile;
    neighbours[2] = tile;
    neighbours[3] = tile;
    neighbours[4] = tile;
    neighbours[5] = tile;
    neighbours[6] = tile;
    neighbours[7] = tile;
    neighbours[0].x--; neighbours[0].y--;
    neighbours[1].y--;
    neighbours[2].x++; neighbours[2].y--;
    neighbours[3].x--;
    neighbours[4].x++;
    neighbours[5].x--; neighbours[5].y++;
    neighbours[6].y++;
    neighbours[7].x++; neighbours[7].y++;
}
extern void manageNeighbours(color_ostream &out, const df::coord &tile);
extern void cancelJob(df::job* job);
extern bool is_group_ready(const GroupData &groups, const GroupData::Group &group);
extern bool is_group_occupied(const GroupData &groups, const GroupData::Group &group);
extern bool is_safe_to_dig_down(const df::coord &tile);
inline bool is_dig(df::job* job) {
    return job->job_type == df::job_type::Dig;
}
inline bool is_channel_job(df::job* job) {
    return job->job_type == df::job_type::DigChannel;
}
inline bool is_channel_designation(df::tile_designation &designation) {
    return designation.bits.dig == df::tile_dig_designation::Channel;
}

