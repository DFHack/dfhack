// I'll fix his little red wagon...

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/historical_entity.h"
#include "df/entity_raw.h"
#include "df/creature_raw.h"
#include "df/caste_raw.h"

using std::string;
using std::vector;
using namespace DFHack;
using namespace df::enums;

using df::global::world;

command_result df_fixwagons (Core *c, vector<string> &parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;

    CoreSuspender suspend(c);
    int32_t wagon_creature = -1, wagon_puller_creature = -1;
    df::creature_raw *wagon, *wagon_puller;
    for (size_t i = 0; i < world->raws.creatures.all.size(); i++)
    {
        df::creature_raw *cr = world->raws.creatures.all[i];
        if (cr->flags.is_set(creature_raw_flags::EQUIPMENT_WAGON) && (wagon_creature == -1))
        {
            wagon = cr;
            wagon_creature = i;
        }
        if ((cr->creature_id == "HORSE") && (wagon_puller_creature == -1))
        {
            wagon_puller = cr;
            wagon_puller_creature = i;
        }
    }
    if (wagon_creature == -1)
    {
        c->con.printerr("Couldn't find a valid wagon creature!\n");
        return CR_FAILURE;
    }
    if (wagon_puller_creature == -1)
    {
        c->con.printerr("Couldn't find 'HORSE' creature for default wagon puller!\n");
        return CR_FAILURE;
    }
    int count = 0;
    for (size_t i = 0; i < world->entities.all.size(); i++)
    {
        bool updated = false;
        df::historical_entity *ent = world->entities.all[i];
        if (!ent->entity_raw->flags.is_set(entity_raw_flags::COMMON_DOMESTIC_PULL))
            continue;
        if (ent->resources.animals.wagon_races.size() == 0)
        {
            updated = true;
            for (size_t j = 0; j < wagon->caste.size(); j++)
            {
                ent->resources.animals.wagon_races.push_back(wagon_creature);
                ent->resources.animals.wagon_castes.push_back(j);
            }
        }
        if (ent->resources.animals.wagon_puller_races.size() == 0)
        {
            updated = true;
            for (size_t j = 0; j < wagon_puller->caste.size(); j++)
            {
                ent->resources.animals.wagon_puller_races.push_back(wagon_puller_creature);
                ent->resources.animals.wagon_puller_castes.push_back(j);
            }
        }
        if (updated)
            count++;
    }
    if(count)
        c->con.print("Fixed %d civilizations to bring wagons once again.\n", count);
    return CR_OK;
}

DFhackCExport const char * plugin_name ( void )
{
    return "fixwagons";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand(
        "fixwagons", "Fix all civilizations to be able to bring wagons.",
        df_fixwagons, false,
        "  Since DF v0.31.1 merchants no longer bring wagons due to a bug.\n"
        "  This command re-enables them for all appropriate civilizations.\n"
        ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}
