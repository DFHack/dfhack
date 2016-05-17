
#include <Console.h>
#include "Core.h"
#include <Error.h>
#include <Export.h>
#include <PluginManager.h>
#include <LuaTools.h>
#include "MiscUtils.h"
#include <TileTypes.h>
#include <VTableInterpose.h>

#include "modules/EventManager.h"
#include <modules/Gui.h>
#include <modules/Job.h>
#include <modules/MapCache.h>
#include <modules/Maps.h>
#include <modules/Materials.h>
#include "modules/Persistent.h"
#include <modules/Random.h>
#include <modules/Screen.h>
#include <modules/Units.h>
#include <modules/World.h>

#include "df/building_drawbuffer.h"
#include "df/building_siegeenginest.h"
#include "df/building_stockpilest.h"
#include "df/buildings_other_id.h"
#include "df/builtin_mats.h"
#include "df/caste_raw.h"
#include "df/caste_raw_flags.h"
#include "df/creature_raw.h"
#include "df/flow_info.h"
#include "df/flow_type.h"
#include "df/game_mode.h"
#include "df/graphic.h"
#include "df/identity.h"
#include "df/invasion_info.h"
#include "df/item_actual.h"
#include "df/items_other_id.h"
#include "df/job.h"
#include "df/job_item.h"
#include "df/material.h"
#include "df/physical_attribute_type.h"
#include "df/proj_itemst.h"
#include "df/report.h"
#include "df/strain_type.h"
#include "df/stockpile_links.h"
#include "df/ui.h"
#include "df/ui_build_selector.h"
#include "df/unit.h"
#include "df/unit_misc_trait.h"
#include "df/unit_skill.h"
#include "df/unit_soul.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/workshop_profile.h"
#include "df/world.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stack>
#include <string>
#include <vector>
#include "jsoncpp.h"

using std::vector;
using std::string;
using std::stack;
using std::cerr;
using namespace DFHack;
using namespace df::enums;

using Screen::Pen;

DFHACK_PLUGIN("siege-engine");
REQUIRE_GLOBAL(gamemode);
REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(ui_build_selector);
REQUIRE_GLOBAL(process_jobs);

static const int32_t persist_version=1;
static void save_config(color_ostream& out);
static void load_config(color_ostream& out);
static void on_presave_callback(color_ostream& out, void* nothing) {
    save_config(out);
}

/*
    Aiming is simulated by using a normal distribution to perturb X and Y.

    The chance a normal distribution is within n*sigma of median is
    erf(n/sqrt(2)). For direct hit, it must be within 0.5 tiles of
    center, so it is erf(0.5/sigma/sqrt(2)) = erf(1/sigma/sqrt(8)).
    Since it must hit in both X and Y, it must be squared, so final
    is erf(1/sigma/sqrt(8))^2 = erf(skill*coeff/(distance*sqrt(8)))^2.

    The chance to hit a RxR area is erf(skill*coeff*R/(distance*sqrt(8)))^2.

    Catapults can fire between 30 and 100, and the projectile is supposed
    to travel in an arc, and is thus harder to aim; yet they require a direct
    hit unless using the feature for firing bins.

    The coefficient of 30 gives the following probabilities:

    |              |  Direct Hit    |  3x3 Area Hit  |
    |              |  30   50  100  |  30   50  100  |
    |       Novice |  15%   5%  <5% |  75%  40%  10% |
    |     Adequate |  45%  20%   5% | 100%  85%  40% |
    |    Competent |  75%  40%  10% | 100% 100%  70% |
    |   Proficient | 100%  75%  30% | 100% 100%  95% |
    | Professional | 100% 100%  80% | 100% 100% 100% |
    |    Legendary | 100% 100%  95% | 100% 100% 100% |

    Original data:

    * http://www.wolframalpha.com/input/?i=erf%2830*x%2Fsqrt%288%29%2F30%29^2%2C+erf%2830*x%2Fsqrt%288%29%2F50%29^2%2C+erf%2830*x%2Fsqrt%288%29%2F100%29^2%2C+x+from+0+to+15
    * http://www.wolframalpha.com/input/?i=erf%2830*x*3%2Fsqrt%288%29%2F30%29^2%2C+erf%2830*x*3%2Fsqrt%288%29%2F50%29^2%2C+erf%2830*x*3%2Fsqrt%288%29%2F100%29^2%2C+x+from+0+to+6

    Ballista can fire up to 200 tiles away, and can't use the bin method
    to compensate for inaccuracy. On the other hand, it shoots straight
    and should be easier to aim. It also damages everything in its path,
    so the hit probability may be estimated using an 1D projection.

    The coefficient of 48 gives the following probabilities:

    |              |  Direct Hit         |  1D Hit             |
    |              |  30   50  100  200  |  30   50  100  200  |
    |       Novice |  25%  10%   5%  <5% |  55%  35%  20%  10% |
    |     Adequate |  80%  45%  15%   5% |  90%  65%  40%  20% |
    |    Competent |  95%  70%  30%   8% | 100%  85%  50%  30% |
    |   Proficient | 100%  95%  65%  20% | 100% 100%  75%  45% |
    | Accomplished | 100% 100%  95%  60% | 100% 100% 100%  75% |
    |    Legendary | 100% 100%  100% 85% | 100% 100% 100%  90% |

    Original data:

    * http://www.wolframalpha.com/input/?i=erf%2848*x%2Fsqrt%288%29%2F30%29^2%2C+erf%2848*x%2Fsqrt%288%29%2F50%29^2%2C+erf%2848*x%2Fsqrt%288%29%2F100%29^2%2C+erf%2848*x%2Fsqrt%288%29%2F200%29^2%2C+x+from+0+to+15
    * http://www.wolframalpha.com/input/?i=erf%2848*x%2Fsqrt%288%29%2F30%29%2C+erf%2848*x%2Fsqrt%288%29%2F50%29%2C+erf%2848*x%2Fsqrt%288%29%2F100%29%2C+erf%2848*x%2Fsqrt%288%29%2F200%29%2C+x+from+0+to+15

    Quality can increase range of both engines by 25% max, so it
    also boosts aiming by 1.06x every step, up to 33.8% gain.
*/

/*
 * Misc. utils
 */

static bool debug_mode = false;

static void setDebug(bool on)
{
    debug_mode = on;
}

static void set_arrow_color(df::coord pos, int color)
{
    auto tile = Maps::getTileOccupancy(pos);

    if (tile)
        tile->bits.arrow_color = color;
}

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

static std::pair<int, int> get_engine_range(df::building_siegeenginest *bld, float quality)
{
    if (bld->type == siegeengine_type::Ballista)
        return std::make_pair(1, 200 + int(10 * quality));
    else
        return std::make_pair(30 - int(quality), 100 + int(5 * quality));
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

static bool is_build_complete(df::building *bld)
{
    return bld->getBuildStage() >= bld->getMaxBuildStage();
}

static float average_quality(df::building_actual *bld)
{
    float quality = 0;
    int count = 0;

    for (size_t i = 0; i < bld->contained_items.size(); i++)
    {
        if (bld->contained_items[i]->use_mode != 2)
            continue;
        count++;
        quality += bld->contained_items[i]->item->getQuality();
    }

    return count > 0 ? quality/count : 0;
}

static int point_distance(df::coord speed)
{
    return std::max(abs(speed.x), std::max(abs(speed.y), abs(speed.z)));
}

inline void normalize(float &x, float &y, float &z)
{
    float dist = sqrtf(x*x + y*y + z*z);
    if (dist == 0.0f) return;
    x /= dist; y /= dist; z /= dist;
}

static Random::MersenneRNG rng;

static void random_direction(float &x, float &y, float &z)
{
    float vec[3];
    rng.unitvector(vec, 3);
    x = vec[0]; y = vec[1]; z = vec[2];
}

static double random_error()
{
    // Irwin-Hall approximation to normal distribution with n = 3; varies in (-3,3)
    return (rng.drandom0() + rng.drandom0() + rng.drandom0()) * 2.0 - 3.0;
}

// round() is only available in C++11
static int int_round (double val)
{
    double frac = val - floor(val);
    if (frac < 0.5)
        return (int)floor(val);
    else
        return (int)ceil(val);
}

static const int WEAR_TICKS = 806400;

static bool apply_impact_damage(df::item *item, int minv, int maxv)
{
    MaterialInfo info(item);
    if (!info.isValid())
    {
        item->setWear(3);
        return false;
    }

    auto &strength = info.material->strength;

    // Use random strain type excluding COMPRESSIVE (conveniently last)
    int type = rng.random(strain_type::COMPRESSIVE);
    int power = minv + rng.random(maxv-minv+1);

    // High elasticity materials just bend
    if (strength.strain_at_yield[type] >= 5000)
        return true;

    // Instant fracture?
    int fracture = strength.fracture[type];
    if (fracture <= power || info.material->flags.is_set(material_flags::IS_GLASS))
    {
        item->setWear(3);
        return false;
    }

    // Impact within elastic strain range?
    int yield = strength.yield[type];
    if (yield > power)
        return true;

    // Can wear?
    auto actual = virtual_cast<df::item_actual>(item);
    if (!actual)
        return false;

    // Transform plastic deformation to wear
    int max_wear = WEAR_TICKS * 4;
    int cur_wear = WEAR_TICKS * actual->wear + actual->wear_timer;
    cur_wear += int64_t(power - yield)*max_wear/(fracture - yield);

    if (cur_wear >= max_wear)
    {
        actual->wear = 3;
        return false;
    }
    else
    {
        actual->wear = cur_wear / WEAR_TICKS;
        actual->wear_timer = cur_wear % WEAR_TICKS;
        return true;
    }
}

/*
 * Configuration object
 */

static bool enable_plugin();

struct EngineInfo {
    int id;
    df::building_siegeenginest *bld;

    df::coord center;
    coord_range building_rect;

    float quality;
    bool is_catapult;
    int proj_speed, hit_delay;
    std::pair<int, int> fire_range;

    double sigma_coeff;

    coord_range target;

    df::job_item_vector_id ammo_vector_id;
    df::item_type ammo_item_type;

    int operator_id, operator_frame;

    std::set<int> stockpiles;
    df::stockpile_links links;
    df::workshop_profile profile;

    bool hasTarget() { return is_range_valid(target); }
    bool onTarget(df::coord pos) { return is_in_range(target, pos); }
    df::coord getTargetSize() { return target.second - target.first; }

    bool isInRange(int dist) {
        return dist >= fire_range.first && dist <= fire_range.second;
    }
};

static std::map<df::building*, EngineInfo*> engines;
static std::map<df::coord, df::building*> coord_engines;

static EngineInfo *find_engine(df::building *bld, bool create = false)
{
    auto ebld = strict_virtual_cast<df::building_siegeenginest>(bld);
    if (!ebld)
        return NULL;

    auto &obj = engines[bld];

    if (obj)
    {
        obj->bld = ebld;
        return obj;
    }

    if (!create || !is_build_complete(bld))
        return NULL;

    obj = new EngineInfo();

    obj->id = bld->id;
    obj->bld = ebld;
    obj->center = df::coord(bld->centerx, bld->centery, bld->z);
    obj->building_rect = coord_range(
        df::coord(bld->x1, bld->y1, bld->z),
        df::coord(bld->x2, bld->y2, bld->z)
    );
    obj->quality = average_quality(ebld);
    obj->is_catapult = (ebld->type == siegeengine_type::Catapult);
    obj->proj_speed = 2;
    obj->hit_delay = obj->is_catapult ? 2 : -1;
    obj->fire_range = get_engine_range(ebld, obj->quality);

    // Base coefficients per engine type, plus 6% exponential bonus per quality level
    obj->sigma_coeff = (obj->is_catapult ? 30.0 : 48.0) * pow(1.06f, obj->quality);

    obj->ammo_vector_id = job_item_vector_id::BOULDER;
    obj->ammo_item_type = item_type::BOULDER;

    obj->operator_id = obj->operator_frame = -1;

    coord_engines[obj->center] = bld;
    return obj;
}

static EngineInfo *find_engine(lua_State *L, int idx, bool create = false, bool silent = false)
{
    auto bld = Lua::CheckDFObject<df::building_siegeenginest>(L, idx);

    auto engine = find_engine(bld, create);
    if (!engine && !silent)
        luaL_error(L, "no such engine");

    return engine;
}

static EngineInfo *find_engine(df::coord pos)
{
    auto engine = find_engine(coord_engines[pos]);

    if (engine)
    {
        auto bld0 = df::building::find(engine->id);
        auto bld = strict_virtual_cast<df::building_siegeenginest>(bld0);
        if (!bld)
            return NULL;

        engine->bld = bld;
    }

    return engine;
}

/*
 * Configuration management
 */

static void clear_engines()
{
    for (auto it = engines.begin(); it != engines.end(); ++it)
        delete it->second;
    engines.clear();
    coord_engines.clear();
}

/*
static void load_engines()
{
    clear_engines();

    std::vector<PersistentDataItem> vec;

    World::GetPersistentData(&vec, "siege-engine/target/", true);
    for (auto it = vec.begin(); it != vec.end(); ++it)
    {
        auto engine = find_engine(df::building::find(it->ival(0)), true);
        if (!engine) continue;
        engine->target.first = df::coord(it->ival(1), it->ival(2), it->ival(3));
        engine->target.second = df::coord(it->ival(4), it->ival(5), it->ival(6));
    }

    World::GetPersistentData(&vec, "siege-engine/ammo/", true);
    for (auto it = vec.begin(); it != vec.end(); ++it)
    {
        auto engine = find_engine(df::building::find(it->ival(0)), true);
        if (!engine) continue;
        engine->ammo_vector_id = (df::job_item_vector_id)it->ival(1);
        engine->ammo_item_type = (df::item_type)it->ival(2);
    }

    World::GetPersistentData(&vec, "siege-engine/stockpiles/", true);
    for (auto it = vec.begin(); it != vec.end(); ++it)
    {
        auto engine = find_engine(df::building::find(it->ival(0)), true);
        if (!engine)
            continue;
        auto pile = df::building::find(it->ival(1));
        if (!pile || pile->getType() != building_type::Stockpile)
        {
            World::DeletePersistentData(*it);
            continue;;
        }

        engine->stockpiles.insert(it->ival(1));
    }

    World::GetPersistentData(&vec, "siege-engine/profiles/", true);
    for (auto it = vec.begin(); it != vec.end(); ++it)
    {
        auto engine = find_engine(df::building::find(it->ival(0)), true);
        if (!engine) continue;
        engine->profile.min_level = it->ival(1);
        engine->profile.max_level = it->ival(2);
    }

    World::GetPersistentData(&vec, "siege-engine/profile-workers/", true);
    for (auto it = vec.begin(); it != vec.end(); ++it)
    {
        auto engine = find_engine(df::building::find(it->ival(0)), true);
        if (!engine)
            continue;
        auto unit = df::unit::find(it->ival(1));
        if (!unit || !Units::isCitizen(unit))
        {
            World::DeletePersistentData(*it);
            continue;
        }
        engine->profile.permitted_workers.push_back(it->ival(1));
    }
}*/

static int getTargetArea(lua_State *L)
{
    auto engine = find_engine(L, 1, false, true);

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

    //auto key = stl_sprintf("siege-engine/target/%d", bld->id);
    //World::DeletePersistentData(World::GetPersistentData(key));
}

static bool setTargetArea(df::building_siegeenginest *bld, df::coord target_min, df::coord target_max)
{
    CHECK_NULL_POINTER(bld);
    CHECK_INVALID_ARGUMENT(target_min.isValid() && target_max.isValid());
    CHECK_INVALID_ARGUMENT(is_build_complete(bld));

    if (!enable_plugin())
        return false;

    //auto key = stl_sprintf("siege-engine/target/%d", bld->id);
    //auto entry = World::GetPersistentData(key, NULL);
    //if (!entry.isValid())
    //    return false;

    auto engine = find_engine(bld, true);

    set_range(&engine->target, target_min, target_max);

    //entry.ival(0) = bld->id;
    //entry.ival(1) = engine->target.first.x;
    //entry.ival(2) = engine->target.first.y;
    //entry.ival(3) = engine->target.first.z;
    //entry.ival(4) = engine->target.second.x;
    //entry.ival(5) = engine->target.second.y;
    //entry.ival(6) = engine->target.second.z;

    df::coord sum = target_min + target_max;
    orient_engine(bld, df::coord(sum.x/2, sum.y/2, sum.z/2));

    return true;
}

static int getAmmoItem(lua_State *L)
{
    auto engine = find_engine(L, 1, false, true);
    if (!engine)
        Lua::Push(L, item_type::BOULDER);
    else
        Lua::Push(L, engine->ammo_item_type);
    return 1;
}

static int setAmmoItem(lua_State *L)
{
    if (!enable_plugin())
        return 0;

    auto engine = find_engine(L, 1, true);
    auto item_type = (df::item_type)luaL_optint(L, 2, item_type::BOULDER);
    if (!is_valid_enum_item(item_type))
        luaL_argerror(L, 2, "invalid item type");

    //auto key = stl_sprintf("siege-engine/ammo/%d", engine->id);
    //auto entry = World::GetPersistentData(key, NULL);
    //if (!entry.isValid())
    //    return 0;

    engine->ammo_vector_id = job_item_vector_id::IN_PLAY;
    engine->ammo_item_type = item_type;

    FOR_ENUM_ITEMS(job_item_vector_id, id)
    {
        auto other = ENUM_ATTR(job_item_vector_id, other, id);
        auto type = ENUM_ATTR(items_other_id, item, other);
        if (type == item_type)
        {
            engine->ammo_vector_id = id;
            break;
        }
    }

    //entry.ival(0) = engine->id;
    //entry.ival(1) = engine->ammo_vector_id;
    //entry.ival(2) = engine->ammo_item_type;

    lua_pushboolean(L, true);
    return 1;
}

static void forgetStockpileLink(EngineInfo *engine, int pile_id)
{
    engine->stockpiles.erase(pile_id);

    //auto key = stl_sprintf("siege-engine/stockpiles/%d/%d", engine->id, pile_id);
    //World::DeletePersistentData(World::GetPersistentData(key));
}

static void update_stockpile_links(EngineInfo *engine)
{
    engine->links.take_from_pile.clear();

    for (auto it = engine->stockpiles.begin(); it != engine->stockpiles.end(); )
    {
        int id = *it; ++it;
        auto pile = df::building::find(id);

        if (!pile || pile->getType() != building_type::Stockpile)
            forgetStockpileLink(engine, id);
        else
            // The vector is sorted, but we are iterating through a sorted set
            engine->links.take_from_pile.push_back(pile);
    }
}

static int getStockpileLinks(lua_State *L)
{
    auto engine = find_engine(L, 1, false, true);
    if (!engine || engine->stockpiles.empty())
        return 0;

    update_stockpile_links(engine);

    auto &links = engine->links.take_from_pile;
    lua_createtable(L, links.size(), 0);

    for (size_t i = 0; i < links.size(); i++)
    {
        Lua::Push(L, links[i]);
        lua_rawseti(L, -2, i+1);
    }

    return 1;
}

static bool isLinkedToPile(df::building_siegeenginest *bld, df::building_stockpilest *pile)
{
    CHECK_NULL_POINTER(bld);
    CHECK_NULL_POINTER(pile);

    auto engine = find_engine(bld);

    return engine && engine->stockpiles.count(pile->id);
}

static bool addStockpileLink(df::building_siegeenginest *bld, df::building_stockpilest *pile)
{
    CHECK_NULL_POINTER(bld);
    CHECK_NULL_POINTER(pile);
    CHECK_INVALID_ARGUMENT(is_build_complete(bld));

    if (!enable_plugin())
        return false;

    //auto key = stl_sprintf("siege-engine/stockpiles/%d/%d", bld->id, pile->id);
    //auto entry = World::GetPersistentData(key, NULL);
    //if (!entry.isValid())
    //    return false;

    auto engine = find_engine(bld, true);

    //entry.ival(0) = bld->id;
    //entry.ival(1) = pile->id;

    engine->stockpiles.insert(pile->id);
    return true;
}

static bool removeStockpileLink(df::building_siegeenginest *bld, df::building_stockpilest *pile)
{
    CHECK_NULL_POINTER(bld);
    CHECK_NULL_POINTER(pile);

    if (auto engine = find_engine(bld))
    {
        forgetStockpileLink(engine, pile->id);
        return true;
    }

    return false;
}

static df::workshop_profile *saveWorkshopProfile(df::building_siegeenginest *bld)
{
    CHECK_NULL_POINTER(bld);
    CHECK_INVALID_ARGUMENT(is_build_complete(bld));

    if (!enable_plugin())
        return NULL;

    // Save skill limits
    //auto key = stl_sprintf("siege-engine/profiles/%d", bld->id);
    //auto entry = World::GetPersistentData(key, NULL);
    //if (!entry.isValid())
    //    return NULL;

    auto engine = find_engine(bld, true);

    //entry.ival(0) = engine->id;
    //entry.ival(1) = engine->profile.min_level;
    //entry.ival(2) = engine->profile.max_level;

    // Save worker list
    //std::vector<PersistentDataItem> vec;
    //auto &workers = engine->profile.permitted_workers;

    //key = stl_sprintf("siege-engine/profile-workers/%d", bld->id);
    //World::GetPersistentData(&vec, key, true);

    //for (auto it = vec.begin(); it != vec.end(); ++it)
    //{
    //    if (linear_index(workers, it->ival(1)) < 0)
    //        World::DeletePersistentData(*it);
    //}

    //for (size_t i = 0; i < workers.size(); i++)
    //{
    //    key = stl_sprintf("siege-engine/profile-workers/%d/%d", bld->id, workers[i]);
    //    entry = World::GetPersistentData(key, NULL);
    //    if (!entry.isValid())
    //        continue;
    //    entry.ival(0) = engine->id;
    //    entry.ival(1) = workers[i];
    //}

    return &engine->profile;
}

static df::unit *getOperatorUnit(df::building_siegeenginest *bld, bool force = false)
{
    CHECK_NULL_POINTER(bld);

    auto engine = find_engine(bld);
    if (!engine)
        return NULL;

    if (engine->operator_id != -1 &&
        (world->frame_counter - engine->operator_frame) <= 5)
    {
        auto op_unit = df::unit::find(engine->operator_id);
        if (op_unit)
            return op_unit;
    }

    if (force)
    {
        color_ostream_proxy out(Core::getInstance().getConsole());
        out.print("Forced siege operator search\n");

        auto &active = world->units.active;
        for (size_t i = 0; i < active.size(); i++)
            if (active[i]->pos == engine->center && Units::isCitizen(active[i]))
                return active[i];
    }

    return NULL;
}

/*
 * Trajectory raytracing
 */

struct ProjectilePath {
    static const int DEFAULT_FUDGE = 31;

    df::coord origin, goal, target, fudge_delta;
    int divisor, fudge_factor;
    df::coord speed, direction;

    ProjectilePath(df::coord origin, df::coord goal) :
        origin(origin), goal(goal), fudge_factor(1)
    {
        fudge_delta = df::coord(0,0,0);
        calc_line();
    }

    ProjectilePath(df::coord origin, df::coord goal, df::coord delta, int factor) :
        origin(origin), goal(goal), fudge_delta(delta), fudge_factor(factor)
    {
        calc_line();
    }

    ProjectilePath(df::coord origin, df::coord goal, float zdelta, int factor = DEFAULT_FUDGE) :
        origin(origin), goal(goal), fudge_factor(factor)
    {
        fudge_delta = df::coord(0,0,int(factor * zdelta));
        calc_line();
    }

    void calc_line()
    {
        speed = goal - origin;
        speed.x *= fudge_factor;
        speed.y *= fudge_factor;
        speed.z *= fudge_factor;
        speed = speed + fudge_delta;
        target = origin + speed;
        divisor = point_distance(speed);
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

static ProjectilePath decode_path(lua_State *L, int idx, df::coord origin)
{
    idx = lua_absindex(L, idx);

    Lua::StackUnwinder frame(L);
    df::coord goal;

    lua_getfield(L, idx, "target");
    Lua::CheckDFAssign(L, &goal, frame[1]);

    lua_getfield(L, idx, "delta");

    if (!lua_isnil(L, frame[2]))
    {
        lua_getfield(L, idx, "factor");
        int factor = luaL_optnumber(L, frame[3], ProjectilePath::DEFAULT_FUDGE);

        if (lua_isnumber(L, frame[2]))
            return ProjectilePath(origin, goal, lua_tonumber(L, frame[2]), factor);

        df::coord delta;
        Lua::CheckDFAssign(L, &delta, frame[2]);

        return ProjectilePath(origin, goal, delta, factor);
    }

    return ProjectilePath(origin, goal);
}

static int projPosAtStep(lua_State *L)
{
    auto engine = find_engine(L, 1);
    auto path = decode_path(L, 2, engine->center);
    int step = luaL_checkint(L, 3);
    Lua::Push(L, path[step]);
    return 1;
}

static bool isPassableTile(df::coord pos)
{
    auto ptile = Maps::getTileType(pos);

    return !ptile || FlowPassable(*ptile);
}

static bool isTargetableTile(df::coord pos)
{
    auto ptile = Maps::getTileType(pos);

    return ptile && FlowPassable(*ptile) && !isOpenTerrain(*ptile);
}

static bool isTreeTile(df::coord pos)
{
    auto ptile = Maps::getTileType(pos);

    return ptile &&
        (tileShape(*ptile) == tiletype_shape::BRANCH ||
         tileShape(*ptile) == tiletype_shape::TRUNK_BRANCH ||
         tileShape(*ptile) == tiletype_shape::TWIG);
}

static bool adjustToTarget(EngineInfo *engine, df::coord *pos)
{
    if (isTargetableTile(*pos))
        return true;

    for (df::coord fudge = *pos;
         fudge.z <= engine->target.second.z; fudge.z++)
    {
        if (!isTargetableTile(fudge))
            continue;
        *pos = fudge;
        return true;
    }

    for (df::coord fudge = *pos;
            fudge.z >= engine->target.first.z; fudge.z--)
    {
        if (!isTargetableTile(fudge))
            continue;
        *pos = fudge;
        return true;
    }

    return false;
}

static int adjustToTarget(lua_State *L)
{
    auto engine = find_engine(L, 1, true);
    df::coord pos;
    Lua::CheckDFAssign(L, &pos, 2);
    bool ok = adjustToTarget(engine, &pos);
    Lua::Push(L, pos);
    Lua::Push(L, ok);
    return 2;
}

static const char* const hit_type_names[] = {
    "wall", "floor", "ceiling", "map_edge", "tree"
};

struct PathMetrics {
    enum CollisionType {
        Impassable,
        Floor,
        Ceiling,
        MapEdge,
        Tree
    } hit_type;

    int collision_step, collision_z_step;
    int goal_step, goal_z_step, goal_distance;

    bool hits() const { return collision_step > goal_step; }

    PathMetrics(const ProjectilePath &path)
    {
        compute(path);
    }

    void compute(const ProjectilePath &path)
    {
        hit_type = Impassable;
        collision_step = goal_step = goal_z_step = 1000000;
        collision_z_step = 0;

        goal_distance = point_distance(path.origin - path.goal);

        int step = 0;
        df::coord prev_pos = path.origin;

        for (;;) {
            df::coord cur_pos = path[++step];
            if (cur_pos == prev_pos)
                break;

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
                if (isTreeTile(cur_pos))
                {
                    // The projectile code has a bug where it will
                    // hit a tree on the same tick as a Z level change.
                    if (cur_pos.z != prev_pos.z)
                    {
                        hit_type = Tree;
                        break;
                    }
                }
                else
                {
                    hit_type = Impassable;
                    break;
                }
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

                collision_z_step = step;
            }

            prev_pos = cur_pos;
        }

        collision_step = step;
    }
};

enum TargetTileStatus {
    TARGET_OK, TARGET_RANGE, TARGET_BLOCKED, TARGET_SEMIBLOCKED
};
static const char* const target_tile_type_names[] = {
    "ok", "out_of_range", "blocked", "semi_blocked"
};

static TargetTileStatus calcTileStatus(EngineInfo *engine, const PathMetrics &raytrace)
{
    if (raytrace.hits())
    {
        if (engine->isInRange(raytrace.goal_step))
            return TARGET_OK;
        else
            return TARGET_RANGE;
    }
    else
        return TARGET_BLOCKED;
}

static int projPathMetrics(lua_State *L)
{
    auto engine = find_engine(L, 1);
    auto path = decode_path(L, 2, engine->center);

    PathMetrics info(path);

    lua_createtable(L, 0, 7);
    Lua::SetField(L, hit_type_names[info.hit_type], -1, "hit_type");
    Lua::SetField(L, info.collision_step, -1, "collision_step");
    Lua::SetField(L, info.collision_z_step, -1, "collision_z_step");
    Lua::SetField(L, info.goal_distance, -1, "goal_distance");
    if (info.goal_step < info.collision_step)
        Lua::SetField(L, info.goal_step, -1, "goal_step");
    if (info.goal_z_step < info.collision_step)
        Lua::SetField(L, info.goal_z_step, -1, "goal_z_step");
    Lua::SetField(L, target_tile_type_names[calcTileStatus(engine, info)], -1, "status");
    return 1;
}

static TargetTileStatus calcTileStatus(EngineInfo *engine, df::coord target, float zdelta)
{
    ProjectilePath path(engine->center, target, zdelta);
    PathMetrics raytrace(path);
    return calcTileStatus(engine, raytrace);
}

static TargetTileStatus calcTileStatus(EngineInfo *engine, df::coord target)
{
    auto status = calcTileStatus(engine, target, 0.0f);

    if (status == TARGET_BLOCKED)
    {
        if (calcTileStatus(engine, target, 0.5f) < TARGET_BLOCKED)
            return TARGET_SEMIBLOCKED;

        if (calcTileStatus(engine, target, -0.5f) < TARGET_BLOCKED)
            return TARGET_SEMIBLOCKED;
    }

    return status;
}

static std::string getTileStatus(df::building_siegeenginest *bld, df::coord tile_pos)
{
    auto engine = find_engine(bld, true);
    if (!engine)
        return "invalid";

    return target_tile_type_names[calcTileStatus(engine, tile_pos)];
}

static void paintAimScreen(df::building_siegeenginest *bld, df::coord view, df::coord2d ltop, df::coord2d size)
{
    auto engine = find_engine(bld, true);
    CHECK_NULL_POINTER(engine);

    for (int x = 0; x < size.x; x++)
    {
        for (int y = 0; y < size.y; y++)
        {
            df::coord tile_pos = view + df::coord(x,y,0);
            if (is_in_range(engine->building_rect, tile_pos))
                continue;

            Pen cur_tile = Screen::readTile(ltop.x+x, ltop.y+y);
            if (!cur_tile.valid())
                continue;

            int color = COLOR_YELLOW;

            switch (calcTileStatus(engine, tile_pos))
            {
                case TARGET_OK:
                    color = COLOR_GREEN;
                    break;
                case TARGET_RANGE:
                    color = COLOR_CYAN;
                    break;
                case TARGET_BLOCKED:
                    color = COLOR_RED;
                    break;
                case TARGET_SEMIBLOCKED:
                    color = COLOR_BROWN;
                    break;
            }

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

            cur_tile.bold = engine->onTarget(tile_pos);

            if (cur_tile.tile)
                cur_tile.tile_mode = Pen::CharColor;

            Screen::paintTile(cur_tile, ltop.x+x, ltop.y+y);
        }
    }
}

/*
 * Unit tracking
 */

static const float MAX_TIME = 1000000.0f;

struct UnitPath {
    df::unit *unit;
    std::map<float, df::coord> path;

    struct Hit {
        UnitPath *path;
        df::coord pos;
        int dist;
        float time, lmargin, rmargin;
    };

    static std::map<df::unit*, UnitPath*> cache;

    static UnitPath *get(df::unit *unit)
    {
        auto &cv = cache[unit];
        if (!cv) cv = new UnitPath(unit);
        return cv;
    };

    UnitPath(df::unit *unit) : unit(unit)
    {
        if (unit->flags1.bits.rider)
        {
            auto mount = df::unit::find(unit->relations.rider_mount_id);

            if (mount)
            {
                path = get(mount)->path;
                return;
            }
        }

        df::coord pos = unit->pos;
        df::coord dest = unit->path.dest;
        auto &upath = unit->path.path;

        if (dest.isValid() && !upath.x.empty())
        {
            float time = unit->counters.job_counter+0.5f;
            float speed = Units::computeMovementSpeed(unit)/100.0f;
            float slowdown = Units::computeSlowdownFactor(unit);

            if (unit->counters.unconscious > 0)
                time += unit->counters.unconscious;

            for (size_t i = 0; i < upath.size(); i++)
            {
                df::coord new_pos = upath[i];
                if (new_pos == pos)
                    continue;

                float delay = speed;

                // Diagonal movement
                if (new_pos.x != pos.x && new_pos.y != pos.y)
                    delay *= 362.0/256.0;

                // Meandering slowdown
                delay += (slowdown - 1) * speed;

                path[time] = pos;
                pos = new_pos;
                time += delay + 1;
            }
        }

        path[MAX_TIME] = pos;
    }

    void get_margin(std::map<float,df::coord>::iterator &it, float time, float *lmargin, float *rmargin)
    {
        auto it2 = it;
        *lmargin = (it == path.begin()) ? MAX_TIME : time - (--it2)->first;
        *rmargin = (it->first == MAX_TIME) ? MAX_TIME : it->first - time;
    }

    df::coord posAtTime(float time, float *lmargin = NULL, float *rmargin = NULL)
    {
        CHECK_INVALID_ARGUMENT(time < MAX_TIME);

        auto it = path.upper_bound(time);
        if (lmargin)
            get_margin(it, time, lmargin, rmargin);
        return it->second;
    }

    bool findHits(EngineInfo *engine, std::vector<Hit> *hit_points, float bias)
    {
        df::coord origin = engine->center;

        Hit info;
        info.path = this;

        for (auto it = path.begin(); it != path.end(); ++it)
        {
            info.pos = it->second;
            info.dist = point_distance(origin - info.pos);
            info.time = float(info.dist)*(engine->proj_speed+1) + engine->hit_delay + bias;
            get_margin(it, info.time, &info.lmargin, &info.rmargin);

            if (info.lmargin > 0 && info.rmargin > 0)
            {
                if (engine->onTarget(info.pos) && engine->isInRange(info.dist))
                    hit_points->push_back(info);
            }
        }

        return !hit_points->empty();
    }
};

std::map<df::unit*, UnitPath*> UnitPath::cache;

static void push_margin(lua_State *L, float margin)
{
    if (margin == MAX_TIME)
        lua_pushnil(L);
    else
        lua_pushnumber(L, margin);
}

static int traceUnitPath(lua_State *L)
{
    auto unit = Lua::CheckDFObject<df::unit>(L, 1);

    CHECK_NULL_POINTER(unit);

    size_t idx = 1;
    auto info = UnitPath::get(unit);
    lua_createtable(L, info->path.size(), 0);

    float last_time = 0.0f;
    for (auto it = info->path.begin(); it != info->path.end(); ++it)
    {
        Lua::Push(L, it->second);
        if (idx > 1)
        {
            lua_pushnumber(L, last_time);
            lua_setfield(L, -2, "from");
        }
        if (idx < info->path.size())
        {
            lua_pushnumber(L, it->first);
            lua_setfield(L, -2, "to");
        }
        lua_rawseti(L, -2, idx++);
        last_time = it->first;
    }

    return 1;
}

static int unitPosAtTime(lua_State *L)
{
    auto unit = Lua::CheckDFObject<df::unit>(L, 1);
    float time = luaL_checknumber(L, 2);

    CHECK_NULL_POINTER(unit);

    float lmargin, rmargin;
    auto info = UnitPath::get(unit);

    Lua::Push(L, info->posAtTime(time, &lmargin, &rmargin));
    push_margin(L, lmargin);
    push_margin(L, rmargin);
    return 3;
}

static bool canTargetUnit(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    if (unit->flags1.bits.dead ||
        unit->flags1.bits.caged ||
        unit->flags1.bits.left ||
        unit->flags1.bits.incoming ||
        unit->flags1.bits.hidden_in_ambush ||
        unit->flags3.bits.ghostly)
        return false;

    return true;
}

static void proposeUnitHits(EngineInfo *engine, std::vector<UnitPath::Hit> *hits, float bias)
{
    auto &active = world->units.active;

    for (size_t i = 0; i < active.size(); i++)
    {
        auto unit = active[i];

        if (!canTargetUnit(unit))
            continue;

        UnitPath::get(unit)->findHits(engine, hits, bias);
    }
}

static int proposeUnitHits(lua_State *L)
{
    auto engine = find_engine(L, 1);
    float bias = luaL_optnumber(L, 2, 0);

    if (!engine->hasTarget())
        luaL_error(L, "target not set");

    std::vector<UnitPath::Hit> hits;
    proposeUnitHits(engine, &hits, bias);

    lua_createtable(L, hits.size(), 0);

    for (size_t i = 0; i < hits.size(); i++)
    {
        auto &hit = hits[i];
        lua_createtable(L, 0, 6);
        Lua::SetField(L, hit.path->unit, -1, "unit");
        Lua::SetField(L, hit.pos, -1, "pos");
        Lua::SetField(L, hit.dist, -1, "dist");
        Lua::SetField(L, hit.time, -1, "time");
        push_margin(L, hit.lmargin);          lua_setfield(L, -2, "lmargin");
        push_margin(L, hit.rmargin);          lua_setfield(L, -2, "rmargin");
        lua_rawseti(L, -2, i+1);
    }

    return 1;
}

static int computeNearbyWeight(lua_State *L)
{
    auto engine = find_engine(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    luaL_checktype(L, 3, LUA_TTABLE);
    const char *fname = luaL_optstring(L, 4, "nearby_weight");

    std::vector<UnitPath*> units;
    std::vector<float> weights;

    lua_pushnil(L);

    while (lua_next(L, 3))
    {
        df::unit *unit;
        if (lua_isnumber(L, -2))
            unit = df::unit::find(lua_tointeger(L, -2));
        else
            unit = Lua::CheckDFObject<df::unit>(L, -2);
        if (!unit)
            continue;
        units.push_back(UnitPath::get(unit));
        weights.push_back(lua_tonumber(L, -1));
        lua_pop(L, 1);
    }

    lua_pushnil(L);

    while (lua_next(L, 2))
    {
        Lua::StackUnwinder frame(L, 1);

        lua_getfield(L, frame[1], "unit");
        df::unit *unit = Lua::CheckDFObject<df::unit>(L, -1);

        lua_getfield(L, frame[1], "time");
        float time = luaL_checknumber(L, lua_gettop(L));

        df::coord pos;

        lua_getfield(L, frame[1], "pos");
        if (lua_isnil(L, -1))
        {
            if (!unit) luaL_error(L, "either unit or pos is required");
            pos = UnitPath::get(unit)->posAtTime(time);
        }
        else
            Lua::CheckDFAssign(L, &pos, -1);

        float sum = 0.0f;

        for (size_t i = 0; i < units.size(); i++)
        {
            if (units[i]->unit == unit)
                continue;

            auto diff = units[i]->posAtTime(time) - pos;
            float dist = 1 + sqrtf(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z);
            sum += weights[i]/(dist*dist);
        }

        lua_pushnumber(L, sum);
        lua_setfield(L, frame[1], fname);
    }

    return 0;
}

static bool isTired(df::unit *worker)
{
    return worker->counters2.exhaustion >= 1000 ||
           worker->counters2.thirst_timer >= 25000 ||
           worker->counters2.hunger_timer >= 50000 ||
           worker->counters2.sleepiness_timer >= 57600;
}

static void releaseTiredWorker(EngineInfo *engine, df::job *job, df::unit *worker)
{
    // If not in siege
    auto &sieges = ui->invasions.list;

    for (size_t i = 0; i < sieges.size(); i++)
        if (sieges[i]->flags.bits.active)
            return;

    // And there is a free replacement
    auto &others = world->units.active;

    for (size_t i = 0; i < others.size(); i++)
    {
        auto unit = others[i];

        if (unit == worker ||
            unit->job.current_job || !unit->status.labors[unit_labor::SIEGEOPERATE] ||
            !Units::isCitizen(unit) || Units::getMiscTrait(unit, misc_trait_type::OnBreak) ||
            isTired(unit) || !Maps::canWalkBetween(job->pos, unit->pos))
            continue;

        int skill2 = Units::getEffectiveSkill(unit, job_skill::SIEGEOPERATE);

        if (skill2 >= engine->profile.min_level && skill2 <= engine->profile.max_level)
        {
            // Remove the worker and request a recheck
            if (Job::removeWorker(job))
            {
                color_ostream_proxy out(Core::getInstance().getConsole());
                out.print("Released tired operator %d from siege engine.\n", worker->id);

                if (process_jobs)
                    *process_jobs = true;
            }

            return;
        }
    }
}

/*
 * Projectile hook
 */

static const int offsets[8][2] = {
    { -1, -1 }, { 0, -1 }, { 1, -1 },
    { -1,  0 },            { 1, 0 },
    { -1,  1 }, { 0,  1 }, { 1, 1 }
};

struct projectile_hook : df::proj_itemst {
    typedef df::proj_itemst interpose_base;

    void aimAtPoint(EngineInfo *engine, const ProjectilePath &path)
    {
        target_pos = path.target;

        // Debug
        if (debug_mode)
            set_arrow_color(path.goal, COLOR_LIGHTMAGENTA);

        PathMetrics raytrace(path);

        // Materialize map blocks, or the projectile will crash into them
        for (int i = 0; i < raytrace.collision_step; i++)
            Maps::ensureTileBlock(path[i]);

        // Find valid hit point for catapult stones
        if (flags.bits.high_flying)
        {
            if (raytrace.hits())
                fall_threshold = raytrace.goal_step;
            else
                fall_threshold = (raytrace.collision_z_step+raytrace.collision_step-1)/2;

            while (fall_threshold < raytrace.collision_step-1)
            {
                if (isTargetableTile(path[fall_threshold]))
                    break;

                fall_threshold++;
            }
        }

        fall_threshold = std::max(fall_threshold, engine->fire_range.first);
        fall_threshold = std::min(fall_threshold, engine->fire_range.second);
    }

    void aimAtPoint(EngineInfo *engine, int skill, const ProjectilePath &path)
    {
        df::coord fail_target = path.goal;

        orient_engine(engine->bld, path.goal);

        // Debug
        if (debug_mode)
            set_arrow_color(path.goal, COLOR_LIGHTRED);

        // Dabbling always hit in 11x11 area
        if (skill < skill_rating::Novice)
        {
            fail_target.x += rng.random(11)-5;
            fail_target.y += rng.random(11)-5;
            aimAtPoint(engine, ProjectilePath(path.origin, fail_target));
            return;
        }

        // Otherwise use a normal distribution to simulate errors
        double sigma = point_distance(path.origin - path.goal) / (engine->sigma_coeff * skill);

        int dx = int_round(random_error() * sigma);
        int dy = int_round(random_error() * sigma);

        if (dx == 0 && dy == 0)
        {
            aimAtPoint(engine, path);
            return;
        }

        fail_target.x += dx;
        fail_target.y += dy;

        ProjectilePath fail(path.origin, fail_target, path.fudge_delta, path.fudge_factor);
        aimAtPoint(engine, fail);
    }

    void aimAtArea(EngineInfo *engine, int skill)
    {
        df::coord target, last_passable;
        df::coord tbase = engine->target.first;
        df::coord tsize = engine->getTargetSize();
        bool success = false;

        for (int i = 0; i < 50; i++)
        {
            target = tbase + df::coord(
                rng.random(tsize.x), rng.random(tsize.y), rng.random(tsize.z)
            );

            if (adjustToTarget(engine, &target))
                last_passable = target;
            else
                continue;

            ProjectilePath path(engine->center, target, engine->is_catapult ? 0.5f : 0.0f);
            PathMetrics raytrace(path);

            if (raytrace.hits() && engine->isInRange(raytrace.goal_step))
            {
                aimAtPoint(engine, skill, path);
                return;
            }
        }

        if (!last_passable.isValid())
            last_passable = target;

        aimAtPoint(engine, skill, ProjectilePath(engine->center, last_passable));
    }

    static int safeAimProjectile(lua_State *L)
    {
        color_ostream &out = *Lua::GetOutput(L);
        auto proj = (projectile_hook*)lua_touserdata(L, 1);
        auto engine = (EngineInfo*)lua_touserdata(L, 2);
        auto unit = (df::unit*)lua_touserdata(L, 3);
        int skill = lua_tointeger(L, 4);

        if (!Lua::PushModulePublic(out, L, "plugins.siege-engine", "doAimProjectile"))
            luaL_error(L, "Projectile aiming AI not available");

        Lua::PushDFObject(L, engine->bld);
        Lua::Push(L, proj->item);
        Lua::Push(L, engine->target.first);
        Lua::Push(L, engine->target.second);
        Lua::PushDFObject(L, unit);
        Lua::Push(L, skill);

        lua_call(L, 6, 1);

        if (lua_isnil(L, -1))
            proj->aimAtArea(engine, skill);
        else
            proj->aimAtPoint(engine, skill, decode_path(L, -1, engine->center));

        return 0;
    }

    void doCheckMovement()
    {
        if (flags.bits.parabolic || distance_flown != 0 ||
            fall_counter != fall_delay || item == NULL)
            return;

        auto engine = find_engine(origin_pos);
        if (!engine || !engine->hasTarget())
            return;

        auto L = Lua::Core::State;
        CoreSuspendClaimer suspend;
        color_ostream_proxy out(Core::getInstance().getConsole());

        df::unit *op_unit = getOperatorUnit(engine->bld, true);
        int skill = op_unit ? Units::getEffectiveSkill(op_unit, job_skill::SIEGEOPERATE) : 0;

        // Dabbling can't aim
        if (skill < skill_rating::Novice)
            aimAtArea(engine, skill);
        else
        {
            lua_pushcfunction(L, safeAimProjectile);
            lua_pushlightuserdata(L, this);
            lua_pushlightuserdata(L, engine);
            lua_pushlightuserdata(L, op_unit);
            lua_pushinteger(L, skill);

            if (!Lua::Core::SafeCall(out, 4, 0))
                aimAtArea(engine, skill);
        }

        bool forbid_ammo = DF_GLOBAL_VALUE(standing_orders_forbid_used_ammo, false);
        if (forbid_ammo)
            item->flags.bits.forbid = true;

        switch (item->getType())
        {
            case item_type::CAGE:
                flags.bits.bouncing = false;
                break;
            case item_type::BIN:
            case item_type::BARREL:
                flags.bits.bouncing = false;
                break;
            default:
                break;
        }
    }

    void doLaunchContents()
    {
        // Translate cartoon flight speed to parabolic
        float speed = 100000.0f / (fall_delay + 1);
        int min_zspeed = (fall_delay+1)*4900;

        float bonus = 1.0f + 0.1f*(origin_pos.z -cur_pos.z);
        bonus *= 1.0f + (distance_flown - 60) / 200.0f;
        speed *= bonus;

        // Flight direction vector
        df::coord dist = target_pos - origin_pos;
        float vx = dist.x, vy = dist.y, vz = fabs((float)dist.z);
        normalize(vx, vy, vz);

        int start_z = 0;

        // Start at tile top, if hit a wall
        ProjectilePath path(origin_pos, target_pos);
        auto next_pos = path[distance_flown+1];
        if (next_pos.z == cur_pos.z && !isPassableTile(next_pos))
            start_z = 49000;

        bool forbid_ammo = DF_GLOBAL_VALUE(standing_orders_forbid_used_ammo, false);

        MapExtras::MapCache mc;
        std::vector<df::item*> contents;
        Items::getContainedItems(item, &contents);

        for (size_t i = 0; i < contents.size(); i++)
        {
            auto child = contents[i];

            if (forbid_ammo)
                child->flags.bits.forbid = true;

            // Liquids are vaporized so that they cover nearby units
            if (child->isLiquid())
            {
                auto flow = Maps::spawnFlow(
                    cur_pos,
                    flow_type::MaterialVapor,
                    child->getMaterial(), child->getMaterialIndex(),
                    100
                );

                // should it leave a puddle too?..
                if (flow && Items::remove(mc, child))
                    continue;
            }

            auto proj = Items::makeProjectile(mc, child);
            if (!proj) continue;

            bool keep = apply_impact_damage(child, 50000, int(250000*bonus));

            proj->flags.bits.no_impact_destroy = keep;
            //proj->flags.bits.bouncing = true;
            proj->flags.bits.piercing = true;
            proj->flags.bits.parabolic = true;
            proj->flags.bits.unk9 = true;
            proj->flags.bits.no_collide = true;

            proj->pos_z = start_z;

            float sx, sy, sz;
            random_direction(sx, sy, sz);
            sx += vx*0.7; sy += vy*0.7; sz += vz*0.7;
            if (sz < 0) sz = -sz;
            normalize(sx, sy, sz);

            proj->speed_x = int(speed * sx);
            proj->speed_y = int(speed * sy);
            proj->speed_z = std::max(min_zspeed, int(speed * sz));
        }
    }

    DEFINE_VMETHOD_INTERPOSE(bool, checkMovement, ())
    {
        if (flags.bits.high_flying || flags.bits.piercing)
            doCheckMovement();

        return INTERPOSE_NEXT(checkMovement)();
    }

    DEFINE_VMETHOD_INTERPOSE(bool, checkImpact, (bool no_damage_floor))
    {
        if (!flags.bits.has_hit_ground && !flags.bits.parabolic &&
            flags.bits.high_flying && !flags.bits.bouncing &&
            !flags.bits.no_impact_destroy && target_pos != origin_pos &&
            item && item->flags.bits.container)
        {
            doLaunchContents();
        }

        return INTERPOSE_NEXT(checkImpact)(no_damage_floor);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(projectile_hook, checkMovement);
IMPLEMENT_VMETHOD_INTERPOSE(projectile_hook, checkImpact);

/*
 * Building hook
 */

struct building_hook : df::building_siegeenginest {
    typedef df::building_siegeenginest interpose_base;

    DEFINE_VMETHOD_INTERPOSE(df::workshop_profile*, getWorkshopProfile, ())
    {
        if (auto engine = find_engine(this))
            return &engine->profile;

        return INTERPOSE_NEXT(getWorkshopProfile)();
    }

    DEFINE_VMETHOD_INTERPOSE(df::stockpile_links*, getStockpileLinks, ())
    {
        if (auto engine = find_engine(this))
        {
            update_stockpile_links(engine);
            return &engine->links;
        }

        return INTERPOSE_NEXT(getStockpileLinks)();
    }

    DEFINE_VMETHOD_INTERPOSE(void, updateAction, ())
    {
        INTERPOSE_NEXT(updateAction)();

        if (jobs.empty())
            return;

        if (auto engine = find_engine(this))
        {
            auto job = jobs[0];
            bool save_op = false;
            bool load_op = false;

            switch (job->job_type)
            {
                case job_type::LoadCatapult:
                    if (!job->job_items.empty())
                    {
                        auto item = job->job_items[0];
                        item->item_type = engine->ammo_item_type;
                        item->vector_id = engine->ammo_vector_id;

                        switch (item->item_type)
                        {
                            case item_type::NONE:
                            case item_type::BOULDER:
                            case item_type::BLOCKS:
                                item->mat_type = 0;
                                break;

                            case item_type::BIN:
                            case item_type::BARREL:
                                item->mat_type = -1;
                                // A hack to make it take objects assigned to stockpiles.
                                // Since reaction_id is not set, the actual value is not used.
                                item->contains.resize(1);
                                break;

                            default:
                                item->mat_type = -1;
                                break;
                        }
                    }
                    // fallthrough

                case job_type::LoadBallista:
                    load_op = true;

                case job_type::FireCatapult:
                case job_type::FireBallista:
                    if (auto worker = Job::getWorker(job))
                    {
                        engine->operator_id = worker->id;
                        engine->operator_frame = world->frame_counter;

                        if (action == PrepareToFire && !load_op &&
                            (world->frame_counter%100) == 0 && isTired(worker))
                            releaseTiredWorker(engine, job, worker);
                    }
                    break;

                default:
                    break;
            }
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(building_hook, getWorkshopProfile);
IMPLEMENT_VMETHOD_INTERPOSE(building_hook, getStockpileLinks);
IMPLEMENT_VMETHOD_INTERPOSE(building_hook, updateAction);

/*
 * Initialization
 */

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(setDebug),
    DFHACK_LUA_FUNCTION(clearTargetArea),
    DFHACK_LUA_FUNCTION(setTargetArea),
    DFHACK_LUA_FUNCTION(isLinkedToPile),
    DFHACK_LUA_FUNCTION(addStockpileLink),
    DFHACK_LUA_FUNCTION(removeStockpileLink),
    DFHACK_LUA_FUNCTION(saveWorkshopProfile),
    DFHACK_LUA_FUNCTION(getTileStatus),
    DFHACK_LUA_FUNCTION(paintAimScreen),
    DFHACK_LUA_FUNCTION(canTargetUnit),
    DFHACK_LUA_FUNCTION(isPassableTile),
    DFHACK_LUA_FUNCTION(isTreeTile),
    DFHACK_LUA_FUNCTION(isTargetableTile),
    DFHACK_LUA_END
};

DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(getTargetArea),
    DFHACK_LUA_COMMAND(getAmmoItem),
    DFHACK_LUA_COMMAND(setAmmoItem),
    DFHACK_LUA_COMMAND(getStockpileLinks),
    DFHACK_LUA_COMMAND(projPosAtStep),
    DFHACK_LUA_COMMAND(projPathMetrics),
    DFHACK_LUA_COMMAND(adjustToTarget),
    DFHACK_LUA_COMMAND(traceUnitPath),
    DFHACK_LUA_COMMAND(unitPosAtTime),
    DFHACK_LUA_COMMAND(proposeUnitHits),
    DFHACK_LUA_COMMAND(computeNearbyWeight),
    DFHACK_LUA_END
};

DFHACK_PLUGIN_IS_ENABLED(is_enabled);

static void enable_hooks(bool enable)
{
    is_enabled = enable;

    INTERPOSE_HOOK(projectile_hook, checkMovement).apply(enable);
    INTERPOSE_HOOK(projectile_hook, checkImpact).apply(enable);

    INTERPOSE_HOOK(building_hook, getWorkshopProfile).apply(enable);
    INTERPOSE_HOOK(building_hook, getStockpileLinks).apply(enable);
    INTERPOSE_HOOK(building_hook, updateAction).apply(enable);

    if (enable) {
        //load_engines();
    } else
        clear_engines();
}

static bool enable_plugin()
{
    if (is_enabled)
        return true;

    //auto entry = World::GetPersistentData("siege-engine/enabled", NULL);
    //if (!entry.isValid())
    //    return false;

    enable_hooks(true);
    return true;
}

static void clear_caches(color_ostream &out)
{
    if (!UnitPath::cache.empty())
    {
        for (auto it = UnitPath::cache.begin(); it != UnitPath::cache.end(); ++it)
            delete it->second;

        UnitPath::cache.clear();
    }
}

static void load_config(color_ostream& out) {
    clear_engines();
    Json::Value& p = Persistent::get("siege-engine");
    int32_t version = p["version"].isInt() ? p["version"].asInt() : 0;
    if ( version == 0 ) {
        auto entry = World::GetPersistentData("siege-engine/enabled",NULL);
        bool enable = entry.isValid();
        World::DeletePersistentData(entry);
        //load_engines
        std::vector<PersistentDataItem> vec;
        World::GetPersistentData(&vec,"siege-engine/target/",true);
        for ( auto it = vec.begin(); it != vec.end(); ++it ) {
            int32_t id = it->ival(0);
            EngineInfo* engine = find_engine(df::building::find(id));
            if ( !engine ) {
                World::DeletePersistentData(*it);
                continue;
            }
            int32_t x1,y1,z1,x2,y2,z2;
            x1 = it->ival(1);
            y1 = it->ival(2);
            z1 = it->ival(3);
            x2 = it->ival(4);
            y2 = it->ival(5);
            z2 = it->ival(6);
            engine->target.first = df::coord(x1,y1,z1);
            engine->target.second = df::coord(x2,y2,z2);
            World::DeletePersistentData(*it);
        }
        vec.clear();
        World::GetPersistentData(&vec, "siege-engine/ammo/", true);
        for ( auto it = vec.begin(); it != vec.end(); ++it ) {
            int32_t id = it->ival(0);
            EngineInfo* engine = find_engine(df::building::find(id));
            if ( !engine ) {
                World::DeletePersistentData(*it);
                continue;
            }
            engine->ammo_vector_id = (df::job_item_vector_id)it->ival(1);
            engine->ammo_item_type = (df::item_type)it->ival(2);
            World::DeletePersistentData(*it);
        }
        vec.clear();
        World::GetPersistentData(&vec, "siege-engine/stockpiles/", true);
        for ( auto it = vec.begin(); it != vec.end(); ++it ) {
            int32_t id = it->ival(0);
            EngineInfo* engine = find_engine(df::building::find(id));
            if ( !engine ) {
                World::DeletePersistentData(*it);
                continue;
            }
            auto pile = df::building::find(it->ival(1));
            if ( !pile || pile->getType() != building_type::Stockpile ) {
                World::DeletePersistentData(*it);
                continue;
            }
            engine->stockpiles.insert(it->ival(1));
            World::DeletePersistentData(*it);
        }
        vec.clear();
        World::GetPersistentData(&vec, "siege-engine/profiles/", true);
        for ( auto it = vec.begin(); it != vec.end(); ++it ) {
            int32_t id = it->ival(0);
            EngineInfo* engine = find_engine(df::building::find(id));
            if ( !engine ) {
                World::DeletePersistentData(*it);
                continue;
            }
            engine->profile.min_level = it->ival(1);
            engine->profile.max_level = it->ival(2);
            World::DeletePersistentData(*it);
        }
        vec.clear();
        World::GetPersistentData(&vec, "siege-engine/profile-workers/", true);
        for ( auto it = vec.begin(); it != vec.end(); ++it ) {
            int32_t id = it->ival(0);
            EngineInfo* engine = find_engine(df::building::find(id));
            if ( !engine ) {
                World::DeletePersistentData(*it);
                continue;
            }
            df::unit* unit = df::unit::find(it->ival(1));
            if ( !unit || !Units::isCitizen(unit) ) {
                World::DeletePersistentData(*it);
                continue;
            }
            engine->profile.permitted_workers.push_back(it->ival(1));
            World::DeletePersistentData(*it);
        }
        vec.clear();
        enable_hooks(enable);
    } else if ( version == 1 ) {
        //TODO: enabled
        Json::Value& engines_node = p["siege_engines"];
        for ( int32_t a = 0; a < engines_node.size(); a++ ) {
            Json::Value& engineNode = engines_node[a];
            int32_t id = engineNode["id"].asInt();
            EngineInfo* engine_info = find_engine(df::building::find(id),true);
            if ( !engine_info )
                continue;
            Json::Value& target = engineNode["target"];
            Json::Value& target1 = target["coord1"];
            Json::Value& target2 = target["coord2"];
            set_range(&engine_info->target, df::coord(target1["x"].asInt(),target1["y"].asInt(),target1["z"].asInt()), df::coord(target2["x"].asInt(),target2["y"].asInt(),target2["z"].asInt()));

            engine_info->ammo_vector_id = (df::job_item_vector_id)engineNode["ammo_vector_id"].asInt();
            engine_info->ammo_item_type = (df::item_type)engineNode["ammo_item_type"].asInt();
            Json::Value& stockpiles = engineNode["stockpiles"];
            for ( int32_t b = 0; b < stockpiles.size(); b++ ) {
                int32_t id = stockpiles[b].asInt();
                df::building* pile = df::building::find(id);
                if ( !pile || pile->getType() != building_type::Stockpile )
                    continue;
                engine_info->stockpiles.insert(id);
            }
            Json::Value& profileNode = engineNode["profile"];
            engine_info->profile.min_level = profileNode["min_level"].asInt();
            engine_info->profile.max_level = profileNode["max_level"].asInt();
            Json::Value& permitted_workers = profileNode["permitted_workers"];
            for ( int32_t b = 0; b < permitted_workers.size(); b++ ) {
                int32_t id = permitted_workers[b].asInt();
                df::unit* unit = df::unit::find(id);
                if ( !unit || !Units::isCitizen(unit) )
                    continue;
                engine_info->profile.permitted_workers.push_back(id);
            }
        }
    } else {
        cerr << __FILE__ << ":" << __LINE__ << ": Unrecognized version: " << version << endl;
        exit(1);
    }
}
static void save_config(color_ostream& out) {
    Json::Value& p = Persistent::get("siege-engine");
    p.clear();
    p["version"] = persist_version;
    Json::Value& siege_engines_node = p["siege_engines"];
    for ( auto i = engines.begin(); i != engines.end(); ++i ) {
        EngineInfo* info = i->second;
        siege_engines_node.append(Json::Value());
        Json::Value& siege_engine_node = siege_engines_node[siege_engines_node.size()-1];
        siege_engine_node["id"] = info->id;
        {
            Json::Value& target_node = siege_engine_node["target"];
            Json::Value& coord1 = target_node["coord1"];
            Json::Value& coord2 = target_node["coord2"];
            coord1["x"] = info->target.first.x;
            coord1["y"] = info->target.first.y;
            coord1["z"] = info->target.first.z;
            coord2["x"] = info->target.second.x;
            coord2["y"] = info->target.second.y;
            coord2["z"] = info->target.second.z;
        }
        siege_engines_node["ammo_vector_id"] = (int32_t)info->ammo_vector_id;
        siege_engines_node["ammo_item_type"] = (int32_t)info->ammo_item_type;
        Json::Value& profile = siege_engines_node["profile"];
        profile["min_level"] = info->profile.min_level;
        profile["max_level"] = info->profile.max_level;
        Json::Value& workers = profile["permitted_workers"];
        for ( auto j = info->profile.permitted_workers.begin(); j != info->profile.permitted_workers.end(); ++j ) {
            int32_t id = *j;
            workers.append(id);
        }
    }
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    if (gamemode && *gamemode != game_mode::DWARF)
        return CR_FAILURE;

    if (enable != is_enabled)
    {
        if (enable)
            enable_plugin();
        else
        {
            //World::DeletePersistentData(World::GetPersistentData("siege-engine/enabled"));
            enable_hooks(false);
        }
    }

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        load_config(out);
        if (!gamemode || *gamemode == game_mode::DWARF)
        {
            bool enable = engines.size()>0;
            //bool enable = World::GetPersistentData("siege-engine/enabled").isValid();

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
    rng.init();

    if (Core::getInstance().isMapLoaded())
        plugin_onstatechange(out, SC_MAP_LOADED);

    EventManager::EventHandler handler(on_presave_callback, 1);
    EventManager::registerListener(EventManager::EventType::PRESAVE, handler, plugin_self);

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    if ( DFHack::Core::getInstance().isMapLoaded() )
       save_config(out);
    enable_hooks(false);
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    clear_caches(out);
    return CR_OK;
}
