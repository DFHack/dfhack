/*
 * Confiscates and dumps garbage owned by dwarfs.
 */

#include <sstream>
#include <climits>
#include <vector>
#include <set>
using namespace std;

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include <vector>
#include <string>
#include "modules/Items.h"
#include "modules/Units.h"
#include "modules/Materials.h"
#include "modules/Translation.h"
using namespace DFHack;
using namespace DFHack::Simple;
#include "DataDefs.h"
#include "df/world.h"

using df::global::world;

DFhackCExport command_result df_cleanowned (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "cleanowned";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("cleanowned",
                                     "Confiscates and dumps garbage owned by dwarfs.",
                                     df_cleanowned));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    return CR_OK;
}

DFhackCExport command_result df_cleanowned (Core * c, vector <string> & parameters)
{
    bool dump_scattered = false;
    bool confiscate_all = false;
    bool dry_run = false;
    int wear_dump_level = 65536;

    for(std::size_t i = 0; i < parameters.size(); i++)
    {
        string & param = parameters[i];
        if(param == "dryrun")
            dry_run = true;
        else if(param == "scattered")
            dump_scattered = true;
        else if(param == "all")
            confiscate_all = true;
        else if(param == "x")
            wear_dump_level = 1;
        else if(param == "X")
            wear_dump_level = 2;
        else if(param == "?" || param == "help")
        {
            c->con.print("This tool lets you confiscate and dump all the garbage\n"
                         "dwarves ultimately accumulate.\n"
                         "By default, only rotten and dropped food is confiscated.\n"
                         "Options:\n"
                         "  dryrun    - don't actually do anything, just print what would be done.\n"
                         "  scattered - confiscate owned items on the ground\n"
                         "  all       - confiscate everything\n"
                         "  x         - confiscate & dump 'x' and worse damaged items\n"
                         "  X         - confiscate & dump 'X' and worse damaged items\n"
                         "  ?         - this help\n"
                         "Example:\n"
                         "  confiscate scattered X\n"
                         "  This will confiscate rotten and dropped food, garbage on the floors\n"
                         "  and any worn items wit 'X' damage and above.\n"
            );
            return CR_OK;
        }
        else
        {
            c->con.printerr("Parameter '%s' is not valid. See 'cleanowned help'.\n",param.c_str());
            return CR_FAILURE;
        }
    }
    c->Suspend();
    DFHack::Materials *Materials = c->getMaterials();
    DFHack::Units *Creatures = c->getUnits();
    DFHack::Translation *Tran = c->getTranslation();

    uint32_t num_creatures;
    bool ok = true;
    ok &= Materials->ReadAllMaterials();
    ok &= Creatures->Start(num_creatures);
    ok &= Tran->Start();

    c->con.print("Found total %d items.\n", world->items.all.size());

    for (std::size_t i=0; i < world->items.all.size(); i++)
    {
        df::item * item = world->items.all[i];
        bool confiscate = false;
        bool dump = false;

        if (!item->flags.bits.owned)
        {
            int32_t owner = Items::getItemOwnerID(item);
            if (owner >= 0)
            {
                c->con.print("Fixing a misflagged item: \t");
                confiscate = true;
            }
            else
            {
                continue;
            }
        }

        if (item->flags.bits.rotten)
        {
            c->con.print("Confiscating a rotten item: \t");
            confiscate = true;
        }
        else if (item->flags.bits.on_ground)
        {
            int32_t type = item->getType();
	    if(type == df::item_type::MEAT ||
               type == df::item_type::FISH ||
               type == df::item_type::VERMIN ||
               type == df::item_type::PET ||
               type == df::item_type::PLANT ||
               type == df::item_type::CHEESE ||
               type == df::item_type::FOOD
            )
            {
                confiscate = true;
                if(dump_scattered)
                {
                    c->con.print("Dumping a dropped item: \t");
                    dump = true;
                }
                else
                {
                    c->con.print("Confiscating a dropped item: \t");
                }
            }
            else if(dump_scattered)
            {
                c->con.print("Confiscating and dumping litter: \t");
                confiscate = true;
                dump = true;
            }
        }
        else if (item->getWear() >= wear_dump_level)
        {
            c->con.print("Confiscating and dumping a worn item: \t");
            confiscate = true;
            dump = true;
        }
        else if (confiscate_all)
        {
            c->con.print("Confiscating: \t");
            confiscate = true;
        }

        if (confiscate)
        {
            std::string description;
            item->getItemDescription(&description, 0);
            c->con.print(
                "0x%x %s (wear %d)",
                item,
                description.c_str(),
                item->getWear()
            );

            int32_t owner = Items::getItemOwnerID(item);
            int32_t owner_index = Creatures->FindIndexById(owner);
            std::string info;

            if (owner_index >= 0)
            {
                df::unit * temp = Creatures->GetCreature(owner_index);
                info = temp->name.first_name;
                if (!temp->name.nickname.empty())
                    info += std::string(" '") + temp->name.nickname + "'";
                info += " ";
                info += Tran->TranslateName(&temp->name,false);
                c->con.print(", owner %s", info.c_str());
            }

            if (!dry_run)
            {
                if (!Items::removeItemOwner(item, Creatures))
                    c->con.print("(unsuccessfully) ");
                if (dump)
                    item->flags.bits.dump = 1;
            }
            c->con.print("\n");
        }
    }
    c->Resume();
    return CR_OK;
}
