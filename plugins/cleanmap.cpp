#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <dfhack/modules/Maps.h>
#include <vector>
#include <string>
#include <string.h>

using std::vector;
using std::string;
using namespace DFHack;

const uint32_t water_idx = 6;
const uint32_t mud_idx = 12;

DFhackCExport command_result cleanmap (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "cleanmap";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("cleanmap","Cleans the map from various substances. Options: 'snow' for removing snow, 'mud' for mud.",cleanmap));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

DFhackCExport command_result cleanmap (Core * c, vector <string> & parameters)
{
    bool snow = false;
    bool mud = false;
    for(int i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "snow")
            snow = true;
        else if(parameters[i] == "mud")
            mud = true;
    }
    vector<DFHack::t_spattervein *> splatter;
    DFHack::Maps *Mapz = c->getMaps();

    // init the map
    if(!Mapz->Start())
    {
        dfout << "Can't init map." << std::endl;
        return CR_FAILURE;
    }

    uint32_t x_max,y_max,z_max;
    Mapz->getSize(x_max,y_max,z_max);

    uint8_t zeroes [16][16] = {{0}};
    DFHack::occupancies40d occ;
    // walk the map
    for(uint32_t x = 0; x< x_max;x++)
    {
        for(uint32_t y = 0; y< y_max;y++)
        {
            for(uint32_t z = 0; z< z_max;z++)
            {
                df_block * block = Mapz->getBlock(x,y,z);
                if(block)
                {
                    Mapz->SortBlockEvents(x,y,z,0,0,&splatter);
                    for(int i = 0; i < 16; i++)
                        for(int j = 0; j < 16; j++)
                        {
                            block->occupancy[i][j].bits.arrow_color = 0;
                            block->occupancy[i][j].bits.broken_arrows_variant = 0;
                        }
                    for(uint32_t i = 0; i < splatter.size(); i++)
                    {
                        DFHack::t_spattervein * vein = splatter[i];
                        // filter snow
                        if(!snow && vein->mat1 == water_idx && vein->matter_state == DFHack::state_powder)
                            continue;
                        // filter mud
                        if(!mud && vein->mat1 == mud_idx && vein->matter_state == DFHack::state_solid)
                            continue;
                        Mapz->RemoveBlockEvent(x,y,z,(t_virtual *) vein);
                    }
                }
            }
        }
    }
    return CR_OK;
}
