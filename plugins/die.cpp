// Since you can't do "Ctrl-Z kill -9 %1" from the console, instead just
// give the "die" command to terminate the game without saving.
// Linux only, since _exit() probably doesn't work on Windows.
//
// Need to set cmake option BUILD_KILL_GAME to ON to compile this
// plugin.

#ifndef LINUX_BUILD
    #error "This plugin only compiles on Linux"
#endif

#include <dfhack/Core.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <vector>
#include <string>

using std::vector;
using std::string;
using namespace DFHack;

DFhackCExport command_result df_die (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "die";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("die",
                       "Kill game without saving", df_die));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

DFhackCExport command_result df_die (Core * c, vector <string> & parameters)
{
    _exit(0);

    return CR_OK;
}
