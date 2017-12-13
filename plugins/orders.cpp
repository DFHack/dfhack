#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/Filesystem.h"
#include "modules/Materials.h"

#include "jsoncpp.h"

#include "df/building.h"
#include "df/historical_figure.h"
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
#include "df/world.h"

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("orders");

REQUIRE_GLOBAL(world);

static command_result orders_command(color_ostream & out, std::vector<std::string> & parameters);

DFhackCExport command_result plugin_init(color_ostream & out, std::vector<PluginCommand> & commands)
{
    commands.push_back(PluginCommand(
        "orders",
        "Manipulate manager orders.",
        orders_command,
        false,
        "orders - Manipulate manager orders\n"
        "  orders export [name]\n"
        "    Exports the current list of manager orders to a file named dfhack-config/orders/[name].json.\n"
        "  orders import [name]\n"
        "    Imports manager orders from a file named dfhack-config/orders/[name].json.\n"
        "  orders clear\n"
        "    Deletes all manager orders in the current embark.\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream & out)
{
    return CR_OK;
}

static command_result orders_export_command(color_ostream & out, const std::string & name);
static command_result orders_import_command(color_ostream & out, const std::string & name);
static command_result orders_clear_command(color_ostream & out);

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

    return CR_WRONG_USAGE;
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

    {
        CoreSuspender suspend;

        for (auto it : world->manager_orders)
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

            if (it->item_category.whole != 0)
            {
                bitfield_to_json_array(order["item_category"], it->item_category);
            }

            if (it->hist_figure_id != -1)
            {
                order["hist_figure"] = it->hist_figure_id;
            }

            if (it->material_category.whole != 0)
            {
                bitfield_to_json_array(order["material_category"], it->material_category);
            }

            if (it->art_spec.type != df::job_art_specification::None)
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

                    if (it2->inorganic_bearing != -1)
                    {
                        condition["bearing"] = df::inorganic_raw::find(it2->inorganic_bearing)->id;
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

                    // TODO: anon_1, anon_2, anon_3

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

                    // TODO: anon_1

                    conditions.append(condition);
                }

                order["order_conditions"] = conditions;
            }

            // TODO: anon_1

            orders.append(order);
        }
    }

    Filesystem::mkdir("dfhack-config/orders");

    std::ofstream file("dfhack-config/orders/" + name + ".json");

    file << orders << std::endl;

    return file.good() ? CR_OK : CR_FAILURE;
}

static command_result orders_import_command(color_ostream & out, const std::string & name)
{
    if (!is_safe_filename(out, name))
    {
        return CR_WRONG_USAGE;
    }

    Json::Value orders;

    {
        std::ifstream file("dfhack-config/orders/" + name + ".json");

        if (!file.good())
        {
            out << COLOR_LIGHTRED << "Cannot find orders file." << std::endl;
            return CR_FAILURE;
        }

        file >> orders;

        if (!file.good())
        {
            out << COLOR_LIGHTRED << "Error reading orders file." << std::endl;
            return CR_FAILURE;
        }
    }

    if (orders.type() != Json::arrayValue)
    {
        out << COLOR_LIGHTRED << "Invalid orders file: expected array" << std::endl;
        return CR_FAILURE;
    }

    CoreSuspender suspend;

    std::map<int32_t, int32_t> id_mapping;
    for (auto it : orders)
    {
        id_mapping[it["id"].asInt()] = world->manager_order_next_id;
        world->manager_order_next_id++;
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
                delete order;

                out << COLOR_LIGHTRED << "Invalid item subtype for imported manager order: " << enum_item_key(order->item_type) << ":" << it["item_subtype"].asString() << std::endl;

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
            json_array_to_bitfield(order->item_category, it["item_category"]);
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

            order->hist_figure_id = it["hist_figure"].asInt();
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
                        delete condition;

                        out << COLOR_YELLOW << "Invalid item condition item subtype for imported manager order: " << enum_item_key(condition->item_type) << ":" << it2["item_subtype"].asString() << std::endl;

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
                    auto found = std::find_if(world->raws.inorganics.begin(), world->raws.inorganics.end(), [bearing](df::inorganic_raw *raw) -> bool { return raw->id == bearing; });
                    if (found == world->raws.inorganics.end())
                    {
                        delete condition;

                        out << COLOR_YELLOW << "Invalid item condition inorganic bearing type for imported manager order: " << it2["bearing"].asString() << std::endl;

                        continue;
                    }
                    condition->inorganic_bearing = found - world->raws.inorganics.begin();
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

                // TODO: anon_1, anon_2, anon_3

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

                // TODO: anon_1

                order->order_conditions.push_back(condition);
            }
        }

        // TODO: anon_1

        world->manager_orders.push_back(order);
    }

    return CR_OK;
}

static command_result orders_clear_command(color_ostream & out)
{
    CoreSuspender suspend;

    for (auto order : world->manager_orders)
    {
        for (auto condition : order->item_conditions)
        {
            delete condition;
        }
        for (auto condition : order->order_conditions)
        {
            delete condition;
        }
        if (order->anon_1)
        {
            for (auto anon_1 : *order->anon_1)
            {
                delete anon_1;
            }
            delete order->anon_1;
        }

        delete order;
    }

    out << "Deleted " << world->manager_orders.size() << " manager orders." << std::endl;

    world->manager_orders.clear();

    return CR_OK;
}
