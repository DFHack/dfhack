// Just show some position data

#include <iostream>
#include <iomanip>
#include <climits>
#include <vector>
#include <sstream>
#include <ctime>
#include <cstdio>
#include <stdio.h>

#define DFHACK_WANT_MISCUTILS
#include <DFHack.h>
using namespace DFHack;

int main (int numargs, const char ** args)
{
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context* DF;
    try
    {
        DF = DFMgr.getSingleContext();
        DF->Attach();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    
    DFHack::Gui *Gui = DF->getGui();
    
    DFHack::Constructions *Cons = DF->getConstructions();
    DFHack::Materials *Mats = DF->getMaterials();
    Mats->ReadInorganicMaterials();
    Mats->ReadOrganicMaterials();
    uint32_t numConstr;
    Cons->Start(numConstr);
    
    int32_t cx, cy, cz;
    Gui->getCursorCoords(cx,cy,cz);
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
                std::string matstr = "unknown";
                if(con.mat_type == 0)
                {
                    if(con.mat_idx != 0xffffffff)
                        matstr = Mats->inorganic[con.mat_idx].id;
                    else matstr = "inorganic";
                }
                if(con.mat_type == 420)
                {
                    if(con.mat_idx != 0xffffffff)
                        matstr = Mats->organic[con.mat_idx].id;
                    else matstr = "organic";
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
