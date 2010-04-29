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


	printf("vtable\ttype\tname\n");
	for(i=0;i<size;i++)
	{
		uint32_t vtable = p->readDWord(p_items[i]);
		uint32_t func0 = p->readDWord(vtable);
		uint64_t func = p->readQuad(func0);
		uint32_t type = (func>>8)&0xffff;
		string desc = p->readClassName(vtable);

		if( (func&0xFFFFFFFFFF000000LL) != 0xCCCCC30000000000LL )
		{
			if(func == 0xCCCCCCCCCCC3C033LL)
				type = 0;
			else
				printf("bad func vtable=%p func=%.16LX\n", (void*)vtable, func);
		}

		printf("%d\t%p\t%s\n", type, (void*)vtable, desc.c_str());
	}

#ifndef LINUX_BUILD
	cout << "Done. Press any key to continue" << endl;
	cin.ignore();
#endif
	return 0;
}
