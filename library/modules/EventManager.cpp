#include "Core.h"
#include "Console.h"
#include "VTableInterpose.h"
#include "modules/Buildings.h"
#include "modules/Constructions.h"
#include "modules/EventManager.h"
#include "modules/Once.h"
#include "modules/Job.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/announcement_type.h"
#include "df/building.h"
#include "df/construction.h"
#include "df/general_ref.h"
#include "df/general_ref_type.h"
#include "df/general_ref_unit_workerst.h"
#include "df/global_objects.h"
#include "df/item.h"
#include "df/item_actual.h"
#include "df/item_constructed.h"
#include "df/item_crafted.h"
#include "df/item_weaponst.h"
#include "df/job.h"
#include "df/job_list_link.h"
#include "df/report.h"
#include "df/ui.h"
#include "df/unit.h"
#include "df/unit_flags1.h"
#include "df/unit_inventory_item.h"
#include "df/unit_report_type.h"
#include "df/unit_syndrome.h"
#include "df/unit_wound.h"
#include "df/world.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace std;
using namespace DFHack;
using namespace EventManager;
using namespace df::enums;

/*
 * TODO:
 *  error checking
 *  consider a typedef instead of a struct for EventHandler
 **/

static multimap<int32_t, EventHandler> tickQueue;

//TODO: consider unordered_map of pairs, or unordered_map of unordered_set, or whatever
static multimap<Plugin*, EventHandler> handlers[EventType::EVENT_MAX];
static int32_t eventLastTick[EventType::EVENT_MAX];

static const int32_t ticksPerYear = 403200;

void DFHack::EventManager::registerListener(EventType::EventType e, EventHandler handler, Plugin* plugin) {
    handlers[e].insert(pair<Plugin*, EventHandler>(plugin, handler));
}

int32_t DFHack::EventManager::registerTick(EventHandler handler, int32_t when, Plugin* plugin, bool absolute) {
    if ( !absolute ) {
        df::world* world = df::global::world;
        if ( world ) {
            when += world->frame_counter;
        } else {
            if ( Once::doOnce("EventManager registerTick unhonored absolute=false") )
                Core::getInstance().getConsole().print("EventManager::registerTick: warning! absolute flag=false not honored.\n");
        }
    }
    handler.freq = when;
    tickQueue.insert(pair<int32_t, EventHandler>(handler.freq, handler));
    handlers[EventType::TICK].insert(pair<Plugin*,EventHandler>(plugin,handler));
    return when;
}

static void removeFromTickQueue(EventHandler getRidOf) {
    for ( auto j = tickQueue.find(getRidOf.freq); j != tickQueue.end(); ) {
        if ( (*j).first > getRidOf.freq )
            break;
        if ( (*j).second != getRidOf ) {
            j++;
            continue;
        }
        j = tickQueue.erase(j);
    }
}

void DFHack::EventManager::unregister(EventType::EventType e, EventHandler handler, Plugin* plugin) {
    for ( auto i = handlers[e].find(plugin); i != handlers[e].end(); ) {
        if ( (*i).first != plugin )
            break;
        EventHandler handle = (*i).second;
        if ( handle != handler ) {
            i++;
            continue;
        }
        i = handlers[e].erase(i);
        if ( e == EventType::TICK )
            removeFromTickQueue(handler);
    }
}

void DFHack::EventManager::unregisterAll(Plugin* plugin) {
    for ( auto i = handlers[EventType::TICK].find(plugin); i != handlers[EventType::TICK].end(); i++ ) {
        if ( (*i).first != plugin )
            break;
        
        removeFromTickQueue((*i).second);
    }
    for ( size_t a = 0; a < (size_t)EventType::EVENT_MAX; a++ ) {
        handlers[a].erase(plugin);
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
static void manageSyndromeEvent(color_ostream& out);
static void manageInvasionEvent(color_ostream& out);
static void manageEquipmentEvent(color_ostream& out);
static void manageReportEvent(color_ostream& out);
static void manageUnitAttackEvent(color_ostream& out);
static void manageUnloadEvent(color_ostream& out){};
static void manageInteractionEvent(color_ostream& out);

typedef void (*eventManager_t)(color_ostream&);

static const eventManager_t eventManager[] = {
    manageTickEvent,
    manageJobInitiatedEvent,
    manageJobCompletedEvent,
    manageUnitDeathEvent,
    manageItemCreationEvent,
    manageBuildingEvent,
    manageConstructionEvent,
    manageSyndromeEvent,
    manageInvasionEvent,
    manageEquipmentEvent,
    manageReportEvent,
    manageUnitAttackEvent,
    manageUnloadEvent,
    manageInteractionEvent,
};

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
static unordered_map<df::coord, df::construction> constructions;
static bool gameLoaded;

//syndrome
static int32_t lastSyndromeTime;

//invasion
static int32_t nextInvasion;

//equipment change
//static unordered_map<int32_t, vector<df::unit_inventory_item> > equipmentLog;
static unordered_map<int32_t, vector<InventoryItem> > equipmentLog;

//report
static int32_t lastReport;

//unit attack
static int32_t lastReportUnitAttack;
static std::map<int32_t,std::vector<int32_t> > reportToRelevantUnits;
static int32_t reportToRelevantUnitsTime = -1;

//interaction
static int32_t lastReportInteraction;

void DFHack::EventManager::onStateChange(color_ostream& out, state_change_event event) {
    static bool doOnce = false;
//    const string eventNames[] = {"world loaded", "world unloaded", "map loaded", "map unloaded", "viewscreen changed", "core initialized", "begin unload", "paused", "unpaused"};
//    out.print("%s,%d: onStateChange %d: \"%s\"\n", __FILE__, __LINE__, (int32_t)event, eventNames[event].c_str());
    if ( !doOnce ) {
        //TODO: put this somewhere else
        doOnce = true;
        EventHandler buildingHandler(Buildings::updateBuildings, 100);
        DFHack::EventManager::registerListener(EventType::BUILDING, buildingHandler, NULL);
        //out.print("Registered listeners.\n %d", __LINE__);
    }
    if ( event == DFHack::SC_MAP_UNLOADED ) {
        lastJobId = -1;
        for ( auto i = prevJobs.begin(); i != prevJobs.end(); i++ ) {
            Job::deleteJobStruct((*i).second, true);
        }
        prevJobs.clear();
        tickQueue.clear();
        livingUnits.clear();
        buildings.clear();
        constructions.clear();
        equipmentLog.clear();

        Buildings::clearBuildings(out);
        lastReport = -1;
        lastReportUnitAttack = -1;
        gameLoaded = false;

        multimap<Plugin*,EventHandler> copy(handlers[EventType::UNLOAD].begin(), handlers[EventType::UNLOAD].end());
        for (auto a = copy.begin(); a != copy.end(); a++ ) {
            (*a).second.eventHandler(out, NULL);
        }
    } else if ( event == DFHack::SC_MAP_LOADED ) {
        /*
        int32_t tick = df::global::world->frame_counter;
        multimap<int32_t,EventHandler> newTickQueue;
        for ( auto i = tickQueue.begin(); i != tickQueue.end(); i++ )
            newTickQueue.insert(pair<int32_t,EventHandler>(tick+(*i).first, (*i).second));
        tickQueue.clear();
        tickQueue.insert(newTickQueue.begin(), newTickQueue.end());
        //out.print("%s,%d: on load, frame_counter = %d\n", __FILE__, __LINE__, tick);
        */
        //tickQueue.clear();
        if (!df::global::item_next_id)
            return;
        if (!df::global::building_next_id)
            return;
        if (!df::global::job_next_id)
            return;
        if (!df::global::ui)
            return;
        if (!df::global::world)
            return;
        
        nextItem = *df::global::item_next_id;
        nextBuilding = *df::global::building_next_id;
        nextInvasion = df::global::ui->invasions.next_id;
        lastJobId = -1 + *df::global::job_next_id;
        
        constructions.clear();
        for ( auto i = df::global::world->constructions.begin(); i != df::global::world->constructions.end(); i++ ) {
            df::construction* constr = *i;
            if ( !constr ) {
                if ( Once::doOnce("EventManager.onLoad null constr") ) {
                    out.print("EventManager.onLoad: null construction.\n");
                }
                continue;
            }
            if ( constr->pos == df::coord() ) {
                if ( Once::doOnce("EventManager.onLoad null position of construction.\n") )
                    out.print("EventManager.onLoad null position of construction.\n");
                continue;
            }
            constructions[constr->pos] = *constr;
        }
        for ( size_t a = 0; a < df::global::world->buildings.all.size(); a++ ) {
            df::building* b = df::global::world->buildings.all[a];
            Buildings::updateBuildings(out, (void*)b);
            buildings.insert(b->id);
        }
        lastSyndromeTime = -1;
        for ( size_t a = 0; a < df::global::world->units.all.size(); a++ ) {
            df::unit* unit = df::global::world->units.all[a];
            for ( size_t b = 0; b < unit->syndromes.active.size(); b++ ) {
                df::unit_syndrome* syndrome = unit->syndromes.active[b];
                int32_t startTime = syndrome->year*ticksPerYear + syndrome->year_time;
                if ( startTime > lastSyndromeTime )
                    lastSyndromeTime = startTime;
            }
        }
        lastReport = -1;
        lastReportUnitAttack = -1;
        lastReportInteraction = -1;
        reportToRelevantUnitsTime = -1;
        reportToRelevantUnits.clear();
        for ( size_t a = 0; a < EventType::EVENT_MAX; a++ ) {
            eventLastTick[a] = -1;//-1000000;
        }
        
        gameLoaded = true;
    }
}

void DFHack::EventManager::manageEvents(color_ostream& out) {
    if ( !gameLoaded ) {
        return;
    }
    if (!df::global::world)
        return;

    CoreSuspender suspender;
    
    int32_t tick = df::global::world->frame_counter;

    for ( size_t a = 0; a < EventType::EVENT_MAX; a++ ) {
        if ( handlers[a].empty() )
            continue;
        int32_t eventFrequency = -100;
        if ( a != EventType::TICK )
            for ( auto b = handlers[a].begin(); b != handlers[a].end(); b++ ) {
                EventHandler bob = (*b).second;
                if ( bob.freq < eventFrequency || eventFrequency == -100 )
                    eventFrequency = bob.freq;
            }
        else eventFrequency = 1;
        
        if ( tick >= eventLastTick[a] && tick - eventLastTick[a] < eventFrequency )
            continue;
        
        eventManager[a](out);
        eventLastTick[a] = tick;
    }
}

static void manageTickEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    unordered_set<EventHandler> toRemove;
    int32_t tick = df::global::world->frame_counter;
    while ( !tickQueue.empty() ) {
        if ( tick < (*tickQueue.begin()).first )
            break;
        EventHandler handle = (*tickQueue.begin()).second;
        tickQueue.erase(tickQueue.begin());
        handle.eventHandler(out, (void*)tick);
        toRemove.insert(handle);
    }
    if ( toRemove.empty() )
        return;
    for ( auto a = handlers[EventType::TICK].begin(); a != handlers[EventType::TICK].end(); ) {
        EventHandler handle = (*a).second;
        if ( toRemove.find(handle) == toRemove.end() ) {
            a++;
            continue;
        }
        a = handlers[EventType::TICK].erase(a);
        toRemove.erase(handle);
        if ( toRemove.empty() )
            break;
    }
}

static void manageJobInitiatedEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    if (!df::global::job_next_id)
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

//helper function for manageJobCompletedEvent
static int32_t getWorkerID(df::job* job) {
    auto ref = findRef(job->general_refs, general_ref_type::UNIT_WORKER);
    return ref ? ref->getID() : -1;
}

/*
TODO: consider checking item creation / experience gain just in case
*/
static void manageJobCompletedEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    int32_t tick0 = eventLastTick[EventType::JOB_COMPLETED];
    int32_t tick1 = df::global::world->frame_counter;
    
    multimap<Plugin*,EventHandler> copy(handlers[EventType::JOB_COMPLETED].begin(), handlers[EventType::JOB_COMPLETED].end());
    map<int32_t, df::job*> nowJobs;
    for ( df::job_list_link* link = &df::global::world->job_list; link != NULL; link = link->next ) {
        if ( link->item == NULL )
            continue;
        nowJobs[link->item->id] = link->item;
    }
    
#if 0
    //testing info on job initiation/completion
    //newly allocated jobs
    for ( auto j = nowJobs.begin(); j != nowJobs.end(); j++ ) {
        if ( prevJobs.find((*j).first) != prevJobs.end() )
            continue;
        
        df::job& job1 = *(*j).second;
        out.print("new job\n"
            "  location         : 0x%X\n"
            "  id               : %d\n"
            "  type             : %d %s\n"
            "  working          : %d\n"
            "  completion_timer : %d\n"
            "  workerID         : %d\n"
            "  time             : %d -> %d\n"
            "\n", job1.list_link->item, job1.id, job1.job_type, ENUM_ATTR(job_type, caption, job1.job_type), job1.flags.bits.working, job1.completion_timer, getWorkerID(&job1), tick0, tick1);
    }
    for ( auto i = prevJobs.begin(); i != prevJobs.end(); i++ ) {
        df::job& job0 = *(*i).second;
        auto j = nowJobs.find((*i).first);
        if ( j == nowJobs.end() ) {
            out.print("job deallocated\n"
                "  location         : 0x%X\n"
                "  id               : %d\n"
                "  type             : %d %s\n"
                "  working          : %d\n"
                "  completion_timer : %d\n"
                "  workerID         : %d\n"
                "  time             : %d -> %d\n"
                ,job0.list_link == NULL ? 0 : job0.list_link->item, job0.id, job0.job_type, ENUM_ATTR(job_type, caption, job0.job_type), job0.flags.bits.working, job0.completion_timer, getWorkerID(&job0), tick0, tick1);
            continue;
        }
        df::job& job1 = *(*j).second;
        
        if ( job0.flags.bits.working == job1.flags.bits.working &&
               (job0.completion_timer == job1.completion_timer || (job1.completion_timer > 0 && job0.completion_timer-1 == job1.completion_timer)) &&
               getWorkerID(&job0) == getWorkerID(&job1) )
            continue;
        
        out.print("job change\n"
            "  location         : 0x%X -> 0x%X\n"
            "  id               : %d -> %d\n"
            "  type             : %d -> %d\n"
            "  type             : %s -> %s\n"
            "  working          : %d -> %d\n"
            "  completion timer : %d -> %d\n"
            "  workerID         : %d -> %d\n"
            "  time             : %d -> %d\n"
            "\n",
            job0.list_link->item, job1.list_link->item,
            job0.id, job1.id,
            job0.job_type, job1.job_type,
            ENUM_ATTR(job_type, caption, job0.job_type), ENUM_ATTR(job_type, caption, job1.job_type),
            job0.flags.bits.working, job1.flags.bits.working,
            job0.completion_timer, job1.completion_timer,
            getWorkerID(&job0), getWorkerID(&job1),
            tick0, tick1
        );
    }
#endif
    
    for ( auto i = prevJobs.begin(); i != prevJobs.end(); i++ ) {
        //if it happened within a tick, must have been cancelled by the user or a plugin: not completed
        if ( tick1 <= tick0 )
            continue;
        
        if ( nowJobs.find((*i).first) != nowJobs.end() ) {
            //could have just finished if it's a repeat job
            df::job& job0 = *(*i).second;
            if ( !job0.flags.bits.repeat )
                continue;
            df::job& job1 = *nowJobs[(*i).first];
            if ( job0.completion_timer != 0 )
                continue;
            if ( job1.completion_timer != -1 )
                continue;
            
            //still false positive if cancelled at EXACTLY the right time, but experiments show this doesn't happen
            for ( auto j = copy.begin(); j != copy.end(); j++ ) {
                (*j).second.eventHandler(out, (void*)&job0);
            }
            continue;
        }
        
        //recently finished or cancelled job
        df::job& job0 = *(*i).second;
        if ( job0.flags.bits.repeat || job0.completion_timer != 0 )
            continue;
        
        for ( auto j = copy.begin(); j != copy.end(); j++ ) {
            (*j).second.eventHandler(out, (void*)&job0);
        }
    }

    //erase old jobs, copy over possibly altered jobs
    for ( auto i = prevJobs.begin(); i != prevJobs.end(); i++ ) {
        Job::deleteJobStruct((*i).second, true);
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
}

static void manageUnitDeathEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    multimap<Plugin*,EventHandler> copy(handlers[EventType::UNIT_DEATH].begin(), handlers[EventType::UNIT_DEATH].end());
    for ( size_t a = 0; a < df::global::world->units.all.size(); a++ ) {
        df::unit* unit = df::global::world->units.all[a];
        //if ( unit->counters.death_id == -1 ) {
        if ( ! unit->flags1.bits.dead ) {
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
    if (!df::global::world)
        return;
    if (!df::global::item_next_id)
        return;
    if ( nextItem >= *df::global::item_next_id ) {
        return;
    }
    
    multimap<Plugin*,EventHandler> copy(handlers[EventType::ITEM_CREATED].begin(), handlers[EventType::ITEM_CREATED].end());
    size_t index = df::item::binsearch_index(df::global::world->items.all, nextItem, false);
    if ( index != 0 ) index--;
    for ( size_t a = index; a < df::global::world->items.all.size(); a++ ) {
        df::item* item = df::global::world->items.all[a];
        //already processed
        if ( item->id < nextItem )
            continue;
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
    if (!df::global::world)
        return;
    if (!df::global::building_next_id)
        return;
    /*
     * TODO: could be faster
     * consider looking at jobs: building creation / destruction
     **/
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
    for ( auto a = buildings.begin(); a != buildings.end(); ) {
        int32_t id = *a;
        int32_t index = df::building::binsearch_index(df::global::world->buildings.all,id);
        if ( index != -1 ) {
            a++;
            continue;
        }
        
        for ( auto b = copy.begin(); b != copy.end(); b++ ) {
            EventHandler bob = (*b).second;
            bob.eventHandler(out, (void*)id);
        }
        a = buildings.erase(a);
    }
}

static void manageConstructionEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    //unordered_set<df::construction*> constructionsNow(df::global::world->constructions.begin(), df::global::world->constructions.end());
    
    multimap<Plugin*,EventHandler> copy(handlers[EventType::CONSTRUCTION].begin(), handlers[EventType::CONSTRUCTION].end());
    for ( auto a = constructions.begin(); a != constructions.end(); ) {
        df::construction& construction = (*a).second;
        if ( df::construction::find(construction.pos) != NULL ) {
            a++;
            continue;
        }
        //construction removed
        //out.print("Removed construction (%d,%d,%d)\n", construction.pos.x,construction.pos.y,construction.pos.z);
        for ( auto b = copy.begin(); b != copy.end(); b++ ) {
            EventHandler handle = (*b).second;
            handle.eventHandler(out, (void*)&construction);
        }
        a = constructions.erase(a);
    }
    
    //for ( auto a = constructionsNow.begin(); a != constructionsNow.end(); a++ ) {
    for ( auto a = df::global::world->constructions.begin(); a != df::global::world->constructions.end(); a++ ) {
        df::construction* construction = *a;
        bool b = constructions.find(construction->pos) != constructions.end();
        constructions[construction->pos] = *construction;
        if ( b )
            continue;
        //construction created
        //out.print("Created construction (%d,%d,%d)\n", construction->pos.x,construction->pos.y,construction->pos.z);
        for ( auto b = copy.begin(); b != copy.end(); b++ ) {
            EventHandler handle = (*b).second;
            handle.eventHandler(out, (void*)construction);
        }
    }
}

static void manageSyndromeEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    multimap<Plugin*,EventHandler> copy(handlers[EventType::SYNDROME].begin(), handlers[EventType::SYNDROME].end());
    int32_t highestTime = -1;
    for ( auto a = df::global::world->units.all.begin(); a != df::global::world->units.all.end(); a++ ) {
        df::unit* unit = *a;
/*
        if ( unit->flags1.bits.dead )
            continue;
*/
        for ( size_t b = 0; b < unit->syndromes.active.size(); b++ ) {
            df::unit_syndrome* syndrome = unit->syndromes.active[b];
            int32_t startTime = syndrome->year*ticksPerYear + syndrome->year_time;
            if ( startTime > highestTime )
                highestTime = startTime;
            if ( startTime <= lastSyndromeTime )
                continue;
            
            SyndromeData data(unit->id, b);
            for ( auto c = copy.begin(); c != copy.end(); c++ ) {
                EventHandler handle = (*c).second;
                handle.eventHandler(out, (void*)&data);
            }
        }
    }
    lastSyndromeTime = highestTime;
}

static void manageInvasionEvent(color_ostream& out) {
    if (!df::global::ui)
        return;
    multimap<Plugin*,EventHandler> copy(handlers[EventType::INVASION].begin(), handlers[EventType::INVASION].end());

    if ( df::global::ui->invasions.next_id <= nextInvasion )
        return;
    nextInvasion = df::global::ui->invasions.next_id;

    for ( auto a = copy.begin(); a != copy.end(); a++ ) {
        EventHandler handle = (*a).second;
        handle.eventHandler(out, (void*)(nextInvasion-1));
    }
}

static void manageEquipmentEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    multimap<Plugin*,EventHandler> copy(handlers[EventType::INVENTORY_CHANGE].begin(), handlers[EventType::INVENTORY_CHANGE].end());
    
    unordered_map<int32_t, InventoryItem> itemIdToInventoryItem;
    unordered_set<int32_t> currentlyEquipped;
    for ( auto a = df::global::world->units.all.begin(); a != df::global::world->units.all.end(); a++ ) {
        itemIdToInventoryItem.clear();
        currentlyEquipped.clear();
        df::unit* unit = *a;
        /*if ( unit->flags1.bits.dead )
            continue;
        */
        
        auto oldEquipment = equipmentLog.find(unit->id);
        bool hadEquipment = oldEquipment != equipmentLog.end();
        vector<InventoryItem>* temp;
        if ( hadEquipment ) {
            temp = &((*oldEquipment).second);
        } else {
            temp = new vector<InventoryItem>;
        }
        //vector<InventoryItem>& v = (*oldEquipment).second;
        vector<InventoryItem>& v = *temp;
        for ( auto b = v.begin(); b != v.end(); b++ ) {
            InventoryItem& i = *b;
            itemIdToInventoryItem[i.itemId] = i;
        }
        for ( size_t b = 0; b < unit->inventory.size(); b++ ) {
            df::unit_inventory_item* dfitem_new = unit->inventory[b];
            currentlyEquipped.insert(dfitem_new->item->id);
            InventoryItem item_new(dfitem_new->item->id, *dfitem_new);
            auto c = itemIdToInventoryItem.find(dfitem_new->item->id);
            if ( c == itemIdToInventoryItem.end() ) {
                //new item equipped (probably just picked up)
                InventoryChangeData data(unit->id, NULL, &item_new);
                for ( auto h = copy.begin(); h != copy.end(); h++ ) {
                    EventHandler handle = (*h).second;
                    handle.eventHandler(out, (void*)&data);
                }
                continue;
            }
            InventoryItem item_old = (*c).second;
            
            df::unit_inventory_item& item0 = item_old.item;
            df::unit_inventory_item& item1 = item_new.item;
            if ( item0.mode == item1.mode && item0.body_part_id == item1.body_part_id && item0.wound_id == item1.wound_id )
                continue;
            //some sort of change in how it's equipped
            
            InventoryChangeData data(unit->id, &item_old, &item_new);
            for ( auto h = copy.begin(); h != copy.end(); h++ ) {
                EventHandler handle = (*h).second;
                handle.eventHandler(out, (void*)&data);
            }
        }
        if ( !hadEquipment )
            delete temp;
        //check for dropped items
        for ( auto b = v.begin(); b != v.end(); b++ ) {
            InventoryItem i = *b;
            if ( currentlyEquipped.find(i.itemId) != currentlyEquipped.end() )
                continue;
            //TODO: delete ptr if invalid
            InventoryChangeData data(unit->id, &i, NULL);
            for ( auto h = copy.begin(); h != copy.end(); h++ ) {
                EventHandler handle = (*h).second;
                handle.eventHandler(out, (void*)&data);
            }
        }
        
        //update equipment
        vector<InventoryItem>& equipment = equipmentLog[unit->id];
        equipment.clear();
        for ( size_t b = 0; b < unit->inventory.size(); b++ ) {
            df::unit_inventory_item* dfitem = unit->inventory[b];
            InventoryItem item(dfitem->item->id, *dfitem);
            equipment.push_back(item);
        }
    }
}

static void updateReportToRelevantUnits() {
    if (!df::global::world)
        return;
    if ( df::global::world->frame_counter <= reportToRelevantUnitsTime )
        return;
    reportToRelevantUnitsTime = df::global::world->frame_counter;
    
    for ( size_t a = 0; a < df::global::world->units.all.size(); a++ ) {
        df::unit* unit = df::global::world->units.all[a];
        for ( int16_t b = df::enum_traits<df::unit_report_type>::first_item_value; b <= df::enum_traits<df::unit_report_type>::last_item_value; b++ ) {
            if ( b == df::unit_report_type::Sparring )
                continue;
            for ( size_t c = 0; c < unit->reports.log[b].size(); c++ ) {
                int32_t report = unit->reports.log[b][c];
                if ( std::find(reportToRelevantUnits[report].begin(), reportToRelevantUnits[report].end(), unit->id) != reportToRelevantUnits[report].end() )
                    continue;
                reportToRelevantUnits[unit->reports.log[b][c]].push_back(unit->id);
            }
        }
    }
}

static void manageReportEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    multimap<Plugin*,EventHandler> copy(handlers[EventType::REPORT].begin(), handlers[EventType::REPORT].end());
    std::vector<df::report*>& reports = df::global::world->status.reports;
    size_t a = df::report::binsearch_index(reports, lastReport, false);
    //this may or may not be needed: I don't know if binsearch_index goes earlier or later if it can't hit the target exactly
    while (a < reports.size() && reports[a]->id <= lastReport) {
        a++;
    }
    for ( ; a < reports.size(); a++ ) {
        df::report* report = reports[a];
        for ( auto b = copy.begin(); b != copy.end(); b++ ) {
            EventHandler handle = (*b).second;
            handle.eventHandler(out, (void*)report->id);
        }
        lastReport = report->id;
    }
}

static df::unit_wound* getWound(df::unit* attacker, df::unit* defender) {
    for ( size_t a = 0; a < defender->body.wounds.size(); a++ ) {
        df::unit_wound* wound = defender->body.wounds[a];
        if ( wound->age <= 1 && wound->unit_id == attacker->id ) {
            return wound;
        }
    }
    return NULL;
}

static void manageUnitAttackEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    multimap<Plugin*,EventHandler> copy(handlers[EventType::UNIT_ATTACK].begin(), handlers[EventType::UNIT_ATTACK].end());
    std::vector<df::report*>& reports = df::global::world->status.reports;
    size_t a = df::report::binsearch_index(reports, lastReportUnitAttack, false);
    //this may or may not be needed: I don't know if binsearch_index goes earlier or later if it can't hit the target exactly
    while (a < reports.size() && reports[a]->id <= lastReportUnitAttack) {
        a++;
    }
    std::set<int32_t> strikeReports;
    for ( ; a < reports.size(); a++ ) {
        df::report* report = reports[a];
        lastReportUnitAttack = report->id;
        if ( report->flags.bits.continuation )
            continue;
        df::announcement_type type = report->type;
        if ( type == df::announcement_type::COMBAT_STRIKE_DETAILS ) {
            strikeReports.insert(report->id);
        }
    }
    
    if ( strikeReports.empty() )
        return;
    updateReportToRelevantUnits();
    map<int32_t, map<int32_t, int32_t> > alreadyDone;
    for ( auto a = strikeReports.begin(); a != strikeReports.end(); a++ ) {
        int32_t reportId = *a;
        df::report* report = df::report::find(reportId);
        if ( !report )
            continue; //TODO: error
        std::string reportStr = report->text;
        for ( int32_t b = reportId+1; ; b++ ) {
            df::report* report2 = df::report::find(b);
            if ( !report2 )
                break;
            if ( report2->type != df::announcement_type::COMBAT_STRIKE_DETAILS )
                break;
            if ( !report2->flags.bits.continuation )
                break;
            reportStr = reportStr + report2->text;
        }
        
        std::vector<int32_t>& relevantUnits = reportToRelevantUnits[report->id];
        if ( relevantUnits.size() != 2 ) {
            continue;
        }
        
        df::unit* unit1 = df::unit::find(relevantUnits[0]);
        df::unit* unit2 = df::unit::find(relevantUnits[1]);
        
        df::unit_wound* wound1 = getWound(unit1,unit2);
        df::unit_wound* wound2 = getWound(unit2,unit1);
        
        if ( wound1 && !alreadyDone[unit1->id][unit2->id] ) {
            UnitAttackData data;
            data.attacker = unit1->id;
            data.defender = unit2->id;
            data.wound = wound1->id;
            
            alreadyDone[data.attacker][data.defender] = 1;
            for ( auto b = copy.begin(); b != copy.end(); b++ ) {
                EventHandler handle = (*b).second;
                handle.eventHandler(out, (void*)&data);
            }
        }
        
        if ( wound2 && !alreadyDone[unit1->id][unit2->id] ) {
            UnitAttackData data;
            data.attacker = unit2->id;
            data.defender = unit1->id;
            data.wound = wound2->id;
            
            alreadyDone[data.attacker][data.defender] = 1;
            for ( auto b = copy.begin(); b != copy.end(); b++ ) {
                EventHandler handle = (*b).second;
                handle.eventHandler(out, (void*)&data);
            }
        }

        if ( unit1->flags1.bits.dead ) {
            UnitAttackData data;
            data.attacker = unit2->id;
            data.defender = unit1->id;
            data.wound = -1;
            alreadyDone[data.attacker][data.defender] = 1;
            for ( auto b = copy.begin(); b != copy.end(); b++ ) {
                EventHandler handle = (*b).second;
                handle.eventHandler(out, (void*)&data);
            }
        }
        
        if ( unit2->flags1.bits.dead ) {
            UnitAttackData data;
            data.attacker = unit1->id;
            data.defender = unit2->id;
            data.wound = -1;
            alreadyDone[data.attacker][data.defender] = 1;
            for ( auto b = copy.begin(); b != copy.end(); b++ ) {
                EventHandler handle = (*b).second;
                handle.eventHandler(out, (void*)&data);
            }
        }
        
        if ( !wound1 && !wound2 ) {
            //if ( unit1->flags1.bits.dead || unit2->flags1.bits.dead )
            //    continue;
            if ( reportStr.find("severed part") )
                continue;
            if ( Once::doOnce("EventManager neither wound") ) {
                out.print("%s, %d: neither wound: %s\n", __FILE__, __LINE__, reportStr.c_str());
            }
        }
    }
}

static std::string getVerb(df::unit* unit, std::string reportStr) {
    std::string result(reportStr);
    std::string name = unit->name.first_name + " ";
    bool useName = strncmp(result.c_str(), name.c_str(), name.length()) == 0;
    if ( useName ) {
        result = result.substr(name.length());
        result = result.substr(0,result.length()-1);
        return result;
    }
    //use profession name
    std::string profession = "The " + Units::getProfessionName(unit) + " ";
    bool match = strncmp(result.c_str(), profession.c_str(), profession.length()) == 0;
    if ( !match )
        return "";
    result = result.substr(profession.length());
    result = result.substr(0,result.length()-1);
    //TODO: special case for the player in adventure mode
    return result;
}

static void manageInteractionEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    multimap<Plugin*,EventHandler> copy(handlers[EventType::INTERACTION].begin(), handlers[EventType::INTERACTION].end());
    std::vector<df::report*>& reports = df::global::world->status.reports;
    size_t a = df::report::binsearch_index(reports, lastReportInteraction, false);
    while (a < reports.size() && reports[a]->id <= lastReportInteraction) {
        a++;
    }
    if ( a < reports.size() )
        updateReportToRelevantUnits();

    int32_t attackerId = -1;
    int32_t lastTime = -1;
    std::string attackVerb;
    int32_t attackReport = -1;
    for ( ; a < reports.size(); a++ ) {
        df::report* report = reports[a];
        lastReportInteraction = report->id;
        if ( report->flags.bits.continuation )
            continue;
        df::announcement_type type = report->type;
        if ( type != df::announcement_type::INTERACTION_ACTOR && type != df::announcement_type::INTERACTION_TARGET )
            continue;
        int32_t unitId = -1;
        int32_t validCount = 0;
        std::string verb;
        //find relevant unit
        for ( auto b = reportToRelevantUnits[report->id].begin(); b != reportToRelevantUnits[report->id].end(); b++ ) {
            int32_t candidateId = *b;
            df::unit* candidate = df::unit::find(candidateId);
            if ( !candidate ) {
                //TODO: error
                continue;
            }
            if ( candidate->pos != report->pos )
                continue;
            std::string verbC = getVerb(candidate, report->text);
            if ( verbC.length() == 0 )
                continue;
            verb = verbC;
            validCount++;
            unitId = candidateId;
        }
        if ( validCount > 1 ) {
            if ( Once::doOnce("EventManager interaction too many actors") ) {
                out.print("%s:%d: too many actors for report %d\n", __FILE__, __LINE__, report->id);
                out.print("reportStr = \"%s\", pos = %d,%d,%d\n", report->text.c_str(), report->pos.x, report->pos.y, report->pos.z);
            }
            attackerId = -1;
            continue;
        }
        if ( validCount == 0 ) {
            if ( Once::doOnce("EventManager interaction too few actors") ) {
                out.print("%s:%d: too few actors for report %d\n", __FILE__, __LINE__, report->id);
                out.print("reportStr = \"%s\", pos = %d,%d,%d\n", report->text.c_str(), report->pos.x, report->pos.y, report->pos.z);
            }
            attackerId = -1;
            continue;
        }
        //int32_t unitId = reportToRelevantUnits[report->id][0];
        bool isActor = attackerId == -1;//type == df::announcement_type::INTERACTION_ACTOR;

        if ( isActor ) {
            attackReport = report->id;
            attackerId = unitId;
            lastTime = report->year*ticksPerYear + report->time;
            attackVerb = verb;
            continue;
        }
        
        if ( attackerId == -1 )
            continue;
        if ( report->year*ticksPerYear + report->time != lastTime ) {
            attackerId = -1;
            continue;
        }
        InteractionData data;
        data.attacker = attackerId;
        data.defender = unitId;
        data.attackReport = attackReport;
        data.defendReport = report->id;
        data.attackVerb = attackVerb;
        data.defendVerb = verb;
        for ( auto b = copy.begin(); b != copy.end(); b++ ) {
            EventHandler handle = (*b).second;
            handle.eventHandler(out, (void*)&data);
        }
    }
}

