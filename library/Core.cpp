#include "Internal.h"
#include "PlatformInternal.h"

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdio>
#include <cstring>
using namespace std;

#include "dfhack/Core.h"
#include "dfhack/VersionInfoFactory.h"
#include "dfhack/Error.h"
#include "dfhack/Process.h"
#include "dfhack/Context.h"
#include "dfhack/modules/Gui.h"
#include "dfhack/modules/Vegetation.h"
#include "dfhack/modules/Maps.h"
using namespace DFHack;

DFHack::Gui * gui = 0;
DFHack::Maps * maps = 0;
DFHack::Vegetation * veg = 0;

Core::Core()
{
    vif = new DFHack::VersionInfoFactory("Memory.xml");
    p = new DFHack::Process(vif);
    if (!p->isIdentified())
    {
        std::cerr << "Couldn't identify this version of DF." << std::endl;
        errorstate = true;
    }
    c = new DFHack::Context(p);
    errorstate = false;
    // bullcrud, push it back to a tool
    gui = c->getGui();
    gui->Start();
    veg = c->getVegetation();
    veg->Start();
    maps = c->getMaps();
    maps->Start();
};

// more bullcrud
int32_t x = 0,y = 0,z = 0;
int32_t xo = 0,yo = 0,zo = 0;
void print_tree( DFHack::df_plant & tree)
{
    //DFHack::Materials * mat = DF->getMaterials();
    printf("%d:%d = ",tree.type,tree.material);
    if(tree.watery)
    {
        std::cout << "near-water ";
    }
    //std::cout << mat->organic[tree.material].id << " ";
    if(!tree.is_shrub)
    {
        std::cout << "tree";
    }
    else
    {
        std::cout << "shrub";
    }
    std::cout << std::endl;
    printf("Grow counter: 0x%08x\n", tree.grow_counter);
    printf("temperature 1: %d\n", tree.temperature_1);
    printf("temperature 2: %d\n", tree.temperature_2);
    printf("On fire: %d\n", tree.is_burning);
    printf("hitpoints: 0x%08x\n", tree.hitpoints);
    printf("update order: %d\n", tree.update_order);
    printf("Address: 0x%x\n", &tree);
    //hexdump(DF,tree.address,13*16);
}

int Core::Update()
{
    if(errorstate)
        return -1;
    // And more bullcrud. Predictable!
    maps->Start();
    gui->getCursorCoords(x,y,z);
    if(x != xo || y!= yo || z != zo)
    {
        xo = x;
        yo = y;
        zo = z;
        std::cout << "Cursor: " << x << "/" << y << "/" << z << std::endl;
        if(x != -30000)
        {
            std::vector <DFHack::df_plant *> * vec;
            if(maps->ReadVegetation(x/16,y/16,z,vec))
            {
                for(size_t i = 0; i < vec->size();i++)
                {
                    DFHack::df_plant * p = vec->at(i);
                    if(p->x == x && p->y == y && p->z == z)
                    {
                        print_tree(*p);
                    }
                }
            }
            else
                std::cout << "No veg vector..." << std::endl;
        }
    }
    return 0;
};

int Core::Shutdown ( void )
{
    if(errorstate)
        return -1;
    return 0;
    // do something here, eventually.
}
