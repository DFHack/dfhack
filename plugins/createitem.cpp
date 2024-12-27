// Create arbitrary items

#include "Console.h"
#include "DataDefs.h"
#include "Export.h"
#include "MiscUtils.h"
#include "PluginManager.h"

#include "modules/Gui.h"
#include "modules/Items.h"
#include "modules/Maps.h"
#include "modules/Materials.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/building.h"
#include "df/caste_raw.h"
#include "df/creature_raw.h"
#include "df/game_type.h"
#include "df/item.h"
#include "df/plant_growth.h"
#include "df/plant_raw.h"
#include "df/tool_uses.h"
#include "df/unit.h"
#include "df/world.h"

using std::string;
using std::vector;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("createitem");
REQUIRE_GLOBAL(cursor);
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(gametype);
REQUIRE_GLOBAL(cur_year_tick);

int dest_container = -1, dest_building = -1;

command_result df_createitem(color_ostream &out, vector<string> &parameters);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector<PluginCommand> &commands) {
    commands.push_back(
        PluginCommand("createitem",
                      "Create arbitrary items.",
                      df_createitem));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    return CR_OK;
}

bool makeItem(df::unit *unit, df::item_type type, int16_t subtype, int16_t mat_type, int32_t mat_index,
    bool move_to_cursor = false, bool second_item = false)
{   // Special logic for making Gloves and Shoes in pairs
    bool is_gloves = (type == item_type::GLOVES);
    bool is_shoes = (type == item_type::SHOES);

    df::item *container = NULL;
    df::building *building = NULL;
    if (dest_container != -1)
        container = df::item::find(dest_container);
    if (dest_building != -1)
        building = df::building::find(dest_building);

    bool on_floor = (container == NULL) && (building == NULL) && !move_to_cursor;

    vector<df::item *> out_items;
    if (!Items::createItem(out_items, unit, type, subtype, mat_type, mat_index, !on_floor))
        return false;

    for (size_t i = 0; i < out_items.size(); i++) {
        if (container)
        {
            out_items[i]->flags.bits.removed = 1;
            if (!Items::moveToContainer(out_items[i], container))
                out_items[i]->moveToGround(container->pos.x, container->pos.y, container->pos.z);
        }
        else if (building)
        {
            out_items[i]->flags.bits.removed = 1;
            if (!Items::moveToBuilding(out_items[i], (df::building_actual *)building, df::building_item_role_type::TEMP))
                out_items[i]->moveToGround(building->centerx, building->centery, building->z);
        }
        else if (move_to_cursor)
            out_items[i]->moveToGround(cursor->x, cursor->y, cursor->z);
        // else createItem() already put it on the floor at the unit's feet, so we're good

        // Special logic for creating proper gloves in pairs
        if (is_gloves)
        {   // If the gloves have non-zero handedness, then disable special pair-making logic
            // ("reaction-gloves" tweak is active, or Toady fixed glove-making reactions)
            // Otherwise, set them to be either Left or Right-handed
            if (out_items[i]->getGloveHandedness() > 0)
                is_gloves = false;
            else
                out_items[i]->setGloveHandedness(second_item ? 2 : 1);
        }
    }
    // If we asked to make a Shoe and we got more than one, then disable special pair-making logic
    if (is_shoes && out_items.size() > 1)
        is_shoes = false;
    // If we asked for gloves/shoes and only got one (and we're making the first one), make another
    if ((is_gloves || is_shoes) && !second_item)
        return makeItem(unit, type, subtype, mat_type, mat_index, move_to_cursor, true);
    return true;
}

static inline bool select_caste_mat(color_ostream &out, vector<string> &tokens,
    int16_t &mat_type, int32_t &mat_index, const string &material_str)
{
    split_string(&tokens, material_str, ":");
    if (tokens.size() == 1)
    {   // Default to empty caste to display a list of valid castes later
        tokens.push_back("");
    }
    else if (tokens.size() != 2) {
        out.printerr("You must specify a creature ID and caste for this item type!\n");
        return CR_FAILURE;
    }

    for (size_t i = 0; i < world->raws.creatures.all.size(); i++)
    {
        string castes = "";
        auto creature = world->raws.creatures.all[i];
        if (creature->creature_id == tokens[0])
        {
            for (size_t j = 0; j < creature->caste.size(); j++)
            {
                auto caste = creature->caste[j];
                castes += " " + caste->caste_id;
                if (caste->caste_id == tokens[1])
                {
                    mat_type = i;
                    mat_index = j;
                    break;
                }
            }
            if (mat_type == -1) {
                if (tokens[1].empty())
                    out.printerr("You must also specify a caste.\n");
                else
                    out.printerr("The creature you specified has no such caste!\n");
                out.printerr("Valid castes:%s\n", castes.c_str());
                return false;
            }
        }
    }
    if (mat_type == -1) {
        out.printerr("Unrecognized creature ID!\n");
        return false;
    }
    return true;
}

static inline bool select_plant_growth(color_ostream &out, vector<string> &tokens, df::item_type &item_type,
    int16_t &item_subtype, int16_t &mat_type, int32_t &mat_index, const string &material_str)
{
    split_string(&tokens, material_str, ":");
    if (tokens.size() == 1)
    {   // Default to empty to display a list of valid growths later
        tokens.push_back("");
    }
    else if (tokens.size() == 3 && tokens[0] == "PLANT")
        tokens.erase(tokens.begin());
    else if (tokens.size() != 2) {
        out.printerr("You must specify a plant and growth ID for this item type!\n");
        return false;
    }

    for (size_t i = 0; i < world->raws.plants.all.size(); i++)
    {
        string growths = "";
        auto plant = world->raws.plants.all[i];
        if (plant->id != tokens[0])
            continue;

        for (size_t j = 0; j < plant->growths.size(); j++)
        {
            auto growth = plant->growths[j];
            growths += " " + growth->id;
            if (growth->id != tokens[1])
                continue;

            // Plant growths specify the actual item type/subtype
            // so that certain growths can drop as SEEDS items
            item_type = growth->item_type;
            item_subtype = growth->item_subtype;
            mat_type = growth->mat_type;
            mat_index = growth->mat_index;
            break;
        }
        if (mat_type == -1) {
            if (tokens[1].empty())
                out.printerr("You must also specify a growth ID.\n");
            else
                out.printerr("The plant you specified has no such growth!\n");
            out.printerr("Valid growths:%s\n", growths.c_str());
            return false;
        }
    }
    if (mat_type == -1) {
        out.printerr("Unrecognized plant ID!\n");
        return false;
    }
    return true;
}

command_result df_createitem (color_ostream &out, vector<string> &parameters) {
    string item_str, material_str;
    auto item_type = item_type::NONE;
    int16_t item_subtype = -1;
    int16_t mat_type = -1;
    int32_t mat_index = -1;
    int count = 1;
    bool move_to_cursor = false;

    if (parameters.size() == 1) {
        if (parameters[0] == "inspect") {
            auto item = Gui::getSelectedItem(out);
            if (!item)
                return CR_FAILURE;

            ItemTypeInfo iinfo(item->getType(), item->getSubtype());
            MaterialInfo minfo(item->getMaterial(), item->getMaterialIndex());
            out.print("%s %s\n", iinfo.getToken().c_str(), minfo.getToken().c_str());
            return CR_OK;
        }
        else if (parameters[0] == "floor") {
            dest_container = -1;
            dest_building = -1;
            out.print("Items created will be placed on the floor.\n");
            return CR_OK;
        }
        else if (parameters[0] == "item") {
            dest_building = -1;
            auto item = Gui::getSelectedItem(out);
            if (!item) {
                out.printerr("You must select a container!\n");
                return CR_FAILURE;
            }
            switch (item->getType())
            {   using namespace df::enums::item_type;
                case FLASK:
                case BARREL:
                case BUCKET:
                case ANIMALTRAP:
                case BOX:
                case BAG:
                case BIN:
                case BACKPACK:
                case QUIVER:
                    break;
                case TOOL:
                    if (item->hasToolUse(tool_uses::LIQUID_CONTAINER) ||
                        item->hasToolUse(tool_uses::FOOD_STORAGE) ||
                        item->hasToolUse(tool_uses::SMALL_OBJECT_STORAGE) ||
                        item->hasToolUse(tool_uses::TRACK_CART)
                    )
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
        else if (parameters[0] == "building") {
            dest_container = -1;
            auto building = Gui::getSelectedBuilding(out);
            if (!building) {
                out.printerr("You must select a building!\n");
                return CR_FAILURE;
            }
            switch (building->getType())
            {   using namespace df::enums::building_type;
                case Coffin:
                case Furnace:
                case TradeDepot:
                case Shop:
                case Box:
                case Weaponrack:
                case Armorstand:
                case Workshop:
                case Cabinet:
                case SiegeEngine:
                case Trap:
                case AnimalTrap:
                case Cage:
                case Wagon:
                case NestBox:
                case Hive:
                    break;
                default:
                    out.printerr("The selected building cannot be used for item storage!\n");
                    return CR_FAILURE;
            }
            if (building->getBuildStage() != building->getMaxBuildStage()) {
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

    if (parameters.size() == 3) {
        std::stringstream ss(parameters[2]);
        ss >> count;
        if (count < 1) {
            out.printerr("You cannot produce less than one item!\n");
            return CR_FAILURE;
        }
    }

    ItemTypeInfo item;
    MaterialInfo material;
    vector<string> tokens;

    if (item.find(item_str)) {
        item_type = item.type;
        item_subtype = item.subtype;
    }
    if (item_type == item_type::NONE) {
        out.printerr("You must specify a valid item type to create!\n");
        return CR_FAILURE;
    }
    switch (item.type)
    {   using namespace df::enums::item_type;
        case REMAINS:
        case FISH:
        case FISH_RAW:
        case VERMIN:
        case PET:
        case EGG:
            if (!select_caste_mat(out, tokens, mat_type, mat_index, material_str))
                return CR_FAILURE;
            break;
        case PLANT_GROWTH:
            if (!select_plant_growth(out, tokens, item_type, item_subtype,
                mat_type, mat_index, material_str)
                )
                return CR_FAILURE;
            break;
        case CORPSE:
        case CORPSEPIECE:
        case FOOD:
            out.printerr("Cannot create that type of item!\n");
            return CR_FAILURE;
        case INSTRUMENT:
        case TOY:
        case WEAPON:
        case ARMOR:
        case SHOES:
        case SHIELD:
        case HELM:
        case GLOVES:
        case AMMO:
        case PANTS:
        case SIEGEAMMO:
        case TRAPCOMP:
        case TOOL:
            if (item_subtype == -1) {
                out.printerr("You must specify a subtype!\n");
                return CR_FAILURE;
            }
        default:
            if (!material.find(material_str)) {
                out.printerr("Unrecognized material!\n");
                return CR_FAILURE;
            }
            mat_type = material.type;
            mat_index = material.index;
    }

    if (!Maps::IsValid()) {
        out.printerr("Map is not available.\n");
        return CR_FAILURE;
    }

    auto unit = Gui::getSelectedUnit(out, true);
    if (!unit) {
        if (*gametype == game_type::ADVENTURE_ARENA || World::isAdventureMode())
        {   // Use the adventurer unit
            unit = World::getAdventurer();
        }
        else if (cursor->x >= 0)
        {   // Use the first possible citizen if possible, otherwise the first unit
            for (auto u : Units::citizensRange(world->units.active)) {
                unit = u;
                break;
            }
            if (!unit)
                unit = vector_get(world->units.active, 0);
            move_to_cursor = true;
        }
        if (!unit) {
            out.printerr("No unit selected!\n");
            return CR_FAILURE;
        }
    }

    auto block = Maps::getTileBlock(unit->pos);
    if (!block) {
        out.printerr("Unit does not reside in a valid map block, somehow?\n");
        return CR_FAILURE;
    }
    if (dest_container != -1 && !df::item::find(dest_container))
    {
        dest_container = -1;
        out.printerr("Previously selected container no longer exists - item will be placed on the floor.\n");
    }
    if (dest_building != -1 && !df::building::find(dest_building))
    {
        dest_building = -1;
        out.printerr("Previously selected building no longer exists - item will be placed on the floor.\n");
    }

    for (int i = 0; i < count; i++) {
        if (!makeItem(unit, item_type, item_subtype, mat_type, mat_index, move_to_cursor, false))
        {
            out.printerr("Failed to create item!\n");
            return CR_FAILURE;
        }
    }
    return CR_OK;
}
