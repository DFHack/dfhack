#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <Error.h>
#include <PluginManager.h>
#include <modules/Gui.h>
#include <modules/Screen.h>
#include <modules/Maps.h>
#include <modules/World.h>
#include <modules/Units.h>
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
#include "df/proj_itemst.h"
#include "df/unit.h"
#include "df/unit_soul.h"
#include "df/unit_skill.h"
#include "df/physical_attribute_type.h"
#include "df/creature_raw.h"
#include "df/caste_raw.h"
#include "df/caste_raw_flags.h"
#include "df/assumed_identity.h"

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

static int random_int(int val)
{
    return int(int64_t(rand())*val/RAND_MAX);
}

/*
 * Configuration management
 */

static bool enable_plugin();

struct EngineInfo {
    int id;
    coord_range target;
    df::coord center;

    bool hasTarget() { return is_range_valid(target); }
    bool onTarget(df::coord pos) { return is_in_range(target, pos); }
    df::coord getTargetSize() { return target.second - target.first; }
};

static std::map<df::building*, EngineInfo> engines;
static std::map<df::coord, df::building*> coord_engines;

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
    obj->center = df::coord(bld->centerx, bld->centery, bld->z);

    coord_engines[obj->center] = bld;
    return obj;
}

static EngineInfo *find_engine(df::coord pos)
{
    return find_engine(coord_engines[pos]);
}

static void clear_engines()
{
    engines.clear();
    coord_engines.clear();
}

static void load_engines()
{
    clear_engines();

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
    df::coord origin, goal, target, fudge_delta;
    int divisor, fudge_factor;
    df::coord speed, direction;

    ProjectilePath(df::coord origin, df::coord goal) :
        origin(origin), goal(goal), target(goal), fudge_factor(1)
    {
        fudge_delta = df::coord(0,0,0);
        calc_line();
    }

    void fudge(int factor, df::coord delta)
    {
        fudge_factor = factor;
        fudge_delta = delta;
        auto diff = goal - origin;
        diff.x *= fudge_factor;
        diff.y *= fudge_factor;
        diff.z *= fudge_factor;
        target = origin + diff + fudge_delta;
        calc_line();
    }

    void calc_line()
    {
        speed = target - origin;
        divisor = std::max(abs(speed.x), std::max(abs(speed.y), abs(speed.z)));
        if (divisor <= 0) divisor = 1;
        direction = df::coord(speed.x>=0?1:-1,speed.y>=0?1:-1,speed.z>=0?1:-1);
    }

    df::coord operator[] (int i) const
    {
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

    bool hits() { return collision_step > goal_step; }

    PathMetrics(const ProjectilePath &path, bool list_coords = false)
    {
        coords.clear();
        collision_step = goal_step = goal_z_step = 1000000;

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

            if (cur_pos.z == path.goal.z)
            {
                goal_z_step = std::min(step, goal_z_step);
                if (cur_pos == path.goal)
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

struct AimContext {
    df::building_siegeenginest *bld;
    df::coord origin;
    coord_range building_rect;
    EngineInfo *engine;
    std::pair<int, int> fire_range;

    AimContext(df::building_siegeenginest *bld, EngineInfo *engine)
        : bld(bld), engine(engine)
    {
        origin = df::coord(bld->centerx, bld->centery, bld->z);
        building_rect = coord_range(
            df::coord(bld->x1, bld->y1, bld->z),
            df::coord(bld->x2, bld->y2, bld->z)
        );
        fire_range = get_engine_range(bld);
    }

    bool isInRange(const PathMetrics &raytrace)
    {
        return raytrace.goal_step >= fire_range.first &&
               raytrace.goal_step <= fire_range.second;
    }

    bool adjustToPassable(df::coord *pos)
    {
        if (isPassableTile(*pos))
            return true;

        for (df::coord fudge = *pos;
             fudge.z < engine->target.second.z; fudge.z++)
        {
            if (!isPassableTile(fudge))
                continue;
            *pos = fudge;
            return true;
        }

        for (df::coord fudge = *pos;
             fudge.z > engine->target.first.z; fudge.z--)
        {
            if (!isPassableTile(fudge))
                continue;
            *pos = fudge;
            return true;
        }

        return false;
    }

};

static std::string getTileStatus(df::building_siegeenginest *bld, df::coord tile_pos)
{
    AimContext context(bld, NULL);

    ProjectilePath path(context.origin, tile_pos);
    PathMetrics raytrace(path);

     if (raytrace.hits())
    {
        if (context.isInRange(raytrace))
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

    AimContext context(bld, find_engine(bld));

    for (int x = 0; x < size.x; x++)
    {
        for (int y = 0; y < size.y; y++)
        {
            df::coord tile_pos = view + df::coord(x,y,0);
            if (is_in_range(context.building_rect, tile_pos))
                continue;

            Pen cur_tile = Screen::readTile(ltop.x+x, ltop.y+y);
            if (!cur_tile.valid())
                continue;

            ProjectilePath path(context.origin, tile_pos);
            PathMetrics raytrace(path);

            int color;
            if (raytrace.hits())
            {
                if (context.isInRange(raytrace))
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

            cur_tile.bold = (context.engine && context.engine->onTarget(tile_pos));

            if (cur_tile.tile)
                cur_tile.tile_mode = Pen::CharColor;

            Screen::paintTile(cur_tile, ltop.x+x, ltop.y+y);
        }
    }
}

/*
 * Unit tracking
 */

static int getAttrValue(const df::unit_attribute &attr)
{
    return std::max(0, attr.value - attr.soft_demotion);
}

static int getPhysAttrValue(df::unit *unit, df::physical_attribute_type attr, bool hide_curse)
{
    int value = getAttrValue(unit->body.physical_attrs[attr]);

    if (auto mod = unit->curse.attr_change)
    {
        int mvalue = (value * mod->phys_att_perc[attr]) + mod->phys_att_add[attr];

        if (hide_curse)
            value = std::min(value, mvalue);
        else
            value = mvalue;
    }

    return value;
}

int getSpeedRating(df::unit *unit)
{
    using namespace df::enums::physical_attribute_type;

    // Base speed
    auto creature = df::creature_raw::find(unit->race);
    if (!creature)
        return 0;

    auto craw = vector_get(creature->caste, unit->caste);
    if (!craw)
        return 0;

    int speed = craw->misc.speed;

    if (unit->flags3.bits.ghostly)
        return speed;

    // Curse multiplier
    if (unit->curse.speed_mul_percent != 100)
    {
        speed *= 100;
        if (unit->curse.speed_mul_percent != 0)
            speed /= unit->curse.speed_mul_percent;
    }

    speed += unit->curse.speed_add;

    // Swimming
    if (unit->flags2.bits.swimming)
    {
        speed = craw->misc.swim_speed;
        if (unit->status2.liquid_type.bits.liquid_type == tile_liquid::Magma)
            speed *= 2;
        if (craw->flags.is_set(caste_raw_flags::SWIMS_LEARNED))
        {
            int skill = Units::getEffectiveSkill(unit, job_skill::SWIMMING);
            if (skill > 1)
                skill = skill * std::max(6, 21-skill) / 20;
        }
    }
    else
    {
        if (unit->status2.liquid_type.bits.liquid_type == tile_liquid::Water)
            speed += 150*unit->status2.liquid_depth;
        else if (unit->status2.liquid_type.bits.liquid_type == tile_liquid::Magma)
            speed += 300*unit->status2.liquid_depth;
    }

    // General counters
    if (unit->profession == profession::BABY)
        speed += 3000;

    if (unit->counters2.exhaustion >= 2000)
    {
        speed += 200;
        if (unit->counters2.exhaustion >= 4000)
        {
            speed += 200;
            if (unit->counters2.exhaustion >= 6000)
                speed += 200;
        }
    }

    if (unit->flags2.bits.gutted) speed += 2000;

    if (unit->counters.soldier_mood == df::unit::T_counters::None)
    {
        if (unit->counters.nausea > 0) speed += 1000;
        if (unit->counters.winded > 0) speed += 1000;
        if (unit->counters.stunned > 0) speed += 1000;
        if (unit->counters.dizziness > 0) speed += 1000;
        if (unit->counters2.fever > 0) speed += 1000;
    }

    if (unit->counters.soldier_mood != df::unit::T_counters::MartialTrance)
    {
        if (unit->counters.pain >= 100 && unit->mood < 0)
            speed += 1000;
    }

    // TODO: bloodsucker

    if (unit->counters2.thirst_timer >= 50000) speed += 100;
    if (unit->counters2.hunger_timer >= 75000) speed += 100;
    if (unit->counters2.sleepiness_timer >= 150000) speed += 200;
    else if (unit->counters2.sleepiness_timer >= 57600) speed += 100;

    if (unit->relations.draggee_id != -1) speed += 1000;

    if (unit->flags1.bits.on_ground)
        speed += 2000;
    else if (unit->flags3.bits.on_crutch)
    {
        int skill = Units::getEffectiveSkill(unit, job_skill::CRUTCH_WALK);
        speed += 2000 - 100*std::min(20, skill);
    }

    // TODO: hidden_in_ambush

    if (unsigned(unit->counters2.paralysis-1) <= 98)
        speed += unit->counters2.paralysis*10;
    if (unsigned(unit->counters.webbed-1) <= 8)
        speed += unit->counters.webbed*100;

    // Muscle weight vs agility
    auto &attr_unk3 = unit->body.physical_attr_unk3;
    int strength = attr_unk3[STRENGTH];
    int agility = attr_unk3[AGILITY];
    speed = std::max(speed*3/4, std::min(speed*3/2, int(int64_t(speed)*strength/agility)));

    // Attributes
    bool hide_curse = false;
    if (!unit->job.hunt_target)
    {
        auto identity = Units::getIdentity(unit);
        if (identity && identity->unk_4c == 0)
            hide_curse = true;
    }

    int strength_attr = getPhysAttrValue(unit, STRENGTH, hide_curse);
    int agility_attr = getPhysAttrValue(unit, AGILITY, hide_curse);
    int total_attr = std::max(200, std::min(3800, strength_attr + agility_attr));

    speed = ((total_attr-200)*(speed*3/2) + (3800-total_attr)*(speed/2))/4800; // ??

    if (!unit->flags1.bits.on_ground && unit->status2.able_stand > 2)
    {
        // WTF
        int as = unit->status2.able_stand;
        int x = (as-1) - (as>>1);
        int y = as - unit->status2.able_stand_impair;
        if (unit->flags3.bits.on_crutch) y--;
        y = y * 500 / x;
        if (y > 0) speed += y;
    }

    if (unit->mood == mood_type::Melancholy) speed += 8000;

    // Inventory encumberance
    int armor_skill = Units::getEffectiveSkill(unit, job_skill::ARMOR);
    armor_skill = std::min(15, armor_skill);

    int inv_weight = 0, inv_weight_fraction = 0;

    for (size_t i = 0; i < unit->inventory.size(); i++)
    {
        auto item = unit->inventory[i]->item;
        if (!item->flags.bits.weight_computed)
            continue;

        int wval = item->weight;
        int wfval = item->weight_fraction;

        auto mode = unit->inventory[i]->mode;
        if ((mode == df::unit_inventory_item::Worn ||
             mode == df::unit_inventory_item::WrappedAround) &&
             item->isArmor() && armor_skill > 1)
        {
            wval = wval * (15 - armor_skill) / 16;
            wfval = wfval * (15 - armor_skill) / 16;
        }

        inv_weight += wval;
        inv_weight_fraction += wfval;
    }

    int total_weight = inv_weight*100 + inv_weight_fraction/10000;
    int free_weight = std::max(1, attr_unk3[STRENGTH]/10 + strength_attr*3);

    if (free_weight < total_weight)
    {
        int delta = (total_weight - free_weight)/10;
        delta = std::min(5000, delta); // dwarfmode only
        speed += delta;
    }

    // skipped: unknown loop on inventory items that amounts to 0 change

    return std::min(10000, std::max(0, speed));
}

/*
 * Projectile hook
 */

struct projectile_hook : df::proj_itemst {
    typedef df::proj_itemst interpose_base;

    void aimAtPoint(AimContext &context, ProjectilePath &path, bool bad_shot = false)
    {
        target_pos = path.target;

        PathMetrics raytrace(path);

        // Materialize map blocks, or the projectile will crash into them
        for (int i = 0; i < raytrace.collision_step; i++)
            Maps::ensureTileBlock(path[i]);

        if (flags.bits.piercing)
        {
            if (bad_shot)
                fall_threshold = std::min(raytrace.goal_z_step, raytrace.collision_step);
        }
        else
        {
            if (bad_shot)
                fall_threshold = context.fire_range.second;
            else
                fall_threshold = raytrace.goal_step;
        }

        fall_threshold = std::max(fall_threshold, context.fire_range.first);
        fall_threshold = std::min(fall_threshold, context.fire_range.second);
    }

    void aimAtArea(AimContext &context)
    {
        df::coord target, last_passable;
        df::coord tbase = context.engine->target.first;
        df::coord tsize = context.engine->getTargetSize();
        bool success = false;

        for (int i = 0; i < 50; i++)
        {
            target = tbase + df::coord(
                random_int(tsize.x), random_int(tsize.y), random_int(tsize.z)
            );

            if (context.adjustToPassable(&target))
                last_passable = target;
            else
                continue;

            ProjectilePath path(context.origin, target);
            PathMetrics raytrace(path);

            if (raytrace.hits() && context.isInRange(raytrace))
            {
                aimAtPoint(context, path);
                return;
            }
        }

        if (!last_passable.isValid())
            last_passable = target;

        ProjectilePath path(context.origin, last_passable);
        aimAtPoint(context, path, true);
    }

    void doCheckMovement()
    {
        if (distance_flown != 0 || fall_counter != fall_delay)
            return;

        auto engine = find_engine(origin_pos);
        if (!engine || !engine->hasTarget())
            return;

        auto bld0 = df::building::find(engine->id);
        auto bld = strict_virtual_cast<df::building_siegeenginest>(bld0);
        if (!bld)
            return;

        AimContext context(bld, engine);

        aimAtArea(context);
    }

    DEFINE_VMETHOD_INTERPOSE(bool, checkMovement, ())
    {
        if (flags.bits.high_flying || flags.bits.piercing)
            doCheckMovement();

        return INTERPOSE_NEXT(checkMovement)();
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(projectile_hook, checkMovement);

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

    INTERPOSE_HOOK(projectile_hook, checkMovement).apply(enable);

    if (enable)
        load_engines();
    else
        clear_engines();
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
