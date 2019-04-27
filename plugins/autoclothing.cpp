// This is a generic plugin that does nothing useful apart from acting as an example... of a plugin that does nothing :D

// some headers required for a plugin. Nothing special, just the basics.
#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

#include <map>

// DF data structure definition headers
#include "DataDefs.h"

#include "modules/Items.h"
#include "modules/Maps.h"
#include "modules/Materials.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/manager_order.h"
#include "df/creature_raw.h"
#include "df/world.h"

using namespace DFHack;
using namespace DFHack::Items;
using namespace DFHack::Units;
using namespace df::enums;

struct ClothingRequirement
{
    df::job_type job_type;
    df::item_type item_type;
    int16_t item_subtype;
    df::job_material_category material_category;
    int16_t needed_per_citizen;
    std::map<int16_t, int32_t> total_needed_per_race;
};

std::vector<ClothingRequirement>clothingOrders;

// A plugin must be able to return its name and version.
// The name string provided must correspond to the filename -
// skeleton.plug.so, skeleton.plug.dylib, or skeleton.plug.dll in this case
DFHACK_PLUGIN("autoclothing");

// Any globals a plugin requires (e.g. world) should be listed here.
// For example, this line expands to "using df::global::world" and prevents the
// plugin from being loaded if df::global::world is null (i.e. missing from symbols.xml):
//
REQUIRE_GLOBAL(world);

// Only run if this is enabled
DFHACK_PLUGIN_IS_ENABLED(autoclothing_enabled);

// Here go all the command declarations...
// mostly to allow having the mandatory stuff on top of the file and commands on the bottom
command_result autoclothing(color_ostream &out, std::vector <std::string> & parameters);

static void do_autoclothing();

// Mandatory init function. If you have some global state, create it here.
DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands)
{
    // Fill the command list with your commands.
    commands.push_back(PluginCommand(
        "autoclothing", "Automatically manage clothing work orders",
        autoclothing, false, /* true means that the command can't be used from non-interactive user interface */
        // Extended help string. Used by CR_WRONG_USAGE and the help command:
        "  This command does nothing at all.\n"
        "Example:\n"
        "  skeleton\n"
        "    Does nothing.\n"
    ));
    return CR_OK;
}

// This is called right before the plugin library is removed from memory.
DFhackCExport command_result plugin_shutdown(color_ostream &out)
{
    // You *MUST* kill all threads you created before this returns.
    // If everything fails, just return CR_FAILURE. Your plugin will be
    // in a zombie state, but things won't crash.
    return CR_OK;
}

// Called to notify the plugin about important state changes.
// Invoked with DF suspended, and always before the matching plugin_onupdate.
// More event codes may be added in the future.

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_WORLD_LOADED:
        // initialize from the world just loaded
        break;
    case SC_WORLD_UNLOADED:
        // cleanup
        break;
    default:
        break;
    }
    return CR_OK;
}


// Whatever you put here will be done in each game step. Don't abuse it.
// It's optional, so you can just comment it out like this if you don't need it.

DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
    if (!autoclothing_enabled)
        return CR_OK;

    if (!Maps::IsValid())
        return CR_OK;

    if (DFHack::World::ReadPauseState())
        return CR_OK;

    if ((world->frame_counter + 500) % 1200 != 0) // Check every day, but not the same day as other things
        return CR_OK;

    do_autoclothing();

    return CR_OK;
}


// A command! It sits around and looks pretty. And it's nice and friendly.
command_result autoclothing(color_ostream &out, std::vector <std::string> & parameters)
{
    // It's nice to print a help message you get invalid options
    // from the user instead of just acting strange.
    // This can be achieved by adding the extended help string to the
    // PluginCommand registration as show above, and then returning
    // CR_WRONG_USAGE from the function. The same string will also
    // be used by 'help your-command'.
    if (!parameters.empty())
        return CR_WRONG_USAGE;
    // Commands are called from threads other than the DF one.
    // Suspend this thread until DF has time for us. If you
    // use CoreSuspender, it'll automatically resume DF when
    // execution leaves the current scope.
    CoreSuspender suspend;
    // Actually do something here. Yay.
    out.print("Hello! I do nothing, remember?\n");
    // Give control back to DF.
    return CR_OK;
}

static void find_needed_clothing_items()
{
    for (auto&& unit : world->units.active)
    {
        //obviously we don't care about illegal aliens.
        if (!isCitizen(unit))
            continue;

        //now check each clothing order to see what the unit might be missing.
        for (auto&& clothingOrder : clothingOrders)
        {
            int alreadyOwnedAmount = 0;

            //looping through the items first, then clothing order might be a little faster, but this way is cleaner.
            for (auto&& ownedItem : unit->owned_items)
            {
                auto item = findItemByID(ownedItem);

                if (item->getType() != clothingOrder.item_type)
                    continue;
                if (item->getSubtype() != clothingOrder.item_subtype)
                    continue;

                MaterialInfo matInfo;
                matInfo.decode(item);

                if (!matInfo.matches(clothingOrder.material_category))
                    continue;

                alreadyOwnedAmount++;
            }
            int neededAmount = clothingOrder.needed_per_citizen - alreadyOwnedAmount;

            if (neededAmount <= 0)
                continue;

            //technically, there's some leeway in sizes, but only caring about exact sizes is simpler.
            clothingOrder.total_needed_per_race[unit->race] += alreadyOwnedAmount;
        }
    }
}

static void remove_available_clothing()
{
    for (auto&& item : world->items.all)
    {
        //skip any owned items.
        if (getOwner(item))
            continue;

        //again, for each item, find if any clothing order matches the 
        for (auto&& clothingOrder : clothingOrders)
        {
            if (item->getType() != clothingOrder.item_type)
                continue;
            if (item->getSubtype() != clothingOrder.item_subtype)
                continue;

            MaterialInfo matInfo;
            matInfo.decode(item);

            if (!matInfo.matches(clothingOrder.material_category))
                continue;

            clothingOrder.total_needed_per_race[item->getMakerRace()] --;
        }
    }
}

static void add_clothing_orders()
{
    for (auto&& clothingOrder : clothingOrders)
    {
        for (auto&& orderNeeded : clothingOrder.total_needed_per_race)
        {
            auto race = orderNeeded.first;
            auto amount = orderNeeded.second;
            //Previous operations can easily make this negative. That jus means we have more than we need already.
            if (amount <= 0)
                continue;

            bool orderExistedAlready = false;
            for (auto&& managerOrder : world->manager_orders)
            {
                //Annoyingly, the manager orders store the job type for clothing orders, and actual item type is left at -1;
                if (managerOrder->job_type != clothingOrder.job_type)
                    continue;
                if (managerOrder->item_subtype != clothingOrder.item_subtype)
                    continue;
                if (managerOrder->hist_figure_id != race)
                    continue;

                //We found a work order, that means we don't need to make a new one.
                orderExistedAlready = true;
                amount -= managerOrder->amount_left;
                if (amount > 0)
                {
                    managerOrder->amount_left += amount;
                    managerOrder->amount_total += amount;
                }
            }
            //if it wasn't there, we need to make a new one.
            if (!orderExistedAlready)
            {
                df::manager_order newOrder;

                newOrder.id = world->manager_order_next_id;
                world->manager_order_next_id++;
                newOrder.job_type = clothingOrder.job_type;
                newOrder.item_subtype = clothingOrder.item_subtype;
                newOrder.hist_figure_id = race;
                newOrder.material_category = clothingOrder.material_category;
                newOrder.amount_left = amount;
                newOrder.amount_total = amount;
                world->manager_orders.push_back(&newOrder);
            }
        }
    }
}

static void do_autoclothing()
{
    if (clothingOrders.size() == 0)
        return;

    //first we look through all the units on the map to see who needs new clothes.
    find_needed_clothing_items();

    //Now we go through all the items in the map to see how many clothing items we have but aren't owned yet.
    remove_available_clothing();

    //Finally loop through the clothing orders to find ones that need more made.
    add_clothing_orders();
}
