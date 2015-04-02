#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <modules/Gui.h>
#include <modules/Screen.h>
#include <modules/Maps.h>
#include <modules/World.h>
#include <TileTypes.h>
#include <vector>
#include <cstdio>
#include <stack>
#include <string>
#include <cmath>
#include <string.h>

#include <VTableInterpose.h>
#include "df/graphic.h"
#include "df/building_workshopst.h"
#include "df/building_def_workshopst.h"
#include "df/item_liquid_miscst.h"
#include "df/power_info.h"
#include "df/workshop_type.h"
#include "df/builtin_mats.h"
#include "df/world.h"
#include "df/buildings_other_id.h"
#include "df/machine.h"
#include "df/job.h"
#include "df/building_drawbuffer.h"
#include "df/ui.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/ui_build_selector.h"
#include "df/flow_info.h"
#include "df/report.h"

#include "MiscUtils.h"

/*
 * This plugin implements a steam engine workshop. It activates
 * if there are any workshops in the raws with STEAM_ENGINE in
 * their token, and provides the necessary behavior.
 *
 * Construction:
 *
 * The workshop needs water as its input, which it takes via a
 * passable floor tile below it, like usual magma workshops do.
 * The magma version also needs magma.
 *
 * ISSUE: Since this building is a machine, and machine collapse
 * code cannot be modified, it would collapse over true open space.
 * As a loophole, down stair provides support to machines, while
 * being passable, so use them.
 *
 * After constructing the building itself, machines can be connected
 * to the edge tiles that look like gear boxes. Their exact position
 * is extracted from the workshop raws.
 *
 * ISSUE: Like with collapse above, part of the code involved in
 * machine connection cannot be modified. As a result, the workshop
 * can only immediately connect to machine components built AFTER it.
 * This also means that engines cannot be chained without intermediate
 * short axles that can be built later.
 *
 * Operation:
 *
 * In order to operate the engine, queue the Stoke Boiler job.
 * A furnace operator will come, possibly bringing a bar of fuel,
 * and perform it. As a result, a "boiling water" item will appear
 * in the 't' view of the workshop.
 *
 * Note: The completion of the job will actually consume one unit
 * of appropriate liquids from below the workshop.
 *
 * Every such item gives 100 power, up to a limit of 300 for coal,
 * and 500 for a magma engine. The building can host twice that
 * amount of items to provide longer autonomous running. When the
 * boiler gets filled to capacity, all queued jobs are suspended;
 * once it drops back to 3+1 or 5+1 items, they are re-enabled.
 *
 * While the engine is providing power, steam is being consumed.
 * The consumption speed includes a fixed 10% waste rate, and
 * the remaining 90% are applied proportionally to the actual
 * load in the machine. With the engine at nominal 300 power with
 * 150 load in the system, it will consume steam for actual
 * 300*(10% + 90%*150/300) = 165 power.
 *
 * Masterpiece mechanism and chain will decrease the mechanical
 * power drawn by the engine itself from 10 to 5. Masterpiece
 * barrel decreases waste rate by 4%. Masterpiece piston and pipe
 * decrease it by further 4%, and also decrease the whole steam
 * use rate by 10%.
 *
 * Explosions:
 *
 * The engine must be constructed using barrel, pipe and piston
 * from fire-safe, or in the magma version magma-safe metals.
 *
 * During operation weak parts get gradually worn out, and
 * eventually the engine explodes. It should also explode if
 * toppled during operation by a building destroyer, or a
 * tantruming dwarf.
 *
 * Save files:
 *
 * It should be safe to load and view fortresses using engines
 * from a DF version without DFHack installed, except that in such
 * case the engines won't work. However actually making modifications
 * to them, or machines they connect to (including by pulling levers),
 * can easily result in inconsistent state once this plugin is
 * available again. The effects may be as weird as negative power
 * being generated.
 */

using std::vector;
using std::string;
using std::stack;
using std::set;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("steam-engine");

REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(ui_build_selector);
REQUIRE_GLOBAL(cursor);

/*
 * List of known steam engine workshop raws.
 */

struct steam_engine_workshop {
    int id;
    df::building_def_workshopst *def;
    // Cached properties
    bool is_magma;
    int max_power, max_capacity;
    int wear_temp;
    // Special tiles (relative position)
    std::vector<df::coord2d> gear_tiles;
    df::coord2d hearth_tile;
    df::coord2d water_tile;
    df::coord2d magma_tile;
};

std::vector<steam_engine_workshop> engines;

steam_engine_workshop *find_steam_engine(int id)
{
    for (size_t i = 0; i < engines.size(); i++)
        if (engines[i].id == id)
            return &engines[i];

    return NULL;
}

/*
 * Misc utilities.
 */

static const int hearth_colors[6][2] = {
    { COLOR_BLACK, 1 },
    { COLOR_BROWN, 0 },
    { COLOR_RED, 0 },
    { COLOR_RED, 1 },
    { COLOR_BROWN, 1 },
    { COLOR_GREY, 1 }
};

void enable_updates_at(df::coord pos, bool flow, bool temp)
{
    static const int delta[4][2] = { { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 } };

    for (int i = 0; i < 4; i++)
    {
        auto blk = Maps::getTileBlock(pos.x+delta[i][0], pos.y+delta[i][1], pos.z);
        Maps::enableBlockUpdates(blk, flow, temp);
    }
}

void decrement_flow(df::coord pos, int amount)
{
    auto pldes = Maps::getTileDesignation(pos);
    if (!pldes) return;

    int nsize = std::max(0, int(pldes->bits.flow_size - amount));
    pldes->bits.flow_size = nsize;
    pldes->bits.flow_forbid = (nsize > 3 || pldes->bits.liquid_type == tile_liquid::Magma);

    enable_updates_at(pos, true, false);
}

void make_explosion(df::coord center, int power)
{
    static const int bias[9] = {
        60, 30, 60,
        30, 0, 30,
        60, 30, 60
    };

    int mat_type = builtin_mats::WATER, mat_index = -1;
    int i = 0;

    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dy = -1; dy <= 1; dy++)
        {
            int size = power - bias[i++];
            auto pos = center + df::coord(dx,dy,0);

            if (size > 0)
                Maps::spawnFlow(pos, flow_type::MaterialDust, mat_type, mat_index, size);
        }
    }

    Gui::showAutoAnnouncement(
        announcement_type::CAVE_COLLAPSE, center,
        "A boiler has exploded!", COLOR_RED, true
    );
}

static const int WEAR_TICKS = 806400;

bool add_wear_nodestroy(df::item_actual *item, int rate)
{
    if (item->incWearTimer(rate))
    {
        while (item->wear_timer >= WEAR_TICKS)
        {
            item->wear_timer -= WEAR_TICKS;
            item->wear++;
        }
    }

    return item->wear > 3;
}

/*
 * Hook for the liquid item. Implements a special 'boiling'
 * matter state with a modified description and temperature
 * locked at boiling-1.
 */

struct liquid_hook : df::item_liquid_miscst {
    typedef df::item_liquid_miscst interpose_base;

    static const uint32_t BOILING_FLAG = 0x80000000U;

    DEFINE_VMETHOD_INTERPOSE(void, getItemDescription, (std::string *buf, int8_t mode))
    {
        if (mat_state.whole & BOILING_FLAG)
            buf->append("boiling ");

        INTERPOSE_NEXT(getItemDescription)(buf, mode);
    }

    DEFINE_VMETHOD_INTERPOSE(bool, adjustTemperature, (uint16_t temp, int32_t unk))
    {
        if (mat_state.whole & BOILING_FLAG)
            temp = std::max(int(temp), getBoilingPoint()-1);

        return INTERPOSE_NEXT(adjustTemperature)(temp, unk);
    }

    DEFINE_VMETHOD_INTERPOSE(bool, checkTemperatureDamage, ())
    {
        if (mat_state.whole & BOILING_FLAG)
            temperature.whole = std::max(int(temperature.whole), getBoilingPoint()-1);

        return INTERPOSE_NEXT(checkTemperatureDamage)();
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(liquid_hook, getItemDescription);
IMPLEMENT_VMETHOD_INTERPOSE(liquid_hook, adjustTemperature);
IMPLEMENT_VMETHOD_INTERPOSE(liquid_hook, checkTemperatureDamage);

/*
 * Hook for the workshop itself. Implements core logic.
 */

struct workshop_hook : df::building_workshopst {
    typedef df::building_workshopst interpose_base;

    // Engine detection

    steam_engine_workshop *get_steam_engine()
    {
        if (type == workshop_type::Custom)
            return find_steam_engine(custom_type);

        return NULL;
    }

    inline bool is_fully_built()
    {
        return getBuildStage() >= getMaxBuildStage();
    }

    // Use high bits of flags to store current steam amount.
    // This is necessary for consistency if items disappear unexpectedly.

    int get_steam_amount()
    {
        return (flags.whole >> 28) & 15;
    }

    void set_steam_amount(int count)
    {
        flags.whole = (flags.whole & 0x0FFFFFFFU) | uint32_t((count & 15) << 28);
    }

    // Find liquids to consume below the engine.

    bool find_liquids(df::coord *pwater, df::coord *pmagma, bool is_magma, int min_level)
    {
        if (!is_magma)
            pmagma = NULL;

        for (int x = x1; x <= x2; x++)
        {
            for (int y = y1; y <= y2; y++)
            {
                auto ptile = Maps::getTileType(x,y,z);
                if (!ptile || !FlowPassableDown(*ptile))
                    continue;

                auto pltile = Maps::getTileType(x,y,z-1);
                if (!pltile || !FlowPassable(*pltile))
                    continue;

                auto pldes = Maps::getTileDesignation(x,y,z-1);
                if (!pldes || pldes->bits.flow_size < min_level)
                    continue;

                if (pldes->bits.liquid_type == tile_liquid::Magma)
                {
                    if (pmagma)
                        *pmagma = df::coord(x,y,z-1);
                    if (pwater->isValid())
                        return true;
                }
                else
                {
                    *pwater = df::coord(x,y,z-1);
                    if (!pmagma || pmagma->isValid())
                        return true;
                }
            }
        }

        return false;
    }

    // Absorbs a water item produced by stoke reaction into the engine.

    bool absorb_unit(steam_engine_workshop *engine, df::item_liquid_miscst *liquid)
    {
        // Consume liquid inputs
        df::coord water, magma;

        if (!find_liquids(&water, &magma, engine->is_magma, 1))
        {
            // Destroy the item with enormous wear amount.
            liquid->addWear(WEAR_TICKS*5, true, false);
            return false;
        }

        decrement_flow(water, 1);
        if (engine->is_magma)
            decrement_flow(magma, 1);

        // Update flags
        liquid->flags.bits.in_building = true;
        liquid->mat_state.whole |= liquid_hook::BOILING_FLAG;
        liquid->temperature.whole = liquid->getBoilingPoint()-1;
        liquid->temperature.fraction = 0;

        // This affects where the steam appears to come from
        if (engine->hearth_tile.isValid())
            liquid->pos = df::coord(x1+engine->hearth_tile.x, y1+engine->hearth_tile.y, z);

        // Enable block temperature updates
        enable_updates_at(liquid->pos, false, true);
        return true;
    }

    bool boil_unit(df::item_liquid_miscst *liquid)
    {
        liquid->wear = 4;
        liquid->flags.bits.in_building = false;
        liquid->temperature.whole = liquid->getBoilingPoint() + 10;

        return liquid->checkMeltBoil();
    }

    void suspend_jobs(bool suspend)
    {
        for (size_t i = 0; i < jobs.size(); i++)
            if (jobs[i]->job_type == job_type::CustomReaction)
                jobs[i]->flags.bits.suspend = suspend;
    }

    // Scan contained items for boiled steam to absorb.

    df::item_liquid_miscst *collect_steam(steam_engine_workshop *engine, int *count)
    {
        df::item_liquid_miscst *first = NULL;
        *count = 0;

        for (int i = contained_items.size()-1; i >= 0; i--)
        {
            auto item = contained_items[i];
            if (item->use_mode != 0)
                continue;

            auto liquid = strict_virtual_cast<df::item_liquid_miscst>(item->item);
            if (!liquid)
                continue;

            if (!liquid->flags.bits.in_building)
            {
                if (liquid->mat_type != builtin_mats::WATER ||
                    liquid->age > 1 ||
                    liquid->wear != 0)
                    continue;

                // This may destroy the item
                if (!absorb_unit(engine, liquid))
                    continue;
            }

            if (*count < engine->max_capacity)
            {
                first = liquid;
                ++*count;
            }
            else
            {
                // Overpressure valve
                boil_unit(liquid);
                suspend_jobs(true);
            }
        }

        return first;
    }

    void random_boil()
    {
        int cnt = 0;

        for (int i = contained_items.size()-1; i >= 0; i--)
        {
            auto item = contained_items[i];
            if (item->use_mode != 0 || !item->item->flags.bits.in_building)
                continue;

            auto liquid = strict_virtual_cast<df::item_liquid_miscst>(item->item);
            if (!liquid)
                continue;

            if (cnt == 0 || rand() < RAND_MAX/2)
            {
                cnt++;
                boil_unit(liquid);
            }
        }
    }

    int classify_component(df::building_actual::T_contained_items *item)
    {
        if (item->use_mode != 2 || item->item->isBuildMat())
            return -1;

        switch (item->item->getType())
        {
        case item_type::TRAPPARTS:
        case item_type::CHAIN:
            return 0;
        case item_type::BARREL:
            return 2;
        default:
            return 1;
        }
    }

    bool check_component_wear(steam_engine_workshop *engine, int count, int power)
    {
        int coeffs[3] = { 0, power, count };

        for (int i = contained_items.size()-1; i >= 0; i--)
        {
            int type = classify_component(contained_items[i]);
            if (type < 0)
                continue;

            df::item *item = contained_items[i]->item;
            int melt_temp = item->getMeltingPoint();
            if (coeffs[type] == 0 || melt_temp >= engine->wear_temp)
                continue;

            // let 500 degree delta at 4 pressure work 1 season
            float ticks = coeffs[type]*(engine->wear_temp - melt_temp)*3.0f/500.0f/4.0f;
            if (item->addWear(int(8*(1 + ticks)), true, true))
                return true;
        }

        return false;
    }

    float get_component_quality(int use_type)
    {
        float sum = 0, cnt = 0;

        for (size_t i = 0; i < contained_items.size(); i++)
        {
            int type = classify_component(contained_items[i]);
            if (type != use_type)
                continue;

            sum += contained_items[i]->item->getQuality();
            cnt += 1;
        }

        return (cnt > 0 ? sum/cnt : 0);
    }

    int get_steam_use_rate(steam_engine_workshop *engine, int dimension, int power_level)
    {
        // total ticks to wear off completely
        float ticks = WEAR_TICKS * 4.0f;
        // dimension == days it lasts * 100
        ticks /= 1200.0f * dimension / 100.0f;
        // true power use
        float power_rate = 1.0f;
        // check the actual load
        if (auto mptr = df::machine::find(machine.machine_id))
        {
            if (mptr->cur_power >= mptr->min_power)
                power_rate = float(mptr->min_power) / mptr->cur_power;
            else
                power_rate = 0.0f;
        }
        // waste rate: 1-10% depending on piston assembly quality
        float piston_qual = get_component_quality(1);
        float waste = 0.1f - 0.016f * 0.5f * (piston_qual + get_component_quality(2));
        float efficiency_coeff = 1.0f - 0.02f * piston_qual;
        // apply rate and waste factor
        ticks *= (waste + 0.9f*power_rate)*power_level*efficiency_coeff;
        // end result
        return std::max(1, int(ticks));
    }

    void update_under_construction(steam_engine_workshop *engine)
    {
        if (machine.machine_id != -1)
            return;

        int cur_count = 0;

        if (auto first = collect_steam(engine, &cur_count))
        {
            if (add_wear_nodestroy(first, WEAR_TICKS*4/10))
            {
                boil_unit(first);
                cur_count--;
            }
        }

        set_steam_amount(cur_count);
    }

    void update_working(steam_engine_workshop *engine)
    {
        int old_count = get_steam_amount();
        int old_power = std::min(engine->max_power, old_count);
        int cur_count = 0;

        if (auto first = collect_steam(engine, &cur_count))
        {
            int rate = get_steam_use_rate(engine, first->dimension, old_power);

            if (add_wear_nodestroy(first, rate))
            {
                boil_unit(first);
                cur_count--;
            }

            if (check_component_wear(engine, old_count, old_power))
                return;
        }

        if (old_count < engine->max_capacity && cur_count == engine->max_capacity)
            suspend_jobs(true);
        else if (cur_count <= engine->max_power+1 && old_count > engine->max_power+1)
            suspend_jobs(false);

        set_steam_amount(cur_count);

        int cur_power = std::min(engine->max_power, cur_count);
        if (cur_power != old_power)
        {
            auto mptr = df::machine::find(machine.machine_id);
            if (mptr)
                mptr->cur_power += (cur_power - old_power)*100;
        }
    }

    // Furnaces need architecture, and this is a workshop
    // only because furnaces cannot connect to machines.
    DEFINE_VMETHOD_INTERPOSE(bool, needsDesign, ())
    {
        if (get_steam_engine())
            return true;

        return INTERPOSE_NEXT(needsDesign)();
    }

    // Machine interface
    DEFINE_VMETHOD_INTERPOSE(void, getPowerInfo, (df::power_info *info))
    {
        if (auto engine = get_steam_engine())
        {
            info->produced = std::min(engine->max_power, get_steam_amount())*100;
            info->consumed = 10 - int(get_component_quality(0));
            return;
        }

        INTERPOSE_NEXT(getPowerInfo)(info);
    }

    DEFINE_VMETHOD_INTERPOSE(df::machine_info*, getMachineInfo, ())
    {
        if (get_steam_engine())
            return &machine;

        return INTERPOSE_NEXT(getMachineInfo)();
    }

    DEFINE_VMETHOD_INTERPOSE(bool, isPowerSource, ())
    {
        if (get_steam_engine())
            return true;

        return INTERPOSE_NEXT(isPowerSource)();
    }

    DEFINE_VMETHOD_INTERPOSE(void, categorize, (bool free))
    {
        if (get_steam_engine())
        {
            auto &vec = world->buildings.other[buildings_other_id::ANY_MACHINE];
            insert_into_vector(vec, &df::building::id, (df::building*)this);
        }

        INTERPOSE_NEXT(categorize)(free);
    }

    DEFINE_VMETHOD_INTERPOSE(void, uncategorize, ())
    {
        if (get_steam_engine())
        {
            auto &vec = world->buildings.other[buildings_other_id::ANY_MACHINE];
            erase_from_vector(vec, &df::building::id, id);
        }

        INTERPOSE_NEXT(uncategorize)();
    }

    DEFINE_VMETHOD_INTERPOSE(bool, canConnectToMachine, (df::machine_tile_set *info))
    {
        if (auto engine = get_steam_engine())
        {
            int real_cx = centerx, real_cy = centery;
            bool ok = false;

            for (size_t i = 0; i < engine->gear_tiles.size(); i++)
            {
                // the original function connects to the center tile
                centerx = x1 + engine->gear_tiles[i].x;
                centery = y1 + engine->gear_tiles[i].y;

                if (!INTERPOSE_NEXT(canConnectToMachine)(info))
                    continue;

                ok = true;
                break;
            }

            centerx = real_cx; centery = real_cy;
            return ok;
        }
        else
            return INTERPOSE_NEXT(canConnectToMachine)(info);
    }

    // Operation logic
    DEFINE_VMETHOD_INTERPOSE(bool, isUnpowered, ())
    {
        if (auto engine = get_steam_engine())
        {
            df::coord water, magma;
            return !find_liquids(&water, &magma, engine->is_magma, 3);
        }

        return INTERPOSE_NEXT(isUnpowered)();
    }

    DEFINE_VMETHOD_INTERPOSE(void, updateAction, ())
    {
        if (auto engine = get_steam_engine())
        {
            if (is_fully_built())
                update_working(engine);
            else
                update_under_construction(engine);

            if (flags.bits.almost_deleted)
                return;
        }

        INTERPOSE_NEXT(updateAction)();
    }

    DEFINE_VMETHOD_INTERPOSE(void, drawBuilding, (df::building_drawbuffer *db, int16_t unk))
    {
        INTERPOSE_NEXT(drawBuilding)(db, unk);

        if (auto engine = get_steam_engine())
        {
            if (!is_fully_built())
                return;

            // If machine is running, tweak gear assemblies
            auto mptr = df::machine::find(machine.machine_id);
            if (mptr && (mptr->visual_phase & 1) != 0)
            {
                for (size_t i = 0; i < engine->gear_tiles.size(); i++)
                {
                    auto pos = engine->gear_tiles[i];
                    db->tile[pos.x][pos.y] = 42;
                }
            }

            // Use the hearth color to display power level
            if (engine->hearth_tile.isValid())
            {
                auto pos = engine->hearth_tile;
                int power = std::min(engine->max_power, get_steam_amount());
                db->fore[pos.x][pos.y] = hearth_colors[power][0];
                db->bright[pos.x][pos.y] = hearth_colors[power][1];
            }

            // Set liquid indicator state
            if (engine->water_tile.isValid() || engine->magma_tile.isValid())
            {
                df::coord water, magma;
                find_liquids(&water, &magma, engine->is_magma, 3);
                df::coord dwater, dmagma;
                find_liquids(&dwater, &dmagma, engine->is_magma, 5);

                if (engine->water_tile.isValid())
                {
                    if (!water.isValid())
                        db->fore[engine->water_tile.x][engine->water_tile.y] = 0;
                    else if (!dwater.isValid())
                        db->bright[engine->water_tile.x][engine->water_tile.y] = 0;
                }
                if (engine->magma_tile.isValid() && engine->is_magma)
                {
                    if (!magma.isValid())
                        db->fore[engine->magma_tile.x][engine->magma_tile.y] = 0;
                    else if (!dmagma.isValid())
                        db->bright[engine->magma_tile.x][engine->magma_tile.y] = 0;
                }
            }
        }
    }

    DEFINE_VMETHOD_INTERPOSE(void, deconstructItems, (bool noscatter, bool lost))
    {
        if (get_steam_engine())
        {
            // Explode if any steam left
            if (int amount = get_steam_amount())
            {
                make_explosion(
                    df::coord((x1+x2)/2, (y1+y2)/2, z),
                    40 + amount * 20
                );

                random_boil();
            }
        }

        INTERPOSE_NEXT(deconstructItems)(noscatter, lost);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, needsDesign);
IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, getPowerInfo);
IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, getMachineInfo);
IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, isPowerSource);
IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, categorize);
IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, uncategorize);
IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, canConnectToMachine);
IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, isUnpowered);
IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, updateAction);
IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, drawBuilding);
IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, deconstructItems);

/*
 * Hook for the dwarfmode screen. Tweaks the build menu
 * behavior to suit the steam engine building more.
 */

struct dwarfmode_hook : df::viewscreen_dwarfmodest
{
    typedef df::viewscreen_dwarfmodest interpose_base;

    steam_engine_workshop *get_steam_engine()
    {
        if (ui->main.mode == ui_sidebar_mode::Build &&
            ui_build_selector->stage == 1 &&
            ui_build_selector->building_type == building_type::Workshop &&
            ui_build_selector->building_subtype == workshop_type::Custom)
        {
            return find_steam_engine(ui_build_selector->custom_type);
        }

        return NULL;
    }

    void check_hanging_tiles(steam_engine_workshop *engine)
    {
        if (!engine) return;

        bool error = false;

        int x1 = cursor->x - engine->def->workloc_x;
        int y1 = cursor->y - engine->def->workloc_y;

        for (int x = 0; x < engine->def->dim_x; x++)
        {
            for (int y = 0; y < engine->def->dim_y; y++)
            {
                if (ui_build_selector->tiles[x][y] >= 5)
                    continue;

                auto ptile = Maps::getTileType(x1+x,y1+y,cursor->z);
                if (ptile && !isOpenTerrain(*ptile))
                    continue;

                ui_build_selector->tiles[x][y] = 6;
                error = true;
            }
        }

        if (error)
        {
            const char *msg = "Hanging - cover channels with down stairs.";
            ui_build_selector->errors.push_back(new std::string(msg));
        }
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        steam_engine_workshop *engine = get_steam_engine();

        // Selector insists that workshops cannot be placed hanging
        // unless they require magma, so pretend we always do.
        if (engine)
            engine->def->needs_magma = true;

        INTERPOSE_NEXT(feed)(input);

        // Restore the flag
        if (engine)
            engine->def->needs_magma = engine->is_magma;

        // And now, check for open space. Since these workshops
        // are machines, they will collapse over true open space.
        check_hanging_tiles(get_steam_engine());
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(dwarfmode_hook, feed);

/*
 * Scan raws for matching workshop buildings.
 */

static bool find_engines(color_ostream &out)
{
    engines.clear();

    auto &wslist = world->raws.buildings.workshops;

    for (size_t i = 0; i < wslist.size(); i++)
    {
        if (strstr(wslist[i]->code.c_str(), "STEAM_ENGINE") == NULL)
            continue;

        steam_engine_workshop ws;
        ws.def = wslist[i];
        ws.id = ws.def->id;

        int bs = ws.def->build_stages;
        for (int x = 0; x < ws.def->dim_x; x++)
        {
            for (int y = 0; y < ws.def->dim_y; y++)
            {
                switch (ws.def->tile[bs][x][y])
                {
                case 15:
                    ws.gear_tiles.push_back(df::coord2d(x,y));
                    break;
                case 19:
                    ws.hearth_tile = df::coord2d(x,y);
                    break;
                }

                if (ws.def->tile_color[2][bs][x][y])
                {
                    switch (ws.def->tile_color[0][bs][x][y])
                    {
                    case 1:
                        ws.water_tile = df::coord2d(x,y);
                        break;
                    case 4:
                        ws.magma_tile = df::coord2d(x,y);
                        break;
                    }
                }
            }
        }

        ws.is_magma = ws.def->needs_magma;
        ws.max_power = ws.is_magma ? 5 : 3;
        ws.max_capacity = ws.is_magma ? 10 : 6;
        ws.wear_temp = ws.is_magma ? 12000 : 11000;

        if (!ws.gear_tiles.empty())
            engines.push_back(ws);
        else
            out.printerr("%s has no gear tiles - ignoring.\n", wslist[i]->code.c_str());
    }

    return !engines.empty();
}

DFHACK_PLUGIN_IS_ENABLED(is_enabled);

static void enable_hooks(bool enable)
{
    is_enabled = enable;

    INTERPOSE_HOOK(liquid_hook, getItemDescription).apply(enable);
    INTERPOSE_HOOK(liquid_hook, adjustTemperature).apply(enable);
    INTERPOSE_HOOK(liquid_hook, checkTemperatureDamage).apply(enable);

    INTERPOSE_HOOK(workshop_hook, needsDesign).apply(enable);
    INTERPOSE_HOOK(workshop_hook, getPowerInfo).apply(enable);
    INTERPOSE_HOOK(workshop_hook, getMachineInfo).apply(enable);
    INTERPOSE_HOOK(workshop_hook, isPowerSource).apply(enable);
    INTERPOSE_HOOK(workshop_hook, categorize).apply(enable);
    INTERPOSE_HOOK(workshop_hook, uncategorize).apply(enable);
    INTERPOSE_HOOK(workshop_hook, canConnectToMachine).apply(enable);
    INTERPOSE_HOOK(workshop_hook, isUnpowered).apply(enable);
    INTERPOSE_HOOK(workshop_hook, updateAction).apply(enable);
    INTERPOSE_HOOK(workshop_hook, drawBuilding).apply(enable);
    INTERPOSE_HOOK(workshop_hook, deconstructItems).apply(enable);

    INTERPOSE_HOOK(dwarfmode_hook, feed).apply(enable);
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        if (World::isFortressMode() && find_engines(out))
        {
            out.print("Detected steam engine workshops - enabling plugin.\n");
            enable_hooks(true);
        }
        else
            enable_hooks(false);
        break;
    case SC_MAP_UNLOADED:
        enable_hooks(false);
        engines.clear();
        break;
    default:
        break;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (Core::getInstance().isWorldLoaded())
        plugin_onstatechange(out, SC_WORLD_LOADED);

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    enable_hooks(false);
    return CR_OK;
}
