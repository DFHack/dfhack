#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <modules/Gui.h>
#include <modules/Screen.h>
#include <modules/Maps.h>
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

using std::vector;
using std::string;
using std::stack;
using namespace DFHack;
using namespace df::enums;

using df::global::gps;
using df::global::world;
using df::global::ui;
using df::global::ui_build_selector;

DFHACK_PLUGIN("steam-engine");

struct steam_engine_workshop {
    int id;
    df::building_def_workshopst *def;
    bool is_magma;
    int max_power, max_capacity;
    int wear_temp;
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

    int nsize = std::max(0, pldes->bits.flow_size - amount);
    pldes->bits.flow_size = nsize;
    pldes->bits.flow_forbid = (nsize > 3 || pldes->bits.liquid_type == tile_liquid::Magma);

    enable_updates_at(pos, true, false);
}

bool make_explosion(df::coord pos, int mat_type, int mat_index, int density)
{
    using df::global::flows;

    auto block = Maps::getTileBlock(pos);
    if (!flows || !block)
        return false;

    auto flow = new df::flow_info();
    flow->type = flow_type::MaterialDust;
    flow->mat_type = mat_type;
    flow->mat_index = mat_index;
    flow->density = std::min(100, density);
    flow->pos = pos;

    block->flows.push_back(flow);
    flows->push_back(flow);
    return true;
}

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
            temperature = std::max(int(temperature), getBoilingPoint()-1);

        return INTERPOSE_NEXT(checkTemperatureDamage)();
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(liquid_hook, getItemDescription);
IMPLEMENT_VMETHOD_INTERPOSE(liquid_hook, adjustTemperature);
IMPLEMENT_VMETHOD_INTERPOSE(liquid_hook, checkTemperatureDamage);

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

    bool find_liquids(df::coord *pwater, df::coord *pmagma, bool is_magma, bool any_level)
    {
        if (!is_magma)
            pmagma = NULL;

        for (int x = x1; x <= x2; x++)
        {
            for (int y = y1; y <= y2; y++)
            {
                auto ptile = Maps::getTileType(x,y,z);
                if (!ptile || !LowPassable(*ptile))
                    continue;

                auto pltile = Maps::getTileType(x,y,z-1);
                if (!pltile || !FlowPassable(*pltile))
                    continue;

                auto pldes = Maps::getTileDesignation(x,y,z-1);
                if (!pldes || pldes->bits.flow_size == 0)
                    continue;

                if (pldes->bits.liquid_type == tile_liquid::Magma)
                {
                    if (!pmagma || (!any_level && pldes->bits.flow_size < 4))
                        continue;

                    *pmagma = df::coord(x,y,z-1);
                    if (pwater->isValid())
                        return true;
                }
                else
                {
                    if (!any_level && pldes->bits.flow_size < 3)
                        continue;

                    *pwater = df::coord(x,y,z-1);
                    if (!pmagma || pmagma->isValid())
                        return true;
                }
            }
        }

        return false;
    }

    bool absorb_unit(steam_engine_workshop *engine, df::item_liquid_miscst *liquid)
    {
        df::coord water, magma;

        if (!find_liquids(&water, &magma, engine->is_magma, true))
        {
            liquid->addWear(WEAR_TICKS*4+1, true, false);
            return false;
        }

        decrement_flow(water, 1);
        if (engine->is_magma)
            decrement_flow(magma, 1);

        liquid->flags.bits.in_building = true;
        liquid->mat_state.whole |= liquid_hook::BOILING_FLAG;
        liquid->temperature = liquid->getBoilingPoint()-1;
        liquid->temperature_fraction = 0;

        // This affects where the steam appears to come from
        if (engine->hearth_tile.isValid())
            liquid->pos = df::coord(x1+engine->hearth_tile.x, y1+engine->hearth_tile.y, z);

        enable_updates_at(liquid->pos, false, true);
        return true;
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

            if (cnt == 0 || random() < RAND_MAX/2)
            {
                cnt++;
                boil_unit(liquid);
            }
        }
    }

    void explode()
    {
        int mat_type = builtin_mats::ASH, mat_index = -1;
        int cx = (x1+x2)/2, cy = (y1+y2)/2;
        int power = std::min(240, get_steam_amount()*80);

        make_explosion(df::coord(cx, cy, z), mat_type, mat_index, power);
        make_explosion(df::coord(cx-1, cy, z), mat_type, mat_index, power/3);
        make_explosion(df::coord(cx, cy-1, z), mat_type, mat_index, power/3);
        make_explosion(df::coord(cx+1, cy, z), mat_type, mat_index, power/3);
        make_explosion(df::coord(cx, cy+1, z), mat_type, mat_index, power/3);

        *df::global::pause_state = true;

        Gui::showAnnouncement("A boiler has exploded!", COLOR_RED, true);
        auto ann = world->status.announcements.back();
        ann->type = announcement_type::CAVE_COLLAPSE;
        ann->pos = df::coord(cx, cy, z);
    }

    bool check_component_wear(steam_engine_workshop *engine, int count, int power)
    {
        for (int i = contained_items.size()-1; i >= 0; i--)
        {
            auto item = contained_items[i];
            if (item->use_mode != 2)
                continue;

            int melt_temp = item->item->getMeltingPoint();
            if (melt_temp >= engine->wear_temp)
                continue;
            if (item->item->isBuildMat())
                continue;

            auto type = item->item->getType();
            if (type == item_type::TRAPPARTS || item_type::CHAIN)
                continue;

            int coeff = power;
            if (type == item_type::BARREL)
                coeff = count;

            int ticks = coeff*(engine->wear_temp - melt_temp);

            if (item->item->addWear(ticks, true, true))
            {
                explode();
                return true;
            }
        }

        return false;
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

    // Operation logic
    DEFINE_VMETHOD_INTERPOSE(bool, isUnpowered, ())
    {
        if (auto engine = get_steam_engine())
        {
            df::coord water, magma;
            return !find_liquids(&water, &magma, engine->is_magma, false);
        }

        return INTERPOSE_NEXT(isUnpowered)();
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

            // Set liquid indicator state
            if (engine->water_tile.isValid() || engine->magma_tile.isValid())
            {
                df::coord water, magma;
                find_liquids(&water, &magma, engine->is_magma, false);

                if (engine->water_tile.isValid() && !water.isValid())
                    db->fore[engine->water_tile.x][engine->water_tile.y] = 0;
                if (engine->magma_tile.isValid() && engine->is_magma && !magma.isValid())
                    db->fore[engine->magma_tile.x][engine->magma_tile.y] = 0;
            }
        }
    }

    DEFINE_VMETHOD_INTERPOSE(void, deconstructItems, (bool noscatter, bool lost))
    {
        if (get_steam_engine())
            random_boil();

        if (lost)
            explode();

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
        using df::global::cursor;

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
            const char *msg = "Hanging - use down stair.";
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

        // And now, check for open space
        check_hanging_tiles(get_steam_engine());
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(dwarfmode_hook, feed);

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
        ws.wear_temp = ws.is_magma ? 12000 : 11000;

        if (!ws.gear_tiles.empty())
            engines.push_back(ws);
    }

    return !engines.empty();
}

static void enable_hooks(bool enable)
{
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
