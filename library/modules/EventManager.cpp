#include "Core.h"
#include "Console.h"
#include "modules/Buildings.h"
#include "modules/Constructions.h"
#include "modules/EventManager.h"
#include "modules/Job.h"
#include "modules/World.h"

#include "df/building.h"
#include "df/construction.h"
#include "df/global_objects.h"
#include "df/item.h"
#include "df/job.h"
#include "df/job_list_link.h"
#include "df/unit.h"
#include "df/world.h"

#include <map>
#include <unordered_map>
#include <unordered_set>

using namespace std;
using namespace DFHack;
using namespace EventManager;

/*
 * TODO:
 *  error checking
 *  consider a typedef instead of a struct for EventHandler
 **/

//map<uint32_t, vector<DFHack::EventManager::EventHandler> > tickQueue;
multimap<uint32_t, EventHandler> tickQueue;

//TODO: consider unordered_map of pairs, or unordered_map of unordered_set, or whatever
multimap<Plugin*, EventHandler> handlers[EventType::EVENT_MAX];
multimap<Plugin*, int32_t> pluginFrequencies[EventType::EVENT_MAX];
map<int32_t, int32_t> eventFrequency[EventType::EVENT_MAX];
uint32_t eventLastTick[EventType::EVENT_MAX];

const uint32_t ticksPerYear = 403200;

void DFHack::EventManager::registerListener(EventType::EventType e, EventHandler handler, int32_t freq, Plugin* plugin) {
    handlers[e].insert(pair<Plugin*, EventHandler>(plugin, handler));
    eventFrequency[e][freq]++;
    pluginFrequencies[e].insert(pair<Plugin*,int32_t>(plugin, freq));
}

void DFHack::EventManager::registerTick(EventHandler handler, int32_t when, Plugin* plugin, bool absolute) {
    uint32_t tick = DFHack::World::ReadCurrentYear()*ticksPerYear
        + DFHack::World::ReadCurrentTick();
    if ( !Core::getInstance().isWorldLoaded() ) {
        tick = 0;
        if ( absolute ) {
            Core::getInstance().getConsole().print("Warning: absolute flag will not be honored.\n");
        }
    }
    if ( absolute ) {
        tick = 0;
    }
    
    tickQueue.insert(pair<uint32_t, EventHandler>(tick+(uint32_t)when, handler));
    handlers[EventType::TICK].insert(pair<Plugin*,EventHandler>(plugin,handler));
    return;
}

void DFHack::EventManager::unregister(EventType::EventType e, EventHandler handler, int32_t freq, Plugin* plugin) {
    for ( multimap<Plugin*, EventHandler>::iterator i = handlers[e].find(plugin); i != handlers[e].end(); i++ ) {
        if ( (*i).first != plugin )
            break;
        EventHandler handle = (*i).second;
        if ( handle == handler ) {
            handlers[e].erase(i);
            break;
        }
    }
    if ( eventFrequency[e].find(freq) == eventFrequency[e].end() ) {
        Core::getInstance().getConsole().print("%s, line %d: Error: incorrect frequency on deregister.\n", __FILE__, __LINE__);
        return;
    }
    eventFrequency[e][freq]--;
    if ( eventFrequency[e][freq] == 0 ) {
        eventFrequency[e].erase(eventFrequency[e].find(freq));
    } else if ( eventFrequency[e][freq] < 0 ) {
        Core::getInstance().getConsole().print("%s, line %d: Error: incorrect frequency on deregister.\n", __FILE__, __LINE__);
    }
    return;
}

void DFHack::EventManager::unregisterAll(Plugin* plugin) {
    for ( auto i = handlers[EventType::TICK].find(plugin); i != handlers[EventType::TICK].end(); i++ ) {
        if ( (*i).first != plugin )
            break;
        
        //shenanigans to avoid concurrent modification
        EventHandler getRidOf = (*i).second;
        bool didSomething;
        do {
            didSomething = false;
            for ( auto j = tickQueue.begin(); j != tickQueue.end(); j++ ) {
                EventHandler candidate = (*j).second;
                if ( getRidOf != candidate )
                    continue;
                tickQueue.erase(j);
                didSomething = true;
                break;
            }
        } while(didSomething);
    }
    for ( size_t a = 0; a < (size_t)EventType::EVENT_MAX; a++ ) {
        handlers[a].erase(plugin);
    }

    for ( size_t a = 0; a < (size_t)EventType::EVENT_MAX; a++ ) {
        for ( auto b = pluginFrequencies[a].begin(); b != pluginFrequencies[a].end(); b++ ) {
            if ( (*b).first != plugin )
                continue;
            int32_t freq = (*b).second;
            eventFrequency[a][freq]--;
            if ( eventFrequency[a][freq] < 0 ) {
                Core::getInstance().getConsole().print("%s, line %d: Error: incorrect frequency on deregister.\n", __FILE__, __LINE__);
                eventFrequency[a].erase(eventFrequency[a].find(freq));
            } else if ( eventFrequency[a][freq] == 0 ) {
                eventFrequency[a].erase(eventFrequency[a].find(freq));
            }
        }
    }
    return;
}

static void manageTickEvent(color_ostream& out);
static void manageJobInitiatedEvent(color_ostream& out);
static void manageJobCompletedEvent(color_ostream& out);
static void manageUnitDeathEvent(color_ostream& out);
static void manageItemCreationEvent(color_ostream& out);
static void manageBuildingEvent(color_ostream& out);
static void manageConstructionEvent(color_ostream& out);

//tick event
static uint32_t lastTick = 0;

//job initiated
static int32_t lastJobId = -1;

//job completed
static unordered_map<int32_t, df::job*> prevJobs;

//unit death
static unordered_set<int32_t> livingUnits;

//item creation
static int32_t nextItem;

//building
static int32_t nextBuilding;
static unordered_set<int32_t> buildings;

//construction
static unordered_set<df::construction*> constructions;

void DFHack::EventManager::onStateChange(color_ostream& out, state_change_event event) {
    static bool doOnce = false;
    if ( !doOnce ) {
        //TODO: put this somewhere else
        doOnce = true;
        EventHandler buildingHandler(Buildings::updateBuildings);
        DFHack::EventManager::registerListener(EventType::BUILDING, buildingHandler, 100, NULL);
        //out.print("Registered listeners.\n %d", __LINE__);
    }
    if ( event == DFHack::SC_MAP_UNLOADED ) {
        lastTick = 0;
        lastJobId = -1;
        for ( auto i = prevJobs.begin(); i != prevJobs.end(); i++ ) {
            Job::deleteJobStruct((*i).second);
        }
        prevJobs.clear();
        tickQueue.clear();
        livingUnits.clear();
        nextItem = -1;
        nextBuilding = -1;
        buildings.clear();
        constructions.clear();

        Buildings::clearBuildings(out);
    } else if ( event == DFHack::SC_MAP_LOADED ) {
        uint32_t tick = DFHack::World::ReadCurrentYear()*ticksPerYear
            + DFHack::World::ReadCurrentTick();
        multimap<uint32_t,EventHandler> newTickQueue;
        for ( auto i = tickQueue.begin(); i != tickQueue.end(); i++ ) {
            newTickQueue.insert(pair<uint32_t,EventHandler>(tick + (*i).first, (*i).second));
        }
        tickQueue.clear();

        tickQueue.insert(newTickQueue.begin(), newTickQueue.end());

        nextItem = 0;
        nextBuilding = 0;
        lastTick = 0;
    }
}

void DFHack::EventManager::manageEvents(color_ostream& out) {
    if ( !Core::getInstance().isWorldLoaded() ) {
        return;
    }
    uint32_t tick = DFHack::World::ReadCurrentYear()*ticksPerYear
        + DFHack::World::ReadCurrentTick();
    
    if ( tick <= lastTick )
        return;
    lastTick = tick;
    
    manageTickEvent(out);
    if ( tick - eventLastTick[EventType::JOB_INITIATED] >= (*eventFrequency[EventType::JOB_INITIATED].begin()).first ) {
        manageJobInitiatedEvent(out);
        eventLastTick[EventType::JOB_INITIATED] = tick;
    }
    if ( tick - eventLastTick[EventType::JOB_COMPLETED] >= (*eventFrequency[EventType::JOB_COMPLETED].begin()).first ) {
        manageJobCompletedEvent(out);
        eventLastTick[EventType::JOB_COMPLETED] = tick;
    }
    if ( tick - eventLastTick[EventType::UNIT_DEATH] >= (*eventFrequency[EventType::UNIT_DEATH].begin()).first ) {
        manageUnitDeathEvent(out);
        eventLastTick[EventType::UNIT_DEATH] = tick;
    }
    if ( tick - eventLastTick[EventType::ITEM_CREATED] >= (*eventFrequency[EventType::ITEM_CREATED].begin()).first ) {
        manageItemCreationEvent(out);
        eventLastTick[EventType::ITEM_CREATED] = tick;
    }
    if ( tick - eventLastTick[EventType::BUILDING] >= (*eventFrequency[EventType::BUILDING].begin()).first ) {
        manageBuildingEvent(out);
        eventLastTick[EventType::BUILDING] = tick;
    }
    if ( tick - eventLastTick[EventType::CONSTRUCTION] >= (*eventFrequency[EventType::CONSTRUCTION].begin()).first ) {
        manageConstructionEvent(out);
        eventLastTick[EventType::CONSTRUCTION] = tick;
    }

    return;
}

static void manageTickEvent(color_ostream& out) {
    uint32_t tick = DFHack::World::ReadCurrentYear()*ticksPerYear
        + DFHack::World::ReadCurrentTick();
    while ( !tickQueue.empty() ) {
        if ( tick < (*tickQueue.begin()).first )
            break;
        EventHandler handle = (*tickQueue.begin()).second;
        tickQueue.erase(tickQueue.begin());
        handle.eventHandler(out, (void*)tick);
    }
    
}

static void manageJobInitiatedEvent(color_ostream& out) {
    if ( handlers[EventType::JOB_INITIATED].empty() )
        return;
    
    if ( lastJobId == -1 ) {
        lastJobId = *df::global::job_next_id - 1;
        return;
    }

    if ( lastJobId+1 == *df::global::job_next_id ) {
        return; //no new jobs
    }
    multimap<Plugin*,EventHandler> copy(handlers[EventType::JOB_INITIATED].begin(), handlers[EventType::JOB_INITIATED].end());
    
    for ( df::job_list_link* link = &df::global::world->job_list; link != NULL; link = link->next ) {
        if ( link->item == NULL )
            continue;
        if ( link->item->id <= lastJobId )
            continue;
        for ( auto i = copy.begin(); i != copy.end(); i++ ) {
            (*i).second.eventHandler(out, (void*)link->item);
        }
    }

    lastJobId = *df::global::job_next_id - 1;
}

static void manageJobCompletedEvent(color_ostream& out) {
    if ( handlers[EventType::JOB_COMPLETED].empty() ) {
        return;
    }
    
    multimap<Plugin*,EventHandler> copy(handlers[EventType::JOB_COMPLETED].begin(), handlers[EventType::JOB_COMPLETED].end());
    map<int32_t, df::job*> nowJobs;
    for ( df::job_list_link* link = &df::global::world->job_list; link != NULL; link = link->next ) {
        if ( link->item == NULL )
            continue;
        nowJobs[link->item->id] = link->item;
    }

    for ( auto i = prevJobs.begin(); i != prevJobs.end(); i++ ) {
        if ( nowJobs.find((*i).first) != nowJobs.end() )
            continue;

        //recently finished or cancelled job!
        for ( auto j = copy.begin(); j != copy.end(); j++ ) {
            (*j).second.eventHandler(out, (void*)(*i).second);
        }
    }

    //erase old jobs, copy over possibly altered jobs
    for ( auto i = prevJobs.begin(); i != prevJobs.end(); i++ ) {
        Job::deleteJobStruct((*i).second);
    }
    prevJobs.clear();
    
    //create new jobs
    for ( auto j = nowJobs.begin(); j != nowJobs.end(); j++ ) {
        /*map<int32_t, df::job*>::iterator i = prevJobs.find((*j).first);
        if ( i != prevJobs.end() ) {
            continue;
        }*/

        df::job* newJob = Job::cloneJobStruct((*j).second, true);
        prevJobs[newJob->id] = newJob;
    }

    /*//get rid of old pointers to deallocated jobs
    for ( size_t a = 0; a < toDelete.size(); a++ ) {
        prevJobs.erase(a);
    }*/
}

static void manageUnitDeathEvent(color_ostream& out) {
    if ( handlers[EventType::UNIT_DEATH].empty() ) {
        return;
    }
    
    multimap<Plugin*,EventHandler> copy(handlers[EventType::UNIT_DEATH].begin(), handlers[EventType::UNIT_DEATH].end());
    for ( size_t a = 0; a < df::global::world->units.active.size(); a++ ) {
        df::unit* unit = df::global::world->units.active[a];
        if ( unit->counters.death_id == -1 ) {
            livingUnits.insert(unit->id);
            continue;
        }
        //dead: if dead since last check, trigger events
        if ( livingUnits.find(unit->id) == livingUnits.end() )
            continue;

        for ( auto i = copy.begin(); i != copy.end(); i++ ) {
            (*i).second.eventHandler(out, (void*)unit->id);
        }
        livingUnits.erase(unit->id);
    }
}

static void manageItemCreationEvent(color_ostream& out) {
    if ( handlers[EventType::ITEM_CREATED].empty() ) {
        return;
    }

    if ( nextItem >= *df::global::item_next_id ) {
        return;
    }

    multimap<Plugin*,EventHandler> copy(handlers[EventType::ITEM_CREATED].begin(), handlers[EventType::ITEM_CREATED].end());
    size_t index = df::item::binsearch_index(df::global::world->items.all, nextItem, false);
    for ( size_t a = index; a < df::global::world->items.all.size(); a++ ) {
        df::item* item = df::global::world->items.all[a];
        //invaders
        if ( item->flags.bits.foreign )
            continue;
        //traders who bring back your items?
        if ( item->flags.bits.trader )
            continue;
        //migrants
        if ( item->flags.bits.owned )
            continue;
        //spider webs don't count
        if ( item->flags.bits.spider_web )
            continue;
        for ( auto i = copy.begin(); i != copy.end(); i++ ) {
            (*i).second.eventHandler(out, (void*)item->id);
        }
    }
    nextItem = *df::global::item_next_id;
}

static void manageBuildingEvent(color_ostream& out) {
    /*
     * TODO: could be faster
     * consider looking at jobs: building creation / destruction
     **/
    if ( handlers[EventType::BUILDING].empty() )
        return;
    
    multimap<Plugin*,EventHandler> copy(handlers[EventType::BUILDING].begin(), handlers[EventType::BUILDING].end());
    //first alert people about new buildings
    for ( int32_t a = nextBuilding; a < *df::global::building_next_id; a++ ) {
        int32_t index = df::building::binsearch_index(df::global::world->buildings.all, a);
        if ( index == -1 ) {
            //out.print("%s, line %d: Couldn't find new building with id %d.\n", __FILE__, __LINE__, a);
            //the tricky thing is that when the game first starts, it's ok to skip buildings, but otherwise, if you skip buildings, something is probably wrong. TODO: make this smarter
            continue;
        }
        buildings.insert(a);
        for ( auto b = copy.begin(); b != copy.end(); b++ ) {
            EventHandler bob = (*b).second;
            bob.eventHandler(out, (void*)a);
        }
    }
    nextBuilding = *df::global::building_next_id;
    
    //now alert people about destroyed buildings
    unordered_set<int32_t> toDelete;
    for ( auto a = buildings.begin(); a != buildings.end(); a++ ) {
        int32_t id = *a;
        int32_t index = df::building::binsearch_index(df::global::world->buildings.all,id);
        if ( index != -1 )
            continue;
        toDelete.insert(id);

        for ( auto b = copy.begin(); b != copy.end(); b++ ) {
            EventHandler bob = (*b).second;
            bob.eventHandler(out, (void*)id);
        }
    }

    for ( auto a = toDelete.begin(); a != toDelete.end(); a++ ) {
        int32_t id = *a;
        buildings.erase(id);
    }
    
    //out.print("Sent building event.\n %d", __LINE__);
}

static void manageConstructionEvent(color_ostream& out) {
    if ( handlers[EventType::CONSTRUCTION].empty() )
        return;

    unordered_set<df::construction*> constructionsNow(df::global::world->constructions.begin(), df::global::world->constructions.end());
    
    multimap<Plugin*,EventHandler> copy(handlers[EventType::CONSTRUCTION].begin(), handlers[EventType::CONSTRUCTION].end());
    for ( auto a = constructions.begin(); a != constructions.end(); a++ ) {
        df::construction* construction = *a;
        if ( constructionsNow.find(construction) != constructionsNow.end() )
            continue;
        for ( auto b = copy.begin(); b != copy.end(); b++ ) {
            EventHandler handle = (*b).second;
            handle.eventHandler(out, (void*)construction);
        }
    }

    for ( auto a = constructionsNow.begin(); a != constructionsNow.end(); a++ ) {
        df::construction* construction = *a;
        if ( constructions.find(construction) != constructions.end() )
            continue;
        for ( auto b = copy.begin(); b != copy.end(); b++ ) {
            EventHandler handle = (*b).second;
            handle.eventHandler(out, (void*)construction);
        }
    }
    
    constructions.clear();
    constructions.insert(constructionsNow.begin(), constructionsNow.end());
}
