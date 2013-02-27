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

bool makeItem (df::reaction_product_itemst *prod, df::unit *unit, int hand = 0)
{
    vector<df::item *> out_items;
    vector<df::reaction_reagent *> in_reag;
    vector<df::item *> in_items;

    prod->produce(unit, &out_items, &in_reag, &in_items, 1, df::job_skill::NONE, ui->main.fortress_entity, df::world_site::find(ui->site_id));
    if (!out_items.size())
        return false;
    for (size_t i = 0; i < out_items.size(); i++)
    {
        out_items[i]->moveToGround(unit->pos.x, unit->pos.y, unit->pos.z);
        if (hand)
            out_items[i]->setGloveHandedness(hand);
    }
    return true;
}

command_result df_createitem (color_ostream &out, vector <string> & parameters)
{
    string item_str, material_str;
    df::item_type item_type;
    int16_t item_subtype;
    int16_t mat_type;
    int32_t mat_index;

    if (parameters.size() != 2)
    {
        out.print("Syntax: createitem ITEM_TYPE:ITEM_SUBTYPE MATERIAL:ETC\n");
        return CR_WRONG_USAGE;
    }
    item_str = parameters[0];
    material_str = parameters[1];

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
            out.printerr("You must specify a valid subtype!\n");
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

        mat_type = mat_index = -1;
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
    prod->count = 1;
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

    bool result;
#if 1
    // Workaround for DF issue #0006273
    if (item_type == df::item_type::GLOVES)
        result = makeItem(prod, unit, 1) && makeItem(prod, unit, 2);
    else
#endif
    result = makeItem(prod, unit);

    if (!result)
    {
        out.printerr("Failed to create item!\n");
        return CR_FAILURE;
    }
    return CR_OK;
}
