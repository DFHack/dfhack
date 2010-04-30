/*
 * dumps vtables, items types and class name for all items in game
 * best used this way : ./dfitemdump | sort -ug
 */
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <climits>
#include <integers.h>
#include <vector>
using namespace std;

#include <DFError.h>
#include <DFTypes.h>
#include <DFHackAPI.h>
#include <DFMemInfo.h>
#include <DFProcess.h>
#include <DFVector.h>
#include <modules/Materials.h>


DFHack::Materials * Materials;

int main ()
{
	DFHack::API DF("Memory.xml");
	DFHack::Process * p;
	unsigned int i;

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

	DFHack::memory_info * mem = DF.getMemoryInfo();
	Materials = DF.getMaterials();
	Materials->ReadAllMaterials();
	p = DF.getProcess();
	DFHack::DfVector <uint32_t> p_items (p, p->getDescriptor()->getAddress ("items_vector"));
	uint32_t size = p_items.size();


	printf("type\tvtable\tname\tquality\n");
	for(i=0;i<size;i++)
	{
		uint32_t vtable = p->readDWord(p_items[i]);
		uint32_t func0 = p->readDWord(vtable);
		uint64_t funct0 = p->readQuad(func0);
		uint32_t func1 = p->readDWord(vtable+0x238);
		uint64_t funct1 = p->readQuad(func1);
		uint32_t type = (funct0>>8)&0xffff;
		uint32_t quality = 0;
		string desc = p->readClassName(vtable);

		if( (funct0&0xFFFFFFFFFF000000LL) != 0xCCCCC30000000000LL )
		{
			if(funct0 == 0xCCCCCCCCCCC3C033LL)
				type = 0;
			else
				printf("bad func vtable=%p func=%.16LX\n", (void*)vtable, funct0);
		}

		if(funct1 == 0xC300000092818B66LL)
			quality = p->readWord(p_items[i]+0x92);
		else if(funct1 == 0xCCCCCCCCCCC3C033)
			quality = 0;
		else
			printf("bad quality function address=%p\n", (void*) func1);

		printf("%d\t%p\t%s\t%d\n", type, (void*)vtable, desc.c_str(), quality);
	}

#ifndef LINUX_BUILD
	cout << "Done. Press any key to continue" << endl;
	cin.ignore();
#endif
	return 0;
}
