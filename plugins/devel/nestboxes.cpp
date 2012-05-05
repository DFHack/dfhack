#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/ui.h"
#include "df/building_nest_boxst.h"
#include "df/building_type.h"
#include "df/global_objects.h"
#include "df/item.h"
#include "df/unit.h"
#include "df/building.h"
#include "df/items_other_id.h"
#include "df/creature_raw.h"
#include "modules/MapCache.h"
#include "modules/Items.h"


using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

using df::global::world;
using df::global::ui;

static command_result nestboxes(color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("nestboxes");

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (world && ui) {
		commands.push_back(
			PluginCommand("nestboxes", "Derp.",
				nestboxes, false, 
				"Derp.\n"
			)
		);
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}


static command_result nestboxes(color_ostream &out, vector <string> & parameters)
{
	CoreSuspender suspend;
	bool clean = false;
    int dump_count = 0;
    int good_egg = 0;

    if (parameters.size() == 1 && parameters[0] == "clean") 
    {
        clean = true;
    }
	for (int i = 0; i < world->buildings.all.size(); ++i)
	{
		df::building *build = world->buildings.all[i];
		auto type = build->getType();
		if (df::enums::building_type::NestBox == type)
		{
            bool needs_clean = false;
			df::building_nest_boxst *nb = virtual_cast<df::building_nest_boxst>(build);
			out << "Nestbox at (" << nb->x1 << "," << nb->y1 << ","<< nb->z << "): claimed-by " << nb->claimed_by << ", contained item count " << nb->contained_items.size() << " (" << nb->anon_1 << ")" << endl;
            if (nb->contained_items.size() > 1)
                needs_clean = true;
            if (nb->claimed_by != -1) 
            {
                df::unit* u = df::unit::find(nb->claimed_by);
                if (u) 
                {
                    out << "  Claimed by ";
                    if (u->name.has_name) 
                        out << u->name.first_name << ", ";
                    df::creature_raw *raw = df::global::world->raws.creatures.all[u->race];
                    out << raw->creature_id
                        << ", pregnancy timer " << u->relations.pregnancy_timer << endl;
                    if (u->relations.pregnancy_timer > 0) 
                        needs_clean = false;
                }
            }
            for (int j = 1; j < nb->contained_items.size(); j++)
            {
                df::item* item = nb->contained_items[j]->item;
                if (needs_clean) {
                    if (clean && !item->flags.bits.dump) 
                    {
                        item->flags.bits.dump = 1;
                        dump_count += item->getStackSize();
                    
                    }
                } else {
                    good_egg += item->getStackSize();
                }
            }
		}
    }

    if (clean)
    {
        out << dump_count << " eggs dumped." << endl;
    }
    out << good_egg << " fertile eggs found." << endl;


	return CR_OK;
}

