#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <modules/Gui.h>
#include <modules/Screen.h>
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

#include "MiscUtils.h"

using std::vector;
using std::string;
using std::stack;
using namespace DFHack;
using namespace df::enums;

using df::global::gps;
using df::global::world;
using df::global::ui_build_selector;

DFHACK_PLUGIN("steam-engine");

struct steam_engine_workshop {
    int id;
    df::building_def_workshopst *def;
    bool is_magma;
    int max_power, max_capacity;
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

static const int hearth_colors[6][2] = {
    { COLOR_BLACK, 1 },
    { COLOR_BROWN, 0 },
    { COLOR_RED, 0 },
    { COLOR_RED, 1 },
    { COLOR_BROWN, 1 },
    { COLOR_GREY, 1 }
};

struct liquid_hook : df::item_liquid_miscst {
    typedef df::item_liquid_miscst interpose_base;

    static const uint32_t BOILING_FLAG = 0x80000000U;

    DEFINE_VMETHOD_INTERPOSE(void, getItemDescription, (std::string *buf, int8_t mode))
    {
        if (mat_state.whole & BOILING_FLAG)
            buf->append("boiling ");

        INTERPOSE_NEXT(getItemDescription)(buf, mode);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(liquid_hook, getItemDescription);

struct workshop_hook : df::building_workshopst {
    typedef df::building_workshopst interpose_base;

    steam_engine_workshop *get_steam_engine()
    {
        if (type == workshop_type::Custom)
            return find_steam_engine(custom_type);

        return NULL;
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

    void absorb_unit(steam_engine_workshop *engine, df::item_liquid_miscst *liquid)
    {
        liquid->flags.bits.in_building = true;
        liquid->mat_state.whole |= liquid_hook::BOILING_FLAG;

        // This affects where the steam appears to come from
        if (engine->hearth_tile.isValid())
            liquid->pos = df::coord(x1+engine->hearth_tile.x, y1+engine->hearth_tile.y, z);
    }

    bool boil_unit(df::item_liquid_miscst *liquid)
    {
        liquid->wear = 4;
        liquid->flags.bits.in_building = false;
        liquid->temperature = liquid->getBoilingPoint() + 10;

        return liquid->checkMeltBoil();
    }

    void suspend_jobs(bool suspend)
    {
        for (size_t i = 0; i < jobs.size(); i++)
            if (jobs[i]->job_type == job_type::CustomReaction)
                jobs[i]->flags.bits.suspend = suspend;
    }

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

                absorb_unit(engine, liquid);
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
            }
        }

        return first;
    }

    static const int WEAR_TICKS = 806400;

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
        // apply rate; 10% steam is wasted anyway
        ticks *= (0.1f + 0.9f*power_rate)*power_level;
        // end result
        return std::max(1, int(ticks));
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
            info->consumed = 10;
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

    DEFINE_VMETHOD_INTERPOSE(void, updateAction, ())
    {
        if (auto engine = get_steam_engine())
        {
            int old_count = get_steam_amount();
            int old_power = std::min(engine->max_power, old_count);
            int cur_count = 0;

            if (auto first = collect_steam(engine, &cur_count))
            {
                int rate = get_steam_use_rate(engine, first->dimension, old_power);

                if (first->incWearTimer(rate))
                {
                    while (first->wear_timer >= WEAR_TICKS)
                    {
                        first->wear_timer -= WEAR_TICKS;
                        first->wear++;
                    }

                    if (first->wear > 3)
                    {
                        boil_unit(first);
                        cur_count--;
                    }
                }
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

        INTERPOSE_NEXT(updateAction)();
    }

    DEFINE_VMETHOD_INTERPOSE(void, drawBuilding, (df::building_drawbuffer *db, void *unk))
    {
        INTERPOSE_NEXT(drawBuilding)(db, unk);

        if (auto engine = get_steam_engine())
        {
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
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, needsDesign);
IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, getPowerInfo);
IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, getMachineInfo);
IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, isPowerSource);
IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, categorize);
IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, uncategorize);
IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, canConnectToMachine);
IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, updateAction);
IMPLEMENT_VMETHOD_INTERPOSE(workshop_hook, drawBuilding);

static bool find_engines()
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
                if (ws.def->tile[bs][x][y] == 15)
                    ws.gear_tiles.push_back(df::coord2d(x,y));

                if (ws.def->tile_color[2][bs][x][y])
                {
                    switch (ws.def->tile_color[0][bs][x][y])
                    {
                    case 0:
                        ws.hearth_tile = df::coord2d(x,y);
                        break;
                    case 1:
                        ws.water_tile = df::coord2d(x,y);
                        break;
                    case 4:
                        ws.magma_tile = df::coord2d(x,y);
                        break;
                    default:
                        break;
                    }
                }
            }
        }

        ws.is_magma = ws.def->needs_magma;
        ws.max_power = ws.is_magma ? 5 : 3;
        ws.max_capacity = ws.is_magma ? 10 : 6;

        if (!ws.gear_tiles.empty())
            engines.push_back(ws);
    }

    return !engines.empty();
}

static void enable_hooks(bool enable)
{
    INTERPOSE_HOOK(liquid_hook, getItemDescription).apply(enable);

    INTERPOSE_HOOK(workshop_hook, needsDesign).apply(enable);
    INTERPOSE_HOOK(workshop_hook, getPowerInfo).apply(enable);
    INTERPOSE_HOOK(workshop_hook, getMachineInfo).apply(enable);
    INTERPOSE_HOOK(workshop_hook, isPowerSource).apply(enable);
    INTERPOSE_HOOK(workshop_hook, categorize).apply(enable);
    INTERPOSE_HOOK(workshop_hook, uncategorize).apply(enable);
    INTERPOSE_HOOK(workshop_hook, canConnectToMachine).apply(enable);
    INTERPOSE_HOOK(workshop_hook, updateAction).apply(enable);
    INTERPOSE_HOOK(workshop_hook, drawBuilding).apply(enable);
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        if (find_engines())
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
    if (Core::getInstance().isMapLoaded())
        plugin_onstatechange(out, SC_MAP_LOADED);

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    enable_hooks(false);
    return CR_OK;
}
