
#include "Console.h"
#include "Core.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/EventManager.h"
#include "DataDefs.h"

#include "df/construction.h"
#include "df/coord.h"
#include "df/item.h"
#include "df/job.h"
#include "df/world.h"

#include <vector>

using namespace DFHack;
using namespace std;

DFHACK_PLUGIN("eventExample");

void jobInitiated(color_ostream& out, void* job);
void jobCompleted(color_ostream& out, void* job);
void timePassed(color_ostream& out, void* ptr);
void unitDeath(color_ostream& out, void* ptr);
void itemCreate(color_ostream& out, void* ptr);
void building(color_ostream& out, void* ptr);
void construction(color_ostream& out, void* ptr);
void syndrome(color_ostream& out, void* ptr);
void invasion(color_ostream& out, void* ptr);

command_result eventExample(color_ostream& out, vector<string>& parameters);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand("eventExample", "Sets up a few event triggers.",eventExample));
    return CR_OK;
}

command_result eventExample(color_ostream& out, vector<string>& parameters) {
    EventManager::EventHandler initiateHandler(jobInitiated, 1);
    EventManager::EventHandler completeHandler(jobCompleted, 0);
    EventManager::EventHandler timeHandler(timePassed, 1);
    EventManager::EventHandler deathHandler(unitDeath, 500);
    EventManager::EventHandler itemHandler(itemCreate, 1);
    EventManager::EventHandler buildingHandler(building, 500);
    EventManager::EventHandler constructionHandler(construction, 100);
    EventManager::EventHandler syndromeHandler(syndrome, 1);
    EventManager::EventHandler invasionHandler(invasion, 1000);
    EventManager::unregisterAll(plugin_self);

    EventManager::registerListener(EventManager::EventType::JOB_INITIATED, initiateHandler, plugin_self);
    EventManager::registerListener(EventManager::EventType::JOB_COMPLETED, completeHandler, plugin_self);
    EventManager::registerListener(EventManager::EventType::UNIT_DEATH, deathHandler, plugin_self);
    EventManager::registerListener(EventManager::EventType::ITEM_CREATED, itemHandler, plugin_self);
    EventManager::registerListener(EventManager::EventType::BUILDING, buildingHandler, plugin_self);
    EventManager::registerListener(EventManager::EventType::CONSTRUCTION, constructionHandler, plugin_self);
    EventManager::registerListener(EventManager::EventType::SYNDROME, syndromeHandler, plugin_self);
    EventManager::registerListener(EventManager::EventType::INVASION, invasionHandler, plugin_self);
    EventManager::registerTick(timeHandler, 1, plugin_self);
    EventManager::registerTick(timeHandler, 2, plugin_self);
    EventManager::registerTick(timeHandler, 4, plugin_self);
    EventManager::registerTick(timeHandler, 8, plugin_self);
    int32_t t = EventManager::registerTick(timeHandler, 16, plugin_self);
    timeHandler.freq = t;
    EventManager::unregister(EventManager::EventType::TICK, timeHandler, plugin_self);
    t = EventManager::registerTick(timeHandler, 32, plugin_self);
    t = EventManager::registerTick(timeHandler, 32, plugin_self);
    t = EventManager::registerTick(timeHandler, 32, plugin_self);
    timeHandler.freq = t;
    EventManager::unregister(EventManager::EventType::TICK, timeHandler, plugin_self);
    EventManager::unregister(EventManager::EventType::TICK, timeHandler, plugin_self);
    
    out.print("Events registered.\n");
    return CR_OK;
}

//static int timerCount=0;
//static int timerDenom=0;
void jobInitiated(color_ostream& out, void* job_) {
    out.print("Job initiated! 0x%X\n", job_);
/*
    df::job* job = (df::job*)job_;
    out.print("  completion_timer = %d\n", job->completion_timer);
    if ( job->completion_timer != -1 ) timerCount++;
    timerDenom++;
    out.print("  frac = %d / %d\n", timerCount, timerDenom);
*/
}

void jobCompleted(color_ostream& out, void* job) {
    out.print("Job completed! 0x%X\n", job);
}

void timePassed(color_ostream& out, void* ptr) {
    out.print("Time: %d\n", (int32_t)(ptr));
}

void unitDeath(color_ostream& out, void* ptr) {
    out.print("Death: %d\n", (int32_t)(ptr));
}

void itemCreate(color_ostream& out, void* ptr) {
    int32_t item_index = df::item::binsearch_index(df::global::world->items.all, (int32_t)ptr);
    if ( item_index == -1 ) {
        out.print("%s, %d: Error.\n", __FILE__, __LINE__);
    }
    df::item* item = df::global::world->items.all[item_index];
    df::item_type type = item->getType();
    df::coord pos = item->pos;
    out.print("Item created: %d, %s, at (%d,%d,%d)\n", (int32_t)(ptr), ENUM_KEY_STR(item_type, type).c_str(), pos.x, pos.y, pos.z);
}

void building(color_ostream& out, void* ptr) {
    out.print("Building created/destroyed: %d\n", (int32_t)ptr);
}

void construction(color_ostream& out, void* ptr) {
    out.print("Construction created/destroyed: 0x%X\n", ptr);
    df::construction* constr = (df::construction*)ptr;
    df::coord pos = constr->pos;
    out.print("  (%d,%d,%d)\n", pos.x, pos.y, pos.z);
    if ( df::construction::find(pos) == NULL )
        out.print("  construction destroyed\n");
    else
        out.print("  construction created\n");
    
}

void syndrome(color_ostream& out, void* ptr) {
    EventManager::SyndromeData* data = (EventManager::SyndromeData*)ptr;
    out.print("Syndrome started: unit %d, syndrome %d.\n", data->unitId, data->syndromeIndex);
}

void invasion(color_ostream& out, void* ptr) {
    out.print("New invasion! %d\n", (int32_t)ptr);
}

