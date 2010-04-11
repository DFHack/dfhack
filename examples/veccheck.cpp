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

#include <DFTypes.h>
#include <DFHackAPI.h>
#include <DFProcess.h>
#include <DFMemInfo.h>
#include <DFVector.h>
#include <DFTypes.h>
#include <modules/Materials.h>
#include <modules/Position.h>
#include <modules/Maps.h>
#include <modules/Constructions.h>
#include "miscutils.h"

using namespace DFHack;

int main (int numargs, const char ** args)
{
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
    
    DFHack::Position *Pos = DF.getPosition();
    
    DFHack::Maps *Maps = DF.getMaps();
    Maps->Start();
    
    int32_t cx, cy, cz;
    Pos->getCursorCoords(cx,cy,cz);
    if(cx != -30000)
    {
        uint32_t bx = cx / 16;
        uint32_t tx = cx % 16;
        uint32_t by = cy / 16;
        uint32_t ty = cy % 16;
        mapblock40d block;
        if(Maps->ReadBlock40d(bx,by,cz,&block))
        {
            int16_t tiletype = block.tiletypes[tx][ty];
            cout << tiletype << endl;
        }
    }
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
