// Dry Buckets : Remove all "water" objects from buckets scattered around the fortress
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
#include <dfhack/modules/Items.h>

using namespace DFHack;

DFhackCExport command_result df_drybuckets (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
	return "drybuckets";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
	commands.clear();
	commands.push_back(PluginCommand("drybuckets", "Removes water from buckets.", df_drybuckets));
	return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
	return CR_OK;
}

DFhackCExport command_result df_drybuckets (Core * c, vector <string> & parameters)
{
	if(parameters.size() > 0)
	{
		string & p = parameters[0];
		if(p == "?" || p == "help")
		{
			c->con.print("This utility removes all objects of type LIQUID_MISC:NONE and material WATER:NONE - that is, water stored in buckets.\n");
			return CR_OK;
		}
	}
	c->Suspend();
	DFHack::Items * Items = c->getItems();

	vector <df_item *> p_items;
	if(!Items->readItemVector(p_items))
	{
		c->con.printerr("Can't access the item vector.\n");
		c->Resume();
		return CR_FAILURE;
	}
	std::size_t numItems = p_items.size();

	int dried_total = 0;
	for (std::size_t i = 0; i < numItems; i++)
	{
		df_item *item = p_items[i];
		if ((item->getType() == 72) && (item->getMaterial() == 6))
		{
			item->flags.garbage_colect = 1;
			dried_total++;
		}
	}
	c->Resume();
	c->con.print("Done. %d buckets of water emptied.\n", dried_total);
	return CR_OK;
}