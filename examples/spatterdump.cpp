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

#include <DFGlobal.h>
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
#include <DFMiscUtils.h>

using namespace DFHack;

typedef uint32_t _DWORD;
int get_material_vector(uint32_t vein_8, uint16_t vein_4, int WORLD_)
{
  int result; // eax@2
  int v4; // ecx@11
  int v5; // eax@12

  if ( (uint16_t)(vein_4 - 0x1A3) > 0xC7u )
  {
    if ( ((int16_t)vein_4 < 19 || (int16_t)vein_4 > 0xDAu)
      && (uint16_t)(vein_4 - 219) > 0xC7u )
    {
      if ( vein_4 )
      {
        if ( vein_4 > 0x292u )
          result = 0;
        else
          result = *(_DWORD *)(WORLD_ + 4 * (int16_t)vein_4 + 0x5DF44);
      }
      else
      {
        if ( (signed int)vein_8 >= 0
          && (v4 = *(_DWORD *)(WORLD_ + 0x54B88), vein_8 < (*(_DWORD *)(WORLD_ + 0x54B8C) - v4) >> 2)
          && (v5 = *(_DWORD *)(v4 + 4 * vein_8)) != 0 )
          result = v5 + 0x178;
        else
          result = *(_DWORD *)(WORLD_ + 0x5DF44);
      }
    }
    else
    {
        /*
      result = sub_4D47A0(vein_8, vein_4, WORLD_ + 0x54C84);
      if ( !result )
        result = *(_DWORD *)(WORLD_ + 0x5DF90);
      */
    }
  }
  else
  {
      /*
    result = sub_41F430(WORLD_ + 0x54B94, vein_4);
    if ( !result )
      result = *(_DWORD *)(WORLD_ + 0x5E5D0);
    */
  }
  return result;
}


char shades[10] = {'#','$','O','=','+','|','-','^','.',' '};
int main (int numargs, const char ** args)
{
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
    DFHack::Materials *Mats =DF.getMaterials();
    
    Mats->ReadCreatureTypes();
    
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
                Maps->ReadVeins(x,y,z,0,0,&splatter);
                if(splatter.size())
                {
                    printf("Block %d/%d/%d\n",x,y,z);
                    
                    for(uint32_t i = 0; i < splatter.size(); i++)
                    {
                        printf("Splatter %d\nmat1: %d\nunknown: %d\nmat2: %d\nmat3: %d\n",i,splatter[i].mat1,splatter[i].unk1,splatter[i].mat2,splatter[i].mat3);
                        cout << PrintSplatterType(splatter[i].mat1,splatter[i].mat2,Mats->race) << endl;
                        printf("Address 0x%08x\n",splatter[i].address_of);
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
        Maps->ReadVeins(bx,by,bz,0,0,&splatter);
        if(splatter.size())
        {
            printf("Block %d/%d/%d\n",bx,by,bz);
            
            for(uint32_t i = 0; i < splatter.size(); i++)
            {
                printf("Splatter %d\nmat1: %d\nunknown: %d\nmat2: %d\nmat3: %d\n",i,splatter[i].mat1,splatter[i].unk1,splatter[i].mat2,splatter[i].mat3);
                PrintSplatterType(splatter[i].mat1,splatter[i].mat2,Mats->race);
                cout << endl;
                printf("Address 0x%08x\n",splatter[i].address_of);
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
