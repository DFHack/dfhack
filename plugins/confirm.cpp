#include <map>
#include <set>
#include <queue>

#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Debug.h"
#include "Error.h"
#include "Export.h"
#include "LuaTools.h"
#include "LuaWrapper.h"
#include "PluginManager.h"
#include "VTableInterpose.h"
#include "modules/Gui.h"
#include "uicommon.h"

#include "df/gamest.h"
#include "df/general_ref.h"
#include "df/general_ref_contained_in_itemst.h"
#include "df/interfacest.h"
#include "df/viewscreen_dwarfmodest.h"

using namespace DFHack;
using namespace df::enums;
using std::map;
using std::queue;
using std::string;
using std::vector;

DFHACK_PLUGIN("confirm");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);
REQUIRE_GLOBAL(game);
REQUIRE_GLOBAL(gps);

typedef std::set<df::interface_key> ikey_set;
command_result df_confirm (color_ostream &out, vector <string> & parameters);

struct conf_wrapper;
static map<string, conf_wrapper*> confirmations;
string active_id;
queue<string> cmds;

namespace DFHack {
    DBG_DECLARE(confirm,status);
}

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
bool set_conf_paused (string name, bool pause);

class confirmation_base {
public:
    enum cstate { INACTIVE, ACTIVE, SELECTED };
    virtual string get_id() = 0;
    virtual string get_focus_string() = 0;
    virtual bool set_state(cstate) = 0;

    static bool set_state(string id, cstate state)
    {
        if (active && active->get_id() == id)
        {
            active->set_state(state);
            return true;
        }
        return false;
    }
protected:
    static confirmation_base *active;
};
confirmation_base *confirmation_base::active = nullptr;

struct conf_wrapper {
private:
    bool enabled;
    bool paused;
    std::set<VMethodInterposeLinkBase*> hooks;
public:
    conf_wrapper()
        :enabled(false),
        paused(false)
    {}
    void add_hook(VMethodInterposeLinkBase *hook)
    {
        if (!hooks.count(hook))
            hooks.insert(hook);
    }
    bool apply (bool state) {
        if (state == enabled)
            return true;
        for (auto hook : hooks)
        {
            if (!hook->apply(state))
                return false;
        }
        enabled = state;
        return true;
    }
    bool set_paused (bool pause) {
        paused = pause;
        return true;
    }
    inline bool is_enabled() { return enabled; }
    inline bool is_paused() { return paused; }
};

namespace trade {
    static bool goods_selected (std::vector<uint8_t> &selected)
    {
        if(!game->main_interface.trade.open)
            return false;

        for (uint8_t sel : selected)
            if (sel == 1)
                return true;
        return false;
    }
    inline bool trader_goods_selected ()
    {
        return goods_selected(game->main_interface.trade.goodflag[0]);
    }
    inline bool broker_goods_selected ()
    {
        return goods_selected(game->main_interface.trade.goodflag[1]);
    }

    /*static bool goods_all_selected(const std::vector<char>& selected, const std::vector<df::item*>& items)  \
    {
        for (size_t i = 0; i < selected.size(); ++i)
        {
            if (!selected[i])
            {
                // check to see if item is in a container
                // (if the container is not selected, it will be detected separately)
                bool in_container = false;
                for (auto ref : items[i]->general_refs)
                {
                    if (virtual_cast<df::general_ref_contained_in_itemst>(ref))
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
    inline bool trader_goods_all_selected()
    {
        return goods_all_selected(screen->trader_selected, screen->trader_items);
    }
    inline bool broker_goods_all_selected()
    {
        return goods_all_selected(screen->broker_selected, screen->broker_items);
    }*/
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
            out = nullptr;
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
    namespace api {
        int get_ids (lua_State *L)
        {
            lua_newtable(L);
            for (auto item : confirmations)
                Lua::TableInsert(L, item.first, true);
            return 1;
        }
        int get_conf_data (lua_State *L)
        {
            lua_newtable(L);
            int i = 1;
            for (auto item : confirmations)
            {
                Lua::Push(L, i++);
                lua_newtable(L);
                Lua::TableInsert(L, "id", item.first);
                Lua::TableInsert(L, "enabled", item.second->is_enabled());
                Lua::TableInsert(L, "paused", item.second->is_paused());
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
    CONF_LUA_FUNC( , set_conf_paused),
    CONF_LUA_FUNC(trade, broker_goods_selected),
    //CONF_LUA_FUNC(trade, broker_goods_all_selected),
    CONF_LUA_FUNC(trade, trader_goods_selected),
    //CONF_LUA_FUNC(trade, trader_goods_all_selected),
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
    cmds.push("gui/confirm");
}

template <class T>
class confirmation : public confirmation_base {
public:
    typedef T screen_type;
    screen_type *screen;

    bool set_state (cstate s) override
    {
        if (confirmation_base::active && confirmation_base::active != this)
        {
            // Stop this confirmation from appearing over another one
            return false;
        }

        state = s;
        if (s == INACTIVE) {
            active_id = "";
            confirmation_base::active = nullptr;
        }
        else {
            active_id = get_id();
            confirmation_base::active = this;
        }
        return true;
    }
    bool feed (ikey_set *input) {
        bool mouseExit = false;
        if(df::global::enabler->mouse_rbut) {
            mouseExit = true;
        }
        bool mouseSelect = false;
        if(df::global::enabler->mouse_lbut) {
            mouseSelect = true;
        }

        conf_wrapper *wrapper = confirmations[this->get_id()];
        if(wrapper->is_paused()) {
            std::string concernedFocus = this->get_focus_string();
            if(!Gui::matchFocusString(this->get_focus_string()))
                wrapper->set_paused(false);
            return false;
        } else if (state == INACTIVE)
        {
            if(mouseExit) {
                if(intercept_key("MOUSE_RIGHT") && set_state(ACTIVE)) {
                    df::global::enabler->mouse_rbut = 0;
                    df::global::enabler->mouse_rbut_down = 0;
                    mouse_pos = df::coord2d(df::global::gps->mouse_x, df::global::gps->mouse_y);
                    last_key_is_right_click = true;
                    return true;
                }
            } else
                last_key_is_right_click = false;

            if(mouseSelect) {
                if(intercept_key("MOUSE_LEFT") && set_state(ACTIVE)) {
                    df::global::enabler->mouse_lbut = 0;
                    df::global::enabler->mouse_lbut_down = 0;
                    mouse_pos = df::coord2d(df::global::gps->mouse_x, df::global::gps->mouse_y);
                    last_key_is_left_click = true;
                    return true;
                }
            } else
                last_key_is_left_click = false;

            for (df::interface_key key : *input)
            {
                if (intercept_key(key) && set_state(ACTIVE))
                {
                    last_key = key;
                    return true;
                }
            }
            return false;
        }
        else if (state == ACTIVE)
        {
            if (input->count(df::interface_key::LEAVESCREEN) || mouseExit) {
                if(mouseExit) {
                    df::global::enabler->mouse_rbut = 0;
                    df::global::enabler->mouse_rbut_down = 0;
                }
                set_state(INACTIVE);
            } else if (input->count(df::interface_key::SELECT))
                set_state(SELECTED);
            else if (input->count(df::interface_key::CUSTOM_P))
            {
                DEBUG(status).print("pausing\n");

                wrapper->set_paused(true);
                set_state(INACTIVE);
            }
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
        static const std::string pause_message =
               "Pause confirmations until you exit this screen";
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
            for (string line : lines)
                max_length = std::max(max_length, line.size());
            int width = max_length + 4;
            vector<string> pause_message_lines;
            word_wrap(&pause_message_lines, pause_message, max_length - 3);
            int height = lines.size() + pause_message_lines.size() + 5;
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
            y = y1 + 3 + lines.size();
            for (size_t i = 0; i < pause_message_lines.size(); i++)
            {
                Screen::paintString(Screen::Pen(' ', COLOR_WHITE, COLOR_BLACK), x1 + 5, y + i, pause_message_lines[i]);
            }
            x = x1 + 2;
            OutputString(COLOR_LIGHTRED, x, y, Screen::getKeyDisplay(df::interface_key::CUSTOM_P));
            OutputString(COLOR_WHITE, x, y, ":");
        }
        else if (state == SELECTED)
        {
            ikey_set tmp;
            if(last_key_is_left_click) {
                long prevx = df::global::gps->mouse_x;
                long prevy = df::global::gps->mouse_y;
                df::global::gps->mouse_x = mouse_pos.x;
                df::global::gps->mouse_y = mouse_pos.y;
                df::global::enabler->mouse_lbut = 1;
                df::global::enabler->mouse_lbut_down = 1;
                screen->feed(&tmp);
                df::global::enabler->mouse_lbut = 0;
                df::global::enabler->mouse_lbut_down = 0;
                df::global::gps->mouse_x = prevx;
                df::global::gps->mouse_y = prevy;
            }
            else if(last_key_is_right_click) {
                tmp.insert(df::interface_key::LEAVESCREEN);
                screen->feed(&tmp);
            }
            else {
                tmp.insert(last_key);
                screen->feed(&tmp);
            }
            set_state(INACTIVE);
        }
        // clean up any artifacts
        df::global::gps->force_full_display_count = 1;
    }
    string get_id() override = 0;
    string get_focus_string() override = 0;
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
    bool intercept_key (std::string mouse_button = "MOUSE_LEFT")
    {
        CONF_LUA_START;
        push(mouse_button);
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
    bool last_key_is_left_click;
    bool last_key_is_right_click;
    df::coord2d mouse_pos;
};

template<typename T>
int conf_register(confirmation<T> *c, const vector<VMethodInterposeLinkBase*> &hooks)
{
    conf_wrapper *w = new conf_wrapper();
    confirmations[c->get_id()] = w;
    for (auto hook : hooks)
        w->add_hook(hook);
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
}; \
IMPLEMENT_VMETHOD_INTERPOSE_PRIO(cls##_hooks, feed, prio); \
IMPLEMENT_VMETHOD_INTERPOSE_PRIO(cls##_hooks, render, prio); \
static int conf_register_##cls = conf_register(&cls##_instance, {\
    &INTERPOSE_HOOK(cls##_hooks, feed), \
    &INTERPOSE_HOOK(cls##_hooks, render), \
});

#define DEFINE_CONFIRMATION(cls, screen, focusString) \
    class confirmation_##cls : public confirmation<df::screen> { \
        virtual string get_id() { static string id = char_replace(#cls, '_', '-'); return id; } \
        virtual string get_focus_string() { return focusString; } \
    }; \
    IMPLEMENT_CONFIRMATION_HOOKS(confirmation_##cls, 0);

/* This section defines stubs for all confirmation dialogs, with methods
    implemented in plugins/lua/confirm.lua.
    IDs (used in the "confirm enable/disable" command, by Lua, and in the docs)
    are obtained by replacing '_' with '-' in the first argument to DEFINE_CONFIRMATION

    The second argument to DEFINE_CONFIRMATION determines the viewscreen that any
    intercepted input will be fed to.

    The third argument to DEFINE_CONFIRMATION determines the focus string that will
    be used to determine if the confirmation should be unpaused. If a confirmation is paused
    and the focus string is no longer found in the current focus, the confirmation will be
    unpaused. Focus strings ending in "*" will use prefix matching e.g. "dwarfmode/Info*" would
    match "dwarfmode/Info/Foo", "dwarfmode/Info/Bar" and so on. All matching is case insensitive.
*/

DEFINE_CONFIRMATION(trade_cancel,         viewscreen_dwarfmodest, "dwarfmode/Trade");
DEFINE_CONFIRMATION(haul_delete_route,    viewscreen_dwarfmodest, "dwarfmode/Hauling");
DEFINE_CONFIRMATION(haul_delete_stop,     viewscreen_dwarfmodest, "dwarfmode/Hauling");
DEFINE_CONFIRMATION(depot_remove,         viewscreen_dwarfmodest, "dwarfmode/ViewSheets/BUILDING");
DEFINE_CONFIRMATION(squad_disband,        viewscreen_dwarfmodest, "dwarfmode/Squads");
DEFINE_CONFIRMATION(order_remove,         viewscreen_dwarfmodest, "dwarfmode/Info/WORK_ORDERS");
DEFINE_CONFIRMATION(zone_remove,          viewscreen_dwarfmodest, "dwarfmode/Zone");
DEFINE_CONFIRMATION(burrow_remove,        viewscreen_dwarfmodest, "dwarfmode/Burrow");
DEFINE_CONFIRMATION(stockpile_remove,     viewscreen_dwarfmodest, "dwarfmode/Some/Stockpile");

// these are more complex to implement
//DEFINE_CONFIRMATION(convict,            viewscreen_dwarfmodest);
//DEFINE_CONFIRMATION(trade,              viewscreen_dwarfmodest);
//DEFINE_CONFIRMATION(trade_seize,        viewscreen_dwarfmodest);
//DEFINE_CONFIRMATION(trade_offer,        viewscreen_dwarfmodest);
//DEFINE_CONFIRMATION(trade_select_all,   viewscreen_dwarfmodest);
//DEFINE_CONFIRMATION(uniform_delete,     viewscreen_dwarfmodest);
//DEFINE_CONFIRMATION(note_delete,        viewscreen_dwarfmodest);
//DEFINE_CONFIRMATION(route_delete,       viewscreen_dwarfmodest);

// locations can't be retired currently
//DEFINE_CONFIRMATION(location_retire,    viewscreen_locationsst);

DFhackCExport command_result plugin_init (color_ostream &out, vector <PluginCommand> &commands)
{
    if (!conf_lua::init(out))
        return CR_FAILURE;
    commands.push_back(PluginCommand(
        "confirm",
        "Add confirmation dialogs for destructive actions.",
        df_confirm));
    return CR_OK;
}

DFhackCExport command_result plugin_enable (color_ostream &out, bool enable)
{
    if (is_enabled != enable)
    {
        for (auto c : confirmations)
        {
            if (!c.second->apply(enable))
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

    for (auto item : confirmations)
    {
        delete item.second;
    }
    confirmations.clear();

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
    for (auto it : confirmations)
    {
        if (it.first == name)
        {
            found = true;
            it.second->apply(state);
        }
    }

    if (state == false)
    {
        // dismiss the confirmation too
        confirmation_base::set_state(name, confirmation_base::INACTIVE);
    }

    return found;
}

bool set_conf_paused (string name, bool pause)
{
    bool found = false;
    for (auto it : confirmations)
    {
        if (it.first == name)
        {
            found = true;
            it.second->set_paused(pause);
        }
    }

    if (pause == true)
    {
        // dismiss the confirmation too
        confirmation_base::set_state(name, confirmation_base::INACTIVE);
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
        for (auto it : confirmations)
            out.print("  %20s: %s\n", it.first.c_str(), it.second->is_enabled() ? "enabled" : "disabled");
        return CR_OK;
    }
    for (string param : parameters)
    {
        if (param == "enable")
            state = true;
        else if (param == "disable")
            state = false;
        else if (param == "all")
        {
            for (auto it : confirmations)
                it.second->apply(state);
        }
        else
            enable_conf(out, param, state);
    }
    return CR_OK;
}
