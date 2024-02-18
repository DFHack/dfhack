#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"

#include "modules/Gui.h"
#include "modules/Translation.h"

#include "df/gamest.h"
#include "df/unit.h"
#include "df/widget_unit_list.h"

using std::vector;
using std::string;

using namespace DFHack;

DFHACK_PLUGIN("sort");

REQUIRE_GLOBAL(game);
REQUIRE_GLOBAL(world);

namespace DFHack {
    DBG_DECLARE(sort, log, DebugCategory::LINFO);
}

static void call_sort_lua(color_ostream *out, const char *fn_name,
        int nargs = 0, int nres = 0,
        Lua::LuaLambda && args_lambda = Lua::DEFAULT_LUA_LAMBDA,
        Lua::LuaLambda && res_lambda = Lua::DEFAULT_LUA_LAMBDA) {
    CoreSuspender guard;

    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!out)
        out = &Core::getInstance().getConsole();

    DEBUG(log, *out).print("calling sort lua function: '%s'\n", fn_name);

    Lua::CallLuaModuleFunction(*out, L, "plugins.sort", fn_name, nargs, nres,
                               std::forward<Lua::LuaLambda&&>(args_lambda),
                               std::forward<Lua::LuaLambda&&>(res_lambda));
}

static bool probing = false;
static bool probe_result = false;

using item_or_unit = std::pair<void *, bool>;
using filter_vec_type = std::vector<std::function<bool(item_or_unit)>>;

static bool do_filter(item_or_unit elem) {
    DEBUG(log).print("filtering elem: unit: %p, flag: %d\n", elem.first, elem.second);

    if (elem.second) return true;
    auto unit = (df::unit *)elem.first;

    if (probing) {
        DEBUG(log).print("probe successful\n");
        probe_result = true;
        return false;
    }

    bool ret = true;
    call_sort_lua(NULL, "do_filter", 1, 1,
        [&](lua_State *L){ Lua::Push(L, unit); },
        [&](lua_State *L){ ret = lua_toboolean(L, 1); }
    );
    DEBUG(log).print("filter result for %s: %d\n", Translation::TranslateName(&unit->name, false).c_str(), ret);
    return !ret;
}

static int32_t our_filter_idx(df::widget_unit_list *unitlist) {
    if (world->units.active.empty())
        return true;

    df::unit *u = world->units.active[0];
    if (!u)
        return true;

    probing = true;
    probe_result = false;
    int32_t idx = 0;

    filter_vec_type *filter_vec = reinterpret_cast<filter_vec_type *>(&unitlist->filter_func);

    DEBUG(log).print("probing for our filter function\n");
    for (auto fn : *filter_vec) {
        fn(std::make_pair(u, false));
        if (probe_result) {
            DEBUG(log).print("found our filter function at idx %d\n", idx);
            break;
        }
        ++idx;
    }

    probing = false;
    return probe_result ? idx : -1;
}

static df::widget_unit_list * get_unit_list() {
    return virtual_cast<df::widget_unit_list>(
        Gui::getWidget(&game->main_interface.unit_selector, "Unit selector"));
}

DFhackCExport command_result plugin_init(color_ostream &out, vector<PluginCommand> &commands) {
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    auto unitlist = get_unit_list();
    if (unitlist) {
        int32_t idx = our_filter_idx(unitlist);
        if (idx >= 0) {
            DEBUG(log).print("removing our filter function\n");
            vector_erase_at(unitlist->filter_func, idx);
        }
    }
    return CR_OK;
}

//
// Lua API
//

static void sort_set_filter_fn(color_ostream &out) {
    auto unitlist = get_unit_list();
    if (unitlist && our_filter_idx(unitlist) == -1) {
        DEBUG(log).print("adding our filter function\n");
        filter_vec_type *filter_vec = reinterpret_cast<filter_vec_type *>(&unitlist->filter_func);
        filter_vec->emplace_back(do_filter);
    }
}

static void sort_set_sort_fn(color_ostream &out) {

}

DFHACK_PLUGIN_LUA_FUNCTIONS{
    DFHACK_LUA_FUNCTION(sort_set_filter_fn),
    DFHACK_LUA_FUNCTION(sort_set_sort_fn),
    DFHACK_LUA_END
};
