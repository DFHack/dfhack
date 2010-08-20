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
    p = DF->getProcess();
    uint32_t item_vec_offset = 0;
    try
    {
        item_vec_offset = p->getDescriptor()->getAddress ("items_vector");
    }
    catch(DFHack::Error::MissingMemoryDefinition & e)
    {
        cerr << "missing offset for the item vector, exiting :(" << endl;
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
        DFHack::t_itemflags flags;
        flags.whole = p->readDWord(p_items[i] + 0x0C);
        if (flags.bits.in_job)
        {
            flags.bits.in_job = 0;
            p->writeDWord(p_items[i] + 0x0C, flags.whole);
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