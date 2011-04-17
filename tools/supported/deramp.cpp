// De-ramp.  All ramps marked for removal are replaced with given tile (presently, normal floor).

#include <iostream>
#include <vector>
#include <map>
#include <stddef.h>
#include <assert.h>
#include <string.h>
using namespace std;

#include <DFHack.h>
#include <dfhack/DFTileTypes.h>


int main (void)
{
    uint32_t x_max,y_max,z_max;
    uint32_t num_blocks = 0;
    uint32_t bytes_read = 0;
    DFHack::designations40d designations;
    DFHack::tiletypes40d tiles;
    DFHack::tiletypes40d tilesAbove;

    //DFHack::TileRow *ptile;
    int32_t oldT, newT;

    bool dirty= false;
    int count=0;
    int countbad=0;

    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context *DF = DFMgr.getSingleContext();

    //sanity check
    assert( sizeof(designations) == (16*16*sizeof(DFHack::t_designation)) );

    //Init
    try
    {
        DF->Attach();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    DFHack::Maps *Mapz = DF->getMaps();

    // init the map
    if (!Mapz->Start())
    {
        cerr << "Can't init map." << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }

    Mapz->getSize(x_max,y_max,z_max);

    uint8_t zeroes [16][16] = {0};

    // walk the map
    for (uint32_t x = 0; x< x_max;x++)
    {
        for (uint32_t y = 0; y< y_max;y++)
        {
            for (uint32_t z = 0; z< z_max;z++)
            {
                if (Mapz->isValidBlock(x,y,z))
                {
                    dirty= false;
                    Mapz->ReadDesignations(x,y,z, &designations);
                    Mapz->ReadTileTypes(x,y,z, &tiles);
                    if (Mapz->isValidBlock(x,y,z+1))
                    {
                        Mapz->ReadTileTypes(x,y,z+1, &tilesAbove);
                    }
                    else
                    {
                        memset(&tilesAbove,0,sizeof(tilesAbove));
                    }

                    for (uint32_t ty=0;ty<16;++ty)
                    {
                        for (uint32_t tx=0;tx<16;++tx)
                        {
                            //Only the remove ramp designation (ignore channel designation, etc)
                            oldT = tiles[tx][ty];
                            if ( DFHack::designation_default == designations[tx][ty].bits.dig
                                    && DFHack::RAMP==DFHack::tileShape(oldT))
                            {
                                //Current tile is a ramp.
                                //Set current tile, as accurately as can be expected
                                newT = DFHack::findSimilarTileType(oldT,DFHack::FLOOR);

                                //If no change, skip it (couldn't find a good tile type)
                                if ( oldT == newT) continue;
                                //Set new tile type, clear designation
                                tiles[tx][ty] = newT;
                                designations[tx][ty].bits.dig = DFHack::designation_no;

                                //Check the tile above this one, in case a downward slope needs to be removed.
                                if ( DFHack::RAMP_TOP == DFHack::tileShape(tilesAbove[tx][ty]) )
                                {
                                    tilesAbove[tx][ty] = 32;
                                }
                                dirty= true;
                                ++count;
                            }
                            // ramp fixer
                            else if(DFHack::RAMP!=DFHack::tileShape(oldT) && DFHack::RAMP_TOP == DFHack::tileShape(tilesAbove[tx][ty]))
                            {
                                tilesAbove[tx][ty] = 32;
                                countbad++;
                                dirty = true;
                            }
                        }
                    }
                    //If anything was changed, write it all.
                    if (dirty)
                    {
                        Mapz->WriteDesignations(x,y,z, &designations);
                        Mapz->WriteTileTypes(x,y,z, &tiles);
                        if (Mapz->isValidBlock(x,y,z+1))
                        {
                            Mapz->WriteTileTypes(x,y,z+1, &tilesAbove);
                        }
                        dirty = false;
                    }
                }
            }
        }
    }
    DF->Detach();
    cout << "Found and changed " << count << " tiles." << endl;
    cout << "Fixed " << countbad << " bad down ramps." << endl;
#ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
#endif
    return 0;
}
