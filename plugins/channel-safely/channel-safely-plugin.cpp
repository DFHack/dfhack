/* Prevent channeling down into known open space.
Author:  Josh Cooper
Created: Aug. 4 2020
Updated: Jun. 29 2021
*/

#include <PluginManager.h>
#include <modules/EventManager.h>
#include "channel-safely.h"

using namespace DFHack;

DFHACK_PLUGIN("channel-safely");
DFHACK_PLUGIN_IS_ENABLED(enabled);
REQUIRE_GLOBAL(world);
//#define CS_DEBUG 1

command_result manage_channel_designations(color_ostream &out, std::vector<std::string> &parameters);
DFhackCExport command_result plugin_enable(color_ostream &out, bool enable);
void onJobStart(color_ostream &out, void* job_ptr);
void onJobComplete(color_ostream &out, void* job_ptr);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand("channel-safely",
                                     "Automatically manage channel designations.",
                                     manage_channel_designations,
                                     false,
                                     ""
                                     " channel-safely <option> <args>\n"
                                     "    You can use this plugin to manage your channel designations safely. It will\n"
                                     "    mark unsafe designations and unmark safe ones.\n"
                                     "\n"
                                     " channel-safely\n"
                                     "    Manually runs the channel-safely management procedure to (un)mark\n"
                                     "    designations as needed.\n"
                                     "\n"
                                     " channel-safely enable\n"
                                     "    Enables the plugin to automatically run the management procedure. The plugin\n"
                                     "    will run when a job starts/completes and on (un)pause/map load state changes.\n"
                                     " channel-safely enable cheats\n"
                                     "    Enables the plugin to instantly dig permanently unsafe designations.\n"
                                     "\n"
                                     " channel-safely disable\n"
                                     "    Disables the plugin. Will not automatically do anything.\n"
                                     " channel-safely disable cheats\n"
                                     "    Disables instantly digging permanently unsafe designations.\n"
                                     "\n"));
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
    } else if (parameters.size() == 2 && parameters[0] == "enable" && parameters[1] == "cheats" && !cheat_mode) {
        cheat_mode = true;
        out.print("channel-safely: cheat mode enabled!\n");
    }
    // disable options
    else if (parameters.size() == 1 && parameters[0] == "disable") {
        return plugin_enable(out, false);
    } else if (parameters.size() == 2 && parameters[0] == "disable" && parameters[1] == "cheats" && cheat_mode) {
        cheat_mode = false;
        out.print("channel-safely: ontick events disabled!\n");
    }
    // developer debug
    else if (parameters.size() == 1 && parameters[0] == "debug") {
        debug_out = &out;
        ChannelManager::Get().debug();
        debug_out = nullptr;
    } else if (parameters.size() == 1 && parameters[0] == "help") {
        return CR_WRONG_USAGE;
    } else {
        debug_out = nullptr;
        out.color(color_value::COLOR_RED);
        out.print("Invalid argument.\n\n");
        out.color(color_value::COLOR_RESET);
        return CR_WRONG_USAGE;
    }
    debug_out = nullptr;
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
        EM::EventHandler jobStartHandler(onJobStart, 0);
        EM::EventHandler jobCompletionHandler(onJobComplete, 0);
        EM::registerListener(EventType::JOB_INITIATED, jobStartHandler, plugin_self);
        EM::registerListener(EventType::JOB_COMPLETED, jobCompletionHandler, plugin_self);
        ChannelManager::Get().manage_designations(out);
        out.print("channel-safely: enabled!\n");
    } else if (!enable) {
        ChannelManager::Get().delete_groups();
        EM::unregisterAll(plugin_self);
        out.print("channel-safely: disabled!\n");
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
            case SC_MAP_LOADED:
                if (debug_out) debug_out->print("SC_MAP_LOADED\n");
                ChannelManager::Get().manage_designations(out);
                break;
            case SC_PAUSED:
                if (debug_out) debug_out->print("SC_PAUSED\n");
                ChannelManager::Get().manage_designations(out);
                break;
            case SC_UNPAUSED:
                if (debug_out) debug_out->print("SC_UNPAUSED\n");
                ChannelManager::Get().manage_designations(out);
                break;
            case SC_WORLD_UNLOADED:
            case SC_MAP_UNLOADED:
                ChannelManager::Get().delete_groups();
                break;
            case SC_UNKNOWN:
            case SC_WORLD_LOADED:
            case SC_VIEWSCREEN_CHANGED:
            case SC_CORE_INITIALIZED:
            case SC_BEGIN_UNLOAD:
                break;
        }
    }
    debug_out = nullptr;
    return CR_OK;
}

void onJobStart(color_ostream &out, void* job_ptr) {
#ifdef CS_DEBUG
    debug_out = &out;
#endif
    if (debug_out) debug_out->print("onJobStart()\n");
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
    if (debug_out) debug_out->print("onJobStart() - return\n");
    debug_out = nullptr;
}

void onJobComplete(color_ostream &out, void* job_ptr) {
#ifdef CS_DEBUG
    debug_out = &out;
#endif
    if (debug_out) debug_out->print("onJobComplete()\n");
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
    if (debug_out) debug_out->print("onJobComplete() - return\n");
    debug_out = nullptr;
}