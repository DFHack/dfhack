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
#include "df/viewscreen_dwarfmodest.h"

#include "MiscUtils.h"

#include <stdlib.h>

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;

using df::global::ui;
using df::global::world;

static bool unit_list_hotkey(df::viewscreen *top);

static command_result sort_units(color_ostream &out, vector <string> & parameters);

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

    if (!lua_istable(L, -1))
    {
        out.printerr("Not a table returned as ordering.\n");
        goto fail;
    }

    if (lua_rawlen(L, -1) != size)
    {
        out.printerr("Invalid ordering size: expected %d, actual %d\n", size, lua_rawlen(L, -1));
        goto fail;
    }

    order->clear();
    order->resize(size);
    found.resize(size);

    for (size_t i = 1; i <= size; i++)
    {
        lua_rawgeti(L, -1, i);
        int v = lua_tointeger(L, -1);
        lua_pop(L, 1);

        if (v < 1 || size_t(v) > size)
        {
            out.printerr("Order value out of range: %d\n", v);
            goto fail;
        }

        if (found[v-1])
        {
            out.printerr("Duplicate order value: %d\n", v);
            goto fail;
        }

        found[v-1] = 1;
        (*order)[i-1] = v-1;
    }

    lua_pop(L, 1);
    return true;
fail:
    lua_pop(L, 1);
    return false;
}

template<class T>
bool compute_order(color_ostream &out, lua_State *L, int base, std::vector<unsigned> *order, const std::vector<T> &key)
{
    lua_pushvalue(L, base+1);
    Lua::PushVector(L, key);
    lua_pushvalue(L, base+2);

    if (!Lua::SafeCall(out, L, 2, 1))
        return false;

    return read_order(out, L, order, key.size());
}


static bool unit_list_hotkey(df::viewscreen *screen)
{
    if (strict_virtual_cast<df::viewscreen_unitlistst>(screen))
        return true;
    if (strict_virtual_cast<df::viewscreen_joblistst>(screen))
        return true;

    if (strict_virtual_cast<df::viewscreen_dwarfmodest>(screen))
    {
        using namespace df::enums::ui_sidebar_mode;

        switch (ui->main.mode)
        {
        case Burrows:
            return ui->burrows.in_add_units_mode;
        default:
            return false;
        }
    }

    return false;
}

static command_result sort_units(color_ostream &out, vector <string> &parameters)
{
    if (parameters.empty())
        return CR_WRONG_USAGE;

    auto L = Lua::Core::State;
    int top = lua_gettop(L);

    if (!Lua::Core::PushModulePublic(out, "plugins.sort", "make_sort_order"))
    {
        out.printerr("Cannot access the sorter function.\n");
        return CR_WRONG_USAGE;
    }

    if (!parse_ordering_spec(out, L, "units", parameters))
    {
        out.printerr("Invalid unit ordering specification.\n");
        lua_settop(L, top);
        return CR_WRONG_USAGE;
    }

    auto screen = Core::getInstance().getTopViewscreen();
    std::vector<unsigned> order;

    if (auto units = strict_virtual_cast<df::viewscreen_unitlistst>(screen))
    {
        for (int i = 0; i < 4; i++)
        {
            if (compute_order(out, L, top, &order, units->units[i]))
            {
                reorder_vector(&units->units[i], order);
                reorder_vector(&units->jobs[i], order);
            }
        }
    }
    else if (auto jobs = strict_virtual_cast<df::viewscreen_joblistst>(screen))
    {
        if (compute_order(out, L, top, &order, jobs->units))
        {
            reorder_vector(&jobs->units, order);
            reorder_vector(&jobs->jobs, order);
        }
    }
    else if (strict_virtual_cast<df::viewscreen_dwarfmodest>(screen))
    {
        switch (ui->main.mode)
        {
        case ui_sidebar_mode::Burrows:
            if (compute_order(out, L, top, &order, ui->burrows.list_units))
            {
                reorder_vector(&ui->burrows.list_units, order);
                reorder_vector(&ui->burrows.sel_units, order);
            }
            break;

        default:
            break;;
        }
    }

    lua_settop(L, top);
    return CR_OK;
}
