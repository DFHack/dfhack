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
#include <DFContextManager.h>
#include <DFContext.h>
#include <DFMemInfo.h>
#include <DFProcess.h>
#include <DFVector.h>
#include <modules/Materials.h>
#include <modules/Items.h>


DFHack::Materials * Materials;
DFHack::Items * Items;

int main ()
{
    DFHack::Process * p;
    unsigned int i,j;
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

    DFHack::memory_info * mem = DF->getMemoryInfo();
    Materials = DF->getMaterials();
    Materials->ReadAllMaterials();
    p = DF->getProcess();
    DFHack::DfVector <uint32_t> p_items (p, p->getDescriptor()->getAddress ("items_vector"));
    uint32_t size = p_items.size();
	Items = DF->getItems();


    printf("type\tvtable\tname\tquality\tdecorate\n");
    for (i=0;i<size;i++)
    {
        uint32_t vtable = p->readDWord(p_items[i]);
        uint32_t func0 = p->readDWord(vtable);
        uint64_t funct0 = p->readQuad(func0);
        uint32_t func1 = p->readDWord(vtable+0x238);
        uint64_t funct1 = p->readQuad(func1);
        uint32_t func2 = p->readDWord(vtable+0x288);
        uint64_t funct2 = p->readQuad(func2);
        uint32_t type = (funct0>>8)&0xffff;

        uint32_t funcB = p->readDWord(vtable + 4);
        uint32_t funcC = p->readDWord(vtable + 8);
        uint32_t funcD = p->readDWord(vtable + 12);
        uint64_t funcBt = p->readQuad(funcB);
        uint64_t funcCt = p->readQuad(funcC);
        uint64_t funcDt = p->readQuad(funcD);
        int16_t typeB = -1;
        int16_t typeC = -1;
        int32_t typeD = -1;
        uint32_t quality = 0;
        bool hasDecorations;
        string desc = p->readClassName(vtable);
		DFHack::t_item itm;

		Items->getItemData(p_items[i], itm);

        if ( (funct0&0xFFFFFFFFFF000000LL) != 0xCCCCC30000000000LL )
        {
            if (funct0 == 0xCCCCCCCCCCC3C033LL)
                type = 0;
            else
                printf("bad type function address=%p", (void*)func0);
        }

        if (funct1 == 0xC300000092818B66LL)
            quality = p->readWord(p_items[i]+0x92);
        else if (funct1 == 0xCCCCCCCCCCC3C033LL)
            quality = 0;
        else
            printf("bad quality function address=%p\n", (void*) func1);

        if (funct2 == 0xCCCCCCCCCCC3C032LL)
            hasDecorations = false;
        else if (funct2 == 0xCCCCCCCCCCC301B0LL)
            hasDecorations = true;
        else
            printf("bad decoration function address=%p\n", (void*) func2);

        if (funcBt == 0xCCCCCCCCC3FFC883LL)
            typeB = -1;
        else
        {
            uint64_t funcBtEnd = p->readQuad(funcB+8);
            if (
                ((funcBt&0xFFFFFFFF0000FFFFLL) == 0x8B6600000000818BLL) &&
                ((funcBtEnd&0xFFFFFFFFFFFF00FFLL) == 0xCCCCCCCCCCC30040LL) )
            {
                uint32_t off1 = (funcBt>>16) & 0xffff;
                uint32_t off2 = (funcBtEnd>>8) & 0xff;
                uint32_t pt1 = p->readDWord(p_items[i] + off1);
                typeB = p->readWord(pt1 + off2);
            }
            else if ((funcBt&0xFFFFFF0000FFFFFFLL)==0xC300000000818B66LL)
            {
                uint32_t off1 = (funcBt>>24) & 0xffff;
                typeB = p->readWord(p_items[i] + off1);
            }
            else
                printf("bad typeB func @%p\n", (void*) funcB);
        }

        if (funcCt == 0xCCCCCCCCC3FFC883LL)
            typeC = -1;
        else if ( (funcCt&0xFFFFFF0000FFFFFFLL) == 0xC300000000818B66LL )
        {
            uint32_t off1 = (funcCt>>24)&0xffff;
            typeC = p->readWord(p_items[i] + off1);
        }
        else
            printf("bad typeC func @%p\n", (void*) funcC);

        if (funcDt == 0xCCCCCCCCC3FFC883LL)
            typeD = -1;
        else if ( (funcDt&0xFFFFFFFF0000FFFFLL) == 0xCCC300000000818BLL )
        {
            uint32_t off1 = (funcDt>>16) & 0xffff;
            typeD = p->readDWord(p_items[i] + off1);
        }
        else if ( (funcDt&0xFFFFFF0000FFFFFFLL) == 0xC30000000081BF0FLL )
        {
            uint32_t off1 = (funcDt>>24) & 0xffff;
            typeD = (int16_t) p->readWord(p_items[i] + off1);
        }
        else
            printf("bad typeD func @%p\n", (void*) funcD);

//      printf("%p\t%.16LX\t", (void*) func2, funct2);
        printf("%d\t%p\t%s\t%d\t[%d,%d,%d]", type, (void*)vtable, desc.c_str(), quality,
               typeB, typeC, typeD);
		printf("\t%s", Items->getItemDescription(p_items[i], Materials).c_str());
//      printf("\t%p\t%.16LX", (void *) funcD, funcDt);
		if( (type!=itm.matdesc.itemType) || (typeB!=itm.matdesc.subType) || (typeC!=itm.matdesc.subIndex) || (typeD!=itm.matdesc.index) || (quality!=itm.quality) )
			printf("\tbad[%d,%d,%d,%d]", itm.matdesc.itemType, itm.matdesc.subType, itm.matdesc.subIndex, itm.matdesc.index);
        if (hasDecorations)
        {
            bool sep = false;
            printf("\tdeco=[");
            uint32_t decStart = p->readDWord(p_items[i] + 0xAC);
            uint32_t decEnd = p->readDWord(p_items[i] + 0xB0);
            if (decStart != decEnd)
            {
                for (j=decStart;j<decEnd;j+=4)
                {
                    uint32_t decoration = p->readDWord(j);
                    uint32_t dvtable = p->readDWord(decoration);
                    string ddesc = p->readClassName(dvtable);
                    uint32_t dtypefunc = p->readDWord(dvtable + 20);
                    uint64_t dtypefunct = p->readQuad(dtypefunc);
                    uint32_t dtypeC = p->readWord(decoration + 0x4);
                    uint32_t dtypeD = p->readDWord(decoration + 0x8);
                    uint32_t dtype = 0;
                    uint32_t dqual = p->readWord(decoration + 20);
                    if ( (dtypefunct&0xFFFFFFFFFFFF00FFLL) == 0xCCCCC300000000B8LL)
                        dtype = (dtypefunct>>8)&0xfffffff;
                    else if ( dtypefunct == 0xCCCCCCCCCCC3C033LL)
                        dtype = 0;
		    else
                        printf("bad decoration type function, address=%p\n", (void*) dtypefunc);
                    if (sep)
                        printf(",");
                    //printf("%s[t=%d,q=%d,%s{%d,%d}]", ddesc.c_str(), dtype, dqual, getMatDesc(-1, dtypeC, dtypeD).c_str(), dtypeC, dtypeD);
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
