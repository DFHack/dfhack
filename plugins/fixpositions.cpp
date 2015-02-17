// Fix Entity Positions - make sure Elves have diplomats and Humans have guild representatives

#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

#include <DataDefs.h>
#include "df/world.h"
#include "df/historical_entity.h"
#include "df/entity_raw.h"
#include "df/entity_position.h"
#include "df/entity_position_responsibility.h"
#include "df/entity_position_assignment.h"

using std::string;
using std::vector;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("fixpositions");
REQUIRE_GLOBAL(world);

command_result df_fixdiplomats (color_ostream &out, vector<string> &parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;

    CoreSuspender suspend;

    int checked = 0, fixed = 0;
    for (int i = 0; i < world->entities.all.size(); i++)
    {
        df::historical_entity *ent = world->entities.all[i];
        // only work with civilizations - ignore groups and religions
        if (ent->type != 0)
            continue;
        // only add diplomats for tree cap diplomacy
        if (!ent->entity_raw->flags.is_set(entity_raw_flags::TREE_CAP_DIPLOMACY))
            continue;
        checked++;

        bool update = true;
        df::entity_position *pos = NULL;
        // see if we need to add a new position or modify an existing one
        for (int j = 0; j < ent->positions.own.size(); j++)
        {
            pos = ent->positions.own[j];
            if (pos->responsibilities[entity_position_responsibility::MAKE_INTRODUCTIONS] &&
                pos->responsibilities[entity_position_responsibility::MAKE_PEACE_AGREEMENTS] &&
                pos->responsibilities[entity_position_responsibility::MAKE_TOPIC_AGREEMENTS])
            {
                // a diplomat position exists with the proper responsibilities - skip to the end
                update = false;
                break;
            }
            // Diplomat position already exists, but has the wrong options - modify it instead of creating a new one
            if (pos->code == "DIPLOMAT")
                break;
            pos = NULL;
        }
        if (update)
        {
            // either there's no diplomat, or there is one and it's got the wrong responsibilities
            if (!pos)
            {
                // there was no diplomat - create it
                pos = new df::entity_position;
                ent->positions.own.push_back(pos);

                pos->code = "DIPLOMAT";
                pos->id = ent->positions.next_position_id++;
                pos->flags.set(entity_position_flags::DO_NOT_CULL);
                pos->flags.set(entity_position_flags::MENIAL_WORK_EXEMPTION);
                pos->flags.set(entity_position_flags::SLEEP_PRETENSION);
                pos->flags.set(entity_position_flags::PUNISHMENT_EXEMPTION);
                pos->flags.set(entity_position_flags::ACCOUNT_EXEMPT);
                pos->flags.set(entity_position_flags::DUTY_BOUND);
                pos->flags.set(entity_position_flags::COLOR);
                pos->flags.set(entity_position_flags::HAS_RESPONSIBILITIES);
                pos->flags.set(entity_position_flags::IS_DIPLOMAT);
                pos->flags.set(entity_position_flags::IS_LEADER);
                // not sure what these flags do, but the game sets them for a valid diplomat
                pos->flags.set(entity_position_flags::unk_12);
                pos->flags.set(entity_position_flags::unk_1a);
                pos->flags.set(entity_position_flags::unk_1b);
                pos->name[0] = "Diplomat";
                pos->name[1] = "Diplomats";
                pos->precedence = 70;
                pos->color[0] = 7;
                pos->color[1] = 0;
                pos->color[2] = 1;
            }
            // assign responsibilities
            pos->responsibilities[entity_position_responsibility::MAKE_INTRODUCTIONS] = true;
            pos->responsibilities[entity_position_responsibility::MAKE_PEACE_AGREEMENTS] = true;
            pos->responsibilities[entity_position_responsibility::MAKE_TOPIC_AGREEMENTS] = true;
        }

        // make sure the diplomat position, whether we created it or not, is set up for proper assignment
        bool assign = true;
        for (int j = 0; j < ent->positions.assignments.size(); j++)
        {
            if (ent->positions.assignments[j]->position_id == pos->id)
            {
                // it is - nothing more to do here
                assign = false;
                break;
            }
        }
        if (assign)
        {
            // it isn't - set it up
            df::entity_position_assignment *asn = new df::entity_position_assignment;
            ent->positions.assignments.push_back(asn);

            asn->id = ent->positions.next_assignment_id++;
            asn->position_id = pos->id;
            asn->flags.extend(0x1F); // make room for 32 flags
            asn->flags.set(0);  // and set the first one
        }
        if (update || assign)
            fixed++;
    }
    out.print("Fixed %d of %d civilizations to enable tree cap diplomacy.\n", fixed, checked);
    return CR_OK;
}

command_result df_fixmerchants (color_ostream &out, vector<string> &parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;

    CoreSuspender suspend;

    int checked = 0, fixed = 0;
    for (int i = 0; i < world->entities.all.size(); i++)
    {
        df::historical_entity *ent = world->entities.all[i];
        // only work with civilizations - ignore groups and religions
        if (ent->type != 0)
            continue;
        // only add guild reps for merchant nobility
        if (!ent->entity_raw->flags.is_set(entity_raw_flags::MERCHANT_NOBILITY))
            continue;
        checked++;

        bool update = true;
        df::entity_position *pos = NULL;
        // see if we need to add a new position or modify an existing one
        for (int j = 0; j < ent->positions.own.size(); j++)
        {
            pos = ent->positions.own[j];
            if (pos->responsibilities[entity_position_responsibility::TRADE])
            {
                // a guild rep exists with the proper responsibilities - skip to the end
                update = false;
                break;
            }
            // Guild Representative position already exists, but has the wrong options - modify it instead of creating a new one
            if (pos->code == "GUILD_REPRESENTATIVE")
                break;
            pos = NULL;
        }
        if (update)
        {
            // either there's no guild rep, or there is one and it's got the wrong responsibilities
            if (!pos)
            {
                // there was no guild rep - create it
                pos = new df::entity_position;
                ent->positions.own.push_back(pos);

                pos->code = "GUILD_REPRESENTATIVE";
                pos->id = ent->positions.next_position_id++;
                pos->flags.set(entity_position_flags::DO_NOT_CULL);
                pos->flags.set(entity_position_flags::MENIAL_WORK_EXEMPTION);
                pos->flags.set(entity_position_flags::SLEEP_PRETENSION);
                pos->flags.set(entity_position_flags::PUNISHMENT_EXEMPTION);
                pos->flags.set(entity_position_flags::ACCOUNT_EXEMPT);
                pos->flags.set(entity_position_flags::DUTY_BOUND);
                pos->flags.set(entity_position_flags::COLOR);
                pos->flags.set(entity_position_flags::HAS_RESPONSIBILITIES);
                pos->flags.set(entity_position_flags::IS_DIPLOMAT);
                pos->flags.set(entity_position_flags::IS_LEADER);
                // not sure what these flags do, but the game sets them for a valid guild rep
                pos->flags.set(entity_position_flags::unk_12);
                pos->flags.set(entity_position_flags::unk_1a);
                pos->flags.set(entity_position_flags::unk_1b);
                pos->name[0] = "Guild Representative";
                pos->name[1] = "Guild Representatives";
                pos->precedence = 40;
                pos->color[0] = 7;
                pos->color[1] = 0;
                pos->color[2] = 1;
            }
            // assign responsibilities
            pos->responsibilities[entity_position_responsibility::TRADE] = true;
        }

        // make sure the guild rep position, whether we created it or not, is set up for proper assignment
        bool assign = true;
        for (int j = 0; j < ent->positions.assignments.size(); j++)
        {
            if (ent->positions.assignments[j]->position_id == pos->id)
            {
                // it is - nothing more to do here
                assign = false;
                break;
            }
        }
        if (assign)
        {
            // it isn't - set it up
            df::entity_position_assignment *asn = new df::entity_position_assignment;
            ent->positions.assignments.push_back(asn);

            asn->id = ent->positions.next_assignment_id++;
            asn->position_id = pos->id;
            asn->flags.extend(0x1F); // make room for 32 flags
            asn->flags.set(0);  // and set the first one
        }
        if (update || assign)
            fixed++;
    }
    out.print("Fixed %d of %d civilizations to enable merchant nobility.\n", fixed, checked);
    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "fixdiplomats", "Add Diplomat position to Elven civilizations for tree cap diplomacy.",
        df_fixdiplomats, false));
    commands.push_back(PluginCommand(
        "fixmerchants", "Add Guild Representative position to Human civilizations for merchant nobility.",
        df_fixmerchants, false));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}
