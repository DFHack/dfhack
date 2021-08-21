/* Prevent channeling down into known open space.
Author:  Josh Cooper
Created: Aug. 4 2020
Updated: Jun. 15 2021
*/

#include <PluginManager.h>
#include <modules/EventManager.h>


#include "channel-safely.h"

using namespace DFHack;

DFHACK_PLUGIN("channel-safely");
DFHACK_PLUGIN_IS_ENABLED(enabled);
REQUIRE_GLOBAL(world);
//#define hourTicks 50
#define dayTicks 1200 //ie. fullBuildFreq = 24 dwarf hours

#include <type_traits>

#define DECLARE_HASA(what) \
template<typename T, typename = int> struct has_##what : std::false_type { };\
template<typename T> struct has_##what<T, decltype((void) T::what, 0)> : std::true_type {};
DECLARE_HASA(when) //declares above statement with 'when' replacing 'what'
// end usage is: `has_when<T>::value`
// the only use is to allow reliance on pull request #1876 which introduces a refactor which prevents some manual management
//#define CS_DEBUG 1

#ifdef hourTicks
void onNewHour(color_ostream &out, void* tick_ptr);
#endif
#ifdef dayTicks
void onNewDay(color_ostream &out, void* tick_ptr);
#endif
void onStart(color_ostream &out, void* job);
void onComplete(color_ostream &out, void* job);

command_result manage_channel_designations(color_ostream &out, std::vector<std::string> &parameters);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand("channel-safely",
                                     "A tool to manage active channel designations.",
                                     manage_channel_designations,
                                     false,
                                     "\n"));
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    namespace EM = EventManager;
#ifdef CS_DEBUG
    debug_out = &out;
#endif
    if (enable && !enabled) {
        using namespace EM::EventType;
#ifdef hourTicks
        EM::EventHandler hoursHandler(onNewHour, hourTicks);
#endif
#ifdef dayTicks
        EM::EventHandler daysHandler(onNewDay, dayTicks);
#endif
        EM::EventHandler jobStartHandler(onStart, 0);
        EM::EventHandler jobCompletionHandler(onComplete, 0);
        if (!has_when<EM::EventHandler>::value) {
#ifdef hourTicks
            EM::registerTick(hoursHandler, hourTicks, plugin_self);
#endif
#ifdef dayTicks
            EM::registerTick(daysHandler, dayTicks, plugin_self);
#endif
        } else {
#ifdef hourTicks
            EM::registerListener(EventType::TICK, hoursHandler, plugin_self);
#endif
#ifdef dayTicks
            EM::registerListener(EventType::TICK, daysHandler, plugin_self);
#endif
        }
        EM::registerListener(EventType::JOB_INITIATED, jobStartHandler, plugin_self);
        EM::registerListener(EventType::JOB_COMPLETED, jobCompletionHandler, plugin_self);
        manager.manage_designations(out);
        out.print("channel-safely enabled!\n");
    } else if (!enable) {
        manager.delete_groups();
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
                manager.manage_designations(out);
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
                manager.manage_designations(out);
                break;
            case SC_UNPAUSED:
                if (debug_out) debug_out->print("SC_UNPAUSED\n");
                manager.manage_designations(out);
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

command_result manage_channel_designations(color_ostream &out, std::vector<std::string> &parameters) {
#ifdef CS_DEBUG
    debug_out = &out;
#endif
    if (debug_out) debug_out->print("manage_channel_designations()\n");
    if (parameters.empty()) {
        if (debug_out) debug_out->print("mcd->manage_designations()\n");
        manager.manage_designations(out);
        if (!enabled) {
            manager.delete_groups();
        }
    } else if (parameters.size() == 1 && parameters[0] == "enable") {
        return plugin_enable(out, true);
    } else if (parameters.size() == 2 && parameters[0] == "enable" && parameters[1] == "cheats") {
        cheat_mode = true;
    } else if (parameters.size() == 1 && parameters[0] == "disable") {
        return plugin_enable(out, false);
    } else if (parameters.size() == 1 && parameters[0] == "debug") {
        debug_out = &out;
        manager.debug();
        debug_out = nullptr;
    } else {
        debug_out = nullptr;
        return CR_FAILURE;
    }
    debug_out = nullptr;
    return CR_OK;
}

#ifdef hourTicks
void onNewHour(color_ostream &out, void* tick_ptr) {
#ifdef CS_DEBUG
    debug_out = &out;
#endif
    if (debug_out) debug_out->print("onNewHour()\n");
    if (enabled && World::isFortressMode() && Maps::IsValid()) {
        static int32_t last_tick_counter = 0;
        int32_t tick_counter = (int32_t) ((intptr_t) tick_ptr);
        if ((tick_counter - last_tick_counter) >= hourTicks) {
            last_tick_counter = tick_counter;
            manager.manage_designations(out);
        }
    }
    namespace EM = EventManager;
    if (!has_when<EM::EventHandler>::value) {
        EM::EventHandler tickHandler(onNewHour, hourTicks);
        EM::registerTick(tickHandler, hourTicks, plugin_self);
    }
    if (debug_out) debug_out->print("onNewHour() - return\n");
    debug_out = nullptr;
}
#endif

#ifdef dayTicks
void onNewDay(color_ostream &out, void* tick_ptr) {
#ifdef CS_DEBUG
    debug_out = &out;
#endif
    if (debug_out) debug_out->print("onNewHour()\n");
    if (enabled && World::isFortressMode() && Maps::IsValid()) {
        static int32_t last_tick_counter = 0;
        int32_t tick_counter = (int32_t) ((intptr_t) tick_ptr);
        if ((tick_counter - last_tick_counter) >= dayTicks) {
            last_tick_counter = tick_counter;
            manager.manage_designations(out);
        }
    }
    namespace EM = EventManager;
    if (!has_when<EM::EventHandler>::value) {
        EM::EventHandler tickHandler(onNewDay, dayTicks);
        EM::registerTick(tickHandler, dayTicks, plugin_self);
    }
    if (debug_out) debug_out->print("onNewHour() - return\n");
    debug_out = nullptr;
}
#endif

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
            manager.manage_safety(out, block, local, job->pos, above);
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
            manager.mark_done(job->pos);
            if (debug_out) debug_out->print("manage_safety()\n");
            manager.manage_safety(out, block, local, below, job->pos);
            if (debug_out) debug_out->print("manageNeighbours()\n");
            manageNeighbours(out, job->pos);
        }
    }
    if (debug_out) debug_out->print("onComplete() - return\n");
    debug_out = nullptr;
}