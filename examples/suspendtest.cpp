// Test suspend/resume

#include <iostream>
#include <climits>
#include <integers.h>
#include <vector>
#include <ctime>
#include <string>
using namespace std;

#include <DFTypes.h>
#include <DFContextManager.h>
#include <DFContext.h>
int main (void)
{
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
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
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
        return 1;
    }
    cout << "Detached, DF should be running again" << endl;
    getline(cin, blah);
    
    return 0;
}
