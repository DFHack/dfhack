#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "PluginLua.h"

#include "modules/Persistence.h"
#include "modules/World.h"

#include <df/report.h>
#include <df/unit.h>
#include <df/world.h>
#include <unordered_set>

using namespace DFHack;

DFHACK_PLUGIN("logcleaner");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);

static const std::string CONFIG_KEY = std::string(plugin_name) + "/config";
static PersistentDataItem config;

enum ConfigValues {
    CONFIG_IS_ENABLED = 0,
    CONFIG_CLEAR_COMBAT = 1,
    CONFIG_CLEAR_SPARING = 2,
    CONFIG_CLEAR_HUNTING = 3,
};

static bool clear_combat = false;
static bool clear_sparring = true;
static bool clear_hunting = false;

namespace DFHack {
    DBG_DECLARE(logcleaner, control, DebugCategory::LINFO);
    DBG_DECLARE(logcleaner, cleanup, DebugCategory::LINFO);
}

static void cleanupLogs(color_ostream& out);
static command_result do_command(color_ostream& out, std::vector<std::string>& params);
static void do_enable();
static void do_disable();

// Getter functions for Lua
static bool logcleaner_getCombat() { return clear_combat; }
static bool logcleaner_getSparring() { return clear_sparring; }
static bool logcleaner_getHunting() { return clear_hunting; }

// Setter functions for Lua (also persist to config)
static void logcleaner_setCombat(color_ostream& out, bool val) {
    clear_combat = val;
    config.set_bool(CONFIG_CLEAR_COMBAT, clear_combat);
}

static void logcleaner_setSparring(color_ostream& out, bool val) {
    clear_sparring = val;
    config.set_bool(CONFIG_CLEAR_SPARING, clear_sparring);
}

static void logcleaner_setHunting(color_ostream& out, bool val) {
    clear_hunting = val;
    config.set_bool(CONFIG_CLEAR_HUNTING, clear_hunting);
}

DFhackCExport command_result plugin_init(color_ostream& out, std::vector<PluginCommand>& commands) {
    commands.push_back(PluginCommand(
        plugin_name,
        "Prevent report buffer from filling up by clearing selected report types (combat, sparring, hunting).",
        do_command));

    return CR_OK;
}

static command_result do_command(color_ostream& out, std::vector<std::string>& params) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot use {} without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    bool show_help = false;
    if (!Lua::CallLuaModuleFunction(out, "plugins.logcleaner", "parse_commandline", params,
            1, [&](lua_State *L) {
                show_help = !lua_toboolean(L, -1);
            })) {
        return CR_FAILURE;
    }

    return show_help ? CR_WRONG_USAGE : CR_OK;
}

static void do_enable() {
}

static void do_disable() {
}

DFhackCExport command_result plugin_enable(color_ostream& out, bool enable) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot enable {} without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(control, out).print("{} from the API; persisting\n",
                                  is_enabled ? "enabled" : "disabled");
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
        if (enable)
            do_enable();
        else
            do_disable();
    } else {
        DEBUG(control, out).print("{} from the API, but already {}; no action\n",
                                  is_enabled ? "enabled" : "disabled",
                                  is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream& out) {
    DEBUG(control, out).print("shutting down {}\n", plugin_name);
    return CR_OK;
}

DFhackCExport command_result plugin_load_site_data(color_ostream& out) {
    config = World::GetPersistentSiteData(CONFIG_KEY);

    if (!config.isValid()) {
        DEBUG(control, out).print("no config found in this save; initializing\n");
        config = World::AddPersistentSiteData(CONFIG_KEY);
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
        config.set_bool(CONFIG_CLEAR_COMBAT, clear_combat);
        config.set_bool(CONFIG_CLEAR_SPARING, clear_sparring);
        config.set_bool(CONFIG_CLEAR_HUNTING, clear_hunting);
    }

    is_enabled = config.get_bool(CONFIG_IS_ENABLED);
    clear_combat = config.get_bool(CONFIG_CLEAR_COMBAT);
    clear_sparring = config.get_bool(CONFIG_CLEAR_SPARING);
    clear_hunting = config.get_bool(CONFIG_CLEAR_HUNTING);

    DEBUG(control, out).print("loading persisted enabled state: {}\n",
                              is_enabled ? "true" : "false");
    if (is_enabled)
        do_enable();

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream& out, state_change_event event) {
    if (event == DFHack::SC_WORLD_UNLOADED && is_enabled) {
        DEBUG(control, out).print("world unloaded; disabling {}\n", plugin_name);
        is_enabled = false;
        do_disable();
    }
    return CR_OK;
}

static void cleanupLogs(color_ostream& out) {
    if (!is_enabled || !world)
        return;

    // Collect all report IDs from unit combat/sparring/hunting logs
    std::unordered_set<int32_t> report_ids_to_remove;

    for (auto unit : world->units.all) {
        // Combat logs (index 0)
        if (clear_combat) {
            auto& log = unit->reports.log[0];
            for (auto report_id : log) {
                report_ids_to_remove.insert(report_id);
            }
            log.clear();
        }
        // Sparring logs (index 1)
        if (clear_sparring) {
            auto& log = unit->reports.log[1];
            for (auto report_id : log) {
                report_ids_to_remove.insert(report_id);
            }
            log.clear();
        }
        // Hunting logs (index 2)
        if (clear_hunting) {
            auto& log = unit->reports.log[2];
            for (auto report_id : log) {
                report_ids_to_remove.insert(report_id);
            }
            log.clear();
        }
    }

    if (report_ids_to_remove.empty())
        return;

    // Remove collected reports from global buffers
    auto& reports = world->status.reports;

    int reports_erased = 0;

    for (auto report_id : report_ids_to_remove) {
        df::report* report = df::report::find(report_id);
        if (!report)
            continue;

        auto it = std::find(reports.begin(), reports.end(), report);
        if (it != reports.end()) {
            delete report;
            reports.erase(it);
            reports_erased++;
        }
    }
}

DFhackCExport command_result plugin_onupdate(color_ostream& out, state_change_event event) {
    static int32_t tick_counter = 0;

    if (!is_enabled || !world)
        return CR_OK;

    tick_counter++;
    if (tick_counter >= 100) {
        tick_counter = 0;
        cleanupLogs(out);
    }

    return CR_OK;
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(logcleaner_getCombat),
    DFHACK_LUA_FUNCTION(logcleaner_getSparring),
    DFHACK_LUA_FUNCTION(logcleaner_getHunting),
    DFHACK_LUA_FUNCTION(logcleaner_setCombat),
    DFHACK_LUA_FUNCTION(logcleaner_setSparring),
    DFHACK_LUA_FUNCTION(logcleaner_setHunting),
    DFHACK_LUA_END
};
