#include "Console.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/Filesystem.h"
#include "modules/Materials.h"
#include "modules/World.h"

#include "json/json.h"

#include "df/building.h"
#include "df/gamest.h"
#include "df/historical_figure.h"
#include "df/inorganic_raw.h"
#include "df/itemdef_ammost.h"
#include "df/itemdef_armorst.h"
#include "df/itemdef_foodst.h"
#include "df/itemdef_glovesst.h"
#include "df/itemdef_helmst.h"
#include "df/itemdef_instrumentst.h"
#include "df/itemdef_pantsst.h"
#include "df/itemdef_shieldst.h"
#include "df/itemdef_shoesst.h"
#include "df/itemdef_siegeammost.h"
#include "df/itemdef_toolst.h"
#include "df/itemdef_toyst.h"
#include "df/itemdef_trapcompst.h"
#include "df/itemdef_weaponst.h"
#include "df/job_item.h"
#include "df/manager_order.h"
#include "df/manager_order_condition_item.h"
#include "df/manager_order_condition_order.h"
#include "df/reaction.h"
#include "df/reaction_reagent.h"
#include "df/world.h"

using namespace DFHack;
using namespace df::enums;

using df::global::game;

DFHACK_PLUGIN("orders");

REQUIRE_GLOBAL(world);

static const std::string ORDERS_DIR = "dfhack-config/orders";
static const std::string ORDERS_LIBRARY_DIR = "hack/data/orders";

static command_result orders_command(color_ostream & out, std::vector<std::string> & parameters);

DFhackCExport command_result plugin_init(color_ostream & out, std::vector<PluginCommand> & commands)
{
    commands.push_back(PluginCommand(
        "orders",
        "Manage manager orders.",
        orders_command));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream & out)
{
    return CR_OK;
}

static command_result orders_list_command(color_ostream & out);
static command_result orders_export_command(color_ostream & out, const std::string & name);
static command_result orders_import_command(color_ostream & out, const std::string & name);
static command_result orders_clear_command(color_ostream & out);
static command_result orders_sort_command(color_ostream & out);
static command_result orders_recheck_command(color_ostream & out);
static command_result orders_recheck_current_command(color_ostream & out);

static command_result orders_command(color_ostream & out, std::vector<std::string> & parameters)
{
    class color_ostream_resetter
    {
        color_ostream & out;

    public:
        color_ostream_resetter(color_ostream & out) : out(out) {}
        ~color_ostream_resetter() { out.reset_color(); }
    } resetter(out);

    if (parameters.empty())
    {
        return CR_WRONG_USAGE;
    }

    if (parameters[0] == "list")
    {
        return orders_list_command(out);
    }

    if (!Core::getInstance().isMapLoaded() || !World::isFortressMode()) {
        out.printerr("Cannot run %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    if (parameters[0] == "export" && parameters.size() == 2)
    {
        return orders_export_command(out, parameters[1]);
    }

    if (parameters[0] == "import" && parameters.size() == 2)
    {
        return orders_import_command(out, parameters[1]);
    }

    if (parameters[0] == "clear" && parameters.size() == 1)
    {
        return orders_clear_command(out);
    }

    if (parameters[0] == "sort" && parameters.size() == 1)
    {
        return orders_sort_command(out);
    }

    if (parameters[0] == "recheck" && parameters.size() == 1)
    {
        return orders_recheck_command(out);
    }

    if (parameters[0] == "recheck" && parameters.size() == 2)
    {
        if (parameters[1] == "this")
        {
            return orders_recheck_current_command(out);
        }
    }

    return CR_WRONG_USAGE;
}

static void list_library(color_ostream &out) {
    std::map<std::string, bool> files;
    if (0 < Filesystem::listdir_recursive(ORDERS_LIBRARY_DIR, files, 0, false)) {
        // if the library directory doesn't exist, just skip it
        return;
    }

    if (files.empty()) {
        // if no files in the library directory, just skip it
        return;
    }

    for (auto it : files)
    {
        if (it.second)
            continue; // skip directories
        std::string name = it.first;
        if (name.length() <= 5 || name.rfind(".json") != name.length() - 5)
            continue; // skip non-.json files
        name.resize(name.length() - 5);
        out << "library/" << name << std::endl;
    }
}

static command_result orders_list_command(color_ostream & out)
{
    // use listdir_recursive instead of listdir even though orders doesn't
    // support subdirs so we can identify and ignore subdirs with ".json" names.
    // also listdir_recursive will alphabetize the list for us.
    std::map<std::string, bool> files;
    Filesystem::listdir_recursive(ORDERS_DIR, files, 0, false);

    for (auto it : files) {
        if (it.second)
            continue; // skip directories
        std::string name = it.first;
        if (name.length() <= 5 || name.rfind(".json") != name.length() - 5)
            continue; // skip non-.json files
        name.resize(name.length() - 5);
        out << name << std::endl;
    }

    list_library(out);

    return CR_OK;
}

static bool is_safe_filename(color_ostream & out, const std::string & name)
{
    if (name.empty())
    {
        out << COLOR_LIGHTRED << "Missing filename!" << std::endl;
        return false;
    }

    for (auto it : name)
    {
        if (isalnum(it))
        {
            continue;
        }

        if (it != ' ' && it != '.' && it != '-' && it != '_')
        {
            out << COLOR_LIGHTRED << "Invalid symbol in name: " << it << std::endl;
            return false;
        }
    }

    return true;
}

template<typename B>
static void bitfield_to_json_array(Json::Value & out, B bits)
{
    std::vector<std::string> names;
    bitfield_to_string(&names, bits);

    for (auto & it : names)
    {
        out.append(it);
    }
}

template<typename B>
static void json_array_to_bitfield(B & bits, Json::Value & arr)
{
    if (arr.size() == 0)
    {
        return;
    }

    for (Json::ArrayIndex i = arr.size(); i != 0; i--)
    {
        if (!arr[i - 1].isString())
        {
            continue;
        }

        std::string str(arr[i - 1].asString());

        int current;
        if (get_bitfield_field(&current, bits, str))
        {
            if (!current && set_bitfield_field(&bits, str, 1))
            {
                Json::Value removed;
                arr.removeIndex(i - 1, &removed);
            }
        }
    }
}

template<typename D>
static df::itemdef *get_itemdef(int16_t subtype)
{
    return D::find(subtype);
}

template<typename D>
static df::itemdef *get_itemdef(const std::string & subtype)
{
    for (auto it : D::get_vector())
    {
        if (it->id == subtype)
        {
            return it;
        }
    }
    return nullptr;
}

template<typename ST>
static df::itemdef *get_itemdef(color_ostream & out, df::item_type type, ST subtype)
{
    switch (type)
    {
    case item_type::AMMO:
        return get_itemdef<df::itemdef_ammost>(subtype);
    case item_type::ARMOR:
        return get_itemdef<df::itemdef_armorst>(subtype);
    case item_type::FOOD:
        return get_itemdef<df::itemdef_foodst>(subtype);
    case item_type::GLOVES:
        return get_itemdef<df::itemdef_glovesst>(subtype);
    case item_type::HELM:
        return get_itemdef<df::itemdef_helmst>(subtype);
    case item_type::INSTRUMENT:
        return get_itemdef<df::itemdef_instrumentst>(subtype);
    case item_type::PANTS:
        return get_itemdef<df::itemdef_pantsst>(subtype);
    case item_type::SHIELD:
        return get_itemdef<df::itemdef_shieldst>(subtype);
    case item_type::SHOES:
        return get_itemdef<df::itemdef_shoesst>(subtype);
    case item_type::SIEGEAMMO:
        return get_itemdef<df::itemdef_siegeammost>(subtype);
    case item_type::TOOL:
        return get_itemdef<df::itemdef_toolst>(subtype);
    case item_type::TOY:
        return get_itemdef<df::itemdef_toyst>(subtype);
    case item_type::TRAPCOMP:
        return get_itemdef<df::itemdef_trapcompst>(subtype);
    case item_type::WEAPON:
        return get_itemdef<df::itemdef_weaponst>(subtype);
    default:
        out << COLOR_YELLOW << "Unhandled raw item type in manager order: " << enum_item_key(type) << "! Please report this bug to DFHack." << std::endl;
        return nullptr;
    }
}

static command_result orders_export_command(color_ostream & out, const std::string & name)
{
    if (!is_safe_filename(out, name))
    {
        return CR_WRONG_USAGE;
    }

    Json::Value orders(Json::arrayValue);

    for (auto it : world->manager_orders.all)
    {
        Json::Value order(Json::objectValue);

        order["id"] = it->id;
        order["job"] = enum_item_key(it->job_type);
        if (!it->reaction_name.empty())
        {
            order["reaction"] = it->reaction_name;
        }

        if (it->item_type != item_type::NONE)
        {
            order["item_type"] = enum_item_key(it->item_type);
        }
        if (it->item_subtype != -1)
        {
            df::itemdef *def = get_itemdef(out, it->item_type == item_type::NONE ? ENUM_ATTR(job_type, item, it->job_type) : it->item_type, it->item_subtype);

            if (def)
            {
                order["item_subtype"] = def->id;
            }
        }

        if (it->job_type == job_type::PrepareMeal)
        {
            order["meal_ingredients"] = it->mat_type;
        }
        else if (it->mat_type != -1 || it->mat_index != -1)
        {
            order["material"] = MaterialInfo(it).getToken();
        }

        if (it->specflag.encrust_flags.whole != 0)
        {
            bitfield_to_json_array(order["item_category"], it->specflag.encrust_flags);
        }

        if (it->specdata.hist_figure_id != -1)
        {
            order["hist_figure"] = it->specdata.hist_figure_id;
        }

        if (it->material_category.whole != 0)
        {
            bitfield_to_json_array(order["material_category"], it->material_category);
        }

        if (it->art_spec.type != df::job_art_specifier_type::None)
        {
            Json::Value art(Json::objectValue);

            art["type"] = enum_item_key(it->art_spec.type);
            art["id"] = it->art_spec.id;
            if (it->art_spec.subid != -1)
            {
                art["subid"] = it->art_spec.subid;
            }

            order["art"] = art;
        }

        order["amount_left"] = it->amount_left;
        order["amount_total"] = it->amount_total;
        order["is_validated"] = bool(it->status.bits.validated);
        order["is_active"] = bool(it->status.bits.active);

        order["frequency"] = enum_item_key(it->frequency);

        // TODO: finished_year, finished_year_tick

        if (it->workshop_id != -1)
        {
            order["workshop_id"] = it->workshop_id;
        }

        if (it->max_workshops != 0)
        {
            order["max_workshops"] = it->max_workshops;
        }

        if (!it->item_conditions.empty())
        {
            Json::Value conditions(Json::arrayValue);

            for (auto it2 : it->item_conditions)
            {
                Json::Value condition(Json::objectValue);

                condition["condition"] = enum_item_key(it2->compare_type);
                condition["value"] = it2->compare_val;

                if (it2->flags1.whole != 0 || it2->flags2.whole != 0 || it2->flags3.whole != 0)
                {
                    bitfield_to_json_array(condition["flags"], it2->flags1);
                    bitfield_to_json_array(condition["flags"], it2->flags2);
                    bitfield_to_json_array(condition["flags"], it2->flags3);
                    // TODO: flags4, flags5
                }

                if (it2->item_type != item_type::NONE)
                {
                    condition["item_type"] = enum_item_key(it2->item_type);
                }
                if (it2->item_subtype != -1)
                {
                    df::itemdef *def = get_itemdef(out, it2->item_type, it2->item_subtype);

                    if (def)
                    {
                        condition["item_subtype"] = def->id;
                    }
                }

                if (it2->mat_type != -1 || it2->mat_index != -1)
                {
                    condition["material"] = MaterialInfo(it2).getToken();
                }

                if (it2->metal_ore != -1)
                {
                    condition["bearing"] = df::inorganic_raw::find(it2->metal_ore)->id;
                }

                if (!it2->reaction_class.empty())
                {
                    condition["reaction_class"] = it2->reaction_class;
                }

                if (!it2->has_material_reaction_product.empty())
                {
                    condition["reaction_product"] = it2->has_material_reaction_product;
                }

                if (it2->has_tool_use != tool_uses::NONE)
                {
                    condition["tool"] = enum_item_key(it2->has_tool_use);
                }

                if (it2->min_dimension != -1)
                {
                    condition["min_dimension"] = it2->min_dimension;
                }

                if (it2->reaction_id != -1)
                {
                    df::reaction *reaction = world->raws.reactions.reactions[it2->reaction_id];
                    condition["reaction_id"] = reaction->code;

                    if (!it2->contains.empty())
                    {
                        Json::Value contains(Json::arrayValue);
                        for (int32_t contains_val : it2->contains)
                        {
                            contains.append(reaction->reagents[contains_val]->code);
                        }
                        condition["contains"] = contains;
                    }
                }

                conditions.append(condition);
            }

            order["item_conditions"] = conditions;
        }

        if (!it->order_conditions.empty())
        {
            Json::Value conditions(Json::arrayValue);

            for (auto it2 : it->order_conditions)
            {
                Json::Value condition(Json::objectValue);

                condition["order"] = it2->order_id;
                condition["condition"] = enum_item_key(it2->condition);

                // TODO: unk_1

                conditions.append(condition);
            }

            order["order_conditions"] = conditions;
        }

        // TODO: items

        orders.append(order);
    }

    Filesystem::mkdir(ORDERS_DIR);

    std::ofstream file(ORDERS_DIR + "/" + name + ".json");

    file << orders << std::endl;

    return file.good() ? CR_OK : CR_FAILURE;
}

static command_result orders_import(color_ostream &out, Json::Value &orders)
{
    std::map<int32_t, int32_t> id_mapping;
    for (auto it : orders)
    {
        id_mapping[it["id"].asInt()] = world->manager_orders.manager_order_next_id;
        world->manager_orders.manager_order_next_id++;
    }

    for (auto & it : orders)
    {
        df::manager_order *order = new df::manager_order();

        order->id = id_mapping.at(it["id"].asInt());

        if (!find_enum_item(&order->job_type, it["job"].asString()))
        {
            delete order;

            out << COLOR_LIGHTRED << "Invalid job type for imported manager order: " << it["job"].asString() << std::endl;

            return CR_FAILURE;
        }

        if (it.isMember("reaction"))
        {
            order->reaction_name = it["reaction"].asString();
        }

        if (it.isMember("item_type"))
        {
            if (!find_enum_item(&order->item_type, it["item_type"].asString()) || order->item_type == item_type::NONE)
            {
                delete order;

                out << COLOR_LIGHTRED << "Invalid item type for imported manager order: " << it["item_type"].asString() << std::endl;

                return CR_FAILURE;
            }
        }
        if (it.isMember("item_subtype"))
        {
            df::itemdef *def = get_itemdef(out, order->item_type == item_type::NONE ? ENUM_ATTR(job_type, item, order->job_type) : order->item_type, it["item_subtype"].asString());

            if (def)
            {
                order->item_subtype = def->subtype;
            }
            else
            {
                out << COLOR_LIGHTRED << "Invalid item subtype for imported manager order: " << enum_item_key(order->item_type) << ":" << it["item_subtype"].asString() << std::endl;

                delete order;

                return CR_FAILURE;
            }
        }

        if (it.isMember("meal_ingredients"))
        {
            order->mat_type = it["meal_ingredients"].asInt();
            order->mat_index = -1;
        }
        else if (it.isMember("material"))
        {
            MaterialInfo mat;
            if (!mat.find(it["material"].asString()))
            {
                delete order;

                out << COLOR_LIGHTRED << "Invalid material for imported manager order: " << it["material"].asString() << std::endl;

                return CR_FAILURE;
            }
            order->mat_type = mat.type;
            order->mat_index = mat.index;
        }

        if (it.isMember("item_category"))
        {
            json_array_to_bitfield(order->specflag.encrust_flags, it["item_category"]);
            if (!it["item_category"].empty())
            {
                delete order;

                out << COLOR_LIGHTRED << "Invalid item_category value for imported manager order: " << it["item_category"] << std::endl;

                return CR_FAILURE;
            }
        }

        if (it.isMember("hist_figure"))
        {
            if (!df::historical_figure::find(it["hist_figure"].asInt()))
            {
                delete order;

                out << COLOR_YELLOW << "Missing historical figure for imported manager order: " << it["hist_figure"].asInt() << std::endl;

                continue;
            }

            order->specdata.hist_figure_id = it["hist_figure"].asInt();
        }

        if (it.isMember("material_category"))
        {
            json_array_to_bitfield(order->material_category, it["material_category"]);
            if (!it["material_category"].empty())
            {
                delete order;

                out << COLOR_LIGHTRED << "Invalid material_category value for imported manager order: " << it["material_category"] << std::endl;

                return CR_FAILURE;
            }
        }

        if (it.isMember("art"))
        {
            if (!find_enum_item(&order->art_spec.type, it["art"]["type"].asString()))
            {
                delete order;

                out << COLOR_LIGHTRED << "Invalid art type value for imported manager order: " << it["art"]["type"].asString() << std::endl;

                return CR_FAILURE;
            }

            order->art_spec.id = it["art"]["id"].asInt();
            if (it["art"].isMember("subid"))
            {
                order->art_spec.subid = it["art"]["subid"].asInt();
            }
        }

        order->amount_left = it["amount_left"].asInt();
        order->amount_total = it["amount_total"].asInt();
        order->status.bits.validated = it["is_validated"].asBool();
        order->status.bits.active = it["is_active"].asBool();

        if (!find_enum_item(&order->frequency, it["frequency"].asString()))
        {
            delete order;

            out << COLOR_LIGHTRED << "Invalid frequency value for imported manager order: " << it["frequency"].asString() << std::endl;

            return CR_FAILURE;
        }

        // TODO: finished_year, finished_year_tick

        if (it.isMember("workshop_id"))
        {
            if (!df::building::find(it["workshop_id"].asInt()))
            {
                delete order;

                out << COLOR_YELLOW << "Missing workshop for imported manager order: " << it["workshop_id"].asInt() << std::endl;

                continue;
            }

            order->workshop_id = it["workshop_id"].asInt();
        }

        if (it.isMember("max_workshops"))
        {
            order->max_workshops = it["max_workshops"].asInt();
        }

        if (it.isMember("item_conditions"))
        {
            for (auto & it2 : it["item_conditions"])
            {
                df::manager_order_condition_item *condition = new df::manager_order_condition_item();

                if (!find_enum_item(&condition->compare_type, it2["condition"].asString()))
                {
                    delete condition;

                    out << COLOR_YELLOW << "Invalid item condition condition for imported manager order: " << it2["condition"].asString() << std::endl;

                    continue;
                }

                condition->compare_val = it2["value"].asInt();

                if (it2.isMember("flags"))
                {
                    json_array_to_bitfield(condition->flags1, it2["flags"]);
                    json_array_to_bitfield(condition->flags2, it2["flags"]);
                    json_array_to_bitfield(condition->flags3, it2["flags"]);
                    // TODO: flags4, flags5

                    if (!it2["flags"].empty())
                    {
                        delete condition;

                        out << COLOR_YELLOW << "Invalid item condition flags for imported manager order: " << it2["flags"] << std::endl;

                        continue;
                    }
                }

                if (it2.isMember("item_type"))
                {
                    if (!find_enum_item(&condition->item_type, it2["item_type"].asString()) || condition->item_type == item_type::NONE)
                    {
                        delete condition;

                        out << COLOR_YELLOW << "Invalid item condition item type for imported manager order: " << it2["item_type"].asString() << std::endl;

                        continue;
                    }
                }
                if (it2.isMember("item_subtype"))
                {
                    df::itemdef *def = get_itemdef(out, condition->item_type, it2["item_subtype"].asString());

                    if (def)
                    {
                        condition->item_subtype = def->subtype;
                    }
                    else
                    {
                        out << COLOR_YELLOW << "Invalid item condition item subtype for imported manager order: " << enum_item_key(condition->item_type) << ":" << it2["item_subtype"].asString() << std::endl;

                        delete condition;

                        continue;
                    }
                }

                if (it2.isMember("material"))
                {
                    MaterialInfo mat;
                    if (!mat.find(it2["material"].asString()))
                    {
                        delete condition;

                        out << COLOR_YELLOW << "Invalid item condition material for imported manager order: " << it2["material"].asString() << std::endl;

                        continue;
                    }
                    condition->mat_type = mat.type;
                    condition->mat_index = mat.index;
                }

                if (it2.isMember("bearing"))
                {
                    std::string bearing(it2["bearing"].asString());
                    auto found = std::find_if(world->raws.inorganics.all.begin(), world->raws.inorganics.all.end(), [bearing](df::inorganic_raw *raw) -> bool { return raw->id == bearing; });
                    if (found == world->raws.inorganics.all.end())
                    {
                        delete condition;

                        out << COLOR_YELLOW << "Invalid item condition inorganic bearing type for imported manager order: " << it2["bearing"].asString() << std::endl;

                        continue;
                    }
                    condition->metal_ore = found - world->raws.inorganics.all.begin();
                }

                if (it2.isMember("reaction_class"))
                {
                    condition->reaction_class = it2["reaction_class"].asString();
                }

                if (it2.isMember("reaction_product"))
                {
                    condition->has_material_reaction_product = it2["reaction_product"].asString();
                }

                if (it2.isMember("tool"))
                {
                    if (!find_enum_item(&condition->has_tool_use, it2["tool"].asString()) || condition->has_tool_use == tool_uses::NONE)
                    {
                        delete condition;

                        out << COLOR_YELLOW << "Invalid item condition tool use for imported manager order: " << it2["tool"].asString() << std::endl;

                        continue;
                    }
                }

                if (it2.isMember("min_dimension"))
                {
                    condition->min_dimension = it2["min_dimension"].asInt();
                }

                if (it2.isMember("reaction_id"))
                {
                    std::string reaction_code = it2["reaction_id"].asString();
                    df::reaction *reaction = NULL;
                    int32_t reaction_id = -1;
                    size_t num_reactions = world->raws.reactions.reactions.size();
                    for (size_t idx = 0; idx < num_reactions; ++idx)
                    {
                        reaction = world->raws.reactions.reactions[idx];
                        if (reaction->code == reaction_code)
                        {
                            reaction_id = idx;
                            break;
                        }
                    }
                    if (reaction_id < 0)
                    {
                        delete condition;

                        out << COLOR_YELLOW << "Reaction code not found for imported manager order: " << reaction_code << std::endl;

                        continue;
                    }

                    condition->reaction_id = reaction_id;

                    if (it2.isMember("contains"))
                    {
                        size_t num_reagents = reaction->reagents.size();
                        std::string bad_reagent_code;
                        for (Json::Value & contains_val : it2["contains"])
                        {
                            std::string reagent_code = contains_val.asString();
                            bool reagent_found = false;
                            for (size_t idx = 0; idx < num_reagents; ++idx)
                            {
                                df::reaction_reagent *reagent = reaction->reagents[idx];
                                if (reagent->code == reagent_code)
                                {
                                    condition->contains.push_back(idx);
                                    reagent_found = true;
                                    break;
                                }
                            }

                            if (!reagent_found)
                            {
                                bad_reagent_code = reagent_code;
                                break;
                            }
                        }
                        if (!bad_reagent_code.empty())
                        {
                            delete condition;

                            out << COLOR_YELLOW << "Invalid reagent code for imported manager order: " << bad_reagent_code << std::endl;

                            continue;
                        }
                    }
                }

                order->item_conditions.push_back(condition);
            }
        }

        if (it.isMember("order_conditions"))
        {
            for (auto & it2 : it["order_conditions"])
            {
                df::manager_order_condition_order *condition = new df::manager_order_condition_order();

                int32_t id = it2["order"].asInt();
                if (id == it["id"].asInt() || !id_mapping.count(id))
                {
                    delete condition;

                    out << COLOR_YELLOW << "Missing order condition target for imported manager order: " << it2["order"].asInt() << std::endl;

                    continue;
                }
                condition->order_id = id_mapping.at(id);

                if (!find_enum_item(&condition->condition, it2["condition"].asString()))
                {
                    delete condition;

                    out << COLOR_YELLOW << "Invalid order condition type for imported manager order: " << it2["condition"].asString() << std::endl;

                    continue;
                }

                // TODO: unk_1

                order->order_conditions.push_back(condition);
            }
        }

        // TODO: items

        world->manager_orders.all.push_back(order);
    }

    return CR_OK;
}

static command_result orders_import_command(color_ostream & out, const std::string & name)
{
    std::string fname = name;
    bool is_library = false;
    if (0 == name.find("library/")) {
        is_library = true;
        fname = name.substr(8);
    }

    if (!is_safe_filename(out, fname))
    {
        return CR_WRONG_USAGE;
    }

    const std::string filename((is_library ? ORDERS_LIBRARY_DIR : ORDERS_DIR) +
                                    "/" + fname + ".json");
    Json::Value orders;

    {
        std::ifstream file(filename);

        if (!file.good())
        {
            out << COLOR_LIGHTRED << "Cannot find orders file: " << filename << std::endl;
            return CR_FAILURE;
        }

        try
        {
            file >> orders;
        }
        catch (const std::exception & e)
        {
            out << COLOR_LIGHTRED << "Error reading orders file: " << filename << ": " << e.what() << std::endl;
            return CR_FAILURE;
        }

        if (!file.good())
        {
            out << COLOR_LIGHTRED << "Error reading orders file: " << filename << std::endl;
            return CR_FAILURE;
        }
    }

    if (orders.type() != Json::arrayValue)
    {
        out << COLOR_LIGHTRED << "Invalid orders file: " << filename << ": expected array" << std::endl;
        return CR_FAILURE;
    }

    try
    {
        return orders_import(out, orders);
    }
    catch (const std::exception & e)
    {
        out << COLOR_LIGHTRED << "Error reading orders file: " << filename << ": " << e.what() << std::endl;
        return CR_FAILURE;
    }
}

static command_result orders_clear_command(color_ostream & out)
{
    for (auto order : world->manager_orders.all)
    {
        for (auto condition : order->item_conditions)
        {
            delete condition;
        }
        for (auto condition : order->order_conditions)
        {
            delete condition;
        }
        if (order->items)
        {
            for (auto item : order->items->elements)
            {
                delete item;
            }
            delete order->items;
        }

        delete order;
    }

    out << "Deleted " << world->manager_orders.all.size() << " manager orders." << std::endl;

    world->manager_orders.all.clear();

    return CR_OK;
}

static bool orders_compare(df::manager_order *a, df::manager_order *b)
{
    if (a->workshop_id != b->workshop_id) {
        return a->workshop_id >= 0;
    }

    if (a->frequency == df::workquota_frequency_type::OneTime
            || b->frequency == df::workquota_frequency_type::OneTime)
        return a->frequency < b->frequency;
    return a->frequency > b->frequency;
}

static command_result orders_sort_command(color_ostream & out)
{
    if (!std::is_sorted(world->manager_orders.all.begin(),
                        world->manager_orders.all.end(),
                        orders_compare))
    {
        std::stable_sort(world->manager_orders.all.begin(),
                         world->manager_orders.all.end(),
                         orders_compare);
        out << "Fixed priority of manager orders." << std::endl;
    }

    return CR_OK;
}

static command_result orders_recheck_command(color_ostream & out)
{
    size_t count = 0;
    for (auto it : world->manager_orders.all) {
        if (it->item_conditions.size() && it->status.bits.active) {
            ++count;
            it->status.bits.active = false;
            it->status.bits.validated = false;
        }
    }
    out << "Re-checking conditions for " << count << " manager orders." << std::endl;
    return CR_OK;
}

static command_result orders_recheck_current_command(color_ostream & out)
{
    if (game->main_interface.info.work_orders.conditions.open)
    {
        game->main_interface.info.work_orders.conditions.wq->status.bits.active = false;
    }
    else
    {
        out << COLOR_LIGHTRED << "Order conditions is not open" << std::endl;
        return CR_FAILURE;
    }
    return CR_OK;
}
