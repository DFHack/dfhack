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
	unsigned int i,j;

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


	printf("type\tvtable\tname\tquality\tdecorate\n");
	for(i=0;i<size;i++)
	{
		uint32_t vtable = p->readDWord(p_items[i]);
		uint32_t func0 = p->readDWord(vtable);
		uint64_t funct0 = p->readQuad(func0);
		uint32_t func1 = p->readDWord(vtable+0x238);
		uint64_t funct1 = p->readQuad(func1);
		uint32_t func2 = p->readDWord(vtable+0x288);
		uint64_t funct2 = p->readQuad(func2);
		uint32_t type = (funct0>>8)&0xffff;
		uint32_t quality = 0;
		bool hasDecorations;
		string desc = p->readClassName(vtable);

		if( (funct0&0xFFFFFFFFFF000000LL) != 0xCCCCC30000000000LL )
		{
			if(funct0 == 0xCCCCCCCCCCC3C033LL)
				type = 0;
			else
				printf("bad type function address=%p", (void*)func0);
		}

		if(funct1 == 0xC300000092818B66LL)
			quality = p->readWord(p_items[i]+0x92);
		else if(funct1 == 0xCCCCCCCCCCC3C033)
			quality = 0;
		else
			printf("bad quality function address=%p\n", (void*) func1);

		if(funct2 == 0xCCCCCCCCCCC3C032LL)
			hasDecorations = false;
		else if(funct2 == 0xCCCCCCCCCCC301B0)
			hasDecorations = true;
		else
			printf("bad decoration function address=%p\n", (void*) func2);

//		printf("%p\t%.16LX\t", (void*) func2, funct2);
		printf("%d\t%p\t%s\t%d", type, (void*)vtable, desc.c_str(), quality);
		if(hasDecorations)
		{
			bool sep = false;
			printf("\tdeco=[");
			uint32_t decStart = p->readDWord(p_items[i] + 0xAC);
			uint32_t decEnd = p->readDWord(p_items[i] + 0xB0);
			if(decStart != decEnd)
			{
				for(j=decStart;j<decEnd;j+=4)
				{
					uint32_t decoration = p->readDWord(j);
					uint32_t dvtable = p->readDWord(decoration);
					string ddesc = p->readClassName(dvtable);
					uint32_t dtypefunc = p->readDWord(dvtable + 20);
					uint64_t dtypefunct = p->readQuad(dtypefunc);
					uint32_t dtype = 0;
					if( (dtypefunct&0xFFFFFFFFFFFF00FFLL) == 0xCCCCC300000000B8LL)
						dtype = (dtypefunct>>8)&0xfffffff;
					else
						printf("bad decoration type function, address=%p\n", (void*) dtypefunc);
					if(sep)
						printf(",");
					printf("%s[%d]", ddesc.c_str(), dtype);
					sep = true;
				}
			}
			printf("]");
		}
		printf("\n");
	}

#ifndef LINUX_BUILD
	cout << "Done. Press any key to continue" << endl;
	cin.ignore();
#endif
	return 0;
}
