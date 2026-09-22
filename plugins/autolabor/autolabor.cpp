/*
 * autolabor plugin: unified labor management for Dwarf Fortress.
 *
 * One plugin binary exposes two commands:
 *
 *   autolabor     legacy mode: controls the labor matrix directly, bypassing
 *                 the work detail system (the pre-v50 behavior).
 *   labormanager  modern mode: expresses labor policy through DF's work
 *                 details so the game's job auction does the assignment.
 *
 * A third mode, monitor, only watches the job board for starving postings
 * and performs no labor management.
 *
 * The active mode is persisted per fort and can be switched from either
 * command ("<cmd> mode legacy|modern"), from the work details overlay, or
 * implicitly by enabling through one of the commands.
 */

#include "engines.h"
#include "joblabormapper.h"
#include "laborcommon.h"
#include "legacyengine.h"
#include "modernengine.h"
#include "monitorengine.h"
#include "workdetails.h"

#include "Core.h"
#include "LuaTools.h"
#include "PluginLua.h"
#include "PluginManager.h"

#include "modules/World.h"

using namespace DFHack;
using namespace df::enums;
using namespace autolabor;

DFHACK_PLUGIN("autolabor");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

command_result autolabor_cmd(color_ostream &out, std::vector<std::string> &params);
command_result labormanager_cmd(color_ostream &out, std::vector<std::string> &params);

static WorkDetailManager wdm;
static LegacyEngine legacy_engine(&wdm);
static ModernEngine modern_engine(&wdm);
static MonitorEngine monitor_engine;
static LaborEngine *active_engine = NULL;

static LaborEngine *mode_engine(int mode)
{
    if (mode == MODE_MODERN)
        return &modern_engine;
    if (mode == MODE_MONITOR)
        return &monitor_engine;
    return &legacy_engine;
}

// switch the running engine to the given mode; safe when disabled
static void activate_mode(color_ostream &out, int mode)
{
    store_engine_mode(mode);
    LaborEngine *want = mode_engine(mode);
    if (active_engine == want)
        return;
    if (active_engine)
        active_engine->disable(out);
    active_engine = want;
    if (is_enabled)
        active_engine->enable(out);
}

static bool can_run(color_ostream &out)
{
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode())
    {
        out.printerr("Cannot run {} without a loaded fort.\n", plugin_name);
        return false;
    }
    return true;
}

DFhackCExport command_result plugin_init(color_ostream &out,
    std::vector<PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "autolabor",
        "Automatically manage dwarf labors (legacy labor matrix mode).",
        autolabor_cmd));
    commands.push_back(PluginCommand(
        "labormanager",
        "Automatically manage dwarf labors (work detail mode).",
        labormanager_cmd));

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out)
{
    if (active_engine && is_enabled && Core::getInstance().isMapLoaded())
        active_engine->disable(out);
    active_engine = NULL;
    return CR_OK;
}

DFhackCExport command_result plugin_load_site_data(color_ostream &out)
{
    load_plugin_config();
    wdm.reset();
    init_labor_to_skill();

    is_enabled = config_flag(CF_ENABLED);
    active_engine = NULL;
    if (is_enabled)
        activate_mode(out, engine_mode());

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out,
    state_change_event event)
{
    if (event == SC_MAP_UNLOADED)
    {
        legacy_engine.map_unload();
        modern_engine.map_unload();
        monitor_engine.map_unload();
        active_engine = NULL;
        // is_enabled stays set; CF_ENABLED persists the user's intent and
        // plugin_load_site_data will re-enable the engine on the next map
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
    if (is_enabled && active_engine)
        active_engine->update(out);
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    if (!can_run(out))
        return CR_FAILURE;

    if (enable && !is_enabled)
    {
        set_config_flag(CF_ENABLED, true);
        is_enabled = true;
        activate_mode(out, engine_mode());
    }
    else if (!enable && is_enabled)
    {
        set_config_flag(CF_ENABLED, false);
        is_enabled = false;
        if (active_engine)
            active_engine->disable(out);
        active_engine = NULL;
    }

    return CR_OK;
}

// handle "<cmd> mode" / "<cmd> mode legacy|modern"; returns true if handled
static bool mode_command(color_ostream &out, std::vector<std::string> &parameters)
{
    if (parameters.empty() || parameters[0] != "mode")
        return false;

    if (parameters.size() == 1)
    {
        out << "Mode: "
            << (engine_mode() == MODE_MODERN ? "modern (labormanager)" :
                engine_mode() == MODE_MONITOR ? "monitor (starvation warnings)" :
                "legacy (autolabor)")
            << std::endl;
        return true;
    }

    int want = -1;
    if (parameters.size() == 2)
    {
        if (parameters[1] == "legacy")
            want = MODE_LEGACY;
        else if (parameters[1] == "modern")
            want = MODE_MODERN;
        else if (parameters[1] == "monitor")
            want = MODE_MONITOR;
    }

    if (want == -1)
    {
        out.printerr("Syntax: <cmd> mode [legacy|modern|monitor]\n");
        return true;
    }

    if (!can_run(out))
        return true;

    activate_mode(out, want);
    out << "Mode: " << parameters[1] << std::endl;
    return true;
}

command_result autolabor_cmd(color_ostream &out, std::vector<std::string> &parameters)
{
    if (!can_run(out))
        return CR_FAILURE;

    if (mode_command(out, parameters))
        return CR_OK;

    if (parameters.size() == 1 &&
        (parameters[0] == "0" || parameters[0] == "enable" ||
         parameters[0] == "1" || parameters[0] == "disable"))
    {
        bool enable = parameters[0] == "1" || parameters[0] == "enable";
        if (enable)
        {
            store_engine_mode(MODE_LEGACY);
            if (is_enabled)
                activate_mode(out, MODE_LEGACY);
        }
        return plugin_enable(out, enable);
    }

    if (legacy_engine.command(out, parameters))
        return CR_OK;

    out.print("Automatically assigns labors to dwarves by writing the labor\n"
        "matrix directly (legacy mode). Activate with 'autolabor enable',\n"
        "deactivate with 'autolabor disable'.\n"
        "Current state: {}, mode: {}.\n", (int)is_enabled,
        engine_mode() == MODE_MODERN ? "modern" : "legacy");
    return CR_OK;
}

command_result labormanager_cmd(color_ostream &out, std::vector<std::string> &parameters)
{
    if (!can_run(out))
        return CR_FAILURE;

    if (mode_command(out, parameters))
        return CR_OK;

    if (parameters.size() == 1 &&
        (parameters[0] == "0" || parameters[0] == "enable" ||
         parameters[0] == "1" || parameters[0] == "disable"))
    {
        bool enable = parameters[0] == "1" || parameters[0] == "enable";
        if (enable)
        {
            store_engine_mode(MODE_MODERN);
            if (is_enabled)
                activate_mode(out, MODE_MODERN);
        }
        return plugin_enable(out, enable);
    }

    // in monitor mode the monitor engine handles status/list; everything
    // else (balance, labor config, ...) falls through to the modern dialect
    if (engine_mode() == MODE_MONITOR && monitor_engine.command(out, parameters))
        return CR_OK;

    if (modern_engine.command(out, parameters))
        return CR_OK;

    out.print("Automatically assigns labors to dwarves through the work detail\n"
        "system (modern mode). Activate with 'labormanager enable',\n"
        "deactivate with 'labormanager disable'.\n"
        "Current state: {}, mode: {}.\n", (int)is_enabled,
        engine_mode() == MODE_MODERN ? "modern" : "legacy");
    return CR_OK;
}

// ---------------------------------------------------------------------------
// Lua API for the work details overlay (plugins.autolabor.*)
// ---------------------------------------------------------------------------

static int autolabor_getMode(lua_State *L)
{
    Lua::Push(L, engine_mode());
    return 1;
}

static int autolabor_getStatus(lua_State *L)
{
    std::string status;
    if (is_enabled && active_engine == &modern_engine)
        status = modern_engine.status_line();
    Lua::Push(L, status);
    return 1;
}

static int autolabor_getBalanceStops(lua_State *L)
{
    for (int i = 0; i < NUM_BALANCE_STOPS; i++)
        Lua::Push(L, balance_stop_name(i));
    return NUM_BALANCE_STOPS;
}

static int autolabor_getBalance(lua_State *L)
{
    Lua::Push(L, balance());
    return 1;
}

static void autolabor_setBalance(color_ostream &out, int32_t stop)
{
    if (can_run(out))
        store_balance(stop);
}

static int autolabor_getIdleReserve(lua_State *L)
{
    Lua::Push(L, idle_reserve());
    return 1;
}

// starving job postings on the board; only tracked while the modern or
// monitor engine is running. returns the count plus, when nonzero, the
// caption, age (ticks), and map position of the longest-starving posting
static int autolabor_getStarvingJobs(lua_State *L)
{
    int count = 0;
    std::string name;
    int32_t wait = 0;
    df::coord pos;
    if (is_enabled && active_engine == &modern_engine)
    {
        count = modern_engine.starving_jobs();
        name = modern_engine.oldest_starving_name();
        wait = modern_engine.oldest_starving_wait();
        pos = modern_engine.oldest_starving_pos();
    }
    else if (is_enabled && active_engine == &monitor_engine)
    {
        count = monitor_engine.starving_jobs();
        name = monitor_engine.oldest_starving_name();
        wait = monitor_engine.oldest_starving_wait();
        pos = monitor_engine.oldest_starving_pos();
    }
    Lua::Push(L, count);
    if (count > 0)
    {
        Lua::Push(L, name);
        Lua::Push(L, (int32_t)wait);
        Lua::Push(L, (int32_t)pos.x);
        Lua::Push(L, (int32_t)pos.y);
        Lua::Push(L, (int32_t)pos.z);
        return 6;
    }
    return 1;
}

static void autolabor_setIdleReserve(color_ostream &out, int32_t pct)
{
    if (can_run(out))
        store_idle_reserve(pct);
}

// returns a list of unit_labor enum names that the engines' static labor
// data doesn't cover; used by the labor coverage test in scripts/test
static int autolabor_checkLaborCoverage(lua_State *L)
{
    init_labor_to_skill();
    std::vector<std::string> missing;
    legacy_labor_coverage(missing);
    modern_labor_coverage(missing);
    Lua::Push(L, missing);
    return 1;
}

// returns a list of job_type enum names with no entry in the job to labor
// table; used by the job coverage test in scripts/test
static int autolabor_checkJobCoverage(lua_State *L)
{
    std::vector<std::string> missing;
    JobLaborMapper mapper;
    mapper.job_coverage(missing);
    Lua::Push(L, missing);
    return 1;
}

static void autolabor_setMode(color_ostream &out, int32_t mode)
{
    if (!can_run(out))
        return;
    if (mode != MODE_LEGACY && mode != MODE_MODERN && mode != MODE_MONITOR)
        return;
    activate_mode(out, mode);
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(autolabor_setMode),
    DFHACK_LUA_FUNCTION(autolabor_setBalance),
    DFHACK_LUA_FUNCTION(autolabor_setIdleReserve),
    DFHACK_LUA_END
};

DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(autolabor_getMode),
    DFHACK_LUA_COMMAND(autolabor_getStatus),
    DFHACK_LUA_COMMAND(autolabor_getBalance),
    DFHACK_LUA_COMMAND(autolabor_getBalanceStops),
    DFHACK_LUA_COMMAND(autolabor_getIdleReserve),
    DFHACK_LUA_COMMAND(autolabor_getStarvingJobs),
    DFHACK_LUA_COMMAND(autolabor_checkLaborCoverage),
    DFHACK_LUA_COMMAND(autolabor_checkJobCoverage),
    DFHACK_LUA_END
};
