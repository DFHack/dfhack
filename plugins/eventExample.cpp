
#include "Console.h"
#include "Core.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/EventManager.h"
#include "DataDefs.h"

using namespace DFHack;

DFHACK_PLUGIN("eventExample");

void jobInitiated(color_ostream& out, void* job);
void jobCompleted(color_ostream& out, void* job);
void timePassed(color_ostream& out, void* ptr);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector<PluginCommand> &commands) {
    EventManager::EventHandler initiateHandler(jobInitiated);
    EventManager::EventHandler completeHandler(jobCompleted);
    EventManager::EventHandler timeHandler(timePassed);
    Plugin* me = Core::getInstance().getPluginManager()->getPluginByName("eventExample");

    EventManager::registerListener(EventManager::EventType::JOB_INITIATED, initiateHandler, me);
    EventManager::registerListener(EventManager::EventType::JOB_COMPLETED, completeHandler, me);
    EventManager::registerTick(timeHandler, 1, me);
    EventManager::registerTick(timeHandler, 2, me);
    EventManager::registerTick(timeHandler, 4, me);
    EventManager::registerTick(timeHandler, 8, me);
    
    return CR_OK;
}

void jobInitiated(color_ostream& out, void* job) {
    out.print("Job initiated! 0x%X\n", job);
}

void jobCompleted(color_ostream& out, void* job) {
    out.print("Job completed! 0x%X\n", job);
}

void timePassed(color_ostream& out, void* ptr) {
    out.print("Time: %d\n", (int32_t)(ptr));
}
