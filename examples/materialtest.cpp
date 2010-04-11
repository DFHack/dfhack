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
    
    cout << "----==== Inorganic ====----" << endl;
    vector<DFHack::t_matgloss> matgloss;
    Materials->ReadInorganicMaterials (matgloss);
    for(uint32_t i = 0; i < matgloss.size();i++)
    {
        cout << i << ": " << matgloss[i].id << endl;
    }
    
    cout << endl << "----==== Organic ====----" << endl;
    vector<DFHack::t_matgloss> organic;
    Materials->ReadOrganicMaterials (matgloss);
    for(uint32_t i = 0; i < matgloss.size();i++)
    {
        cout << i << ": " << matgloss[i].id << endl;
    }
    cout << endl << "----==== Creature types ====----" << endl;
    vector<DFHack::t_matgloss> creature;
    Materials->ReadCreatureTypes (matgloss);
    for(uint32_t i = 0; i < matgloss.size();i++)
    {
        cout << i << ": " << matgloss[i].id << endl;
    }
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
