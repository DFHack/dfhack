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
#include "modules/Translation.h"
#include "DataDefs.h"
#include "df/world.h"

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("cleanowned");
REQUIRE_GLOBAL(world);

command_result df_cleanowned (color_ostream &out, vector <string> & parameters);

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "cleanowned",
        "Confiscates and dumps garbage owned by dwarves.",
        df_cleanowned));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

command_result df_cleanowned (color_ostream &out, vector <string> & parameters)
{
    bool dump_scattered = false;
    bool confiscate_all = false;
    bool dry_run = false;
    int wear_dump_level = 65536;
    bool nodump = false;

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
        else if(param == "nodump")
            nodump = true;
        else
            return CR_WRONG_USAGE;
    }

    CoreSuspender suspend;

    if (!Translation::IsValid())
    {
        out.printerr("Translation data unavailable!\n");
        return CR_FAILURE;
    }

    for (std::size_t i=0; i < world->items.all.size(); i++)
    {
        df::item * item = world->items.all[i];
        bool confiscate = false;
        bool dump = false;

        if (!item->flags.bits.owned)
        {
            if (Items::getOwner(item))
            {
                out.print("Fixing a misflagged item: \t");
                confiscate = true;
            }
            else
            {
                continue;
            }
        }

        if (item->flags.bits.rotten)
        {
            out.print("Confiscating a rotten item: \t");
            confiscate = true;
        }
        else if (item->flags.bits.on_ground)
        {
            df::item_type type = item->getType();
            if(type == item_type::MEAT ||
               type == item_type::FISH ||
               type == item_type::VERMIN ||
               type == item_type::PET ||
               type == item_type::PLANT ||
               type == item_type::CHEESE ||
               type == item_type::FOOD
            )
            {
                confiscate = true;
                if(dump_scattered && !nodump)
                {
                    out.print("Dumping a dropped item: \t");
                    dump = true;
                }
                else
                {
                    out.print("Confiscating a dropped item: \t");
                }
            }
            else if(dump_scattered)
            {
                if (nodump)
                    out.print("Confiscating litter: \t");
                else
                {
                    out.print("Confiscating and dumping litter: \t");
                    dump = true;
                }
                confiscate = true;   
            }
        }
        else if (item->getWear() >= wear_dump_level)
        {
            if (nodump)
                out.print("Confiscating a worn item: \t");
            else
            {
                out.print("Confiscating and dumping a worn item: \t");
                dump = true;
            }
            confiscate = true;
        }
        else if (confiscate_all)
        {
            out.print("Confiscating: \t");
            confiscate = true;
        }

        if (confiscate)
        {
            std::string description;
            item->getItemDescription(&description, 0);
            out.print(
                "[%d] %s (wear level %d)",
                item->id,
                DF2CONSOLE(description).c_str(),
                item->getWear()
            );

            df::unit *owner = Items::getOwner(item);

            if (owner)
                out.print(", owner %s", DF2CONSOLE(Translation::TranslateName(&owner->name,false)).c_str());

            if (!dry_run)
            {
                if (!Items::setOwner(item,NULL))
                    out.print("(unsuccessfully) ");
                if (dump)
                    item->flags.bits.dump = 1;
            }
            out.print("\n");
        }
    }
    return CR_OK;
}
