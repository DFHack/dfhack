#include <stdint.h>
#include <iostream>
#include <map>
#include <vector>
#include <dfhack/Core.h>
#include <dfhack/Export.h>
#include <dfhack/modules/Maps.h>
#include <dfhack/modules/World.h>

DFhackCExport const char * plugin_name ( void )
{
    return "reveal";
}

struct hideblock
{
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint8_t hiddens [16][16];
};

DFhackCExport int plugin_run (DFHack::Core * c)
{
    c->Suspend();
    DFHack::Maps *Maps =c->getMaps();
    DFHack::World *World =c->getWorld();

    // init the map
    if(!Maps->Start())
    {
        std::cerr << "Can't init map." << std::endl;
        c->Resume();
        return 1;
    }

    std::cout << "Revealing, please wait..." << std::endl;

    uint32_t x_max, y_max, z_max;
    DFHack::designations40d designations;
    Maps->getSize(x_max,y_max,z_max);
    std::vector <hideblock> hidesaved;

    for(uint32_t x = 0; x< x_max;x++)
    {
        for(uint32_t y = 0; y< y_max;y++)
        {
            for(uint32_t z = 0; z< z_max;z++)
            {
                if(Maps->isValidBlock(x,y,z))
                {
                    hideblock hb;
                    hb.x = x;
                    hb.y = y;
                    hb.z = z;
                    // read block designations
                    Maps->ReadDesignations(x,y,z, &designations);
                    // change the hidden flag to 0
                    for (uint32_t i = 0; i < 16;i++) for (uint32_t j = 0; j < 16;j++)
                    {
                        hb.hiddens[i][j] = designations[i][j].bits.hidden;
                        designations[i][j].bits.hidden = 0;
                    }
                    hidesaved.push_back(hb);
                    // write the designations back
                    Maps->WriteDesignations(x,y,z, &designations);
                }
            }
        }
    }
    World->SetPauseState(true);
    c->Resume();
    std::cout << "Map revealed. The game has been paused for you." << std::endl;
    std::cout << "Unpausing can unleash the forces of hell!" << std::endl;
    std::cout << "Saving will make this state permanent. Don't do it." << std::endl << std::endl;
    std::cout << "Press any key to unreveal." << std::endl;
    std::cin.ignore();
    std::cout << "Unrevealing... please wait." << std::endl;
    // FIXME: do some consistency checks here!
    c->Suspend();
    Maps = c->getMaps();
    Maps->Start();
    for(size_t i = 0; i < hidesaved.size();i++)
    {
        hideblock & hb = hidesaved[i];
        Maps->ReadDesignations(hb.x,hb.y,hb.z, &designations);
        for (uint32_t i = 0; i < 16;i++) for (uint32_t j = 0; j < 16;j++)
        {
            designations[i][j].bits.hidden = hb.hiddens[i][j];
        }
        Maps->WriteDesignations(hb.x,hb.y,hb.z, &designations);
    }
    c->Resume();
    return 0;
}
