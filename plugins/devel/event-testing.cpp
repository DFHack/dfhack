#include "Core.h"
#include <DataDefs.h>
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <modules/EventManager.h>
#include <modules/Job.h>
#include <df/job.h>
#include <df/construction.h>

//#include "df/world.h"

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("event-testing");
DFHACK_PLUGIN_IS_ENABLED(enabled);
//REQUIRE_GLOBAL(world);

void onTick(color_ostream& out, void* tick);
void onJob(color_ostream &out, void* job);
void onConstruction(color_ostream &out, void* construction);
void onSyndrome(color_ostream &out, void* syndrome);
void onDeath(color_ostream &out, void* unit_id);
void onNewActive(color_ostream &out, void* unit_id);

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
const int32_t interval = 500; // 5 dwarf-hours

void enable_job_events() {
    namespace EM = EventManager;
    using namespace EM::EventType;
    EM::EventHandler e1(onJob, 0); // constantly
    EM::EventHandler e2(onJob, interval); // every interval
    EM::registerListener(EventType::JOB_INITIATED, e1, plugin_self);
    EM::registerListener(EventType::JOB_INITIATED, e2, plugin_self);
    EM::EventHandler e3(onJob, 0); // constantly
    EM::EventHandler e4(onJob, interval); // every interval
    EM::registerListener(EventType::JOB_STARTED, e3, plugin_self);
    EM::registerListener(EventType::JOB_STARTED, e4, plugin_self);
    EM::EventHandler e5(onJob, 0); // constantly
    EM::EventHandler e6(onJob, interval); // every interval
    EM::registerListener(EventType::JOB_COMPLETED, e5, plugin_self);
    EM::registerListener(EventType::JOB_COMPLETED, e6, plugin_self);
}
void enable_construction_events() {
    namespace EM = EventManager;
    using namespace EM::EventType;
    EM::EventHandler e1(onConstruction, 0); // constantly
    EM::EventHandler e2(onConstruction, interval); // every interval
    EM::registerListener(EventType::CONSTRUCTION_REMOVED, e1, plugin_self);
    EM::registerListener(EventType::CONSTRUCTION_REMOVED, e2, plugin_self);
}

void enable_unit_events() {
    namespace EM = EventManager;
    using namespace EM::EventType;
    EM::EventHandler e1(onSyndrome, 0); // constantly
    EM::EventHandler e2(onDeath, 0); // constantly
    EM::EventHandler e3(onNewActive, 0); // constantly
    EM::registerListener(EventType::SYNDROME, e1, plugin_self);
    EM::registerListener(EventType::UNIT_DEATH, e2, plugin_self);
    EM::registerListener(EventType::UNIT_NEW_ACTIVE, e3, plugin_self);
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    namespace EM = EventManager;
    if (enable && !enabled) {
        enable_job_events();
        enable_construction_events();
        enable_unit_events();
        out.print("plugin enabled!\n");
    } else if (!enable && enabled) {
        EM::unregisterAll(plugin_self);
        out.print("plugin disabled!\n");
    }
    enabled = enable;
    return CR_OK;
}

void onJob(color_ostream &out, void* job) {
    auto j = (df::job*)job;
    std::string type = ENUM_KEY_STR(job_type, j->job_type);
    out.print("onJob: (id: %d) (type: %s) (expire: %d) (completion: %d) (wait: %d)\n", j->id, type.c_str(), j->expire_timer, j->completion_timer, j->wait_timer);
}

void onConstruction(color_ostream &out, void* construction) {
    auto c = (df::construction*)construction;
    std::string type = ENUM_KEY_STR(item_type, c->item_type);
    out.print("onConstruction: (type: %s)\n", type.c_str());
}
