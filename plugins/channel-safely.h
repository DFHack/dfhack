//
// Created by josh on 6/15/21.
//

#pragma once
#include <PluginManager.h>
#include <modules/World.h>
#include <modules/Maps.h>
#include <modules/MapCache.h>
#include <modules/Job.h>
#include <df/map_block.h>
#include <df/world.h>

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
    using Groups = std::map<df::coord, Group>;
    Groups groups;
    DigJobs &jobs;
protected:
    void foreach_block();
    void foreach_tile(df::map_block* block, int z);
    void add(df::coord pos, df::map_block* block);
public:
    GroupData(DigJobs &jobs) : jobs(jobs) {}
    void read();
    void erase(const df::coord &tile) {
        auto iter = groups.find(tile);
        if(iter != groups.end()){

        }
    }
    void cancel_job(const df::coord &pos) { jobs.cancel_job(pos); }
    void clear() { groups.clear(); }
    Groups::const_iterator find(const df::coord &pos) const { return groups.find(pos); }
    Groups::const_iterator begin() const { return groups.begin(); }
    Groups::const_iterator end() const { return groups.end(); }
};

// Uses GroupData to detect an unsafe work environment
class ChannelManager {
private:
    DigJobs jobs;
    GroupData groups = GroupData(jobs);
public:
    void build_groups() { groups.read(); }
    void manage_all_designations(color_ostream &out);
    void manage_safety(df::map_block* block, const df::coord &local, const df::coord &tile, const df::coord &tile_above);
    void mark_done(const df::coord &tile);
};