//
// Created by josh on 6/15/21.
//

#pragma once
#include <PluginManager.h>
#include <modules/Maps.h>
#include <modules/Job.h>
#include <df/map_block.h>
#include <df/world.h>

#include <map>

using namespace DFHack;

DFHACK_PLUGIN("channel-safely");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

class ChannelManager {
public:
    void manage_designations();

private:
    std::multimap<int16_t, df::job*> dig_jobs;

protected:
    df::job* find_job(df::coord &tile_pos);
    void foreach_block_column();
    void foreach_tile(df::map_block* top, df::map_block* bottom);
    void find_dig_jobs();
};