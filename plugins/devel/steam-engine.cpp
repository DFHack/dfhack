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
    std::vector<df::coord2d> gear_tiles;
    df::coord2d hearth_tile;
    df::coord2d water_tile;
    df::coord2d magma_tile;
};

std::vector<steam_engine_workshop> engines;

struct workshop_hook : df::building_workshopst {
    typedef df::building_workshopst interpose_base;

    steam_engine_workshop *get_steam_engine()
    {
        if (type == workshop_type::Custom)
            for (size_t i = 0; i < engines.size(); i++)
                if (engines[i].id == custom_type)
                    return &engines[i];

        return NULL;
    }

    int get_steam_amount()
    {
        int cnt = 0;

        for (size_t i = 0; i < contained_items.size(); i++)
        {
            if (contained_items[i]->use_mode == 0 &&
                contained_items[i]->item->flags.bits.in_building)
                cnt++;
        }

        return cnt;
    }

    int get_power_output(steam_engine_workshop *engine)
    {
        int maxv = engine->def->needs_magma ? 5 : 3;
        return std::min(get_steam_amount(), maxv)*100;
    }

    df::item_liquid_miscst *collect_steam()
    {
        df::item_liquid_miscst *first = NULL;

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
                    liquid->dimension != 333 ||
                    liquid->wear != 0)
                    continue;

                liquid->flags.bits.in_building = true;
            }

            first = liquid;
        }

        return first;
    }

    DEFINE_VMETHOD_INTERPOSE(bool, needsDesign, ())
    {
        if (get_steam_engine())
            return true;

        return INTERPOSE_NEXT(needsDesign)();
    }

    DEFINE_VMETHOD_INTERPOSE(void, getPowerInfo, (df::power_info *info))
    {
        if (auto engine = get_steam_engine())
        {
            info->produced = get_power_output(engine);
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
            int output = get_power_output(engine);

            if (auto first = collect_steam())
            {
                if (first->incWearTimer(output))
                {
                    while (first->wear_timer >= 806400)
                    {
                        first->wear_timer -= 806400;
                        first->wear++;
                    }

                    if (first->wear > 3)
                    {
                        first->flags.bits.in_building = 0;
                        first->temperature = first->getBoilingPoint()+50;
                    }
                }
            }

            int new_out = get_power_output(engine);
            if (new_out != output)
            {
                auto mptr = df::machine::find(machine.machine_id);
                if (mptr)
                    mptr->cur_power += (new_out - output);
            }
        }

        INTERPOSE_NEXT(updateAction)();
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

static void find_engines()
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

        engines.push_back(ws);
    }
}

static void enable_hooks()
{
    INTERPOSE_HOOK(workshop_hook, needsDesign).apply();
    INTERPOSE_HOOK(workshop_hook, getPowerInfo).apply();
    INTERPOSE_HOOK(workshop_hook, getMachineInfo).apply();
    INTERPOSE_HOOK(workshop_hook, isPowerSource).apply();
    INTERPOSE_HOOK(workshop_hook, categorize).apply();
    INTERPOSE_HOOK(workshop_hook, uncategorize).apply();
    INTERPOSE_HOOK(workshop_hook, canConnectToMachine).apply();
    INTERPOSE_HOOK(workshop_hook, updateAction).apply();
}

static void disable_hooks()
{
    INTERPOSE_HOOK(workshop_hook, needsDesign).remove();
    INTERPOSE_HOOK(workshop_hook, getPowerInfo).remove();
    INTERPOSE_HOOK(workshop_hook, getMachineInfo).remove();
    INTERPOSE_HOOK(workshop_hook, isPowerSource).remove();
    INTERPOSE_HOOK(workshop_hook, categorize).remove();
    INTERPOSE_HOOK(workshop_hook, uncategorize).remove();
    INTERPOSE_HOOK(workshop_hook, canConnectToMachine).remove();
    INTERPOSE_HOOK(workshop_hook, updateAction).remove();
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        find_engines();
        if (!engines.empty())
        {
            out.print("Detected steam engine workshops - enabling plugin.\n");
            enable_hooks();
        }
        break;
    case SC_MAP_UNLOADED:
        disable_hooks();
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
    disable_hooks();
    return CR_OK;
}
