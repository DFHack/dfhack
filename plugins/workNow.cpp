#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "DataDefs.h"

#include "modules/EventManager.h"
#include "modules/World.h"

#include "df/global_objects.h"

#include <vector>

using namespace std;
using namespace DFHack;

DFHACK_PLUGIN("workNow");
REQUIRE_GLOBAL(process_jobs);
REQUIRE_GLOBAL(process_dig);

static int mode = 0;

DFhackCExport command_result workNow(color_ostream& out, vector<string>& parameters);

void jobCompletedHandler(color_ostream& out, void* ptr);
EventManager::EventHandler handler(jobCompletedHandler,1);

DFhackCExport command_result plugin_init(color_ostream& out, std::vector<PluginCommand> &commands) {
    if (!process_jobs || !process_dig)
        return CR_FAILURE;

    commands.push_back(PluginCommand("workNow", "Don't allow dwarves to idle if any jobs are available", workNow, false, "When workNow is active, every time the game pauses, DF will make dwarves perform any appropriate available jobs. This includes when you one step through the game using the pause menu. When workNow is in mode 2, it will make dwarves look for jobs every time a job completes.\n"
                "workNow\n"
                "  print workNow status\n"
                "workNow 0\n"
                "  deactivate workNow\n"
                "workNow 1\n"
                "  activate workNow (look for jobs on pause, and only then)\n"
                "workNow 2\n"
                "  make dwarves look for jobs whenever a job completes\n"
    ));

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out ) {
    mode = 0;
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event e) {
    if ( !mode )
        return CR_OK;
    if ( e == DFHack::SC_WORLD_UNLOADED ) {
        mode = 0;
        return CR_OK;
    }
    if ( e != DFHack::SC_PAUSED )
        return CR_OK;

    *process_jobs = true;
    *process_dig  = true;

    return CR_OK;
}

DFhackCExport command_result workNow(color_ostream& out, vector<string>& parameters) {
    if ( parameters.size() == 0 ) {
        out.print("workNow status = %d\n", mode);
        return CR_OK;
    }
    if ( parameters.size() > 1 ) {
        return CR_WRONG_USAGE;
    }
    int32_t a = atoi(parameters[0].c_str());

    if (a < 0 || a > 2)
        return CR_WRONG_USAGE;

    if ( a == 2 && mode != 2 ) {
        EventManager::registerListener(EventManager::EventType::JOB_COMPLETED, handler, plugin_self);
    } else if ( mode == 2 && a != 2 ) {
        EventManager::unregister(EventManager::EventType::JOB_COMPLETED, handler, plugin_self);
    }

    mode = a;
    out.print("workNow status = %d\n", mode);

    return CR_OK;
}

void jobCompletedHandler(color_ostream& out, void* ptr) {
    if ( mode < 2 )
        return;

    *process_jobs = true;
    *process_dig = true;
}
