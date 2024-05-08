// Destroys items being used as part of constructions
// and flags the constructions to recreate their components upon disassembly

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Maps.h"

#include "DataDefs.h"
#include "df/item.h"
#include "df/world.h"
#include "df/construction.h"
#include "df/map_block.h"

using namespace std;
using namespace DFHack;

DFHACK_PLUGIN("cleanconst");
REQUIRE_GLOBAL(world);

command_result df_cleanconst(color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    int cleaned_total = 0;

    // proceed with the cleanup operation
    for (auto item : world->items.other.IN_PLAY) {
        // only process items marked as "in construction"
        if (!item->flags.bits.construction)
            continue;
        df::coord pos(item->pos.x, item->pos.y, item->pos.z);

        df::construction *cons = df::construction::find(pos);
        if (!cons)
        {
            out.printerr("Item at %i,%i,%i marked as construction but no construction is present!\n", pos.x, pos.y, pos.z);
            continue;
        }
        // if the construction is already labeled as "no build item", then leave it alone
        if (cons->flags.bits.no_build_item)
            continue;

        // only destroy the item if the construction claims to be made of the exact same thing
        if (item->getType() != cons->item_type ||
            item->getSubtype() != cons->item_subtype ||
            item->getMaterial() != cons->mat_type ||
            item->getMaterialIndex() != cons->mat_index)
            continue;

        item->flags.bits.garbage_collect = 1;
        cons->flags.bits.no_build_item = 1;

        cleaned_total++;
    }

    out.print("Done. %d construction items cleaned up.\n", cleaned_total);
    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "cleanconst",
        "Cleans up construction materials.",
        df_cleanconst));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}
