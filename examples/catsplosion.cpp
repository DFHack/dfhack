// Catsplosion
// By Zhentar
// This work of evil makes every cat, male or female, grown or kitten, pregnant and due within 2 in-game hours...

#include <iostream>
#include <climits>
#include <integers.h>
#include <vector>
#include <stdlib.h> // for rand()
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>
#include <DFMemInfo.h>
#include <DFProcess.h>


vector<DFHack::t_matgloss> creaturestypes;
DFHack::memory_info *mem;
DFHack::Process *proc;
uint32_t creature_pregnancy_offset;

int fertilizeCat(DFHack::API & DF, const DFHack::t_creature & creature)
{
	if(string(creaturestypes[creature.type].id) == "CAT")
	{
		proc->writeDWord(creature.origin + creature_pregnancy_offset, rand() % 100 + 1);
		return 1;
    }
	return 0;
}

int main (void)
{
    DFHack::API DF("Memory.xml");
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }    


    proc = DF.getProcess();
    mem = DF.getMemoryInfo();
	creature_pregnancy_offset = mem->getOffset("creature_pregnancy");

    if(!DF.ReadCreatureMatgloss(creaturestypes))
    {
        cerr << "Can't get the creature types." << endl;
        return 1; 
    }
	
    uint32_t numCreatures;
    if(!DF.InitReadCreatures(numCreatures))
    {
        cerr << "Can't get creatures" << endl;
        return 1;
    }

	int cats=0;

    for(uint32_t i = 0; i < numCreatures; i++)
    {
        DFHack::t_creature temp;
        DF.ReadCreature(i,temp);
        cats+=fertilizeCat(DF,temp);
    }

	cout << cats << " cats impregnated." << endl;

    DF.FinishReadCreatures();
    DF.Detach();
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}
