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
    Materials->ReadInorganicMaterials ();
    for(uint32_t i = 0; i < Materials->inorganic.size();i++)
    {
        cout << i << ": " << Materials->inorganic[i].id << endl;
    }
    
    cout << endl << "----==== Organic ====----" << endl;
    Materials->ReadOrganicMaterials ();
    for(uint32_t i = 0; i < Materials->organic.size();i++)
    {
        cout << i << ": " << Materials->organic[i].id << endl;
    }
    cout << endl << "----==== Organic - trees ====----" << endl;
    Materials->ReadWoodMaterials ();
    for(uint32_t i = 0; i < Materials->tree.size();i++)
    {
        cout << i << ": " << Materials->tree[i].id << endl;
    }
    cout << endl << "----==== Organic - plants ====----" << endl;
    Materials->ReadPlantMaterials ();
    for(uint32_t i = 0; i < Materials->plant.size();i++)
    {
        cout << i << ": " << Materials->plant[i].id << endl;
    }
    cout << endl << "----==== Creature types ====----" << endl;
    Materials->ReadCreatureTypesEx ();
    for(uint32_t i = 0; i < Materials->raceEx.size();i++)
    {
        cout << i << ": " << Materials->raceEx[i].rawname << endl;
        vector<DFHack::t_creaturecaste> & castes = Materials->raceEx[i].castes;
        for(uint32_t j = 0; j < castes.size();j++)
        {
            cout << " ["
            << castes[j].rawname << ":"
            << castes[j].singular << ":"
            << castes[j].plural << ":"
            << castes[j].adjective << "] ";
            cout << endl;
            for(uint32_t k = 0; k < castes[j].ColorModifier.size(); k++)
            {
                cout << "    colormod[" << k << "] ";
                for(uint32_t l = 0; l < castes[j].ColorModifier[k].size(); l++)
                    cout << castes[j].ColorModifier[k][l] << " ";
                cout << endl;
            }
        }
        cout << endl;
    }
    cout << endl << "----==== Color descriptors ====----" << endl;
    Materials->ReadDescriptorColors();
    for(uint32_t i = 0; i < Materials->color.size();i++)
    {
	cout << i << ": " << Materials->color[i].id << " - " << Materials->color[i].name << "["
		<< (unsigned int) (Materials->color[i].r*255) << ":"
		<< (unsigned int) (Materials->color[i].v*255) << ":"
		<< (unsigned int) (Materials->color[i].b*255) << ":"
		<< "]" << endl;
    }
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
