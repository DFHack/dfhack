#include <iostream>
#include <climits>
#include <integers.h>
#include <vector>
using namespace std;

#include <DFGlobal.h>
#include <DFError.h>
#include <DFTypes.h>
#include <DFHackAPI.h>
#include <DFMemInfo.h>
#include <DFProcess.h>
#include <modules/Materials.h>
#include <modules/Creatures.h>
#include <modules/Translation.h>

struct matGlosses 
{
    vector<DFHack::t_matglossPlant> plantMat;
    vector<DFHack::t_matgloss> woodMat;
    vector<DFHack::t_matgloss> stoneMat;
    vector<DFHack::t_matgloss> metalMat;
    vector<DFHack::t_matgloss> creatureMat;
};

vector<DFHack::t_creaturetype> creaturestypes;
matGlosses mat;
vector< vector <DFHack::t_itemType> > itemTypes;
DFHack::memory_info *mem;
vector< vector<string> > englishWords;
vector< vector<string> > foreignWords;

int main (int numargs, char ** args)
{
    DFHack::API DF("Memory.xml");
    DFHack::Process * p;
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
    p = DF.getProcess();
    string check = "";
    if(numargs == 2)
        check = args[1];
    
    DFHack::Creatures * Creatures = DF.getCreatures();
    DFHack::Materials * Materials = DF.getMaterials();
    DFHack::Translation * Tran = DF.getTranslation();
    
    uint32_t numCreatures;
    if(!Creatures->Start(numCreatures))
    {
        cerr << "Can't get creatures" << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    if(!numCreatures)
    {
        cerr << "No creatures to print" << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    mem = DF.getMemoryInfo();
    // get stone matgloss mapping
    if(!Materials->ReadCreatureTypesEx(creaturestypes))
    {
        cerr << "Can't get the creature types." << endl;
        return 1; 
    }
        
    if(!Tran->Start())
    {
        cerr << "Can't get name tables" << endl;
        return 1;
    }
    vector<uint32_t> addrs;
    //DF.InitViewAndCursor();
    for(uint32_t i = 0; i < numCreatures; i++)
    {
        DFHack::t_creature temp;
	unsigned int current_job;
	unsigned int mat_start;
	unsigned int mat_end;
	unsigned int j;

        Creatures->ReadCreature(i,temp);
	if(temp.mood)
	{
		cout << "address: " << hex <<  temp.origin << dec << " creature type: " << creaturestypes[temp.race].rawname << endl;
		current_job = p->readDWord(temp.origin + 0x390);
		mat_start = p->readDWord(current_job + 0xa4 + 4*3);
		mat_end = p->readDWord(current_job + 0xa4 + 4*4);
	}
    }
    Creatures->Finish();
    DF.Detach();
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}

