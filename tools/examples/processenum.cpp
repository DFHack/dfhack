// Demonstrates the use of ProcessEnumerator
// queries the Enumerator for all DF Processes on user input. Prints them to the terminal.
// also tracks processes that were invalidated

#include <iostream>
#include <climits>
#include <vector>
#include <ctime>
using namespace std;

#include <DFHack.h>
#include <dfhack/DFProcessEnumerator.h>
using namespace DFHack;
#ifndef LINUX_BUILD
#endif
int main (void)
{
    vector<Process*> inval;
    ProcessEnumerator Penum("Memory.xml");
    memory_info * mem;
    for(int cnt = 0; cnt < 100; cnt++)
    {
        // make the ProcessEnumerator update its list of Processes
        // by passing the pointer to 'inval', we make it export expired
        // processes instead of destroying them outright
        // (processes expire when the OS kills them for whatever reason)
        Penum.Refresh(&inval);
        int nProc = Penum.size();
        int nInval = inval.size();

        cout << "Processes:" << endl;
        for(int i = 0; i < nProc; i++)
        {
            mem = Penum[i]->getDescriptor();
            cout << "DF instance: " << Penum[i]->getPID()
                 << ", " << mem->getVersion() << endl;
        }

        cout << "Invalidated:" << endl;
        for(int i = 0; i < nInval; i++)
        {
            mem = inval[i]->getDescriptor();
            cout << "DF instance: " << inval[i]->getPID()
                 << ", " << mem->getVersion() << endl;
            // we own the expired process, we must take care of freeing its resources
            delete inval[i];
        }
        cout << "<-* Press Enter to refresh *->" << endl << endl;
        cin.ignore();
    }
    
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    return 0;
}
