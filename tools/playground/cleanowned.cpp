/*
 * Confiscates and dumps garbage owned by dwarfs.
 */

#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <climits>
#include <vector>
using namespace std;

#include <DFHack.h>
#include <dfhack/DFVector.h>
#include <dfhack/DFTypes.h>

DFHack::Materials * Materials;
DFHack::Items * Items;

int main (int argc, char *argv[])
{
    bool dump_scattered = false;
    int wear_dump_level = 65536;
    
    for(int i = 1; i < argc; i++)
    {
        char *arg = argv[i];
        if (arg[0] != '-')
            continue;
        
        for (; *arg; arg++) {
            switch (arg[0]) {
            case 'j':
                dump_scattered = true;
                break;
            case 'x':
                wear_dump_level = 1;
                break;
            case 'X':
                wear_dump_level = 2;
                break;
            }
        }
    }
    
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

    DFHack::VersionInfo * mem = DF->getMemoryInfo();
    Materials = DF->getMaterials();
    Items = DF->getItems();
    
    Materials->ReadAllMaterials();
    p = DF->getProcess();
    DFHack::OffsetGroup* itemGroup = mem->getGroup("Items");
    unsigned vector_addr = itemGroup->getAddress("items_vector");
    DFHack::DfVector <uint32_t> p_items (p, vector_addr);
    uint32_t size = p_items.size();
    
    printf("Found total %d items.\n", size);

    for (i=0;i<size;i++)
    {
        uint32_t curItem = p_items[i];
        DFHack::t_item itm;
        Items->getItemData(curItem, itm);
        
        if (!itm.header.flags.bits.owned)
            continue;

        bool confiscate = false;
        bool dump = false;
        
        if (itm.header.flags.bits.rotten)
        {
            printf("Confiscating a rotten item.\n");
            confiscate = true;
        }
        else if (itm.wear_level >= wear_dump_level)
        {
            printf("Confiscating and dumping a worn item.\n");
            confiscate = true;
            dump = true;
        }
        else if (dump_scattered && itm.header.flags.bits.on_ground)
        {
            printf("Confiscating and dumping an untidily placed item.\n");
            confiscate = true;
            dump = true;
        }
        
        if (confiscate)
        {
            itm.header.flags.bits.owned = 0;
            if (dump)
                itm.header.flags.bits.dump = 1;

            Items->setItemFlags(curItem, itm.header.flags);

            printf(
                "%5d: %08x %08x (%d,%d,%d) #%08x [%d] %s - %s\n", 
                i, curItem, itm.header.flags.whole, 
                itm.header.x, itm.header.y, itm.header.z, 
                p->readDWord(curItem),
                itm.wear_level,
                Items->getItemClass(itm.matdesc.itemType).c_str(),
                Items->getItemDescription(curItem, Materials).c_str()
            );
        }
    }    

#ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
#endif
    return 0;
}
