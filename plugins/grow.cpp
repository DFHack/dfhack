#include <iostream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <vector>
#include <string>

#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <dfhack/modules/Vegetation.h>
#include <dfhack/modules/Maps.h>
#include <dfhack/modules/Gui.h>
#include <dfhack/TileTypes.h>
#include <dfhack/extra/MapExtras.h>

using std::vector;
using std::string;
using namespace DFHack;

DFhackCExport command_result df_grow (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "grow";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("grow", "Grows saplings into trees (with active cursor, only the targetted one).", df_grow));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

DFhackCExport command_result df_grow (Core * c, vector <string> & parameters)
{
    //uint32_t x_max = 0, y_max = 0, z_max = 0;
    c->Suspend();
    DFHack::Maps *maps = c->getMaps();
    Console & con = c->con;
    if (!maps->Start())
    {
        con.printerr("Cannot get map info!\n");
        c->Resume();
        return CR_FAILURE;
    }
    //maps->getSize(x_max, y_max, z_max);
    MapExtras::MapCache map(maps);
    DFHack::Vegetation *veg = c->getVegetation();
    if (!veg->all_plants)
    {
        con.printerr("Unable to read vegetation!\n");
        c->Resume();
        return CR_FAILURE;
    }
    DFHack::Gui *Gui = c->getGui();
    int32_t x,y,z;
    if(Gui->getCursorCoords(x,y,z))
    {
        vector<DFHack::df_plant *> * alltrees;
        if(maps->ReadVegetation(x/16,y/16,z,alltrees))
        {
            for(size_t i = 0 ; i < alltrees->size(); i++)
            {
                DFHack::df_plant * tree = alltrees->at(i);
                if(tree->x == x && tree->y == y && tree->z == z)
                {
                    if(DFHack::tileShape(map.tiletypeAt(DFHack::DFCoord(x,y,z))) == DFHack::SAPLING_OK)
                    {
                        tree->grow_counter = DFHack::sapling_to_tree_threshold;
                    }
                    break;
                }
            }
        }
    }
    else
    {
        int grown = 0;
        for(size_t i = 0 ; i < veg->all_plants->size(); i++)
        {
            DFHack::df_plant *p = veg->all_plants->at(i);
            uint16_t ttype = map.tiletypeAt(DFHack::DFCoord(p->x,p->y,p->z));
            if(!p->is_shrub && DFHack::tileShape(ttype) == DFHack::SAPLING_OK)
            {
                p->grow_counter = DFHack::sapling_to_tree_threshold;
            }
        }
    }

    // Cleanup
    veg->Finish();
    maps->Finish();
    c->Resume();
    return CR_OK;
}

