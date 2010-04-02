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
#include <DFVector.h>

void DumpObjStr0Vector (const char * name, DFHack::Process *p, uint32_t addr)
{
    cout << "----==== " << name << " ====----" << endl;
    DFHack::DfVector vect(p,addr,4);
    for(int i = 0; i < vect.getSize();i++)
    {
        uint32_t addr = *(uint32_t *) vect[i];
        cout << p->readSTLString(addr) << endl;
    }
    cout << endl;
}

void DumpDWordVector (const char * name, DFHack::Process *p, uint32_t addr)
{
    cout << "----==== " << name << " ====----" << endl;
    DFHack::DfVector vect(p,addr,4);
    for(int i = 0; i < vect.getSize();i++)
    {
        uint32_t number = *(uint32_t *) vect[i];
        cout << number << endl;
    }
    cout << endl;
}

int main (int numargs, const char ** args)
{
    uint32_t addr;
    if (numargs == 2)
    {
        istringstream input (args[1],istringstream::in);
        input >> std::hex >> addr;
    }
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
    
    DFHack::Process* p = DF.getProcess();
    DFHack::memory_info* mem = DF.getMemoryInfo();
    //const vector<string> * names = mem->getClassIDMapping();
    
    DumpObjStr0Vector("Inorganics",p,0x16afd04);
    
    DumpObjStr0Vector("Organics - all",p,0x16afd1C);
    
    DumpObjStr0Vector("Organics - filtered",p,0x16afd34);
    
    DumpDWordVector("Some weird numbers",p,0x16afd4C);
    
    DumpObjStr0Vector("Trees/wood",p,0x16afd64);
    
    DumpDWordVector("More weird numbers",p,0x16afd7C);
    
    DumpObjStr0Vector("WTF",p,0x16afd7C + 0x18 );
    
    DumpObjStr0Vector("WTF2",p,0x16afd7C + 0x18 + 0x18);
    
    DumpObjStr0Vector("WTF3",p,0x16afd7C + 0x18 + 0x18 + 0x18 );
    
    DumpObjStr0Vector("WTF4",p,0x16afd7C + 0x18 + 0x18 + 0x18 + 0x18);
    
    DumpObjStr0Vector("WTF5",p,0x16afd7C + 0x18 + 0x18 + 0x18 + 0x18 + 0x18);
    
    DumpObjStr0Vector("Creature types",p,0x16afd7C + 0x18 + 0x18 + 0x18 + 0x18 + 0x18 + 0x18);
    
    
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
