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

DFhackCExport command_result plugin_shutdown ( color_ostream &out ) {
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate ( color_ostream &out ) {
    if ( !DFHack::Core::getInstance().getWorld()->ReadPauseState() )
        return CR_OK;
    *df::global::process_jobs = true;
    return CR_OK;
}

DFhackCExport command_result workNow(color_ostream& out, vector<string>& parameters);

DFhackCExport command_result plugin_init(color_ostream& out, std::vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand("workNow", "makes dwarves look for jobs every time you pause", workNow, false, "Full help."));

    return CR_OK;
}

DFhackCExport command_result workNow(color_ostream& out, vector<string>& parameters) {
    
    
    return CR_OK;
}


