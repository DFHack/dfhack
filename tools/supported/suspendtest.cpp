// Test suspend/resume

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
    DFHack::Context * DF;
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
    cout << "Attached, DF should be suspended now" << endl;
    getline(cin, blah);
    
    DF->Resume();
    cout << "Resumed, DF should be running" << endl;
    getline(cin, blah);
    
    DF->Suspend();
    cout << "Suspended, DF should be suspended now" << endl;
    getline(cin, blah);
    
    DF->Resume();
    cout << "Resumed, testing ForceResume. Suspend using SysInternals Process Explorer" << endl;
    getline(cin, blah);

    DF->ForceResume();
    cout << "ForceResumed. DF should be running." << endl;
    getline(cin, blah);

    if(!DF->Detach())
    {
        cerr << "Can't detach from DF" << endl;
        if(temporary_terminal)
            cin.ignore();
        return 1;
    }
    cout << "Detached, DF should be running again" << endl;
    if(temporary_terminal)
        getline(cin, blah);
    return 0;
}
