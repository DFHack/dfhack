/*
 * Stockflow plugin.
 * For best effect, place "stockflow enable" in your dfhack.init configuration,
 * or set AUTOENABLE to true.
 */

#include "uicommon.h"
#include "LuaTools.h"

#include "df/building_stockpilest.h"
#include "df/job.h"
#include "df/viewscreen_dwarfmodest.h"

#include "modules/Gui.h"
#include "modules/Maps.h"
#include "modules/World.h"

using namespace DFHack;
using namespace std;

using df::building_stockpilest;

DFHACK_PLUGIN("stockflow");
#define AUTOENABLE false
DFHACK_PLUGIN_IS_ENABLED(enabled);

REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(ui);

bool fast = false;
const char *tagline = "Allows the fortress bookkeeper to queue jobs through the manager.";
const char *usage = (
    "  stockflow enable\n"
    "    Enable the plugin.\n"
    "  stockflow disable\n"
    "    Disable the plugin.\n"
    "  stockflow fast\n"
    "    Enable the plugin in fast mode.\n"
    "  stockflow list\n"
    "    List any work order settings for your stockpiles.\n"
    "  stockflow status\n"
    "    Display whether the plugin is enabled.\n"
    "\n"
    "While enabled, the 'q' menu of each stockpile will have two new options:\n"
    "  j: Select a job to order, from an interface like the manager's screen.\n"
    "  J: Cycle between several options for how many such jobs to order.\n"
    "\n"
    "Whenever the bookkeeper updates stockpile records, new work orders will\n"
    "be placed on the manager's queue for each such selection, reduced by the\n"
    "number of identical orders already in the queue.\n"
    "\n"
    "In fast mode, new work orders will be enqueued once per day, instead of\n"
    "waiting for the bookkeeper.\n"
);

/*
 * Lua interface.
 * Currently calls out to Lua functions, but never back in.
 */
class LuaHelper {
public:
    void cycle(color_ostream &out) {
        bool found = false;

        if (fast) {
            // Ignore the bookkeeper; either gather or enqueue orders every cycle.
            found = !bookkeeping;
        } else {
            // Gather orders when the bookkeeper starts updating stockpile records,
            // and enqueue them when the job is done.
            for (df::job_list_link* link = &world->job_list; link != NULL; link = link->next) {
                if (link->item == NULL) continue;
                if (link->item->job_type == job_type::UpdateStockpileRecords) {
                    found = true;
                    break;
                }
            }
        }

        if (found) {
            // Entice the bookkeeper to spend less time update records.
            ui->bookkeeper_precision += ui->bookkeeper_precision >> 3;
            if (!bookkeeping) {
                command_method("start_bookkeeping", out);
                bookkeeping = true;
            }
        } else {
            // Entice the bookkeeper to update records more often.
            ui->bookkeeper_precision -= ui->bookkeeper_precision >> 5;
            ui->bookkeeper_cooldown -= ui->bookkeeper_cooldown >> 2;
            if (bookkeeping) {
                command_method("finish_bookkeeping", out);
                bookkeeping = false;
            }
        }
    }

    void init() {
        stockpile_id = -1;
        initialized = false;
        bookkeeping = false;
    }

    bool reset(color_ostream &out, bool load) {
        stockpile_id = -1;
        bookkeeping = false;
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

        // Suspension is required for "stockflow enable" from the command line,
        // but may be overkill for other situations.
        CoreSuspender suspend;

        auto L = Lua::Core::State;
        Lua::StackUnwinder top(L);

        if (!lua_checkstack(L, 1))
            return false;

        if (!Lua::PushModulePublic(out, L, "plugins.stockflow", method))
            return false;

        if (!Lua::SafeCall(out, L, 0, 0))
            return false;

        return true;
    }

    bool stockpile_method(const char *method, building_stockpilest *sp) {
        // Combines the select_order and toggle_trigger method calls,
        // because they share the same signature.
        CoreSuspendClaimer suspend;

        auto L = Lua::Core::State;
        color_ostream_proxy out(Core::getInstance().getConsole());

        Lua::StackUnwinder top(L);

        if (!lua_checkstack(L, 2))
            return false;

        if (!Lua::PushModulePublic(out, L, "plugins.stockflow", method))
            return false;

        Lua::Push(L, sp);

        if (!Lua::SafeCall(out, L, 1, 0))
            return false;

        // Invalidate the string cache.
        stockpile_id = -1;

        return true;
    }

    bool collect_settings(building_stockpilest *sp) {
        // Find strings representing the job to order, and the trigger condition.
        // There might be a memory leak here; C++ is odd like that.
        auto L = Lua::Core::State;
        color_ostream_proxy out(Core::getInstance().getConsole());

        CoreSuspendClaimer suspend;
        Lua::StackUnwinder top(L);

        if (!lua_checkstack(L, 2))
            return false;

        if (!Lua::PushModulePublic(out, L, "plugins.stockflow", "stockpile_settings"))
            return false;

        Lua::Push(L, sp);

        if (!Lua::SafeCall(out, L, 1, 2))
            return false;

        if (!lua_isstring(L, -1))
            return false;

        current_trigger = lua_tostring(L, -1);
        lua_pop(L, 1);

        if (!lua_isstring(L, -1))
            return false;

        current_job = lua_tostring(L, -1);
        lua_pop(L, 1);

        stockpile_id = sp->id;

        return true;
    }

    void draw(building_stockpilest *sp) {
        if (sp->id != stockpile_id) {
            if (!collect_settings(sp)) {
                Core::printerr("Stockflow job collection failed!\n");
                return;
            }
        }

        auto dims = Gui::getDwarfmodeViewDims();
        int left_margin = dims.menu_x1 + 1;
        int x = left_margin;
        int y = dims.y2 - 3;

        int links = 0;
        links += sp->links.give_to_pile.size();
        links += sp->links.take_from_pile.size();
        links += sp->links.give_to_workshop.size();
        links += sp->links.take_from_workshop.size();
        if (links + 12 >= y)
           y += 1;

        OutputHotkeyString(x, y, current_job, "j", true, left_margin, COLOR_WHITE, COLOR_LIGHTRED);
        if (*current_trigger)
            OutputHotkeyString(x, y, current_trigger, "   J", true, left_margin, COLOR_WHITE, COLOR_LIGHTRED);
    }


private:
    long stockpile_id;
    bool initialized;
    bool bookkeeping;
    const char *current_job;
    const char *current_trigger;
};

static LuaHelper helper;

#define DELTA_TICKS 600

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (!enabled)
        return CR_OK;

    if (!Maps::IsValid())
        return CR_OK;

    static decltype(world->frame_counter) last_frame_count = 0;

    if (DFHack::World::ReadPauseState())
        return CR_OK;

    if (world->frame_counter - last_frame_count < DELTA_TICKS)
        return CR_OK;

    last_frame_count = world->frame_counter;

    helper.cycle(out);

    return CR_OK;
}


/*
 * Interface hooks
 */
struct stockflow_hook : public df::viewscreen_dwarfmodest {
    typedef df::viewscreen_dwarfmodest interpose_base;

    bool handleInput(set<df::interface_key> *input) {
        building_stockpilest *sp = get_selected_stockpile();
        if (!sp)
            return false;

        if (input->count(interface_key::CUSTOM_J)) {
            // Select a new order for this stockpile.
            if (!helper.stockpile_method("select_order", sp)) {
                Core::printerr("Stockflow order selection failed!\n");
            }

            return true;
        } else if (input->count(interface_key::CUSTOM_SHIFT_J)) {
            // Toggle the order trigger for this stockpile.
            if (!helper.stockpile_method("toggle_trigger", sp)) {
                Core::printerr("Stockflow trigger toggle failed!\n");
            }

            return true;
        }

        return false;
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input)) {
        if (!handleInput(input))
            INTERPOSE_NEXT(feed)(input);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ()) {
        INTERPOSE_NEXT(render)();

        building_stockpilest *sp = get_selected_stockpile();
        if (sp)
            helper.draw(sp);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(stockflow_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(stockflow_hook, render);


static bool apply_hooks(color_ostream &out, bool enabling) {
    if (enabling && !gps) {
        out.printerr("Stockflow needs graphics.\n");
        return false;
    }

    if (!INTERPOSE_HOOK(stockflow_hook, feed).apply(enabling) || !INTERPOSE_HOOK(stockflow_hook, render).apply(enabling)) {
        out.printerr("Could not %s stockflow hooks!\n", enabling? "insert": "remove");
        return false;
    }

    if (!helper.reset(out, enabling && Maps::IsValid())) {
        out.printerr("Could not reset stockflow world data!\n");
        return false;
    }

    return true;
}

static command_result stockflow_cmd(color_ostream &out, vector <string> & parameters) {
    bool desired = enabled;
    if (parameters.size() == 1) {
        if (parameters[0] == "enable" || parameters[0] == "on" || parameters[0] == "1") {
            desired = true;
            fast = false;
        } else if (parameters[0] == "disable" || parameters[0] == "off" || parameters[0] == "0") {
            desired = false;
            fast = false;
        } else if (parameters[0] == "fast" || parameters[0] == "always" || parameters[0] == "2") {
            desired = true;
            fast = true;
        } else if (parameters[0] == "usage" || parameters[0] == "help" || parameters[0] == "?") {
            out.print("%s: %s\nUsage:\n%s", plugin_name, tagline, usage);
            return CR_OK;
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
        if (!apply_hooks(out, desired)) {
            return CR_FAILURE;
        }
    }

    out.print("Stockflow is %s %s%s.\n", (desired == enabled)? "currently": "now", desired? "enabled": "disabled", fast? ", in fast mode": "");
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
        if (!apply_hooks(out, enable)) {
            return CR_FAILURE;
        }

        enabled = enable;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    helper.init();
    if (AUTOENABLE) {
        if (!apply_hooks(out, true)) {
            return CR_FAILURE;
        }

        enabled = true;
    }

    commands.push_back(PluginCommand(plugin_name, tagline, stockflow_cmd, false, usage));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    return plugin_enable(out, false);
}
