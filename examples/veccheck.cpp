// Just show some position data

#include <iostream>
#include <climits>
#include <integers.h>
#include <vector>
#include <sstream>
#include <ctime>
#include <cstdio>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>
#include <DFProcess.h>
#include <DFMemInfo.h>
#include <DFVector.h>
#include <DFTypes.h>

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
void DumpObjVtables (const char * name, DFHack::Process *p, uint32_t addr)
{
    cout << "----==== " << name << " ====----" << endl;
    DFHack::DfVector vect(p,addr,4);
    for(int i = 0; i < vect.getSize();i++)
    {
        uint32_t addr = *(uint32_t *) vect[i];
        uint32_t vptr = p->readDWord(addr);
        cout << p->readClassName(vptr) << endl;
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
/*    
    string name="Stuff";
    cout << "----==== " << name << " ====----" << endl;
    DFHack::DfVector vect(p,0x165b290,4);
    for(int i = 0; i < vect.getSize();i++)
    {
        uint32_t addr = *(uint32_t *) vect[i];
        DFHack::t_construction_df40d constr;
        DF.ReadRaw(addr, sizeof(constr), (uint8_t *) &constr);
        printf("0x%x %dX %dY %dZ: %d:%d\n", addr, constr.x, constr.y, constr.z,
               constr.material.type,constr.material.index);
    }
    
    
    cout << endl;
  */  
    /*
    DumpObjVtables("Constructions?",p,0x165b278);
    DumpObjVtables("Constructions?",p,0x166edb8);
    */
    /*
    DumpObjStr0Vector("Material templates",p, mem->getAddress("mat_templates"));
    */
    DumpObjStr0Vector("Inorganics",p, mem->getAddress("mat_inorganics"));
    
    cout << "----==== Inorganics ====----" << endl;
    DFHack::DfVector vect(p,addr,4);
    for(int i = 0; i < vect.getSize();i++)
    {
        uint32_t addr = *(uint32_t *) vect[i];
        cout << p->readSTLString(addr) << endl;
    }
    cout << endl;
    
    /*
    DumpObjStr0Vector("Organics - all",p, mem->getAddress("mat_organics_all"));
    
    DumpObjStr0Vector("Organics - plants",p, mem->getAddress("mat_organics_plants"));
    
    DumpDWordVector("Maybe map between all organics and plants",p, mem->getAddress("mat_unk1_numbers"));
    
    DumpObjStr0Vector("Trees/wood",p,  mem->getAddress("mat_organics_trees"));
    
    DumpDWordVector("Maybe map between all organics and trees",p, mem->getAddress("mat_unk2_numbers"));
    
    DumpObjStr0Vector("Body material templates",p, mem->getAddress("mat_body_material_templates"));
    
    DumpObjStr0Vector("Body detail plans",p, mem->getAddress("mat_body_detail_plans"));
    
    DumpObjStr0Vector("Bodies",p, mem->getAddress("mat_bodies"));
    
    DumpObjStr0Vector("Bodygloss",p, mem->getAddress("mat_bodygloss"));
    
    DumpObjStr0Vector("Creature variations",p, mem->getAddress("mat_creature_variations"));
    
    DumpObjStr0Vector("Creature types",p, mem->getAddress("mat_creature_types"));
    */
    
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
