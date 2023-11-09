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
    ChannelManager()= default;
protected:
public:
    ChannelGroups groups = ChannelGroups(jobs);
    ChannelJobs jobs;

    static ChannelManager& Get(){
        static ChannelManager instance;
        return instance;
    }

    void build_groups(bool full_scan = false) { groups.scan(full_scan); debug(); }
    void destroy_groups() { groups.clear(); debug(); }
    void manage_groups();
    void manage_group(const df::coord &map_pos, bool set_marker_mode = false, bool marker_mode = false);
    void manage_group(const Group &group, bool set_marker_mode = false, bool marker_mode = false) const;
    bool manage_one(const df::coord &map_pos, bool set_marker_mode = false, bool marker_mode = false) const;
    void mark_done(const df::coord &map_pos);
    bool exists(const df::coord &map_pos) const { return groups.count(map_pos); }
    void debug() {
        DEBUG(groups).print(" DEBUGGING GROUPS:\n");
        groups.debug_map();
        groups.debug_groups();
    }
};
