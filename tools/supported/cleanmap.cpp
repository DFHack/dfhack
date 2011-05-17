// Map cleaner. Removes all the snow, mud spills, blood and vomit from map tiles.

#include <iostream>
#include <vector>
#include <map>
#include <stddef.h>
using namespace std;

#include <DFHack.h>
#include <dfhack/extra/termutil.h>

int main (int argc, char** argv)
{
    bool temporary_terminal = TemporaryTerminal();
    bool quiet = false;
    for(int i = 1; i < argc; i++)
    {
        string test = argv[i];
        if(test == "-q")
        {
            quiet = true;
        }
    }

    uint32_t x_max,y_max,z_max;
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
        if(temporary_terminal)
            cin.ignore();
        return 1;
    }
    DFHack::Maps *Mapz = DF->getMaps();

    // init the map
    if(!Mapz->Start())
    {
        cerr << "Can't init map." << endl;
        if(temporary_terminal)
            cin.ignore();
        return 1;
    }

    Mapz->getSize(x_max,y_max,z_max);

    uint8_t zeroes [16][16] = {{0}};
    DFHack::occupancies40d occ;
    DFHack::t_temperatures tt1;
    DFHack::t_temperatures tt2;

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
                    Mapz->ReadOccupancy(x,y,z,&occ);
                    for(int i = 0; i < 16; i++)
                        for(int j = 0; j < 16; j++)
                        {
                            occ[i][j].bits.arrow_color = 0;
                            occ[i][j].bits.broken_arrows_variant = 0;
                        }
                    Mapz->WriteOccupancy(x,y,z,&occ);
                    for(uint32_t i = 0; i < splatter.size(); i++)
                    {
                        DFHack::t_spattervein & vein = splatter[i];
                        // filter away snow and mud
                        if(vein.mat1 != 0xC && vein.mat1 != 0x6)
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
    if (!quiet && temporary_terminal) 
    {
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    }
    return 0;
}
