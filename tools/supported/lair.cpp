// This is a simple bit writer... it marks the whole map as a creature lair, preventing item scatter.

#include <iostream>
#include <vector>
#include <map>
using namespace std;
#include <DFHack.h>
#include <extra/termutil.h>

int main (void)
{
    bool temporary_terminal = TemporaryTerminal();
    uint32_t x_max,y_max,z_max;
    DFHack::occupancies40d occupancies;

    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context *DF;
    try
    {
        DF = DFMgr.getSingleContext();
        DF->Attach();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        if(temporary_terminal)
            cin.ignore();
        return 1;
    }

    DFHack::Maps *Maps =DF->getMaps();

    // init the map
    if(!Maps->Start())
    {
        cerr << "Can't init map." << endl;
        if(temporary_terminal)
            cin.ignore();
        return 1;
    }

    cout << "Designating, please wait..." << endl;

    Maps->getSize(x_max,y_max,z_max);
    for(uint32_t x = 0; x< x_max;x++)
    {
        for(uint32_t y = 0; y< y_max;y++)
        {
            for(uint32_t z = 0; z< z_max;z++)
            {
                if(Maps->isValidBlock(x,y,z))
                {
                    // read block designations
                    Maps->ReadOccupancy(x,y,z, &occupancies);
                    //Maps->ReadTileTypes(x,y,z, &tiles);
                    // change the monster_lair flag to 1
                    for (uint32_t i = 0; i < 16;i++) for (uint32_t j = 0; j < 16;j++)
                    {
                        // add tile type chack here
                        occupancies[i][j].bits.monster_lair = 1;
                    }
                    // write the designations back
                    Maps->WriteOccupancy(x,y,z, &occupancies);
                }
            }
        }
    }
    if(temporary_terminal)
    {
        cout << "The map has been marked as a creature lair. Items shouldn't scatter." << endl;
        cin.ignore();
    }
    return 0;
}
