// Just show some position data

#include <iostream>
#include <climits>
#include <integers.h>
#include <vector>
#include <sstream>
#include <ctime>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>
#include <DFProcess.h>
#include <DFMemInfo.h>

int main (int numargs, const char ** args)
{
    uint32_t addr;
    if (numargs == 2)
    {
        istringstream input (args[1],istringstream::in);
        input >> std::hex >> addr;
    }
    DFHack::API DF("Memory.xml");
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
    }
    else
    {
        DFHack::Process* p = DF.getProcess();
        DFHack::memory_info* mem = DF.getMemoryInfo();
        const vector<string> * names = mem->getClassIDMapping();
        for(int i = 0; i < names->size();i++)
        {
            cout << i << " " << names->at(i) << endl;
        }
        /*
        #ifdef LINUX_BUILD
        cout << "start 0x" << hex << p->readDWord(addr+0x0) << endl;
        cout << "end   0x" << hex << p->readDWord(addr+0x4) << endl;
        cout << "cap   0x" << hex << p->readDWord(addr+0x8) << endl;
        #else
        cout << "start 0x" << hex << p->readDWord(addr+0x4) << endl;
        cout << "end   0x" << hex << p->readDWord(addr+0x8) << endl;
        cout << "cap   0x" << hex << p->readDWord(addr+0xC) << endl;
        #endif
        */
    }
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
