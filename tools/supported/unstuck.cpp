// Make stuck DF run again.

#include <iostream>
#include <climits>
#include <vector>
#include <ctime>
#include <string>
using namespace std;

#include <DFHack.h>
int main (void)
{
    string blah;
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context *DF;
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

    DF->ForceResume();
    cout << "DF should be running again :)" << endl;
    getline(cin, blah);

    if(!DF->Detach())
    {
        cerr << "Can't detach from DF" << endl;
        return 1;
    }
    return 0;
}
