#include "Debug.h"
#include "PluginManager.h"

#include "modules/EventManager.h"
#include "modules/Persistence.h"
#include "modules/World.h"

using std::string;
using std::vector;
using namespace DFHack;

DFHACK_PLUGIN("work-now");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(process_jobs);
REQUIRE_GLOBAL(process_dig);

namespace DFHack {
    DBG_DECLARE(worknow, log, DebugCategory::LINFO);
}

static const string CONFIG_KEY = string(plugin_name) + "/config";
static PersistentDataItem config;

enum ConfigValues {
    CONFIG_IS_ENABLED = 0,
};

static int get_config_val(PersistentDataItem &c, int index) {
    if (!c.isValid())
        return -1;
    return c.ival(index);
}
static bool get_config_bool(PersistentDataItem &c, int index) {
    return get_config_val(c, index) == 1;
}
static void set_config_val(PersistentDataItem &c, int index, int value) {
    if (c.isValid())
        c.ival(index) = value;
}
static void set_config_bool(PersistentDataItem &c, int index, bool value) {
    set_config_val(c, index, value ? 1 : 0);
}

static command_result work_now(color_ostream& out, vector<string>& parameters);

static void jobCompletedHandler(color_ostream& out, void* ptr);
EventManager::EventHandler handler(jobCompletedHandler,1);

DFhackCExport command_result plugin_init(color_ostream& out, std::vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand(
        "work-now",
        "Reduce the time that dwarves idle after completing a job.",
        work_now));

    return CR_OK;
}

static void cleanup() {
    EventManager::unregister(EventManager::EventType::JOB_COMPLETED, handler, plugin_self);
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("Cannot enable %s without a loaded world.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(log,out).print("%s from the API; persisting\n",
                                is_enabled ? "enabled" : "disabled");
        set_config_bool(config, CONFIG_IS_ENABLED, is_enabled);
        if (enable)
            EventManager::registerListener(EventManager::EventType::JOB_COMPLETED, handler, plugin_self);
        else
            cleanup();
    } else {
        DEBUG(log,out).print("%s from the API, but already %s; no action\n",
                                is_enabled ? "enabled" : "disabled",
                                is_enabled ? "enabled" : "disabled");
    }

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out ) {
    cleanup();
    return CR_OK;
}

DFhackCExport command_result plugin_load_data (color_ostream &out) {
    config = World::GetPersistentSiteData(CONFIG_KEY);

    if (!config.isValid()) {
        DEBUG(log,out).print("no config found in this save; initializing\n");
        config = World::AddPersistentSiteData(CONFIG_KEY);
        set_config_bool(config, CONFIG_IS_ENABLED, is_enabled);
    }

    is_enabled = get_config_bool(config, CONFIG_IS_ENABLED);
    DEBUG(log,out).print("loading persisted enabled state: %s\n",
                            is_enabled ? "true" : "false");

    return CR_OK;
}

static void poke_idlers() {
    *process_jobs = true;
    *process_dig  = true;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event e) {
    if (e == SC_PAUSED) {
        DEBUG(log,out).print("game paused; poking idlers\n");
        poke_idlers();
    } else if (e == DFHack::SC_WORLD_UNLOADED) {
        if (is_enabled) {
            DEBUG(log,out).print("world unloaded; disabling %s\n",
                                    plugin_name);
            is_enabled = false;
        }
    }

    return CR_OK;
}

static command_result work_now(color_ostream& out, vector<string>& parameters) {
    if (parameters.empty() || parameters[0] == "status") {
        out.print("work_now is %sactively poking idle dwarves.\n", is_enabled ? "" : "not ");
        return CR_OK;
    }

    return CR_WRONG_USAGE;
}

static void jobCompletedHandler(color_ostream& out, void* ptr) {
    DEBUG(log,out).print("job completed; poking idlers\n");
    poke_idlers();
}
