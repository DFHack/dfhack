#include "Core.h"
#include "Console.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Units.h"
#include "modules/Maps.h"

#include "df/map_block.h"
#include "df/unit.h"
#include "df/unit_action.h"
#include "df/unit_relationship_type.h"
#include "df/units_other_id.h"
#include "df/world.h"
#include "df/action_type_group.h"

using std::string;
using std::vector;

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("fastdwarf");
DFHACK_PLUGIN_IS_ENABLED(active);
REQUIRE_GLOBAL(world);
using df::global::debug_turbospeed;  // not required

static bool enable_fastdwarf = false;
static bool enable_teledwarf = false;

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    if (debug_turbospeed)
        *debug_turbospeed = false;
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    // do we even need to do anything at all?
    if (!enable_fastdwarf && !enable_teledwarf)
        return CR_OK;

    // make sure the world is actually loaded
    if (!world || !world->map.block_index)
    {
        enable_fastdwarf = enable_teledwarf = false;
        return CR_OK;
    }

    for (size_t i = 0; i < world->units.active.size(); i++)
    {
        df::unit *unit = world->units.active[i];
        // citizens only
        if (!Units::isCitizen(unit))
            continue;

        if (enable_teledwarf) do
        {
            // skip dwarves that are dragging creatures or being dragged
            if ((unit->relationship_ids[df::unit_relationship_type::Draggee] != -1) ||
                (unit->relationship_ids[df::unit_relationship_type::Dragger] != -1))
                break;

            // skip dwarves that are following other units
            if (unit->following != 0)
                break;

            // skip unconscious units
            if (unit->counters.unconscious > 0)
                break;

            // don't do anything if the dwarf isn't going anywhere
            if (!unit->pos.isValid() || !unit->path.dest.isValid() || unit->pos == unit->path.dest) {
                break;
            }

            if (!Units::teleport(unit, unit->path.dest))
                break;

            unit->path.path.clear();
        } while (0);

        if (enable_fastdwarf)
        {
            Units::setActionTimerCategory(unit, 1, df::action_type_group::All);
        }
    }
    return CR_OK;
}

static command_result fastdwarf (color_ostream &out, vector <string> & parameters)
{
    if (parameters.size() > 2)
        return CR_WRONG_USAGE;

    if ((parameters.size() == 1) || (parameters.size() == 2))
    {
        if (parameters.size() == 2)
        {
            if (parameters[1] == "0")
                enable_teledwarf = false;
            else if (parameters[1] == "1")
                enable_teledwarf = true;
            else
                return CR_WRONG_USAGE;
        }
        else
            enable_teledwarf = false;
        if (parameters[0] == "0")
        {
            enable_fastdwarf = false;
            if (debug_turbospeed)
                *debug_turbospeed = false;
        }
        else if (parameters[0] == "1")
        {
            enable_fastdwarf = true;
            if (debug_turbospeed)
                *debug_turbospeed = false;
        }
        else if (parameters[0] == "2")
        {
            if (debug_turbospeed)
            {
                enable_fastdwarf = false;
                *debug_turbospeed = true;
            }
            else
            {
                out.print("Speed level 2 not available.\n");
                return CR_FAILURE;
            }
        }
        else
            return CR_WRONG_USAGE;
    }

    active = enable_fastdwarf || enable_teledwarf;

    out.print("Current state: fast = %d, teleport = %d.\n",
        (debug_turbospeed && *debug_turbospeed) ? 2 : (enable_fastdwarf ? 1 : 0),
        enable_teledwarf ? 1 : 0);

    return CR_OK;
}

DFhackCExport command_result plugin_enable ( color_ostream &out, bool enable )
{
    if (active != enable)
    {
        active = enable_fastdwarf = enable;
        enable_teledwarf = false;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "fastdwarf",
        "Dwarves teleport and/or finish jobs instantly.",
        fastdwarf));

    return CR_OK;
}
