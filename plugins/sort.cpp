#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/Gui.h"
#include "modules/Translation.h"
#include "modules/Units.h"
#include "modules/Job.h"

#include "LuaTools.h"

#include "DataDefs.h"
#include "df/plotinfost.h"
#include "df/world.h"
#include "df/viewscreen_joblistst.h"
#include "df/viewscreen_unitlistst.h"
#include "df/viewscreen_layer_militaryst.h"
#include "df/viewscreen_layer_noblelistst.h"
#include "df/viewscreen_layer_overall_healthst.h"
#include "df/viewscreen_layer_assigntradest.h"
#include "df/viewscreen_tradegoodsst.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/viewscreen_petst.h"
#include "df/viewscreen_storesst.h"
#include "df/viewscreen_workshop_profilest.h"
#include "df/layer_object_listst.h"
#include "df/assign_trade_status.h"

#include "MiscUtils.h"

#include <stdlib.h>

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("sort");
REQUIRE_GLOBAL(plotinfo);
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(ui_building_in_assign);
REQUIRE_GLOBAL(ui_building_item_cursor);
REQUIRE_GLOBAL(ui_building_assign_type);
REQUIRE_GLOBAL(ui_building_assign_is_marked);
REQUIRE_GLOBAL(ui_building_assign_units);
REQUIRE_GLOBAL(ui_building_assign_items);

static bool unit_list_hotkey(df::viewscreen *top);
static bool item_list_hotkey(df::viewscreen *top);

static command_result sort_units(color_ostream &out, vector <string> & parameters);
static command_result sort_items(color_ostream &out, vector <string> & parameters);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "sort-units", "Sort the visible unit list.", sort_units, unit_list_hotkey));
    commands.push_back(PluginCommand(
        "sort-items", "Sort the visible item list.", sort_items, item_list_hotkey));
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
        out.printerr("Invalid ordering size: expected %zu, actual %zu\n", size, lua_rawlen(L, -1));
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
    std::vector<unsigned> order; \
    if (!ParseSpec(*pout, L, type, params)) return;

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

static df::layer_object_listst *getLayerList(df::viewscreen_layer *layer, int idx)
{
    return virtual_cast<df::layer_object_listst>(vector_get(layer->layer_objects,idx));
}

typedef void (*SortHandler)(color_ostream *pout, lua_State *L, int top,
                            df::viewscreen *screen, vector<string> &parameters);

#define VIEWSCREEN(name) df::viewscreen_##name##st
#define DEFINE_SORT_HANDLER(map, screen_type, tail, screen) \
    static void CONCAT_TOKENS(SortHandler_##screen_type,__LINE__)\
        (color_ostream *pout, lua_State *L, int top, \
         VIEWSCREEN(screen_type) *screen, vector<string> &parameters); \
    DFHACK_STATIC_ADD_TO_MAP(&map, #screen_type tail, \
        (SortHandler)CONCAT_TOKENS(SortHandler_##screen_type,__LINE__) ); \
    static void CONCAT_TOKENS(SortHandler_##screen_type,__LINE__)\
        (color_ostream *pout, lua_State *L, int top, \
         VIEWSCREEN(screen_type) *screen, vector<string> &parameters)

static std::map<std::string, SortHandler> unit_sorters;

/*
 * Sort units in the 'u'nit list screen.
 */

DEFINE_SORT_HANDLER(unit_sorters, unitlist, "", units)
{
    PARSE_SPEC("units", parameters);

    int page = units->page;

    if (compute_order(*pout, L, top, &order, units->units[page]))
    {
        reorder_cursor(&units->cursor_pos[page], order);
        reorder_vector(&units->units[page], order);
        reorder_vector(&units->jobs[page], order);
    }
}

/*
 * Sort units in the 'j'ob list screen.
 */

DEFINE_SORT_HANDLER(unit_sorters, joblist, "", jobs)
{
    PARSE_SPEC("units", parameters);

    std::vector<df::unit*> units;
    for (size_t i = 0; i < jobs->units.size(); i++)
    {
        auto unit = jobs->units[i];
        if (!unit && jobs->jobs[i])
            unit = Job::getWorker(jobs->jobs[i]);
        units.push_back(unit);
    }

    if (compute_order(*pout, L, top, &order, units))
    {
        reorder_cursor(&jobs->cursor_pos, order);
        reorder_vector(&jobs->units, order);
        reorder_vector(&jobs->jobs, order);
    }
}

/*
 * Sort candidate units in the 'p'osition page of the 'm'ilitary screen.
 */

DEFINE_SORT_HANDLER(unit_sorters, layer_military, "/Positions/Candidates", military)
{
    auto &candidates = military->positions.candidates;
    auto list3 = getLayerList(military, 2);

    PARSE_SPEC("units", parameters);

    if (compute_order(*pout, L, top, &order, candidates))
    {
        reorder_cursor(&list3->cursor, order);
        reorder_vector(&candidates, order);
    }
}


DEFINE_SORT_HANDLER(unit_sorters, layer_noblelist, "/Appoint", nobles)
{
    auto list2 = getLayerList(nobles, 1);

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
}

/*
 * Sort animal units in the Animal page of the 'z' status screen.
 */

DEFINE_SORT_HANDLER(unit_sorters, pet, "/List", animals)
{
    PARSE_SPEC("units", parameters);

    std::vector<df::unit*> units;
    for (size_t i = 0; i < animals->animal.size(); i++)
        units.push_back(animals->is_vermin[i] ? NULL : animals->animal[i].unit);

    if (compute_order(*pout, L, top, &order, units))
    {
        reorder_cursor(&animals->cursor, order);
        reorder_vector(&animals->animal, order);
        reorder_vector(&animals->is_vermin, order);
        reorder_vector(&animals->is_tame, order);
        reorder_vector(&animals->is_adopting, order);
    }
}

/*
 * Sort candidate trainers in the Animal page of the 'z' status screen.
 */

DEFINE_SORT_HANDLER(unit_sorters, pet, "/SelectTrainer", animals)
{
    sort_null_first(parameters);
    PARSE_SPEC("units", parameters);

    if (compute_order(*pout, L, top, &order, animals->trainer_unit))
    {
        reorder_cursor(&animals->trainer_cursor, order);
        reorder_vector(&animals->trainer_unit, order);
        reorder_vector(&animals->trainer_mode, order);
    }
}

/*
 * Sort units in the Health page of the 'z' status screen.
 */

DEFINE_SORT_HANDLER(unit_sorters, layer_overall_health, "/Units", health)
{
    auto list1 = getLayerList(health, 0);

    PARSE_SPEC("units", parameters);

    if (compute_order(*pout, L, top, &order, health->unit))
    {
        reorder_cursor(&list1->cursor, order);
        reorder_vector(&health->unit, order);
        reorder_vector(&health->bits1, order);
        reorder_vector(&health->bits2, order);
        reorder_vector(&health->bits3, order);
    }
}

/*
 * Sort burrow member candidate units in the 'w' sidebar mode.
 */

DEFINE_SORT_HANDLER(unit_sorters, dwarfmode, "/Burrows/AddUnits", screen)
{
    PARSE_SPEC("units", parameters);

    if (compute_order(*pout, L, top, &order, plotinfo->burrows.list_units))
    {
        reorder_cursor(&plotinfo->burrows.unit_cursor_pos, order);
        reorder_vector(&plotinfo->burrows.list_units, order);
        reorder_vector(&plotinfo->burrows.sel_units, order);
    }
}

/*
 * Sort building owner candidate units in the 'q' sidebar mode, or cage assignment.
 */

DEFINE_SORT_HANDLER(unit_sorters, dwarfmode, "/QueryBuilding/Some/Assign", screen)
{
    sort_null_first(parameters);

    PARSE_SPEC("units", parameters);
    if (compute_order(*pout, L, top, &order, *ui_building_assign_units))
    {
        reorder_cursor(ui_building_item_cursor, order);
        reorder_vector(ui_building_assign_type, order);
        reorder_vector(ui_building_assign_units, order);
        // this is for cages. cages need some extra sorting
        if(ui_building_assign_items->size() == ui_building_assign_units->size())
        {
            reorder_vector(ui_building_assign_items, order);
            reorder_vector(ui_building_assign_is_marked, order);
        }
    }
}

/*
 * Sort units in the workshop 'q'uery 'P'rofile modification screen.
 */

DEFINE_SORT_HANDLER(unit_sorters, workshop_profile, "/Unit", profile)
{
    PARSE_SPEC("units", parameters);

    if (compute_order(*pout, L, top, &order, profile->workers))
    {
        reorder_cursor(&profile->worker_idx, order);
        reorder_vector(&profile->workers, order);
    }
}

/*
 * Sort pen assignment candidate units in 'z'->'N'.
 */

DEFINE_SORT_HANDLER(unit_sorters, dwarfmode, "/ZonesPenInfo/Assign", screen)
{
    PARSE_SPEC("units", parameters);

    if (compute_order(*pout, L, top, &order, *ui_building_assign_units))
    {
        reorder_cursor(ui_building_item_cursor, order);
        reorder_vector(ui_building_assign_type, order);
        reorder_vector(ui_building_assign_units, order);
        reorder_vector(ui_building_assign_items, order);
        reorder_vector(ui_building_assign_is_marked, order);
    }
}

static bool unit_list_hotkey(df::viewscreen *screen)
{
    auto focus = Gui::getFocusString(screen);
    return findPrefixInMap(unit_sorters, focus) != NULL;
}

static command_result sort_units(color_ostream &out, vector <string> &parameters)
{
    if (parameters.empty())
        return CR_WRONG_USAGE;

    auto L = Lua::Core::State;
    auto screen = Core::getInstance().getTopViewscreen();

    Lua::StackUnwinder top(L);

    if (!prepare_sort(&out, L))
        return CR_WRONG_USAGE;

    auto focus = Gui::getFocusString(screen);
    auto handler = findPrefixInMap(unit_sorters, focus);

    if (!handler)
        return CR_WRONG_USAGE;
    else
        handler(&out, L, top, screen, parameters);

    return CR_OK;
}

static std::map<std::string, SortHandler> item_sorters;

DEFINE_SORT_HANDLER(item_sorters, tradegoods, "/Items/Broker", trade)
{
    PARSE_SPEC("items", parameters);

    if (compute_order(*pout, L, top, &order, trade->broker_items))
    {
        reorder_cursor(&trade->broker_cursor, order);
        reorder_vector(&trade->broker_items, order);
        reorder_vector(&trade->broker_selected, order);
        reorder_vector(&trade->broker_count, order);
    }
}

DEFINE_SORT_HANDLER(item_sorters, tradegoods, "/Items/Trader", trade)
{
    PARSE_SPEC("items", parameters);

    if (compute_order(*pout, L, top, &order, trade->trader_items))
    {
        reorder_cursor(&trade->trader_cursor, order);
        reorder_vector(&trade->trader_items, order);
        reorder_vector(&trade->trader_selected, order);
        reorder_vector(&trade->trader_count, order);
    }
}

DEFINE_SORT_HANDLER(item_sorters, layer_assigntrade, "/Items", bring)
{
    auto list1 = getLayerList(bring, 0);
    auto list2 = getLayerList(bring, 1);
    int list_idx = vector_get(bring->visible_lists, list1->cursor, (int16_t)-1);

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
}

DEFINE_SORT_HANDLER(item_sorters, stores, "/Items", stocks)
{
    PARSE_SPEC("items", parameters);

    if (compute_order(*pout, L, top, &order, stocks->items))
    {
        reorder_cursor(&stocks->item_cursor, order);
        reorder_vector(&stocks->items, order);
    }
}

static bool item_list_hotkey(df::viewscreen *screen)
{
    auto focus = Gui::getFocusString(screen);
    return findPrefixInMap(item_sorters, focus) != NULL;
}

static command_result sort_items(color_ostream &out, vector <string> &parameters)
{
    if (parameters.empty())
        return CR_WRONG_USAGE;

    auto L = Lua::Core::State;
    auto screen = Core::getInstance().getTopViewscreen();

    Lua::StackUnwinder top(L);

    if (!prepare_sort(&out, L))
        return CR_WRONG_USAGE;

    auto focus = Gui::getFocusString(screen);
    auto handler = findPrefixInMap(item_sorters, focus);

    if (!handler)
        return CR_WRONG_USAGE;
    else
        handler(&out, L, top, screen, parameters);

    return CR_OK;
}
