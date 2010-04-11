// Make stuck DF run again.

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
    
    DF.ForceResume();
    cout << "DF should be running again :)" << endl;
    getline(cin, blah);

    if(!DF.Detach())
    {
        cerr << "Can't detach from DF" << endl;
        return 1;
    }
    return 0;
}
