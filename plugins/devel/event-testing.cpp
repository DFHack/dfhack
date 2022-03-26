#include "Core.h"
#include <DataDefs.h>
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <modules/EventManager.h>
#include <modules/Job.h>
#include <df/job.h>

//#include "df/world.h"

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("event-testing");
DFHACK_PLUGIN_IS_ENABLED(enabled);
//REQUIRE_GLOBAL(world);

void onTick(color_ostream& out, void* tick);
void onJobStart(color_ostream &out, void* job);
void onJobCompletion(color_ostream &out, void* job);

command_result skeleton2 (color_ostream &out, std::vector <std::string> & parameters);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands) {
    commands.push_back(PluginCommand("event-testing",
                                     "~54 character description of plugin", //to use one line in the ``[DFHack]# ls`` output
                                     skeleton2,
                                     false,
                                     "example usage"));
    return CR_OK;
}

command_result skeleton2 (color_ostream &out, std::vector <std::string> & parameters) {
    if (!parameters.empty())
        return CR_WRONG_USAGE;
    CoreSuspender suspend;
    out.print("blah");
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    namespace EM = EventManager;
    EM::unregisterAll(plugin_self);
    return CR_OK;
}

void enable_job_completed_events() {
    namespace EM = EventManager;
    using namespace EM::EventType;
    EM::EventHandler eventHandler1(onJobCompletion, 0); // constantly
    EM::EventHandler eventHandler2(onJobCompletion, 500); // every dwarf-hour
    EM::registerListener(EventType::JOB_COMPLETED, eventHandler1, plugin_self);
    EM::registerListener(EventType::JOB_COMPLETED, eventHandler2, plugin_self);
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    namespace EM = EventManager;
    if (enable && !enabled) {
        enable_job_completed_events();
        out.print("plugin enabled!\n");
    } else if (!enable && enabled) {
        EM::unregisterAll(plugin_self);
        out.print("plugin disabled!\n");
    }
    enabled = enable;
    return CR_OK;
}

void onJobCompletion(color_ostream &out, void* job) {
    auto j = (df::job*)job;
    std::string type = ENUM_KEY_STR(job_type, j->job_type);
    out.print("onJobCompletion: (id: %d) (type: %s) (expire: %d) (completion: %d) (wait: %d)\n", j->id, type.c_str(), j->expire_timer, j->completion_timer, j->wait_timer);
}

