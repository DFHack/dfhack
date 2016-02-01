#include <map>
#include <set>
#include <queue>

#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Error.h"
#include "Export.h"
#include "LuaTools.h"
#include "LuaWrapper.h"
#include "PluginManager.h"
#include "VTableInterpose.h"
#include "modules/Gui.h"
#include "uicommon.h"

#include "df/building_tradedepotst.h"
#include "df/general_ref.h"
#include "df/general_ref_contained_in_itemst.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/viewscreen_layer_militaryst.h"
#include "df/viewscreen_locationsst.h"
#include "df/viewscreen_tradegoodsst.h"

using namespace DFHack;
using namespace df::enums;
using std::string;
using std::vector;

DFHACK_PLUGIN("confirm");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);
REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(ui);

typedef std::set<df::interface_key> ikey_set;
command_result df_confirm (color_ostream &out, vector <string> & parameters);

struct conf_wrapper;
static std::map<string, conf_wrapper*> confirmations;
string active_id;
std::queue<string> cmds;

template <typename VT, typename FT>
inline bool in_vector (std::vector<VT> &vec, FT item)
{
    return std::find(vec.begin(), vec.end(), item) != vec.end();
}

string char_replace (string s, char a, char b)
{
    string res = s;
    size_t i = res.size();
    while (i--)
        if (res[i] == a)
            res[i] = b;
    return res;
}

bool set_conf_state (string name, bool state);

struct conf_wrapper {
private:
    bool enabled;
    std::set<VMethodInterposeLinkBase*> hooks;
public:
    conf_wrapper()
        :enabled(false)
    {}
    void add_hook(VMethodInterposeLinkBase *hook)
    {
        if (!hooks.count(hook))
            hooks.insert(hook);
    }
    bool apply (bool state) {
        if (state == enabled)
            return true;
        for (auto h = hooks.begin(); h != hooks.end(); ++h)
        {
            if (!(**h).apply(state))
                return false;
        }
        enabled = state;
        return true;
    }
    inline bool is_enabled() { return enabled; }
};

namespace trade {
    static bool goods_selected (const std::vector<char> &selected)
    {
        for (auto it = selected.begin(); it != selected.end(); ++it)
            if (*it)
                return true;
        return false;
    }
    inline bool trader_goods_selected (df::viewscreen_tradegoodsst *screen)
    {
        CHECK_NULL_POINTER(screen);
        return goods_selected(screen->trader_selected);
    }
    inline bool broker_goods_selected (df::viewscreen_tradegoodsst *screen)
    {
        CHECK_NULL_POINTER(screen);
        return goods_selected(screen->broker_selected);
    }

    static bool goods_all_selected(const std::vector<char> &selected, const std::vector<df::item*> &items)  \
    {
        for (size_t i = 0; i < selected.size(); ++i)
        {
            if (!selected[i])
            {
                // check to see if item is in a container
                // (if the container is not selected, it will be detected separately)
                std::vector<df::general_ref*> &refs = items[i]->general_refs;
                bool in_container = false;
                for (auto it = refs.begin(); it != refs.end(); ++it)
                {
                    if (virtual_cast<df::general_ref_contained_in_itemst>(*it))
                    {
                        in_container = true;
                        break;
                    }
                }
                if (!in_container)
                    return false;
            }
        }
        return true;
    }
    inline bool trader_goods_all_selected(df::viewscreen_tradegoodsst *screen)
    {
        CHECK_NULL_POINTER(screen);
        return goods_all_selected(screen->trader_selected, screen->trader_items);
    }
    inline bool broker_goods_all_selected(df::viewscreen_tradegoodsst *screen)
    {
        CHECK_NULL_POINTER(screen);
        return goods_all_selected(screen->broker_selected, screen->broker_items);
    }
}

namespace conf_lua {
    static color_ostream_proxy *out;
    static lua_State *l_state;
    bool init (color_ostream &dfout)
    {
        out = new color_ostream_proxy(Core::getInstance().getConsole());
        l_state = Lua::Open(*out);
        return l_state;
    }
    void cleanup()
    {
        if (out)
        {
            delete out;
            out = NULL;
        }
        lua_close(l_state);
    }
    bool call (const char *func, int nargs = 0, int nres = 0)
    {
        if (!Lua::PushModulePublic(*out, l_state, "plugins.confirm", func))
            return false;
        if (nargs > 0)
            lua_insert(l_state, lua_gettop(l_state) - nargs);
        return Lua::SafeCall(*out, l_state, nargs, nres);
    }
    bool simple_call (const char *func)
    {
        Lua::StackUnwinder top(l_state);
        return call(func, 0, 0);
    }
    template <typename T>
    void push (T val)
    {
        Lua::Push(l_state, val);
    }
    template <typename KeyType, typename ValueType>
    void table_set (lua_State *L, KeyType k, ValueType v)
    {
        Lua::Push(L, k);
        Lua::Push(L, v);
        lua_settable(L, -3);
    }
    namespace api {
        int get_ids (lua_State *L)
        {
            lua_newtable(L);
            for (auto it = confirmations.begin(); it != confirmations.end(); ++it)
                table_set(L, it->first, true);
            return 1;
        }
        int get_conf_data (lua_State *L)
        {
            lua_newtable(L);
            int i = 1;
            for (auto it = confirmations.begin(); it != confirmations.end(); ++it)
            {
                Lua::Push(L, i++);
                lua_newtable(L);
                table_set(L, "id", it->first);
                table_set(L, "enabled", it->second->is_enabled());
                lua_settable(L, -3);
            }
            return 1;
        }
        int get_active_id (lua_State *L)
        {
            if (active_id.size())
                Lua::Push(L, active_id);
            else
                lua_pushnil(L);
            return 1;
        }
    }
}

#define CONF_LUA_FUNC(ns, name) {#name, df::wrap_function(ns::name, true)}
DFHACK_PLUGIN_LUA_FUNCTIONS {
    CONF_LUA_FUNC( , set_conf_state),
    CONF_LUA_FUNC(trade, broker_goods_selected),
    CONF_LUA_FUNC(trade, broker_goods_all_selected),
    CONF_LUA_FUNC(trade, trader_goods_selected),
    CONF_LUA_FUNC(trade, trader_goods_all_selected),
    DFHACK_LUA_END
};

#define CONF_LUA_CMD(name) {#name, conf_lua::api::name}
DFHACK_PLUGIN_LUA_COMMANDS {
    CONF_LUA_CMD(get_ids),
    CONF_LUA_CMD(get_conf_data),
    CONF_LUA_CMD(get_active_id),
    DFHACK_LUA_END
};

void show_options()
{
    cmds.push("gui/confirm-opts");
}

template <class T>
class confirmation {
public:
    enum cstate { INACTIVE, ACTIVE, SELECTED };
    typedef T screen_type;
    screen_type *screen;
    void set_state (cstate s)
    {
        state = s;
        if (s == INACTIVE)
            active_id = "";
        else
            active_id = get_id();
    }
    bool feed (ikey_set *input) {
        if (state == INACTIVE)
        {
            for (auto it = input->begin(); it != input->end(); ++it)
            {
                if (intercept_key(*it))
                {
                    last_key = *it;
                    set_state(ACTIVE);
                    return true;
                }
            }
            return false;
        }
        else if (state == ACTIVE)
        {
            if (input->count(df::interface_key::LEAVESCREEN))
                set_state(INACTIVE);
            else if (input->count(df::interface_key::SELECT))
                set_state(SELECTED);
            else if (input->count(df::interface_key::CUSTOM_S))
                show_options();
            return true;
        }
        return false;
    }
    bool key_conflict (df::interface_key key)
    {
        if (key == df::interface_key::SELECT || key == df::interface_key::LEAVESCREEN)
            return false;
        return state == ACTIVE;
    }
    void render() {
        static vector<string> lines;
        Screen::Pen corner_ul = Screen::Pen((char)201, COLOR_GREY, COLOR_BLACK);
        Screen::Pen corner_ur = Screen::Pen((char)187, COLOR_GREY, COLOR_BLACK);
        Screen::Pen corner_dl = Screen::Pen((char)200, COLOR_GREY, COLOR_BLACK);
        Screen::Pen corner_dr = Screen::Pen((char)188, COLOR_GREY, COLOR_BLACK);
        Screen::Pen border_ud = Screen::Pen((char)205, COLOR_GREY, COLOR_BLACK);
        Screen::Pen border_lr = Screen::Pen((char)186, COLOR_GREY, COLOR_BLACK);
        if (state == ACTIVE)
        {
            split_string(&lines, get_message(), "\n");
            size_t max_length = 40;
            for (auto it = lines.begin(); it != lines.end(); ++it)
                max_length = std::max(max_length, it->size());
            int width = max_length + 4;
            int height = lines.size() + 4;
            int x1 = (gps->dimx / 2) - (width / 2);
            int x2 = x1 + width - 1;
            int y1 = (gps->dimy / 2) - (height / 2);
            int y2 = y1 + height - 1;
            for (int x = x1; x <= x2; x++)
            {
                Screen::paintTile(border_ud, x, y1);
                Screen::paintTile(border_ud, x, y2);
            }
            for (int y = y1; y <= y2; y++)
            {
                Screen::paintTile(border_lr, x1, y);
                Screen::paintTile(border_lr, x2, y);
            }
            Screen::paintTile(corner_ul, x1, y1);
            Screen::paintTile(corner_ur, x2, y1);
            Screen::paintTile(corner_dl, x1, y2);
            Screen::paintTile(corner_dr, x2, y2);
            string title = " " + get_title() + " ";
            Screen::paintString(Screen::Pen(' ', COLOR_DARKGREY, COLOR_BLACK),
                x2 - 6, y1, "DFHack");
            Screen::paintString(Screen::Pen(' ', COLOR_BLACK, COLOR_GREY),
                (gps->dimx / 2) - (title.size() / 2), y1, title);
            int x = x1 + 2;
            int y = y2;
            OutputString(COLOR_LIGHTRED, x, y, Screen::getKeyDisplay(df::interface_key::LEAVESCREEN));
            OutputString(COLOR_WHITE, x, y, ": Cancel");
            x = (gps->dimx - (Screen::getKeyDisplay(df::interface_key::CUSTOM_S) + ": Settings").size()) / 2 + 1;
            OutputString(COLOR_LIGHTRED, x, y, Screen::getKeyDisplay(df::interface_key::CUSTOM_S));
            OutputString(COLOR_WHITE, x, y, ": Settings");
            x = x2 - 2 - 3 - Screen::getKeyDisplay(df::interface_key::SELECT).size();
            OutputString(COLOR_LIGHTRED, x, y, Screen::getKeyDisplay(df::interface_key::SELECT));
            OutputString(COLOR_WHITE, x, y, ": Ok");
            Screen::fillRect(Screen::Pen(' ', COLOR_BLACK, COLOR_BLACK), x1 + 1, y1 + 1, x2 - 1, y2 - 1);
            for (size_t i = 0; i < lines.size(); i++)
            {
                Screen::paintString(Screen::Pen(' ', get_color(), COLOR_BLACK), x1 + 2, y1 + 2 + i, lines[i]);
            }
        }
        else if (state == SELECTED)
        {
            ikey_set tmp;
            tmp.insert(last_key);
            screen->feed(&tmp);
            set_state(INACTIVE);
        }
    }
    virtual string get_id() = 0;
    #define CONF_LUA_START using namespace conf_lua; Lua::StackUnwinder unwind(l_state); push(screen); push(get_id());
    bool intercept_key (df::interface_key key)
    {
        CONF_LUA_START;
        push(key);
        if (call("intercept_key", 3, 1))
            return lua_toboolean(l_state, -1);
        else
            return false;
    };
    string get_title()
    {
        CONF_LUA_START;
        if (call("get_title", 2, 1) && lua_isstring(l_state, -1))
            return lua_tostring(l_state, -1);
        else
            return "Confirm";
    }
    string get_message()
    {
        CONF_LUA_START;
        if (call("get_message", 2, 1) && lua_isstring(l_state, -1))
            return lua_tostring(l_state, -1);
        else
            return "<Message generation failed>";
    };
    UIColor get_color()
    {
        CONF_LUA_START;
        if (call("get_color", 2, 1) && lua_isnumber(l_state, -1))
            return lua_tointeger(l_state, -1) % 16;
        else
            return COLOR_YELLOW;
    }
    #undef CONF_LUA_START
protected:
    cstate state;
    df::interface_key last_key;
};

template<typename T>
int conf_register(confirmation<T> *c, ...)
{
    conf_wrapper *w = new conf_wrapper();
    confirmations[c->get_id()] = w;
    va_list args;
    va_start(args, c);
    while (VMethodInterposeLinkBase *hook = va_arg(args, VMethodInterposeLinkBase*))
        w->add_hook(hook);
    va_end(args);
    return 0;
}

#define IMPLEMENT_CONFIRMATION_HOOKS(cls, prio) \
static cls cls##_instance; \
struct cls##_hooks : cls::screen_type { \
    typedef cls::screen_type interpose_base; \
    DEFINE_VMETHOD_INTERPOSE(void, feed, (ikey_set *input)) \
    { \
        cls##_instance.screen = this; \
        if (!cls##_instance.feed(input)) \
            INTERPOSE_NEXT(feed)(input); \
    } \
    DEFINE_VMETHOD_INTERPOSE(void, render, ()) \
    { \
        cls##_instance.screen = this; \
        INTERPOSE_NEXT(render)(); \
        cls##_instance.render(); \
    } \
    DEFINE_VMETHOD_INTERPOSE(bool, key_conflict, (df::interface_key key)) \
    { \
        return cls##_instance.key_conflict(key) || INTERPOSE_NEXT(key_conflict)(key); \
    } \
}; \
IMPLEMENT_VMETHOD_INTERPOSE_PRIO(cls##_hooks, feed, prio); \
IMPLEMENT_VMETHOD_INTERPOSE_PRIO(cls##_hooks, render, prio); \
IMPLEMENT_VMETHOD_INTERPOSE_PRIO(cls##_hooks, key_conflict, prio); \
static int conf_register_##cls = conf_register(&cls##_instance, \
    &INTERPOSE_HOOK(cls##_hooks, feed), \
    &INTERPOSE_HOOK(cls##_hooks, render), \
    &INTERPOSE_HOOK(cls##_hooks, key_conflict), \
    NULL);

#define DEFINE_CONFIRMATION(cls, screen, prio) \
    class confirmation_##cls : public confirmation<df::screen> { \
        virtual string get_id() { static string id = char_replace(#cls, '_', '-'); return id; } \
    }; \
    IMPLEMENT_CONFIRMATION_HOOKS(confirmation_##cls, prio);

/* This section defines stubs for all confirmation dialogs, with methods
    implemented in plugins/lua/confirm.lua.
    IDs (used in the "confirm enable/disable" command, by Lua, and in the docs)
    are obtained by replacing '_' with '-' in the first argument to DEFINE_CONFIRMATION
*/
DEFINE_CONFIRMATION(trade,              viewscreen_tradegoodsst, 0);
DEFINE_CONFIRMATION(trade_cancel,       viewscreen_tradegoodsst, -1);
DEFINE_CONFIRMATION(trade_seize,        viewscreen_tradegoodsst, 0);
DEFINE_CONFIRMATION(trade_offer,        viewscreen_tradegoodsst, 0);
DEFINE_CONFIRMATION(trade_select_all,   viewscreen_tradegoodsst, 0);
DEFINE_CONFIRMATION(haul_delete,        viewscreen_dwarfmodest, 0);
DEFINE_CONFIRMATION(depot_remove,       viewscreen_dwarfmodest, 0);
DEFINE_CONFIRMATION(squad_disband,      viewscreen_layer_militaryst, 0);
DEFINE_CONFIRMATION(uniform_delete,     viewscreen_layer_militaryst, 0);
DEFINE_CONFIRMATION(note_delete,        viewscreen_dwarfmodest, 0);
DEFINE_CONFIRMATION(route_delete,       viewscreen_dwarfmodest, 0);
DEFINE_CONFIRMATION(location_retire,    viewscreen_locationsst, 0);

DFhackCExport command_result plugin_init (color_ostream &out, vector <PluginCommand> &commands)
{
    if (!conf_lua::init(out))
        return CR_FAILURE;
    commands.push_back(PluginCommand(
        "confirm",
        "Confirmation dialogs",
        df_confirm,
        false, //allow non-interactive use

        "  confirmation enable|disable option|all ...\n"
        "  confirmation help|status\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_enable (color_ostream &out, bool enable)
{
    if (is_enabled != enable)
    {
        for (auto c = confirmations.begin(); c != confirmations.end(); ++c)
        {
            if (!c->second->apply(enable))
                return CR_FAILURE;
        }
        is_enabled = enable;
    }
    if (is_enabled)
    {
        conf_lua::simple_call("check");
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    if (plugin_enable(out, false) != CR_OK)
        return CR_FAILURE;
    conf_lua::cleanup();
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate (color_ostream &out)
{
    while (!cmds.empty())
    {
        Core::getInstance().runCommand(out, cmds.front());
        cmds.pop();
    }
    return CR_OK;
}

bool set_conf_state (string name, bool state)
{
    bool found = false;
    for (auto it = confirmations.begin(); it != confirmations.end(); ++it)
    {
        if (it->first == name)
        {
            found = true;
            it->second->apply(state);
        }
    }
    return found;
}

void enable_conf (color_ostream &out, string name, bool state)
{
    if (!set_conf_state(name, state))
        out.printerr("Unrecognized option: %s\n", name.c_str());
}

command_result df_confirm (color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;
    bool state = true;
    if (parameters.empty() || in_vector(parameters, "help") || in_vector(parameters, "status"))
    {
        out << "Available options: \n";
        for (auto it = confirmations.begin(); it != confirmations.end(); ++it)
            out.print("  %20s: %s\n", it->first.c_str(), it->second->is_enabled() ? "enabled" : "disabled");
        return CR_OK;
    }
    for (auto it = parameters.begin(); it != parameters.end(); ++it)
    {
        if (*it == "enable")
            state = true;
        else if (*it == "disable")
            state = false;
        else if (*it == "all")
        {
            for (auto it = confirmations.begin(); it != confirmations.end(); ++it)
                it->second->apply(state);
        }
        else
            enable_conf(out, *it, state);
    }
    return CR_OK;
}
