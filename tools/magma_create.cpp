// This will create 7 deep magama on the square the cursor is on. It does not
// enable magma buildings at this time.

#include <iostream>
#include <integers.h>
#include <vector>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>

int main (void)
{
    int32_t x,y,z;
    DFHack::designations40d designations;

    DFHack::API DF("Memory.xml");

    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
    DF.InitMap();

    if (DF.InitViewAndCursor())
    {
        if(DF.getCursorCoords(x,y,z))
        {
            cout << "cursor coords: " << x << "/" << y << "/" << z << endl;
            if(DF.isValidBlock(x/16,y/16,z))
            {
                // place the magma
                DF.ReadDesignations((x/16),(y/16),z, &designations);
                designations[x%16][y%16].bits.flow_size = 7;
                designations[x%16][y%16].bits.liquid_type = DFHack::liquid_magma;
                DF.WriteDesignations(x/16,y/16,z, &designations);
                
                // make the magma flow :)
                DFHack::t_blockflags bflags;
                DF.ReadBlockFlags((x/16),(y/16),z,bflags);
                // 0x00000001 = job-designated
                // 0x0000000C = run flows? - both bit 3 and 4 required for making magma placed on a glacier flow
                bflags.bits.liquid_1 = true;
                bflags.bits.liquid_2 = true;
                DF.WriteBlockFlags((x/16),(y/16),z,bflags);
                cout << "Success" << endl;
            }
            else
                cout << "Failure 1" << endl;
        }
        else
            cout << "Failure 2" << endl;
    }
    else
        cout << "Process Failed" << endl;
    DF.Detach();
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
