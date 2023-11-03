#include "Core.h"
#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "TileTypes.h"

#include "modules/Burrows.h"
#include "modules/EventManager.h"
#include "modules/Job.h"
#include "modules/Persistence.h"
#include "modules/World.h"

#include "df/block_burrow.h"
#include "df/burrow.h"
#include "df/map_block.h"
#include "df/plotinfost.h"
#include "df/tile_designation.h"
#include "df/unit.h"
#include "df/world.h"

using std::vector;
using std::string;
using namespace DFHack;

DFHACK_PLUGIN("burrow");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(plotinfo);
REQUIRE_GLOBAL(window_z);
REQUIRE_GLOBAL(world);

// logging levels can be dynamically controlled with the `debugfilter` command.
namespace DFHack {
    // for configuration-related logging
    DBG_DECLARE(burrow, status, DebugCategory::LINFO);
    // for logging during the periodic scan
    DBG_DECLARE(burrow, event, DebugCategory::LINFO);
}

static std::unordered_map<df::coord, df::tiletype> active_dig_jobs;

static command_result do_command(color_ostream &out, vector<string> &parameters);
static void init_diggers(color_ostream& out);
static void jobStartedHandler(color_ostream& out, void* ptr);
static void jobCompletedHandler(color_ostream& out, void* ptr);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector<PluginCommand> &commands) {
    DEBUG(status, out).print("initializing %s\n", plugin_name);
    commands.push_back(
        PluginCommand("burrow",
                      "Quickly adjust burrow tiles and units.",
                      do_command));
    return CR_OK;
}

static void reset() {
    active_dig_jobs.clear();
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(status, out).print("%s from the API\n", is_enabled ? "enabled" : "disabled");
        reset();
        if (enable) {
            init_diggers(out);
            EventManager::registerListener(EventManager::EventType::JOB_STARTED, EventManager::EventHandler(jobStartedHandler, 0), plugin_self);
            EventManager::registerListener(EventManager::EventType::JOB_COMPLETED, EventManager::EventHandler(jobCompletedHandler, 0), plugin_self);
        }
    }
    else {
        DEBUG(status, out).print("%s from the API, but already %s; no action\n", is_enabled ? "enabled" : "disabled", is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    DEBUG(status, out).print("shutting down %s\n", plugin_name);
    reset();
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == DFHack::SC_WORLD_UNLOADED)
        reset();
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
// listener logic
//

static void init_diggers(color_ostream& out) {
    if (!Core::getInstance().isWorldLoaded()) {
        DEBUG(status, out).print("world not yet loaded; not scanning jobs\n");
        return;
    }

    std::vector<df::job*> pvec;
    int start_id = 0;
    if (Job::listNewlyCreated(&pvec, &start_id)) {
        for (auto job : pvec) {
            if (Job::getWorker(job))
                jobStartedHandler(out, job);
        }
    }
}

static void jobStartedHandler(color_ostream& out, void* ptr) {
    DEBUG(event, out).print("entering jobStartedHandler\n");

    df::job *job = (df::job *)ptr;
    auto type = ENUM_ATTR(job_type, type, job->job_type);
    if (type != job_type_class::Digging)
        return;

    const df::coord &pos = job->pos;
    DEBUG(event, out).print("dig job started: id=%d, pos=(%d,%d,%d), type=%s\n",
            job->id, pos.x, pos.y, pos.z, ENUM_KEY_STR(job_type, job->job_type).c_str());
    df::tiletype *tt = Maps::getTileType(pos);
    if (tt)
        active_dig_jobs[pos] = *tt;
}

static void add_walls_to_burrow(color_ostream &out, df::burrow* b,
    const df::coord & pos1, const df::coord & pos2)
{
    for (int z = pos1.z; z <= pos2.z; z++) {
        for (int y = pos1.y; y <= pos2.y; y++) {
            for (int x = pos1.x; x <= pos2.x; x++) {
                df::coord pos(x,y,z);
                df::tiletype *tt = Maps::getTileType(pos);
                if (tt && isWallTerrain(*tt))
                    Burrows::setAssignedTile(b, pos, true);
            }
        }
    }
}

static void expand_burrows(color_ostream &out, const df::coord & pos, df::tiletype prev_tt, df::tiletype tt) {
    if (!isWalkable(tt) && tileShape(tt) != tiletype_shape::RAMP_TOP)
        return;

    bool changed = false;
    for (auto b : plotinfo->burrows.list) {
        if (!b->name.ends_with('+') || !Burrows::isAssignedTile(b, pos))
            continue;

        if (!isWalkable(prev_tt)) {
            changed = true;
            add_walls_to_burrow(out, b, pos+df::coord(-1,-1,0), pos+df::coord(1,1,0));

            if (isWalkableUp(tt))
                Burrows::setAssignedTile(b, pos+df::coord(0,0,1), true);

            if (tileShape(tt) == tiletype_shape::RAMP)
                add_walls_to_burrow(out, b, pos+df::coord(-1,-1,1), pos+df::coord(1,1,1));
        }

        if (LowPassable(tt) && !LowPassable(prev_tt)) {
            changed = true;
            Burrows::setAssignedTile(b, pos-df::coord(0,0,1), true);
            if (tileShape(tt) == tiletype_shape::RAMP_TOP)
                add_walls_to_burrow(out, b, pos+df::coord(-1,-1,-1), pos+df::coord(1,1,-1));
        }
    }

    if (changed)
        Job::checkDesignationsNow();
}

static void jobCompletedHandler(color_ostream& out, void* ptr) {
    DEBUG(event, out).print("entering jobCompletedHandler\n");

    df::job *job = (df::job *)ptr;
    auto type = ENUM_ATTR(job_type, type, job->job_type);
    if (type != job_type_class::Digging)
        return;

    const df::coord &pos = job->pos;
    DEBUG(event, out).print("dig job completed: id=%d, pos=(%d,%d,%d), type=%s\n",
            job->id, pos.x, pos.y, pos.z, ENUM_KEY_STR(job_type, job->job_type).c_str());

    df::tiletype prev_tt = active_dig_jobs[pos];
    df::tiletype *tt = Maps::getTileType(pos);

    if (tt && *tt && *tt != prev_tt)
        expand_burrows(out, pos, prev_tt, *tt);

    active_dig_jobs.erase(pos);
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

static void get_opts(lua_State *L, int idx, bool &zlevel) {
    if (lua_gettop(L) < idx)
        return;
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

static df::burrow* get_burrow(lua_State *L, int idx) {
    df::burrow *burrow = NULL;
    if (lua_isuserdata(L, idx))
        burrow = Lua::GetDFObject<df::burrow>(L, idx);
    else if (lua_isstring(L, idx))
        burrow = Burrows::findByName(luaL_checkstring(L, idx), true);
    else if (lua_isinteger(L, idx))
        burrow = df::burrow::find(luaL_checkinteger(L, idx));
    return burrow;
}

static void copyTiles(df::burrow *target, df::burrow *source, bool enable) {
    CHECK_NULL_POINTER(target);
    CHECK_NULL_POINTER(source);

    if (source == target) {
        if (!enable)
            Burrows::clearTiles(target);
        return;
    }

    vector<df::map_block*> pvec;
    Burrows::listBlocks(&pvec, source);

    for (auto block : pvec) {
        auto smask = Burrows::getBlockMask(source, block);
        if (!smask)
            continue;

        auto tmask = Burrows::getBlockMask(target, block, enable);
        if (!tmask)
            continue;

        if (enable) {
            for (int j = 0; j < 16; j++)
                tmask->tile_bitmask[j] |= smask->tile_bitmask[j];
        } else {
            for (int j = 0; j < 16; j++)
                tmask->tile_bitmask[j] &= ~smask->tile_bitmask[j];

            if (!tmask->has_assignments())
                Burrows::deleteBlockMask(target, block, tmask);
        }
    }
}

static void setTilesByDesignation(df::burrow *target, df::tile_designation d_mask,
                                  df::tile_designation d_value, bool enable) {
    CHECK_NULL_POINTER(target);

    auto &blocks = world->map.map_blocks;

    for (auto block : blocks) {
        df::block_burrow *mask = NULL;

        for (int x = 0; x < 16; x++) {
            for (int y = 0; y < 16; y++) {
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

static bool setTilesByKeyword(df::burrow *target, std::string name, bool enable) {
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

static void copyUnits(df::burrow *target, df::burrow *source, bool enable) {
    CHECK_NULL_POINTER(target);
    CHECK_NULL_POINTER(source);

    if (source == target) {
        if (!enable)
            Burrows::clearUnits(target);
        return;
    }

    for (size_t i = 0; i < source->units.size(); i++) {
        if (auto unit = df::unit::find(source->units[i]))
            Burrows::setAssignedUnit(target, unit, enable);
    }
}

static int burrow_tiles_clear(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_tiles_clear\n");

    lua_pushnil(L);   // first key
    while (lua_next(L, 1)) {
        df::burrow * burrow = get_burrow(L, -1);
        if (burrow)
            Burrows::clearTiles(burrow);
        lua_pop(L, 1);  // remove value, leave key
    }

    return 0;
}

static void tiles_set_add_remove(lua_State *L, bool do_set, bool enable) {
    df::burrow *target = get_burrow(L, 1);
    if (!target) {
        luaL_argerror(L, 1, "invalid burrow specifier or burrow not found");
        return;
    }

    if (do_set)
        Burrows::clearTiles(target);

    lua_pushnil(L);   // first key
    while (lua_next(L, 2)) {
        if (!lua_isstring(L, -1) || !setTilesByKeyword(target, luaL_checkstring(L, -1), enable)) {
            if (auto burrow = get_burrow(L, -1))
                copyTiles(target, burrow, enable);
        }
        lua_pop(L, 1);  // remove value, leave key
    }
}

static int burrow_tiles_set(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_tiles_set\n");
    tiles_set_add_remove(L, true, true);
    return 0;
}

static int burrow_tiles_add(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_tiles_add\n");
    tiles_set_add_remove(L, false, true);
    return 0;
}

static int burrow_tiles_remove(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_tiles_remove\n");
    tiles_set_add_remove(L, false, false);
    return 0;
}

static void box_fill(lua_State *L, bool enable) {
    df::burrow *burrow = get_burrow(L, 1);
    if (!burrow) {
        luaL_argerror(L, 1, "invalid burrow specifier or burrow not found");
        return;
    }

    df::coord pos_start, pos_end;
    if (!get_bounds(L, 2, pos_start, pos_end)) {
        luaL_argerror(L, 2, "invalid box bounds");
        return;
    }

    for (int32_t z = pos_start.z; z <= pos_end.z; ++z) {
        for (int32_t y = pos_start.y; y <= pos_end.y; ++y) {
            for (int32_t x = pos_start.x; x <= pos_end.x; ++x) {
                df::coord pos(x, y, z);
                Burrows::setAssignedTile(burrow, pos, enable);
            }
        }
    }
}

static int burrow_tiles_box_add(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_tiles_box_add\n");
    box_fill(L, true);
    return 0;
}

static int burrow_tiles_box_remove(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_tiles_box_remove\n");
    box_fill(L, false);
    return 0;
}

// ramp tops inherit walkability group of the tile below
static uint16_t get_walk_group(const df::coord & pos) {
    uint16_t walk = Maps::getWalkableGroup(pos);
    if (walk)
        return walk;
    if (auto tt = Maps::getTileType(pos)) {
        if (tileShape(*tt) == df::tiletype_shape::RAMP_TOP) {
            df::coord pos_below(pos);
            --pos_below.z;
            walk = Maps::getWalkableGroup(pos_below);
        }
    }
    return walk;
}

static void flood_fill(lua_State *L, bool enable) {
    df::coord start_pos;
    bool zlevel = false;

    df::burrow *burrow = get_burrow(L, 1);
    if (!burrow) {
        luaL_argerror(L, 1, "invalid burrow specifier or burrow not found");
        return;
    }

    Lua::CheckDFAssign(L, &start_pos, 2);
    get_opts(L, 3, zlevel);

    df::tile_designation *start_des = Maps::getTileDesignation(start_pos);
    if (!start_des) {
        luaL_argerror(L, 2, "invalid starting coordinates");
        return;
    }
    uint16_t start_walk = Maps::getWalkableGroup(start_pos);

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

        uint16_t walk = get_walk_group(pos);
        if (!start_walk && walk)
            continue;

        if (pos != start_pos && enable == Burrows::isAssignedTile(burrow, pos))
            continue;

        Burrows::setAssignedTile(burrow, pos, enable);

        // only go one tile outside of a walkability group
        if (start_walk && start_walk != walk)
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
}

static int burrow_tiles_flood_add(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_tiles_flood_add\n");
    flood_fill(L, true);
    return 0;
}

static int burrow_tiles_flood_remove(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_tiles_flood_remove\n");
    flood_fill(L, false);
    return 0;
}

static int burrow_units_clear(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_units_clear\n");

    int32_t count = 0;
    lua_pushnil(L);   // first key
    while (lua_next(L, 1)) {
        df::burrow * burrow = get_burrow(L, -1);
        if (burrow) {
            count += burrow->units.size();
            Burrows::clearUnits(burrow);
        }
        lua_pop(L, 1);  // remove value, leave key
    }

    Lua::Push(L, count);
    return 1;
}

static void units_set_add_remove(lua_State *L, bool do_set, bool enable) {
    df::burrow *target = get_burrow(L, 1);
    if (!target) {
        luaL_argerror(L, 1, "invalid burrow specifier or burrow not found");
        return;
    }

    if (do_set)
        Burrows::clearUnits(target);

    lua_pushnil(L);   // first key
    while (lua_next(L, 2)) {
        if (auto burrow = get_burrow(L, -1))
            copyUnits(target, burrow, enable);
        lua_pop(L, 1);  // remove value, leave key
    }
}

static int burrow_units_set(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_units_set\n");
    units_set_add_remove(L, true, true);
    return 0;
}

static int burrow_units_add(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_units_add\n");
    units_set_add_remove(L, false, true);
    return 0;
}

static int burrow_units_remove(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    DEBUG(status,*out).print("entering burrow_units_remove\n");
    units_set_add_remove(L, false, false);
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
