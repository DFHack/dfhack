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
#include <modules/Materials.h>

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
    DFHack::Materials *Materials = DF.getMaterials();
    
    //const vector<string> * names = mem->getClassIDMapping();
    /*
    DumpObjStr0Vector("Material templates",p, mem->getAddress("mat_templates"));
    
    DumpObjStr0Vector("Inorganics",p, mem->getAddress("mat_inorganics"));
    
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
    */
    
    cout << "----==== Inorganic ====----" << endl;
    vector<DFHack::t_matgloss> matgloss;
    Materials->ReadInorganicMaterials (matgloss);
    for(int i = 0; i < matgloss.size();i++)
    {
        cout << matgloss[i].id << endl;
    }
    
    cout << endl << "----==== Organic ====----" << endl;
    vector<DFHack::t_matgloss> organic;
    Materials->ReadOrganicMaterials (matgloss);
    for(int i = 0; i < matgloss.size();i++)
    {
        cout << matgloss[i].id << endl;
    }
    cout << endl << "----==== Creature types ====----" << endl;
    vector<DFHack::t_matgloss> creature;
    Materials->ReadCreatureTypes (matgloss);
    for(int i = 0; i < matgloss.size();i++)
    {
        cout << matgloss[i].id << endl;
    }
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
