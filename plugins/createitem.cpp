// Create arbitrary items

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "MiscUtils.h"

#include "modules/Maps.h"
#include "modules/MapCache.h"
#include "modules/Gui.h"
#include "modules/Items.h"
#include "modules/Materials.h"

#include "DataDefs.h"
#include "df/game_type.h"
#include "df/world.h"
#include "df/ui.h"
#include "df/unit.h"
#include "df/historical_entity.h"
#include "df/world_site.h"
#include "df/item.h"
#include "df/creature_raw.h"
#include "df/caste_raw.h"
#include "df/reaction_reagent.h"
#include "df/reaction_product_itemst.h"
#include "df/tool_uses.h"

using namespace std;
using namespace DFHack;

using df::global::world;
using df::global::ui;
using df::global::gametype;

DFHACK_PLUGIN("createitem");

int dest_container = -1, dest_building = -1;

command_result df_createitem (color_ostream &out, vector <string> & parameters);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector<PluginCommand> &commands)
{
    commands.push_back(PluginCommand("createitem", "Create arbitrary items.", df_createitem, false,
        "Syntax: createitem <item> <material> [count]\n"
        "    <item> - Item token for what you wish to create, as specified in custom\n"
        "             reactions. If the item has no subtype, omit the :NONE.\n"
        "    <material> - The material you want the item to be made of, as specified\n"
        "                 in custom reactions. For REMAINS, FISH, FISH_RAW, VERMIN,\n"
        "                 PET, and EGG, replace this with a creature ID and caste.\n"
        "    [count] - How many of the item you wish to create.\n"
        "\n"
        "To use this command, you must select which unit will create the items.\n"
        "By default, items created will be placed at that unit's feet.\n"
        "To change this, type 'createitem <destination>'.\n"
        "Valid destinations:\n"
        "* floor - Place items on floor beneath maker's feet.\n"
        "* item - Place items inside selected container.\n"
        "* building - Place items inside selected building.\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

bool makeItem (df::reaction_product_itemst *prod, df::unit *unit, bool second_item = false)
{
    vector<df::item *> out_items;
    vector<df::reaction_reagent *> in_reag;
    vector<df::item *> in_items;
    bool is_gloves = (prod->item_type == df::item_type::GLOVES);
    bool is_shoes = (prod->item_type == df::item_type::SHOES);

    df::item *container = NULL;
    df::building *building = NULL;
    if (dest_container != -1)
        container = df::item::find(dest_container);
    if (dest_building != -1)
        building = df::building::find(dest_building);

    prod->produce(unit, &out_items, &in_reag, &in_items, 1, df::job_skill::NONE,
        df::historical_entity::find(unit->civ_id),
        ((*gametype == df::game_type::DWARF_MAIN) || (*gametype == df::game_type::DWARF_RECLAIM)) ? df::world_site::find(ui->site_id) : NULL);
    if (!out_items.size())
        return false;
    // if we asked to make shoes and we got twice as many as we asked, then we're okay
    // otherwise, make a second set because shoes are normally made in pairs
    if (is_shoes && out_items.size() == prod->count * 2)
        is_shoes = false;

    MapExtras::MapCache mc;

    for (size_t i = 0; i < out_items.size(); i++)
    {
        bool on_ground = true;
        if (container)
        {
            on_ground = false;
            out_items[i]->flags.bits.removed = 1;
            if (!Items::moveToContainer(mc, out_items[i], container))
                out_items[i]->moveToGround(container->pos.x, container->pos.y, container->pos.z);
        }
        if (building)
        {
            on_ground = false;
            out_items[i]->flags.bits.removed = 1;
            if (!Items::moveToBuilding(mc, out_items[i], (df::building_actual *)building, 0))
                out_items[i]->moveToGround(building->centerx, building->centery, building->z);
        }
        if (on_ground)
            out_items[i]->moveToGround(unit->pos.x, unit->pos.y, unit->pos.z);
        if (is_gloves)
        {
            // if the reaction creates gloves without handedness, then create 2 sets (left and right)
            if (out_items[i]->getGloveHandedness() > 0)
                is_gloves = false;
            else
                out_items[i]->setGloveHandedness(second_item ? 2 : 1);
        }
    }
    if ((is_gloves || is_shoes) && !second_item)
        return makeItem(prod, unit, true);

    return true;
}

command_result df_createitem (color_ostream &out, vector <string> & parameters)
{
    string item_str, material_str;
    df::item_type item_type = df::item_type::NONE;
    int16_t item_subtype = -1;
    int16_t mat_type = -1;
    int32_t mat_index = -1;
    int count = 1;

    if (parameters.size() == 1)
    {
        if (parameters[0] == "floor")
        {
            dest_container = -1;
            dest_building = -1;
            out.print("Items created will be placed on the floor.\n");
            return CR_OK;
        }
        else if (parameters[0] == "item")
        {
            dest_building = -1;
            df::item *item = Gui::getSelectedItem(out);
            if (!item)
            {
                out.printerr("You must select a container!\n");
                return CR_FAILURE;
            }
            switch (item->getType())
            {
            case df::item_type::FLASK:
            case df::item_type::BARREL:
            case df::item_type::BUCKET:
            case df::item_type::ANIMALTRAP:
            case df::item_type::BOX:
            case df::item_type::BIN:
            case df::item_type::BACKPACK:
            case df::item_type::QUIVER:
                break;
            case df::item_type::TOOL:
                if (item->hasToolUse(df::tool_uses::LIQUID_CONTAINER))
                    break;
                if (item->hasToolUse(df::tool_uses::FOOD_STORAGE))
                    break;
                if (item->hasToolUse(df::tool_uses::SMALL_OBJECT_STORAGE))
                    break;
                if (item->hasToolUse(df::tool_uses::TRACK_CART))
                    break;
            default:
                out.printerr("The selected item cannot be used for item storage!\n");
                return CR_FAILURE;
            }
            dest_container = item->id;
            string name;
            item->getItemDescription(&name, 0);
            out.print("Items created will be placed inside %s.\n", name.c_str());
            return CR_OK;
        }
        else if (parameters[0] == "building")
        {
            dest_container = -1;
            df::building *building = Gui::getSelectedBuilding(out);
            if (!building)
            {
                out.printerr("You must select a building!\n");
                return CR_FAILURE;
            }
            switch (building->getType())
            {
            case df::building_type::Coffin:
            case df::building_type::Furnace:
            case df::building_type::TradeDepot:
            case df::building_type::Shop:
            case df::building_type::Box:
            case df::building_type::Weaponrack:
            case df::building_type::Armorstand:
            case df::building_type::Workshop:
            case df::building_type::Cabinet:
            case df::building_type::SiegeEngine:
            case df::building_type::Trap:
            case df::building_type::AnimalTrap:
            case df::building_type::Cage:
            case df::building_type::Wagon:
            case df::building_type::NestBox:
            case df::building_type::Hive:
                break;
            default:
                out.printerr("The selected building cannot be used for item storage!\n");
                return CR_FAILURE;
            }
            if (building->getBuildStage() != building->getMaxBuildStage())
            {
                out.printerr("The selected building has not yet been fully constructed!\n");
                return CR_FAILURE;
            }
            dest_building = building->id;
            string name;
            building->getName(&name);
            out.print("Items created will be placed inside %s.\n", name.c_str());
            return CR_OK;
        }
        else
            return CR_WRONG_USAGE;
    }

    if ((parameters.size() < 2) || (parameters.size() > 3))
        return CR_WRONG_USAGE;

    item_str = parameters[0];
    material_str = parameters[1];

    if (parameters.size() == 3)
    {
        stringstream ss(parameters[2]);
        ss >> count;
        if (count < 1)
        {
            out.printerr("You cannot produce less than one item!\n");
            return CR_FAILURE;
        }
    }

    ItemTypeInfo item;
    MaterialInfo material;
    vector<string> tokens;

    if (!item.find(item_str))
    {
        out.printerr("Unrecognized item type!\n");
        return CR_FAILURE;
    }
    item_type = item.type;
    item_subtype = item.subtype;
    switch (item.type)
    {
    case df::item_type::INSTRUMENT:
    case df::item_type::TOY:
    case df::item_type::WEAPON:
    case df::item_type::ARMOR:
    case df::item_type::SHOES:
    case df::item_type::SHIELD:
    case df::item_type::HELM:
    case df::item_type::GLOVES:
    case df::item_type::AMMO:
    case df::item_type::PANTS:
    case df::item_type::SIEGEAMMO:
    case df::item_type::TRAPCOMP:
    case df::item_type::TOOL:
        if (item_subtype == -1)
        {
            out.printerr("You must specify a subtype!\n");
            return CR_FAILURE;
        }
    default:
        if (!material.find(material_str))
        {
            out.printerr("Unrecognized material!\n");
            return CR_FAILURE;
        }
        mat_type = material.type;
        mat_index = material.index;
        break;

    case df::item_type::REMAINS:
    case df::item_type::FISH:
    case df::item_type::FISH_RAW:
    case df::item_type::VERMIN:
    case df::item_type::PET:
    case df::item_type::EGG:
        split_string(&tokens, material_str, ":");
        if (tokens.size() != 2)
        {
            out.printerr("You must specify a creature ID and caste for this item type!\n");
            return CR_FAILURE;
        }

        for (size_t i = 0; i < world->raws.creatures.all.size(); i++)
        {
            df::creature_raw *creature = world->raws.creatures.all[i];
            if (creature->creature_id == tokens[0])
            {
                for (size_t j = 0; j < creature->caste.size(); j++)
                {
                    df::caste_raw *caste = creature->caste[j];
                    if (creature->caste[j]->caste_id == tokens[1])
                    {
                        mat_type = i;
                        mat_index = j;
                        break;
                    }
                }
                if (mat_type == -1)
                {
                    out.printerr("The creature you specified has no such caste!\n");
                    return CR_FAILURE;
                }
            } 
        }
        if (mat_type == -1)
        {
            out.printerr("Unrecognized creature ID!\n");
            return CR_FAILURE;
        }
        break;

    case df::item_type::CORPSE:
    case df::item_type::CORPSEPIECE:
    case df::item_type::FOOD:
        out.printerr("Cannot create that type of item!\n");
        return CR_FAILURE;
        break;
    }

    CoreSuspender suspend;

    df::unit *unit = Gui::getSelectedUnit(out, true);
    if (!unit)
    {
        if (*gametype == game_type::ADVENTURE_ARENA || *gametype == game_type::ADVENTURE_MAIN)
        {
            // Use the adventurer unit
            unit = world->units.active[0];
        }
        else
        {
            out.printerr("No unit selected!\n");
            return CR_FAILURE;
        }
    }
    if (!Maps::IsValid())
    {
        out.printerr("Map is not available.\n");
        return CR_FAILURE;
    }
    df::map_block *block = Maps::getTileBlock(unit->pos.x, unit->pos.y, unit->pos.z);
    if (block == NULL)
    {
        out.printerr("Unit does not reside in a valid map block, somehow?\n");
        return CR_FAILURE;
    }

    df::reaction_product_itemst *prod = df::allocate<df::reaction_product_itemst>();
    prod->item_type = item_type;
    prod->item_subtype = item_subtype;
    prod->mat_type = mat_type;
    prod->mat_index = mat_index;
    prod->probability = 100;
    prod->count = count;
    switch (item_type)
    {
    case df::item_type::BAR:
    case df::item_type::POWDER_MISC:
    case df::item_type::LIQUID_MISC:
    case df::item_type::DRINK:
        prod->product_dimension = 150;
        break;
    case df::item_type::THREAD:
        prod->product_dimension = 15000;
        break;
    case df::item_type::CLOTH:
        prod->product_dimension = 10000;
        break;
    default:
        prod->product_dimension = 1;
        break;
    }

    if ((dest_container != -1) && !df::item::find(dest_container))
    {
        dest_container = -1;
        out.printerr("Previously selected container no longer exists - item will be placed on the floor.\n");
    }
    if ((dest_building != -1) && !df::building::find(dest_building))
    {
        dest_building = -1;
        out.printerr("Previously selected building no longer exists - item will be placed on the floor.\n");
    }

    bool result = makeItem(prod, unit);
    delete prod;
    if (!result)
    {
        out.printerr("Failed to create item!\n");
        return CR_FAILURE;
    }
    return CR_OK;
}
