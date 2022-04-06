#pragma once
#include <PluginManager.h>
#include <modules/World.h>
#include <modules/Maps.h>
#include <modules/Job.h>
#include <df/map_block.h>
#include "channel-groups.h"

using namespace DFHack;

// Uses GroupData to detect an unsafe work environment
class ChannelManager {
private:
    ChannelJobs jobs;
    ChannelManager()= default;
protected:
public:
    ChannelGroups groups = ChannelGroups(jobs);

    static ChannelManager Get(){
        static ChannelManager instance;
        return instance;
    }

    void build_groups() { groups.build(); }
    void delete_groups() { groups.clear(); }
    void manage_all(color_ostream &out);
    void manage_neighbours(color_ostream &out, const df::coord &map_pos);
    void manage_one(color_ostream &out, const df::coord &map_pos, df::map_block* block, bool manage_neighbours = false);
    void manage_one(color_ostream &out, const Group &group, const df::coord &map_pos, df::map_block* block);
    void mark_done(const df::coord &map_pos) { groups.mark_done(map_pos); }
    void debug() {
        groups.debug();
    }
};
