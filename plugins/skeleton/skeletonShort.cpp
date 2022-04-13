#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <modules/EventManager.h>

//#include "df/world.h"

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("skeleton2");
DFHACK_PLUGIN_IS_ENABLED(enabled);
//REQUIRE_GLOBAL(world);

command_result skeleton2 (color_ostream &out, std::vector <std::string> & parameters);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands) {
    commands.push_back(PluginCommand("skeleton2",
                                     "~54 character description of plugin", //to use one line in the ``[DFHack]# ls`` output
                                     skeleton2,
                                     false,
                                     "example usage"
                                     " skeleton2 <option> <args>\n"
                                     "    explanation of plugin/command\n"
                                     "\n"
                                     " skeleton2\n"
                                     "    what happens when using the command\n"
                                     "\n"
                                     " skeleton2 option1\n"
                                     "    what happens when using the command with option1\n"
                                     "\n"));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
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

command_result skeleton2 (color_ostream &out, std::vector <std::string> & parameters) {
    if (!parameters.empty())
        return CR_WRONG_USAGE;
    CoreSuspender suspend;
    out.print("blah");
    return CR_OK;
}
