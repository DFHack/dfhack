// Automatically manage clothing work orders.

#include "Debug.h"
#include "PluginManager.h"

#include "modules/Items.h"
#include "modules/Materials.h"
#include "modules/Persistence.h"
#include "modules/Translation.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/creature_raw.h"
#include "df/item.h"
#include "df/item_actual.h"
#include "df/item_armorst.h"
#include "df/item_constructed.h"
#include "df/item_crafted.h"
#include "df/item_glovesst.h"
#include "df/item_helmst.h"
#include "df/item_pantsst.h"
#include "df/item_shoesst.h"
#include "df/itemdef_armorst.h"
#include "df/itemdef_glovesst.h"
#include "df/itemdef_helmst.h"
#include "df/itemdef_pantsst.h"
#include "df/itemdef_shoesst.h"
#include "df/manager_order.h"
#include "df/unit.h"
#include "df/world.h"

using std::endl;
using std::string;
using std::vector;
using std::map;

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("autoclothing");
REQUIRE_GLOBAL(world);

// Only run if this is enabled
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

// Logging levels can be dynamically controlled with the `debugfilter` command.
namespace DFHack {
    // For configuration-related logging
    DBG_DECLARE(autoclothing, control, DebugCategory::LINFO);
    // For logging during the periodic scan
    DBG_DECLARE(autoclothing, cycle, DebugCategory::LINFO);
}

static const int32_t CYCLE_TICKS = 1283; // one day-ish
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

static const string CONFIG_KEY = string(plugin_name) + "/config";
enum ConfigValues {
    CONFIG_IS_ENABLED = 0,
};

// Here go all the command declarations...
// mostly to allow having the mandatory stuff on top of the file and commands on the bottom
struct ClothingRequirement;
command_result autoclothing(color_ostream &out, vector<string> &parameters);
static void do_autoclothing();
static bool validateMaterialCategory(ClothingRequirement *requirement);
static bool setItem(string name, ClothingRequirement *requirement);
static void generate_control(color_ostream &out);
static bool isAvailableItem(df::item *item);

enum match_strictness {
    STRICT_PERMISSIVE,
    STRICT_TYPE,
    STRICT_MATERIAL
};

match_strictness strictnessSetting = STRICT_PERMISSIVE;

vector<ClothingRequirement> clothingOrders;

struct ClothingRequirement {
    df::job_type jobType = job_type::NONE;
    df::item_type itemType = item_type::NONE;
    int16_t item_subtype = -1;
    df::job_material_category material_category;
    int16_t needed_per_citizen = 0;
    map<int16_t, int32_t> total_needed_per_race;

    bool matches(ClothingRequirement *b) {
        return b->jobType == this->jobType
            && b->itemType == this->itemType
            && b->item_subtype == this->item_subtype
            && b->material_category.whole == this->material_category.whole;
    }

    string Serialize() {
        std::stringstream stream;
        stream << ENUM_KEY_STR(job_type, jobType) << " ";
        stream << ENUM_KEY_STR(item_type,itemType) << " ";
        stream << item_subtype << " ";
        stream << material_category.whole << " ";
        stream << needed_per_citizen;
        return stream.str();
    }

    void Deserialize(string s) {
        std::stringstream stream(s);
        string loadedJob;
        stream >> loadedJob;
        FOR_ENUM_ITEMS(job_type, job) {
            if (ENUM_KEY_STR(job_type, job) == loadedJob) {
                jobType = job;
                break;
            }
        }
        string loadedItem;
        stream >> loadedItem;
        FOR_ENUM_ITEMS(item_type, item) {
            if (ENUM_KEY_STR(item_type, item) == loadedItem) {
                itemType = item;
                break;
            }
        }
        stream >> item_subtype;
        stream >> material_category.whole;
        stream >> needed_per_citizen;
    }

    bool SetFromParameters(color_ostream &out, vector<string> &parameters)
    {
        if (!set_bitfield_field(&material_category, parameters[0], 1))
            out << "Unrecognized material type: " << parameters[0] << endl;
        if (!setItem(parameters[1], this)) {
            out << "Unrecognized item name or token: " << parameters[1] << endl;
            return false;
        }
        else if (!validateMaterialCategory(this)) {
            out << parameters[0] << " is not a valid material category for " << parameters[1] << endl;
            return false;
        }
        return true;
    }

    string ToReadableLabel() {
        std::stringstream stream;
        stream << bitfield_to_string(material_category) << " ";
        string adjective = "";
        string name = "";
        auto &itemdefs = world->raws.itemdefs;
        switch (itemType)
        {
            case item_type::ARMOR:
                adjective = itemdefs.armor[item_subtype]->adjective;
                name = itemdefs.armor[item_subtype]->name;
                break;
            case item_type::SHOES:
                adjective = itemdefs.shoes[item_subtype]->adjective;
                name = itemdefs.shoes[item_subtype]->name;
                break;
            case item_type::HELM:
                adjective = itemdefs.helms[item_subtype]->adjective;
                name = itemdefs.helms[item_subtype]->name;
                break;
            case item_type::GLOVES:
                adjective = itemdefs.gloves[item_subtype]->adjective;
                name = itemdefs.gloves[item_subtype]->name;
                break;
            case item_type::PANTS:
                adjective = itemdefs.pants[item_subtype]->adjective;
                name = itemdefs.pants[item_subtype]->name;
                break;
            default:
                break;
        }
        if (!adjective.empty())
            stream << adjective << " ";
        stream << name << " ";
        stream << needed_per_citizen;

        return stream.str();
    }
};


// Mandatory init function. If you have some global state, create it here.
DFhackCExport command_result plugin_init(color_ostream &out, vector<PluginCommand> &commands)
{   // Fill the command list with your commands.
    commands.push_back(PluginCommand(
        "autoclothing",
        "Automatically manage clothing work orders",
        autoclothing));
    return CR_OK;
}

// This is called right before the plugin library is removed from memory.
DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    return CR_OK;
}

// Called to notify the plugin about important state changes.
// Invoked with DF suspended, and always before the matching plugin_onupdate.
// More event codes may be added in the future.
DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    switch (event)
    {
        case SC_WORLD_UNLOADED:
            clothingOrders.clear();
            is_enabled = false;
            break;
        default:
            break;
    }
    return CR_OK;
}

DFhackCExport command_result plugin_load_site_data(color_ostream &out) {
    cycle_timestamp = 0;
    auto enabled = World::GetPersistentSiteData(CONFIG_KEY);
    if (!enabled.isValid()) {
        DEBUG(control, out).print("no config found in this save; initializing\n");
        enabled = World::AddPersistentSiteData(CONFIG_KEY);
        enabled.set_bool(CONFIG_IS_ENABLED, is_enabled);
    }

    is_enabled = enabled.get_bool(CONFIG_IS_ENABLED);
    DEBUG(control, out).print("loading persisted enabled state: %s\n",
        is_enabled ? "true" : "false");

    // Parse constraints
    vector<PersistentDataItem> items;
    World::GetPersistentSiteData(&items, "autoclothing/clothingItems");

    for (auto &item : items) {
        if (!item.isValid())
            continue;
        ClothingRequirement req;
        req.Deserialize(item.get_str());
        clothingOrders.push_back(req);
        out << "autoclothing added " << req.ToReadableLabel() << endl;
    }
    return CR_OK;
}

DFhackCExport command_result plugin_save_site_data(color_ostream &out) {
    for (auto &order : clothingOrders)
    {
        auto orderSave = World::AddPersistentSiteData("autoclothing/clothingItems");
        orderSave.set_str(order.Serialize());
    }

    // Parse constraints
    vector<PersistentDataItem> items;
    World::GetPersistentSiteData(&items, "autoclothing/clothingItems");

    for (size_t i = 0; i < items.size(); i++) {
        if (i < clothingOrders.size())
            items[i].set_str(clothingOrders[i].Serialize());
        else
            World::DeletePersistentData(items[i]);
    }
    for (size_t i = items.size(); i < clothingOrders.size(); i++)
    {
        auto item = World::AddPersistentSiteData("autoclothing/clothingItems");
        item.set_str(clothingOrders[i].Serialize());
    }
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot enable %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable != is_enabled) {
        auto enabled = World::GetPersistentSiteData(CONFIG_KEY);
        is_enabled = enable;
        DEBUG(control, out).print("%s from the API; persisting\n",
            is_enabled ? "enabled" : "disabled");
        enabled.set_bool(CONFIG_IS_ENABLED, is_enabled);
        if (enable)
            do_autoclothing();
    }
    else {
        DEBUG(control, out).print("%s from the API, but already %s; no action\n",
            is_enabled ? "enabled" : "disabled",
            is_enabled ? "enabled" : "disabled");
    }
    return CR_OK;
}

// Whatever you put here will be done in each game step. Don't abuse it.
// It's optional, so you can just comment it out like this if you don't need it.
DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (world->frame_counter - cycle_timestamp >= CYCLE_TICKS)
        do_autoclothing();
    return CR_OK;
}

static bool setItemFromName(string name, ClothingRequirement *requirement)
{
#define SEARCH_ITEM_RAWS(rawType, job, item) \
for (auto &itemdef : world->raws.itemdefs.rawType) { \
    string fullName = itemdef->adjective.empty() ? itemdef->name : itemdef->adjective + " " + itemdef->name; \
    if (fullName == name) { \
        requirement->jobType = job_type::job; \
        requirement->itemType = item_type::item; \
        requirement->item_subtype = itemdef->subtype; \
        return true; \
    } \
}
    SEARCH_ITEM_RAWS(armor, MakeArmor, ARMOR);
    SEARCH_ITEM_RAWS(gloves, MakeGloves, GLOVES);
    SEARCH_ITEM_RAWS(shoes, MakeShoes, SHOES);
    SEARCH_ITEM_RAWS(helms, MakeHelm, HELM);
    SEARCH_ITEM_RAWS(pants, MakePants, PANTS);
    return false;
}

static bool setItemFromToken(string token, ClothingRequirement *requirement) {
    ItemTypeInfo itemInfo;
    if (!itemInfo.find(token))
        return false;
    switch (itemInfo.type)
    {
        case item_type::ARMOR:
            requirement->jobType = job_type::MakeArmor;
            break;
        case item_type::GLOVES:
            requirement->jobType = job_type::MakeGloves;
            break;
        case item_type::SHOES:
            requirement->jobType = job_type::MakeShoes;
            break;
        case item_type::HELM:
            requirement->jobType = job_type::MakeHelm;
            break;
        case item_type::PANTS:
            requirement->jobType = job_type::MakePants;
            break;
        default:
            return false;
    }
    requirement->itemType = itemInfo.type;
    requirement->item_subtype = itemInfo.subtype;
    return true;
}

static bool setItem(string name, ClothingRequirement *requirement) {
    return setItemFromName(name, requirement) || setItemFromToken(name, requirement);
}

static bool armorFlagsMatch(BitArray<df::armor_general_flags> *flags, df::job_material_category *category) {
    if (flags->is_set(df::armor_general_flags::SOFT) &&
        (category->bits.cloth || category->bits.yarn || category->bits.silk)
    )
        return true;
    else if (flags->is_set(df::armor_general_flags::BARRED) && category->bits.bone)
        return true;
    else if (flags->is_set(df::armor_general_flags::SCALED) && category->bits.shell)
        return true;
    return flags->is_set(df::armor_general_flags::LEATHER) && category->bits.leather;
}

static bool validateMaterialCategory(ClothingRequirement *requirement) {
    auto itemDef = Items::getSubtypeDef(requirement->itemType, requirement->item_subtype);
    switch (requirement->itemType)
    {
        case item_type::ARMOR:
            if (STRICT_VIRTUAL_CAST_VAR(armor, df::itemdef_armorst, itemDef))
                return armorFlagsMatch(&armor->props.flags, &requirement->material_category);
            break;
        case item_type::GLOVES:
            if (STRICT_VIRTUAL_CAST_VAR(armor, df::itemdef_glovesst, itemDef))
                return armorFlagsMatch(&armor->props.flags, &requirement->material_category);
            break;
        case item_type::SHOES:
            if (STRICT_VIRTUAL_CAST_VAR(armor, df::itemdef_shoesst, itemDef))
                return armorFlagsMatch(&armor->props.flags, &requirement->material_category);
            break;
        case item_type::HELM:
            if (STRICT_VIRTUAL_CAST_VAR(armor, df::itemdef_helmst, itemDef))
                return armorFlagsMatch(&armor->props.flags, &requirement->material_category);
            break;
        case item_type::PANTS:
            if (STRICT_VIRTUAL_CAST_VAR(armor, df::itemdef_pantsst, itemDef))
                return armorFlagsMatch(&armor->props.flags, &requirement->material_category);
            break;
        default:
            break;
    }
    return false;
}

// A command! It sits around and looks pretty. And it's nice and friendly.
command_result autoclothing(color_ostream &out, vector<string> &parameters)
{   // Be sure to suspend the core if any DF state is read or modified
    CoreSuspender suspend;

    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot run %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    // It's nice to print a help message you get invalid options
    // from the user instead of just acting strange.
    // This can be achieved by adding the extended help string to the
    // PluginCommand registration as show above, and then returning
    // CR_WRONG_USAGE from the function. The same string will also
    // be used by 'help your-command'.
    if (parameters.empty())
    {
        out << "Automatic clothing management is currently " << (is_enabled ? "enabled" : "disabled") << "." << endl;
        out << "Currently set " << clothingOrders.size() << " automatic clothing orders" << endl;
        for (auto &order : clothingOrders)
            out << order.ToReadableLabel() << endl;
        generate_control(out);
        return CR_OK;
    }
    /* TODO: Disabled until I have time to fully implement it.
    else if (parameters[0] == "strictness") {
        if (parameters.size() != 2) {
            out << "Wrong number of arguments." << endl;
            return CR_WRONG_USAGE;
        }
        else if (parameters[1] == "permissive")
            strictnessSetting = STRICT_PERMISSIVE;
        else if (parameters[1] == "type")
            strictnessSetting = STRICT_TYPE;
        else if (parameters[1] == "material")
            strictnessSetting = STRICT_MATERIAL;
    }*/
    else if (parameters.size() < 2 || parameters.size() > 3) {
        out << "Wrong number of arguments." << endl;
        return CR_WRONG_USAGE;
    }

    // Create a new requirement from the available parameters.
    ClothingRequirement newRequirement;
    if (!newRequirement.SetFromParameters(out, parameters))
        return CR_WRONG_USAGE;
    // All checks are passed. Now we either show or set the amount.
    bool settingSize = false;
    bool matchedExisting = false;
    if (parameters.size() > 2) {
        try {
            newRequirement.needed_per_citizen = std::stoi(parameters[2]);
        }
        catch (const std::exception&) {
            out << parameters[2] << " is not a valid number." << endl;
            return CR_WRONG_USAGE;
        }
        settingSize = true;
    }

    for (size_t i = clothingOrders.size(); i-- > 0;) {
        if (!clothingOrders[i].matches(&newRequirement))
            continue;
        matchedExisting = true;
        if (settingSize) {
            if (newRequirement.needed_per_citizen == 0) {
                clothingOrders.erase(clothingOrders.begin() + i);
                out << "Unset " << parameters[0] << " " << parameters[1] << endl;
            }
            else {
                clothingOrders[i] = newRequirement;
                out << "Set " << parameters[0] << " " << parameters[1] << " to " << parameters[2] << endl;
            }
        }
        else
            out << parameters[0] << " " << parameters[1] << " is set to " << clothingOrders[i].needed_per_citizen << endl;
        break;
    }
    if (!matchedExisting) {
        if (settingSize) {
            if (newRequirement.needed_per_citizen == 0)
                out << parameters[0] << " " << parameters[1] << " already unset." << endl;
            else {
                clothingOrders.push_back(newRequirement);
                out << "Added order for " << parameters[0] << " " << parameters[1] << " to " << parameters[2] << endl;
            }
        }
        else
            out << parameters[0] << " " << parameters[1] << " is not set." << endl;
    }
    if (settingSize) {
        if (!is_enabled) {
            out << "Enabling automatic clothing management" << endl;
            is_enabled = true;
        }
        do_autoclothing();
    }
    // Give control back to DF.
    return CR_OK;
}

static void find_needed_clothing_items() {
    MaterialInfo matInfo;
    Units::forCitizens([&](auto unit) {
        // Now check each clothing order to see what the unit might be missing.
        for (auto &clothingOrder : clothingOrders)
        {
            int alreadyOwnedAmount = 0;
            // Looping through the items first, then clothing order might be a little faster, but this way is cleaner.
            for (auto ownedItem : unit->owned_items)
            {
                auto item = Items::findItemByID(ownedItem);
                if (!item) {
                    DEBUG(cycle).print("autoclothing: Invalid inventory item ID: %d\n", ownedItem);
                    continue;
                }

                if (item->getType() != clothingOrder.itemType ||
                    item->getSubtype() != clothingOrder.item_subtype ||
                    item->getWear() >= 1
                )
                    continue;

                matInfo.decode(item);
                if (!matInfo.matches(clothingOrder.material_category))
                    continue;

                alreadyOwnedAmount++;
            }
            int neededAmount = clothingOrder.needed_per_citizen - alreadyOwnedAmount;
            if (neededAmount <= 0)
                continue;
            // Technically, there's some leeway in sizes, but only caring about exact sizes is simpler.
            clothingOrder.total_needed_per_race[unit->race] += neededAmount;
        }
    });
}

static void remove_available_clothing() {
    MaterialInfo matInfo;
    for (auto &item : world->items.other.IN_PLAY)
    {   // Skip any non-worn owned items
        if (Items::getOwner(item) || item->getWear() >= 1)
            continue;
        // Again, for each item, find if any clothing order matches
        for (auto &clothingOrder : clothingOrders) {
            if (item->getType() != clothingOrder.itemType ||
                item->getSubtype() != clothingOrder.item_subtype
            )
                continue;

            matInfo.decode(item);
            if (!matInfo.matches(clothingOrder.material_category))
                continue;

            clothingOrder.total_needed_per_race[item->getMakerRace()] --;
        }
    }
}

static void add_clothing_orders() {
    for (auto &clothingOrder : clothingOrders) {
        for (auto &orderNeeded : clothingOrder.total_needed_per_race)
        {
            auto race = orderNeeded.first;
            auto amount = orderNeeded.second;
            orderNeeded.second = 0; // Once we get what we need, set it back to zero so we don't add it to further counts.
            // Previous operations can easily make this negative. That just means we have more than we need already.
            if (amount <= 0)
                continue;

            bool orderExistedAlready = false;
            for (auto &managerOrder : world->manager_orders.all)
            {   // Annoyingly, the manager orders store the job type for clothing orders, and actual item type is left at -1
                if (managerOrder->job_type != clothingOrder.jobType ||
                    managerOrder->item_subtype != clothingOrder.item_subtype ||
                    managerOrder->specdata.hist_figure_id != race
                )
                    continue;
                // We found a work order, that means we don't need to make a new one.
                orderExistedAlready = true;
                amount -= managerOrder->amount_left;
                if (amount > 0) {
                    managerOrder->amount_left += amount;
                    managerOrder->amount_total += amount;
                }
            }
            // If it wasn't there, we need to make a new one.
            if (!orderExistedAlready) {
                df::manager_order *newOrder = new df::manager_order();

                newOrder->id = world->manager_orders.manager_order_next_id;
                world->manager_orders.manager_order_next_id++;
                newOrder->job_type = clothingOrder.jobType;
                newOrder->item_subtype = clothingOrder.item_subtype;
                newOrder->specdata.hist_figure_id = race;
                newOrder->material_category = clothingOrder.material_category;
                newOrder->amount_left = amount;
                newOrder->amount_total = amount;
                world->manager_orders.all.push_back(newOrder);
            }
        }
    }
}

static void do_autoclothing()
{   // Mark that we have recently run
    cycle_timestamp = world->frame_counter;
    if (clothingOrders.empty())
        return;
    // First we look through all the units on the map to see who needs new clothes.
    find_needed_clothing_items();
    // Now we go through all the items in the map to see how many clothing items we have but aren't owned yet.
    remove_available_clothing();
    // Finally loop through the clothing orders to find ones that need more made.
    add_clothing_orders();
}

static void list_unit_counts(color_ostream &out, map<int, int> &unitList) {
    for (const auto &race : unitList) {
        if (race.second == 1)
            out << "    1 " << Units::getRaceReadableNameById(race.first) << endl;
        else
            out << "    " << race.second << " " << Units::getRaceNamePluralById(race.first) << endl;
    }
}

static constexpr uint32_t bad_flags = (
    df::item_flags::mask_in_job |
    df::item_flags::mask_hostile |
    df::item_flags::mask_removed |
    df::item_flags::mask_in_building |
    df::item_flags::mask_rotten |
    df::item_flags::mask_spider_web |
    df::item_flags::mask_construction |
    df::item_flags::mask_encased |
    df::item_flags::mask_foreign |
    df::item_flags::mask_trader |
    df::item_flags::mask_owned |
    df::item_flags::mask_garbage_collect |
    // df::item_flags::mask_artifact | // TODO: use this?
    df::item_flags::mask_forbid |
    df::item_flags::mask_dump |
    df::item_flags::mask_on_fire |
    df::item_flags::mask_melt |
    df::item_flags::mask_hidden
);

static bool isAvailableItem(df::item *item) {
    return (item->flags.whole & bad_flags) == 0
        && item->getWear() < 1
        && item->isClothing();
}

static void generate_control(color_ostream &out) {
    map<int, int> fullUnitList;
    map<int, int> missingArmor;
    map<int, int> missingShoes;
    map<int, int> missingHelms;
    map<int, int> missingGloves;
    map<int, int> missingPants;

    Units::forCitizens([&](auto unit) {
        fullUnitList[unit->race]++;
        int numArmor = 0, numShoes = 0, numHelms = 0, numGloves = 0, numPants = 0;
        for (auto itemId : unit->owned_items)
        {
            auto item = Items::findItemByID(itemId);
            if (!item) {
                DEBUG(cycle, out).print("autoclothing: Invalid inventory item ID: %d\n", itemId);
                continue;
            }
            else if (item->getWear() >= 1)
                continue;
            switch (item->getType())
            {
                case df::item_type::ARMOR:
                    numArmor++;
                    break;
                case df::item_type::SHOES:
                    numShoes++;
                    break;
                case df::item_type::HELM:
                    numHelms++;
                    break;
                case df::item_type::GLOVES:
                    numGloves++;
                    break;
                case df::item_type::PANTS:
                    numPants++;
                    break;
                default:
                    break;
            }
        }
        if (numArmor == 0)
            missingArmor[unit->race]++;
        if (numShoes < 2)
            missingShoes[unit->race]++;
        if (numHelms == 0)
            missingHelms[unit->race]++;
        if (numGloves < 2)
            missingGloves[unit->race]++;
        if (numPants == 0)
            missingPants[unit->race]++;
        DEBUG(control, out) << Translation::TranslateName(Units::getVisibleName(unit)) <<
            " has " << numArmor << " armor, " << numShoes << " shoes, " << numHelms <<
            " helms, " << numGloves << " gloves, " << numPants << " pants" << endl;
    });

    if (missingArmor.empty() && missingShoes.empty() && missingHelms.empty() &&
        missingGloves.empty() && missingPants.empty())
    {
        out << "Everybody has a full set of clothes to wear, congrats!" << endl;
        return;
    }
    else {
        if (!missingArmor.empty()) {
            out << "The following units need new bodywear:" << endl;
            list_unit_counts(out, missingArmor);
        }
        if (!missingShoes.empty()) {
            out << "The following units need new shoes:" << endl;
            list_unit_counts(out, missingShoes);
        }
        if (!missingHelms.empty()) {
            out << "The following units need new headwear:" << endl;
            list_unit_counts(out, missingHelms);
        }
        if (!missingGloves.empty()) {
            out << "The following units need new handwear:" << endl;
            list_unit_counts(out, missingGloves);
        }
        if (!missingPants.empty()) {
            out << "The following units need new legwear:" << endl;
            list_unit_counts(out, missingPants);
        }
    }
    map<int, int> availableArmor;
    for (auto armor : world->items.other.ARMOR) {
        if (!isAvailableItem(armor))
            continue;
        availableArmor[armor->maker_race]++;
    }
    if (!availableArmor.empty()) {
        out << "We have available bodywear for:" << endl;
        list_unit_counts(out, availableArmor);
    }

    map<int, int> availableShoes;
    for (auto shoe : world->items.other.SHOES) {
        if (!isAvailableItem(shoe))
            continue;
        availableShoes[shoe->maker_race]++;
    }
    if (!availableShoes.empty()) {
        out << "We have available footwear for:" << endl;
        list_unit_counts(out, availableShoes);
    }

    map<int, int> availableHelms;
    for (auto helm : world->items.other.HELM) {
        if (!isAvailableItem(helm))
            continue;
        availableHelms[helm->maker_race]++;
    }
    if (!availableHelms.empty()) {
        out << "We have available headwear for:" << endl;
        list_unit_counts(out, availableHelms);
    }

    map<int, int> availableGloves;
    for (auto glove : world->items.other.HELM) {
        if (!isAvailableItem(glove))
            continue;
        availableGloves[glove->maker_race]++;
    }
    if (!availableGloves.empty()) {
        out << "We have available handwear for:" << endl;
        list_unit_counts(out, availableGloves);
    }

    map<int, int> availablePants;
    for (auto pants : world->items.other.HELM) {
        if (!isAvailableItem(pants))
            continue;
        availablePants[pants->maker_race]++;
    }
    if (!availablePants.empty()) {
        out << "We have available legwear for:" << endl;
        list_unit_counts(out, availablePants);
    }
}

/////////////////////////////////////////////////////
// Lua API
// TODO: implement Lua hooks to manipulate the persistent order configuration
//

static void autoclothing_doCycle(color_ostream &out) {
    DEBUG(control, out).print("entering autoclothing_doCycle\n");
    do_autoclothing();
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(autoclothing_doCycle),
    DFHACK_LUA_END
};

DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_END
};
