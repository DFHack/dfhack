/*
 * Confiscates and dumps garbage owned by dwarfs.
 */

#include <cstdio>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <climits>
#include <vector>
using namespace std;

#include <DFHack.h>
#include <dfhack/DFVector.h>
#include <dfhack/DFTypes.h>

int main (int argc, char *argv[])
{
    bool dump_scattered = false;
    bool confiscate_all = false;
    bool dry_run = false;
    int wear_dump_level = 65536;

    for(int i = 1; i < argc; i++)
    {
        char *arg = argv[i];
        if (arg[0] != '-')
            continue;

        for (; *arg; arg++) {
            switch (arg[0]) {
            case 'd':
                dry_run = true;
                break;
            case 'l':
                dump_scattered = true;
                break;
            case 'a':
                confiscate_all = true;
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
    DFHack::Materials *Materials = DF->getMaterials();
    DFHack::Items *Items = DF->getItems();
    DFHack::Creatures *Creatures = DF->getCreatures();
    DFHack::Translation *Tran = DF->getTranslation();

    Materials->ReadAllMaterials();
    uint32_t num_creatures;
    Creatures->Start(num_creatures);
    Tran->Start();

    p = DF->getProcess();
    DFHack::OffsetGroup* itemGroup = mem->getGroup("Items");
    unsigned vector_addr = itemGroup->getAddress("items_vector");
    DFHack::DfVector <uint32_t> p_items (p, vector_addr);
    uint32_t size = p_items.size();

    printf("Found total %d items.\n", size);

    for (i=0;i<size;i++)
    {
        uint32_t curItem = p_items[i];
        DFHack::dfh_item itm;
        Items->readItem(curItem, itm);

        if (!itm.base.flags.owned)
            continue;

        bool confiscate = false;
        bool dump = false;

        if (itm.base.flags.rotten)
        {
            printf("Confiscating a rotten item: \t");
            confiscate = true;
        }
        else if (itm.wear_level >= wear_dump_level)
        {
            printf("Confiscating and dumping a worn item: \t");
            confiscate = true;
            dump = true;
        }
        else if (dump_scattered && itm.base.flags.on_ground)
        {
            printf("Confiscating and dumping litter: \t");
            confiscate = true;
            dump = true;
        }
        else if (confiscate_all)
        {
            printf("Confiscating: \t");
            confiscate = true;
        }

        if (confiscate)
        {
            itm.base.flags.owned = 0;
            if (dump)
                itm.base.flags.dump = 1;

            if (!dry_run)
                Items->writeItem(itm);

            printf(
                "%s (wear %d)",
                Items->getItemDescription(itm, Materials).c_str(),
                itm.wear_level
            );

            int32_t owner = Items->getItemOwnerID(itm);
            int32_t owner_index = Creatures->FindIndexById(owner);
            std::string info;

            if (owner_index >= 0)
            {
                DFHack::t_creature temp;
                Creatures->ReadCreature(owner_index,temp);
                temp.name.first_name[0] = toupper(temp.name.first_name[0]);
                info = temp.name.first_name;
                if (temp.name.nickname[0])
                    info += std::string(" '") + temp.name.nickname + "'";
                info += " ";
                info += Tran->TranslateName(temp.name,false);
                printf(", owner %s", info.c_str());
            }

            printf("\n");
/*
            printf(
                "%5d: %08x %08x (%d,%d,%d) #%08x [%d] %s - %s %s\n",
                i, itm.origin, itm.base.flags.whole,
                itm.base.x, itm.base.y, itm.base.z,
                itm.base.vtable,
                itm.wear_level,
                Items->getItemClass(itm.matdesc.itemType).c_str(),
                Items->getItemDescription(itm, Materials).c_str(),
                info.c_str()
                  );
                  */
        }
    }

#ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
#endif
    return 0;
}
