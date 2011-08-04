// clears the "tasked" flag on all items
// original code by Quietust (http://www.bay12forums.com/smf/index.php?action=profile;u=18111)
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <climits>
#include <vector>
using namespace std;

#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <dfhack/modules/Items.h>
#include <dfhack/Types.h>
#include <dfhack/extra/termutil.h>
using namespace DFHack;

DFhackCExport command_result df_cleartask (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "cleartask";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("cleartask",
                                     "Clears the \"tasked\" flag on all items. This is dangerous. Only use after reclaims.",
                                     df_cleartask));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

DFhackCExport command_result df_cleartask (Core * c, vector <string> & parameters)
{
    c->Suspend();
    DFHack::Items * Items = c->getItems();
    uint32_t item_vec_offset = 0;
    vector <t_item *> p_items;
    if(!Items->readItemVector(p_items))
    {
        c->con.printerr("Can't read items...\n");
        c->Resume();
        return CR_FAILURE;
    }

    int numtasked = 0;
    for (std::size_t i = 0; i < p_items.size(); i++)
    {
        t_item * ptr;
        DFHack::dfh_item temp;
        Items->readItem(p_items[i],temp);
        if (ptr->flags.in_job)
        {
            ptr->flags.in_job = 0;
            numtasked++;
        }
    }
    c->con.print("Found and untasked %d items.\n", numtasked);
    c->Resume();
    return CR_OK;
}
