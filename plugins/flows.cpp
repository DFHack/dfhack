// This tool counts static tiles and active flows of water and magma.

#include <iostream>
#include <vector>
#include <map>
#include <stddef.h>
#include <string.h>
using namespace std;
#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <dfhack/modules/Maps.h>
using namespace DFHack;

DFhackCExport command_result df_flows (Core * c, vector <string> & parameters);

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

DFhackCExport command_result df_flows (Core * c, vector <string> & parameters)
{
    uint32_t x_max,y_max,z_max;
    DFHack::designations40d designations;
    DFHack::Maps *Maps;

    c->Suspend();
    Maps = c->getMaps();
    // init the map
    if(!Maps->Start())
    {
        c->con.printerr("Can't init map.\n");
        c->Resume();
        return CR_FAILURE;
    }
    DFHack::t_blockflags bflags;
    Maps->getSize(x_max,y_max,z_max);
    // walk the map, count flowing tiles, magma, water
    uint32_t flow1=0, flow2=0, flowboth=0, water=0, magma=0;
    c->con.print("Counting flows and liquids ...\n");
    for(uint32_t x = 0; x< x_max;x++)
    {
        for(uint32_t y = 0; y< y_max;y++)
        {
            for(uint32_t z = 0; z< z_max;z++)
            {
                if(Maps->getBlock(x,y,z))
                {
                    Maps->ReadBlockFlags(x, y, z, bflags);
                    Maps->ReadDesignations(x, y, z, &designations);
                    if (bflags.bits.liquid_1)
                        flow1++;
                    if (bflags.bits.liquid_2)
                        flow2++;
                    if (bflags.bits.liquid_1 && bflags.bits.liquid_2)
                        flowboth++;
                    for (uint32_t i = 0; i < 16;i++) for (uint32_t j = 0; j < 16;j++)
                    {
                        if (designations[i][j].bits.liquid_type == DFHack::liquid_magma)
                            magma++;
                        if (designations[i][j].bits.liquid_type == DFHack::liquid_water)
                            water++;
                    }
                }
            }
        }
    }
    c->con.print("Blocks with liquid_1=true: %d\n"
                 "Blocks with liquid_2=true: %d\n"
                 "Blocks with both:          %d\n"
                 "Water tiles:               %d\n"
                 "Magma tiles:               %d\n"
                 ,flow1, flow2, flowboth, water, magma
                );
    c->Resume();
    return CR_OK;
}
