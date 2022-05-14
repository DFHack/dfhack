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
#include "df/historical_figure.h"
#include "df/interaction.h"
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
#include <array>

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
static std::unordered_map<EventHandler,int32_t> eventLastTick;

static const int32_t ticksPerYear = 403200;

// this function is only used within the file in registerListener and manageTickEvent
void enqueueTickEvent(EventHandler &handler){
    int32_t when = 0;
    df::world* world = df::global::world;
    if ( world ) {
        when = world->frame_counter + handler.freq;
    } else {
        if ( Once::doOnce("EventManager registerListener unhonored absolute=false") )
            Core::getInstance().getConsole().print("EventManager::registerTick: warning! absolute flag=false not honored.\n");
    }
    handler.when = when;
    tickQueue.emplace(handler.when, handler);
}

void DFHack::EventManager::registerListener(EventType::EventType e, EventHandler handler, Plugin* plugin, bool backlog) {
    if(e == EventType::TICK){
        enqueueTickEvent(handler);
    }
    int32_t tick = backlog ? -2 : (df::global::world ? df::global::world->frame_counter : -1);
    eventLastTick[handler] = tick; // received ranges.. // backlog: -1,n // !world: 0,n // world: tick,n
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
    handler.when = when;
    tickQueue.insert(pair<int32_t, EventHandler>(handler.when, handler));
    // we don't track this handler, this allows registerTick to retain the old behaviour of needing to re-register the tick event
    //handlers[EventType::TICK].insert(pair<Plugin*,EventHandler>(plugin,handler));
    // since the event isn't added to the handlers, we don't need to unregister these events
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
        EventHandler &handle = (*i).second;
        if ( handle != handler ) {
            i++;
            continue;
        }
        i = handlers[e].erase(i);
        eventLastTick.erase(handler);
        if ( e == EventType::TICK )
            removeFromTickQueue(handler);
    }
}

void DFHack::EventManager::unregisterAll(Plugin* plugin) {
    for ( auto i = handlers[EventType::TICK].find(plugin); i != handlers[EventType::TICK].end(); ++i ) {
        if ( (*i).first != plugin )
            break;

        eventLastTick.erase(i->second);
        removeFromTickQueue((*i).second);
    }
    for (auto &event_type : handlers) {
        for (auto i = event_type.find(plugin); i != event_type.end(); ++i ) {
            if ( (*i).first != plugin )
                break;

            eventLastTick.erase(i->second);
        }
        event_type.erase(plugin);
    }
}

static void manageTickEvent(color_ostream& out);
static void manageJobInitiatedEvent(color_ostream& out);
static void manageJobStartedEvent(color_ostream& out);
static void manageJobCompletedEvent(color_ostream& out);
static void manageUnitNewActiveEvent(color_ostream& out);
static void manageUnitDeathEvent(color_ostream& out);
static void manageItemCreationEvent(color_ostream& out);
static void manageBuildingEvent(color_ostream& out);
static void manageBuildingCreatedEvent(color_ostream& out);
static void manageBuildingDestroyedEvent(color_ostream& out);
static void manageConstructionEvent(color_ostream& out);
static void manageConstructionAddedEvent(color_ostream& out);
static void manageConstructionRemovedEvent(color_ostream& out);
static void manageSyndromeEvent(color_ostream& out);
static void manageInvasionEvent(color_ostream& out);
static void manageInventoryChangeEvent(color_ostream& out);
static void manageReportEvent(color_ostream& out);
static void manageUnitAttackEvent(color_ostream& out);
static void manageUnloadEvent(color_ostream& out){};
static void manageInteractionEvent(color_ostream& out);

typedef void (*eventManager_t)(color_ostream&);

// integrate new events into this function, and no longer worry about syncing the enum list with the `eventManager` array
eventManager_t getManager(EventType::EventType t) {
    switch (t) {
        case EventType::TICK:
            return manageTickEvent;
        case EventType::JOB_INITIATED:
            return manageJobInitiatedEvent;
        case EventType::JOB_STARTED:
            return manageJobStartedEvent;
        case EventType::JOB_COMPLETED:
            return manageJobCompletedEvent;
        case EventType::UNIT_NEW_ACTIVE:
            return manageUnitNewActiveEvent;
        case EventType::UNIT_DEATH:
            return manageUnitDeathEvent;
        case EventType::ITEM_CREATED:
            return manageItemCreationEvent;
        case EventType::BUILDING_DESTROYED:
            return manageBuildingDestroyedEvent;
        case EventType::BUILDING_CREATED:
            return manageBuildingCreatedEvent;
        case EventType::BUILDING:
            return manageBuildingEvent;
        case EventType::CONSTRUCTION_REMOVED:
            return manageConstructionRemovedEvent;
        case EventType::CONSTRUCTION_ADDED:
            return manageConstructionAddedEvent;
        case EventType::CONSTRUCTION:
            return manageConstructionEvent;
        case EventType::SYNDROME:
            return manageSyndromeEvent;
        case EventType::INVASION:
            return manageInvasionEvent;
        case EventType::INVENTORY_CHANGE:
            return manageInventoryChangeEvent;
        case EventType::REPORT:
            return manageReportEvent;
        case EventType::UNIT_ATTACK:
            return manageUnitAttackEvent;
        case EventType::UNLOAD:
            return manageUnloadEvent;
        case EventType::INTERACTION:
            return manageInteractionEvent;
        case EventType::EVENT_MAX:
            return nullptr;
            //default:
            //we don't do this... because then the compiler wouldn't error for missing cases in the enum
    }
    return nullptr;
}

std::array<eventManager_t,EventType::EVENT_MAX> compileManagerArray() {
    std::array<eventManager_t, EventType::EVENT_MAX> managers{};
    auto t = (EventType::EventType) 0;
    while (t < EventType::EVENT_MAX) {
        managers[t] = getManager(t);
        t = (EventType::EventType) int(t + 1);
    }
    return managers;
}

template<typename T>
class event_tracker { //todo: use inheritance? stl seems to use variadics, so it's unclear how well that would actually work
private:
    std::unordered_map<T, int32_t> seen;
    std::multimap<int32_t, T> history;
public:
    event_tracker()= default;
    void clear() {
        seen.clear();
        history.clear();
    }
    bool contains(const T &event_data) {
        return seen.find(event_data) != seen.end();
    }
    typename std::multimap<int32_t, T>::iterator find(const T &event_data) {
        auto iter = seen.find(event_data);
        if(iter != seen.end()){
            auto tick = iter->second;
            for(auto jter = history.lower_bound(tick); jter != history.end(); ++jter) {
                if (jter->second == event_data) {
                    return jter;
                }
            }
        }
        return history.end();
    }
    typename std::multimap<int32_t, T>::iterator begin() {
        return history.begin();
    }
    typename std::multimap<int32_t, T>::iterator end() {
        return history.end();
    }
    typename std::multimap<int32_t, T>::iterator upper_bound(const int32_t &tick) {
        return history.upper_bound(tick);
    }
    std::pair<typename std::multimap<int32_t, T>::iterator, bool> emplace(const int32_t &tick, const T &event_data) {
        // data structure is replacing uses of std::unordered_set, so we can do this if(!contains)
        if (!contains(event_data)) {
            seen.emplace(event_data, tick);
            return std::make_pair(history.emplace(tick, event_data), true);
        }
        return std::make_pair(history.end(), false);
    }
    typename std::multimap<int32_t, T>::iterator erase(typename std::multimap<int32_t, T>::iterator &pos) {
        if (pos != history.end()) {
            seen.erase(pos->second);
            return history.erase(pos);
        }
        return history.end();
    }
    typename std::multimap<int32_t, T>::iterator erase(const T& event_data) {
        auto iter = seen.find(event_data);
        if(iter != seen.end()){
            auto tick = iter->second;
            seen.erase(iter);
            for(auto jter = history.lower_bound(tick); jter != history.end(); ++jter) {
                if(jter->second == event_data) {
                    return history.erase(jter);
                }
            }
        }
        return history.end();
    }
};

static int32_t getTime(){
    if(df::global::world) {
        return (*df::global::cur_year) * ticksPerYear + (*df::global::cur_year_tick);
    }
    return -1;
}

//job initiated
static int32_t lastJobId = -1;
// static std::unordered_set<int32_t> newJobIDs; // function fulfilled by lastJobId
static event_tracker<df::job*> newJobs;

//job started
static std::unordered_set<int32_t> startedJobIDs;
static event_tracker<df::job*> startedJobs;

//job completed
//static std::unordered_set<int32_t> completedJobIDs;
static event_tracker<df::job*> completedJobs;

//new unit active
static event_tracker<int32_t> activeUnits;

//unit death
static event_tracker<int32_t> deadUnits;

//item creation
static int32_t nextItem;
static event_tracker<int32_t> newItems;

//building
static int32_t nextBuilding;
static event_tracker<int32_t> createdBuildings;
static event_tracker<int32_t> destroyedBuildings;

//construction
static event_tracker<df::construction> createdConstructions;
static event_tracker<df::construction> destroyedConstructions;
static bool gameLoaded;

//syndrome
static event_tracker<SyndromeData> syndromes;

//invasion
static int32_t nextInvasion;
static event_tracker<int32_t> invasions;

//equipment change
static unordered_map<int32_t, unordered_map<int32_t, InventoryItem> > inventoryLog;
static event_tracker<InventoryChangeData> equipmentChanges;

//report
static event_tracker<int32_t> newReports;

//unit attack
static int32_t lastReportUnitAttack;
static event_tracker<UnitAttackData> attackEvents;
static std::map<int32_t,std::vector<int32_t> > reportToRelevantUnits;
static int32_t reportToRelevantUnitsTime = -1;

//interaction
static int32_t lastReportInteraction;
static event_tracker<InteractionData> interactionEvents;

void DFHack::EventManager::onStateChange(color_ostream& out, state_change_event event) {
    static bool doOnce = false;
//    const string eventNames[] = {"world loaded", "world unloaded", "map loaded", "map unloaded", "viewscreen changed", "core initialized", "begin unload", "paused", "unpaused"};
//    out.print("%s,%d: onStateChange %d: \"%s\"\n", __FILE__, __LINE__, (int32_t)event, eventNames[event].c_str());
    if ( !doOnce ) {
        //TODO: put this somewhere else
        doOnce = true;
        EventHandler buildingHandler(Buildings::updateBuildings, 100);
        DFHack::EventManager::registerListener(EventType::BUILDING, buildingHandler, nullptr);
        //out.print("Registered listeners.\n %d", __LINE__);
    }
    if ( event == DFHack::SC_MAP_UNLOADED ) {
        lastJobId = -1;
        for (auto &key_value : newJobs) {
            Job::deleteJobStruct(key_value.second, true);
        }
        for (auto &key_value : startedJobs) {
            Job::deleteJobStruct(key_value.second, true);
        }
        for (auto &key_value : completedJobs) {
            Job::deleteJobStruct(key_value.second, true);
        }
//        for (auto &key_value : inventoryLog) {
//            //key_value
//        }
        newJobs.clear();
        startedJobs.clear();
        completedJobs.clear();
        activeUnits.clear();
        deadUnits.clear();
        createdBuildings.clear();
        destroyedBuildings.clear();
        createdConstructions.clear();
        destroyedConstructions.clear();
        inventoryLog.clear();
        equipmentChanges.clear();
        newReports.clear();
        //todo: clear reportToRelevantUnits?
        tickQueue.clear();

        Buildings::clearBuildings(out);
        lastReportUnitAttack = -1;
        gameLoaded = false;

        multimap<Plugin*,EventHandler> copy(handlers[EventType::UNLOAD].begin(), handlers[EventType::UNLOAD].end());
        for (auto &key_value : copy) {
            key_value.second.eventHandler(out, nullptr);
        }
    } else if ( event == DFHack::SC_MAP_LOADED ) {

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

        // initialize our started jobs list
        for ( df::job_list_link* link = df::global::world->jobs.list.next; link != nullptr; link = link->next ) {
            df::job* job = link->item;
            if (job && Job::getWorker(job)) {
                // the job was already started, so emplace to a non-iterable area
                startedJobIDs.emplace(job->id);
                startedJobs.emplace(-1, Job::cloneJobStruct(job, true));
            }
        }
        // initialize our buildings list
        for (df::building* building : df::global::world->buildings.all) {
            Buildings::updateBuildings(out, (void*)intptr_t(building->id));
            // building already existed, so emplace to a non-iterable area
            createdBuildings.emplace(-1, building->id);
        }
        // initialize our constructions list
        for (auto construction : df::global::world->constructions) {
            if ( !construction ) {
                if ( Once::doOnce("EventManager.onLoad null constr") ) {
                    out.print("EventManager.onLoad: null construction.\n");
                }
                continue;
            }
            if (construction->pos == df::coord() ) {
                if ( Once::doOnce("EventManager.onLoad null position of construction.\n") ) {
                    out.print("EventManager.onLoad null position of construction.\n");
                }
                continue;
            }
            createdConstructions.emplace(-1, *construction);
        }
        int32_t current_time = getTime();
        // initialize our active units list
        // initialize our dead units list
        // initialize our inventory lists
        // initialize our syndromes list
        for ( df::unit *unit : df::global::world->units.all ) {
            if(Units::isActive(unit)) {
                activeUnits.emplace(-1, unit->id);
            }
            if (Units::isDead(unit)) {
                deadUnits.emplace(-1, unit->id);
            }
            for (size_t idx = 0; idx < unit->syndromes.active.size(); ++idx) {
                auto &syndrome = unit->syndromes.active[idx];
                int32_t startTime = syndrome->year * ticksPerYear + syndrome->year_time;
                // add the syndrome if it started now or in the past
                if (startTime <= current_time) {
                    SyndromeData data(unit->id, (int32_t)idx);
                    syndromes.emplace(-1, data);
                }
            }
            //update equipment
            unordered_map<int32_t, InventoryItem> &last_tick_inventory = inventoryLog[unit->id];
            vector<df::unit_inventory_item*> &current_tick_inventory = unit->inventory;
            last_tick_inventory.clear();
            for (auto ct_item : current_tick_inventory) {
                auto itemId = ct_item->item->id;
                last_tick_inventory.emplace(itemId, InventoryItem(itemId, *ct_item));
            }
        }
        // initialize our reports list
        for(auto &r : df::global::world->status.reports){
            newReports.emplace(-1, r->id);
        }
        lastReportUnitAttack = -1;
        lastReportInteraction = -1;
        reportToRelevantUnitsTime = -1;
        reportToRelevantUnits.clear();
//        for ( size_t a = 0; a < EventType::EVENT_MAX; ++a ) {
//            eventLastTick[a] = -1;//-1000000;
//        }
        for (auto unit : df::global::world->history.figures) {
            if ( unit->id < 0 && unit->name.language < 0 )
                unit->name.language = 0;
        }

        gameLoaded = true;
    }
}

void DFHack::EventManager::manageEvents(color_ostream& out) {
    static const std::array<eventManager_t, EventType::EVENT_MAX> eventManager = compileManagerArray();
    if ( !gameLoaded ) {
        return;
    }
    if (!df::global::world)
        return;

    CoreSuspender suspender;

    // iterate the event types
    for (size_t type = 0; type < EventType::EVENT_MAX; type++ ) {
        if (handlers[type].empty())
            continue;
        // events might be missed if we only do the processing when a listener wants to receive the events
        eventManager[type](out);
        /*bool call_events = false;
        // iterate event type's handlers
        for (auto &key_value : handlers[type]) {
            const EventHandler &handler = key_value.second;
            int32_t last_tick = eventLastTick[handler];
            // see if there is at least one handler ready to fire
            if (tick - last_tick >= handler.freq){
                call_events = true;
                break;
            }
        }
        if(call_events)*/
    }
}

static void manageTickEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    int32_t tick = df::global::world->frame_counter;
    unordered_set<EventHandler> requeue;
    // call due tick events
    auto end = tickQueue.upper_bound(tick);
    for (auto iter = tickQueue.begin(); iter != end;) {
        EventHandler &handler = iter->second;
        handler.eventHandler(out, (void*) intptr_t(tick));
        requeue.emplace(handler);
        iter = tickQueue.erase(iter);
    }
    // re-register tick events (only registered tick events though)
    for (auto &key_value : handlers[EventType::TICK]) {
        if (requeue.empty()) {
            break;
        }
        // check that this handler was fired
        auto &handler = key_value.second;
        if (requeue.erase(handler) != 0){
            /**
             * todo(1): move re-queue to loop above (would mean ALL tick events get re-queued)
             * todo(2): don't even use a queue, just follow the same pattern as other managers (fire handler according to freq, removes need for `when` member of handlers)
             */
            // any registered handler that was fired must be re-queued
            enqueueTickEvent(handler);
        }
    }
}

static void manageJobInitiatedEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    if (!df::global::job_next_id)
        return;
    int32_t tick = df::global::world->frame_counter;
    int32_t oldest_last_tick = (uint16_t)-1;

    // update new jobs list
    for ( df::job_list_link* link = df::global::world->jobs.list.next; link != nullptr; link = link->next ) {
        df::job* job = link->item;
        // the jobs must come after the last known job id
        if (job && job->id > lastJobId) {
            newJobs.emplace(tick, Job::cloneJobStruct(link->item, true));
        }
    }
    lastJobId = *df::global::job_next_id - 1;

    // iterate event handler callbacks
    multimap<Plugin*,EventHandler> copy(handlers[EventType::JOB_INITIATED].begin(), handlers[EventType::JOB_INITIATED].end());
    for (auto &key_value : copy) {
        auto &handler = key_value.second;
        auto last_tick = eventLastTick[handler];
        oldest_last_tick = oldest_last_tick < last_tick ? oldest_last_tick : last_tick;
        // make sure the frequency of this handler is obeyed
        if (tick - last_tick >= handler.freq) {
            eventLastTick[handler] = tick;
            // send the handler all the new jobs since it last fired
            auto jter = newJobs.upper_bound(last_tick);
            for(;jter != newJobs.end(); ++jter){
                handler.eventHandler(out, (void*) jter->second);
            }
        }
    }
    // clean up memory we no longer need
    auto iter = newJobs.begin();
    for(; iter != newJobs.end() && iter->first < oldest_last_tick;) {
        // we cloned it we delete it
        Job::deleteJobStruct(iter->second, true);
        // if we delete it, we best not reference it
        iter = newJobs.erase(iter);
    }


}

static void manageJobStartedEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    int32_t tick = df::global::world->frame_counter;
    int32_t oldest_last_tick = (uint16_t)-1;

    // update the started jobs list for the current tick
    for (df::job_list_link* link = df::global::world->jobs.list.next; link != nullptr; link = link->next) {
        df::job* job = link->item;
        // the jobs must have a worker to start
        if (job && Job::getWorker(job)) {
            // the job won't be added if it already exists
            if (startedJobIDs.emplace(job->id).second) {
                // DF is going to delete completed jobs, and we might not send these before they're completed
                startedJobs.emplace(tick, Job::cloneJobStruct(job, true));
            }
        }
    }
    // iterate event handler callbacks
    multimap<Plugin*, EventHandler> copy(handlers[EventType::JOB_STARTED].begin(), handlers[EventType::JOB_STARTED].end());
    for (auto &key_value : copy) {
        auto &handler = key_value.second;
        auto last_tick = eventLastTick[handler];
        oldest_last_tick = oldest_last_tick < last_tick ? oldest_last_tick : last_tick;
        // make sure the frequency of this handler is obeyed
        if (tick - last_tick >= handler.freq) {
            eventLastTick[handler] = tick;
            // send the handler all the new jobs since it last fired
            auto jter = startedJobs.upper_bound(last_tick);
            for (; jter != startedJobs.end(); ++jter) {
                // todo: entity check? before/inside handler loop?
                handler.eventHandler(out, (void*) jter->second);
            }
        }
    }
    // clean up memory we no longer need
    auto iter = startedJobs.begin();
    for(; iter != startedJobs.end() && iter->first < oldest_last_tick;) {
        startedJobIDs.erase(iter->second->id);
        // we cloned it we delete it
        Job::deleteJobStruct(iter->second, true);
        // if we delete it, we best not reference it
        iter = startedJobs.erase(iter);
    }
}
//helper function for manageJobCompletedEvent
//static int32_t getWorkerID(df::job* job) {
//    auto ref = findRef(job->general_refs, general_ref_type::UNIT_WORKER);
//    return ref ? ref->getID() : -1;
//}

/*
TODO: consider checking item creation / experience gain just in case
*/
static void manageJobCompletedEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    static std::unordered_map<int32_t, df::job*> last_tick_jobs;
    static int32_t fn_last_tick = -1;
    int32_t tick = df::global::world->frame_counter;
    int32_t oldest_last_tick = (uint16_t)-1;
    std::unordered_map<int32_t, df::job*> current_jobs;

    // update the completed jobs list (this apparently isn't as straight forward as one might think)
    // todo: verify the need for this check
    //if it happened within a tick, must have been cancelled by the user or a plugin: not completed
    if(tick > fn_last_tick) {
        // we're going to need to check if started jobs are still on this list, so we copy the IDs to avoid a linear search into this list for each started_job
        for (df::job_list_link* link = df::global::world->jobs.list.next; link != nullptr; link = link->next) {
            df::job* job = link->item;
            //can't really minimize the list, because maybe somehow a non-started job will be complete next tick (hax)
            //if(job && Job::getWorker(job)) {
            current_jobs.emplace(job->id, Job::cloneJobStruct(job, true)); // clone now so we can swap into last_tick_jobs later
            //}
        }
        // iterate the started jobs list
        for (auto &key_value: last_tick_jobs) {
            df::job* lt_job = key_value.second;
            int32_t id = lt_job->id;
            // check for the started job in the current jobs (the jobs we just copied above)
            auto cter = current_jobs.find(id);
            if (cter != current_jobs.end()) {
                // if we found it, that doesn't mean it didn't complete.. it may be a repeat job
                df::job* ct_job = cter->second; //ct: current tick
                if (!lt_job->flags.bits.repeat)
                    continue;
                // I'd comment these, but I don't know what we're checking exactly.. gl
                if (lt_job->completion_timer != 0)
                    continue;
                if (ct_job->completion_timer != -1)
                    continue;

                //still false positive if cancelled at EXACTLY the right time, but experiments show this doesn't happen

                auto clone = Job::cloneJobStruct(lt_job, true);
                // the job won't be added if it already exists - NOTE: this means a repeat job can't be seen as completed again until it gets cleaned from the list
                if (!completedJobs.emplace(tick, clone).second) {
                    // the clone wasn't accepted, so we delete it to clean memory
                    Job::deleteJobStruct(clone, true);
                }
                continue;
            }
            // we didn't find it, so it's a recently finished or cancelled job
            if (lt_job->flags.bits.repeat || lt_job->completion_timer != 0)
                continue;
            completedJobs.emplace(tick, Job::cloneJobStruct(lt_job, true));
        }
    }

    multimap<Plugin*,EventHandler> copy(handlers[EventType::JOB_COMPLETED].begin(), handlers[EventType::JOB_COMPLETED].end());
    // iterate event handler callbacks
    for (auto &key_value : copy) {
        auto &handler = key_value.second;
        auto last_tick = eventLastTick[handler];
        oldest_last_tick = oldest_last_tick < last_tick ? oldest_last_tick : last_tick;
        // enforce handler's callback frequency
        if (tick - last_tick >= handler.freq) {
            eventLastTick[handler] = tick;
            fn_last_tick = tick;
            // send the handler all the newly completed jobs since it last fired
            auto iter = completedJobs.upper_bound(last_tick);
            for (; iter != completedJobs.end(); ++iter) {
                handler.eventHandler(out, iter->second);
            }
        }
    }
    // clean up memory of job clones which have been sent to all handlers
    auto iter = completedJobs.begin();
    for(; iter != completedJobs.end() && iter->first < oldest_last_tick;) {
        // we cloned it we delete it
        Job::deleteJobStruct(iter->second, true);
        // if we delete it, we best not reference it
        iter = completedJobs.erase(iter);
    }
    // clean up memory of job clones from last tick
    for(auto &key_value : last_tick_jobs){
        Job::deleteJobStruct(key_value.second, true);
    }
    // last tick jobs takes on the current tick's jobs
    last_tick_jobs.swap(current_jobs);

    // moved to the bottom to be out of the way, but also all the structures used herein no longer exist
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
            "\n", job1.list_link->item, job1.id, job1.job_type, ENUM_ATTR(job_type, caption, job1.job_type), job1.flags.bits.working, job1.completion_timer, getWorkerID(&job1), last_tick, tick);
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
                ,job0.list_link == NULL ? 0 : job0.list_link->item, job0.id, job0.job_type, ENUM_ATTR(job_type, caption, job0.job_type), job0.flags.bits.working, job0.completion_timer, getWorkerID(&job0), last_tick, tick);
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
            fn_last_tick, tick
        );
    }
#endif
}

static void manageUnitNewActiveEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    int32_t tick = df::global::world->frame_counter;
    // update active units list for the current tick
    for (df::unit* unit : df::global::world->units.active) {
        int32_t id = unit->id;
        if (!activeUnits.contains(id)) {
            activeUnits.emplace(tick, id);
        }
    }
    multimap<Plugin*,EventHandler> copy(handlers[EventType::UNIT_NEW_ACTIVE].begin(), handlers[EventType::UNIT_NEW_ACTIVE].end());
    // iterate event handler callbacks
    for (auto &key_value : copy) {
        auto &handler = key_value.second;
        auto last_tick = eventLastTick[handler];
        // enforce handler's callback frequency
        if(tick - last_tick >= handler.freq) {
            eventLastTick[handler] = tick;
            // send the handler all the new active unit id's since it last fired
            auto jter = activeUnits.upper_bound(last_tick);
            for(;jter != activeUnits.end(); ++jter){
                // todo: entity check? before/inside handler loop?
                handler.eventHandler(out, (void*) intptr_t(jter->second)); // intptr_t() avoids cast from smaller type warning
            }
        }
    }
}

static void manageUnitDeathEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    int32_t tick = df::global::world->frame_counter;
    // update dead units list for the current tick
    for (df::unit* unit: df::global::world->units.all) {
        //if ( unit->counters.death_id == -1 ) {
        if (Units::isDead(unit)) {
            if (!deadUnits.contains(unit->id)) {
                deadUnits.emplace(tick, unit->id);
                activeUnits.erase(unit->id);
            }
        } else if (deadUnits.contains(unit->id)) {
            // unit must have been revived
            deadUnits.erase(unit->id);
        }
    }
    // iterate event handler callbacks
    multimap<Plugin*, EventHandler> copy(handlers[EventType::UNIT_DEATH].begin(), handlers[EventType::UNIT_DEATH].end());
    for (auto &key_value : copy) {
        auto &handler = key_value.second;
        auto last_tick = eventLastTick[handler];
        // enforce handler's callback frequency
        if (tick - last_tick >= handler.freq) {
            eventLastTick[handler] = tick;
            // send the handler all the new dead unit id's since it last fired
            auto jter = deadUnits.upper_bound(last_tick);
            for(; jter != deadUnits.end(); ++jter) {
                handler.eventHandler(out, (void*) intptr_t(jter->second)); // intptr_t() avoids cast from smaller type warning
            }
        }
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
    int32_t tick = df::global::world->frame_counter;
    size_t index = df::item::binsearch_index(df::global::world->items.all, nextItem, false);
    if ( index != 0 ) index--;
    for (size_t a = index; a < df::global::world->items.all.size(); ++a) {
        df::item* item = df::global::world->items.all[a];
        //already processed
        if (item->id < nextItem)
            continue;
        //invaders
        if (item->flags.bits.foreign)
            continue;
        //traders who bring back your items?
        if (item->flags.bits.trader)
            continue;
        //migrants
        if (item->flags.bits.owned)
            continue;
        //spider webs don't count
        if (item->flags.bits.spider_web)
            continue;
        newItems.emplace(tick, item->id);
    }
    nextItem = *df::global::item_next_id;

    // iterate event handlers
    multimap<Plugin*,EventHandler> copy(handlers[EventType::ITEM_CREATED].begin(), handlers[EventType::ITEM_CREATED].end());
    for (auto &key_value : copy) {
        auto &handler = key_value.second;
        auto last_tick = eventLastTick[handler];
        // enforce handler's callback frequency
        if (tick - last_tick >= handler.freq) {
            eventLastTick[handler] = tick;
            // send the handler all the new item id's since it last fired
            auto iter = newItems.upper_bound(last_tick);
            for (; iter != newItems.end(); ++iter) {
                handler.eventHandler(out, (void*) intptr_t(iter->second));
            }
        }
    }

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
    int32_t tick = df::global::world->frame_counter;
    // update destroyed building list
    for (auto iter = createdBuildings.begin(); iter != createdBuildings.end();) {
        int32_t id = iter->second;
        int32_t index = df::building::binsearch_index(df::global::world->buildings.all, id);
        // continue if we found the id in world->buildings.all
        if (index != -1) {
            ++iter;
            continue;
        }
        // pretty sure we'd invalidate our loop if we added to buildings here, so we just save the id in an intermediary for now
        destroyedBuildings.emplace(tick, id);
        // handlers which haven't handled this building yet aren't going to (it would be very tricky to make this work)
        iter = createdBuildings.erase(iter);
    }
    // update created building list
    for (int32_t id = nextBuilding; id < *df::global::building_next_id; ++id) {
        int32_t index = df::building::binsearch_index(df::global::world->buildings.all, id);
        if (index == -1) {
            //out.print("%s, line %d: Couldn't find new building with id %d.\n", __FILE__, __LINE__, a);
            //the tricky thing is that when the game first starts, it's ok to skip buildings, but otherwise, if you skip buildings, something is probably wrong. TODO: make this smarter
            continue;
        }
        createdBuildings.emplace(tick, id);
        // handlers which haven't handled this building yet aren't going to (it would be very tricky to make this work)
        destroyedBuildings.erase(id);
    }
    nextBuilding = *df::global::building_next_id;

    // todo: maybe we should create static lists, and compare sizes and only copy if they don't match, otherwise every manager function is copying their handlers every single they execute
    multimap<Plugin*, EventHandler> copy(handlers[EventType::BUILDING].begin(), handlers[EventType::BUILDING].end());
    // iterate event handler callbacks
    for (auto &key_value : copy) {
        auto &handler = key_value.second;
        auto last_tick = eventLastTick[handler];
        // enforce handler's callback frequency
        if (tick - last_tick >= handler.freq) {
            eventLastTick[handler] = tick;
            // send the handler all the destroyed buildings since it last fired
            for (auto jter = destroyedBuildings.upper_bound(last_tick); jter != destroyedBuildings.end(); ++jter) {
                handler.eventHandler(out, (void*) intptr_t(jter->second));
            }
            // send the handler all the new buildings since it last fired
            for (auto jter = createdBuildings.upper_bound(last_tick); jter != createdBuildings.end(); ++jter) {
                handler.eventHandler(out, (void*) intptr_t(jter->second));
            }
        }
    }
}

static void manageBuildingCreatedEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    if (!df::global::building_next_id)
        return;
    int32_t tick = df::global::world->frame_counter;
    // update created building list
    for (int32_t id = nextBuilding; id < *df::global::building_next_id; ++id) {
        int32_t index = df::building::binsearch_index(df::global::world->buildings.all, id);
        if (index == -1) {
            //out.print("%s, line %d: Couldn't find new building with id %d.\n", __FILE__, __LINE__, a);
            //the tricky thing is that when the game first starts, it's ok to skip buildings, but otherwise, if you skip buildings, something is probably wrong. TODO: make this smarter
            continue;
        }
        createdBuildings.emplace(tick, id);
        // handlers which haven't handled this building yet aren't going to (it would be very tricky to make this work)
        destroyedBuildings.erase(id);
    }
    nextBuilding = *df::global::building_next_id;
    multimap<Plugin*, EventHandler> copy(handlers[EventType::BUILDING_CREATED].begin(), handlers[EventType::BUILDING_CREATED].end());
    // iterate event handler callbacks
    for (auto &key_value : copy) {
        auto &handler = key_value.second;
        auto last_tick = eventLastTick[handler];
        // enforce handler's callback frequency
        if (tick - last_tick >= handler.freq) {
            eventLastTick[handler] = tick;
            // send the handler all the new & destroyed buildings since it last fired
            auto jter = createdBuildings.upper_bound(last_tick);
            for (; jter != createdBuildings.end(); ++jter) {
                handler.eventHandler(out, (void*) intptr_t(jter->second));
            }
        }
    }
}

static void manageBuildingDestroyedEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    int32_t tick = df::global::world->frame_counter;
    // update destroyed building list
    for (auto iter = createdBuildings.begin(); iter != createdBuildings.end();) {
        int32_t id = iter->second;
        int32_t index = df::building::binsearch_index(df::global::world->buildings.all, id);
        // continue if we found the id in world->buildings.all
        if (index != -1) {
            ++iter;
            continue;
        }
        // pretty sure we'd invalidate our loop if we added to buildings here, so we just save the id in an intermediary for now
        destroyedBuildings.emplace(tick, id);
        // handlers which haven't handled this building yet aren't going to (it would be very tricky to make this work)
        iter = createdBuildings.erase(id);
    }
    multimap<Plugin*, EventHandler> copy(handlers[EventType::BUILDING_DESTROYED].begin(), handlers[EventType::BUILDING_DESTROYED].end());
    // iterate event handler callbacks
    for (auto &key_value : copy) {
        auto &handler = key_value.second;
        auto last_tick = eventLastTick[handler];
        // enforce handler's callback frequency
        if (tick - last_tick >= handler.freq) {
            eventLastTick[handler] = tick;
            // send the handler all the new & destroyed buildings since it last fired
            auto jter = destroyedBuildings.upper_bound(last_tick);
            for (; jter != destroyedBuildings.end(); ++jter) {
                handler.eventHandler(out, (void*) intptr_t(jter->second));
            }
        }
    }
}

static void manageConstructionEvent(color_ostream& out) {
    if (!df::global::world)
        return;

    int32_t tick = df::global::world->frame_counter;
    for (auto &c : df::global::world->constructions) {
        // anything on the global list, obviously exists.. so we ensure that coord is on the created list and isn't on the destroyed list
        createdConstructions.emplace(tick, *c); // hashes based on c->pos (coord)
        // handlers which haven't handled this construction yet aren't going to (it would be very tricky to make this work)
        destroyedConstructions.erase(*c);
    }
    for (auto iter = createdConstructions.begin(); iter != createdConstructions.end();) {
        // if we can't find it, it was removed
        if (!df::construction::find(iter->second.pos)) {
            destroyedConstructions.emplace(tick, iter->second);
            // handlers which haven't handled this construction yet aren't going to (it would be very tricky to make this work)
            iter = createdConstructions.erase(iter);
            continue;
        }
        ++iter;
    }

    // iterate event handler callbacks
    multimap<Plugin*,EventHandler> copy(handlers[EventType::CONSTRUCTION].begin(), handlers[EventType::CONSTRUCTION].end());
    for (auto &key_value : copy) {
        auto &handler = key_value.second;
        auto last_tick = eventLastTick[handler];
        // enforce handler's callback frequency
        if (tick - last_tick >= handler.freq) {
            eventLastTick[handler] = tick;
            // send the handler all the removed constructions since it last fired
            for(auto iter = destroyedConstructions.upper_bound(last_tick); iter != destroyedConstructions.end(); ++iter) {
                handler.eventHandler(out, &iter->second);
            }
            // send the handler all the added constructions since it last fired
            for(auto iter = createdConstructions.upper_bound(last_tick); iter != createdConstructions.end(); ++iter) {
                handler.eventHandler(out, &iter->second);
            }
        }
    }
}

static void manageConstructionAddedEvent(color_ostream& out) {
    if (!df::global::world)
        return;

    int32_t tick = df::global::world->frame_counter;
    // update created construction list
    for (auto &c : df::global::world->constructions) {
        // anything on the global list, obviously exists.. so we ensure that coord is on the created list and isn't on the destroyed list
        createdConstructions.emplace(tick, *c); // hashes based on c->pos (coord)
        // handlers which haven't handled this construction yet aren't going to (it would be very tricky to make this work)
        destroyedConstructions.erase(*c);
    }
    // iterate event handler callbacks
    multimap<Plugin*,EventHandler> copy(handlers[EventType::CONSTRUCTION_ADDED].begin(), handlers[EventType::CONSTRUCTION_ADDED].end());
    for (auto &key_value : copy) {
        auto &handler = key_value.second;
        auto last_tick = eventLastTick[handler];
        // enforce handler's callback frequency
        if (tick - last_tick >= handler.freq) {
            eventLastTick[handler] = tick;
            // send the handler all the added constructions since it last fired
            for(auto iter = createdConstructions.upper_bound(last_tick); iter != createdConstructions.end(); ++iter) {
                handler.eventHandler(out, &iter->second);
            }
        }
    }
}

static void manageConstructionRemovedEvent(color_ostream& out) {
    if (!df::global::world)
        return;

    int32_t tick = df::global::world->frame_counter;
    // update destroyed constructions list
    for (auto iter = createdConstructions.begin(); iter != createdConstructions.end();) {
        // if we can't find it, it was removed
        if (!df::construction::find(iter->second.pos)) {
            destroyedConstructions.emplace(tick, iter->second);
            // handlers which haven't handled this construction yet aren't going to (it would be very tricky to make this work)
            iter = createdConstructions.erase(iter);
            continue;
        }
        ++iter;
    }
    // iterate event handler callbacks
    multimap<Plugin*,EventHandler> copy(handlers[EventType::CONSTRUCTION_REMOVED].begin(), handlers[EventType::CONSTRUCTION_REMOVED].end());
    for (auto &key_value : copy) {
        auto &handler = key_value.second;
        auto last_tick = eventLastTick[handler];
        // enforce handler's callback frequency
        if (tick - last_tick >= handler.freq) {
            eventLastTick[handler] = tick;
            // send the handler all the removed constructions since it last fired
            for(auto iter = destroyedConstructions.upper_bound(last_tick); iter != destroyedConstructions.end(); ++iter) {
                handler.eventHandler(out, &iter->second);
            }
        }
    }
}

static void manageSyndromeEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    int32_t tick = df::global::world->frame_counter;

    int32_t current_time = getTime();
    // update syndromes list
    for ( df::unit *unit : df::global::world->units.all ) {
        for (size_t idx = 0; idx < unit->syndromes.active.size(); ++idx) {
            auto &syndrome = unit->syndromes.active[idx];
            int32_t startTime = syndrome->year * ticksPerYear + syndrome->year_time;
            // add the syndrome if it started now or in the past
            if (startTime <= current_time) {
                syndromes.emplace(tick, SyndromeData(unit->id, (int32_t)idx));
            }
        }
    }

    // iterate event handler callbacks
    multimap<Plugin*,EventHandler> copy(handlers[EventType::SYNDROME].begin(), handlers[EventType::SYNDROME].end());
    for (auto &key_value : copy) {
        auto &handler = key_value.second;
        auto last_tick = eventLastTick[handler];
        // enforce handler's callback frequency
        if (tick - last_tick >= handler.freq) {
            eventLastTick[handler] = tick;
            // send all new syndromes since it last fired
            for(auto iter = syndromes.upper_bound(last_tick); iter != syndromes.end(); ++iter){
                handler.eventHandler(out, &iter->second);
            }
        }
    }
}

static void manageInvasionEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    if (!df::global::ui)
        return;

    int32_t tick = df::global::world->frame_counter;
    invasions.emplace(tick, nextInvasion);
    nextInvasion = df::global::ui->invasions.next_id;

    // iterate event handler callbacks
    multimap<Plugin*,EventHandler> copy(handlers[EventType::INVASION].begin(), handlers[EventType::INVASION].end());
    for (auto &key_value : copy) {
        auto &handler = key_value.second;
        auto last_tick = eventLastTick[handler];
        // enforce handler's callback frequency
        if (tick - last_tick >= handler.freq) {
            eventLastTick[handler] = tick;
            // send all new invasions since it last fired
            for(auto iter = invasions.upper_bound(last_tick); iter != invasions.end(); ++iter){
                handler.eventHandler(out, (void*)(intptr_t)iter->second);
            }
        }
    }
}

static void manageInventoryChangeEvent(color_ostream& out) {
    if (!df::global::world)
        return;

    int32_t tick = df::global::world->frame_counter;
    for (auto unit : df::global::world->units.all) {
        auto unitId = unit->id;
        unordered_map<int32_t, InventoryItem> &last_tick_inventory = inventoryLog[unitId];
        vector<df::unit_inventory_item*> &current_tick_inventory = unit->inventory;
        unordered_set<int32_t> currentlyEquipped;

        // iterate current tick's inventory for unit (std::vector)
        for (auto ct_item : current_tick_inventory) {
            auto itemId = ct_item->item->id;
            currentlyEquipped.emplace(itemId);

            // detect new
            auto lti_iter = last_tick_inventory.find(itemId);
            if (lti_iter == last_tick_inventory.end()) {
                // add to list because the item didn't exist before
                equipmentChanges.emplace(tick, InventoryChangeData(unitId, nullptr, new InventoryItem(itemId, *ct_item)));
                continue;
            }

            // detect change - (in how item is equipped)
            InventoryItem item_old = lti_iter->second;
            df::unit_inventory_item &item0 = item_old.item;
            df::unit_inventory_item &item1 = *ct_item;
            if (item0.mode == item1.mode && item0.body_part_id == item1.body_part_id &&
                item0.wound_id == item1.wound_id)
                continue;
            // add to list because item is not equipped in the same way
            equipmentChanges.emplace(tick, InventoryChangeData(unit->id, new InventoryItem(item_old.itemId, item_old.item), new InventoryItem(itemId, *ct_item)));
        }
        //check for dropped items
        for (auto &key_value : last_tick_inventory) {
            auto &item = key_value.second;
            // add to list if unit doesn't have the item
            if (currentlyEquipped.find(item.itemId) == currentlyEquipped.end()) {
                equipmentChanges.emplace(tick, InventoryChangeData(unit->id, new InventoryItem(item.itemId, item.item), nullptr));
            }
        }

        //update equipment
        last_tick_inventory.clear();
        for (auto ct_item : current_tick_inventory) {
            auto itemId = ct_item->item->id;
            last_tick_inventory.emplace(itemId, InventoryItem(itemId, *ct_item));
        }
    }
    // iterate event handler callbacks
    multimap<Plugin*,EventHandler> copy(handlers[EventType::INVENTORY_CHANGE].begin(), handlers[EventType::INVENTORY_CHANGE].end());
    for (auto &key_value : copy) {
        auto &handler = key_value.second;
        auto last_tick = eventLastTick[handler];
        // ensure handler's callback frequency
        if (tick - last_tick >= handler.freq) {
            eventLastTick[handler] = tick;
            // send all new equipment changes since it last fired
            for(auto iter = equipmentChanges.upper_bound(last_tick); iter != equipmentChanges.end(); ++iter){
                handler.eventHandler(out, &iter->second);
            }
        }
    }
}

static void updateReportToRelevantUnits();

static void manageReportEvent(color_ostream& out) {
    if (!df::global::world)
        return;

    int32_t tick = df::global::world->frame_counter;
    std::unordered_set<int32_t> valid_reports;
    // update reports list
    for(auto &r : df::global::world->status.reports){
        valid_reports.emplace(r->id);
        newReports.emplace(tick, r->id);
    }

    // iterate event handler callbacks
    multimap<Plugin*,EventHandler> copy(handlers[EventType::REPORT].begin(), handlers[EventType::REPORT].end());
    for (auto &key_value : copy) {
        auto &handler = key_value.second;
        auto last_tick = eventLastTick[handler];
        // enforce handler's callback frequency
        if (tick - last_tick >= handler.freq) {
            eventLastTick[handler] = tick;
            // send all new reports since it last fired
            auto iter = newReports.upper_bound(last_tick);
            for(;iter != newReports.end();) {
                if (valid_reports.count(iter->second)) {
                    handler.eventHandler(out, (void*) intptr_t(iter->second));
                    ++iter;
                } else {
                    iter = newReports.erase(iter->second);
                }
            }
        }
    }
}

static df::unit_wound* getWound(df::unit* attacker, df::unit* defender) {
    for (auto wound : defender->body.wounds) {
        if ( wound->age <= 1 && wound->attacker_unit_id == attacker->id ) {
            return wound;
        }
    }
    return nullptr;
}

static void manageUnitAttackEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    int32_t tick = df::global::world->frame_counter;
    std::vector<df::report*>& reports = df::global::world->status.reports;
    size_t idx = df::report::binsearch_index(reports, lastReportUnitAttack, false);
    // returns the index to the key equal to or greater than the key provided
    idx = reports[idx]->id == lastReportUnitAttack ? idx + 1 : idx; // we need the index after (where the new stuff is)

    std::set<int32_t> strikeReports;
    for ( ; idx < reports.size(); ++idx ) {
        df::report* report = reports[idx];
        lastReportUnitAttack = report->id;
        if ( report->flags.bits.continuation ) {
            continue;
        }
        df::announcement_type type = report->type;
        if ( type == df::announcement_type::COMBAT_STRIKE_DETAILS ) {
            strikeReports.insert(report->id);
        }
    }
    updateReportToRelevantUnits();
    map<int32_t, map<int32_t, int32_t> > alreadyDone;
    for (int reportId : strikeReports) {
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
            reportStr += report2->text;
        }

        std::vector<int32_t>& relevantUnits = reportToRelevantUnits[report->id];
        if ( relevantUnits.size() != 2 ) {
            continue;
        }

        df::unit* unit1 = df::unit::find(relevantUnits[0]);
        df::unit* unit2 = df::unit::find(relevantUnits[1]);

        df::unit_wound* wound1 = getWound(unit1,unit2);
        df::unit_wound* wound2 = getWound(unit2,unit1);

        UnitAttackData data{};
        data.report_id = report->id;
        if ( wound1 && !alreadyDone[unit1->id][unit2->id] ) {
            data.attacker = unit1->id;
            data.defender = unit2->id;
            data.wound = wound1->id;

            alreadyDone[data.attacker][data.defender] = 1;
            attackEvents.emplace(tick, data);
        }

        if ( wound2 && !alreadyDone[unit1->id][unit2->id] ) {
            data.attacker = unit2->id;
            data.defender = unit1->id;
            data.wound = wound2->id;

            alreadyDone[data.attacker][data.defender] = 1;
            attackEvents.emplace(tick, data);
        }

        if ( Units::isKilled(unit1) ) {
            data.attacker = unit2->id;
            data.defender = unit1->id;
            data.wound = -1;

            alreadyDone[data.attacker][data.defender] = 1;
            attackEvents.emplace(tick, data);
        }

        if ( Units::isKilled(unit2) ) {
            data.attacker = unit1->id;
            data.defender = unit2->id;
            data.wound = -1;

            alreadyDone[data.attacker][data.defender] = 1;
            attackEvents.emplace(tick, data);
        }

        if ( !wound1 && !wound2 ) {
            //if ( unit1->flags1.bits.inactive || unit2->flags1.bits.inactive )
            //    continue;
            if ( reportStr.find("severed part") )
                continue;
            if ( Once::doOnce("EventManager neither wound") ) {
                out.print("%s, %d: neither wound: %s\n", __FILE__, __LINE__, reportStr.c_str());
            }
        }
    }

    multimap<Plugin*,EventHandler> copy(handlers[EventType::UNIT_ATTACK].begin(), handlers[EventType::UNIT_ATTACK].end());
    for (auto &key_value : copy) {
        auto &handler = key_value.second;
        auto last_tick = eventLastTick[handler];
        if (tick - last_tick >= handler.freq) {
            eventLastTick[handler] = tick;
            for(auto iter = attackEvents.upper_bound(last_tick); iter != attackEvents.end(); ++iter){
                handler.eventHandler(out, &iter->second);
            }
        }
    }
}

static std::string getVerb(df::unit* unit, const std::string &reportStr) {
    std::string result(reportStr);
    std::string name = unit->name.first_name + " ";
    bool match = strncmp(result.c_str(), name.c_str(), name.length()) == 0;
    if ( match ) {
        result = result.substr(name.length());
        result = result.substr(0,result.length()-1);
        return result;
    }
    //use profession name
    name = "The " + Units::getProfessionName(unit) + " ";
    match = strncmp(result.c_str(), name.c_str(), name.length()) == 0;
    if ( match ) {
        result = result.substr(name.length());
        result = result.substr(0,result.length()-1);
        return result;
    }

    if ( unit->id != 0 ) {
        return "";
    }

    std::string you = "You ";
    match = strncmp(result.c_str(), name.c_str(), name.length()) == 0;
    if ( match ) {
        result = result.substr(name.length());
        result = result.substr(0,result.length()-1);
        return result;
    }
    return "";
}

static InteractionData getAttacker(color_ostream& out, df::report* attackEvent, df::unit* lastAttacker, df::report* defendEvent, vector<df::unit*>& relevantUnits) {
    vector<df::unit*> attackers = relevantUnits;
    vector<df::unit*> defenders = relevantUnits;

    //find valid interactions: TODO
    /*map<int32_t,vector<df::interaction*> > validInteractions;
    for ( size_t a = 0; a < relevantUnits.size(); a++ ) {
        df::unit* unit = relevantUnits[a];
        vector<df::interaction*>& interactions = validInteractions[unit->id];
        for ( size_t b = 0; b < unit->body.
    }*/

    //if attackEvent
    //  attacker must be same location
    //  attacker name must start attack str
    //  attack verb must match valid interaction of this attacker
    std::string attackVerb;
    if ( attackEvent ) {
//out.print("%s,%d\n",__FILE__,__LINE__);
        for ( size_t a = 0; a < attackers.size(); a++ ) {
            if ( attackers[a]->pos != attackEvent->pos ) {
                attackers.erase(attackers.begin()+a);
                a--;
                continue;
            }
            if ( lastAttacker && attackers[a] != lastAttacker ) {
                attackers.erase(attackers.begin()+a);
                a--;
                continue;
            }

            std::string verbC = getVerb(attackers[a], attackEvent->text);
            if ( verbC.length() == 0 ) {
                attackers.erase(attackers.begin()+a);
                a--;
                continue;
            }
            attackVerb = verbC;
        }
    }

    //if defendEvent
    //  defender must be same location
    //  defender name must start defend str
    //  defend verb must match valid interaction of some attacker
    std::string defendVerb;
    if ( defendEvent ) {
//out.print("%s,%d\n",__FILE__,__LINE__);
        for ( size_t a = 0; a < defenders.size(); a++ ) {
            if ( defenders[a]->pos != defendEvent->pos ) {
                defenders.erase(defenders.begin()+a);
                a--;
                continue;
            }
            std::string verbC = getVerb(defenders[a], defendEvent->text);
            if ( verbC.length() == 0 ) {
                defenders.erase(defenders.begin()+a);
                a--;
                continue;
            }
            defendVerb = verbC;
        }
    }

    //keep in mind one attacker zero defenders is perfectly valid for self-cast
    if ( attackers.size() == 1 && defenders.size() == 1 && attackers[0] == defenders[0] ) {
//out.print("%s,%d\n",__FILE__,__LINE__);
    } else {
        if ( defenders.size() == 1 ) {
//out.print("%s,%d\n",__FILE__,__LINE__);
            auto a = std::find(attackers.begin(),attackers.end(),defenders[0]);
            if ( a != attackers.end() )
                attackers.erase(a);
        }
        if ( attackers.size() == 1 ) {
//out.print("%s,%d\n",__FILE__,__LINE__);
            auto a = std::find(defenders.begin(),defenders.end(),attackers[0]);
            if ( a != defenders.end() )
                defenders.erase(a);
        }
    }

    //if trying attack-defend pair and it fails to find attacker, try defend only
    InteractionData result = /*(InteractionData)*/ { std::string(), std::string(), -1, -1, -1, -1 };
    if ( attackers.size() > 1 ) {
//out.print("%s,%d\n",__FILE__,__LINE__);
        if ( Once::doOnce("EventManager interaction ambiguous attacker") ) {
            out.print("%s,%d: ambiguous attacker on report\n \'%s\'\n '%s'\n", __FILE__, __LINE__, attackEvent ? attackEvent->text.c_str() : "", defendEvent ? defendEvent->text.c_str() : "");
        }
    } else if ( attackers.empty() ) {
//out.print("%s,%d\n",__FILE__,__LINE__);
        if ( attackEvent && defendEvent )
            return getAttacker(out, nullptr, nullptr, defendEvent, relevantUnits);
    } else {
//out.print("%s,%d\n",__FILE__,__LINE__);
        //attackers.size() == 1
        result.attacker = attackers[0]->id;
        if ( !defenders.empty() )
            result.defender = defenders[0]->id;
        if ( defenders.size() > 1 ) {
            if ( Once::doOnce("EventManager interaction ambiguous defender") ) {
                out.print("%s,%d: ambiguous defender: shouldn't happen. On report\n \'%s\'\n '%s'\n", __FILE__, __LINE__, attackEvent ? attackEvent->text.c_str() : "", defendEvent ? defendEvent->text.c_str() : "");
            }
        }
        result.attackVerb = attackVerb;
        result.defendVerb = defendVerb;
        if ( attackEvent )
            result.attackReport = attackEvent->id;
        if ( defendEvent )
            result.defendReport = defendEvent->id;
    }
//out.print("%s,%d\n",__FILE__,__LINE__);
    return result;
}

static vector<df::unit*> gatherRelevantUnits(color_ostream& out, df::report* r1, df::report* r2) {
    vector<df::report*> reports;
    if ( r1 == r2 ) r2 = nullptr;
    if ( r1 ) reports.push_back(r1);
    if ( r2 ) reports.push_back(r2);
    vector<df::unit*> result;
    unordered_set<int32_t> ids;
//out.print("%s,%d\n",__FILE__,__LINE__);
    for (auto report : reports) {
//out.print("%s,%d\n",__FILE__,__LINE__);
        vector<int32_t>& units = reportToRelevantUnits[report->id];
        if ( units.size() > 2 ) {
            if ( Once::doOnce("EventManager interaction too many relevant units") ) {
                out.print("%s,%d: too many relevant units. On report\n \'%s\'\n", __FILE__, __LINE__, report->text.c_str());
            }
        }
        for (int & unit_id : units)
            if (ids.find(unit_id) == ids.end() ) {
                ids.insert(unit_id);
                result.push_back(df::unit::find(unit_id));
            }
    }
//out.print("%s,%d\n",__FILE__,__LINE__);
    return result;
}

static void updateReportToRelevantUnits() {
    if (!df::global::world)
        return;
    if ( df::global::world->frame_counter <= reportToRelevantUnitsTime )
        return;
    reportToRelevantUnitsTime = df::global::world->frame_counter;

    for (auto unit : df::global::world->units.all) {
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

static void manageInteractionEvent(color_ostream& out) {
    if (!df::global::world)
        return;

    int32_t tick = df::global::world->frame_counter;
    std::vector<df::report*>& reports = df::global::world->status.reports;
    size_t idx = df::report::binsearch_index(reports, lastReportInteraction, false);
    // returns the index to the key equal to or greater than the key provided
    idx = reports[idx]->id == lastReportInteraction ? idx + 1 : idx; // we need the index after (where the new stuff is)

    if (idx < reports.size() )
        updateReportToRelevantUnits();

    df::report* lastAttackEvent = nullptr;
    df::unit* lastAttacker = nullptr;
    //df::unit* lastDefender = NULL;
    unordered_map<int32_t,unordered_set<int32_t> > history;
    for ( ; idx < reports.size(); idx++ ) {
        df::report* report = reports[idx];
        lastReportInteraction = report->id;
        df::announcement_type type = report->type;
        if ( type != df::announcement_type::INTERACTION_ACTOR && type != df::announcement_type::INTERACTION_TARGET )
            continue;
        if ( report->flags.bits.continuation )
            continue;
        bool attack = type == df::announcement_type::INTERACTION_ACTOR;
        if ( attack ) {
            lastAttackEvent = report;
            lastAttacker = nullptr;
            //lastDefender = NULL;
        }
        vector<df::unit*> relevantUnits = gatherRelevantUnits(out, lastAttackEvent, report);
        InteractionData data = getAttacker(out, lastAttackEvent, lastAttacker, attack ? nullptr : report, relevantUnits);
        if ( data.attacker < 0 )
            continue;
//out.print("%s,%d\n",__FILE__,__LINE__);
        //if ( !attack && lastAttacker && data.attacker == lastAttacker->id && lastDefender && data.defender == lastDefender->id )
        //    continue; //lazy way of preventing duplicates
        if (attack && idx + 1 < reports.size() && reports[idx + 1]->type == df::announcement_type::INTERACTION_TARGET ) {
//out.print("%s,%d\n",__FILE__,__LINE__);
            vector<df::unit*> relevant_units = gatherRelevantUnits(out, lastAttackEvent, reports[idx + 1]);
            InteractionData data2 = getAttacker(out, lastAttackEvent, lastAttacker, reports[idx + 1], relevant_units);
            if ( data.attacker == data2.attacker && (data.defender == -1 || data.defender == data2.defender) ) {
//out.print("%s,%d\n",__FILE__,__LINE__);
                data = data2;
                idx++;
            }
        }
        {
#define HISTORY_ITEM 1
#if HISTORY_ITEM
            unordered_set<int32_t>& b = history[data.attacker];
            if ( b.find(data.defender) != b.end() )
                continue;
            history[data.attacker].insert(data.defender);
            //b.insert(data.defender);
#else
            unordered_set<int32_t>& b = history[data.attackReport];
            if ( b.find(data.defendReport) != b.end() )
                continue;
            history[data.attackReport].insert(data.defendReport);
            //b.insert(data.defendReport);
#endif
        }
//out.print("%s,%d\n",__FILE__,__LINE__);
        lastAttacker = df::unit::find(data.attacker);
        //lastDefender = df::unit::find(data.defender);
        //fire event
        interactionEvents.emplace(tick, data);
        //TODO: deduce attacker from latest defend event first
    }

    multimap<Plugin*,EventHandler> copy(handlers[EventType::INTERACTION].begin(), handlers[EventType::INTERACTION].end());
    for (auto &key_value : copy) {
        auto &handler = key_value.second;
        auto last_tick = eventLastTick[handler];
        if (tick - last_tick >= handler.freq) {
            eventLastTick[handler] = tick;
            for(auto iter = interactionEvents.upper_bound(last_tick); iter != interactionEvents.end(); ++iter){
                handler.eventHandler(out, &iter->second);
            }
        }
    }
}
