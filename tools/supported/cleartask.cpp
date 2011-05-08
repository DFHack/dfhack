// clears the "tasked" flag on all items
// original code by Quietust (http://www.bay12forums.com/smf/index.php?action=profile;u=18111)
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

int main ()
{
    DFHack::Process * p;
    unsigned int i;
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context * DF;
    DFHack::Items * Items;
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

    p = DF->getProcess();
    uint32_t item_vec_offset = 0;
    try
    {
        Items = DF->getItems();
        DFHack::OffsetGroup* itemGroup = p->getDescriptor()->getGroup("Items");
        item_vec_offset = itemGroup->getAddress("items_vector");
    }
    catch(DFHack::Error::All & e)
    {
        cerr << "Fatal error, exiting :(" << endl << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }

    DFHack::DfVector <uint32_t> p_items (p, item_vec_offset);
    uint32_t size = p_items.size();

    int numtasked = 0;
    for (i=0;i<size;i++)
    {
        DFHack::dfh_item temp;
        Items->readItem(p_items[i],temp);
        DFHack::t_itemflags & flags = temp.base.flags;
        if (flags.in_job)
        {
            flags.in_job = 0;
            Items->writeItem(temp);
            numtasked++;
        }
    }
    cout << "Found and untasked " << numtasked << " items." << endl;

#ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
#endif
    return 0;
}
