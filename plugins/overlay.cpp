#include "df/enabler.h"
#include "df/viewscreen_adopt_regionst.h"
#include "df/viewscreen_choose_game_typest.h"
#include "df/viewscreen_choose_start_sitest.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/viewscreen_export_regionst.h"
#include "df/viewscreen_game_cleanerst.h"
#include "df/viewscreen_initial_prepst.h"
#include "df/viewscreen_legendsst.h"
#include "df/viewscreen_loadgamest.h"
#include "df/viewscreen_new_arenast.h"
#include "df/viewscreen_new_regionst.h"
#include "df/viewscreen_savegamest.h"
#include "df/viewscreen_setupdwarfgamest.h"
#include "df/viewscreen_titlest.h"
#include "df/viewscreen_update_regionst.h"
#include "df/viewscreen_worldst.h"
#include "df/world.h"

#include "Debug.h"
#include "LuaTools.h"
#include "MemAccess.h"
#include "PluginManager.h"
#include "VTableInterpose.h"

#include "modules/Gui.h"
#include "modules/Screen.h"

using namespace DFHack;

DFHACK_PLUGIN("overlay");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(enabler);

namespace DFHack {
    DBG_DECLARE(overlay, control, DebugCategory::LINFO);
    DBG_DECLARE(overlay, event, DebugCategory::LINFO);
}

static df::coord2d screenSize;

// instrumentation
uint32_t total_overlay_ms = 0;
static uint32_t get_framework_timer()   { return total_overlay_ms; }
static void     reset_framework_timer() { total_overlay_ms = 0;    }

static void overlay_interpose_lua(const char *fn_name, int nargs = 0, int nres = 0,
        Lua::LuaLambda && args_lambda = Lua::DEFAULT_LUA_LAMBDA,
        Lua::LuaLambda && res_lambda = Lua::DEFAULT_LUA_LAMBDA) {
    DEBUG(event).print("calling overlay lua function: '%s'\n", fn_name);

    CoreSuspender guard;

    color_ostream & out = Core::getInstance().getConsole();
    auto L = Lua::Core::State;

    uint32_t start_ms = Core::getInstance().p->getTickCount();

    Lua::CallLuaModuleFunction(out, L, "plugins.overlay", fn_name, nargs, nres,
                               std::forward<Lua::LuaLambda&&>(args_lambda),
                               std::forward<Lua::LuaLambda&&>(res_lambda));

   total_overlay_ms += Core::getInstance().p->getTickCount() - start_ms;
}

template<class T>
struct viewscreen_overlay : T {
    typedef T interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, logic, ()) {
        INTERPOSE_NEXT(logic)();
        overlay_interpose_lua("plugins.overlay", "update_viewscreen_widgets", 2, 0,
                [&](lua_State *L) {
                    Lua::Push(L, T::_identity.getName());
                    Lua::Push(L, this);
                });
    }
    DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key> *input)) {
        bool input_is_handled = false;
        // don't send input to the overlays if there is a modal dialog up
        if (!world->status.popups.size()) {
            overlay_interpose_lua("plugins.overlay", "feed_viewscreen_widgets", 3, 1,
                    [&](lua_State *L) {
                        Lua::Push(L, T::_identity.getName());
                        Lua::Push(L, this);
                        Lua::PushInterfaceKeys(L, Screen::normalize_text_keys(*input));
                    }, [&](lua_State *L) {
                        input_is_handled = lua_toboolean(L, -1);
                    });
        }
        if (!input_is_handled)
            INTERPOSE_NEXT(feed)(input);
        else
            dfhack_lua_viewscreen::markInputAsHandled();
    }
    DEFINE_VMETHOD_INTERPOSE(void, render, (uint32_t curtick)) {
        INTERPOSE_NEXT(render)(curtick);
        overlay_interpose_lua("plugins.overlay", "render_viewscreen_widgets", 2, 0,
                [&](lua_State *L) {
                    Lua::Push(L, T::_identity.getName());
                    Lua::Push(L, this);
                });
    }
};

#define IMPLEMENT_HOOKS(screen) \
    typedef viewscreen_overlay<df::viewscreen_##screen##st> screen##_overlay; \
    template<> IMPLEMENT_VMETHOD_INTERPOSE_PRIO(screen##_overlay, logic, 100); \
    template<> IMPLEMENT_VMETHOD_INTERPOSE_PRIO(screen##_overlay, feed, 100); \
    template<> IMPLEMENT_VMETHOD_INTERPOSE_PRIO(screen##_overlay, render, 100);

IMPLEMENT_HOOKS(adopt_region)
IMPLEMENT_HOOKS(choose_game_type)
IMPLEMENT_HOOKS(choose_start_site)
IMPLEMENT_HOOKS(dwarfmode)
IMPLEMENT_HOOKS(export_region)
IMPLEMENT_HOOKS(game_cleaner)
IMPLEMENT_HOOKS(initial_prep)
IMPLEMENT_HOOKS(legends)
IMPLEMENT_HOOKS(loadgame)
IMPLEMENT_HOOKS(new_arena)
IMPLEMENT_HOOKS(new_region)
IMPLEMENT_HOOKS(savegame)
IMPLEMENT_HOOKS(setupdwarfgame)
IMPLEMENT_HOOKS(title)
IMPLEMENT_HOOKS(update_region)
IMPLEMENT_HOOKS(world)

#undef IMPLEMENT_HOOKS

#define INTERPOSE_HOOKS_FAILED(screen) \
    !INTERPOSE_HOOK(screen##_overlay, logic).apply(enable) || \
    !INTERPOSE_HOOK(screen##_overlay, feed).apply(enable) || \
    !INTERPOSE_HOOK(screen##_overlay, render).apply(enable)

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (is_enabled == enable)
        return CR_OK;

    if (enable) {
        screenSize = Screen::getWindowSize();
        Lua::CallLuaModuleFunction(out, "plugins.overlay", "rescan");
    }

    DEBUG(control).print("%sing interpose hooks\n", enable ? "enabl" : "disabl");

    if (INTERPOSE_HOOKS_FAILED(adopt_region) ||
            INTERPOSE_HOOKS_FAILED(choose_start_site) ||
            INTERPOSE_HOOKS_FAILED(choose_game_type) ||
            INTERPOSE_HOOKS_FAILED(dwarfmode) ||
            INTERPOSE_HOOKS_FAILED(export_region) ||
            INTERPOSE_HOOKS_FAILED(game_cleaner) ||
            INTERPOSE_HOOKS_FAILED(initial_prep) ||
            INTERPOSE_HOOKS_FAILED(legends) ||
            INTERPOSE_HOOKS_FAILED(loadgame) ||
            INTERPOSE_HOOKS_FAILED(new_arena) ||
            INTERPOSE_HOOKS_FAILED(new_region) ||
            INTERPOSE_HOOKS_FAILED(savegame) ||
            INTERPOSE_HOOKS_FAILED(setupdwarfgame) ||
            INTERPOSE_HOOKS_FAILED(title) ||
            INTERPOSE_HOOKS_FAILED(update_region) ||
            INTERPOSE_HOOKS_FAILED(world))
        return CR_FAILURE;

    is_enabled = enable;
    return CR_OK;
}

#undef INTERPOSE_HOOKS_FAILED

static command_result overlay_cmd(color_ostream &out, std::vector <std::string> & parameters) {
    CoreSuspender suspend;

    bool show_help = false;
    Lua::CallLuaModuleFunction(out, "plugins.overlay", "overlay_command", std::make_tuple(parameters),
        1, [&](lua_State *L) {
            show_help = !lua_toboolean(L, -1);
        });

    return show_help ? CR_WRONG_USAGE : CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    commands.push_back(
        PluginCommand(
            "overlay",
            "Manage onscreen widgets.",
            overlay_cmd,
            Gui::anywhere_hotkey));

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    return plugin_enable(out, false);
}

DFhackCExport command_result plugin_onupdate (color_ostream &out) {
    df::coord2d newScreenSize = Screen::getWindowSize();
    if (newScreenSize != screenSize) {
        Lua::CallLuaModuleFunction(out, "plugins.overlay", "reposition_widgets");
        screenSize = newScreenSize;
    }
    Lua::CallLuaModuleFunction(out, "plugins.overlay", "update_hotspot_widgets");
    return CR_OK;
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(get_framework_timer),
    DFHACK_LUA_FUNCTION(reset_framework_timer),
    DFHACK_LUA_END
};
