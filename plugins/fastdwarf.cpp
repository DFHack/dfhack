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

            //move unit's riders(including babies) to destination
            if (unit->flags1.bits.ridden)
            {
                for (size_t j = 0; j < world->units.other[units_other_id::ANY_RIDER].size(); j++)
                {
                    df::unit *rider = world->units.other[units_other_id::ANY_RIDER][j];
                    if (rider->relationship_ids[df::unit_relationship_type::RiderMount] == unit->id)
                        rider->pos = unit->pos;
                }
            }
        } while (0);

        if (enable_fastdwarf)
        {
            for (size_t i = 0; i < unit->actions.size(); i++)
            {
                df::unit_action *action = unit->actions[i];
                switch (action->type)
                {
                case unit_action_type::Move:
                    action->data.move.timer = 1;
                    break;
                case unit_action_type::Attack:
                    // Attacks are executed when timer1 reaches zero, which will be
                    // on the following tick.
                    if (action->data.attack.timer1 > 1)
                        action->data.attack.timer1 = 1;
                    // Attack actions are completed, and new ones generated, when
                    // timer2 reaches zero.
                    if (action->data.attack.timer2 > 1)
                        action->data.attack.timer2 = 1;
                    break;
                case unit_action_type::HoldTerrain:
                    action->data.holdterrain.timer = 1;
                    break;
                case unit_action_type::Climb:
                    action->data.climb.timer = 1;
                    break;
                case unit_action_type::Job:
                    action->data.job.timer = 1;
                    // could also patch the unit->job.current_job->completion_timer
                    break;
                case unit_action_type::Talk:
                    action->data.talk.timer = 1;
                    break;
                case unit_action_type::Unsteady:
                    action->data.unsteady.timer = 1;
                    break;
                case unit_action_type::Dodge:
                    action->data.dodge.timer = 1;
                    break;
                case unit_action_type::Recover:
                    action->data.recover.timer = 1;
                    break;
                case unit_action_type::StandUp:
                    action->data.standup.timer = 1;
                    break;
                case unit_action_type::LieDown:
                    action->data.liedown.timer = 1;
                    break;
                case unit_action_type::Job2:
                    action->data.job2.timer = 1;
                    // could also patch the unit->job.current_job->completion_timer
                    break;
                case unit_action_type::PushObject:
                    action->data.pushobject.timer = 1;
                    break;
                case unit_action_type::SuckBlood:
                    action->data.suckblood.timer = 1;
                    break;
                }
            }
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
    commands.push_back(PluginCommand("fastdwarf",
        "let dwarves teleport and/or finish jobs instantly",
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
