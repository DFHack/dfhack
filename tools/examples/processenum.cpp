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

void printhelp ()
{
    cout << "enter empty line for next try." << endl;
    cout << "enter 'next' or 'n' for next test." << endl;
    cout << "enter 'help' to show this text again." << endl;
}

int inputwait (const char * prompt)
{
inputwait_reset:
    string command = "";
    cout <<"[" << prompt << "]# ";
    getline(cin, command);
    if(command == "help")
    {
        printhelp();
        goto inputwait_reset;
    }
    else if(command == "")
    {
        return 1;
    }
    else if(command == "next")
    {
        return 0;
    }
    else
    {
        cout << "Command not recognized. Try 'help' for a list of valid commands." << endl;
        goto inputwait_reset;
    }
    return 0;
}

int main (void)
{
    printhelp();
    cout << endl;
    // first test ProcessEnumerator and BadProcesses
    {
        cout << "Testing ProcessEnumerator" << endl;
        ProcessEnumerator Penum("Memory.xml");
        memory_info * mem;
        do
        {
            // make the ProcessEnumerator update its list of Processes
            // by passing the pointer to 'inval', we make it export expired
            // processes instead of destroying them outright
            // (processes expire when the OS kills them for whatever reason)
            BadProcesses inval;
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
            }
        }
        while(inputwait("ProcessEnumerator"));
    }
    // next test ContextManager and BadContexts
    {
        cout << "Testing ProcessEnumerator" << endl;
        ContextManager Cman("Memory.xml");
        memory_info * mem;
        do
        {
            // make the ProcessEnumerator update its list of Processes
            // by passing the pointer to 'inval', we make it export expired
            // processes instead of destroying them outright
            // (processes expire when the OS kills them for whatever reason)
            BadContexts inval;
            Cman.Refresh(&inval);
            int nCont = Cman.size();
            int nInval = inval.size();

            cout << "Contexts:" << endl;
            for(int i = 0; i < nCont; i++)
            {
                mem = Cman[i]->getMemoryInfo();
                cout << "DF instance: " << Cman[i]->getProcess()->getPID()
                     << ", " << mem->getVersion() << endl;
            }

            cout << "Invalidated:" << endl;
            for(int i = 0; i < nInval; i++)
            {
                mem = inval[i]->getMemoryInfo();
                cout << "DF instance: " << inval[i]->getProcess()->getPID()
                     << ", " << mem->getVersion() << endl;
            }
        }
        while(inputwait("ContextManager"));
    }
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    return 0;
}
