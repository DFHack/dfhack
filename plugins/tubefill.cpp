// Adamantine tube filler. It fills the hollow ones.

#include <stdint.h>
#include <iostream>
#include <map>
#include <vector>
#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <dfhack/modules/Maps.h>
#include <dfhack/modules/World.h>
#include <dfhack/extra/MapExtras.h>
#include <dfhack/modules/Gui.h>
using MapExtras::MapCache;
using namespace DFHack;

DFhackCExport command_result tubefill(DFHack::Core * c, std::vector<std::string> & params);

DFhackCExport const char * plugin_name ( void )
{
    return "tubefill";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("tubefill","Fill in all the adamantine tubes again.",tubefill));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

DFhackCExport command_result tubefill(DFHack::Core * c, std::vector<std::string> & params)
{
    uint32_t x_max,y_max,z_max;
    DFHack::designations40d designations;
    DFHack::tiletypes40d tiles;

    int32_t oldT, newT;
    uint64_t count = 0;

    int dirty=0;

    c->Suspend();
    DFHack::Maps *Mapz = c->getMaps();

    // init the map
    if (!Mapz->Start())
    {
        c->con.printerr("Can't init map.\n");
        c->Resume();
        return CR_FAILURE;
    }

    Mapz->getSize(x_max,y_max,z_max);
    if(!Mapz->StartFeatures())
    {
        c->con.printerr("Can't get map features.\n");
        c->Resume();
        return CR_FAILURE;
    }

    // walk the map
    for (uint32_t x = 0; x< x_max;x++)
    {
        for (uint32_t y = 0; y< y_max;y++)
        {
            for (uint32_t z = 0; z< z_max;z++)
            {
                DFHack::t_feature * locf = 0;
                DFHack::t_feature * glof = 0;
                if (Mapz->ReadFeatures(x,y,z,&locf,&glof))
                {
                    // we're looking for addy tubes
                    if(!locf) continue;
                    if(locf->type != DFHack::feature_Adamantine_Tube) continue;

                    dirty=0;
                    Mapz->ReadDesignations(x,y,z, &designations);
                    Mapz->ReadTileTypes(x,y,z, &tiles);

                    for (uint32_t ty=0;ty<16;++ty)
                    {
                        for (uint32_t tx=0;tx<16;++tx)
                        {
                            if(!designations[tx][ty].bits.feature_local) continue;
                            oldT = tiles[tx][ty];
                            if ( DFHack::tileShape(oldT) != DFHack::WALL )
                            {
                                //Current tile is not a wall.
                                //Set current tile, as accurately as can be expected
                                //newT = DFHack::findSimilarTileType(oldT,DFHack::WALL);
                                newT = DFHack::findTileType( DFHack::WALL, DFHack::FEATSTONE, DFHack::tilevariant_invalid, DFHack::TILE_NORMAL, DFHack::TileDirection() );

                                //If no change, skip it (couldn't find a good tile type)
                                if ( oldT == newT) continue;
                                //Set new tile type, clear designation
                                tiles[tx][ty] = newT;
                                dirty=1;
                                ++count;
                            }
                        }
                    }
                    //If anything was changed, write it all.
                    if (dirty)
                    {
                        Mapz->WriteTileTypes(x,y,z, &tiles);
                    }
                }
            }
        }
    }
    c->Resume();
    c->con.print("Found and changed %d tiles.\n", count);
    return CR_OK;
}
