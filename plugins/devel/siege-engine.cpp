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
 * Configuration management
 */

static bool enable_plugin();

struct EngineInfo {
    int id;
    df::coord target_min, target_max;

    bool hasTarget() {
        return target_min.isValid() && target_max.isValid();
    }
    bool onTarget(df::coord pos) {
        return hasTarget() &&
               target_min.x <= pos.x && pos.x <= target_max.x &&
               target_min.y <= pos.y && pos.y <= target_max.y &&
               target_min.z <= pos.z && pos.z <= target_max.z;
    }
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
        engine->target_min = df::coord(it->ival(1), it->ival(2), it->ival(3));
        engine->target_max = df::coord(it->ival(4), it->ival(5), it->ival(6));
    }
}

static int getTargetArea(lua_State *L)
{
    auto bld = Lua::CheckDFObject<df::building>(L, 1);
    if (!bld) luaL_argerror(L, 1, "null building");
    auto engine = find_engine(bld);

    if (engine && engine->target_min.isValid())
    {
        Lua::Push(L, engine->target_min);
        Lua::Push(L, engine->target_max);
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
        engine->target_min = engine->target_max = df::coord();

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

    entry.ival(0) = bld->id;
    entry.ival(1) = engine->target_min.x = std::min(target_min.x, target_max.x);
    entry.ival(2) = engine->target_min.y = std::min(target_min.y, target_max.y);
    entry.ival(3) = engine->target_min.z = std::min(target_min.z, target_max.z);
    entry.ival(4) = engine->target_max.x = std::max(target_min.x, target_max.x);
    entry.ival(5) = engine->target_max.y = std::max(target_min.y, target_max.y);
    entry.ival(6) = engine->target_max.z = std::max(target_min.z, target_max.z);

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

void paintAimScreen(df::building_siegeenginest *bld, df::coord view, df::coord2d ltop, df::coord2d size)
{
    CHECK_NULL_POINTER(bld);

    df::coord origin = df::coord(bld->centerx, bld->centery, bld->z);

    auto engine = find_engine(bld);
    int min_distance, max_distance;

    if (bld->type == siegeengine_type::Ballista)
    {
        min_distance = 0;
        max_distance = 200;
    }
    else
    {
        min_distance = 30;
        max_distance = 100;
    }

    df::coord cursor = Gui::getCursorPos();

    for (int x = 0; x < size.x; x++)
    {
        for (int y = 0; y < size.y; y++)
        {
            df::coord tile_pos = view + df::coord(x,y,0);
            if (tile_pos == cursor)
                continue;
            if (tile_pos.z == bld->z &&
                tile_pos.x >= bld->x1 && tile_pos.x <= bld->x2 &&
                tile_pos.y >= bld->y1 && tile_pos.y <= bld->y2)
                continue;

            Pen cur_tile = Screen::readTile(ltop.x+x, ltop.y+y);
            if (!cur_tile.valid())
                continue;

            ProjectilePath path(origin, tile_pos);

            if (path.speed.z != 0 && abs(path.speed.z) != path.divisor) {
                path.divisor *= 20;
                path.speed.x *= 20;
                path.speed.y *= 20;
                path.speed.z *= 20;
                path.speed.z += 9;
            }

            PathMetrics raytrace(path, tile_pos);

            int color;
            if (raytrace.hits())
            {
                if (raytrace.goal_step >= min_distance &&
                    raytrace.goal_step <= max_distance)
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
