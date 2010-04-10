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
    DFHack::Materials *Mats = DF.getMaterials();
    vector<t_matgloss> inorganics;
    Mats->ReadInorganicMaterials(inorganics);
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
                printf("Construction %d/%d/%d @ 0x%x\n", con.x, con.y, con.z,con.origin);
                // inorganic stuff - we can recognize that
                printf("Material: form %d, type %d, index %d\n",con.form, con.mat_type, con.mat_idx);
                string matstr = "unknown";
                if(con.mat_type == 0)
                {
                    if(con.mat_idx != 0xffffffff)
                        matstr = inorganics[con.mat_idx].id;
                    else matstr = "inorganic";
                }
                switch(con.form)
                {
                    case constr_bar:
                        printf("It is made of %s bars!\n",matstr.c_str());
                        break;
                    case constr_block:
                        printf("It is made of %s blocks!\n",matstr.c_str());
                        break;
                    case constr_boulder:
                        printf("It is made of %s stones!\n",matstr.c_str());
                        break;
                    case constr_logs:
                        printf("It is made of %s logs!\n",matstr.c_str());
                        break;
                    default:
                        printf("It is made of something we don't know yet! The material is %s.\n",matstr.c_str());
                }
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
