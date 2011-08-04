// This tool counts static tiles and active flows of water and magma.

#include <iostream>
#include <vector>
using namespace std;

#include <DFHack.h>
#include <dfhack/extra/termutil.h>
int main (void)
{
    bool temporary_terminal = TemporaryTerminal();
    uint32_t x_max,y_max,z_max;
    DFHack::designations40d designations;

    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context * DF;
    DFHack::Maps *Maps;
    try
    {
        DF = DFMgr.getSingleContext();
        DF->Attach();
        Maps = DF->getMaps();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        if(temporary_terminal)
            cin.ignore();
        return 1;
    }
    // init the map
    if(!Maps->Start())
    {
        cerr << "Can't init map." << endl;
        if(temporary_terminal)
            cin.ignore();
        return 1;
    }
    DFHack::t_blockflags bflags;
    Maps->getSize(x_max,y_max,z_max);
    // walk the map, count flowing tiles, magma, water
    uint32_t flow1=0, flow2=0, flowboth=0, water=0, magma=0;
    cout << "Counting flows and liquids ...";
    for(uint32_t x = 0; x< x_max;x++)
    {
        for(uint32_t y = 0; y< y_max;y++)
        {
            for(uint32_t z = 0; z< z_max;z++)
            {
                if(Maps->isValidBlock(x,y,z))
                {
                    Maps->ReadBlockFlags(x, y, z, bflags);
                    Maps->ReadDesignations(x, y, z, &designations);
                    if (bflags.bits.liquid_1)
                        flow1++;
                    if (bflags.bits.liquid_2)
                        flow2++;
                    if (bflags.bits.liquid_1 && bflags.bits.liquid_2)
                        flowboth++;
                    for (uint32_t i = 0; i < 16;i++) for (uint32_t j = 0; j < 16;j++)
                    {
                        if (designations[i][j].bits.liquid_type == DFHack::liquid_magma)
                            magma++;
                        if (designations[i][j].bits.liquid_type == DFHack::liquid_water)
                            water++;
                    }
                }
            }
        }
    }
    cout << "Blocks with liquid_1=true: " << flow1 << endl;
    cout << "Blocks with liquid_2=true: " << flow2 << endl;
    cout << "Blocks with both: " << flowboth << endl;
    cout << "Water tiles: " << water << endl;
    cout << "Magma tiles: " << magma << endl;
    
    cout << endl << "Done." << endl;
    if(temporary_terminal)
        cin.ignore();
    return 0;
}
