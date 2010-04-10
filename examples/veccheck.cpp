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
    
    DFHack::Constructions *Cons = DF.getConstructions();
    uint32_t numConstr;
    Cons->Start(numConstr);
    
    int32_t cx, cy, cz;
    Pos->getCursorCoords(cx,cy,cz);
    if(cx != -30000)
    {
        t_construction con;
        for(uint32_t i = 0; i < numConstr; i++)
        {
            Cons->Read(i,con);
            if(cx == con.x && cy == con.y && cz == con.z)
            {
                printf("Construction %d/%d/%d @ 0x%x - Material %d %d\n", con.x, con.y, con.z,con.origin, con.mat_type, con.mat_idx);
                printf("Material form: %d ", con.form);
                if(con.form == 4)
                {
                    printf("It is rough.");
                }
                printf("\n");
                hexdump(DF,con.origin,2);
            }
        }
    }
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
