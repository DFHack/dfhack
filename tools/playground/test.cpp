#include <iostream>
#include <iomanip>
#include <climits>
#include <vector>
#include <sstream>
#include <ctime>
#include <cstdio>
using namespace std;

#include <DFHack.h>
#include <DFHack_C.h>
#include <dfhack-c/DFTypes_C.h>
#include <dfhack-c/DFContext_C.h>
#include <dfhack-c/modules/Maps_C.h>
using namespace DFHack;

int main (int numargs, const char ** args)
{
    printf("From C: ");
    DFHackObject* cman = ContextManager_Alloc("Memory.xml");
    DFHackObject* context = ContextManager_getSingleContext(cman);
    if(context)
    {
        Context_Attach(context);
        DFHackObject * maps = Context_getMaps(context);
        if(maps)
        {
            Maps_Start(maps);
            uint32_t x,y,z;
            Maps_getSize(maps, &x, &y, &z);
            printf("Map size: %d, %d, %d\n", x,y,z);
        }
    }
    ContextManager_Free(cman);

    cout << "From C++:";
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context * DF;
    try
    {
        DF = DFMgr.getSingleContext();
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
    
    // DO STUFF HERE
    Maps * m = DF->getMaps();
    m->Start();
    uint32_t x,y,z;
    m->getSize(x,y,z);
    cout << "Map size " << x << ", "<< y << ", " << z << endl;
    
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    return 0;
}
