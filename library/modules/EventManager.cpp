#include "Core.h"
#include "Console.h"
#include "Debug.h"
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
#include "df/plotinfost.h"
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
#include <utility>

namespace DFHack {
    DBG_DECLARE(eventmanager, log, DebugCategory::LINFO);
}

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

void DFHack::EventManager::registerListener(EventType::EventType e, EventHandler handler) {
    DEBUG(log).print("registering handler %p from plugin %s for event %d\n", handler.eventHandler, !handler.plugin ? "<null>" : handler.plugin->getName().c_str(), e);
    handlers[e].insert(pair<Plugin*, EventHandler>(handler.plugin, handler));
}

int32_t DFHack::EventManager::registerTick(EventHandler handler, int32_t when, bool absolute) {
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
    DEBUG(log).print("registering handler %p from plugin %s for event TICK\n", handler.eventHandler, !handler.plugin ? "<null>" : handler.plugin->getName().c_str());
    handlers[EventType::TICK].insert(pair<Plugin*,EventHandler>(handler.plugin,handler));
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

void DFHack::EventManager::unregister(EventType::EventType e, EventHandler handler) {
    for ( auto i = handlers[e].find(handler.plugin); i != handlers[e].end(); ) {
        if ( (*i).first != handler.plugin )
            break;
        EventHandler &handle = (*i).second;
        if ( handle != handler ) {
            i++;
            continue;
        }
        DEBUG(log).print("unregistering handler %p from plugin %s for event %d\n", handler.eventHandler, !handler.plugin ? "<null>" : handler.plugin->getName().c_str(), e);
        i = handlers[e].erase(i);
        if ( e == EventType::TICK )
            removeFromTickQueue(handler);
    }
}

void DFHack::EventManager::unregisterAll(Plugin* plugin) {
    DEBUG(log).print("unregistering all handlers for plugin %s\n", !plugin ? "<null>" : plugin->getName().c_str());
    for ( auto i = handlers[EventType::TICK].find(plugin); i != handlers[EventType::TICK].end(); i++ ) {
        if ( (*i).first != plugin )
            break;

        removeFromTickQueue((*i).second);
    }
    for (auto &handler : handlers) {
        handler.erase(plugin);
    }
}

static void manageTickEvent(color_ostream& out);
static void manageJobInitiatedEvent(color_ostream& out);
static void manageJobStartedEvent(color_ostream& out);
static void manageJobCompletedEvent(color_ostream& out);
static void manageNewUnitActiveEvent(color_ostream& out);
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
            return manageNewUnitActiveEvent;
        case EventType::UNIT_DEATH:
            return manageUnitDeathEvent;
        case EventType::ITEM_CREATED:
            return manageItemCreationEvent;
        case EventType::BUILDING:
            return manageBuildingEvent;
        case EventType::CONSTRUCTION:
            return manageConstructionEvent;
        case EventType::SYNDROME:
            return manageSyndromeEvent;
        case EventType::INVASION:
            return manageInvasionEvent;
        case EventType::INVENTORY_CHANGE:
            return manageEquipmentEvent;
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

//job initiated
static int32_t lastJobId = -1;

//job started
static unordered_set<int32_t> startedJobs;

//job completed
struct JobCompleteData {
    int32_t id;
    int32_t completion_timer;
    uint32_t flags_bits_repeat;
};


static std::unordered_map<int32_t, Job::JobUniquePtr> seenJobs;
static std::vector<JobCompleteData> prevJobs;

//active units
static unordered_set<int32_t> activeUnits;

//unit death
static unordered_set<int32_t> livingUnits;

//item creation
static int32_t nextItem;

//building
static int32_t nextBuilding;
static unordered_set<int32_t> buildings;

namespace std {
    template <>
    struct hash<df::construction> {
        std::size_t operator()(const df::construction& construct) const {
            return construct();
        }
    };
}

//construction
static unordered_set<df::construction> constructions;
static bool gameLoaded;

//syndrome
static int32_t lastSyndromeTime;

//invasion
static int32_t nextInvasion;

//equipment change
//static unordered_map<int32_t, vector<df::unit_inventory_item> > equipmentLog;
static unordered_map<int32_t, vector<InventoryItem>> equipmentLog;

//report
static int32_t lastReport;

//unit attack
static int32_t lastReportUnitAttack;
static std::map<int32_t,std::vector<int32_t>> reportToRelevantUnits;
static int32_t reportToRelevantUnitsTime = -1;

//interaction
static int32_t lastReportInteraction;

struct hash_pair {
    template<typename A, typename B>
    size_t operator()(const std::pair<A,B>& p) const {
        auto h1 = std::hash<A>{}(p.first);
        auto h2 = std::hash<B>{}(p.second);
        return h1 ^ (h2 << 1);
    }
};

static void run_handler(color_ostream& out, EventType::EventType eventType, const EventHandler & handle, void * arg) {
    auto &core = Core::getInstance();
    auto &counters = core.perf_counters;
    uint32_t start_ms = core.p->getTickCount();
    const char * plugin_name = !handle.plugin ? "<null>" : handle.plugin->getName().c_str();
    handle.eventHandler(out, arg);
    counters.incCounter(counters.event_manager_event_per_plugin_ms[eventType][plugin_name], start_ms);
}

void DFHack::EventManager::onStateChange(color_ostream& out, state_change_event event) {
    static bool doOnce = false;
//    const string eventNames[] = {"world loaded", "world unloaded", "map loaded", "map unloaded", "viewscreen changed", "core initialized", "begin unload", "paused", "unpaused"};
//    out.print("%s,%d: onStateChange %d: \"%s\"\n", __FILE__, __LINE__, (int32_t)event, eventNames[event].c_str());
    if ( !doOnce ) {
        //TODO: put this somewhere else
        doOnce = true;
        EventHandler buildingHandler(nullptr, Buildings::updateBuildings, 100);
        DFHack::EventManager::registerListener(EventType::BUILDING, buildingHandler);
        //out.print("Registered listeners.\n %d", __LINE__);
    }
    if ( event == DFHack::SC_MAP_UNLOADED ) {
        lastJobId = -1;
        startedJobs.clear();
        seenJobs.clear();
        prevJobs.clear();
        tickQueue.clear();
        livingUnits.clear();
        buildings.clear();
        constructions.clear();
        equipmentLog.clear();
        activeUnits.clear();

        Buildings::clearBuildings(out);
        lastReport = -1;
        lastReportUnitAttack = -1;
        gameLoaded = false;

        multimap<Plugin*,EventHandler> copy(handlers[EventType::UNLOAD].begin(), handlers[EventType::UNLOAD].end());
        for (auto &[_,handle] : copy) {
            DEBUG(log,out).print("calling handler for map unloaded state change event\n");
            run_handler(out, EventType::UNLOAD, handle, nullptr);
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
        if (!df::global::plotinfo)
            return;
        if (!df::global::world)
            return;

        nextItem = *df::global::item_next_id;
        nextBuilding = *df::global::building_next_id;
        nextInvasion = df::global::plotinfo->invasions.next_id;
        lastJobId = -1 + *df::global::job_next_id;

        constructions.clear();
        for (auto c : df::global::world->event.constructions) {
            if ( !c ) {
                if ( Once::doOnce("EventManager.onLoad null constr") ) {
                    out.print("EventManager.onLoad: null construction.\n");
                }
                continue;
            }
            if (c->pos == df::coord() ) {
                if ( Once::doOnce("EventManager.onLoad null position of construction.\n") )
                    out.print("EventManager.onLoad null position of construction.\n");
                continue;
            }
            constructions.emplace(*c);
        }
        for (auto b : df::global::world->buildings.all) {
            Buildings::updateBuildings(out, (void*)intptr_t(b->id));
            buildings.insert(b->id);
        }
        lastSyndromeTime = -1;
        for (auto unit : df::global::world->units.all) {
            if (Units::isActive(unit)) {
                activeUnits.emplace(unit->id);
            }
            for (auto syndrome : unit->syndromes.active) {
                int32_t startTime = syndrome->year*ticksPerYear + syndrome->year_time;
                if ( startTime > lastSyndromeTime )
                    lastSyndromeTime = startTime;
            }
        }
        lastReport = -1;
        if ( !df::global::world->status.reports.empty() ) {
            lastReport = df::global::world->status.reports[df::global::world->status.reports.size()-1]->id;
        }
        lastReportUnitAttack = -1;
        lastReportInteraction = -1;
        reportToRelevantUnitsTime = -1;
        reportToRelevantUnits.clear();
        for (int &last_tick : eventLastTick) {
            last_tick = -1;//-1000000;
        }
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

    int32_t tick = df::global::world->frame_counter;
    TRACE(log,out).print("processing events at tick %d\n", tick);

    auto &core = Core::getInstance();
    auto &counters = core.perf_counters;
    for ( size_t a = 0; a < EventType::EVENT_MAX; a++ ) {
        if ( handlers[a].empty() )
            continue;
        int32_t eventFrequency = -100;
        if ( a != EventType::TICK )
            for (auto &[_,handle] : handlers[a]) {
                if (handle.freq < eventFrequency || eventFrequency == -100 )
                    eventFrequency = handle.freq;
            }
        else eventFrequency = 1;

        if ( tick >= eventLastTick[a] && tick - eventLastTick[a] < eventFrequency )
            continue;

        uint32_t start_ms = core.p->getTickCount();
        eventManager[a](out);
        eventLastTick[a] = tick;
        counters.incCounter(counters.event_manager_event_total_ms[a], start_ms);
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
        EventHandler &handle = (*tickQueue.begin()).second;
        tickQueue.erase(tickQueue.begin());
        DEBUG(log,out).print("calling handler for tick event\n");
        run_handler(out, EventType::TICK, handle, (void*)intptr_t(tick));
        toRemove.insert(handle);
    }
    if ( toRemove.empty() )
        return;
    for ( auto a = handlers[EventType::TICK].begin(); a != handlers[EventType::TICK].end(); ) {
        EventHandler &handle = (*a).second;
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

    for ( df::job_list_link* link = &df::global::world->jobs.list; link != nullptr; link = link->next ) {
        if ( link->item == nullptr )
            continue;
        if ( link->item->id <= lastJobId )
            continue;
        for (auto &[_,handle] : copy) {
            DEBUG(log,out).print("calling handler for job initiated event\n");
            run_handler(out, EventType::JOB_INITIATED, handle, (void*)link->item);
        }
    }

    lastJobId = *df::global::job_next_id - 1;
}

static void manageJobStartedEvent(color_ostream& out) {
    if (!df::global::world)
        return;

    // iterate event handler callbacks
    multimap<Plugin*, EventHandler> copy(handlers[EventType::JOB_STARTED].begin(), handlers[EventType::JOB_STARTED].end());

    unordered_set<int32_t> newStartedJobs;
    newStartedJobs.reserve(startedJobs.size());

    for (df::job_list_link* link = &df::global::world->jobs.list; link != nullptr; link = link->next) {
        if (link->item != nullptr) {
            df::job& job = *link->item;
            // To have a worker, there must be at least one general_ref.
            if (job.general_refs.size() > 0 && Job::getWorker(&job)) {
                newStartedJobs.emplace(job.id);
                if (!startedJobs.count(job.id)) {
                    for (auto &[_,handle] : copy) {
                        DEBUG(log,out).print("calling handler for job started event\n");
                        run_handler(out, EventType::JOB_STARTED, handle, &job);
                    }
                }
            }
        }
    }
    startedJobs = std::move(newStartedJobs);
}

//helper function for manageJobCompletedEvent
#if 0
static int32_t getWorkerID(df::job* job) {
    auto ref = findRef(job->general_refs, general_ref_type::UNIT_WORKER);
    return ref ? ref->getID() : -1;
}
#endif

/*
TODO: consider checking item creation / experience gain just in case
*/
static void manageJobCompletedEvent(color_ostream& out) {
    if (!df::global::world)
        return;

    int32_t tick0 = eventLastTick[EventType::JOB_COMPLETED];
    int32_t tick1 = df::global::world->frame_counter;

    multimap<Plugin*, EventHandler> copy(handlers[EventType::JOB_COMPLETED].begin(), handlers[EventType::JOB_COMPLETED].end());
    std::vector<JobCompleteData> nowJobs;
    // predict the size in advance, this will prevent or reduce memory reallocation.
    nowJobs.reserve(prevJobs.size());
    for (const auto jobPtr : df::global::world->jobs.list) {
        auto& job = *jobPtr;

        auto seenIt = seenJobs.find(job.id);
        if (seenIt != seenJobs.end()) {
            // No reference here, to prevent dangling reference situation.
            auto seenJob = seenIt->second.get();

            //out.print("SEEN0: %d, %ld, %ld, %d, %ld, %d %s\n", seenJob.id, seenJob.items.size(), seenJob.job_items.elements.size(), seenJob.flags.bits.working, seenJob.general_refs.size(), getWorkerID(&seenJob), ENUM_ATTR(job_type, caption, seenJob.job_type));
            //out.print("SEEN1: %d, %ld, %ld, %d, %ld, %d %s\n", job.id, job.items.size(), job.job_items.elements.size(), job.flags.bits.working, job.general_refs.size(), getWorkerID(&job), ENUM_ATTR(job_type, caption, job.job_type));

            // The key here is to strategically check the most important bits to reduce churn.
            if (seenJob->flags.whole != job.flags.whole
                    || (seenJob->items.size() != job.items.size())
                    || (seenJob->general_refs.size() != job.general_refs.size())) {
                seenIt->second = Job::JobUniquePtr(Job::cloneJobStruct(&job, true));
            }
        } else {
            seenJobs.emplace(job.id, Job::JobUniquePtr(Job::cloneJobStruct(&job, true)));
        }
        nowJobs.emplace_back(job.id, job.completion_timer, (bool)job.flags.bits.repeat);
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

    // if it happened within a tick, must have been cancelled by the user or a plugin: not completed
    // note observation: delta 0 tick events happens normally on load, save, paused (whether by user or game)
    if (tick1 >= tick0) {
        auto prevIt = prevJobs.begin();
        auto nowIt = nowJobs.begin();
        /*
         * Iterate through two ordered sets, prevJobs and nowJobs, where job IDs in nowJobs are invariably
         * greater than or equal to job IDs in prevJobs. The algorithm maintains the invariant that for each
         * iteration nowIt is within valid range (not equal to nowJobs.end()), and prevIt->id is less than
         * or equal to nowIt->id. Entries in nowJobs that are not found in prevJobs have IDs greater
         * than any in prevJobs.
         */
        while (prevIt != prevJobs.end()) {
            if (nowIt == nowJobs.end() || prevIt->id != nowIt->id) { // job ID is in prevJobs. ID does not exist in nowJobs.
                auto& prevJob = *prevIt;
                df::job& seenJob = *seenJobs[prevJob.id];
                // recently finished or cancelled job
                if (!prevJob.flags_bits_repeat && prevJob.completion_timer == 0) {
                    for (auto& [_, handle] : copy) {
                        DEBUG(log, out).print("calling handler for job completed event\n");
                        run_handler(out, EventType::JOB_COMPLETED, handle, (void*)&seenJob);
                    }
                }
                seenJobs.erase(prevJob.id);
            } else { // prevIt job ID and nowIt job ID are equal.
                // could have just finished if it's a repeat job
                auto& prevJob = *prevIt;
                if (prevJob.flags_bits_repeat && prevJob.completion_timer == 0
                        && nowIt->completion_timer == -1) {
                    df::job& seenJob = *seenJobs[prevJob.id];
                    // still false positive if cancelled at EXACTLY the right time, but experiments show this doesn't happen
                    for (auto& [_, handle] : copy) {
                        DEBUG(log, out).print("calling handler for repeated job completed event\n");
                        run_handler(out, EventType::JOB_COMPLETED, handle, (void*)&seenJob);
                    }
                }
                // prevIt has caught up to nowIt.
                ++nowIt;
            }
            ++prevIt;
        }
    }

    /*
     * Clean up garbage, if any.
     * Jobs may be missed if tick delta > zero,
     * and possibly other circumstances.
     * To prevent leaking memory, we need to cleanup
     * these missed jobs.
     */
    if (seenJobs.size() > nowJobs.size() * 2) {
        std::unordered_map<int32_t, Job::JobUniquePtr> newMap;
        newMap.reserve(nowJobs.size());
        for (auto& data : nowJobs) {
            auto it = seenJobs.find(data.id);
            if (it != seenJobs.end()) {
                newMap.emplace(std::move(*it));
            }
        }
        seenJobs.swap(newMap);
    }

    prevJobs = std::move(nowJobs);
}

static void manageNewUnitActiveEvent(color_ostream& out) {
    if (!df::global::world)
        return;

    multimap<Plugin*,EventHandler> copy(handlers[EventType::UNIT_NEW_ACTIVE].begin(), handlers[EventType::UNIT_NEW_ACTIVE].end());
    // iterate event handler callbacks
    vector<int32_t> new_active_unit_ids;
    for (df::unit* unit : df::global::world->units.active) {
        if (!activeUnits.count(unit->id)) {
            activeUnits.emplace(unit->id);
            new_active_unit_ids.emplace_back(unit->id);
        }
    }
    for (int32_t unit_id : new_active_unit_ids) {
        for (auto &[_,handle] : copy) {
            DEBUG(log,out).print("calling handler for new unit event\n");
            run_handler(out, EventType::UNIT_NEW_ACTIVE, handle, (void*) intptr_t(unit_id)); // intptr_t() avoids cast from smaller type warning
        }
    }
}


static void manageUnitDeathEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    multimap<Plugin*,EventHandler> copy(handlers[EventType::UNIT_DEATH].begin(), handlers[EventType::UNIT_DEATH].end());
    vector<int32_t> dead_unit_ids;
    for (auto unit : df::global::world->units.all) {
        //if ( unit->counters.death_id == -1 ) {
        if ( Units::isActive(unit) ) {
            livingUnits.insert(unit->id);
            continue;
        }
        if (!Units::isDead(unit)) continue; // for units that have left the map but aren't dead
        //dead: if dead since last check, trigger events
        if ( livingUnits.find(unit->id) == livingUnits.end() )
            continue;
        livingUnits.erase(unit->id);
        dead_unit_ids.emplace_back(unit->id);
    }

    for (int32_t unit_id : dead_unit_ids) {
        for (auto &[_,handle] : copy) {
            DEBUG(log,out).print("calling handler for unit death event\n");
            run_handler(out, EventType::UNIT_DEATH, handle, (void*)intptr_t(unit_id));
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

    multimap<Plugin*,EventHandler> copy(handlers[EventType::ITEM_CREATED].begin(), handlers[EventType::ITEM_CREATED].end());
    size_t index = df::item::binsearch_index(df::global::world->items.all, nextItem, false);
    if ( index != 0 ) index--;

    std::vector<int32_t> created_items;
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
        created_items.push_back(item->id);
    }

    // handle all created items
    for (int32_t item_id : created_items) {
        for (auto &[_,handle] : copy) {
            DEBUG(log,out).print("calling handler for item created event\n");
            run_handler(out, EventType::ITEM_CREATED, handle, (void*)intptr_t(item_id));
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
    vector<int32_t> new_buildings;
    for ( int32_t a = nextBuilding; a < *df::global::building_next_id; a++ ) {
        int32_t index = df::building::binsearch_index(df::global::world->buildings.all, a);
        if ( index == -1 ) {
            //out.print("%s, line %d: Couldn't find new building with id %d.\n", __FILE__, __LINE__, a);
            //the tricky thing is that when the game first starts, it's ok to skip buildings, but otherwise, if you skip buildings, something is probably wrong. TODO: make this smarter
            continue;
        }
        buildings.insert(a);
        new_buildings.emplace_back(a);

    }
    nextBuilding = *df::global::building_next_id;

    //now alert people about destroyed buildings
    for ( auto it = buildings.begin(); it != buildings.end(); ) {
        int32_t id = *it;
        int32_t index = df::building::binsearch_index(df::global::world->buildings.all,id);
        if ( index != -1 ) {
            ++it;
            continue;
        }

        for (auto &[_,handle] : copy) {
            DEBUG(log,out).print("calling handler for destroyed building event\n");
            run_handler(out, EventType::BUILDING, handle, (void*)intptr_t(id));
        }
        it = buildings.erase(it);
    }

    //alert people about newly created buildings
    std::for_each(new_buildings.begin(), new_buildings.end(), [&](int32_t building){
        for (auto &[_,handle] : copy) {
            DEBUG(log,out).print("calling handler for created building event\n");
            run_handler(out, EventType::BUILDING, handle, (void*)intptr_t(building));
        }
    });
}

static void manageConstructionEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    //unordered_set<df::construction*> constructionsNow(df::global::world->event.constructions.begin(), df::global::world->event.constructions.end());

    multimap<Plugin*, EventHandler> copy(handlers[EventType::CONSTRUCTION].begin(), handlers[EventType::CONSTRUCTION].end());

    unordered_set<df::construction> next_construction_set; // will be swapped with constructions
    next_construction_set.reserve(constructions.bucket_count());
    vector<df::construction> new_constructions;

    // find new constructions - swapping found constructions over from constructions to next_construction_set
    for (auto c : df::global::world->event.constructions) {
        auto &construction = *c;
        auto it = constructions.find(construction);
        if (it == constructions.end()) {
            // handle new construction event later
            new_constructions.emplace_back(construction);
        }
        else {
            constructions.erase(it);
        }
        next_construction_set.emplace(construction);
    }

    constructions.swap(next_construction_set);

    // now next_construction_set contains all the constructions that were removed (not found in df::global::world->event.constructions)
    for (auto& construction : next_construction_set) {
        // handle construction removed event
        for (const auto &[_,handle]: copy) {
            DEBUG(log,out).print("calling handler for destroyed construction event\n");
            run_handler(out, EventType::CONSTRUCTION, handle, (void*) &construction);
        }
    }

    // now handle all the new constructions
    for (auto& construction : new_constructions) {
        for (const auto &[_,handle]: copy) {
            DEBUG(log,out).print("calling handler for created construction event\n");
            run_handler(out, EventType::CONSTRUCTION, handle, (void*) &construction);
        }
    }
}

static void manageSyndromeEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    multimap<Plugin*,EventHandler> copy(handlers[EventType::SYNDROME].begin(), handlers[EventType::SYNDROME].end());
    int32_t highestTime = -1;

    std::vector<SyndromeData> new_syndrome_data;
    for (auto unit : df::global::world->units.all) {

/*
        if ( unit->flags1.bits.inactive )
            continue;
*/
        for ( size_t b = 0; b < unit->syndromes.active.size(); b++ ) {
            df::unit_syndrome* syndrome = unit->syndromes.active[b];
            int32_t startTime = syndrome->year*ticksPerYear + syndrome->year_time;
            if ( startTime > highestTime )
                highestTime = startTime;
            if ( startTime <= lastSyndromeTime )
                continue;

            new_syndrome_data.emplace_back(unit->id, b);
        }
    }
    for (auto& data : new_syndrome_data) {
        for (auto &[_,handle] : copy) {
            DEBUG(log,out).print("calling handler for syndrome event\n");
            run_handler(out, EventType::SYNDROME, handle, (void*)&data);
        }
    }

    lastSyndromeTime = highestTime;
}

static void manageInvasionEvent(color_ostream& out) {
    if (!df::global::plotinfo)
        return;
    multimap<Plugin*,EventHandler> copy(handlers[EventType::INVASION].begin(), handlers[EventType::INVASION].end());

    if ( df::global::plotinfo->invasions.next_id <= nextInvasion )
        return;
    nextInvasion = df::global::plotinfo->invasions.next_id;

    for (auto &[_,handle] : copy) {
        DEBUG(log,out).print("calling handler for invasion event\n");
        run_handler(out, EventType::INVASION, handle, (void*)intptr_t(nextInvasion-1));
    }
}

static void manageEquipmentEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    multimap<Plugin*,EventHandler> copy(handlers[EventType::INVENTORY_CHANGE].begin(), handlers[EventType::INVENTORY_CHANGE].end());

    unordered_map<int32_t, InventoryItem> itemIdToInventoryItem;
    unordered_set<int32_t> currentlyEquipped;

    vector<InventoryChangeData> equipment_pickups;
    vector<InventoryChangeData> equipment_drops;
    vector<InventoryChangeData> equipment_changes;


    // This vector stores the pointers to newly created changed items
    // needed as the stack allocated temporary (in the loop) is lost when we go to
    // handle the event calls, so we move that data to the heap if its needed,
    // and then once we are done we delete everything.
    vector<InventoryItem*> changed_items;

    for (auto unit : df::global::world->units.all) {
        itemIdToInventoryItem.clear();
        currentlyEquipped.clear();
        /*if ( unit->flags1.bits.inactive )
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
        for (auto & i : v) {
            itemIdToInventoryItem[i.itemId] = i;
        }
        for ( size_t b = 0; b < unit->inventory.size(); b++ ) {
            df::unit_inventory_item* dfitem_new = unit->inventory[b];
            currentlyEquipped.insert(dfitem_new->item->id);
            InventoryItem item_new(dfitem_new->item->id, *dfitem_new);
            auto c = itemIdToInventoryItem.find(dfitem_new->item->id);
            if ( c == itemIdToInventoryItem.end() ) {
                //new item equipped (probably just picked up)
                changed_items.emplace_back(new InventoryItem(item_new));
                equipment_pickups.emplace_back(unit->id, nullptr, changed_items.back());
                continue;
            }
            InventoryItem item_old = c->second;

            df::unit_inventory_item& item0 = item_old.item;
            df::unit_inventory_item& item1 = item_new.item;
            if ( item0.mode == item1.mode && item0.body_part_id == item1.body_part_id && item0.wound_id == item1.wound_id )
                continue;
            //some sort of change in how it's equipped
            changed_items.emplace_back(new InventoryItem(item_new));
            InventoryItem* item_new_ptr = changed_items.back();
            changed_items.emplace_back(new InventoryItem(item_old));
            InventoryItem* item_old_ptr = changed_items.back();
            equipment_changes.emplace_back(unit->id, item_old_ptr, item_new_ptr);
        }
        //check for dropped items
        for (auto i : v) {
            if ( currentlyEquipped.find(i.itemId) != currentlyEquipped.end() )
                continue;
            //TODO: delete ptr if invalid
            changed_items.emplace_back(new InventoryItem(i));
            equipment_drops.emplace_back(unit->id, changed_items.back(), nullptr);
        }
        if ( !hadEquipment )
            delete temp;

        //update equipment
        vector<InventoryItem>& equipment = equipmentLog[unit->id];
        equipment.clear();
        for (auto dfitem : unit->inventory) {
            InventoryItem item(dfitem->item->id, *dfitem);
            equipment.push_back(item);
        }
    }

    // now handle events
    std::for_each(equipment_pickups.begin(), equipment_pickups.end(), [&](InventoryChangeData& data) {
        for (auto &[_, handle] : copy) {
            DEBUG(log,out).print("calling handler for new item equipped inventory change event\n");
            run_handler(out, EventType::INVENTORY_CHANGE, handle, (void*) &data);
        }
    });
    std::for_each(equipment_drops.begin(), equipment_drops.end(), [&](InventoryChangeData& data) {
        for (auto &[_, handle] : copy) {
            DEBUG(log,out).print("calling handler for dropped item inventory change event\n");
            run_handler(out, EventType::INVENTORY_CHANGE, handle, (void*) &data);
        }
    });
    std::for_each(equipment_changes.begin(), equipment_changes.end(), [&](InventoryChangeData& data) {
        for (auto &[_, handle] : copy) {
            DEBUG(log,out).print("calling handler for inventory change event\n");
            run_handler(out, EventType::INVENTORY_CHANGE, handle, (void*) &data);
        }
    });

    // clean up changed items list
    std::for_each(changed_items.begin(), changed_items.end(), [](InventoryItem* p){
        delete p;
    });
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

static void manageReportEvent(color_ostream& out) {
    if (!df::global::world)
        return;
    multimap<Plugin*,EventHandler> copy(handlers[EventType::REPORT].begin(), handlers[EventType::REPORT].end());
    std::vector<df::report*>& reports = df::global::world->status.reports;
    size_t idx = df::report::binsearch_index(reports, lastReport, false);
    // returns the index to the key equal to or greater than the key provided
    while (idx < reports.size() && reports[idx]->id <= lastReport) {
        idx++;
    }
    // returns the index to the key equal to or greater than the key provided

    for ( ; idx < reports.size(); idx++ ) {
        df::report* report = reports[idx];
        for (auto &[_,handle] : copy) {
            DEBUG(log,out).print("calling handler for report event\n");
            run_handler(out, EventType::REPORT, handle, (void*)intptr_t(report->id));
        }
        lastReport = report->id;
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
    multimap<Plugin*,EventHandler> copy(handlers[EventType::UNIT_ATTACK].begin(), handlers[EventType::UNIT_ATTACK].end());
    std::vector<df::report*>& reports = df::global::world->status.reports;
    size_t idx = df::report::binsearch_index(reports, lastReportUnitAttack, false);
    // returns the index to the key equal to or greater than the key provided
    idx = reports[idx]->id == lastReportUnitAttack ? idx + 1 : idx; // we need the index after (where the new stuff is)

    std::set<int32_t> strikeReports;
    for ( ; idx < reports.size(); idx++ ) {
        df::report* report = reports[idx];
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
    unordered_set<std::pair<int32_t, int32_t>, hash_pair> already_done;
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
        if ( wound1 && already_done.find(std::make_pair(unit1->id,unit2->id)) == already_done.end() ) {
            data.attacker = unit1->id;
            data.defender = unit2->id;
            data.wound = wound1->id;

            already_done.emplace(unit1->id, unit2->id);
            for (auto &[_,handle] : copy) {
                DEBUG(log,out).print("calling handler for unit1 attack unit attack event\n");
                run_handler(out, EventType::UNIT_ATTACK, handle, (void*)&data);
            }
        }

        if ( wound2 && already_done.find(std::make_pair(unit1->id,unit2->id)) == already_done.end() ) {
            data.attacker = unit2->id;
            data.defender = unit1->id;
            data.wound = wound2->id;

            already_done.emplace(unit1->id, unit2->id);
            for (auto &[_,handle] : copy) {
                DEBUG(log,out).print("calling handler for unit2 attack unit attack event\n");
                run_handler(out, EventType::UNIT_ATTACK, handle, (void*)&data);
            }
        }

        if ( Units::isKilled(unit1) ) {
            data.attacker = unit2->id;
            data.defender = unit1->id;
            data.wound = -1;

            already_done.emplace(unit1->id, unit2->id);
            for (auto &[_,handle] : copy) {
                DEBUG(log,out).print("calling handler for unit1 killed unit attack event\n");
                run_handler(out, EventType::UNIT_ATTACK, handle, (void*)&data);
            }
        }

        if ( Units::isKilled(unit2) ) {
            data.attacker = unit1->id;
            data.defender = unit2->id;
            data.wound = -1;

            already_done.emplace(unit1->id, unit2->id);
            for (auto &[_,handle] : copy) {
                DEBUG(log,out).print("calling handler for unit2 killed unit attack event\n");
                run_handler(out, EventType::UNIT_ATTACK, handle, (void*)&data);
            }
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

    df::report* lastAttackEvent = nullptr;
    df::unit* lastAttacker = nullptr;
    //df::unit* lastDefender = NULL;
    unordered_map<int32_t,unordered_set<int32_t> > history;
    for ( ; a < reports.size(); a++ ) {
        df::report* report = reports[a];
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
        if ( attack && a+1 < reports.size() && reports[a+1]->type == df::announcement_type::INTERACTION_TARGET ) {
//out.print("%s,%d\n",__FILE__,__LINE__);
            vector<df::unit*> relevant_units = gatherRelevantUnits(out, lastAttackEvent, reports[a + 1]);
            InteractionData data2 = getAttacker(out, lastAttackEvent, lastAttacker, reports[a+1], relevant_units);
            if ( data.attacker == data2.attacker && (data.defender == -1 || data.defender == data2.defender) ) {
//out.print("%s,%d\n",__FILE__,__LINE__);
                data = data2;
                a++;
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
        for (auto &[_,handle] : copy) {
            DEBUG(log,out).print("calling handler for interaction event\n");
            run_handler(out, EventType::INTERACTION, handle, (void*)&data);
        }
        //TODO: deduce attacker from latest defend event first
    }
}
