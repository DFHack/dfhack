//
// Created by josh on 6/27/21.
//
#include "channel-safely.h"

color_ostream* debug_out = nullptr;
bool cheat_mode = false;
ChannelManager manager;

void ChannelManager::manage_designations(color_ostream &out) {
    if (debug_out) debug_out->print("manage_designations()\n");
    if (World::isFortressMode() && Maps::IsValid()) {
        static bool getMapSize = false;
        static uint32_t t1, t2, zmax;
        if (!getMapSize) {
            getMapSize = true;
            Maps::getSize(t1, t2, zmax);
        }
        if (debug_out) debug_out->print("map size: %d, %d, %d\n", t1, t2, zmax);
        build_groups();
        for (auto &group : groups) {
            if (debug_out) debug_out->print("foreach group\n");
            for (auto &tile : group) {
                if (debug_out) debug_out->print("foreach tile\n");
                const df::coord &world_pos = tile.first;
                df::map_block* block = tile.second;
                df::coord local(world_pos);
                local.x = local.x % 16;
                local.y = local.y % 16;
                if (world_pos.z < zmax - 1) {
                    df::coord above(world_pos);
                    above.z++;
                    manage_safety(out, block, local, world_pos, above);
                } else {
                    block->occupancy[local.x][local.y].bits.dig_marked = false;
                }
            }
        }
        if (debug_out) debug_out->print("done managing designations\n");
    }
}

void ChannelManager::manage_safety(color_ostream &out, df::map_block* block, const df::coord &local, const df::coord &tile, const df::coord &tile_above) {
    auto group_iter = groups.find(tile);
    if (group_iter != groups.end()) {
        df::tile_occupancy &tile_occupancy = block->occupancy[local.x][local.y];
        // first we make sure the tile has a designation priority
        for (df::block_square_event* event : block->block_events) {
            if (debug_out) debug_out->print("switch(event->getType())\n");
            if (event->getType() == df::block_square_event_type::designation_priority) {
                auto evT = (df::block_square_event_designation_priorityst*) event;
                // second we ensure the priority is less than 6 - let the user keep some free from interference
                if (evT->priority[local.x][local.y] < 6000) {
                    if (debug_out) debug_out->print("*group_iter\n");
                    const GroupData::Group &group = *group_iter;
                    if (debug_out) debug_out->print("if(is_group_ready())\n");
                    if (is_group_ready(groups, group)) {
                        // no pending groups above this group
                        if (debug_out) debug_out->print("dig_marked = false\n");
                        tile_occupancy.bits.dig_marked = false;
                        if (debug_out) debug_out->print("block->flags.bits.designated = true\n");
                        block->flags.bits.designated = true;
                        if (debug_out) debug_out->print("after setting.\n");
                    } else {
                        // not safe
                        if (debug_out) debug_out->print("dig_marked = true\n");
                        tile_occupancy.bits.dig_marked = true;
                        if (debug_out) debug_out->print("cancel_job()\n");
                        jobs.cancel_job(tile); //cancels job if designation is an open/active job
                    }
                }
            }
        }
    }
}

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

void manageNeighbours(color_ostream &out, const df::coord &tile) {
    df::coord neighbours[8];
    getNeighbours(tile, neighbours);
    for (auto &position : neighbours) {
        if (Maps::isValidTilePos(position)) {
            df::coord local(position);
            local.x = local.x % 16;
            local.y = local.y % 16;
            df::coord above(position);
            above.z++;
            if (debug_out) debug_out->print("getTileBlock()\n");
            df::map_block* block = Maps::getTileBlock(position);
            if (debug_out) debug_out->print("manage_safety()\n");
            //activate designation if group is done now, postpone if not
            manager.manage_safety(out, block, local, position, above);
        }
    }
}

void cancelJob(df::job* job) {
    if (job != nullptr) {
        df::coord &pos = job->pos;
        df::map_block* job_block = Maps::getTileBlock(pos);
        uint16_t x, y;
        x = pos.x % 16;
        y = pos.y % 16;
        df::tile_designation &designation = job_block->designation[x][y];
        designation.bits.dig = is_channel(job) ?
                               df::tile_dig_designation::Channel : df::tile_dig_designation::Default;
        Job::removeJob(job);
    }
}

bool is_group_ready(const GroupData &groups, const GroupData::Group &below_group) {
    for (auto &group_tile : below_group) {
        df::coord world_pos(group_tile.first);
        world_pos.z++;
        auto group_iter = groups.find(world_pos);
        if (debug_out) debug_out->print("if(group_iter != groups.end()) => %d == %d\n", group_iter, groups.end());
        if (group_iter != groups.end()) {
            if (debug_out) debug_out->print("if(!group_iter->empty())\n");
            if (!group_iter->empty()) {
                if (debug_out) debug_out->print("return false\n");
                return false;
            }
        }
    }
    if (debug_out) debug_out->print("return true\n");
    return true;
}

bool is_dig(df::job* job) {
    return job->job_type == df::job_type::Dig;
}

bool is_channel(df::job* job) {
    return job->job_type == df::job_type::DigChannel;
}

bool is_channel(df::tile_designation &designation) {
    return designation.bits.dig == df::tile_dig_designation::Channel;
}

void GroupData::read() {
    foreach_block();
    jobs.read();
    for (auto &map_entry : jobs) {
        df::job* job = map_entry.second;
        if (is_channel(job)) {
            add(map_entry.first, Maps::getTileBlock(job->pos));
        }
    }
}

void GroupData::foreach_block() {
    static bool getMapSize = false;
    static uint32_t x, y, z;
    if (!getMapSize) {
        getMapSize = true;
        Maps::getSize(x, y, z);
    }
    if (debug_out) debug_out->print("map size: %d, %d, %d\n", x, y, z);
    for (int ix = 0; ix < x; ++ix) {
        for (int iy = 0; iy < y; ++iy) {
            for (int iz = z - 1; iz >= 0; --iz) {
                df::map_block* block = Maps::getBlock(ix, iy, iz);
                if (debug_out) debug_out->print("foreach block [%d]\n", block);
                if (block) {
                    foreach_tile(block, iz);
                }
            }
        }
    }
}

void GroupData::foreach_tile(df::map_block* block, int z) {
    for (int16_t local_x = 0; local_x < 16; ++local_x) {
        for (int16_t local_y = 0; local_y < 16; ++local_y) {
            if (is_channel(block->designation[local_x][local_y])) {
                if (debug_out) debug_out->print("foreach tile\n");
                df::coord world_pos(block->map_pos);
                world_pos.x += local_x;
                world_pos.y += local_y;
                world_pos.z = z;
                add(world_pos, block);
            }
        }
    }
}

void GroupData::add(df::coord world_pos, df::map_block* block) {
    if (groups_map.find(world_pos) == groups_map.end()) {
        df::coord neighbors[8];
        getNeighbours(world_pos, neighbors);
        int group_index = -1;
        bool populated = false;
        for (auto &neighbour : neighbors) {
            auto groups_map_iter = groups_map.find(neighbour);
            // does neighbour belong to a group?
            if (groups_map_iter != groups_map.end()) {
                if (groups_map_iter->second >= groups.size()) {
                    if (debug_out) debug_out->print("ERROR!!!\n");
                }
                // get the group
                Group &group = *(groups.begin() + groups_map_iter->second);
                if (!populated) {
                    // this is the first group we found, use it to merge others into
                    if (debug_out) debug_out->print("found adjacent merge host\n");
                    populated = true;
                    group_index = groups_map_iter->second;
                } else if (group_index != groups_map_iter->second) {
                    // merge group into host
                    if (debug_out) debug_out->print("found adjacent group. host size: %d. group size: %d\n", groups[group_index].size(), group.size());
                    groups[group_index].insert(group.begin(), group.end());
                    if (debug_out) debug_out->print("merged size: %d\n", groups[group_index].size());
                    group.clear();
                    free_spots.emplace(groups_map_iter->second);
                }
            }
        }
        if (!populated) {
            if (debug_out) debug_out->print("brand new group\n");
            if (!free_spots.empty()) {
                if (debug_out) debug_out->print("being clever and re-using old merged positions\n");
                group_index = *free_spots.begin();
                free_spots.erase(free_spots.begin());
            } else {
                groups.push_back(Group());
                group_index = groups.size() - 1;
            }
        }
        groups[group_index].emplace(std::make_pair(world_pos, block));
        if (debug_out) debug_out->print("final size: %d\n", groups[group_index].size());
        for (auto &pair : groups[group_index]) {
            const df::coord &world_pos = pair.first;
            groups_map.erase(world_pos);
            groups_map.emplace(world_pos, group_index);
        }
        if (debug_out) debug_out->print("group index: %d\n", group_index);
        //debug();
    }
}

void GroupData::debug() {
    int idx = 0;
    if (debug_out) debug_out->print("debugging group data\n");
    for (auto &group : groups) {
        if (debug_out) debug_out->print("group %d (size: %d)\n", idx++, group.size());
        for (auto &pair : group) {
            if (debug_out) debug_out->print("(%d,%d,%d)\n", pair.first.x, pair.first.y, pair.first.z);
        }
        idx++;
    }
}

void DigJobs::read() {
    jobs.clear();
    df::job_list_link* node = df::global::world->jobs.list.next;
    while (node) {
        df::job* job = node->item;
        node = node->next;
        if (is_channel(job)) {
            jobs.emplace(job->pos, job);
        }
    }
}

void DigJobs::cancel_job(const df::coord &pos) {
    auto iter = jobs.find(pos);
    if (iter != jobs.end()) {
        df::job* job = iter->second;
        cancelJob(job);
    }
}