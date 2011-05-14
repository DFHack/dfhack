#include <iostream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <vector>

using namespace std;
#include <DFHack.h>
#include <dfhack/extra/MapExtras.h>
#include <xgetopt.h>
#include <time.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    srand(time(0));

    uint32_t x_max = 0, y_max = 0, z_max = 0;
    DFHack::ContextManager manager("Memory.xml");

    DFHack::Context *context = manager.getSingleContext();
    if (!context->Attach())
    {
        std::cerr << "Unable to attach to DF!" << std::endl;
        #ifndef LINUX_BUILD
        std::cin.ignore();
        #endif
        return 1;
    }

    DFHack::Maps *maps = context->getMaps();
    if (!maps->Start())
    {
        std::cerr << "Cannot get map info!" << std::endl;
        context->Detach();
        #ifndef LINUX_BUILD
        std::cin.ignore();
        #endif
        return 1;
    }
    DFHack::Gui * Gui = context->getGui();
    maps->getSize(x_max, y_max, z_max);
    MapExtras::MapCache map(maps);
    uint32_t vegCount = 0;
    DFHack::Vegetation *veg = context->getVegetation();
    if (!veg->Start(vegCount))
    {
        std::cerr << "Unable to read vegetation!" << std::endl;
        return 1;
    }
    int32_t x,y,z;
    if(Gui->getCursorCoords(x,y,z))
    {
        vector<DFHack::dfh_plant> alltrees;
        if(maps->ReadVegetation(x/16,y/16,z,&alltrees))
        {
            for(int i = 0 ; i < alltrees.size(); i++)
            {
                DFHack::dfh_plant & tree = alltrees[i];
                if(tree.sdata.x == x && tree.sdata.y == y && tree.sdata.z == z)
                {
                    if(DFHack::tileShape(map.tiletypeAt(DFHack::DFCoord(x,y,z))) == DFHack::SAPLING_OK)
                    {
                        tree.sdata.grow_counter = DFHack::sapling_to_tree_threshold;
                        veg->Write(tree);
                    }
                    break;
                }
            }
        }
    }
    else
    {
        int grown = 0;
        for(int i = 0 ; i < vegCount; i++)
        {
            DFHack::dfh_plant p;
            veg->Read(i,p);
            uint16_t ttype = map.tiletypeAt(DFHack::DFCoord(p.sdata.x,p.sdata.y,p.sdata.z));
            if(!p.sdata.is_shrub && DFHack::tileShape(ttype) == DFHack::SAPLING_OK)
            {
                p.sdata.grow_counter = DFHack::sapling_to_tree_threshold;
                veg->Write(p);
            }
        }
    }

    // Cleanup
    veg->Finish();
    maps->Finish();
    context->Detach();
    #ifndef LINUX_BUILD
    std::cout << " Press any key to finish.";
    std::cin.ignore();
    #endif
    std::cout << std::endl;
    return 0;
}
