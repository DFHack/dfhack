/*
 * Simple, pretty item dump example.
 */

#include <cstdio>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <climits>
#include <vector>
#include <cstring>
using namespace std;
#define DFHACK_WANT_MISCUTILS
#include <DFHack.h>
#include <DFVector.h>

int main (int argc, char *argv[])
{
    bool print_refs = false;
    bool print_hex = false;
    bool print_acc = false;

    for(int i = 1; i < argc; i++)
    {
        char *arg = argv[i];
        if (arg[0] != '-')
            continue;

        for (; *arg; arg++) {
            switch (arg[0]) {
            case 'r': print_refs = true; break;
            case 'x': print_hex = true; break;
            case 'a': print_acc = true; break;
            }
        }
    }


    DFHack::Process * p;
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
    DFHack::Materials * Materials = DF->getMaterials();
    Materials->ReadAllMaterials();
    
    DFHack::Gui * Gui = DF->getGui();

    DFHack::Items * Items = DF->getItems();
    Items->Start();

    DFHack::VersionInfo * mem = DF->getMemoryInfo();
    p = DF->getProcess();
    int32_t x,y,z;
    Gui->getCursorCoords(x,y,z);

    std::vector<uint32_t> p_items;
    Items->readItemVector(p_items);
    uint32_t size = p_items.size();

    // FIXME: tools should never be exposed to DFHack internals!
    DFHack::OffsetGroup* itemGroup = mem->getGroup("Items");
    uint32_t ref_vector = itemGroup->getOffset("item_ref_vector");

    for(size_t i = 0; i < size; i++)
    {
        DFHack::dfh_item itm;
        memset(&itm, 0, sizeof(DFHack::dfh_item));
        Items->readItem(p_items[i],itm);

        if (x != -30000
            && !(itm.base.x == x && itm.base.y == y && itm.base.z == z
                 && itm.base.flags.on_ground
                 && !itm.base.flags.in_chest
                 && !itm.base.flags.in_inventory
                 && !itm.base.flags.in_building))
            continue;

        printf(
            "%5d: %08x %6d %08x (%d,%d,%d) #%08x [%d] *%d %s - %s\n",
            i, itm.origin, itm.id, itm.base.flags.whole,
            itm.base.x, itm.base.y, itm.base.z,
            itm.base.vtable,
            itm.wear_level,
            itm.quantity,
            Items->getItemClass(itm).c_str(),
            Items->getItemDescription(itm, Materials).c_str()
        );

        if (print_hex)
            hexdump(DF,p_items[i],0x300);

        if (print_acc)
            cout << Items->dumpAccessors(itm) << endl;

        if (print_refs) {
            DFHack::DfVector<uint32_t> p_refs(p, itm.origin + ref_vector);
            for (size_t j = 0; j < p_refs.size(); j++) {
                uint32_t vptr = p->readDWord(p_refs[j]);
                uint32_t val = p->readDWord(p_refs[j]+4);
                printf("\t-> %d \t%s\n", int(val), p->readClassName(vptr).c_str());
            }
        }
    }
#ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
#endif
    return 0;
}
