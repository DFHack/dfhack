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

DFHACK_PLUGIN("drybuckets");
REQUIRE_GLOBAL(world);

command_result df_drybuckets (color_ostream &out, vector <string> & parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;

    CoreSuspender suspend;

    int dried_total = 0;
    for (size_t i = 0; i < world->items.all.size(); i++)
    {
        df::item *item = world->items.all[i];
        if ((item->getType() == item_type::LIQUID_MISC) && (item->getMaterial() == builtin_mats::WATER))
        {
            item->flags.bits.garbage_collect = 1;
            dried_total++;
        }
    }
    if (dried_total)
        out.print("Done. %d buckets of water marked for emptying.\n", dried_total);
    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("drybuckets", "Removes water from buckets.", df_drybuckets));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}
