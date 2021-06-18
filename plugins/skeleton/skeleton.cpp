// This is a generic plugin that does nothing useful apart from acting as an example... of a plugin that does nothing :D

// some headers required for a plugin. Nothing special, just the basics.
#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

// If you need to save data per-world:
//#include "modules/Persistence.h"

// DF data structure definition headers
#include "DataDefs.h"
//#include "df/world.h"

// our own, empty header.
#include "skeleton.h"

using namespace DFHack;
using namespace df::enums;

// Expose the plugin name to the DFHack core, as well as metadata like the DFHack version.
// The name string provided must correspond to the filename -
// skeleton.plug.so, skeleton.plug.dylib, or skeleton.plug.dll in this case
DFHACK_PLUGIN("skeleton");

// Any globals a plugin requires (e.g. world) should be listed here.
// For example, this line expands to "using df::global::world" and prevents the
// plugin from being loaded if df::global::world is null (i.e. missing from symbols.xml):
//
// REQUIRE_GLOBAL(world);

// Here go all the command declarations...
// mostly to allow having the mandatory stuff on top of the file and commands on the bottom
command_result skeleton (color_ostream &out, std::vector <std::string> & parameters);

// Mandatory init function. If you have some global state, create it here.
DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    // Fill the command list with your commands.
    commands.push_back(PluginCommand(
        "skeleton", "Do nothing, look pretty.",
        skeleton, false, /* true means that the command can't be used from non-interactive user interface */
        // Extended help string. Used by CR_WRONG_USAGE and the help command:
        "  This command does nothing at all.\n"
        "Example:\n"
        "  skeleton\n"
        "    Does nothing.\n"
    ));
    return CR_OK;
}

// This is called right before the plugin library is removed from memory.
DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    // You *MUST* kill all threads you created before this returns.
    // If everything fails, just return CR_FAILURE. Your plugin will be
    // in a zombie state, but things won't crash.
    return CR_OK;
}

// Used by `plug` to track this plugin's status.
// Called to enable/disable this plugin's state.
// Is called when using `enable` or `disable` in the console
/*
DFHACK_PLUGIN_IS_ENABLED(enabled);
DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    enabled = enable;
    // other code
    return CR_OK;
}
*/

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

// If you need to save or load world-specific data, define these functions.
// plugin_save_data is called when the game might be about to save the world,
// and plugin_load_data is called whenever a new world is loaded. If the plugin
// is loaded or unloaded while a world is active, plugin_save_data or
// plugin_load_data will be called immediately.
/*
DFhackCExport command_result plugin_save_data (color_ostream &out)
{
    // Call functions in the Persistence module here.
    return CR_OK;
}

DFhackCExport command_result plugin_load_data (color_ostream &out)
{
    // Call functions in the Persistence module here.
    return CR_OK;
}
*/

// A command! It sits around and looks pretty. And it's nice and friendly.
command_result skeleton (color_ostream &out, std::vector <std::string> & parameters)
{
    // It's nice to print a help message you get invalid options
    // from the user instead of just acting strange.
    // This can be achieved by adding the extended help string to the
    // PluginCommand registration as show above, and then returning
    // CR_WRONG_USAGE from the function. The same string will also
    // be used by 'help your-command'.
    if (!parameters.empty())
        return CR_WRONG_USAGE;
    // Commands are called from threads other than the DF one.
    // Suspend this thread until DF has time for us. If you
    // use CoreSuspender, it'll automatically resume DF when
    // execution leaves the current scope.
    CoreSuspender suspend;
    // Actually do something here. Yay.
    out.print("Hello! I do nothing, remember?\n");
    // Give control back to DF.
    return CR_OK;
}
