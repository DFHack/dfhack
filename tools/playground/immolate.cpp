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

bool parseOptions(int argc, char **argv,
                  bool &trees, bool &shrubs, bool &immolate)
{
    char c;
    xgetopt opt(argc, argv, "sti");
    opt.opterr = 0;
    while ((c = opt()) != -1)
    {
        switch (c)
        {
        case 's':
            shrubs = true;
            break;
        case 't':
            trees = true;
            break;
        case 'i':
            immolate = true;
            break;
        case '?':
            switch (opt.optopt)
            {
            // For when we take arguments
            default:
                if (isprint(opt.optopt))
                    std::cerr << "Unknown option -" << opt.optopt << "!"
                            << std::endl;
                else
                    std::cerr << "Unknown option character " << (int) opt.optopt << "!"
                            << std::endl;
            }
        default:
            // Um.....
            return false;
        }
    }
    return true;
}

int main(int argc, char *argv[])
{
    bool all_trees = false;
    bool all_shrubs = false;
    bool immolate = false;
    srand(time(0));
    if (!parseOptions(argc, argv, all_trees, all_shrubs, immolate))
    {
        return -1;
    }

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
    if(all_shrubs || all_trees)
    {
        int destroyed = 0;
        for(int i = 0 ; i < vegCount; i++)
        {
            DFHack::dfh_plant p;
            veg->Read(i,p);
            if(all_shrubs && p.sdata.is_shrub || all_trees && !p.sdata.is_shrub)
            {
                //p.sdata.temperature_1 = 0;
                //p.sdata.temperature_2 = 0;
                if (immolate)
                    p.sdata.is_burning = true;
                p.sdata.hitpoints = 0;
                veg->Write(p);
                destroyed ++;
            }
        }
        cout << "Sacrificed " << destroyed;
        if(all_shrubs)
            cout << " shrubs to Armok." << endl;
        if(all_trees)
            cout << " trees to Armok." << endl;
        cout << "----==== Praise Armok! ====----" << endl;
    }
    else
    {
        int32_t x,y,z;
        if(Gui->getCursorCoords(x,y,z))
        {
            vector<DFHack::dfh_plant> alltrees;
            if(maps->ReadVegetation(x/16,y/16,z,&alltrees))
            {
                bool didit = false;
                for(int i = 0 ; i < alltrees.size(); i++)
                {
                    DFHack::dfh_plant & tree = alltrees[i];
                    if(tree.sdata.x == x && tree.sdata.y == y && tree.sdata.z == z)
                    {
                        cout << "----==== Praise Armok! ====----" << endl;
                        if(immolate)
                            tree.sdata.is_burning = true;
                        tree.sdata.hitpoints = 0;
                        veg->Write(tree);
                        didit = true;
                        break;
                    }
                }
                if(!didit)
                {
                    cout << "----==== There's NOTHING there! ====----" << endl;
                }
            }
        }
        else
        {
            cout << "No mass destruction and no cursor." << endl;
            cout << "----==== Armok is not pleased! ====----" << endl;
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
