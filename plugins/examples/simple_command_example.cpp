// This template is appropriate for plugins that simply provide one or more
// commands, but don't need to be "enabled" to function.

#include "Debug.h"
#include "PluginManager.h"

#include <string>
#include <vector>

using std::string;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("simple_command_example");

namespace DFHack {
    DBG_DECLARE(simple_command_example, log);
}

static command_result do_command(color_ostream &out, vector<string> &parameters);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    DEBUG(log,out).print("initializing %s\n", plugin_name);

    commands.push_back(PluginCommand(
        plugin_name,
        "Short (~54 character) description of command.",
        do_command));

    return CR_OK;
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    // TODO: command logic

    return CR_OK;
}
