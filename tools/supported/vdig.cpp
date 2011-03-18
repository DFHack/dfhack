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
//#include <argstream.h>

int main (int argc, char* argv[])
{
    // Command line options
    bool updown = false;
    /*
    argstream as(argc,argv);

    as  >>option('x',"updown",updown,"Dig up and down stairs to reach other z-levels.")
        >>help();
        

    // sane check
    if (!as.isOk())
    {
        cout << as.errorLog();
        return 1;
    }
        */
    if(argc > 1 && strcmp(argv[1],"-x") == 0)
        updown = true;

    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context * DF;
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
    DFHack::Maps * Maps = DF->getMaps();
    DFHack::Gui * Gui = DF->getGui();

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
    while(cx == -30000)
    {
        cerr << "Cursor is not active. Point the cursor at a vein." << endl;
        DF->Resume();
        cin.ignore();
        DF->Suspend();
        Gui->getCursorCoords(cx,cy,cz);
    }
    DFHack::DFCoord xy ((uint32_t)cx,(uint32_t)cy,cz);
    if(xy.x == 0 || xy.x == tx_max - 1 || xy.y == 0 || xy.y == ty_max - 1)
    {
        cerr << "I won't dig the borders. That would be cheating!" << endl;
        DF->Detach();
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    MapCache * MCache = new MapCache(Maps);

    DFHack::t_designation des = MCache->designationAt(xy);
    int16_t tt = MCache->tiletypeAt(xy);
    int16_t veinmat = MCache->materialAt(xy);

    if( veinmat == -1 )
    {
        cerr << "This tile is non-vein. Bye :)" << endl;
        delete MCache;
        DF->Detach();
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    printf("%d/%d/%d tiletype: %d, veinmat: %d, designation: 0x%x ... DIGGING!\n", cx,cy,cz, tt, veinmat, des.whole);
    stack <DFHack::DFCoord> flood;
    flood.push(xy);


    while( !flood.empty() )
    {
        DFHack::DFCoord current = flood.top();
        flood.pop();
        int16_t vmat2 = MCache->materialAt(current);
        tt = MCache->tiletypeAt(current);
        if(!DFHack::isWallTerrain(tt))
            continue;
        if(vmat2!=veinmat)
            continue;

        // found a good tile, dig+unset material
        DFHack::t_designation des = MCache->designationAt(current);
        DFHack::t_designation des_minus;
        DFHack::t_designation des_plus;
        des_plus.whole = des_minus.whole = 0;
        int16_t vmat_minus = -1;
        int16_t vmat_plus = -1;
        bool below = 0;
        bool above = 0;
        if(updown)
        {
            if(MCache->testCoord(current-1))
            {
                below = 1;
                des_minus = MCache->designationAt(current-1);
                vmat_minus = MCache->materialAt(current-1);
            }

            if(MCache->testCoord(current+1))
            {
                above = 1;
                des_plus = MCache->designationAt(current+1);
                vmat_plus = MCache->materialAt(current+1);
            }
        }
        if(MCache->testCoord(current))
        {
            MCache->clearMaterialAt(current);
            if(current.x < tx_max - 2)
            {
                flood.push(DFHack::DFCoord(current.x + 1, current.y, current.z));
                if(current.y < ty_max - 2)
                {
                    flood.push(DFHack::DFCoord(current.x + 1, current.y + 1,current.z));
                    flood.push(DFHack::DFCoord(current.x, current.y + 1,current.z));
                }
                if(current.y > 1)
                {
                    flood.push(DFHack::DFCoord(current.x + 1, current.y - 1,current.z));
                    flood.push(DFHack::DFCoord(current.x, current.y - 1,current.z));
                }
            }
            if(current.x > 1)
            {
                flood.push(DFHack::DFCoord(current.x - 1, current.y,current.z));
                if(current.y < ty_max - 2)
                {
                    flood.push(DFHack::DFCoord(current.x - 1, current.y + 1,current.z));
                    flood.push(DFHack::DFCoord(current.x, current.y + 1,current.z));
                }
                if(current.y > 1)
                {
                    flood.push(DFHack::DFCoord(current.x - 1, current.y - 1,current.z));
                    flood.push(DFHack::DFCoord(current.x, current.y - 1,current.z));
                }
            }
            if(updown)
            {
                if(current.z > 0 && below && vmat_minus == vmat2)
                {
                    flood.push(current-1);

                    if(des_minus.bits.dig == DFHack::designation_d_stair)
                        des_minus.bits.dig = DFHack::designation_ud_stair;
                    else
                        des_minus.bits.dig = DFHack::designation_u_stair;
                    MCache->setDesignationAt(current-1,des_minus);

                    des.bits.dig = DFHack::designation_d_stair;
                }
                if(current.z < z_max - 1 && above && vmat_plus == vmat2)
                {
                    flood.push(current+ 1);

                    if(des_plus.bits.dig == DFHack::designation_u_stair)
                        des_plus.bits.dig = DFHack::designation_ud_stair;
                    else
                        des_plus.bits.dig = DFHack::designation_d_stair;
                    MCache->setDesignationAt(current+1,des_plus);

                    if(des.bits.dig == DFHack::designation_d_stair)
                        des.bits.dig = DFHack::designation_ud_stair;
                    else
                        des.bits.dig = DFHack::designation_u_stair;
                }
            }
            if(des.bits.dig == DFHack::designation_no)
                des.bits.dig = DFHack::designation_default;
            MCache->setDesignationAt(current,des);
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

