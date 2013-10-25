#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Units.h"
#include "modules/Maps.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/unit.h"
#include "df/map_block.h"

using std::string;
using std::vector;
using namespace DFHack;

using df::global::world;

// dfhack interface
DFHACK_PLUGIN("fastdwarf");

DFHACK_PLUGIN_IS_ENABLED(active);

static bool enable_fastdwarf = false;
static bool enable_teledwarf = false;

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    if (df::global::debug_turbospeed)
        *df::global::debug_turbospeed = false;
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

        if (enable_fastdwarf && !enable_teledwarf )
        {
            if (unit->counters.job_counter > 0)
                unit->counters.job_counter = 0;
            // could also patch the unit->job.current_job->completion_timer
        }

        if (enable_teledwarf) do
        {
            // don't do anything if the dwarf isn't going anywhere
            if (!unit->pos.isValid() || !unit->path.dest.isValid() || unit->pos == unit->path.dest) {
                if ( enable_fastdwarf && unit->counters.job_counter > 0 )
                    unit->counters.job_counter = 0;
                break;
            }

            // skip dwarves that are dragging creatures or being dragged
            if ((unit->relations.draggee_id != -1) || (unit->relations.dragger_id != -1))
                break;

            // skip dwarves that are following other units
            if (unit->relations.following != 0)
                break;

            // skip unconscious units
            if (unit->counters.unconscious > 0)
                break;

            // make sure source and dest map blocks are valid
            auto old_occ = Maps::getTileOccupancy(unit->pos);
            auto new_occ = Maps::getTileOccupancy(unit->path.dest);
            if (!old_occ || !new_occ)
                break;

            // clear appropriate occupancy flags at old tile
            if (unit->flags1.bits.on_ground)
                // this is technically wrong, but the game will recompute this as needed
                old_occ->bits.unit_grounded = 0;
            else
                old_occ->bits.unit = 0;

            // if there's already somebody standing at the destination, then force the unit to lay down
            if (new_occ->bits.unit)
                unit->flags1.bits.on_ground = 1;

            // set appropriate occupancy flags at new tile
            if (unit->flags1.bits.on_ground)
                new_occ->bits.unit_grounded = 1;
            else
                new_occ->bits.unit = 1;

            // move unit to destination
            unit->pos = unit->path.dest;
            unit->path.path.clear();
        } while (0);
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
            if (df::global::debug_turbospeed)
                *df::global::debug_turbospeed = false;
        }
        else if (parameters[0] == "1")
        {
            enable_fastdwarf = true;
            if (df::global::debug_turbospeed)
                *df::global::debug_turbospeed = false;
        }
        else if (parameters[0] == "2")
        {
            if (df::global::debug_turbospeed)
            {
                enable_fastdwarf = false;
                *df::global::debug_turbospeed = true;
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
        (df::global::debug_turbospeed && *df::global::debug_turbospeed) ? 2 : (enable_fastdwarf ? 1 : 0),
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
    commands.push_back(PluginCommand("fastdwarf",
        "enable/disable fastdwarf and teledwarf (parameters=0/1)",
        fastdwarf, false,
        "fastdwarf: make dwarves faster.\n"
        "Usage:\n"
        "  fastdwarf <speed> (tele)\n"
        "Valid values for speed:\n"
        " * 0 - Make dwarves move and work at standard speed.\n"
        " * 1 - Make dwarves move and work at maximum speed.\n"
        " * 2 - Make ALL creatures move and work at maximum speed.\n"
        "Valid values for (tele):\n"
        " * 0 - Disable dwarf teleportation (default)\n"
        " * 1 - Make dwarves teleport to their destinations instantly.\n"
        ));
    
    return CR_OK;
}
