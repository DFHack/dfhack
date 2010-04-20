// Just show some position data

#include <iostream>
#include <climits>
#include <integers.h>
#include <vector>
#include <sstream>
#include <ctime>
using namespace std;

#include <DFGlobal.h>
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
    cout << endl << "----==== Organic - trees ====----" << endl;
    Materials->ReadWoodMaterials (matgloss);
    for(uint32_t i = 0; i < matgloss.size();i++)
    {
        cout << i << ": " << matgloss[i].id << endl;
    }
    cout << endl << "----==== Organic - plants ====----" << endl;
    Materials->ReadPlantMaterials (matgloss);
    for(uint32_t i = 0; i < matgloss.size();i++)
    {
        cout << i << ": " << matgloss[i].id << endl;
    }
    cout << endl << "----==== Creature types ====----" << endl;
    vector<DFHack::t_creaturetype> creature;
    Materials->ReadCreatureTypesEx (creature);
    for(uint32_t i = 0; i < creature.size();i++)
    {
        cout << i << ": " << creature[i].rawname << endl;
        vector<DFHack::t_creaturecaste> & castes = creature[i].castes;
        for(uint32_t j = 0; j < castes.size();j++)
        {
            cout << " ["
            << castes[j].rawname << ":"
            << castes[j].singular << ":"
            << castes[j].plural << ":"
            << castes[j].adjective << "] ";
            cout << endl;
        }
        cout << endl;
    }
    cout << endl << "----==== Color descriptors ====----" << endl;
    vector<DFHack::t_descriptor_color> colors;
    Materials->ReadDescriptorColors(colors);
    for(uint32_t i = 0; i < colors.size();i++)
    {
	cout << i << ": " << colors[i].id << " - " << colors[i].name << "["
		<< (unsigned int) (colors[i].r*255) << ":"
		<< (unsigned int) (colors[i].v*255) << ":"
		<< (unsigned int) (colors[i].b*255) << ":"
		<< "]" << endl;
    }
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
