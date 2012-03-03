// Allow changing the material of a mineral inclusion

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "DataDefs.h"
#include "modules/Maps.h"
#include "modules/Materials.h"
#include "TileTypes.h"

using std::vector;
using std::string;
using namespace DFHack;
using namespace df::enums;

using df::global::world;
using df::global::cursor;

command_result df_changevein (Core * c, vector <string> & parameters)
{
    if (parameters.size() != 1)
        return CR_WRONG_USAGE;

    CoreSuspender suspend(c);

    if (!Maps::IsValid())
    {
        c->con.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
    if (!cursor || cursor->x == -30000)
    {
        c->con.printerr("No cursor detected - please place the cursor over a mineral vein.\n");
        return CR_FAILURE;
    }

    MaterialInfo mi;
    if (!mi.findInorganic(parameters[0]))
    {
        c->con.printerr("No such material!\n");
        return CR_FAILURE;
    }
    if (mi.inorganic->material.flags.is_set(material_flags::IS_METAL) ||
        mi.inorganic->material.flags.is_set(material_flags::NO_STONE_STOCKPILE) ||
        mi.inorganic->flags.is_set(inorganic_flags::SOIL_ANY))
    {
        c->con.printerr("Invalid material - you must select a type of stone or gem\n");
        return CR_FAILURE;
    }

    df::map_block *block = Maps::getBlockAbs(cursor->x, cursor->y, cursor->z);
    if (!block)
    {
        c->con.printerr("Invalid tile selected.\n");
        return CR_FAILURE;
    }
    df::block_square_event_mineralst *mineral = NULL;
    int tx = cursor->x % 16, ty = cursor->y % 16;
    for (size_t j = 0; j < block->block_events.size(); j++)
    {
        df::block_square_event *evt = block->block_events[j];
        if (evt->getType() != block_square_event_type::mineral)
            continue;
        mineral = (df::block_square_event_mineralst *)evt;
        if (mineral->getassignment(tx, ty))
            break;
        mineral = NULL;
    }
    if (!mineral)
    {
        c->con.printerr("Selected tile does not contain a mineral vein.\n");
        return CR_FAILURE;
    }
    mineral->inorganic_mat = mi.index;

    return CR_OK;
}

DFHACK_PLUGIN("changevein");

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("changevein",
        "Changes the material of a mineral inclusion.",
        df_changevein, false,
        "Syntax: changevein <inorganic material ID>\n"));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}
