#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "PluginLua.h"

#include "modules/EventManager.h"
#include "modules/Items.h"
#include "modules/Job.h"
#include "modules/Persistence.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/activity_entry.h"
#include "df/activity_event.h"
#include "df/activity_event_conflictst.h"
#include "df/activity_event_conversationst.h"
#include "df/activity_event_copy_written_contentst.h"
#include "df/activity_event_discuss_topicst.h"
#include "df/activity_event_make_believest.h"
#include "df/activity_event_performancest.h"
#include "df/activity_event_playst.h"
#include "df/activity_event_play_with_toyst.h"
#include "df/activity_event_ponder_topicst.h"
#include "df/activity_event_prayerst.h"
#include "df/activity_event_ranged_practicest.h"
#include "df/activity_event_readst.h"
#include "df/activity_event_skill_demonstrationst.h"
#include "df/activity_event_socializest.h"
#include "df/activity_event_sparringst.h"
#include "df/activity_event_teach_topicst.h"
#include "df/activity_event_writest.h"
#include "df/activity_event_worshipst.h"
#include "df/building_nest_boxst.h"
#include "df/building_trapst.h"
#include "df/init.h"
#include "df/item_eggst.h"
#include "df/unit.h"
#include "df/world.h"

#include <array>

using std::string;
using std::vector;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("timestream");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(cur_year_tick);
REQUIRE_GLOBAL(cur_year_tick_advmode);
REQUIRE_GLOBAL(init);
REQUIRE_GLOBAL(world);

namespace DFHack {
    // for configuration-related logging
    DBG_DECLARE(timestream, control, DebugCategory::LINFO);
    // for logging during the periodic scan
    DBG_DECLARE(timestream, cycle, DebugCategory::LINFO);
    // for logging during event callbacks
    DBG_DECLARE(timestream, event, DebugCategory::LINFO);
}

static const string CONFIG_KEY = string(plugin_name) + "/config";
static PersistentDataItem config;

enum ConfigValues {
    CONFIG_IS_ENABLED = 0,
    CONFIG_TARGET_FPS = 1,
};

static const int32_t CYCLE_TICKS = 1;
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

static command_result do_command(color_ostream &out, vector<string> &parameters);
static void on_new_active_unit(color_ostream& out, void* data);
static void do_cycle(color_ostream &out);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    DEBUG(control,out).print("initializing %s\n", plugin_name);

    commands.push_back(PluginCommand(
        plugin_name,
        "Fix FPS death.",
        do_command));

    return CR_OK;
}


// ensure we never skip over cur_year_tick values that match this list
// first element of the pair is the modulus
// second element of the pair is a vector of remainders to match
static vector<std::pair<int32_t, vector<int32_t>>> TICK_TRIGGERS = {
    {10,  {0}},           // 0: season ticks (and lots of other stuff)
                          // 0 mod 100: crop growth, strange mood, minimap update, rot
                          // 20 mod 100: building updates
                          // 40 mod 100: assign tombs to newly tomb-eligible corpses
                          // 80 mod 100: incarceration updates
                          // 40 mod 1000: remove excess seeds
    {50,  {25, 35, 45}},  // 25: stockpile updates
                          // 35: check bags
                          // 35 mod 100: job auction
                          // 45: stockpile updates
    {100, {99}}           // 99: new job creation
};

// "owed" ticks we would like to skip at the next opportunity
static float timeskip_deficit = 0.0F;

// birthday_triggers is a dense sequence of cur_year_tick values -> next unit birthday
// the sequence covers 0 .. greatest unit birthday value
// this cache is augmented when new units appear (as per the new unit event) and is cleared and
// refreshed from scratch once a year to evict data for units that are no longer active.
static vector<int32_t> birthday_triggers;

// coverage record for cur_year_tick % 50 so we can be sure that all items are being scanned
// (DF scans 1/50th of items every tick based on cur_year_tick % 50)
// we want every section hit at least once every 1000 ticks
static const uint32_t NUM_COVERAGE_TICKS = 50;
static std::array<bool, NUM_COVERAGE_TICKS> tick_coverage;

// only throttle due to tick_coverage at most once per season tick to avoid clustering
static bool season_tick_throttled = false;

static const int32_t TICKS_PER_YEAR = 403200;

static void register_birthday(df::unit * unit) {
    int32_t btick = unit->birth_time;
    if (btick < 0 || btick > TICKS_PER_YEAR)
        return;
    if (birthday_triggers.size() <= (size_t)btick)
        birthday_triggers.resize(btick+1, INT32_MAX);
    for (int32_t tick=btick; tick >= 0; --tick) {
        if (birthday_triggers[tick] > btick)
            birthday_triggers[tick] = btick;
        else
            break;
    }
}

static void refresh_birthday_triggers() {
    birthday_triggers.clear();
    for (auto unit : world->units.active) {
        if (Units::isActive(unit) && !Units::isDead(unit))
            register_birthday(unit);
    }
}

static void reset_ephemeral_state() {
    timeskip_deficit = 0.0F;
    refresh_birthday_triggers();
    tick_coverage = {};
    season_tick_throttled = false;
}

static void do_disable() {
    EventManager::unregisterAll(plugin_self);
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot enable %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    static EventManager::EventHandler new_unit_handler(plugin_self, on_new_active_unit, CYCLE_TICKS);

    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(control,out).print("%s from the API; persisting\n",
                                is_enabled ? "enabled" : "disabled");
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
        if (enable) {
            reset_ephemeral_state();
            EventManager::registerListener(EventManager::EventType::UNIT_NEW_ACTIVE, new_unit_handler);
            do_cycle(out);
        } else {
            do_disable();
        }
    } else {
        DEBUG(control,out).print("%s from the API, but already %s; no action\n",
                                is_enabled ? "enabled" : "disabled",
                                is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    DEBUG(control,out).print("shutting down %s\n", plugin_name);

    return CR_OK;
}

static int clamp_fps_to_valid(int32_t fps) {
    return std::max(10, fps);
}

static void set_default_settings() {
    config.set_int(CONFIG_TARGET_FPS, clamp_fps_to_valid(init->fps_cap));
}

static void migrate_old_config(color_ostream &out) {
    Lua::CallLuaModuleFunction(out, "plugins.timestream", "migrate_old_config");
}

DFhackCExport command_result plugin_load_site_data(color_ostream &out) {
    cycle_timestamp = 0;
    config = World::GetPersistentSiteData(CONFIG_KEY);

    if (!config.isValid()) {
        DEBUG(control,out).print("no config found in this save; initializing\n");
        config = World::AddPersistentSiteData(CONFIG_KEY);
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
        set_default_settings();
        migrate_old_config(out);
    }

    plugin_enable(out, config.get_bool(CONFIG_IS_ENABLED));
    DEBUG(control,out).print("loading persisted enabled state: %s\n",
                            is_enabled ? "true" : "false");

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == DFHack::SC_WORLD_UNLOADED) {
        if (is_enabled) {
            DEBUG(control,out).print("world unloaded; disabling %s\n",
                                    plugin_name);
            do_disable();
        }
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (world->frame_counter - cycle_timestamp >= CYCLE_TICKS)
        do_cycle(out);
    return CR_OK;
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot run %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    bool show_help = false;
    if (!Lua::CallLuaModuleFunction(out, "plugins.timestream", "parse_commandline", std::make_tuple(parameters),
            1, [&](lua_State *L) {
                show_help = !lua_toboolean(L, -1);
            })) {
        return CR_FAILURE;
    }

    return show_help ? CR_WRONG_USAGE : CR_OK;
}

/////////////////////////////////////////////////////
// cycle logic
//

static void record_coverage(color_ostream &out) {
    uint32_t coverage_slot = *cur_year_tick % NUM_COVERAGE_TICKS;
    if (coverage_slot >= NUM_COVERAGE_TICKS)
        return;
    if (!tick_coverage[coverage_slot]) {
        DEBUG(cycle,out).print("recording coverage for slot: %u", coverage_slot);
    }
    tick_coverage[coverage_slot] = true;
}

static float get_desired_timeskip(int32_t real_fps, int32_t target_fps) {
    // minus 1 to account for the current frame
    return (float(target_fps) / float(real_fps)) - 1;
}

static int32_t get_next_trigger_year_tick(int32_t next_tick) {
    int32_t next_trigger_tick = INT32_MAX;
    for (auto trigger : TICK_TRIGGERS) {
        int32_t cur_rem = next_tick % trigger.first;
        for (auto rem : trigger.second) {
            if (cur_rem <= rem) {
                next_trigger_tick = std::min(next_trigger_tick, next_tick + (rem - cur_rem));
                continue;
            }
        }
        next_trigger_tick = std::min(next_trigger_tick, next_tick + trigger.first - cur_rem + trigger.second.back());
    }
    return next_trigger_tick;
}

static int32_t get_next_birthday(int32_t next_tick) {
    if (next_tick < 0 || next_tick >= (int32_t)birthday_triggers.size())
        return INT32_MAX;
    return birthday_triggers[next_tick];
}

static int32_t clamp_coverage(int32_t timeskip) {
    if (season_tick_throttled)
        return timeskip;
    for (int32_t val = 1; val <= timeskip; ++val) {
        uint32_t coverage_slot = (*cur_year_tick + val) % NUM_COVERAGE_TICKS;
        if (coverage_slot < NUM_COVERAGE_TICKS && !tick_coverage[coverage_slot]) {
            season_tick_throttled = true;
            return val - 1;
        }
    }
    return timeskip;
}

static int32_t clamp_timeskip(int32_t timeskip) {
    if (timeskip <= 0)
        return 0;
    int32_t next_tick = *cur_year_tick + 1;
    timeskip = std::min(timeskip, get_next_trigger_year_tick(next_tick) - next_tick);
    timeskip = std::min(timeskip, get_next_birthday(next_tick) - next_tick);
    return clamp_coverage(timeskip);
}

template <typename FT, typename T>
static void increment_counter(T *obj, FT T::*field, int32_t timeskip) {
    if (obj->*field <= 0)
        return;
    obj->*field = obj->*field + timeskip;
}

template <typename FT, typename T>
static void decrement_counter(T *obj, FT T::*field, int32_t timeskip) {
    if (obj->*field <= 0)
        return;
    // TODO: check for overflow/underflow
    int32_t cur_val = static_cast<int32_t>(obj->*field);
    obj->*field = static_cast<FT>(std::max(1, cur_val - timeskip));
}

static void adjust_unit_counters(df::unit * unit, int32_t timeskip) {
    auto * c1 = &unit->counters;
    decrement_counter(c1, &df::unit::T_counters::think_counter, timeskip);
    decrement_counter(c1, &df::unit::T_counters::job_counter, timeskip);
    decrement_counter(c1, &df::unit::T_counters::swap_counter, timeskip);
    decrement_counter(c1, &df::unit::T_counters::winded, timeskip);
    decrement_counter(c1, &df::unit::T_counters::stunned, timeskip);
    decrement_counter(c1, &df::unit::T_counters::unconscious, timeskip);
    decrement_counter(c1, &df::unit::T_counters::suffocation, timeskip);
    decrement_counter(c1, &df::unit::T_counters::webbed, timeskip);
    decrement_counter(c1, &df::unit::T_counters::soldier_mood_countdown, timeskip);
    decrement_counter(c1, &df::unit::T_counters::pain, timeskip);
    decrement_counter(c1, &df::unit::T_counters::nausea, timeskip);
    decrement_counter(c1, &df::unit::T_counters::dizziness, timeskip);
    auto * c2 = &unit->counters2;
    decrement_counter(c2, &df::unit::T_counters2::paralysis, timeskip);
    decrement_counter(c2, &df::unit::T_counters2::numbness, timeskip);
    decrement_counter(c2, &df::unit::T_counters2::fever, timeskip);
    decrement_counter(c2, &df::unit::T_counters2::exhaustion, timeskip * 3);
    increment_counter(c2, &df::unit::T_counters2::hunger_timer, timeskip);
    increment_counter(c2, &df::unit::T_counters2::thirst_timer, timeskip);
    auto job = unit->job.current_job;
    if (job && job->job_type == df::job_type::Rest)
        decrement_counter(c2, &df::unit::T_counters2::sleepiness_timer, timeskip * 200);
    else if (job && job->job_type == df::job_type::Sleep)
        decrement_counter(c2, &df::unit::T_counters2::sleepiness_timer, timeskip * 19);
    else
        increment_counter(c2, &df::unit::T_counters2::sleepiness_timer, timeskip);
    decrement_counter(c2, &df::unit::T_counters2::stomach_content, timeskip * 5);
    decrement_counter(c2, &df::unit::T_counters2::stomach_food, timeskip * 5);
    decrement_counter(c2, &df::unit::T_counters2::vomit_timeout, timeskip);
    // stored_fat wanders about based on other state; we can likely leave it alone and
    // not materially affect gameplay
}

// need to manually adjust job completion_timer values for jobs that are controlled by unit actions
// with a timer of 1, which are destroyed immediately after they are created. longer-lived unit
// actions are already sufficiently handled by dfhack.units.subtractGroupActionTimers().
// this will also decrement timers for jobs with actions that have just expired, but on average, this
// should balance out to be correct, since we're losing time when we subtract from the action timers
// and cap the value so it never drops below 1.
static void adjust_job_counter(df::unit * unit, int32_t timeskip) {
    auto job = unit->job.current_job;
    if (!job)
        return;
    for (auto action : unit->actions) {
        if (action->type == df::unit_action_type::Job || action->type == df::unit_action_type::JobRecover)
            return;
    }
    decrement_counter(job, &df::job::completion_timer, timeskip);
}

// unit needs appear to be incremented on season ticks, so we don't need to worry about those
// since the TICK_TRIGGERS check makes sure that we never skip season ticks
static void adjust_units(color_ostream &out, int32_t timeskip) {
    for (auto unit : world->units.active) {
        if (!Units::isActive(unit))
            continue;
        decrement_counter(unit, &df::unit::pregnancy_timer, timeskip);
        Units::subtractGroupActionTimers(out, unit, timeskip, df::unit_action_type_group::All);
        if (!Units::isOwnGroup(unit))
            continue;
        adjust_unit_counters(unit, timeskip);
        adjust_job_counter(unit, timeskip);
    }
}

// behavior ascertained from in-game observation
static void adjust_activities(color_ostream &out, int32_t timeskip) {
    for (auto act : world->activities.all) {
        for (auto ev : act->events) {
            switch (ev->getType()) {
                using namespace df::enums::activity_event_type;

            case TrainingSession:
                // no counters
                break;

            case CombatTraining:
                // has organize_counter at a non-zero value, but it doesn't seem to move
                break;

            case SkillDemonstration:
            if (auto sd_ev = virtual_cast<df::activity_event_skill_demonstrationst>(ev)) {
                // can be negative or positive, but always counts towards 0
                if (sd_ev->organize_counter < 0)
                    sd_ev->organize_counter = std::min(-1, sd_ev->organize_counter + timeskip);
                else
                    decrement_counter(sd_ev, &df::activity_event_skill_demonstrationst::organize_counter, timeskip);
                decrement_counter(sd_ev, &df::activity_event_skill_demonstrationst::train_countdown, timeskip);
                break;
            }

            case IndividualSkillDrill:
                // only counts down on season ticks, nothing to do here
                break;

            case Sparring:
            if (auto s_ev = virtual_cast<df::activity_event_sparringst>(ev)) {
                decrement_counter(s_ev, &df::activity_event_sparringst::countdown, timeskip * 2);
                break;
            }

            case RangedPractice:
            // countdown appears to never move from 0
            if (auto rp_ev = virtual_cast<df::activity_event_ranged_practicest>(ev)) {
                decrement_counter(rp_ev, &df::activity_event_ranged_practicest::countdown, timeskip);
                break;
            }

            case Harassment:
            {
                DEBUG(cycle,out).print("activity_event_harassmentst ready for analysis\n");
                break;
            }

            case Conversation:
            if (auto c_ev = virtual_cast<df::activity_event_conversationst>(ev)) {
                increment_counter(c_ev, &df::activity_event_conversationst::pause, timeskip);
                break;
            }

            case Conflict:
            if (auto c_ev = virtual_cast<df::activity_event_conflictst>(ev)) {
                increment_counter(c_ev, &df::activity_event_conflictst::inactivity_timer, timeskip);
                increment_counter(c_ev, &df::activity_event_conflictst::attack_inactivity_timer, timeskip);
                increment_counter(c_ev, &df::activity_event_conflictst::stop_fort_fights_timer, timeskip);
                break;
            }

            case Guard:
                // no counters
                break;

            case Reunion:
            {
                DEBUG(cycle,out).print("activity_event_reunionst ready for analysis\n");
                break;
            }

            case Prayer:
            if (auto rp_ev = virtual_cast<df::activity_event_prayerst>(ev)) {
                decrement_counter(rp_ev, &df::activity_event_prayerst::timer, timeskip);
                break;
            }

            case Socialize:
            if (auto p_ev = virtual_cast<df::activity_event_socializest>(ev)) {
                increment_counter(p_ev, &df::activity_event_socializest::down_time_counter, timeskip);
                break;
            }

            case Worship:
            if (auto p_ev = virtual_cast<df::activity_event_worshipst>(ev)) {
                increment_counter(p_ev, &df::activity_event_worshipst::down_time_counter, timeskip);
                break;
            }

            case Performance:
            if (auto p_ev = virtual_cast<df::activity_event_performancest>(ev)) {
                increment_counter(p_ev, &df::activity_event_performancest::current_position, timeskip);
                break;
            }

            case Research:
                // no counters
                break;

            case PonderTopic:
            if (auto pt_ev = virtual_cast<df::activity_event_ponder_topicst>(ev)) {
                decrement_counter(pt_ev, &df::activity_event_ponder_topicst::timer, timeskip);
                break;
            }

            case DiscussTopic:
            if (auto pt_ev = virtual_cast<df::activity_event_discuss_topicst>(ev)) {
                decrement_counter(pt_ev, &df::activity_event_discuss_topicst::timer, timeskip);
                break;
            }

            case Read:
            if (auto pt_ev = virtual_cast<df::activity_event_readst>(ev)) {
                decrement_counter(pt_ev, &df::activity_event_readst::timer, timeskip);
                break;
            }

            case FillServiceOrder:
                // no counters
                break;

            case Write:
            if (auto pt_ev = virtual_cast<df::activity_event_writest>(ev)) {
                decrement_counter(pt_ev, &df::activity_event_writest::timer, timeskip);
                break;
            }

            case CopyWrittenContent:
            if (auto pt_ev = virtual_cast<df::activity_event_copy_written_contentst>(ev)) {
                decrement_counter(pt_ev, &df::activity_event_copy_written_contentst::timer, timeskip);
                break;
            }

            case TeachTopic:
            if (auto pt_ev = virtual_cast<df::activity_event_teach_topicst>(ev)) {
                decrement_counter(pt_ev, &df::activity_event_teach_topicst::time_left, timeskip);
                break;
            }

            case Play:
            if (auto p_ev = virtual_cast<df::activity_event_playst>(ev)) {
                increment_counter(p_ev, &df::activity_event_playst::down_time_counter, timeskip);
                break;
            }

            case MakeBelieve:
            if (auto pt_ev = virtual_cast<df::activity_event_make_believest>(ev)) {
                decrement_counter(pt_ev, &df::activity_event_make_believest::time_left, timeskip);
                break;
            }

            case PlayWithToy:
            if (auto pt_ev = virtual_cast<df::activity_event_play_with_toyst>(ev)) {
                decrement_counter(pt_ev, &df::activity_event_play_with_toyst::time_left, timeskip);
                break;
            }

            case Encounter:
            {
                DEBUG(cycle,out).print("activity_event_encounterst ready for analysis\n");
                break;
            }

            case StoreObject:
            {
                DEBUG(cycle,out).print("activity_event_store_objectst ready for analysis\n");
                break;
            }

            // no default case so compiler complains if we miss something
            }
        }
    }
}

static void adjust_buildings(color_ostream &out, int32_t timeskip) {
    // decrement trap timers
    for (df::building_trapst *tr : world->buildings.other.TRAP) {
        decrement_counter(tr, &df::building_trapst::ready_timeout, timeskip);
        // used by pressure plates to delay until the plate is triggerable again
        // other trap types never set this to a value higher than 1 so it is safe to decrement here
        decrement_counter(tr, &df::building_trapst::state, timeskip);
    }

    for (df::building *bld : world->buildings.all) {
        // assumes age > 0, but that will become true very quickly for all new buildings
        increment_counter(bld, &df::building::age, timeskip);
    }
}

static void adjust_items(color_ostream &out, int32_t timeskip) {
    // increment incubation counters for fertile eggs in non-forbidden nestboxes
    for (df::building_nest_boxst *nb : world->buildings.other.NEST_BOX) {
        for (auto & contained_item : nb->contained_items) {
            if (contained_item->use_mode == df::building_item_role_type::PERM) {
                if (contained_item->item->flags.bits.forbid)
                    break;
                else
                    continue;
            }
            if (auto *egg = virtual_cast<df::item_eggst>(contained_item->item); egg && egg->egg_flags.bits.fertile)
                increment_counter(egg, &df::item_eggst::incubation_counter, timeskip);
        }
    }
}

static void do_cycle(color_ostream &out) {
    DEBUG(cycle,out).print("running %s cycle\n", plugin_name);

    // mark that we have recently run
    cycle_timestamp = world->frame_counter;

    record_coverage(out);

    if (*cur_year_tick % 10 == 0) {
        season_tick_throttled = false;
        if (*cur_year_tick % 1000 == 0) {
            DEBUG(cycle,out).print("resetting coverage tracking\n");
            tick_coverage = {};
        }
        if (*cur_year_tick == 0)
            refresh_birthday_triggers();
    }

    uint32_t unpaused_fps = Core::getInstance().perf_counters.getUnpausedFps();
    int32_t real_fps = std::max(1, static_cast<int32_t>(std::min(static_cast<uint32_t>(INT32_MAX), unpaused_fps)));
    int32_t target_fps = config.get_int(CONFIG_TARGET_FPS);
    if (real_fps >= target_fps) {
        timeskip_deficit = 0.0F;
        return;
    }

    float desired_timeskip = get_desired_timeskip(real_fps, target_fps) + timeskip_deficit;
    int32_t timeskip = std::max(0, clamp_timeskip(int32_t(desired_timeskip)));

    // don't let our deficit grow unbounded if we can never catch up
    timeskip_deficit = std::min(desired_timeskip - float(timeskip), 100.0F);

    DEBUG(cycle,out).print("cur_year_tick: %d, real_fps: %d, timeskip: (%d, +%.2f)\n", *cur_year_tick, real_fps, timeskip, timeskip_deficit);
    if (timeskip <= 0)
        return;

    *cur_year_tick += timeskip;
    *cur_year_tick_advmode += timeskip * 144;

    adjust_units(out, timeskip);
    adjust_activities(out, timeskip);
    adjust_buildings(out, timeskip);
    adjust_items(out, timeskip);
}

/////////////////////////////////////////////////////
// event logic
//

static void on_new_active_unit(color_ostream& out, void* data) {
    int32_t unit_id = reinterpret_cast<std::intptr_t>(data);
    auto unit = df::unit::find(unit_id);
    if (!unit)
        return;
    DEBUG(event,out).print("registering new unit %d (%s)\n", unit->id, Units::getReadableName(unit).c_str());
    register_birthday(unit);
}

/////////////////////////////////////////////////////
// Lua API
//

static void timestream_setFps(color_ostream &out, int fps) {
    DEBUG(cycle,out).print("timestream_setFps: %d\n", fps);
    config.set_int(CONFIG_TARGET_FPS, clamp_fps_to_valid(fps));
}

static int timestream_getFps(color_ostream &out) {
    DEBUG(cycle,out).print("timestream_getFps\n");
    return config.get_int(CONFIG_TARGET_FPS);
}

static void timestream_resetSettings(color_ostream &out) {
    DEBUG(cycle,out).print("timestream_resetSettings\n");
    set_default_settings();
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(timestream_setFps),
    DFHACK_LUA_FUNCTION(timestream_getFps),
    DFHACK_LUA_FUNCTION(timestream_resetSettings),
    DFHACK_LUA_END
};
