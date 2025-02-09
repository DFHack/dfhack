#include "Debug.h"
#include "LuaTools.h"
#include "PluginLua.h"
#include "PluginManager.h"
#include "VTableInterpose.h"

#include "modules/Gui.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/announcements.h"
#include "df/d_init.h"
#include "df/plotinfost.h"
#include "df/unit.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/world.h"

#include <random>

using namespace DFHack;

using std::string;
using std::vector;

DFHACK_PLUGIN("spectate");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(d_init);
REQUIRE_GLOBAL(plotinfo);
REQUIRE_GLOBAL(world);

namespace DFHack {
    DBG_DECLARE(spectate, control, DebugCategory::LINFO);
    DBG_DECLARE(spectate, cycle, DebugCategory::LINFO);
}

static uint32_t next_cycle_unpaused_ms = 0;  // threshold for the next cycle

static const size_t MAX_HISTORY = 200;

static struct Configuration {
    bool auto_disengage;
    bool auto_unpause;
    bool cinematic_action;
    bool include_animals;
    bool include_hostiles;
    bool include_visitors;
    bool include_wildlife;
    bool prefer_conflict;
    bool prefer_new_arrivals;
    int32_t follow_ms;

    void reset() {
        auto_disengage = true;
        auto_unpause = false;
        cinematic_action = true;
        include_animals = false;
        include_hostiles = false;
        include_visitors = false;
        include_wildlife = false;
        prefer_conflict = true;
        prefer_new_arrivals = true;
        follow_ms = 10000;
    }
} config;

static class AnnouncementSettings {
    bool was_in_settings = false; // whether we were in the vanilla settings screen last update

    const size_t announcement_flag_arr_size = sizeof(decltype(df::announcements::flags)) / sizeof(df::announcement_flags);
    std::unique_ptr<uint32_t *> saved;

    void save_settings(color_ostream &out) {
        if (!saved)
            saved = std::make_unique<uint32_t *>(new uint32_t[announcement_flag_arr_size]);
        DEBUG(control,out).print("saving announcement settings\n");
        for (size_t i = 0; i < announcement_flag_arr_size; ++i)
            (*saved)[i] = d_init->announcements.flags[i].whole;
    }

public:
    void reset(color_ostream &out, bool skip_restore = false) {
        was_in_settings = false;

        if (saved) {
            if (!skip_restore)
                restore_settings(out);
            delete[] *saved;
            saved.reset();
        }
    }

    void on_update(color_ostream &out) {
        if (Gui::matchFocusString("dwarfmode/Settings")) {
            if (!was_in_settings) {
                DEBUG(cycle,out).print("settings screen active; restoring announcement settings\n");
                restore_settings(out);
                was_in_settings = true;
            }
        } else if (was_in_settings) {
            was_in_settings = false;
            if (config.auto_unpause) {
                DEBUG(cycle,out).print("settings screen now inactive; disabling announcement pausing\n");
                save_and_scrub_settings(out);
            }
        }
    }

    void restore_settings(color_ostream &out) {
        if (!saved || was_in_settings)
            return;
        DEBUG(control,out).print("restoring saved announcement settings\n");
        for (size_t i = 0; i < announcement_flag_arr_size; ++i)
            d_init->announcements.flags[i].whole = (*saved)[i];
    }

    // remove pausing, popups, and recentering from all announcements
    // saves first so the original settings can be restored
    void save_and_scrub_settings(color_ostream &out) {
        if (Gui::matchFocusString("dwarfmode/Settings")) {
            DEBUG(control,out).print("not modifying announcement settings; vanilla settings screen is active\n");
            return;
        }

        save_settings(out);

        DEBUG(control,out).print("scrubbing announcement settings\n");
        for (auto& flag : d_init->announcements.flags) {
            flag.bits.DO_MEGA = false;
            flag.bits.PAUSE = false;
            flag.bits.RECENTER = false;
        }
    }
} announcement_settings;

static void follow_a_dwarf(color_ostream &out);

static class UnitHistory {
    std::deque<int32_t> history;
    size_t offset = 0;

public:
    void reset() {
        history.clear();
        offset = 0;
    }

    void add(color_ostream &out, df::unit *unit) {
        int32_t id = unit->id;
        DEBUG(cycle,out).print("now following unit %d: %s\n", id, DF2CONSOLE(Units::getReadableName(unit)).c_str());
        Gui::revealInDwarfmodeMap(Units::getPosition(unit), false, World::ReadPauseState());
        plotinfo->follow_unit = id;
        if (offset > 0) {
            DEBUG(cycle,out).print("trimming history forward of offset %zd\n", offset);
            history.resize(history.size() - offset);
            offset = 0;
        }
        history.push_back(id);
        if (history.size() > MAX_HISTORY) {
            DEBUG(cycle,out).print("history full, truncating\n");
            history.pop_front();
        }
        DEBUG(cycle,out).print("history now has %zd entries\n", history.size());
    }

    void scan_back(color_ostream &out) {
        if (history.empty() || offset >= history.size()-1)
            return;
        int unit_id = history[history.size() - (1 + ++offset)];
        DEBUG(cycle,out).print("scanning back to unit %d at offset %zd\n", unit_id, offset);
        if (auto unit = df::unit::find(unit_id))
            Gui::revealInDwarfmodeMap(Units::getPosition(unit), false, World::ReadPauseState());
        plotinfo->follow_unit = unit_id;
    }

    void scan_forward(color_ostream &out) {
        if (history.empty() || offset == 0) {
            DEBUG(cycle,out).print("already at most recent unit; following new unit\n");
            follow_a_dwarf(out);
            return;
        }

        int unit_id = history[history.size() - (1 + --offset)];
        DEBUG(cycle,out).print("scanning forward to unit %d at offset %zd\n", unit_id, offset);
        if (auto unit = df::unit::find(unit_id))
            Gui::revealInDwarfmodeMap(Units::getPosition(unit), false, World::ReadPauseState());
        plotinfo->follow_unit = unit_id;
    }
} unit_history;

struct forward_back_interceptor : df::viewscreen_dwarfmodest {
    typedef df::viewscreen_dwarfmodest interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key> *input)) {
        bool is_at_default_view = Gui::matchFocusString("dwarfmode/Default", Gui::getDFViewscreen());
        if (is_at_default_view && input->count(df::interface_key::CUSTOM_LEFT))
            unit_history.scan_back(Core::getInstance().getConsole());
        else if (is_at_default_view && input->count(df::interface_key::CUSTOM_RIGHT))
            unit_history.scan_forward(Core::getInstance().getConsole());
        else {
            INTERPOSE_NEXT(feed)(input);
        }
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(forward_back_interceptor, feed);

static command_result do_command(color_ostream &out, vector<string> &parameters);
static void follow_a_dwarf(color_ostream &out);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    DEBUG(control,out).print("initializing %s\n", plugin_name);

    commands.push_back(PluginCommand(
        plugin_name,
        "Automated spectator mode.",
        do_command));

    return CR_OK;
}

void on_disable(color_ostream &out) {
    INTERPOSE_HOOK(forward_back_interceptor, feed).apply(false);
    announcement_settings.reset(out);
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot enable %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(control,out).print("%s from the API; persisting\n",
                                is_enabled ? "enabled" : "disabled");
        if (enable) {
            INFO(control,out).print("Spectate mode enabled!\n");
            config.reset();
            if (!Lua::CallLuaModuleFunction(out, "plugins.spectate", "refresh_cpp_config")) {
                WARN(control,out).print("Failed to refresh config\n");
            }
            INTERPOSE_HOOK(forward_back_interceptor, feed).apply();
            follow_a_dwarf(out);
        } else {
            INFO(control,out).print("Spectate mode disabled!\n");
            plotinfo->follow_unit = -1;
            on_disable(out);
            // don't reset the unit history since we may want to re-enable
        }
    } else {
        DEBUG(control,out).print("%s from the API, but already %s; no action\n",
                                is_enabled ? "enabled" : "disabled",
                                is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    DEBUG(control,out).print("shutting down %s\n", plugin_name);
    on_disable(out);
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    switch (event) {
    case SC_WORLD_LOADED:
        next_cycle_unpaused_ms = 0;
        break;
    case SC_WORLD_UNLOADED:
        if (is_enabled) {
            DEBUG(control,out).print("world unloaded; disabling %s\n",
                                    plugin_name);
            is_enabled = false;
            announcement_settings.reset(out, true);
            unit_history.reset();
        }
        break;
    default:
        break;
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    announcement_settings.on_update(out);

    if (config.auto_disengage && plotinfo->follow_unit < 0) {
        DEBUG(cycle,out).print("auto-disengage triggered\n");
        is_enabled = false;
        on_disable(out);
        return CR_OK;
    }

    if ((!config.auto_disengage && plotinfo->follow_unit < 0) || Core::getInstance().getUnpausedMs() >= next_cycle_unpaused_ms)
        follow_a_dwarf(out);
    return CR_OK;
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    bool show_help = false;
    if (!Lua::CallLuaModuleFunction(out, "plugins.spectate", "parse_commandline", std::make_tuple(parameters),
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

static bool is_in_combat(df::unit *unit) {
    return false;
}

static bool is_fleeing(df::unit *unit) {
    return false;
}

static void get_dwarf_buckets(color_ostream &out,
    vector<df::unit*> &active_combat_units,
    vector<df::unit*> &passive_combat_units,
    vector<df::unit*> &job_units,
    vector<df::unit*> &other_units)
{
    static const std::unordered_set<int32_t> boring_jobs = {
        df::job_type::Eat,
        df::job_type::Drink,
        df::job_type::Sleep,
    };

    for (auto unit : world->units.active) {
        if (Units::isDead(unit) || !Units::isActive(unit) || unit->flags1.bits.caged || unit->flags1.bits.chained || Units::isHidden(unit))
            continue;
        if (!config.include_animals && Units::isAnimal(unit))
            continue;
        if (!config.include_hostiles && Units::isDanger(unit))
            continue;
        if (!config.include_visitors && Units::isVisitor(unit))
            continue;
        if (!config.include_wildlife && Units::isWildlife(unit))
            continue;

        if (is_in_combat(unit)) {
            if (is_fleeing(unit))
                passive_combat_units.push_back(unit);
            else
                active_combat_units.push_back(unit);
        } else if (unit->job.current_job && !boring_jobs.contains(unit->job.current_job->job_type)) {
            job_units.push_back(unit);
        } else {
            other_units.push_back(unit);
        }
    }
}

static std::default_random_engine rng;

static uint32_t get_next_cycle_unpaused_ms(bool has_active_combat) {
    int32_t delay_ms = config.follow_ms;
    if (has_active_combat) {
        std::normal_distribution<float> distribution(config.follow_ms / 2, config.follow_ms / 6);
        int32_t delay_ms = distribution(rng);
        delay_ms = std::min(config.follow_ms, std::max(1, delay_ms));
    }
    return Core::getInstance().getUnpausedMs() + delay_ms;
}

static void add_bucket(const vector<df::unit*> &bucket, vector<df::unit*> &units, vector<float> &intervals, vector<float> &weights, float weight) {
    if (bucket.empty())
        return;
    intervals.push_back(units.size() + bucket.size());
    weights.push_back(weight);
    units.insert(units.end(), bucket.begin(), bucket.end());
}

#define DUMP_BUCKET(name) \
    DEBUG(cycle,out).print("bucket: " #name ", size: %zd\n", name.size()); \
    if (debug_cycle.isEnabled(DebugCategory::LTRACE)) { \
        for (auto u : name) { \
            DEBUG(cycle,out).print("  unit %d: %s\n", u->id, DF2CONSOLE(Units::getReadableName(u)).c_str()); \
        } \
    }

#define DUMP_FLOAT_VECTOR(name) \
    DEBUG(cycle,out).print(#name ":\n"); \
    for (float f : name) { \
        DEBUG(cycle,out).print("  %d\n", (int)f); \
    }

static const float ACTIVE_COMBAT_PREFERRED_WEIGHT = 25.0f;
static const float PASSIVE_COMBAT_PREFERRED_WEIGHT = 8.0f;
static const float JOB_WEIGHT = 3.0f;
static const float OTHER_WEIGHT = 1.0f;

static void follow_a_dwarf(color_ostream &out) {
    DEBUG(cycle,out).print("choosing a unit to follow\n");

    vector<df::unit*> active_combat_units;
    vector<df::unit*> passive_combat_units;
    vector<df::unit*> job_units;
    vector<df::unit*> other_units;
    get_dwarf_buckets(out, active_combat_units, passive_combat_units, job_units, other_units);

    next_cycle_unpaused_ms = get_next_cycle_unpaused_ms(!active_combat_units.empty());

    // coalesce the buckets and add weights
    vector<df::unit*> units;
    vector<float> intervals;
    vector<float> weights;
    intervals.push_back(0);
    add_bucket(active_combat_units, units, intervals, weights, config.prefer_conflict ? ACTIVE_COMBAT_PREFERRED_WEIGHT : JOB_WEIGHT);
    add_bucket(passive_combat_units, units, intervals, weights, config.prefer_conflict ? PASSIVE_COMBAT_PREFERRED_WEIGHT : JOB_WEIGHT);
    add_bucket(job_units, units, intervals, weights, JOB_WEIGHT);
    add_bucket(other_units, units, intervals, weights, OTHER_WEIGHT);

    if (units.empty()) {
        DEBUG(cycle,out).print("no units to follow\n");
        return;
    }

    std::piecewise_constant_distribution<float> distribution(intervals.begin(), intervals.end(), weights.begin());
    int unit_idx = distribution(rng);
    df::unit *unit = units[unit_idx];

    if (debug_cycle.isEnabled(DebugCategory::LDEBUG)) {
        DUMP_BUCKET(active_combat_units);
        DUMP_BUCKET(passive_combat_units);
        DUMP_BUCKET(job_units);
        DUMP_BUCKET(other_units);
        DUMP_FLOAT_VECTOR(intervals);
        DUMP_FLOAT_VECTOR(weights);
        DEBUG(cycle,out).print("selected unit idx %d\n", unit_idx);
    }

    unit_history.add(out, unit);
}

/////////////////////////////////////////////////////
// Lua API
//

static void spectate_setSetting(color_ostream &out, string name, int val) {
    DEBUG(control,out).print("entering spectate_setSetting %s = %d\n", name.c_str(), val);

    if (name == "auto-disengage") {
        config.auto_disengage = val;
    } else if (name == "auto-unpause") {
        if (val && !config.auto_unpause) {
            announcement_settings.save_and_scrub_settings(out);
        } else if (!val && config.auto_unpause) {
            announcement_settings.restore_settings(out);
        }
        config.auto_unpause = val;
    } else if (name == "cinematic-action") {
        config.cinematic_action = val;
    } else if (name == "include-animals") {
        config.include_animals = val;
    } else if (name == "include-hostiles") {
        config.include_hostiles = val;
    } else if (name == "include-visitors") {
        config.include_visitors = val;
    } else if (name == "include-wildlife") {
        config.include_wildlife = val;
    } else if (name == "prefer-conflict") {
        config.prefer_conflict = val;
    } else if (name == "prefer-new-arrivals") {
        config.prefer_new_arrivals = val;
    } else if (name == "follow-seconds") {
        if (val <= 0) {
            WARN(control,out).print("follow-seconds must be a positive integer\n");
            return;
        }
        config.follow_ms = val * 1000;
    } else {
        WARN(control,out).print("Unknown setting: %s\n", name.c_str());
    }
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(spectate_setSetting),
    DFHACK_LUA_END
};
