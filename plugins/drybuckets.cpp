// Dry Buckets : Remove all "water" objects from buckets scattered around the fortress

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/item.h"
#include "df/builtin_mats.h"

using std::string;
using std::vector;
using namespace DFHack;
using namespace df::enums;

using df::global::world;

DFhackCExport command_result df_drybuckets (Core * c, vector <string> & parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;

    CoreSuspender suspend(c);

    int dried_total = 0;
    for (size_t i = 0; i < world->items.all.size(); i++)
    {
        df::item *item = world->items.all[i];
        if ((item->getType() == item_type::LIQUID_MISC) && (item->getMaterial() == builtin_mats::WATER))
        {
            item->flags.bits.garbage_colect = 1;
            dried_total++;
        }
    }
    if (dried_total)
        c->con.print("Done. %d buckets of water marked for emptying.\n", dried_total);
    return CR_OK;
}

DFhackCExport const char * plugin_name ( void )
{
    return "drybuckets";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("drybuckets", "Removes water from buckets.", df_drybuckets));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}
