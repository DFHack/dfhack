// Just show some position data

#include <iostream>
#include <climits>
#include <vector>
#include <sstream>
#include <ctime>
using namespace std;

#include <DFHack.h>

int main (int numargs, const char ** args)
{
    uint32_t addr;
    if (numargs == 2)
    {
        istringstream input (args[1],istringstream::in);
        input >> std::hex >> addr;
    }
    
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context * DF;
    try
    {
        DF = DFMgr.getSingleContext();
        DF->Attach();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    
    DFHack::Process* p = DF->getProcess();
    DFHack::VersionInfo* mem = DF->getMemoryInfo();
    DFHack::Materials *Materials = DF->getMaterials();
    
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
    cout << endl << "----==== Color descriptors ====----" << endl;
    Materials->ReadDescriptorColors();
    for(uint32_t i = 0; i < Materials->color.size();i++)
    {
	cout << i << ": " << Materials->color[i].id << " - " << Materials->color[i].name << "["
		<< (unsigned int) (Materials->color[i].r*255) << ":"
		<< (unsigned int) (Materials->color[i].v*255) << ":"
		<< (unsigned int) (Materials->color[i].b*255)
		<< "]" << endl;
    }
    cout << endl << "----==== All descriptors ====----" << endl;
    Materials->ReadDescriptorColors();
    for(uint32_t i = 0; i < Materials->alldesc.size();i++)
    {
	cout << i << ": " << Materials->alldesc[i].id << endl;
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
            << castes[j].adjective << "] ["
            << "st:" << castes[j].strength.level << "/"
            << "ag:" << castes[j].agility.level << "/"
            << "to:" << castes[j].toughness.level << "/"
            << "en:" << castes[j].endurance.level << "/"
            << "re:" << castes[j].recuperation.level << "/"
            << "di:" << castes[j].disease_resistance.level << "/"
            << "an:" << castes[j].analytical_ability.level << "/"
            << "fo:" << castes[j].focus.level << "/"
            << "wi:" << castes[j].willpower.level << "/"
            << "cr:" << castes[j].creativity.level << "/"
            << "in:" << castes[j].intuition.level << "/"
            << "pa:" << castes[j].patience.level << "/"
            << "me:" << castes[j].memory.level << "/"
            << "li:" << castes[j].linguistic_ability.level << "/"
            << "sp:" << castes[j].spatial_sense.level << "/"
            << "mu:" << castes[j].musicality.level << "/"
            << "ki:" << castes[j].kinesthetic_sense.level << "]";
            cout << endl;
            for(uint32_t k = 0; k < castes[j].ColorModifier.size(); k++)
            {
                cout << "    colormod[" << castes[j].ColorModifier[k].part;
                if(castes[j].ColorModifier[k].startdate>0)
                    cout << " start:" << castes[j].ColorModifier[k].startdate << " days, end:" << castes[j].ColorModifier[k].enddate << " days";
                cout << "] ";
                for(uint32_t l = 0; l < castes[j].ColorModifier[k].colorlist.size(); l++)
                {
                    if( castes[j].ColorModifier[k].colorlist[l] < Materials->color.size() )
                        cout << Materials->color[castes[j].ColorModifier[k].colorlist[l]].name << " ";
                    else
                        cout << Materials->alldesc[castes[j].ColorModifier[k].colorlist[l]].id << " ";
                }
                cout << endl;
            }
            cout << "     body: ";
            for(uint32_t k = 0; k < castes[j].bodypart.size(); k++)
            {
                cout << castes[j].bodypart[k].category << " ";
            }
            cout << endl;
        }
        cout << endl;
    }
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
