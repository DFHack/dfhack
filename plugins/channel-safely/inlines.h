#pragma once
#include "channel-groups.h"
#include <TileTypes.h>
#include <modules/Maps.h>
#include <df/job.h>

inline bool is_dig(df::job* job) {
    return job->job_type == df::job_type::Dig;
}

inline bool is_channel_job(df::job* job) {
    return job->job_type == df::job_type::DigChannel;
}

inline bool is_channel_designation(df::tile_designation &designation) {
    return designation.bits.dig == df::tile_dig_designation::Channel;
}

inline bool is_safe_to_dig_down(const df::coord &map_pos){
    df::coord below(map_pos);
    below.z--;
    df::tiletype type = *Maps::getTileType(below);
    return !isOpenTerrain(type) && !isFloorTerrain(type);
}

inline bool is_group_occupied(const ChannelGroups &groups, const Group &group) {
    // return true if any tile in the group is occupied by a unit
    return std::any_of(group.begin(), group.end(), [](const Group::key_type &pair){
        return Maps::getTileOccupancy(pair.first)->bits.unit;
    });
}

inline bool has_groups_above(const ChannelGroups &groups, const Group &group) {
    // for each designation in the group
    for (auto &key_value : group) {
        auto &tile_pos = key_value.first;
        //auto &block = key_value.second;
        // check if we could potentially fall
        if(!is_safe_to_dig_down(tile_pos)){
            return false;
        }
        df::coord above(tile_pos);
        above.z++;
        // check whether there is a non-empty group above any of the tiles in `group`
        if (groups.count(above)) {
            if (!groups.find(above)->empty()) {
                return true;
            }
        }
    }
    // if there are no incomplete groups above this group, then this group is ready
    return false;
}

inline void cancel_job(df::job* job) {
    if (job != nullptr) {
        df::coord &pos = job->pos;
        df::map_block* job_block = Maps::getTileBlock(pos);
        uint16_t x, y;
        x = pos.x % 16;
        y = pos.y % 16;
        df::tile_designation &designation = job_block->designation[x][y];
        designation.bits.dig = is_channel_job(job) ?
                               df::tile_dig_designation::Channel : df::tile_dig_designation::Default;
        Job::removeJob(job);
    }
}

inline void get_neighbours(const df::coord &map_pos, df::coord(&neighbours)[8]) {
    neighbours[0] = map_pos;
    neighbours[1] = map_pos;
    neighbours[2] = map_pos;
    neighbours[3] = map_pos;
    neighbours[4] = map_pos;
    neighbours[5] = map_pos;
    neighbours[6] = map_pos;
    neighbours[7] = map_pos;
    neighbours[0].x--; neighbours[0].y--;
    neighbours[1].y--;
    neighbours[2].x++; neighbours[2].y--;
    neighbours[3].x--;
    neighbours[4].x++;
    neighbours[5].x--; neighbours[5].y++;
    neighbours[6].y++;
    neighbours[7].x++; neighbours[7].y++;
}
