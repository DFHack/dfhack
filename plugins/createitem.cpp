// Create arbitrary items

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "MiscUtils.h"

#include "modules/Maps.h"
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

using namespace std;
using namespace DFHack;

using df::global::world;
using df::global::ui;
using df::global::gametype;

DFHACK_PLUGIN("createitem");

command_result df_createitem (color_ostream &out, vector <string> & parameters);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector<PluginCommand> &commands)
{
    commands.push_back(PluginCommand("createitem", "Create arbitrary item at the selected unit's feet.", df_createitem));
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

    prod->produce(unit, &out_items, &in_reag, &in_items, 1, df::job_skill::NONE,
        df::historical_entity::find(unit->civ_id),
        ((*gametype == df::game_type::DWARF_MAIN) || (*gametype == df::game_type::DWARF_RECLAIM)) ? df::world_site::find(ui->site_id) : NULL);
    if (!out_items.size())
        return false;
    // if we asked to make shoes and we got twice as many as we asked, then we're okay
    // otherwise, make a second set because shoes are normally made in pairs
    if (is_shoes && out_items.size() == prod->count * 2)
        is_shoes = false;
    for (size_t i = 0; i < out_items.size(); i++)
    {
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

    if ((parameters.size() < 2) || (parameters.size() > 3))
    {
        out.print("Syntax: createitem <item> <material> [count]\n"
                  "    <item> - Item token for what you wish to create, as specified in custom\n"
                  "             reactions. If the item has no subtype, omit the :NONE.\n"
                  "    <material> - The material you want the item to be made of, as specified\n"
                  "                 in custom reactions. For REMAINS, FISH, FISH_RAW, VERMIN,\n"
                  "                 PET, and EGG, replace this with a creature ID and caste.\n"
                  "    [count] - How many of the item you wish to create.\n"
        );
        return CR_WRONG_USAGE;
    }
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
        out.printerr("No unit selected!\n");
        return CR_FAILURE;
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

    bool result = makeItem(prod, unit);
    delete prod;
    if (!result)
    {
        out.printerr("Failed to create item!\n");
        return CR_FAILURE;
    }
    return CR_OK;
}
