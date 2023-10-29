#include "Core.h"
#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "TileTypes.h"

#include "modules/Burrows.h"
#include "modules/Persistence.h"
#include "modules/World.h"

#include "df/burrow.h"
#include "df/tile_designation.h"
#include "df/world.h"

using std::vector;
using std::string;
using namespace DFHack;

DFHACK_PLUGIN("burrow");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(window_z);
REQUIRE_GLOBAL(world);

// logging levels can be dynamically controlled with the `debugfilter` command.
namespace DFHack {
    // for configuration-related logging
    DBG_DECLARE(burrow, status, DebugCategory::LINFO);
    // for logging during the periodic scan
    DBG_DECLARE(burrow, cycle, DebugCategory::LINFO);
}

static const auto CONFIG_KEY = std::string(plugin_name) + "/config";
static PersistentDataItem config;
enum ConfigValues {
    CONFIG_IS_ENABLED = 0,
};
static int get_config_val(int index) {
    if (!config.isValid())
        return -1;
    return config.ival(index);
}
static bool get_config_bool(int index) {
    return get_config_val(index) == 1;
}
static void set_config_val(int index, int value) {
    if (config.isValid())
        config.ival(index) = value;
}
static void set_config_bool(int index, bool value) {
    set_config_val(index, value ? 1 : 0);
}

static const int32_t CYCLE_TICKS = 100;
static int32_t cycle_timestamp = 0; // world->frame_counter at last cycle

static command_result do_command(color_ostream &out, vector<string> &parameters);
static void do_cycle(color_ostream &out);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector<PluginCommand> &commands) {
    DEBUG(status, out).print("initializing %s\n", plugin_name);
    commands.push_back(
        PluginCommand("burrow",
                      "Quickly adjust burrow tiles and units.",
                      do_command));
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("Cannot enable %s without a loaded world.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(status, out).print("%s from the API; persisting\n", is_enabled ? "enabled" : "disabled");
        set_config_bool(CONFIG_IS_ENABLED, is_enabled);
        if (enable)
            do_cycle(out);
    }
    else {
        DEBUG(status, out).print("%s from the API, but already %s; no action\n", is_enabled ? "enabled" : "disabled", is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    DEBUG(status, out).print("shutting down %s\n", plugin_name);
    return CR_OK;
}

DFhackCExport command_result plugin_load_data(color_ostream &out) {
    cycle_timestamp = 0;
    config = World::GetPersistentData(CONFIG_KEY);

    if (!config.isValid()) {
        DEBUG(status, out).print("no config found in this save; initializing\n");
        config = World::AddPersistentData(CONFIG_KEY);
        set_config_bool(CONFIG_IS_ENABLED, is_enabled);
    }

    // we have to copy our enabled flag into the global plugin variable, but
    // all the other state we can directly read/modify from the persistent
    // data structure.
    is_enabled = get_config_bool(CONFIG_IS_ENABLED);
    DEBUG(status, out).print("loading persisted enabled state: %s\n", is_enabled ? "true" : "false");
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == DFHack::SC_WORLD_UNLOADED) {
        if (is_enabled) {
            DEBUG(status, out).print("world unloaded; disabling %s\n", plugin_name);
            is_enabled = false;
        }
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    CoreSuspender suspend;
    if (is_enabled && world->frame_counter - cycle_timestamp >= CYCLE_TICKS)
        do_cycle(out);
    return CR_OK;
}

static bool call_burrow_lua(color_ostream *out, const char *fn_name,
        int nargs = 0, int nres = 0,
        Lua::LuaLambda && args_lambda = Lua::DEFAULT_LUA_LAMBDA,
        Lua::LuaLambda && res_lambda = Lua::DEFAULT_LUA_LAMBDA) {
    DEBUG(status).print("calling %s lua function: '%s'\n", plugin_name, fn_name);

    CoreSuspender guard;

    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!out)
        out = &Core::getInstance().getConsole();

    return Lua::CallLuaModuleFunction(*out, L, "plugins.burrow", fn_name,
            nargs, nres,
            std::forward<Lua::LuaLambda&&>(args_lambda),
            std::forward<Lua::LuaLambda&&>(res_lambda));
}

static command_result do_command(color_ostream &out, vector<string> &parameters) {
    CoreSuspender suspend;

    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("Cannot run %s without a loaded world.\n", plugin_name);
        return CR_FAILURE;
    }

    bool show_help = false;
    if (!call_burrow_lua(&out, "parse_commandline", parameters.size(), 1,
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

static void do_cycle(color_ostream &out)
{
    // mark that we have recently run
    cycle_timestamp = world->frame_counter;

    // TODO
}

/////////////////////////////////////////////////////
// Lua API
//

static void get_bool_field(lua_State *L, int idx, const char *name, bool *dest) {
    lua_getfield(L, idx, name);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return;
    }
    *dest = lua_toboolean(L, -1);
    lua_pop(L, 1);
}

static void get_opts(lua_State *L, int idx, bool &dry_run, bool &zlevel) {
    if (lua_gettop(L) < idx)
        return;
    get_bool_field(L, idx, "dry_run", &dry_run);
    get_bool_field(L, idx, "zlevel", &zlevel);
}

static bool get_int_field(lua_State *L, int idx, const char *name, int16_t *dest) {
    lua_getfield(L, idx, name);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return false;
    }
    *dest = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return true;
}

static bool get_bounds(lua_State *L, int idx, df::coord &pos1, df::coord &pos2) {
    return get_int_field(L, idx, "x1", &pos1.x) &&
        get_int_field(L, idx, "y1", &pos1.y) &&
        get_int_field(L, idx, "z1", &pos1.z) &&
        get_int_field(L, idx, "x2", &pos2.x) &&
        get_int_field(L, idx, "y2", &pos2.y) &&
        get_int_field(L, idx, "z2", &pos2.z);
}

static int burrow_tiles_clear(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_tiles_clear\n");
    // TODO
    return 0;
}

static int burrow_tiles_set(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_tiles_set\n");
    // TODO
    return 0;
}

static int burrow_tiles_add(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_tiles_add\n");
    // TODO
    return 0;
}

static int burrow_tiles_remove(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_tiles_remove\n");
    // TODO
    return 0;
}

static df::burrow* get_burrow(lua_State *L, int idx) {
    df::burrow *burrow = NULL;
    if (lua_isuserdata(L, idx))
        burrow = Lua::GetDFObject<df::burrow>(L, idx);
    else if (lua_isstring(L, idx))
        burrow = Burrows::findByName(luaL_checkstring(L, idx));
    else if (lua_isinteger(L, idx))
        burrow = df::burrow::find(luaL_checkinteger(L, idx));
    return burrow;
}

static int box_fill(lua_State *L, bool enable) {
    df::coord pos_start, pos_end;
    bool dry_run = false, zlevel = false;

    df::burrow *burrow = get_burrow(L, 1);
    if (!burrow) {
        luaL_argerror(L, 1, "invalid burrow specifier or burrow not found");
        return 0;
    }

    if (!get_bounds(L, 2, pos_start, pos_end)) {
        luaL_argerror(L, 2, "invalid box bounds");
        return 0;
    }
    get_opts(L, 3, dry_run, zlevel);

    if (zlevel) {
        pos_start.z = *window_z;
        pos_end.z = *window_z;
    }

    int32_t count = 0;
    for (int32_t z = pos_start.z; z <= pos_end.z; ++z) {
        for (int32_t y = pos_start.y; y <= pos_end.y; ++y) {
            for (int32_t x = pos_start.x; x <= pos_end.x; ++x) {
                df::coord pos(x, y, z);
                if (enable != Burrows::isAssignedTile(burrow, pos))
                    ++count;
                if (!dry_run)
                    Burrows::setAssignedTile(burrow, pos, enable);
            }
        }
    }

    Lua::Push(L, count);
    return 1;
}

static int burrow_tiles_box_add(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_tiles_box_add\n");
    return box_fill(L, true);
}

static int burrow_tiles_box_remove(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_tiles_box_remove\n");
    return box_fill(L, false);
}

static int flood_fill(lua_State *L, bool enable) {
    df::coord start_pos;
    bool dry_run = false, zlevel = false;

    df::burrow *burrow = get_burrow(L, 1);
    if (!burrow) {
        luaL_argerror(L, 1, "invalid burrow specifier or burrow not found");
        return 0;
    }

    Lua::CheckDFAssign(L, &start_pos, 2);
    get_opts(L, 3, dry_run, zlevel);

    // record properties to match
    df::tile_designation *start_des = Maps::getTileDesignation(start_pos);
    if (!start_des) {
        luaL_argerror(L, 2, "invalid starting coordinates");
        return 0;
    }
    uint16_t start_walk = Maps::getWalkableGroup(start_pos);

    int32_t count = 0;

    std::stack<df::coord> flood;
    flood.emplace(start_pos);

    while(!flood.empty()) {
        const df::coord pos = flood.top();
        flood.pop();

        df::tile_designation *des = Maps::getTileDesignation(pos);
        if(!des ||
            des->bits.outside != start_des->bits.outside ||
            des->bits.hidden != start_des->bits.hidden)
        {
            continue;
        }

        if (!start_walk && Maps::getWalkableGroup(pos))
            continue;

        if (pos != start_pos && enable == Burrows::isAssignedTile(burrow, pos))
            continue;

        ++count;
        if (!dry_run)
            Burrows::setAssignedTile(burrow, pos, enable);

        // only go one tile outside of a walkability group
        if (start_walk && start_walk != Maps::getWalkableGroup(pos))
            continue;

        flood.emplace(pos.x-1, pos.y-1, pos.z);
        flood.emplace(pos.x,   pos.y-1, pos.z);
        flood.emplace(pos.x+1, pos.y-1, pos.z);
        flood.emplace(pos.x-1, pos.y,   pos.z);
        flood.emplace(pos.x+1, pos.y,   pos.z);
        flood.emplace(pos.x-1, pos.y+1, pos.z);
        flood.emplace(pos.x,   pos.y+1, pos.z);
        flood.emplace(pos.x+1, pos.y+1, pos.z);

        if (!zlevel) {
            df::coord pos_above(pos);
            ++pos_above.z;
            df::tiletype *tt = Maps::getTileType(pos);
            df::tiletype *tt_above = Maps::getTileType(pos_above);
            if (tt_above && LowPassable(*tt_above))
                flood.emplace(pos_above);
            if (tt && LowPassable(*tt))
                flood.emplace(pos.x, pos.y, pos.z-1);
        }
    }

    Lua::Push(L, count);
    return 1;
}

static int burrow_tiles_flood_add(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_tiles_flood_add\n");
    return flood_fill(L, true);
}

static int burrow_tiles_flood_remove(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_tiles_flood_remove\n");
    return flood_fill(L, false);
}

static int burrow_units_clear(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_units_clear\n");
    // TODO
    return 0;
}

static int burrow_units_set(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_units_set\n");
    // TODO
    return 0;
}

static int burrow_units_add(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_units_add\n");
    // TODO
    return 0;
}

static int burrow_units_remove(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_units_remove\n");
    // TODO
    return 0;
}

DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(burrow_tiles_clear),
    DFHACK_LUA_COMMAND(burrow_tiles_set),
    DFHACK_LUA_COMMAND(burrow_tiles_add),
    DFHACK_LUA_COMMAND(burrow_tiles_remove),
    DFHACK_LUA_COMMAND(burrow_tiles_box_add),
    DFHACK_LUA_COMMAND(burrow_tiles_box_remove),
    DFHACK_LUA_COMMAND(burrow_tiles_flood_add),
    DFHACK_LUA_COMMAND(burrow_tiles_flood_remove),
    DFHACK_LUA_COMMAND(burrow_units_clear),
    DFHACK_LUA_COMMAND(burrow_units_set),
    DFHACK_LUA_COMMAND(burrow_units_add),
    DFHACK_LUA_COMMAND(burrow_units_remove),
    DFHACK_LUA_END
};










/*


#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "Error.h"

#include "DataFuncs.h"
#include "LuaTools.h"

#include "modules/Gui.h"
#include "modules/Job.h"
#include "modules/Maps.h"
#include "modules/MapCache.h"
#include "modules/World.h"
#include "modules/Units.h"
#include "TileTypes.h"

#include "DataDefs.h"
#include "df/plotinfost.h"
#include "df/world.h"
#include "df/unit.h"
#include "df/burrow.h"
#include "df/map_block.h"
#include "df/block_burrow.h"
#include "df/job.h"
#include "df/job_list_link.h"

#include "MiscUtils.h"

#include <stdlib.h>

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;
using namespace dfproto;

DFHACK_PLUGIN("burrow");
REQUIRE_GLOBAL(plotinfo);
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(gamemode);

static void init_map(color_ostream &out);
static void deinit_map(color_ostream &out);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{

    if (Core::getInstance().isMapLoaded())
        init_map(out);

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    deinit_map(out);

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        deinit_map(out);
        if (gamemode &&
            *gamemode == game_mode::DWARF)
            init_map(out);
        break;
    case SC_MAP_UNLOADED:
        deinit_map(out);
        break;
    default:
        break;
    }

    return CR_OK;
}

static int name_burrow_id = -1;

static void handle_burrow_rename(color_ostream &out, df::burrow *burrow);

DEFINE_LUA_EVENT_1(onBurrowRename, handle_burrow_rename, df::burrow*);

static void detect_burrow_renames(color_ostream &out)
{
    if (plotinfo->main.mode == ui_sidebar_mode::Burrows &&
        plotinfo->burrows.in_edit_name_mode &&
        plotinfo->burrows.sel_id >= 0)
    {
        name_burrow_id = plotinfo->burrows.sel_id;
    }
    else if (name_burrow_id >= 0)
    {
        auto burrow = df::burrow::find(name_burrow_id);
        name_burrow_id = -1;
        if (burrow)
            onBurrowRename(out, burrow);
    }
}

struct DigJob {
    int id;
    df::job_type job;
    df::coord pos;
    df::tiletype old_tile;
};

static int next_job_id_save = 0;
static std::map<int,DigJob> diggers;

static void handle_dig_complete(color_ostream &out, df::job_type job, df::coord pos,
                                df::tiletype old_tile, df::tiletype new_tile, df::unit *worker);

DEFINE_LUA_EVENT_5(onDigComplete, handle_dig_complete,
                   df::job_type, df::coord, df::tiletype, df::tiletype, df::unit*);

static void detect_digging(color_ostream &out)
{
    for (auto it = diggers.begin(); it != diggers.end();)
    {
        auto worker = df::unit::find(it->first);

        if (!worker || !worker->job.current_job ||
            worker->job.current_job->id != it->second.id)
        {
            //out.print("Dig job %d expired.\n", it->second.id);

            df::coord pos = it->second.pos;

            if (auto block = Maps::getTileBlock(pos))
            {
                df::tiletype new_tile = block->tiletype[pos.x&15][pos.y&15];

                //out.print("Tile %d -> %d\n", it->second.old_tile, new_tile);

                if (new_tile != it->second.old_tile)
                {
                    onDigComplete(out, it->second.job, pos, it->second.old_tile, new_tile, worker);
                }
            }

            auto cur = it; ++it; diggers.erase(cur);
        }
        else
            ++it;
    }

    std::vector<df::job*> jvec;

    if (Job::listNewlyCreated(&jvec, &next_job_id_save))
    {
        for (size_t i = 0; i < jvec.size(); i++)
        {
            auto job = jvec[i];
            auto type = ENUM_ATTR(job_type, type, job->job_type);
            if (type != job_type_class::Digging)
                continue;

            auto worker = Job::getWorker(job);
            if (!worker)
                continue;

            df::coord pos = job->pos;
            auto block = Maps::getTileBlock(pos);
            if (!block)
                continue;

            auto &info = diggers[worker->id];

            //out.print("New dig job %d.\n", job->id);

            info.id = job->id;
            info.job = job->job_type;
            info.pos = pos;
            info.old_tile = block->tiletype[pos.x&15][pos.y&15];
        }
    }
}

DFHACK_PLUGIN_IS_ENABLED(active);

static bool auto_grow = false;
static std::vector<int> grow_burrows;

DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
    if (!active)
        return CR_OK;

    detect_burrow_renames(out);

    if (auto_grow)
        detect_digging(out);

    return CR_OK;
}

static std::map<std::string,int> name_lookup;

static void parse_names()
{
    auto &list = plotinfo->burrows.list;

    grow_burrows.clear();
    name_lookup.clear();

    for (size_t i = 0; i < list.size(); i++)
    {
        auto burrow = list[i];

        std::string name = burrow->name;

        if (!name.empty())
        {
            name_lookup[name] = burrow->id;

            if (name[name.size()-1] == '+')
            {
                grow_burrows.push_back(burrow->id);
                name.resize(name.size()-1);
            }

            if (!name.empty())
                name_lookup[name] = burrow->id;
        }
    }
}

static void reset_tracking()
{
    diggers.clear();
    next_job_id_save = 0;
}

static void init_map(color_ostream &out)
{
    auto config = World::GetPersistentData("burrows/config");
    if (config.isValid())
    {
        auto_grow = !!(config.ival(0) & 1);
    }

    parse_names();
    name_burrow_id = -1;

    reset_tracking();
    active = true;

    if (auto_grow && !grow_burrows.empty())
        out.print("Auto-growing %zu burrows.\n", grow_burrows.size());
}

static void deinit_map(color_ostream &out)
{
    active = false;
    auto_grow = false;
    reset_tracking();
}

static PersistentDataItem create_config(color_ostream &out)
{
    bool created;
    auto rv = World::GetPersistentData("burrows/config", &created);
    if (created && rv.isValid())
        rv.ival(0) = 0;
    if (!rv.isValid())
        out.printerr("Could not write configuration.");
    return rv;
}

static void enable_auto_grow(color_ostream &out, bool enable)
{
    if (enable == auto_grow)
        return;

    auto config = create_config(out);
    if (!config.isValid())
        return;

    if (enable)
        config.ival(0) |= 1;
    else
        config.ival(0) &= ~1;

    auto_grow = enable;

    if (enable)
        reset_tracking();
}

static void handle_burrow_rename(color_ostream &out, df::burrow *burrow)
{
    parse_names();
}

static void add_to_burrows(std::vector<df::burrow*> &burrows, df::coord pos)
{
    for (size_t i = 0; i < burrows.size(); i++)
        Burrows::setAssignedTile(burrows[i], pos, true);
}

static void add_walls_to_burrows(color_ostream &out, std::vector<df::burrow*> &burrows,
                                MapExtras::MapCache &mc, df::coord pos1, df::coord pos2)
{
    for (int x = pos1.x; x <= pos2.x; x++)
    {
        for (int y = pos1.y; y <= pos2.y; y++)
        {
            for (int z = pos1.z; z <= pos2.z; z++)
            {
                df::coord pos(x,y,z);

                auto tile = mc.tiletypeAt(pos);

                if (isWallTerrain(tile))
                    add_to_burrows(burrows, pos);
            }
        }
    }
}

static void handle_dig_complete(color_ostream &out, df::job_type job, df::coord pos,
                                df::tiletype old_tile, df::tiletype new_tile, df::unit *worker)
{
    if (!isWalkable(new_tile))
        return;

    std::vector<df::burrow*> to_grow;

    for (size_t i = 0; i < grow_burrows.size(); i++)
    {
        auto b = df::burrow::find(grow_burrows[i]);
        if (b && Burrows::isAssignedTile(b, pos))
            to_grow.push_back(b);
    }

    //out.print("%d to grow.\n", to_grow.size());

    if (to_grow.empty())
        return;

    MapExtras::MapCache mc;
    bool changed = false;

    if (!isWalkable(old_tile))
    {
        changed = true;
        add_walls_to_burrows(out, to_grow, mc, pos+df::coord(-1,-1,0), pos+df::coord(1,1,0));

        if (isWalkableUp(new_tile))
            add_to_burrows(to_grow, pos+df::coord(0,0,1));

        if (tileShape(new_tile) == tiletype_shape::RAMP)
        {
            add_walls_to_burrows(out, to_grow, mc,
                                 pos+df::coord(-1,-1,1), pos+df::coord(1,1,1));
        }
    }

    if (LowPassable(new_tile) && !LowPassable(old_tile))
    {
        changed = true;
        add_to_burrows(to_grow, pos-df::coord(0,0,1));

        if (tileShape(new_tile) == tiletype_shape::RAMP_TOP)
        {
            add_walls_to_burrows(out, to_grow, mc,
                                 pos+df::coord(-1,-1,-1), pos+df::coord(1,1,-1));
        }
    }

    if (changed && worker && !worker->job.current_job)
        Job::checkDesignationsNow();
}

static void renameBurrow(color_ostream &out, df::burrow *burrow, std::string name)
{
    CHECK_NULL_POINTER(burrow);

    // The event makes this absolutely necessary
    CoreSuspender suspend;

    burrow->name = name;
    onBurrowRename(out, burrow);
}

static df::burrow *findByName(color_ostream &out, std::string name, bool silent = false)
{
    int id = -1;
    if (name_lookup.count(name))
        id = name_lookup[name];
    auto rv = df::burrow::find(id);
    if (!rv && !silent)
        out.printerr("Burrow not found: '%s'\n", name.c_str());
    return rv;
}

static void copyUnits(df::burrow *target, df::burrow *source, bool enable)
{
    CHECK_NULL_POINTER(target);
    CHECK_NULL_POINTER(source);

    if (source == target)
    {
        if (!enable)
            Burrows::clearUnits(target);

        return;
    }

    for (size_t i = 0; i < source->units.size(); i++)
    {
        auto unit = df::unit::find(source->units[i]);

        if (unit)
            Burrows::setAssignedUnit(target, unit, enable);
    }
}

static void copyTiles(df::burrow *target, df::burrow *source, bool enable)
{
    CHECK_NULL_POINTER(target);
    CHECK_NULL_POINTER(source);

    if (source == target)
    {
        if (!enable)
            Burrows::clearTiles(target);

        return;
    }

    std::vector<df::map_block*> pvec;
    Burrows::listBlocks(&pvec, source);

    for (size_t i = 0; i < pvec.size(); i++)
    {
        auto block = pvec[i];
        auto smask = Burrows::getBlockMask(source, block);
        if (!smask)
            continue;

        auto tmask = Burrows::getBlockMask(target, block, enable);
        if (!tmask)
            continue;

        if (enable)
        {
            for (int j = 0; j < 16; j++)
                tmask->tile_bitmask[j] |= smask->tile_bitmask[j];
        }
        else
        {
            for (int j = 0; j < 16; j++)
                tmask->tile_bitmask[j] &= ~smask->tile_bitmask[j];

            if (!tmask->has_assignments())
                Burrows::deleteBlockMask(target, block, tmask);
        }
    }
}

static void setTilesByDesignation(df::burrow *target, df::tile_designation d_mask,
                                  df::tile_designation d_value, bool enable)
{
    CHECK_NULL_POINTER(target);

    auto &blocks = world->map.map_blocks;

    for (size_t i = 0; i < blocks.size(); i++)
    {
        auto block = blocks[i];
        df::block_burrow *mask = NULL;

        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                if ((block->designation[x][y].whole & d_mask.whole) != d_value.whole)
                    continue;

                if (!mask)
                    mask = Burrows::getBlockMask(target, block, enable);
                if (!mask)
                    goto next_block;

                mask->setassignment(x, y, enable);
            }
        }

        if (mask && !enable && !mask->has_assignments())
            Burrows::deleteBlockMask(target, block, mask);

    next_block:;
    }
}

static bool setTilesByKeyword(df::burrow *target, std::string name, bool enable)
{
    CHECK_NULL_POINTER(target);

    df::tile_designation mask;
    df::tile_designation value;

    if (name == "ABOVE_GROUND")
        mask.bits.subterranean = true;
    else if (name == "SUBTERRANEAN")
        mask.bits.subterranean = value.bits.subterranean = true;
    else if (name == "LIGHT")
        mask.bits.light = value.bits.light = true;
    else if (name == "DARK")
        mask.bits.light = true;
    else if (name == "OUTSIDE")
        mask.bits.outside = value.bits.outside = true;
    else if (name == "INSIDE")
        mask.bits.outside = true;
    else if (name == "HIDDEN")
        mask.bits.hidden = value.bits.hidden = true;
    else if (name == "REVEALED")
        mask.bits.hidden = true;
    else
        return false;

    setTilesByDesignation(target, mask, value, enable);
    return true;
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(renameBurrow),
    DFHACK_LUA_FUNCTION(findByName),
    DFHACK_LUA_FUNCTION(copyUnits),
    DFHACK_LUA_FUNCTION(copyTiles),
    DFHACK_LUA_FUNCTION(setTilesByKeyword),
    DFHACK_LUA_END
};

DFHACK_PLUGIN_LUA_EVENTS {
    DFHACK_LUA_EVENT(onBurrowRename),
    DFHACK_LUA_EVENT(onDigComplete),
    DFHACK_LUA_END
};

static command_result burrow(color_ostream &out, vector <string> &parameters)
{
    CoreSuspender suspend;

    if (!active)
    {
        out.printerr("The plugin cannot be used without map.\n");
        return CR_FAILURE;
    }

    string cmd;
    if (!parameters.empty())
        cmd = parameters[0];

    if (cmd == "enable" || cmd == "disable")
    {
        if (parameters.size() < 2)
            return CR_WRONG_USAGE;

        bool state = (cmd == "enable");

        for (size_t i = 1; i < parameters.size(); i++)
        {
            string &option = parameters[i];

            if (option == "auto-grow")
                enable_auto_grow(out, state);
            else
                return CR_WRONG_USAGE;
        }
    }
    else if (cmd == "clear-units")
    {
        if (parameters.size() < 2)
            return CR_WRONG_USAGE;

        for (size_t i = 1; i < parameters.size(); i++)
        {
            auto target = findByName(out, parameters[i]);
            if (!target)
                return CR_WRONG_USAGE;

            Burrows::clearUnits(target);
        }
    }
    else if (cmd == "set-units" || cmd == "add-units" || cmd == "remove-units")
    {
        if (parameters.size() < 3)
            return CR_WRONG_USAGE;

        auto target = findByName(out, parameters[1]);
        if (!target)
            return CR_WRONG_USAGE;

        if (cmd == "set-units")
            Burrows::clearUnits(target);

        bool enable = (cmd != "remove-units");

        for (size_t i = 2; i < parameters.size(); i++)
        {
            auto source = findByName(out, parameters[i]);
            if (!source)
                return CR_WRONG_USAGE;

            copyUnits(target, source, enable);
        }
    }
    else if (cmd == "clear-tiles")
    {
        if (parameters.size() < 2)
            return CR_WRONG_USAGE;

        for (size_t i = 1; i < parameters.size(); i++)
        {
            auto target = findByName(out, parameters[i]);
            if (!target)
                return CR_WRONG_USAGE;

            Burrows::clearTiles(target);
        }
    }
    else if (cmd == "set-tiles" || cmd == "add-tiles" || cmd == "remove-tiles")
    {
        if (parameters.size() < 3)
            return CR_WRONG_USAGE;

        auto target = findByName(out, parameters[1]);
        if (!target)
            return CR_WRONG_USAGE;

        if (cmd == "set-tiles")
            Burrows::clearTiles(target);

        bool enable = (cmd != "remove-tiles");

        for (size_t i = 2; i < parameters.size(); i++)
        {
            if (setTilesByKeyword(target, parameters[i], enable))
                continue;

            auto source = findByName(out, parameters[i]);
            if (!source)
                return CR_WRONG_USAGE;

            copyTiles(target, source, enable);
        }
    }
    else
    {
        if (!parameters.empty() && cmd != "?")
            out.printerr("Invalid command: %s\n", cmd.c_str());
        return CR_WRONG_USAGE;
    }

    return CR_OK;
}
*/
