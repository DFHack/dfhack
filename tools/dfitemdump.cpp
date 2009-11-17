// Item dump

#include <iostream>
#include <iomanip>
#include <sstream>
#include <climits>
#include <integers.h>
#include <vector>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>

int main ()
{
   
    DFHack::API DF ("Memory.xml");
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
   
    
    vector <string> buildingtypes;
    uint32_t numBuildings = DF.InitReadBuildings(buildingtypes);
    DF.InitViewAndCursor();
    cout << "q to quit, anything else to look up items at that location\n";
    while(1)
    {
        string input;
        DF.Resume();
        getline (cin, input);
        DF.Suspend();
        uint32_t numItems = DF.InitReadItems();
        if(input == "q")
        {
            break;
        }
        int32_t x,y,z;
        DF.getCursorCoords(x,y,z);
        for(uint32_t i = 0; i < numItems; i++)
        {
            t_item temp;
            DF.ReadItem(i, temp);
            if(temp.x == x && temp.y == y && temp.z == z)
            {
                cout << buildingtypes[temp.type] << " 0x" << hex << temp.origin << endl;
            }
        }
        DF.FinishReadItems();
    }
    DF.FinishReadBuildings();
    DF.Detach();
#ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
#endif
    return 0;
}
