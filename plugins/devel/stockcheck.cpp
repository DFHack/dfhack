#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/ui.h"
#include "df/building_stockpilest.h"
#include "df/global_objects.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/item.h"
#include "df/items_other_id.h"
#include "modules/MapCache.h"

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

using df::global::world;
using df::global::ui;
using df::global::selection_rect;

using df::building_stockpilest;

static command_result copystock(color_ostream &out, vector <string> & parameters);
static command_result stockcheck(color_ostream &out, vector <string> & parameters);
static bool copystock_guard(df::viewscreen *top);

DFHACK_PLUGIN("stockcheck");

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (world && ui) {
		commands.push_back(
			PluginCommand("stockcheck", "Check for stockpile adequacy.",
				stockcheck, false, 
				"Scan world for unstockpiled items and verify stockpiles exist for them.\n"
			)
		);
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

static command_result stockcheck(color_ostream &out, vector <string> & parameters)
{
	CoreSuspender suspend;
	MapExtras::MapCache mc;

	for (int i = 0; i < world->buildings.all.size(); ++i)
	{
		df::building *build = world->buildings.all[i];
		auto type = build->getType();
		if (df::enums::building_type::Stockpile == type)
		{
			building_stockpilest *sp = virtual_cast<building_stockpilest>(build);
			df::stockpile_settings st = sp->settings;
			out << "Stockpile " << sp->stockpile_number << ":\n";
			out << " width=" << build->room.width << " height= " << build->room.height << endl;

			int x1 = build->room.x; 
			int x2 = build->room.x + build->room.width;
			int y1 = build->room.y;
			int y2 = build->room.y + build->room.height;
			int e = 0;
			int size = 0, free = 0;
			for (int x = x1; x < x2; x++) 
				for (int y = y1; y < y2; y++) 
					if (build->room.extents[e++] == 1) 
					{
						size++;
						DFCoord cursor (x,y,build->z);
						uint32_t blockX = x / 16;
						uint32_t tileX = x % 16;
						uint32_t blockY = y / 16;
						uint32_t tileY = y % 16;
						MapExtras::Block * b = mc.BlockAt(cursor/16);
						if(b && b->is_valid())
						{
							auto &block = *b->getRaw();
						    df::tile_occupancy &occ = block.occupancy[tileX][tileY];
							if (!occ.bits.item)
								free++;
						}
					}
						
			out << " size=" << size << " free= " << free << endl;
		}
	}

	std::vector<df::item*> &items = world->items.other[items_other_id::ANY_FREE];

	// Precompute a bitmask with the bad flags
    df::item_flags bad_flags;
    bad_flags.whole = 0;

#define F(x) bad_flags.bits.x = true;
    F(dump); F(forbid); F(garbage_collect);
    F(hostile); F(on_fire); F(rotten); F(trader);
    F(in_building); F(construction); F(artifact1);
#undef F

    for (size_t i = 0; i < items.size(); i++)
    {
        df::item *item = items[i];
		if (item->flags.whole & bad_flags.whole)
            continue;


	}
	out << "Stockcheck done" << endl;

	return CR_OK;
}
