//
// Created by josh on 6/15/21.
//

#pragma once
#include <PluginManager.h>
#include <map>

using namespace DFHack;

// Used to read/store/iterate channel digging jobs
class DigJobs {
private:
    friend class GroupData;
    using Jobs = std::map<df::coord, df::job*>;
    Jobs jobs;
public:
    void read();
    void clear() { jobs.clear(); }
    void cancel_job(const df::coord &pos);
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
    void clear() {
        free_spots.clear();
        groups.clear();
        groups_map.clear();
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
protected:
    void build_groups() { groups.read(); }
public:
    void manage_designations(color_ostream &out);
    void manage_safety(color_ostream &out, df::map_block* block, const df::coord &local, const df::coord &tile, const df::coord &tile_above);
    void delete_groups() { groups.clear(); }
    void mark_done(const df::coord &tile) {
        auto iter = groups.find(tile);
        if (iter != groups.end()) {
            GroupData::Group group = *iter;
            df::map_block* block = Maps::getTileBlock(tile);
            group.erase(std::make_pair(tile, block));
        }
    }
    void debug(){
        groups.debug();
    }
};