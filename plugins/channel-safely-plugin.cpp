/* Prevent channeling down into known open space.
Author:  Josh Cooper
Created: Aug. 4 2020
Updated: Jun. 29 2021
*/

#include <PluginManager.h>
#include <modules/EventManager.h>
#include "channel-safely.h"

// the only use is to allow reliance on pull request #1876 which introduces a refactor which prevents some manual management
#include <type_traits>
#define DECLARE_HASA(what) \
template<typename T, typename = int> struct has_##what : std::false_type { };\
template<typename T> struct has_##what<T, decltype((void) T::what, 0)> : std::true_type {};
DECLARE_HASA(when) //declares above statement with 'when' replacing 'what'
// end usage is: `has_when<T>::value`

using namespace DFHack;

DFHACK_PLUGIN("channel-safely");
DFHACK_PLUGIN_IS_ENABLED(enabled);
REQUIRE_GLOBAL(world);
//#define CS_DEBUG 1
bool useTicks = true;
uint16_t tickFreq = 1200;

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable);
void onNewTickEvent(color_ostream &out, void* tick_ptr);
void onStart(color_ostream &out, void* job);
void onComplete(color_ostream &out, void* job);

command_result manage_channel_designations(color_ostream &out, std::vector<std::string> &parameters) {
#ifdef CS_DEBUG
    debug_out = &out;
#endif
    namespace EM = EventManager;
    using namespace EM::EventType;
    if (debug_out) debug_out->print("manage_channel_designations()\n");
    // manually trigger managing designations
    if (parameters.empty()) {
        if (debug_out) debug_out->print("mcd->manage_designations()\n");
        ChannelManager::Get().manage_designations(out);
        if (!enabled) {
            ChannelManager::Get().delete_groups();
        }
    }
    // enable options
    else if (parameters.size() == 1 && parameters[0] == "enable") {
        return plugin_enable(out, true);
    } else if (parameters.size() == 2 && parameters[0] == "enable" && parameters[1] == "cheats") {
        cheat_mode = true;
    } else if (parameters.size() == 2 && parameters[0] == "enable" && parameters[1] == "ticks") {
        if(!useTicks){
            useTicks = true;
            if (enabled) {
                EM::EventHandler tickHandler(onNewTickEvent, tickFreq);
                if (!has_when<EM::EventHandler>::value) {
                    EM::registerTick(tickHandler, tickFreq, plugin_self);
                } else {
                    EM::registerListener(EventType::TICK, tickHandler, plugin_self);
                }
            }
        }
    }
    // disable options
    else if (parameters.size() == 1 && parameters[0] == "disable") {
        return plugin_enable(out, false);
    } else if (parameters.size() == 2 && parameters[0] == "disable" && parameters[1] == "cheats") {
        cheat_mode = false;
    } else if (parameters.size() == 2 && parameters[0] == "disable" && parameters[1] == "ticks") {
        if (useTicks) {
            useTicks = false;
            if (enabled) {
                EM::unregisterAll(plugin_self);
                EM::EventHandler jobStartHandler(onStart, 0);
                EM::EventHandler jobCompletionHandler(onComplete, 0);
                EM::registerListener(EventType::JOB_INITIATED, jobStartHandler, plugin_self);
                EM::registerListener(EventType::JOB_COMPLETED, jobCompletionHandler, plugin_self);
            }
        }
    }
    // developer debug
    else if (parameters.size() == 1 && parameters[0] == "debug") {
        debug_out = &out;
        ChannelManager::Get().debug();
        debug_out = nullptr;
    } else {
        debug_out = nullptr;
        return CR_FAILURE;
    }
    debug_out = nullptr;
    return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream &out, std::vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand("channel-safely",
                                     "Automatically manage channel designations.",
                                     manage_channel_designations,
                                     false,
                                     "channel-safely <option> <args>\n"
                                     "With no options given, the command will check all current designations\n"
                                     "and manage them each accordingly.\n"
                                     "\n"
                                     "You can use this plugin to manage your channel designations\n"
                                     "safely. It will mark unsafe designations and unmark safe ones.\n"
                                     "\n"
                                     "Using cheat mode it will even safely get your impossible designations done.\n"
                                     "\n"
                                     "options:\n"
                                     "\tenable (no args)  -enables the plugin's to run on (un)pause/tick/job/map-load events.\n"
                                     "\tdisable (no args) -disables the plugin's to run on (un)pause/tick/job/map-load events.\n"
                                     "\ttick-freq <value> -set's the tick event frequency. Default: 1200 (1 day)\n"
                                     "\n"
                                     "This plugin has multiple modes that can be toggled on/off.\n"
                                     "(usage: enable/disable <mode>)\n"
                                     "modes:\n"
                                     "\tcheats     -enables the plugin's ability to instantly dig\n"
                                     "\t            permanently unsafe designations.\n"
                                     "\t            (disabled by default)\n"
                                     "\tticks      -enables the plugin to run on tick events specifically.\n"
                                     "\t            (enabled by default)\n"
                                     "\n"));
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    namespace EM = EventManager;
#ifdef CS_DEBUG
    debug_out = &out;
    out.print("debugging enabled\n");
#endif
    if (enable && !enabled) {
        using namespace EM::EventType;
        EM::EventHandler tickHandler(onNewTickEvent, tickFreq);
        EM::EventHandler jobStartHandler(onStart, 0);
        EM::EventHandler jobCompletionHandler(onComplete, 0);
        if (!has_when<EM::EventHandler>::value) {
            EM::registerTick(tickHandler, tickFreq, plugin_self);
        } else {
            EM::registerListener(EventType::TICK, tickHandler, plugin_self);
        }
        EM::registerListener(EventType::JOB_INITIATED, jobStartHandler, plugin_self);
        EM::registerListener(EventType::JOB_COMPLETED, jobCompletionHandler, plugin_self);
        ChannelManager::Get().manage_designations(out);
        out.print("channel-safely enabled!\n");
    } else if (!enable) {
        ChannelManager::Get().delete_groups();
        EM::unregisterAll(plugin_self);
        out.print("channel-safely disabled!\n");
    }
    enabled = enable;
    debug_out = nullptr;
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
#ifdef CS_DEBUG
    debug_out = &out;
#endif
    if (debug_out) debug_out->print("onstatechange()\n");
    if (enabled && World::isFortressMode() && Maps::IsValid()) {
        switch (event) {
            case SC_UNKNOWN:
                if (debug_out) debug_out->print("SC_UNKNOWN\n");
                break;
            case SC_WORLD_LOADED:
                if (debug_out) debug_out->print("SC_WORLD_LOADED\n");
                break;
            case SC_WORLD_UNLOADED:
                if (debug_out) debug_out->print("SC_WORLD_UNLOADED\n");
                break;
            case SC_MAP_LOADED:
                if (debug_out) debug_out->print("SC_MAP_LOADED\n");
                ChannelManager::Get().manage_designations(out);
                break;
            case SC_MAP_UNLOADED:
                if (debug_out) debug_out->print("SC_MAP_UNLOADED\n");
                break;
            case SC_VIEWSCREEN_CHANGED:
                break;
            case SC_CORE_INITIALIZED:
                if (debug_out) debug_out->print("SC_CORE_INITIALIZED\n");
                break;
            case SC_BEGIN_UNLOAD:
                if (debug_out) debug_out->print("SC_BEGIN_UNLOAD\n");
                break;
            case SC_PAUSED:
                if (debug_out) debug_out->print("SC_PAUSED\n");
                ChannelManager::Get().manage_designations(out);
                break;
            case SC_UNPAUSED:
                if (debug_out) debug_out->print("SC_UNPAUSED\n");
                ChannelManager::Get().manage_designations(out);
                break;
        }
    }
    debug_out = nullptr;
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    namespace EM = EventManager;
    EM::unregisterAll(plugin_self);
    enabled = false;
    return CR_OK;
}

void onNewTickEvent(color_ostream &out, void* tick_ptr) {
#ifdef CS_DEBUG
    debug_out = &out;
#endif
    if (debug_out) debug_out->print("onNewHour()\n");
    if (enabled && World::isFortressMode() && Maps::IsValid()) {
        static int32_t last_tick_counter = 0;
        int32_t tick_counter = (int32_t) ((intptr_t) tick_ptr);
        if ((tick_counter - last_tick_counter) >= tickFreq) {
            last_tick_counter = tick_counter;
            ChannelManager::Get().manage_designations(out);
        }
    }
    namespace EM = EventManager;
    if (!has_when<EM::EventHandler>::value) {
        EM::EventHandler tickHandler(onNewTickEvent, tickFreq);
        EM::registerTick(tickHandler, tickFreq, plugin_self);
    }
    if (debug_out) debug_out->print("onNewHour() - return\n");
    debug_out = nullptr;
}

void onStart(color_ostream &out, void* job_ptr) {
#ifdef CS_DEBUG
    debug_out = &out;
#endif
    if (debug_out) debug_out->print("onStart()\n");
    if (enabled && World::isFortressMode() && Maps::IsValid()) {
        auto job = (df::job*) job_ptr;
        if (is_dig(job) || is_channel(job)) {
            df::coord local(job->pos);
            local.x = local.x % 16;
            local.y = local.y % 16;
            df::coord above(job->pos);
            above.z++;
            df::map_block* block = Maps::getTileBlock(job->pos);
            //postpone job if above isn't done
            ChannelManager::Get().manage_safety(out, block, local, job->pos, above);
        }
    }
    if (debug_out) debug_out->print("onStart() - return\n");
    debug_out = nullptr;
}

void onComplete(color_ostream &out, void* job_ptr) {
#ifdef CS_DEBUG
    debug_out = &out;
#endif
    if (debug_out) debug_out->print("onComplete()\n");
    if (enabled && World::isFortressMode() && Maps::IsValid()) {
        auto job = (df::job*) job_ptr;
        if (is_channel(job)) {
            df::coord local(job->pos);
            local.x = local.x % 16;
            local.y = local.y % 16;
            df::coord below(job->pos);
            below.z--;
            df::map_block* block = Maps::getTileBlock(below);
            //activate designation below if group is done now, postpone if not
            if (debug_out) debug_out->print("mark_done()\n");
            ChannelManager::Get().mark_done(job->pos);
            if (debug_out) debug_out->print("manage_safety()\n");
            ChannelManager::Get().manage_safety(out, block, local, below, job->pos);
            if (debug_out) debug_out->print("manageNeighbours()\n");
            manageNeighbours(out, job->pos);
        }
    }
    if (debug_out) debug_out->print("onComplete() - return\n");
    debug_out = nullptr;
}