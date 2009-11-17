// Test suspend/resume

#include <iostream>
#include <climits>
#include <integers.h>
#include <vector>
#include <ctime>
#include <string>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>

int main (void)
{
    string blah;
    DFHack::API DF("Memory.xml");
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
    cout << "Attached, DF should be suspended now" << endl;
    getline(cin, blah);
    
    DF.Resume();
    
    cout << "Resumed, DF should be running" << endl;
    getline(cin, blah);
    
    DF.Suspend();
    
    cout << "Suspended, DF should be suspended now" << endl;
    getline(cin, blah);
    
    if(!DF.Detach())
    {
        cerr << "Can't detach from DF" << endl;
        return 1;
    }
    cout << "Detached, DF should be running again" << endl;
    getline(cin, blah);
    
    return 0;
}