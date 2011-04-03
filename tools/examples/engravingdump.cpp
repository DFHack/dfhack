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
    
    DFHack::Engravings *Cons = DF->getEngravings();
    uint32_t numEngr;
    Cons->Start(numEngr);
    
    int32_t cx, cy, cz;
    Gui->getCursorCoords(cx,cy,cz);
    if(cx != -30000)
    {
        dfh_engraving engraved;
        t_engraving &data = engraved.s;
        for(uint32_t i = 0; i < numEngr; i++)
        {
            Cons->Read(i,engraved);
            if(cx == data.x && cy == data.y && cz == data.z)
            {
                printf("Engraving %d/%d/%d @ 0x%x\n", data.x, data.y, data.z, engraved.origin);
                // inorganic stuff - we can recognize that
                printf("type %d, index %d, character %c\n",data.type, data.subtype_idx, data.display_character);
                hexdump(DF,engraved.origin,3);
            }
        }
    }
    else
    {
        dfh_engraving engraved;
        t_engraving &data = engraved.s;
        for(uint32_t i = 0; i < numEngr; i++)
        {
            Cons->Read(i,engraved);
            {
                printf("Engraving %d/%d/%d @ 0x%x\n", data.x, data.y, data.z, engraved.origin);
                // inorganic stuff - we can recognize that
                printf("type %d, index %d, character %c\n",data.type, data.subtype_idx, data.display_character);
                hexdump(DF,engraved.origin,2);
            }
        }
    }
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
