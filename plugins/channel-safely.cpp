/* Prevent channeling down into known open space.
Author:  Josh Cooper
Created: Aug. 4 2020
Updated: Jun. 15 2021
*/

#include <PluginManager.h>
#include <modules/EventManager.h>
#include <modules/Job.h>

#include "channel-safely.h"

using namespace DFHack;

/* todo list:
 * 1) we should track channel designations as groups of designations (presumably part of the same project) designations which are collectively adjacent to one another
 * 2) check if there is a group above a given tile
 *
 */

DFHACK_PLUGIN("channel-safely");
DFHACK_PLUGIN_IS_ENABLED(enabled);
REQUIRE_GLOBAL(world);
#define tickFreq 75

#include <type_traits>
#define DECLARE_HASA(what) \
template<typename T, typename = int> struct has_##what : std::false_type { };\
template<typename T> struct has_##what<T, decltype((void) T::what, 0)> : std::true_type {};

DECLARE_HASA(when) //declares above statement with 'when' replacing 'what'
// end usage is: `has_when<T>::value`
// the only use is to allow reliance on pull request #1876 which introduces a refactor which prevents some manual management

ChannelManager manager;
void onTick(color_ostream &out, void* tick_ptr);
void onStart(color_ostream &out, void* job);
void onComplete(color_ostream &out, void* job);

void manageNeighbours(const df::coord &tile);
void cancelJob(df::job* job);
bool is_group_done(const GroupData::Group &group);
bool is_dig(df::job* job);
bool is_channel(df::job* job);
bool is_channel(df::tile_designation &designation);


command_result manage_channel_designations (color_ostream &out, std::vector <std::string> &parameters);

DFhackCExport command_result plugin_init( color_ostream &out, std::vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand("channel-safely",
                                     "A tool to manage active channel designations.",
                                     manage_channel_designations,
                                     false,
                                     "\n"));
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    namespace EM = EventManager;
    if(enable && !enabled) {
        using namespace EM::EventType;
        EM::EventHandler tickHandler(onTick, tickFreq);
        EM::EventHandler jobStartHandler(onStart, 0);
        EM::EventHandler jobCompletionHandler(onComplete, 0);

        if(!has_when<EM::EventHandler>::value){
            EM::registerTick(tickHandler, tickFreq, plugin_self);
        } else {
            EM::registerListener(EventType::TICK, tickHandler, plugin_self);
        }
        EM::registerListener(EventType::JOB_INITIATED, jobStartHandler, plugin_self);
        EM::registerListener(EventType::JOB_COMPLETED, jobCompletionHandler, plugin_self);
        manager.build_groups();
        manager.manage_all_designations(out);
    } else if (!enable) {
        EM::unregisterAll(plugin_self);
    }
    enabled = enable;
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    namespace EM = EventManager;
    EM::unregisterAll(plugin_self);
    enabled = false;
    return CR_OK;
}

// todo: figure out if useful to perhaps rebuild group data
//DFhackCExport command_result plugin_onupdate(color_ostream &out) {
//    static uint64_t count = 0;
//    out.print("onupdate() - %d\n", ++count);
//    return CR_OK;
//}

command_result manage_channel_designations(color_ostream &out, std::vector<std::string> &parameters){
    if(parameters.empty()){
        out.print("Work safe inspection! Now checking for unsafe work conditions..\n");
        manager.build_groups();
        manager.manage_all_designations(out);
    } else if (parameters.size() == 1 && parameters[0] == "enable") {
        if (plugin_enable(out, true) == CR_OK){
            out.print("channel-safely enabled!\n");
        }
    } else if (parameters.size() == 1 && parameters[0] == "disable") {
        if (plugin_enable(out, false) == CR_OK){
            out.print("channel-safely disabled!\n");
        }
    } else {
        return CR_FAILURE;
    }
    return CR_OK;
}

void onTick(color_ostream &out, void* tick_ptr){
    if(enabled && World::isFortressMode()) {
        static uint64_t count = 0;
        static int32_t last_tick_counter = 0;
        int32_t tick_counter = (int32_t) ((intptr_t) tick_ptr);
        //out.print("onTick() - %d, %d\n", ++count, tick_counter);
        if ((tick_counter - last_tick_counter) >= tickFreq) {
            last_tick_counter = tick_counter;
            manager.build_groups();
        }
    }
    namespace EM = EventManager;
    if(!has_when<EM::EventHandler>::value){
        EM::EventHandler tickHandler(onTick, tickFreq);
        EM::registerTick(tickHandler, tickFreq, plugin_self);
    }
    CoreSuspender suspend;
}

void onStart(color_ostream &out, void* job_ptr){
    if(enabled && World::isFortressMode()) {
        auto job = (df::job*) job_ptr;
        if (is_dig(job) || is_channel(job)) {
            df::coord local(job->pos);
            local.x = local.x % 16;
            local.y = local.y % 16;
            df::coord above(job->pos);
            above.z++;
            df::map_block* block = Maps::getTileBlock(job->pos);
            //postpone job if above isn't done
            manager.manage_safety(block, local, job->pos, above);
        }
    }
}

void onComplete(color_ostream &out, void* job_ptr){
    if(enabled && World::isFortressMode()) {
        auto job = (df::job*) job_ptr;
        if (is_channel(job)) {
            df::coord local(job->pos);
            local.x = local.x % 16;
            local.y = local.y % 16;
            df::coord below(job->pos);
            below.z--;
            df::map_block* block = Maps::getTileBlock(below);
            //activate designation below if group is done now, postpone if not
            manager.manage_safety(block, local, below, job->pos);
            manager.mark_done(job->pos);
            manageNeighbours(job->pos);
        }
    }
}

void ChannelManager::manage_all_designations(color_ostream &out) {
    static std::once_flag getMapSize;
    static uint32_t t1, t2, zmax;
    std::call_once(getMapSize, Maps::getSize, t1, t2, zmax);
    for(auto &map_pair : groups){
        GroupData::Group group = map_pair.second;
        for(auto &group_pair : group){
            const df::coord &world_pos = group_pair.first;
            df::map_block* block = group_pair.second;
            df::coord local(world_pos);
            local.x = local.x % 16;
            local.y = local.y % 16;
            if(world_pos.z < zmax - 1){
                df::coord above(world_pos);
                above.z++;
                manage_safety(block, local, world_pos, above);
            } else {
                block->occupancy[local.x][local.y].bits.dig_marked = false;
            }
        }
    }
}

//#include <df/block_square_event.h>
#include <df/block_square_event_designation_priorityst.h>

void ChannelManager::manage_safety(df::map_block* block, const df::coord &local, const df::coord &tile, const df::coord &tile_above) {
    auto iter = groups.find(tile_above);
    df::tile_occupancy &tile_occupancy = block->occupancy[local.x][local.y];
    for(df::block_square_event* event : block->block_events){
        switch(event->getType()){
            case block_square_event_type::designation_priority:
                auto evT = (df::block_square_event_designation_priorityst*)event;
                if(evT->priority[local.x][local.y] < 6000) {
                    if (iter != groups.end()) {
                        GroupData::Group group = iter->second;
                        if (is_group_done(group)) {
                            tile_occupancy.bits.dig_marked = false;
                        } else {
                            // not safe
                            tile_occupancy.bits.dig_marked = true;
                            jobs.cancel_job(tile); //cancels job if designation is an open/active job
                        }
                    }
                    else {
                        // no group above tile
                        tile_occupancy.bits.dig_marked = false;
                    }
                }
                break;
        }
    }
}

void ChannelManager::mark_done(const df::coord &tile) {
    auto iter = groups.find(tile);
    if(iter != groups.end()){
        GroupData::Group group = iter->second;
        df::map_block* block = Maps::getTileBlock(tile);
        group.erase(std::make_pair(tile,block));
    }
}

void manageNeighbours(const df::coord &tile){
    df::coord neighbors[8];
    neighbors[0] = tile;
    neighbors[1] = tile;
    neighbors[2] = tile;
    neighbors[3] = tile;
    neighbors[4] = tile;
    neighbors[5] = tile;
    neighbors[6] = tile;
    neighbors[7] = tile;
    neighbors[0].x--; neighbors[0].y--;
    neighbors[1].x++; neighbors[1].y++;
    neighbors[2].x++; neighbors[2].y--;
    neighbors[3].x--; neighbors[3].y++;
    neighbors[4].x++;
    neighbors[5].x--;
    neighbors[6].y--;
    neighbors[7].y++;

    for(auto &position : neighbors){
        df::coord local(position);
        local.x = local.x % 16;
        local.y = local.y % 16;
        df::coord above(position);
        above.z++;
        df::map_block* block = Maps::getTileBlock(position);
        //activate designation if group is done now, postpone if not
        manager.manage_safety(block, local, position, above);
    }
}

void cancelJob(df::job* job) {
    if(job != nullptr) {
        df::coord &pos = job->pos;
        df::map_block *job_block = Maps::getTileBlock(pos);
        uint16_t x, y;
        x = pos.x % 16;
        y = pos.y % 16;
        df::tile_designation &designation = job_block->designation[x][y];
        designation.bits.dig = is_channel(job) ?
                               df::tile_dig_designation::Channel : df::tile_dig_designation::Default;
        Job::removeJob(job);
    }
}

bool is_group_done(const GroupData::Group &group){
    return group.empty();
}

bool is_dig(df::job* job) {
    return job->job_type == df::job_type::Dig;
}

bool is_channel(df::job* job){
    return job->job_type == df::job_type::DigChannel;
}

bool is_channel(df::tile_designation &designation){
    return designation.bits.dig == df::tile_dig_designation::Channel;
}

void GroupData::read(){
    groups.clear();
    foreach_block();
    jobs.read();
    for(auto &map_entry : jobs){
        df::job* job = map_entry.second;
        if(is_channel(job)){
            add(map_entry.first, Maps::getTileBlock(job->pos));
        }
    }
}

void GroupData::foreach_block() {
    static std::once_flag getMapSize;
    static uint32_t x, y, z;
    std::call_once(getMapSize, Maps::getSize, x, y, z);
    for (int ix = 0; ix < x; ++ix) {
        for (int iy = 0; iy < y; ++iy) {
            for (int iz = z - 1; iz >= 0; --iz) {
                df::map_block* block = Maps::getBlock(ix, iy, iz);
                if(block->flags.bits.designated) {
                    foreach_tile(block, iz);
                }
            }
        }
    }
}

void GroupData::foreach_tile(df::map_block* block, int z) {
    for (int16_t local_x = 0; local_x < 16; ++local_x) {
        for (int16_t local_y = 0; local_y < 16; ++local_y) {
            if(is_channel(block->designation[local_x][local_y])){
                df::coord world_pos(block->map_pos);
                world_pos.x = local_x + (world_pos.x << 4);
                world_pos.y = local_y + (world_pos.y << 4);
                world_pos.z = z;
                add(world_pos, block);
            }
        }
    }
}

void GroupData::add(df::coord pos, df::map_block* block) {
    df::coord neighbors[8];
    neighbors[0] = pos;
    neighbors[1] = pos;
    neighbors[2] = pos;
    neighbors[3] = pos;
    neighbors[4] = pos;
    neighbors[5] = pos;
    neighbors[6] = pos;
    neighbors[7] = pos;
    neighbors[0].x--; neighbors[0].y--;
    neighbors[1].x++; neighbors[1].y++;
    neighbors[2].x++; neighbors[2].y--;
    neighbors[3].x--; neighbors[3].y++;
    neighbors[4].x++;
    neighbors[5].x--;
    neighbors[6].y--;
    neighbors[7].y++;

    Group return_group;
    return_group.insert(std::make_pair(pos, block));
    for(auto &position : neighbors){
        auto group_iter = groups.find(position);
        if(group_iter != groups.end()){
            Group &group = group_iter->second;
            return_group.insert(group.begin(), group.end());
        }
    }
    for(auto &pair : return_group){
        groups.emplace(pair.first, return_group);
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
    if(iter != jobs.end()){
        df::job* job = iter->second;
        cancelJob(job);
    }
}