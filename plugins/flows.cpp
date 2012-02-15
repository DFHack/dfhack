// This tool counts static tiles and active flows of water and magma.

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/map_block.h"
#include "df/tile_liquid.h"

using std::string;
using std::vector;
using namespace DFHack;
using namespace df::enums;

using df::global::world;

command_result df_flows (Core * c, vector <string> & parameters)
{
    CoreSuspender suspend(c);

    int flow1 = 0, flow2 = 0, flowboth = 0, water = 0, magma = 0;
    c->con.print("Counting flows and liquids ...\n");

    for (size_t i = 0; i < world->map.map_blocks.size(); i++)
    {
        df::map_block *cur = world->map.map_blocks[i];
        if (cur->flags.bits.update_liquid)
            flow1++;
        if (cur->flags.bits.update_liquid_twice)
            flow2++;
        if (cur->flags.bits.update_liquid && cur->flags.bits.update_liquid_twice)
            flowboth++;
        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                // only count tiles with actual liquid in them
                if (cur->designation[x][y].bits.flow_size == 0)
                    continue;
                if (cur->designation[x][y].bits.liquid_type == tile_liquid::Magma)
                    magma++;
                if (cur->designation[x][y].bits.liquid_type == tile_liquid::Water)
                    water++;
            }
        }
    }

    c->con.print("Blocks with liquid_1=true: %d\n", flow1);
    c->con.print("Blocks with liquid_2=true: %d\n", flow2);
    c->con.print("Blocks with both:          %d\n", flowboth);
    c->con.print("Water tiles:               %d\n", water);
    c->con.print("Magma tiles:               %d\n", magma);
    return CR_OK;
}

DFhackCExport const char * plugin_name ( void )
{
    return "flows";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("flows",
        "Counts map blocks with flowing liquids.",
        df_flows));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}
