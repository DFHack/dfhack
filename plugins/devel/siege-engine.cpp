#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <Error.h>
#include <PluginManager.h>
#include <modules/Gui.h>
#include <modules/Screen.h>
#include <modules/Maps.h>
#include <modules/World.h>
#include <LuaTools.h>
#include <TileTypes.h>
#include <vector>
#include <cstdio>
#include <stack>
#include <string>
#include <cmath>
#include <string.h>

#include <VTableInterpose.h>
#include "df/graphic.h"
#include "df/building_siegeenginest.h"
#include "df/builtin_mats.h"
#include "df/world.h"
#include "df/buildings_other_id.h"
#include "df/job.h"
#include "df/building_drawbuffer.h"
#include "df/ui.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/ui_build_selector.h"
#include "df/flow_info.h"
#include "df/report.h"

#include "MiscUtils.h"

using std::vector;
using std::string;
using std::stack;
using namespace DFHack;
using namespace df::enums;

using df::global::gps;
using df::global::world;
using df::global::ui;
using df::global::ui_build_selector;

using Screen::Pen;

DFHACK_PLUGIN("siege-engine");

/*
 * Misc. utils
 */

typedef std::pair<df::coord, df::coord> coord_range;

static void set_range(coord_range *target, df::coord p1, df::coord p2)
{
    if (!p1.isValid() || !p2.isValid())
    {
        *target = coord_range();
    }
    else
    {
        target->first.x = std::min(p1.x, p2.x);
        target->first.y = std::min(p1.y, p2.y);
        target->first.z = std::min(p1.z, p2.z);
        target->second.x = std::max(p1.x, p2.x);
        target->second.y = std::max(p1.y, p2.y);
        target->second.z = std::max(p1.z, p2.z);
    }
}

static bool is_range_valid(const coord_range &target)
{
    return target.first.isValid() && target.second.isValid();
}

static bool is_in_range(const coord_range &target, df::coord pos)
{
    return target.first.isValid() && target.second.isValid() &&
           target.first.x <= pos.x && pos.x <= target.second.x &&
           target.first.y <= pos.y && pos.y <= target.second.y &&
           target.first.z <= pos.z && pos.z <= target.second.z;
}

static std::pair<int, int> get_engine_range(df::building_siegeenginest *bld)
{
    if (bld->type == siegeengine_type::Ballista)
        return std::make_pair(0, 200);
    else
        return std::make_pair(30, 100);
}

static void orient_engine(df::building_siegeenginest *bld, df::coord target)
{
    int dx = target.x - bld->centerx;
    int dy = target.y - bld->centery;

    if (abs(dx) > abs(dy))
        bld->facing = (dx > 0) ?
            df::building_siegeenginest::Right :
            df::building_siegeenginest::Left;
    else
        bld->facing = (dy > 0) ?
            df::building_siegeenginest::Down :
            df::building_siegeenginest::Up;
}

/*
 * Configuration management
 */

static bool enable_plugin();

struct EngineInfo {
    int id;
    coord_range target;

    bool hasTarget() { return is_range_valid(target); }
    bool onTarget(df::coord pos) { return is_in_range(target, pos); }
};

static std::map<df::building*, EngineInfo> engines;

static EngineInfo *find_engine(df::building *bld, bool create = false)
{
    if (!bld)
        return NULL;

    auto it = engines.find(bld);
    if (it != engines.end())
        return &it->second;
    if (!create)
        return NULL;

    auto *obj = &engines[bld];
    obj->id = bld->id;
    return obj;
}

static void load_engines()
{
    engines.clear();

    auto pworld = Core::getInstance().getWorld();
    std::vector<PersistentDataItem> vec;

    pworld->GetPersistentData(&vec, "siege-engine/target/", true);
    for (auto it = vec.begin(); it != vec.end(); ++it)
    {
        auto engine = find_engine(df::building::find(it->ival(0)), true);
        if (!engine) continue;
        engine->target.first = df::coord(it->ival(1), it->ival(2), it->ival(3));
        engine->target.second = df::coord(it->ival(4), it->ival(5), it->ival(6));
    }
}

static int getTargetArea(lua_State *L)
{
    auto bld = Lua::CheckDFObject<df::building>(L, 1);
    if (!bld) luaL_argerror(L, 1, "null building");
    auto engine = find_engine(bld);

    if (engine && engine->hasTarget())
    {
        Lua::Push(L, engine->target.first);
        Lua::Push(L, engine->target.second);
    }
    else
    {
        lua_pushnil(L);
        lua_pushnil(L);
    }

    return 2;
}

static void clearTargetArea(df::building_siegeenginest *bld)
{
    CHECK_NULL_POINTER(bld);

    if (auto engine = find_engine(bld))
        engine->target = coord_range();

    auto pworld = Core::getInstance().getWorld();
    auto key = stl_sprintf("siege-engine/target/%d", bld->id);
    pworld->DeletePersistentData(pworld->GetPersistentData(key));
}

static bool setTargetArea(df::building_siegeenginest *bld, df::coord target_min, df::coord target_max)
{
    CHECK_NULL_POINTER(bld);
    CHECK_INVALID_ARGUMENT(target_min.isValid() && target_max.isValid());

    if (!enable_plugin())
        return false;

    auto pworld = Core::getInstance().getWorld();
    auto key = stl_sprintf("siege-engine/target/%d", bld->id);
    auto entry = pworld->GetPersistentData(key, NULL);
    if (!entry.isValid())
        return false;

    auto engine = find_engine(bld, true);

    set_range(&engine->target, target_min, target_max);

    entry.ival(0) = bld->id;
    entry.ival(1) = engine->target.first.x;
    entry.ival(2) = engine->target.first.y;
    entry.ival(3) = engine->target.first.z;
    entry.ival(4) = engine->target.second.x;
    entry.ival(5) = engine->target.second.y;
    entry.ival(6) = engine->target.second.z;

    df::coord sum = target_min + target_max;
    orient_engine(bld, df::coord(sum.x/2, sum.y/2, sum.z/2));

    return true;
}

/*
 * Trajectory
 */

struct ProjectilePath {
    df::coord origin, target;
    int divisor;
    df::coord speed, direction;

    ProjectilePath(df::coord origin, df::coord target) :
        origin(origin), target(target)
    {
        speed = target - origin;
        divisor = std::max(abs(speed.x), std::max(abs(speed.y), abs(speed.z)));
        if (divisor <= 0) divisor = 1;
        direction = df::coord(speed.x>=0?1:-1,speed.y>=0?1:-1,speed.z>=0?1:-1);
    }

    df::coord operator[] (int i) const {
        int div2 = divisor * 2;
        int bias = divisor-1;
        return origin + df::coord(
            (2*speed.x*i + direction.x*bias)/div2,
            (2*speed.y*i + direction.y*bias)/div2,
            (2*speed.z*i + direction.z*bias)/div2
        );
    }
};

static bool isPassableTile(df::coord pos)
{
    auto ptile = Maps::getTileType(pos);
    return !ptile || FlowPassable(*ptile);
}

struct PathMetrics {
    enum CollisionType {
        Impassable,
        Floor,
        Ceiling,
        MapEdge
    } hit_type;

    int collision_step;
    int goal_step, goal_z_step;
    std::vector<df::coord> coords;

    bool hits() { return goal_step != -1 && collision_step > goal_step; }

    PathMetrics(const ProjectilePath &path, df::coord goal, bool list_coords = false)
    {
        coords.clear();
        collision_step = goal_step = goal_z_step = -1;

        int step = 0;
        df::coord prev_pos = path.origin;
        if (list_coords)
            coords.push_back(prev_pos);

        for (;;) {
            df::coord cur_pos = path[++step];
            if (cur_pos == prev_pos)
                break;
            if (list_coords)
                coords.push_back(cur_pos);

            if (cur_pos.z == goal.z)
            {
                if (goal_z_step == -1)
                    goal_z_step = step;
                if (cur_pos == goal)
                    goal_step = step;
            }

            if (!Maps::isValidTilePos(cur_pos))
            {
                hit_type = PathMetrics::MapEdge;
                break;
            }

            if (!isPassableTile(cur_pos))
            {
                hit_type = Impassable;
                break;
            }

            if (cur_pos.z != prev_pos.z)
            {
                int top_z = std::max(prev_pos.z, cur_pos.z);
                auto ptile = Maps::getTileType(cur_pos.x, cur_pos.y, top_z);

                if (ptile && !LowPassable(*ptile))
                {
                    hit_type = (cur_pos.z > prev_pos.z ? Ceiling : Floor);
                    break;
                }
            }

            prev_pos = cur_pos;
        }

        collision_step = step;
    }
};

static std::string getTileStatus(df::building_siegeenginest *bld, df::coord tile_pos)
{
    df::coord origin(bld->centerx, bld->centery, bld->z);
    auto fire_range = get_engine_range(bld);

    ProjectilePath path(origin, tile_pos);
    PathMetrics raytrace(path, tile_pos);

     if (raytrace.hits())
    {
        if (raytrace.goal_step >= fire_range.first &&
            raytrace.goal_step <= fire_range.second)
            return "ok";
        else
            return "out_of_range";
    }
    else
        return "blocked";
}

static void paintAimScreen(df::building_siegeenginest *bld, df::coord view, df::coord2d ltop, df::coord2d size)
{
    CHECK_NULL_POINTER(bld);

    df::coord origin(bld->centerx, bld->centery, bld->z);
    coord_range building_rect(
        df::coord(bld->x1, bld->y1, bld->z),
        df::coord(bld->x2, bld->y2, bld->z)
    );

    auto engine = find_engine(bld);
    auto fire_range = get_engine_range(bld);

    for (int x = 0; x < size.x; x++)
    {
        for (int y = 0; y < size.y; y++)
        {
            df::coord tile_pos = view + df::coord(x,y,0);
            if (is_in_range(building_rect, tile_pos))
                continue;

            Pen cur_tile = Screen::readTile(ltop.x+x, ltop.y+y);
            if (!cur_tile.valid())
                continue;

            ProjectilePath path(origin, tile_pos);
            PathMetrics raytrace(path, tile_pos);

            int color;
            if (raytrace.hits())
            {
                if (raytrace.goal_step >= fire_range.first &&
                    raytrace.goal_step <= fire_range.second)
                    color = COLOR_GREEN;
                else
                    color = COLOR_CYAN;
            }
            else
                color = COLOR_RED;

            if (cur_tile.fg && cur_tile.ch != ' ')
            {
                cur_tile.fg = color;
                cur_tile.bg = 0;
            }
            else
            {
                cur_tile.fg = 0;
                cur_tile.bg = color;
            }

            cur_tile.bold = (engine && engine->onTarget(tile_pos));

            if (cur_tile.tile)
                cur_tile.tile_mode = Pen::CharColor;

            Screen::paintTile(cur_tile, ltop.x+x, ltop.y+y);
        }
    }
}

/*
 * Initialization
 */

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(clearTargetArea),
    DFHACK_LUA_FUNCTION(setTargetArea),
    DFHACK_LUA_FUNCTION(getTileStatus),
    DFHACK_LUA_FUNCTION(paintAimScreen),
    DFHACK_LUA_END
};

DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(getTargetArea),
    DFHACK_LUA_END
};

static bool is_enabled = false;

static void enable_hooks(bool enable)
{
    is_enabled = enable;

    if (enable)
        load_engines();
}

static bool enable_plugin()
{
    if (is_enabled)
        return true;

    auto pworld = Core::getInstance().getWorld();
    auto entry = pworld->GetPersistentData("siege-engine/enabled", NULL);
    if (!entry.isValid())
        return false;

    enable_hooks(true);
    return true;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        {
            auto pworld = Core::getInstance().getWorld();
            bool enable = pworld->GetPersistentData("siege-engine/enabled").isValid();

            if (enable)
            {
                out.print("Enabling the siege engine plugin.\n");
                enable_hooks(true);
            }
            else
                enable_hooks(false);
        }
        break;
    case SC_MAP_UNLOADED:
        enable_hooks(false);
        break;
    default:
        break;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (Core::getInstance().isMapLoaded())
        plugin_onstatechange(out, SC_MAP_LOADED);

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    enable_hooks(false);
    return CR_OK;
}
