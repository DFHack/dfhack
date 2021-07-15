// This is a generic plugin that does nothing useful apart from acting as an example... of a plugin that does nothing :D

// some headers required for a plugin. Nothing special, just the basics.
#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <modules/EventManager.h>
// If you need to save data per-world:
//#include "modules/Persistence.h"

// DF data structure definition headers
#include "DataDefs.h"
//#include "df/world.h"

// our own, empty header.
#include "skeleton.h"

using namespace DFHack;
using namespace df::enums;

// A plugin must be able to return its name and version.
// The name string provided must correspond to the filename -
// skeleton.plug.so, skeleton.plug.dylib, or skeleton.plug.dll in this case
DFHACK_PLUGIN("skeleton");

// A plugin can have a state which can be used to manage how the plugin operates.
// The state is referred to by this the identifier provided to the macro. eg. `enabled`
DFHACK_PLUGIN_IS_ENABLED(enabled);

// Any globals a plugin requires (e.g. world) should be listed here.
// For example, this line expands to "using df::global::world" and prevents the
// plugin from being loaded if df::global::world is null (i.e. missing from symbols.xml):
//
REQUIRE_GLOBAL(world);

// You may want some compile time debugging options
// one easy system just requires you to cache the color_ostream &out into a global debug variable
//#define P_DEBUG 1
//uint16_t maxTickFreq = 1200; //maybe you want to use some events

command_result command_callback1(color_ostream &out, std::vector<std::string> &parameters);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand("skeleton",
                                     "~54 character description of plugin", //to use one line in the ``[DFHack]# ls`` output
                                     command_callback1,
                                     false,
                                     ""
                                     " skeleton <option> <args>\n"
                                     "    explanation of plugin/command\n"
                                     "\n"
                                     " skeleton\n"
                                     "    what happens when using the command\n"
                                     "\n"
                                     " skeleton option1\n"
                                     "    what happens when using the command with option1\n"
                                     "\n"));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    // You *MUST* kill all threads you created before this returns.
    // If everything fails, just return CR_FAILURE. Your plugin will be
    // in a zombie state, but things won't crash.
    return CR_OK;

}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    namespace EM = EventManager;
    if (enable && !enabled) {
        //using namespace EM::EventType;
        //EM::EventHandler eventHandler(onNewEvent, maxTickFreq);
        //EM::registerListener(EventType::JOB_COMPLETED, eventHandler, plugin_self);
        //out.print("plugin enabled!\n");
    } else if (!enable && enabled) {
        EM::unregisterAll(plugin_self);
        //out.print("plugin disabled!\n");
    }
    enabled = enable;
    return CR_OK;
}


/* OPTIONAL *
// Called to notify the plugin about important state changes.
// Invoked with DF suspended, and always before the matching plugin_onupdate.
// More event codes may be added in the future.
DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (enabled) {
        switch (event) {
            case SC_UNKNOWN:
                break;
            case SC_WORLD_LOADED:
                break;
            case SC_WORLD_UNLOADED:
                break;
            case SC_MAP_LOADED:
                break;
            case SC_MAP_UNLOADED:
                break;
            case SC_VIEWSCREEN_CHANGED:
                break;
            case SC_CORE_INITIALIZED:
                break;
            case SC_BEGIN_UNLOAD:
                break;
            case SC_PAUSED:
                break;
            case SC_UNPAUSED:
                break;
        }
    }
    return CR_OK;
}

// Whatever you put here will be done in each game step. Don't abuse it.
DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    // whetever. You don't need to suspend DF execution here.
    return CR_OK;
}

// If you need to save or load world-specific data, define these functions.
// plugin_save_data is called when the game might be about to save the world,
// and plugin_load_data is called whenever a new world is loaded. If the plugin
// is loaded or unloaded while a world is active, plugin_save_data or
// plugin_load_data will be called immediately.
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
* OPTIONAL */


// A command! It sits around and looks pretty. And it's nice and friendly.
command_result command_callback1(color_ostream &out, std::vector<std::string> &parameters) {
    // It's nice to print a help message you get invalid options
    // from the user instead of just acting strange.
    // This can be achieved by adding the extended help string to the
    // PluginCommand registration as show above, and then returning
    // CR_WRONG_USAGE from the function. The same string will also
    // be used by 'help your-command'.
    if (!parameters.empty()) {
        return CR_WRONG_USAGE; //or maybe you want it to do something else
    }
    // Commands are called from threads other than the DF one.
    // Suspend this thread until DF has time for us.
    // **If you use CoreSuspender** it'll automatically resume DF when
    // execution leaves the current scope.
    CoreSuspender suspend;
    // Actually do something here. Yay.

    // process parameters
    if (parameters.size() == 1 && parameters[0] == "option1") {
        // stuff
    } else {
        return CR_FAILURE;
    }
    // Give control back to DF.
    return CR_OK;
}