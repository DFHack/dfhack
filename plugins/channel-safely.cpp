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
#define tickFreq 5

#include <type_traits>


#define DECLARE_HASA(what) \
template<typename T, typename = int> struct has_##what : std::false_type { };\
template<typename T> struct has_##what<T, decltype((void) T::what, 0)> : std::true_type {};

DECLARE_HASA(when) //declares above statement with 'when' replacing 'what'
// if 




/*
template <int X>
struct MyMacro { int value = X; };
using myShortcut = MyMacro<X>::value;

myShortcut<x>
*/
void onTick(color_ostream &out, void* tick_ptr);
void onStart(color_ostream &out, void* job);
void onComplete(color_ostream &out, void* job);

void cancelJob(df::job* job);
bool is_dig(df::job* job);
bool is_dig(df::tile_designation &designation);
bool is_channel(df::job* job);
bool is_channel(df::tile_designation &designation);
bool is_designated(df::tile_designation &designation);
bool is_marked(df::tile_occupancy &occupancy);


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

//DFhackCExport command_result plugin_onupdate(color_ostream &out) {
//    static uint64_t count = 0;
//    out.print("onupdate() - %d\n", ++count);
//    return CR_OK;
//}

command_result manage_channel_designations(color_ostream &out, std::vector<std::string> &parameters){
    if(parameters.empty()){
        out.print("Checking for unsafe designations!\n");
        ChannelManager m;
        m.manage_designations(out);
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
    out.print("the only once onTick()! Come one, come ALL!\n");
    if(enabled && World::isFortressMode()) {
        static uint64_t count = 0;
        static int32_t last_tick_counter = 0;
        int32_t tick_counter = (int32_t) ((intptr_t) tick_ptr);
        out.print("onTick() - %d, %d\n", ++count, tick_counter);
        if ((tick_counter - last_tick_counter) >= tickFreq) {
            last_tick_counter = tick_counter;
            ChannelManager m;
            m.manage_designations(out);
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
        df::job* job = (df::job*) job_ptr;
        if (is_dig(job) || is_channel(job)) {
            // channeling from above would be unsafe
            df::coord pos = job->pos;
            pos.z += 1;
            df::tile_designation &aboved = *Maps::getTileDesignation(pos);
            if (is_channel(aboved)) {
                cancelJob(job);
                df::tile_occupancy &job_o = *Maps::getTileOccupancy(pos);
                job_o.bits.dig_marked = true;
            }
        }
    }
}

void onComplete(color_ostream &out, void* job_ptr){
    if(enabled && World::isFortressMode()) {
        df::job* job = (df::job*) job_ptr;
        if (is_dig(job) || is_channel(job)) {
            //above isn't safe to channel
            df::coord pos = job->pos;
            pos.z += 1;
            df::tile_designation &aboved = *Maps::getTileDesignation(pos);
            if (is_channel(aboved)) {
                df::tile_occupancy &aboveo = *Maps::getTileOccupancy(pos);
                aboveo.bits.dig_marked = true;
            }
            //below is safe to do anything
            pos.z -= 2;
            df::tile_designation &belowd = *Maps::getTileDesignation(pos);
            if (is_channel(belowd) || is_dig(belowd)) {
                df::tile_occupancy &belowo = *Maps::getTileOccupancy(pos);
                //todo: belowo.bits.dig_marked = false;
            }
        }
    }
}

void ChannelManager::manage_designations(color_ostream &out) {
//    out.print("isValid: %d\nworld: %d\nmap: %d\n", Maps::IsValid(), world, &world->map);
//    out.print("map count block (%d, %d, %d)\n", world->map.x_count, world->map.y_count, world->map.z_count);
//    out.print("map block index (%d, %d, %d)\n", world->map.x_count, world->map.y_count, world->map.z_count);
    dig_jobs.clear();
    find_dig_jobs();
    foreach_block_column(out);
}

df::job* ChannelManager::find_job(df::coord &tile_pos) {
    auto iter = dig_jobs.lower_bound(tile_pos.z);
    while (iter != dig_jobs.end()) {
        df::coord &pos = iter->second->pos;
        if (pos == tile_pos) {
            return iter->second;
        }
        iter++;
    }
    return nullptr;
}

void ChannelManager::foreach_block_column(color_ostream &out) {
    uint32_t x, y, z;
    Maps::getSize(x, y, z);
    //out.print("map size in blocks: %d, %d, %d\n", x,y,z);
    for (int ix = 0; ix < x; ++ix) {
        for (int iy = 0; iy < y; ++iy) {
            for (int iz = z - 1; iz > 0; --iz) {
                df::map_block* top = Maps::getBlock(ix, iy, iz);
                df::map_block* bottom = Maps::getBlock(ix, iy, iz - 1);
                if(top && bottom && (top->flags.bits.designated || bottom->flags.bits.designated)) {
                    foreach_tile(out, top, bottom);
                }
            }
        }
    }
}

void ChannelManager::foreach_tile(color_ostream &out, df::map_block* top, df::map_block* bottom) {
    /** Safety checks
     * is the top tile being channeled or designated to be channeled
     * - Yes
     *      is the bottom tile being dug or designated and not marked
     *      - Yes
     *           try to cancelJob
     *           mark it for later
     * - No
     *      is the bottom tile designated
     *      - Yes
     *           unmark for later
     */
    for (int x = 0; x < 16; ++x) {
        for (int y = 0; y < 16; ++y) {
            auto &top_d = top->designation[x][y];
            auto &top_o = top->occupancy[x][y];
            auto &bottom_d = bottom->designation[x][y];
            auto &bottom_o = bottom->occupancy[x][y];
            df::job* top_job = find_job(top->map_pos);
            df::job* bottom_job = find_job(bottom->map_pos);
            if (top_job || is_channel(top_d)) {
                if (bottom_job || (!is_marked(bottom_o) && is_designated(bottom_d))) {
                    cancelJob(bottom_job);
                    bottom_o.bits.dig_marked = true;
                }
            } else if (is_marked(bottom_o) && is_channel(bottom_d)) {
                bottom_o.bits.dig_marked = false;
            }
        }
    }
}

void ChannelManager::find_dig_jobs() {
    df::job_list_link* p = df::global::world->jobs.list.next;
    while (p) {
        df::job* job = p->item;
        p = p->next;
        if (is_dig(job) || is_channel(job)) {
            dig_jobs.emplace(job->pos.z, job);
        }
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

bool is_dig(df::job* job) {
    return job->job_type == df::job_type::Dig;
}

bool is_dig(df::tile_designation &designation) {
    return designation.bits.dig == df::tile_dig_designation::Default;
}

bool is_channel(df::job* job){
    return job->job_type == df::job_type::DigChannel;
}

bool is_channel(df::tile_designation &designation){
    return designation.bits.dig == df::tile_dig_designation::Channel;
}

bool is_designated(df::tile_designation &designation){
    return is_dig(designation) || is_channel(designation);
}

bool is_marked(df::tile_occupancy &occupancy){
    return occupancy.bits.dig_marked;
}