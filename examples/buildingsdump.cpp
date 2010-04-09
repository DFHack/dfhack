// Creature dump

#include <iostream>
#include <iomanip>
#include <sstream>
#include <climits>
#include <integers.h>
#include <vector>
using namespace std;

#include <DFError.h>
#include <DFTypes.h>
#include <DFHackAPI.h>
#include <DFMemInfo.h>
#include <DFProcess.h>
#include <DFTypes.h>
#include <modules/Buildings.h>
#include <modules/Materials.h>
#include "miscutils.h"

int main (int argc,const char* argv[])
{
    if (argc < 2 || argc > 3)
    {
        cout << "usage:" << endl;
        cout << argv[0] << " object_name [number of lines]" << endl;
        #ifndef LINUX_BUILD
            cout << "Done. Press any key to continue" << endl;
            cin.ignore();
        #endif
        return 0;
    }
    int lines = 16;
    if(argc == 3)
    {
        string s = argv[2]; //blah. I don't care
        istringstream ins; // Declare an input string stream.
        ins.str(s);        // Specify string to read.
        ins >> lines;     // Reads the integers from the string.
    }
    
    vector<DFHack::t_matgloss> creaturestypes;
    
    DFHack::API DF ("Memory.xml");
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
    DFHack::memory_info * mem = DF.getMemoryInfo();
    DFHack::Buildings * Bld = DF.getBuildings();
    
    uint32_t numBuildings;
    if(Bld->Start(numBuildings))
    {
        cout << numBuildings << endl;
        vector < uint32_t > addresses;
        for(uint32_t i = 0; i < numBuildings; i++)
        {
            DFHack::t_building temp;
            Bld->Read(i, temp);
            if(temp.type != 0xFFFFFFFF) // check if type isn't invalid
            {
                string typestr;
                mem->resolveClassIDToClassname(temp.type, typestr);
                cout << typestr << endl;
                if(typestr == argv[1])
                {
                    //cout << buildingtypes[temp.type] << " 0x" << hex << temp.origin << endl;
                    //hexdump(DF, temp.origin, 16);
                    addresses.push_back(temp.origin);
                }
            }
            else
            {
                // couldn't translate type, print out the vtable
                cout << "unknown vtable: " << temp.vtable << endl;
            }
        }
        interleave_hex(DF,addresses,lines / 4);
        Bld->Finish();
    }
    else
    {
        cerr << "buildings not supported for this DF version" << endl;
    }

    DF.Detach();
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    return 0;
}
