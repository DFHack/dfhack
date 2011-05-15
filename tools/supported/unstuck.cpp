// Make stuck DF run again.

#include <iostream>
#include <climits>
#include <vector>
#include <ctime>
#include <string>
using namespace std;

#include <DFHack.h>
#include "termutil.h"
int main (void)
{
    bool temporary_terminal = TemporaryTerminal();
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
        if(temporary_terminal)
            cin.ignore();
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
