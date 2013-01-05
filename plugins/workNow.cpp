#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "DataDefs.h"
#include "modules/World.h"
#include "df/global_objects.h"

#include <vector>
using namespace std;
using namespace DFHack;

DFHACK_PLUGIN("workNow");

static bool active = false;

DFhackCExport command_result workNow(color_ostream& out, vector<string>& parameters);

DFhackCExport command_result plugin_init(color_ostream& out, std::vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand("workNow", "makes dwarves look for jobs every time you pause", workNow, false, "When workNow is active, every time the game pauses, DF will make dwarves perform any appropriate available jobs. This includes when you one step through the game using the pause menu.\n"
                "workNow 1\n"
                "  activate workNow\n"
                "workNow 0\n"
                "  deactivate workNow\n"));

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out ) {
    active = false;
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event e) {
    if ( !active )
        return CR_OK;
    if ( e == DFHack::SC_WORLD_UNLOADED ) {
        active = false;
        return CR_OK;
    }
    if ( e != DFHack::SC_PAUSED )
        return CR_OK;
    
    *df::global::process_jobs = true;
    *df::global::process_dig = true;
    
    return CR_OK;
}



DFhackCExport command_result workNow(color_ostream& out, vector<string>& parameters) {
    if ( parameters.size() == 0 ) {
        out.print("workNow status = %s\n", active ? "active" : "inactive");
        return CR_OK;
    }
    if ( parameters.size() > 1 ) {
        return CR_WRONG_USAGE;
    }
    int32_t a = atoi(parameters[0].c_str());
    
    if (a < 0 || a > 1)
        return CR_WRONG_USAGE;

    active = (bool)a;
    
    return CR_OK;
}


