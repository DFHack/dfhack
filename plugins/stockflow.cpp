/*
 * Stockflow plugin.
 * For best effect, place "stockflow enable" in your dfhack.init configuration,
 * or set AUTOENABLE to true.
 */

#include "LuaTools.h"
#include "PluginManager.h"

#include "modules/Maps.h"
#include "modules/World.h"

#include "df/global_objects.h"
#include "df/world.h"

using namespace DFHack;

DFHACK_PLUGIN("stockflow");
#define AUTOENABLE false
DFHACK_PLUGIN_IS_ENABLED(enabled);

REQUIRE_GLOBAL(world);

/*
 * Lua interface.
 * Currently calls out to Lua functions, but never back in.
 */
class LuaHelper {
public:
    void cycle(color_ostream &out) {
        // Gather orders and enqueue them in a single pass. This used to be
        // gated on the bookkeeper's UpdateStockpileRecords job, but that job
        // does not run reliably due to a base game bug.
        command_method("start_bookkeeping", out);
        command_method("finish_bookkeeping", out);
    }

    void init() {
        initialized = false;
    }

    bool reset(color_ostream &out, bool load) {
        if (load) {
            return initialized = command_method("initialize_world", out);
        } else if (initialized) {
            initialized = false;
            return command_method("clear_caches", out);
        }

        return true;
    }

    bool command_method(const char *method, color_ostream &out) {
        // Calls a lua function with no parameters.
        return Lua::CallLuaModuleFunction(out, "plugins.stockflow", method);
    }

private:
    bool initialized;
};

static LuaHelper helper;

// Once per in-game day, more or less. A prime number is used so that
// periodic tools don't all fire on the same tick.
constexpr auto DELTA_TICKS = 1187;

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (!Maps::IsValid())
        return CR_OK;

    if (DFHack::World::ReadPauseState())
        return CR_OK;

    if (world->frame_counter % DELTA_TICKS != 0)
        return CR_OK;

    helper.cycle(out);

    return CR_OK;
}

static bool apply_state(color_ostream &out, bool enabling) {
    if (!helper.reset(out, enabling && Maps::IsValid())) {
        out.printerr("Could not reset stockflow world data!\n");
        return false;
    }

    return true;
}

static command_result stockflow_cmd(color_ostream &out, std::vector<std::string> & parameters) {
    bool desired = enabled;
    if (parameters.size() == 1) {
        if (parameters[0] == "enable" || parameters[0] == "on" || parameters[0] == "1") {
            desired = true;
        } else if (parameters[0] == "disable" || parameters[0] == "off" || parameters[0] == "0") {
            desired = false;
        } else if (parameters[0] == "usage" || parameters[0] == "help" || parameters[0] == "?") {
            return CR_WRONG_USAGE;
        } else if (parameters[0] == "list") {
            if (!enabled) {
                out.printerr("Stockflow is not currently enabled.\n");
                return CR_FAILURE;
            }

            if (!Maps::IsValid()) {
                out.printerr("You haven't loaded a map yet.\n");
                return CR_FAILURE;
            }

            // Tell Lua to list any saved stockpile orders.
            return helper.command_method("list_orders", out)? CR_OK: CR_FAILURE;
        } else if (parameters[0] != "status") {
            return CR_WRONG_USAGE;
        }
    } else if (parameters.size() > 1) {
        return CR_WRONG_USAGE;
    }

    if (desired != enabled) {
        if (!apply_state(out, desired)) {
            return CR_FAILURE;
        }
    }

    out.print("Stockflow is {} {}.\n", (desired == enabled)? "currently": "now", desired? "enabled": "disabled");
    enabled = desired;
    return CR_OK;
}


DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == DFHack::SC_MAP_LOADED) {
        if (!helper.reset(out, enabled)) {
            out.printerr("Could not load stockflow world data!\n");
            return CR_FAILURE;
        }
    } else if (event == DFHack::SC_MAP_UNLOADED) {
        if (!helper.reset(out, false)) {
            out.printerr("Could not unload stockflow world data!\n");
            return CR_FAILURE;
        }
    }

    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream& out, bool enable) {
    /* Accept the "enable stockflow"/"disable stockflow" syntax, where available. */
    /* Same as "stockflow enable"/"stockflow disable" except without the status line. */
    if (enable != enabled) {
        if (!apply_state(out, enable)) {
            return CR_FAILURE;
        }

        enabled = enable;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    helper.init();
    if (AUTOENABLE) {
        if (!apply_state(out, true)) {
            return CR_FAILURE;
        }

        enabled = true;
    }

    commands.push_back(PluginCommand(
        plugin_name,
        "Queue manager jobs based on free space in stockpiles.",
        stockflow_cmd));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    return plugin_enable(out, false);
}
