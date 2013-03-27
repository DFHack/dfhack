#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <Error.h>
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
#include "df/building_trapst.h"
#include "df/builtin_mats.h"
#include "df/world.h"
#include "df/buildings_other_id.h"
#include "df/machine.h"
#include "df/machine_info.h"
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

DFHACK_PLUGIN("power-meter");

static const uint32_t METER_BIT = 0x80000000U;

static void init_plate_info(df::pressure_plate_info &plate_info)
{
    plate_info.water_min = 1;
    plate_info.water_max = 7;
    plate_info.flags.whole = METER_BIT;
    plate_info.flags.bits.water = true;
    plate_info.flags.bits.resets = true;
}

/*
 * Hook for the pressure plate itself. Implements core logic.
 */

struct trap_hook : df::building_trapst {
    typedef df::building_trapst interpose_base;

    // Engine detection

    bool is_power_meter()
    {
        return trap_type == trap_type::PressurePlate &&
               (plate_info.flags.whole & METER_BIT) != 0;
    }

    inline bool is_fully_built()
    {
        return getBuildStage() >= getMaxBuildStage();
    }

    DEFINE_VMETHOD_INTERPOSE(void, getName, (std::string *buf))
    {
        if (is_power_meter())
        {
            buf->clear();
            *buf += "Power Meter";
            return;
        }

        INTERPOSE_NEXT(getName)(buf);
    }

    DEFINE_VMETHOD_INTERPOSE(void, updateAction, ())
    {
        if (is_power_meter())
        {
            auto pdsgn = Maps::getTileDesignation(centerx,centery,z);

            if (pdsgn)
            {
                bool active = false;
                auto &gears = world->buildings.other[buildings_other_id::GEAR_ASSEMBLY];

                for (size_t i = 0; i < gears.size(); i++)
                {
                    // Adjacent
                    auto gear = gears[i];
                    int deltaxy = abs(centerx - gear->centerx) + abs(centery - gear->centery);
                    if (gear->z != z || deltaxy != 1)
                        continue;
                    // Linked to machine
                    auto info = gears[i]->getMachineInfo();
                    if (!info || info->machine_id < 0)
                        continue;
                    // an active machine
                    auto machine = df::machine::find(info->machine_id);
                    if (!machine || !machine->flags.bits.active)
                        continue;
                    // with adequate power?
                    int power = machine->cur_power - machine->min_power;
                    if (power < 0 || machine->cur_power <= 0)
                        continue;
                    if (power < plate_info.track_min)
                        continue;
                    if (power > plate_info.track_max && plate_info.track_max >= 0)
                        continue;

                    active = true;
                    break;
                }

                if (plate_info.flags.bits.citizens)
                    active = !active;

                // Temporarily set the tile water amount based on power state
                auto old_dsgn = *pdsgn;
                pdsgn->bits.liquid_type = tile_liquid::Water;
                pdsgn->bits.flow_size = (active ? 7 : 0);

                INTERPOSE_NEXT(updateAction)();

                *pdsgn = old_dsgn;
                return;
            }
        }

        INTERPOSE_NEXT(updateAction)();
    }

    DEFINE_VMETHOD_INTERPOSE(void, drawBuilding, (df::building_drawbuffer *db, void *unk))
    {
        INTERPOSE_NEXT(drawBuilding)(db, unk);

        if (is_power_meter() && is_fully_built())
        {
            db->fore[0][0] = 3;
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(trap_hook, getName);
IMPLEMENT_VMETHOD_INTERPOSE(trap_hook, updateAction);
IMPLEMENT_VMETHOD_INTERPOSE(trap_hook, drawBuilding);

static bool enabled = false;

static void enable_hooks(bool enable)
{
    enabled = enable;

    INTERPOSE_HOOK(trap_hook, getName).apply(enable);
    INTERPOSE_HOOK(trap_hook, updateAction).apply(enable);
    INTERPOSE_HOOK(trap_hook, drawBuilding).apply(enable);
}

static bool makePowerMeter(df::pressure_plate_info *info, int min_power, int max_power, bool invert)
{
    CHECK_NULL_POINTER(info);

    if (!enabled)
    {
        auto entry = World::GetPersistentData("power-meter/enabled", NULL);
        if (!entry.isValid())
            return false;

        enable_hooks(true);
    }

    init_plate_info(*info);
    info->track_min = min_power;
    info->track_max = max_power;
    info->flags.bits.citizens = invert;
    return true;
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(makePowerMeter),
    DFHACK_LUA_END
};

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_WORLD_LOADED:
        {
            bool enable = World::GetPersistentData("power-meter/enabled").isValid();

            if (enable)
            {
                out.print("Enabling the power meter plugin.\n");
                enable_hooks(true);
            }
        }
        break;
    case SC_WORLD_UNLOADED:
        enable_hooks(false);
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
