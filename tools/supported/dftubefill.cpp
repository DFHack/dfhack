// Adamantine tube filler. It fills the hollow ones.

#include <iostream>
#include <vector>
#include <map>
#include <stddef.h>
#include <assert.h>
#include <string.h>
using namespace std;

#include <DFHack.h>
#include <dfhack/DFTileTypes.h>
#include <dfhack/extra/termutil.h>

int main (void)
{
    bool temporary_terminal = TemporaryTerminal();
    uint32_t x_max,y_max,z_max;
    DFHack::designations40d designations;
    DFHack::tiletypes40d tiles;

    int32_t oldT, newT;
    int16_t t;
    uint64_t count = 0;

    int dirty=0;

    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context *DF = DFMgr.getSingleContext();

    //Init
    try
    {
        DF->Attach();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        if(temporary_terminal)
            cin.ignore();
        return 1;
    }
    DFHack::Maps *Mapz = DF->getMaps();

    // init the map
    if (!Mapz->Start())
    {
        cerr << "Can't init map." << endl;
        if(temporary_terminal)
            cin.ignore();
        return 1;
    }

    Mapz->getSize(x_max,y_max,z_max);
    if(!Mapz->StartFeatures())
    {
        cerr << "Can't get features." << endl;
        if(temporary_terminal)
            cin.ignore();
        return 1; 
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
    DF->Detach();
    cout << "Found and changed " << count << " tiles." << endl;
    if(temporary_terminal)
    {
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    }
    return 0;
}
