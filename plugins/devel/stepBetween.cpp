
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

// DF data structure definition headers
#include "DataDefs.h"
#include "df/world.h"

#include "modules/Gui.h"
#include "modules/Maps.h"

using namespace DFHack;
using namespace df::enums;



command_result stepBetween (color_ostream &out, std::vector <std::string> & parameters);

// A plugin must be able to return its name and version.
// The name string provided must correspond to the filename - skeleton.plug.so or skeleton.plug.dll in this case
DFHACK_PLUGIN("stepBetween");

// Mandatory init function. If you have some global state, create it here.
DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    // Fill the command list with your commands.
    commands.push_back(PluginCommand(
        "stepBetween", "Do nothing, look pretty.",
        stepBetween, false, /* true means that the command can't be used from non-interactive user interface */
        // Extended help string. Used by CR_WRONG_USAGE and the help command:
        "  This command does nothing at all.\n"
    ));
    return CR_OK;
}

// This is called right before the plugin library is removed from memory.
DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    // You *MUST* kill all threads you created before this returns.
    // If everything fails, just return CR_FAILURE. Your plugin will be
    // in a zombie state, but things won't crash.
    return CR_OK;
}

// Called to notify the plugin about important state changes.
// Invoked with DF suspended, and always before the matching plugin_onupdate.
// More event codes may be added in the future.
/*
DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_GAME_LOADED:
        // initialize from the world just loaded
        break;
    case SC_GAME_UNLOADED:
        // cleanup
        break;
    default:
        break;
    }
    return CR_OK;
}
*/

// Whatever you put here will be done in each game step. Don't abuse it.
// It's optional, so you can just comment it out like this if you don't need it.
/*
DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    // whetever. You don't need to suspend DF execution here.
    return CR_OK;
}
*/

df::coord prev;

// A command! It sits around and looks pretty. And it's nice and friendly.
command_result stepBetween (color_ostream &out, std::vector <std::string> & parameters)
{
    df::coord bob = Gui::getCursorPos();
    out.print("(%d,%d,%d), (%d,%d,%d): canStepBetween = %d, canWalkBetween = %d\n", prev.x, prev.y, prev.z, bob.x, bob.y, bob.z, Maps::canStepBetween(prev, bob), Maps::canWalkBetween(prev,bob));

    prev = bob;
    return CR_OK;
}
