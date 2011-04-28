#include <iostream>
#include <string.h> // for memset
#include <string>
#include <vector>
#include <stack>
#include <map>
#include <stdio.h>
#include <cstdlib>
using namespace std;

#include <DFHack.h>
#include <dfhack/extra/MapExtras.h>
using namespace MapExtras;
using namespace DFHack;

int main (int argc, char* argv[])
{
    ContextManager DFMgr("Memory.xml");
    Context * DF;
    try
    {
        DF = DFMgr.getSingleContext();
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

    uint32_t x_max,y_max,z_max;
    Maps * Maps = DF->getMaps();
    Gui * Gui = DF->getGui();

    // init the map
    if(!Maps->Start())
    {
        cerr << "Can't init map. Make sure you have a map loaded in DF." << endl;
        DF->Detach();
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }

    int32_t cx, cy, cz;
    Maps->getSize(x_max,y_max,z_max);
    uint32_t tx_max = x_max * 16;
    uint32_t ty_max = y_max * 16;

    Gui->getCursorCoords(cx,cy,cz);
    if(cx == -30000)
    {
        cerr << "Cursor is not active. Point the cursor at some empty space you want to be unhidden." << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    DFCoord xy ((uint32_t)cx,(uint32_t)cy,cz);
    MapCache * MCache = new MapCache(Maps);
    int16_t tt = MCache->tiletypeAt(xy);
    if(isWallTerrain(tt))
    {
        cerr << "Point the cursor at some empty space you want to be unhidden." << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    // hide all tiles, flush cache
    Maps->getSize(x_max,y_max,z_max);

    for(uint32_t x = 0; x< x_max;x++)
    {
        for(uint32_t y = 0; y< y_max;y++)
        {
            for(uint32_t z = 0; z< z_max;z++)
            {
                if(Maps->isValidBlock(x,y,z))
                {
                    designations40d des;
                    // read block designations
                    Maps->ReadDesignations(x,y,z, &des);
                    // change the hidden flag to 0
                    for (uint32_t i = 0; i < 16;i++) for (uint32_t j = 0; j < 16;j++)
                    {
                        des[i][j].bits.hidden = 1;
                    }
                    // write the designations back
                    Maps->WriteDesignations(x,y,z, &des);
                }
            }
        }
    }
    MCache->trash();

    typedef pair <DFCoord, bool> foo;
    stack < foo > flood;
    flood.push( foo(xy,false) );

    while( !flood.empty() )
    {
        foo tile = flood.top();
        DFCoord & current = tile.first;
        bool & from_below = tile.second;
        flood.pop();

        if(!MCache->testCoord(current))
            continue;
        int16_t tt = MCache->tiletypeAt(current);
        t_designation des = MCache->designationAt(current);
        if(!des.bits.hidden)
        {
            continue;
        }
        const TileRow * r = getTileRow(tt);
        if(!r)
        {
            cerr << "unknown tiletype! " << dec << tt << endl;
            continue;
        }
        bool below = 0;
        bool above = 0;
        bool sides = 0;
        bool unhide = 1;
        // by tile shape, determine behavior and action
        switch (r->shape)
        {
            // walls:
            case WALL:
            case PILLAR:
                if(from_below)
                    unhide = 0;
                break;
            // air/free space
            case EMPTY:
            case RAMP_TOP:
            case STAIR_UPDOWN:
            case STAIR_DOWN:
            case STREAM:
            case STREAM_TOP:
                above = below = sides = true;
                break;
            // has floor
            case FORTIFICATION:
            case STAIR_UP:
            case RAMP:
            case FLOOR:
            case TREE_DEAD:
            case TREE_OK:
            case SAPLING_DEAD:
            case SAPLING_OK:
            case SHRUB_DEAD:
            case SHRUB_OK:
            case BOULDER:
            case PEBBLES:
                if(from_below)
                    unhide = 0;
                above = sides = true;
                break;
        }
        if(unhide)
        {
            des.bits.hidden = false;
            MCache->setDesignationAt(current,des);
        }
        if(sides)
        {
            flood.push(foo(DFCoord(current.x + 1, current.y ,current.z),0));
            flood.push(foo(DFCoord(current.x + 1, current.y + 1 ,current.z),0));
            flood.push(foo(DFCoord(current.x, current.y + 1 ,current.z),0));
            flood.push(foo(DFCoord(current.x - 1, current.y + 1 ,current.z),0));
            flood.push(foo(DFCoord(current.x - 1, current.y ,current.z),0));
            flood.push(foo(DFCoord(current.x - 1, current.y - 1 ,current.z),0));
            flood.push(foo(DFCoord(current.x, current.y - 1 ,current.z),0));
            flood.push(foo(DFCoord(current.x + 1, current.y - 1 ,current.z),0));
        }
        if(above)
        {
            flood.push(foo(DFCoord(current.x, current.y ,current.z + 1),1));
        }
        if(below)
        {
            flood.push(foo(DFCoord(current.x, current.y ,current.z - 1),0));
        }
    }
    MCache->WriteAll();
    delete MCache;
    DF->Detach();
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    return 0;
}

