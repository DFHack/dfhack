// A container for random minor tweaks that don't fit anywhere else,
// in order to avoid creating lots of plugins and polluting the namespace.

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/Gui.h"
#include "modules/Units.h"
#include "modules/Items.h"

#include "DataDefs.h"
#include "df/ui.h"
#include "df/world.h"
#include "df/squad.h"
#include "df/unit.h"
#include "df/unit_soul.h"
#include "df/historical_entity.h"
#include "df/historical_figure.h"
#include "df/historical_figure_info.h"
#include "df/assumed_identity.h"
#include "df/language_name.h"
#include "df/death_info.h"
#include "df/criminal_case.h"
#include "df/unit_inventory_item.h"

#include <stdlib.h>

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

using df::global::ui;
using df::global::world;

using namespace DFHack::Gui;

static command_result tweak(color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("tweak");

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "tweak", "Various tweaks for minor bugs.", tweak, false,
        "  tweak clear-missing\n"
        "    Remove the missing status from the selected unit.\n"
        "  tweak clear-ghostly\n"
        "    Remove the ghostly status from the selected unit.\n"
        "    Intended to fix the case where you can't engrave memorials for ghosts.\n"
        "    Note that this is very dirty and possibly dangerous!\n"
        "    Most probably does not have the positive effect of a proper burial.\n"
        "  tweak clear-resident\n"
        "    Remove the resident flag from the selected unit.\n"
        "    Intended to fix bugged migrants who stay at the map edge.\n"
        "    Only works for dwarves of the own civilization.\n"
        "  tweak clear-merchant\n"
        "    Remove the merchant flag from the selected unit.\n"
        "    Assimilates bugged merchants who don't leave the map into your fort.\n"
        "    Only works for dwarves of the own civilization.\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    return CR_OK;
}

static command_result lair(color_ostream &out, std::vector<std::string> & params);


// to be called by tweak-merchant and tweak-resident
// units forced into the fort by removing the flags do not own their clothes
// which has the result that they drop all their clothes and become unhappy because they are naked
command_result fix_clothing_ownership(color_ostream &out, df::unit* unit)
{
    // first, find one owned item to initialize the vtable
    bool vt_initialized = false;
    size_t numItems = world->items.all.size();
    for(size_t i=0; i< numItems; i++)
    {
        df::item * item = world->items.all[i];
        if(Items::getOwner(item))
        {
            vt_initialized = true;
            break;
        }
    }
    if(!vt_initialized)
    {
        out << "fix_clothing_ownership: could not initialize vtable!" << endl;
        return CR_FAILURE;
    }

    int fixcount = 0;
    for(size_t j=0; j<unit->inventory.size(); j++)
    {
        df::unit_inventory_item* inv_item = unit->inventory[j];
        df::item* item = inv_item->item;
        if(inv_item->mode == df::unit_inventory_item::T_mode::Worn)
        {
            // ignore armor?
            // it could be leather boots, for example, in which case it would not be nice to forbid ownership
            //if(item->getEffectiveArmorLevel() != 0)
            //    continue;

            if(!Items::getOwner(item))
            {
                if(Items::setOwner(item, unit))
                    fixcount++;
                else
                    out << "could not change ownership for item!" << endl;
            }
        }
    }
    out << "ownership for " << fixcount << " clothes fixed" << endl;
    return CR_OK;
}

static command_result tweak(color_ostream &out, vector <string> &parameters)
{
    CoreSuspender suspend;

    if (parameters.empty())
        return CR_WRONG_USAGE;

    string cmd = parameters[0];

    if (cmd == "clear-missing")
    {
        df::unit *unit = getSelectedUnit(out);
        if (!unit)
            return CR_FAILURE;

        auto death = df::death_info::find(unit->counters.death_id);

        if (death)
        {
            death->flags.bits.discovered = true;

            auto crime = df::criminal_case::find(death->crime_id);
            if (crime)
                crime->flags.bits.discovered = true;
        }
    }
    else if (cmd == "clear-ghostly")
    {
        df::unit *unit = getSelectedUnit(out);
        if (!unit)
            return CR_FAILURE;

        // don't accidentally kill non-ghosts!
        if (unit->flags3.bits.ghostly)
        {
            // remove ghostly, set to dead instead
            unit->flags3.bits.ghostly = 0;
            unit->flags1.bits.dead = 1;
        }
        else
        {
            out.print("That's not a ghost!\n");
            return CR_FAILURE;
        }
    }
    else if (cmd == "clear-resident")
    {
        df::unit *unit = getSelectedUnit(out);
        if (!unit)
            return CR_FAILURE;

        // must be own race and civ and a merchant
        if (   unit->flags2.bits.resident
            && unit->race == df::global::ui->race_id
            && unit->civ_id == df::global::ui->civ_id)
        {
            // remove resident flag
            unit->flags2.bits.resident = 0;
            return fix_clothing_ownership(out, unit);
        }
        else
        {
            out.print("That's not a resident dwarf of your civilization!\n");
            return CR_FAILURE;
        }
    }
    else if (cmd == "clear-merchant")
    {
        df::unit *unit = getSelectedUnit(out);
        if (!unit)
            return CR_FAILURE;

        // must be own race and civ and a merchant
        if (   unit->flags1.bits.merchant
            && unit->race == df::global::ui->race_id
            && unit->civ_id == df::global::ui->civ_id)
        {
            // remove merchant flag
            unit->flags1.bits.merchant = 0;
            return fix_clothing_ownership(out, unit);
        }
        else
        {
            out.print("That's not a dwarf merchant of your civilization!\n");
            return CR_FAILURE;
        }
    }
    else return CR_WRONG_USAGE;

    return CR_OK;
}
