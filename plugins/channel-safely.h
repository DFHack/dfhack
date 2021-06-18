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

#include <map>

using namespace DFHack;

struct dig_job{
    enum {
        NONE,
        JOB,
        DESIGNATED
    } type;
    union{
        df::job* job;
        df::map_block* block;
    } pointer;
    df::coord map_pos;

    dig_job(df::job* job){
        type = JOB;
        pointer.job = job;
        map_pos = job->pos;
    }
    dig_job(df::map_block* block, df::coord pos){
        type = DESIGNATED;
        pointer.block = block;
        map_pos = pos;
    }
};

class ChannelManager {
public:
    void manage_designations(color_ostream &out);

private:
    std::multimap<int16_t, df::job*> dig_jobs;

protected:
    df::job* find_job(df::coord &tile_pos);
    void foreach_block_column(color_ostream &out);
    void foreach_tile(color_ostream &out, df::map_block* top, df::map_block* bottom);
    void find_dig_jobs();
};