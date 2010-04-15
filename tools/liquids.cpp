// This will create 7 deep magama on the square the cursor is on. It does not
// enable magma buildings at this time.

#include <iostream>
#include <integers.h>
#include <vector>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>
#include <modules/Maps.h>
#include <modules/Position.h>

int main (void)
{
    int32_t x,y,z;
    DFHack::designations40d designations;

    DFHack::API DF("Memory.xml");
    DFHack::Maps * Maps;
    DFHack::Position * Position;
    try
    {
        DF.Attach();
        Maps = DF.getMaps();
        Position = DF.getPosition();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    bool end = false;
    cout << "Welcome to the liquid spawner. type 'help' for a list of available commands, 'q' to quit." << endl;
    string mode="magma";
    string flowmode="f+";
    int amount = 7;
    while(!end)
    {
        DF.Resume();
        string command = "";
        cout <<"[" << mode << ":" << amount << ":" << flowmode << "]# ";
        getline(cin, command);
        if(command=="help")
        {
            cout << "Modes:" << endl
                 << "m             - switch to magma" << endl
                 << "w             - switch to water" << endl
                 << "f             - flow bits only" << endl
                 << "Properties:" << endl
                 << "f+            - make the spawned liquid flow" << endl
                 << "f.            - don't change flow state (read state in flow mode)" << endl
                 << "f-            - make the spawned liquid static" << endl
                 << "0-7           - set liquid amount" << endl
                 << "Other:" << endl
                 << "q             - quit" << endl
                 << "help          - print this list of commands" << endl
                 << "empty line    - put liquid" << endl
                 << endl
                 << "Usage: point the DF cursor at a tile you want to modify" << endl
                 << "and use the commands available :)" << endl;
                 
        }
        else if(command == "m")
        {
            mode = "magma";
        }
        else if(command == "w")
        {
            mode = "water";
        }
        else if(command == "f")
        {
            mode = "flowbits";
        }
        else if(command == "q")
        {
            end = true;
        }
        else if(command == "f+")
        {
            flowmode = "f+";
        }
        else if(command == "f-")
        {
            flowmode = "f-";
        }
        else if(command == "f.")
        {
            flowmode = "f.";
        }
        // blah blah, bad code, bite me.
        else if(command == "0")
            amount = 0;
        else if(command == "1")
            amount = 1;
        else if(command == "2")
            amount = 2;
        else if(command == "3")
            amount = 3;
        else if(command == "4")
            amount = 4;
        else if(command == "5")
            amount = 5;
        else if(command == "6")
            amount = 6;
        else if(command == "7")
            amount = 7;
        else if(command.empty())
        {
            DF.Suspend();
            do
            {
                if(!Maps->Start())
                {
                    cout << "Can't see any DF map loaded." << endl;
                    break;
                }
                if(!Position->getCursorCoords(x,y,z))
                {
                    cout << "Can't get cursor coords! Make sure you have a cursor active in DF." << endl;
                    break;
                }
                cout << "cursor coords: " << x << "/" << y << "/" << z << endl;
                if(!Maps->isValidBlock(x/16,y/16,z))
                {
                    cout << "Not a valid block." << endl;
                    break;
                }
                // place the magma
                Maps->ReadDesignations((x/16),(y/16),z, &designations);
                if(mode != "flowbits")
                    designations[x%16][y%16].bits.flow_size = amount;
                if(mode == "magma")
                    designations[x%16][y%16].bits.liquid_type = DFHack::liquid_magma;
                else if(mode == "water")
                    designations[x%16][y%16].bits.liquid_type = DFHack::liquid_water;
                Maps->WriteDesignations(x/16,y/16,z, &designations);
                
                // make the magma flow :)
                DFHack::t_blockflags bflags;
                Maps->ReadBlockFlags((x/16),(y/16),z,bflags);
                // 0x00000001 = job-designated
                // 0x0000000C = run flows? - both bit 3 and 4 required for making magma placed on a glacier flow
                if(flowmode == "f+")
                {
                    bflags.bits.liquid_1 = true;
                    bflags.bits.liquid_2 = true;
                }
                else if(flowmode == "f-")
                {
                    bflags.bits.liquid_1 = false;
                    bflags.bits.liquid_2 = false;
                }
                else
                {
                    cout << "flow bit 1 = " << bflags.bits.liquid_1 << endl; 
                    cout << "flow bit 2 = " << bflags.bits.liquid_2 << endl;
                }
                Maps->WriteBlockFlags((x/16),(y/16),z,bflags);
                cout << "OK" << endl;
                Maps->Finish();
            } while (0);
        }
    }
    DF.Detach();
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
