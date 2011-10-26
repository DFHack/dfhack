// Clean Items : Remove contaminants from all objects
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

DFhackCExport command_result df_cleanitems (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
	return "cleanitems";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
	commands.clear();
	commands.push_back(PluginCommand("cleanitems", "Removes contaminants from items.", df_cleanitems));
	return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
	return CR_OK;
}

DFhackCExport command_result df_cleanitems (Core * c, vector <string> & parameters)
{
	if(parameters.size() > 0)
	{
		string & p = parameters[0];
		if(p == "?" || p == "help")
		{
			c->con.print("This utility removes all contaminants from all objects in your fortress.\n");
			return CR_OK;
		}
	}
	c->Suspend();
	DFHack::Items * Items = c->getItems();

	vector <t_item*> p_items;
	if(!Items->readItemVector(p_items))
	{
		c->con.printerr("Can't access the item vector.\n");
		c->Resume();
		return CR_FAILURE;
	}
	std::size_t numItems = p_items.size();

	int cleaned_items = 0, cleaned_total = 0;
	for (std::size_t i = 0; i < numItems; i++)
	{
		t_item * itm = p_items[i];
		// TODO - get peterix to expand the item base class so it includes this pointer
		uint32_t cont_ptr = *(uint32_t *)((int8_t *)itm + 0x0064);
		if (!cont_ptr)
			continue;
		std::vector<void *> *contaminants = (std::vector<void *> *)cont_ptr;
		if (contaminants->size())
		{
			cleaned_items++;
			cleaned_total += contaminants->size();
			contaminants->clear();
		}
	}
	c->Resume();
	c->con.print("Done. %d items cleaned of %d contaminants.\n", cleaned_items, cleaned_total);
	return CR_OK;
}