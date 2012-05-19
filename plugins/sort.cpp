#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/Gui.h"
#include "modules/Translation.h"
#include "modules/Units.h"

#include "LuaTools.h"

#include "DataDefs.h"
#include "df/ui.h"
#include "df/world.h"
#include "df/viewscreen_joblistst.h"
#include "df/viewscreen_unitlistst.h"
#include "df/viewscreen_layer_militaryst.h"
#include "df/viewscreen_layer_workshop_profilest.h"
#include "df/viewscreen_layer_noblelistst.h"
#include "df/viewscreen_layer_overall_healthst.h"
#include "df/viewscreen_layer_assigntradest.h"
#include "df/viewscreen_tradegoodsst.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/viewscreen_petst.h"
#include "df/layer_object_listst.h"
#include "df/assign_trade_status.h"

#include "MiscUtils.h"

#include <stdlib.h>

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

using df::global::ui;
using df::global::world;
using df::global::ui_building_in_assign;
using df::global::ui_building_item_cursor;
using df::global::ui_building_assign_type;
using df::global::ui_building_assign_is_marked;
using df::global::ui_building_assign_units;
using df::global::ui_building_assign_items;

static bool unit_list_hotkey(df::viewscreen *top);
static bool item_list_hotkey(df::viewscreen *top);

static command_result sort_units(color_ostream &out, vector <string> & parameters);
static command_result sort_items(color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("sort");

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "sort-units", "Sort the visible unit list.", sort_units, unit_list_hotkey,
        "  sort-units order [order...]\n"
        "    Sort the unit list using the given sequence of comparisons.\n"
        "    The '<' prefix for an order makes undefined values sort first.\n"
        "    The '>' prefix reverses the sort order for defined values.\n"
        "  Unit order examples:\n"
        "    name, age, arrival, squad, squad_position, profession\n"
        "The orderings are defined in hack/lua/plugins/sort/*.lua\n"
    ));
    commands.push_back(PluginCommand(
        "sort-items", "Sort the visible item list.", sort_items, item_list_hotkey,
        "  sort-items order [order...]\n"
        "    Sort the item list using the given sequence of comparisons.\n"
        "    The '<' prefix for an order makes undefined values sort first.\n"
        "    The '>' prefix reverses the sort order for defined values.\n"
        "  Item order examples:\n"
        "    description, material, wear, type, quality\n"
        "The orderings are defined in hack/lua/plugins/sort/*.lua\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

template<class T>
void reorder_vector(std::vector<T> *pvec, const std::vector<unsigned> &order)
{
    assert(pvec->size() == order.size());

    std::vector<T> tmp(*pvec);
    for (size_t i = 0; i < order.size(); i++)
        (*pvec)[i] = tmp[order[i]];
}

template<class T>
void reorder_cursor(T *cursor, const std::vector<unsigned> &order)
{
    if (*cursor == 0)
        return;

    for (size_t i = 0; i < order.size(); i++)
    {
        if (unsigned(*cursor) == order[i])
        {
            *cursor = T(i);
            break;
        }
    }
}

bool parse_ordering_spec(color_ostream &out, lua_State *L, std::string type, const std::vector<std::string> &params)
{
    if (!lua_checkstack(L, params.size() + 2))
        return false;

    if (!Lua::PushModulePublic(out, L, "plugins.sort", "parse_ordering_spec"))
        return false;

    Lua::Push(L, type);
    for (size_t i = 0; i < params.size(); i++)
        Lua::Push(L, params[i]);

    if (!Lua::SafeCall(out, L, params.size()+1, 1))
        return false;

    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        return false;
    }

    return true;
}

bool read_order(color_ostream &out, lua_State *L, std::vector<unsigned> *order, size_t size)
{
    std::vector<char> found;

    Lua::StackUnwinder frame(L, 1);

    if (!lua_istable(L, -1))
    {
        out.printerr("Not a table returned as ordering.\n");
        return false;
    }

    if (lua_rawlen(L, -1) != size)
    {
        out.printerr("Invalid ordering size: expected %d, actual %d\n", size, lua_rawlen(L, -1));
        return false;
    }

    order->clear();
    order->resize(size);
    found.resize(size);

    for (size_t i = 1; i <= size; i++)
    {
        lua_rawgeti(L, frame[1], i);
        int v = lua_tointeger(L, -1);
        lua_pop(L, 1);

        if (v < 1 || size_t(v) > size)
        {
            out.printerr("Order value out of range: %d\n", v);
            return false;
        }

        if (found[v-1])
        {
            out.printerr("Duplicate order value: %d\n", v);
            return false;
        }

        found[v-1] = 1;
        (*order)[i-1] = v-1;
    }

    return true;
}

template<class T>
bool compute_order(color_ostream &out, lua_State *L, int base, std::vector<unsigned> *order, const std::vector<T> &key)
{
    lua_pushvalue(L, base+1);
    Lua::PushVector(L, key, true);
    lua_pushvalue(L, base+2);

    if (!Lua::SafeCall(out, L, 2, 1))
        return false;

    return read_order(out, L, order, key.size());
}

static bool ParseSpec(color_ostream &out, lua_State *L, const char *type, vector<string> &params)
{
    if (!parse_ordering_spec(out, L, type, params))
    {
        out.printerr("Invalid ordering specification for %s.\n", type);
        return false;
    }

    return true;
}

#define PARSE_SPEC(type, params) \
    if (!ParseSpec(*pout, L, type, params)) return false;

static bool prepare_sort(color_ostream *pout, lua_State *L)
{
    if (L)
    {
        if (!Lua::PushModulePublic(*pout, L, "plugins.sort", "make_sort_order"))
        {
            pout->printerr("Cannot access the sorter function.\n");
            return false;
        }
    }

    return true;
}

static void sort_null_first(vector<string> &parameters)
{
    vector_insert_at(parameters, 0, std::string("<exists"));
}

static df::layer_object_listst *getLayerList(df::viewscreen_layerst *layer, int idx)
{
    return virtual_cast<df::layer_object_listst>(vector_get(layer->layer_objects,idx));
}

static bool maybe_sort_units(color_ostream *pout, lua_State *L,
                             df::viewscreen *screen, vector<string> &parameters)
{
    Lua::StackUnwinder top(L);

    if (!prepare_sort(pout, L))
        return false;

    std::vector<unsigned> order;

    if (auto units = strict_virtual_cast<df::viewscreen_unitlistst>(screen))
    {
        if (!L) return true;

        /*
         * Sort units in the 'u'nit list screen.
         */

        PARSE_SPEC("units", parameters);

        int page = units->page;

        if (compute_order(*pout, L, top, &order, units->units[page]))
        {
            reorder_cursor(&units->cursor_pos[page], order);
            reorder_vector(&units->units[page], order);
            reorder_vector(&units->jobs[page], order);
        }

        return true;
    }
    else if (auto jobs = strict_virtual_cast<df::viewscreen_joblistst>(screen))
    {
        if (!L) return true;

        /*
         * Sort units in the 'j'ob list screen.
         */

        PARSE_SPEC("units", parameters);

        if (compute_order(*pout, L, top, &order, jobs->units))
        {
            reorder_cursor(&jobs->cursor_pos, order);
            reorder_vector(&jobs->units, order);
            reorder_vector(&jobs->jobs, order);
        }

        return true;
    }
    else if (auto military = strict_virtual_cast<df::viewscreen_layer_militaryst>(screen))
    {
        switch (military->page)
        {
        case df::viewscreen_layer_militaryst::Positions:
            {
                auto &candidates = military->positions.candidates;
                auto list3 = getLayerList(military, 2);

                /*
                 * Sort candidate units in the 'p'osition page of the 'm'ilitary screen.
                 */

                if (list3 && !candidates.empty() && list3->bright)
                {
                    if (!L) return true;

                    PARSE_SPEC("units", parameters);

                    if (compute_order(*pout, L, top, &order, candidates))
                    {
                        reorder_cursor(&list3->cursor, order);
                        reorder_vector(&candidates, order);
                    }

                    return true;
                }

                return false;
            }

        default:
            return false;
        }
    }
    else if (auto profile = strict_virtual_cast<df::viewscreen_layer_workshop_profilest>(screen))
    {
        auto list1 = getLayerList(profile, 0);

        if (!list1) return false;
        if (!L) return true;

        /*
         * Sort units in the workshop 'q'uery 'P'rofile modification screen.
         */

        PARSE_SPEC("units", parameters);

        if (compute_order(*pout, L, top, &order, profile->workers))
        {
            reorder_cursor(&list1->cursor, order);
            reorder_vector(&profile->workers, order);
        }

        return true;
    }
    else if (auto nobles = strict_virtual_cast<df::viewscreen_layer_noblelistst>(screen))
    {
        switch (nobles->mode)
        {
        case df::viewscreen_layer_noblelistst::Appoint:
            {
                auto list2 = getLayerList(nobles, 1);

                /*
                 * Sort units in the appointment candidate list of the 'n'obles screen.
                 */

                if (list2)
                {
                    if (!L) return true;

                    sort_null_first(parameters);
                    PARSE_SPEC("units", parameters);

                    std::vector<df::unit*> units;
                    for (size_t i = 0; i < nobles->candidates.size(); i++)
                        units.push_back(nobles->candidates[i]->unit);

                    if (compute_order(*pout, L, top, &order, units))
                    {
                        reorder_cursor(&list2->cursor, order);
                        reorder_vector(&nobles->candidates, order);
                    }

                    return true;
                }

                return false;
            }

        default:
            return false;
        }
    }
    else if (auto animals = strict_virtual_cast<df::viewscreen_petst>(screen))
    {
        switch (animals->mode)
        {
        case df::viewscreen_petst::List:
            {
                if (!L) return true;

                /*
                 * Sort animal units in the Animal page of the 'z' status screen.
                 */

                PARSE_SPEC("units", parameters);

                std::vector<df::unit*> units;
                for (size_t i = 0; i < animals->animal.size(); i++)
                    units.push_back(animals->is_vermin[i] ? NULL : (df::unit*)animals->animal[i]);

                if (compute_order(*pout, L, top, &order, units))
                {
                    reorder_cursor(&animals->cursor, order);
                    reorder_vector(&animals->animal, order);
                    reorder_vector(&animals->is_vermin, order);
                    reorder_vector(&animals->pet_info, order);
                    reorder_vector(&animals->is_tame, order);
                    reorder_vector(&animals->is_adopting, order);
                }

                return true;
            }

        case df::viewscreen_petst::SelectTrainer:
            {
                if (!L) return true;

                /*
                 * Sort candidate trainers in the Animal page of the 'z' status screen.
                 */

                sort_null_first(parameters);
                PARSE_SPEC("units", parameters);

                if (compute_order(*pout, L, top, &order, animals->trainer_unit))
                {
                    reorder_cursor(&animals->trainer_cursor, order);
                    reorder_vector(&animals->trainer_unit, order);
                    reorder_vector(&animals->trainer_mode, order);
                }

                return true;
            }

        default:
            return false;
        }
    }
    else if (auto health = strict_virtual_cast<df::viewscreen_layer_overall_healthst>(screen))
    {
        auto list1 = getLayerList(health, 0);

        if (!list1) return false;
        if (!L) return true;

        /*
         * Sort units in the Health page of the 'z' status screen.
         */

        PARSE_SPEC("units", parameters);

        if (compute_order(*pout, L, top, &order, health->unit))
        {
            reorder_cursor(&list1->cursor, order);
            reorder_vector(&health->unit, order);
            reorder_vector(&health->bits1, order);
            reorder_vector(&health->bits2, order);
            reorder_vector(&health->bits3, order);
        }

        return true;
    }
    else if (strict_virtual_cast<df::viewscreen_dwarfmodest>(screen))
    {
        switch (ui->main.mode)
        {
        case ui_sidebar_mode::Burrows:
            if (!L) return true;

            /*
             * Sort burrow member candidate units in the 'w' sidebar mode.
             */

            PARSE_SPEC("units", parameters);

            if (compute_order(*pout, L, top, &order, ui->burrows.list_units))
            {
                reorder_cursor(&ui->burrows.unit_cursor_pos, order);
                reorder_vector(&ui->burrows.list_units, order);
                reorder_vector(&ui->burrows.sel_units, order);
            }

            return true;

        case ui_sidebar_mode::QueryBuilding:
            if (!ui_building_in_assign || !*ui_building_in_assign)
                return false;
            // fall through for building owner / chain assign animal

        case ui_sidebar_mode::ZonesPenInfo:
            if (ui_building_item_cursor &&
                ui_building_assign_type &&
                ui_building_assign_is_marked &&
                ui_building_assign_units &&
                ui_building_assign_items &&
                ui_building_assign_type->size() == ui_building_assign_units->size() &&
                !ui_building_assign_type->empty())
            {
                if (!L) return true;

                /*
                 * Sort building owner candidate units in the 'q' sidebar mode,
                 * or pen assignment candidate units in 'z'->'N', or cage assignment.
                 */

                // TODO: better way
                bool is_assign_owner = ((*ui_building_assign_type)[0] == -1);

                if (is_assign_owner)
                    sort_null_first(parameters);

                PARSE_SPEC("units", parameters);

                if (compute_order(*pout, L, top, &order, *ui_building_assign_units))
                {
                    reorder_cursor(ui_building_item_cursor, order);
                    reorder_vector(ui_building_assign_type, order);
                    reorder_vector(ui_building_assign_units, order);

                    if (ui_building_assign_units->size() == ui_building_assign_items->size())
                        reorder_vector(ui_building_assign_items, order);
                    if (ui_building_assign_units->size() == ui_building_assign_is_marked->size())
                        reorder_vector(ui_building_assign_is_marked, order);
                }

                return true;
            }
            return false;

        default:
            return false;
        }
    }
    else
        return false;
}

static bool unit_list_hotkey(df::viewscreen *screen)
{
    vector<string> dummy;
    return maybe_sort_units(NULL, NULL, screen, dummy);
}

static command_result sort_units(color_ostream &out, vector <string> &parameters)
{
    if (parameters.empty())
        return CR_WRONG_USAGE;

    auto L = Lua::Core::State;
    auto screen = Core::getInstance().getTopViewscreen();

    if (!maybe_sort_units(&out, L, screen, parameters))
        return CR_WRONG_USAGE;

    return CR_OK;
}

static bool maybe_sort_items(color_ostream *pout, lua_State *L,
                             df::viewscreen *screen, vector<string> &parameters)
{
    Lua::StackUnwinder top(L);

    if (!prepare_sort(pout, L))
        return false;

    std::vector<unsigned> order;

    if (auto trade = strict_virtual_cast<df::viewscreen_tradegoodsst>(screen))
    {
        if (!L) return true;

        PARSE_SPEC("items", parameters);

        if (trade->in_right_pane)
        {
            if (compute_order(*pout, L, top, &order, trade->broker_items))
            {
                reorder_cursor(&trade->broker_cursor, order);
                reorder_vector(&trade->broker_items, order);
                reorder_vector(&trade->broker_selected, order);
                reorder_vector(&trade->broker_count, order);
            }
        }
        else
        {
            if (compute_order(*pout, L, top, &order, trade->trader_items))
            {
                reorder_cursor(&trade->trader_cursor, order);
                reorder_vector(&trade->trader_items, order);
                reorder_vector(&trade->trader_selected, order);
                reorder_vector(&trade->trader_count, order);
            }
        }

        return true;
    }
    else if (auto bring = strict_virtual_cast<df::viewscreen_layer_assigntradest>(screen))
    {
        auto list1 = getLayerList(bring, 0);
        auto list2 = getLayerList(bring, 1);
        if (!list1 || !list2 || !list2->bright)
            return false;

        int list_idx = vector_get(bring->visible_lists, list1->cursor, (int16_t)-1);
        unsigned num_lists = sizeof(bring->lists)/sizeof(std::vector<int32_t>);
        if (unsigned(list_idx) >= num_lists)
            return false;

        if (!L) return true;

        PARSE_SPEC("items", parameters);

        auto &vec = bring->lists[list_idx];

        std::vector<df::item*> items;
        for (size_t i = 0; i < vec.size(); i++)
            items.push_back(bring->info[vec[i]]->item);

        if (compute_order(*pout, L, top, &order, items))
        {
            reorder_cursor(&list2->cursor, order);
            reorder_vector(&vec, order);
        }

        return true;
    }
    else
        return false;
}

static bool item_list_hotkey(df::viewscreen *screen)
{
    vector<string> dummy;
    return maybe_sort_items(NULL, NULL, screen, dummy);
}

static command_result sort_items(color_ostream &out, vector <string> &parameters)
{
    if (parameters.empty())
        return CR_WRONG_USAGE;

    auto L = Lua::Core::State;
    auto screen = Core::getInstance().getTopViewscreen();

    if (!maybe_sort_items(&out, L, screen, parameters))
        return CR_WRONG_USAGE;

    return CR_OK;
}
