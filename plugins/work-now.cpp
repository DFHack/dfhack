#include "Debug.h"
#include "PluginManager.h"

#include "modules/EventManager.h"

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

DFhackCExport command_result work_now(color_ostream& out, vector<string>& parameters);

static void jobCompletedHandler(color_ostream& out, void* ptr);
EventManager::EventHandler handler(jobCompletedHandler,1);

DFhackCExport command_result plugin_init(color_ostream& out, std::vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand(
        "work-now",
        "Reduce the time that dwarves idle after completing a job.",
        work_now));

    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (enable != is_enabled)
    {
        if (enable)
            EventManager::registerListener(EventManager::EventType::JOB_COMPLETED, handler, plugin_self);
        else
            EventManager::unregister(EventManager::EventType::JOB_COMPLETED, handler, plugin_self);

        is_enabled = enable;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out ) {
    return plugin_enable(out, false);
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event e) {
    if (e == SC_PAUSED) {
        DEBUG(log,out).print("game paused; poking idlers\n");
        *process_jobs = true;
        *process_dig  = true;
    }

    return CR_OK;
}

DFhackCExport command_result work_now(color_ostream& out, vector<string>& parameters) {
    if (parameters.empty() || parameters[0] == "status") {
        out.print("work_now is %sactively poking idle dwarves.\n", is_enabled ? "" : "not ");
        return CR_OK;
    }

    return CR_WRONG_USAGE;
}

static void jobCompletedHandler(color_ostream& out, void* ptr) {
    DEBUG(log,out).print("job completed; poking idlers\n");
    *process_jobs = true;
    *process_dig = true;
}
