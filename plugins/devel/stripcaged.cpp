
#include <iostream>
#include <iomanip>
#include <climits>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <ctime>
#include <cstdio>
using namespace std;

#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Units.h"
#include "modules/Maps.h"
#include "modules/Gui.h"
#include "modules/World.h"
#include "MiscUtils.h"

#include <df/plotinfost.h>
#include "df/world.h"
#include "df/world_raws.h"
#include "df/building_def.h"
#include "df/unit_inventory_item.h"
#include <df/creature_raw.h>
#include <df/caste_raw.h>
#include "df/general_ref.h"
#include "df/item.h"
#include "df/unit.h"

using std::vector;
using std::string;
using namespace DFHack;
using namespace df::enums;
using df::global::world;
using df::global::plotinfo;

using namespace DFHack::Gui;

command_result df_stripcaged(color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("stripcaged");

// check if contained in item (e.g. animals in cages)
bool isContainedInItem(df::unit* unit)
{
    bool contained = false;
    for (size_t r=0; r < unit->general_refs.size(); r++)
    {
        df::general_ref * ref = unit->general_refs[r];
        auto rtype = ref->getType();
        if(rtype == df::general_ref_type::CONTAINED_IN_ITEM)
        {
            contained = true;
            break;
        }
    }
    return contained;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "stripcaged", "strip caged units of all items",
        df_stripcaged, false,
        "Clears forbid and sets dump for the inventories of all caged units."
        ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

command_result df_stripcaged(color_ostream &out, vector <string> & parameters)
{
    bool keeparmor = true;

    if (parameters.size() == 1 && parameters[0] == "dumparmor")
    {
        out << "Dumping armor too" << endl;
        keeparmor = false;
    }

    size_t count = 0;
    for (size_t i=0; i < world->units.all.size(); i++)
    {
        df::unit* unit = world->units.all[i];
        if (isContainedInItem(unit))
        {
            for (size_t j=0; j < unit->inventory.size(); j++)
            {
                df::unit_inventory_item* uii = unit->inventory[j];
                if (uii->item)
                {
                    if (keeparmor && (uii->item->isArmorNotClothing() || uii->item->isClothing()))
                        continue;
                    std::string desc;
                    uii->item->getItemDescription(&desc,0);
                    out << "Item " << desc << " dumped." << endl;
                    uii->item->flags.bits.forbid = 0;
                    uii->item->flags.bits.dump = 1;
                    count++;
                }
            }
        }
    }

    out << count << " items marked for dumping" << endl;

    return CR_OK;
}
