// Clean Units : Remove contaminants from all creatures
#include <iostream>
#include <iomanip>
#include <sstream>
#include <climits>
#include <vector>
#include <set>
using namespace std;

#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <vector>
#include <string>
#include <algorithm>
#include <dfhack/modules/Creatures.h>

using namespace DFHack;

DFhackCExport command_result df_cleanunits (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "cleanunits";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("cleanunits", "Removes contaminants from creatures.", df_cleanunits));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

DFhackCExport command_result df_cleanunits (Core * c, vector <string> & parameters)
{
    if(parameters.size() > 0)
    {
        string & p = parameters[0];
        if(p == "?" || p == "help")
        {
            c->con.print("This utility removes all contaminants from all creatures in your fortress.\n");
            return CR_OK;
        }
    }
    c->Suspend();
    DFHack::Creatures * Creatures = c->getCreatures();

    uint32_t num_creatures;
    if (!Creatures->Start(num_creatures))
    {
        c->con.printerr("Can't read unit list!\n");
        c->Resume();
        return CR_FAILURE;
    }

    int cleaned_units = 0, cleaned_total = 0;
    for (std::size_t i = 0; i < num_creatures; i++)
    {
        df_creature *unit = Creatures->creatures->at(i);
        int num = unit->contaminants.size();
        if (num)
        {
            cleaned_units++;
            cleaned_total += num;
            unit->contaminants.clear();
        }
    }
    c->Resume();
    c->con.print("Done. %d creatures cleaned of %d contaminants.\n", cleaned_units, cleaned_total);
    return CR_OK;
}