// this will be an incremental search tool in the future. now it is just a memory region dump thing

#include <iostream>
#include <climits>
#include <integers.h>
#include <vector>
#include <map>
#include <ctime>
using namespace std;

#ifndef LINUX_BUILD
    #define WINVER 0x0500
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

#include <DFTypes.h>
#include <DFHackAPI.h>
#include <DFProcessManager.h>

int main (void)
{
    DFHack::API DF("Memory.xml");
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
    DFHack::Process * p = DF.getProcess();
    vector <DFHack::t_memrange> ranges;
    p->getMemRanges(ranges);
    for(int i = 0; i< ranges.size();i++)
    {
        ranges[i].print();
    }
    
    if(!DF.Detach())
    {
        cerr << "Can't detach from DF" << endl;
        return 1;
    }
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}