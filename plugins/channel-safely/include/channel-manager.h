#pragma once
#include <PluginManager.h>
#include <modules/World.h>
#include <modules/Maps.h>
#include <modules/Job.h>
#include <df/map_block.h>
#include "channel-groups.h"
#include "plugin.h"

using namespace DFHack;

// Uses GroupData to detect an unsafe work environment
class ChannelManager {
private:
    ChannelJobs jobs;
    ChannelManager()= default;
protected:
public:
    ChannelGroups groups = ChannelGroups(jobs);

    static ChannelManager& Get(){
        static ChannelManager instance;
        return instance;
    }

    void build_groups() { groups.build(); debug(); }
    void manage_all();
    void manage_group(const df::coord &map_pos, bool set_marker_mode = false, bool marker_mode = false);
    void manage_group(const Group &group, bool set_marker_mode = false, bool marker_mode = false);
    bool manage_one(const Group &group, const df::coord &map_pos, bool set_marker_mode = false, bool marker_mode = false);
    void mark_done(const df::coord &map_pos);
    void debug() {
        if (config.debug) {
            groups.debug_groups();
            groups.debug_map();
            //std::terminate();
        }
    }
};
