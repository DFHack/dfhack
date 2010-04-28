#include <iostream>
#include <climits>
#include <integers.h>
#include <vector>
#include <stdio.h>
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

DFHack::Materials * Materials;
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
    Materials = DF.getMaterials();
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

    if(!Materials->ReadInorganicMaterials())
    {
	    cerr << "Can't get the inorganics types." << endl;
	    return 1;
    }
    
    if(!Materials->ReadCreatureTypesEx())
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
	unsigned int j,k;
	unsigned int matptr;
	unsigned int tmp;

    Creatures->ReadCreature(i,temp);
	if(temp.mood>=0)
	{
		current_job = p->readDWord(temp.origin + 0x390);
		if(current_job == 0)
			continue;
		mat_start = p->readDWord(current_job + 0xa4 + 4*3);
		mat_end = p->readDWord(current_job + 0xa4 + 4*4);
		for(j=mat_start;j<mat_end;j+=4)
		{
			matptr = p->readDWord(j);
			for(k=0;k<4;k++)
				printf("%.4X ", p->readWord(matptr + k*2));
			for(k=0;k<3;k++)
				printf("%.8X ", p->readDWord(matptr + k*4 + 0x8));
			for(k=0;k<2;k++)
				printf("%.4X ", p->readWord(matptr + k*2 + 0x14));
			for(k=0;k<3;k++)
				printf("%.8X ", p->readDWord(matptr + k*4 + 0x18));
			for(k=0;k<4;k++)
				printf("%.2X ", p->readByte(matptr + k + 0x24));
			for(k=0;k<6;k++)
				printf("%.8X ", p->readDWord(matptr + k*4 + 0x28));
			for(k=0;k<4;k++)
				printf("%.2X ", p->readByte(matptr + k + 0x40));
			for(k=0;k<9;k++)
				printf("%.8X ", p->readDWord(matptr + k*4 + 0x44));
			printf(" [%p]\n", matptr);
		}
	}
    }
    Creatures->Finish();
    DF.Detach();
    return 0;
}

