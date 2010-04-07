// Just show some position data

#include <iostream>
#include <iomanip>
#include <climits>
#include <integers.h>
#include <vector>
#include <sstream>
#include <ctime>
#include <cstdio>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>
#include <DFProcess.h>
#include <DFMemInfo.h>
#include <DFVector.h>
#include <DFTypes.h>
#include <modules/Vegetation.h>
#include <modules/Materials.h>
#include <modules/Position.h>
#include <modules/Maps.h>
#include "miscutils.h"

using namespace DFHack;

char shades[10] = {'#','$','O','=','+','|','-','^','.',' '};
int main (int numargs, const char ** args)
{
    uint32_t addr;
    uint32_t x_max,y_max,z_max;
    vector<t_vein> veinVector;
    vector<t_frozenliquidvein> IceVeinVector;
    vector<t_spattervein> splatter;

    DFHack::API DF("Memory.xml");
    try
    {
        DF.Attach();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    
    DFHack::Maps *Maps =DF.getMaps();
    DFHack::Position *Pos =DF.getPosition();
    // init the map
    if(!Maps->Start())
    {
        cerr << "Can't init map." << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    int32_t cx, cy, cz;
    Maps->getSize(x_max,y_max,z_max);
    Pos->getCursorCoords(cx,cy,cz);
    if(cx == -30000)
    {
        // walk the map
        for(uint32_t x = 0; x< x_max;x++) for(uint32_t y = 0; y< y_max;y++) for(uint32_t z = 0; z< z_max;z++)
        {
            if(Maps->isValidBlock(x,y,z))
            {
                // look for splater veins
                Maps->ReadVeins(x,y,z,veinVector,IceVeinVector,splatter);
                if(splatter.size())
                {
                    printf("Block %d/%d/%d\n",x,y,z);
                    
                    for(int i = 0; i < splatter.size(); i++)
                    {
                        printf("Splatter %d\nmat1: %d\nunknown: %d\nmat2: %d\nmat3: %d\n",i,splatter[i].mat1,splatter[i].unk1,splatter[i].mat2,splatter[i].mat3);
                        for(uint32_t yyy = 0; yyy < 16; yyy++)
                        {
                            cout << "|";
                            for(uint32_t xxx = 0; xxx < 16; xxx++) 
                            {
                                uint8_t intensity = splatter[i].intensity[xxx][yyy];
                                cout << shades[9 - (intensity / 28)];
                            }
                            cout << "|" << endl;
                        }
                            
                        hexdump(DF, splatter[i].address_of,20);
                        cout << endl;
                    }
                }
            }
        }
    }
    else
    {
        uint32_t bx,by,bz;
        bx = cx / 16;
        by = cy / 16;
        bz = cz;
        // look for splater veins
        Maps->ReadVeins(bx,by,bz,veinVector,IceVeinVector,splatter);
        if(splatter.size())
        {
            printf("Block %d/%d/%d\n",bx,by,bz);
            
            for(int i = 0; i < splatter.size(); i++)
            {
                printf("Splatter %d\nmat1: %d\nunknown: %d\nmat2: %d\nmat3: %d\n",i,splatter[i].mat1,splatter[i].unk1,splatter[i].mat2,splatter[i].mat3);
                for(uint32_t y = 0; y < 16; y++)
                {
                    cout << "|";
                    for(uint32_t x = 0; x < 16; x++) 
                    {
                        uint8_t intensity = splatter[i].intensity[x][y];
                        if(intensity)
                        {
                            cout << "#";
                        }
                        else
                        {
                            cout << " ";
                        }
                    }
                    cout << "|" << endl;
                }
                    
                hexdump(DF, splatter[i].address_of,20);
                cout << endl;
            }
        }
    }
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
