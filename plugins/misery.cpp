#include "Core.h"
#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"

#include "modules/Persistence.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/emotion_type.h"
#include "df/plotinfost.h"
#include "df/unit.h"
#include "df/unit_personality.h"
#include "df/unit_soul.h"
#include "df/unit_thought_type.h"
#include "df/world.h"

using std::string;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("misery");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(cur_year);
REQUIRE_GLOBAL(cur_year_tick);
REQUIRE_GLOBAL(world);

namespace DFHack {
    DBG_DECLARE(misery, control, DebugCategory::LINFO);
    DBG_DECLARE(misery, cycle, DebugCategory::LINFO);
}

static const string CONFIG_KEY = string(plugin_name) + "/config";
static PersistentDataItem config;

enum ConfigValues {
    CONFIG_IS_ENABLED = 0,
    CONFIG_FACTOR = 1,
};

static const int32_t CYCLE_TICKS = 1229; // one day
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

static command_result do_command(color_ostream &out, vector<string> &parameters);
static void do_cycle(color_ostream &out);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    DEBUG(control,out).print("initializing %s\n", plugin_name);

    // provide a configuration interface for the plugin
    commands.push_back(PluginCommand(
        plugin_name,
        "Make citizens more miserable.",
        do_command));

    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (!Core::getInstance().isMapLoaded()) {
        out.printerr("Cannot enable %s without a loaded map.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(control,out).print("%s from the API; persisting\n",
                                is_enabled ? "enabled" : "disabled");
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
        if (enable)
            do_cycle(out);
    } else {
        DEBUG(control,out).print("%s from the API, but already %s; no action\n",
                                is_enabled ? "enabled" : "disabled",
                                is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    DEBUG(control,out).print("shutting down %s\n", plugin_name);

    return CR_OK;
}

DFhackCExport command_result plugin_load_site_data (color_ostream &out) {
    cycle_timestamp = 0;
    config = World::GetPersistentSiteData(CONFIG_KEY);

    if (!config.isValid()) {
        DEBUG(control,out).print("no config found in this save; initializing\n");
        config = World::AddPersistentSiteData(CONFIG_KEY);
        config.set_bool(CONFIG_IS_ENABLED, is_enabled);
        config.set_int(CONFIG_FACTOR, 2);
    }

    is_enabled = config.get_bool(CONFIG_IS_ENABLED);
    DEBUG(control,out).print("loading persisted enabled state: %s\n",
                            is_enabled ? "true" : "false");

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == DFHack::SC_WORLD_UNLOADED) {
        if (is_enabled) {
            DEBUG(control,out).print("world unloaded; disabling %s\n",
                                    plugin_name);
            is_enabled = false;
        }
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (is_enabled && world->frame_counter - cycle_timestamp >= CYCLE_TICKS)
        do_cycle(out);
    return CR_OK;
}

static bool call_misery_lua(color_ostream *out, const char *fn_name,
        int nargs = 0, int nres = 0,
        Lua::LuaLambda && args_lambda = Lua::DEFAULT_LUA_LAMBDA,
        Lua::LuaLambda && res_lambda = Lua::DEFAULT_LUA_LAMBDA) {
    DEBUG(control).print("calling misery lua function: '%s'\n", fn_name);

    CoreSuspender guard;

    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!out)
        out = &Core::getInstance().getConsole();

    return Lua::CallLuaModuleFunction(*out, L, "plugins.misery", fn_name,
            nargs, nres,
            std::forward<Lua::LuaLambda&&>(args_lambda),
            std::forward<Lua::LuaLambda&&>(res_lambda));
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    CoreSuspender suspend;

    if (!Core::getInstance().isMapLoaded()) {
        out.printerr("Cannot run %s without a loaded map.\n", plugin_name);
        return CR_FAILURE;
    }

    bool show_help = false;
    if (!call_misery_lua(&out, "parse_commandline", parameters.size(), 1,
            [&](lua_State *L) {
                for (const string &param : parameters)
                    Lua::Push(L, param);
            },
            [&](lua_State *L) {
                show_help = !lua_toboolean(L, -1);
            })) {
        return CR_FAILURE;
    }

    return show_help ? CR_WRONG_USAGE : CR_OK;
}

/////////////////////////////////////////////////////
// cycle logic
//

const int FAKE_EMOTION_FLAG = (1 << 30);
const int STRENGTH_MULTIPLIER = 100;

typedef df::unit_personality::T_emotions Emotion;

static bool is_fake_emotion(Emotion *e) {
    return e->flags.whole & FAKE_EMOTION_FLAG;
}

static void clear_misery(df::unit *unit) {
    if (!unit || !unit->status.current_soul)
        return;
    auto &emotions = unit->status.current_soul->personality.emotions;
    auto it = std::remove_if(emotions.begin(), emotions.end(), [](Emotion *e) {
        if (is_fake_emotion(e)) {
            delete e;
            return true;
        }
        return false;
    });
    emotions.erase(it, emotions.end());
}
// clears fake negative thoughts then runs the given lambda
static void affect_units(
        std::function<void(df::unit *)> &&process_unit = [](df::unit *){}) {
    for (auto unit : world->units.active) {
        if (!Units::isCitizen(unit) || !unit->status.current_soul)
            continue;

        clear_misery(unit);
        std::forward<std::function<void(df::unit *)> &&>(process_unit)(unit);
    }
}

static void do_cycle(color_ostream &out) {
    // mark that we have recently run
    cycle_timestamp = world->frame_counter;

    DEBUG(cycle,out).print("running %s cycle\n", plugin_name);

    int strength = STRENGTH_MULTIPLIER * config.get_int(CONFIG_FACTOR);

    affect_units([&](df::unit *unit) {
        Emotion *e = new Emotion;
        e->type = df::emotion_type::MISERY;
        e->thought = df::unit_thought_type::SoapyBath;
        e->flags.whole |= FAKE_EMOTION_FLAG;
        e->year = *cur_year;
        e->year_tick = *cur_year_tick;
        e->strength = strength;
        e->severity = strength;
        unit->status.current_soul->personality.emotions.push_back(e);
    });
}

/////////////////////////////////////////////////////
// Lua API
//

static void misery_clear(color_ostream &out) {
    DEBUG(control,out).print("entering misery_clear\n");
    affect_units();
}

static void misery_setFactor(color_ostream &out, int32_t factor) {
    DEBUG(control,out).print("entering misery_setFactor\n");
    if (1 >= factor) {
        out.printerr("factor must be at least 2\n");
        return;
    }
    config.set_int(CONFIG_FACTOR, factor);
    if (is_enabled)
        do_cycle(out);
}

static int misery_getFactor(color_ostream &out) {
    DEBUG(control,out).print("entering tailor_getFactor\n");
    return config.get_int(CONFIG_FACTOR);
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(misery_clear),
    DFHACK_LUA_FUNCTION(misery_setFactor),
    DFHACK_LUA_FUNCTION(misery_getFactor),
    DFHACK_LUA_END
};
