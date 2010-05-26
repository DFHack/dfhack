// Map cleaner. Removes all the snow, mud spills, blood and vomit from map tiles.

#include <iostream>
#include <integers.h>
#include <vector>
#include <map>
using namespace std;

#include <DFTypes.h>
#include <DFContextManager.h>
#include <DFContext.h>
#include <DFMemInfo.h>
#include <DFTypes.h>
#include <stddef.h>
#include <modules/Maps.h>

int main (void)
{
    uint32_t x_max,y_max,z_max;
    uint32_t num_blocks = 0;
    uint32_t bytes_read = 0;
    vector<DFHack::t_spattervein> splatter;
    
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context *DF = DFMgr.getSingleContext();
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
    if(!Mapz->Start())
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
    for(uint32_t x = 0; x< x_max;x++)
    {
        for(uint32_t y = 0; y< y_max;y++)
        {
            for(uint32_t z = 0; z< z_max;z++)
            {
                if(Mapz->isValidBlock(x,y,z))
                {
                    Mapz->ReadVeins(x,y,z,0,0,&splatter);
                    for(uint32_t i = 0; i < splatter.size(); i++)
                    {
                        DFHack::t_spattervein & vein = splatter[i];
                        if(vein.mat1 != 0xC)
                        {
                            uint32_t addr = vein.address_of;
                            uint32_t offset = offsetof(DFHack::t_spattervein, intensity);
                            DF->WriteRaw(addr + offset,sizeof(zeroes),(uint8_t *) zeroes);
                        }
                    }
                }
            }
        }
    }
    DF->Detach();
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
