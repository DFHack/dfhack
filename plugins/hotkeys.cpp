#include <map>
#include <string>
#include <vector>

#include "modules/Gui.h"
#include "modules/Screen.h"

#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "PluginLua.h"

DFHACK_PLUGIN("hotkeys");

namespace DFHack {
    DBG_DECLARE(hotkeys, log, DebugCategory::LINFO);
}

using std::map;
using std::string;
using std::vector;

using namespace DFHack;

static const string INVOKE_MENU_BASE_COMMAND = "overlay trigger ";
static const string INVOKE_MENU_PREFIX = INVOKE_MENU_BASE_COMMAND + "hotkeys.";
static const string INVOKE_MENU_DEFAULT_COMMAND = INVOKE_MENU_BASE_COMMAND + "hotkeys.menu";
static const string INVOKE_HOTKEYS_COMMAND = "hotkeys";
static const std::string MENU_SCREEN_FOCUS_STRING = "dfhack/lua/hotkeys/menu";

static bool valid = false; // whether the following two vars contain valid data
static map<string, string> current_bindings;
static vector<string> sorted_keys;

static bool can_invoke(const string &cmdline, df::viewscreen *screen) {
    vector<string> cmd_parts;
    split_string(&cmd_parts, cmdline, " ");

    return Core::getInstance().getPluginManager()->CanInvokeHotkey(cmd_parts[0], screen);
}

static int cleanupHotkeys(lua_State *) {
    DEBUG(log).print("cleaning up old stub keybindings for: %s\n", join_strings(", ", Gui::getCurFocus(true)).c_str());
    std::for_each(sorted_keys.begin(), sorted_keys.end(), [](const string &sym) {
        string keyspec = sym + "@" + MENU_SCREEN_FOCUS_STRING;
        DEBUG(log).print("clearing keybinding: %s\n", keyspec.c_str());
        Core::getInstance().ClearKeyBindings(keyspec);
    });
    valid = false;
    sorted_keys.clear();
    current_bindings.clear();
    return 0;
}

static bool should_hide_armok(color_ostream &out, const string &cmdline) {
    bool should_hide = false;

    Lua::CallLuaModuleFunction(out, "plugins.hotkeys", "should_hide_armok", std::make_tuple(cmdline),
        1, [&](lua_State *L){
            should_hide = lua_toboolean(L, -1);
        });

    return should_hide;
}

static void add_binding_if_valid(color_ostream &out, const string &sym, const string &cmdline, df::viewscreen *screen, bool filtermenu) {
    if (!can_invoke(cmdline, screen))
        return;

    if (filtermenu && (cmdline.starts_with(INVOKE_MENU_PREFIX) ||
                       cmdline == INVOKE_HOTKEYS_COMMAND)) {
        DEBUG(log).print("filtering out hotkey menu keybinding\n");
        return;
    }

    if (should_hide_armok(out, cmdline)) {
        DEBUG(log).print("filtering out armok keybinding\n");
        return;
    }

    current_bindings[sym] = cmdline;
    sorted_keys.push_back(sym);
    string keyspec = sym + "@" + MENU_SCREEN_FOCUS_STRING;
    string binding = "hotkeys invoke " + int_to_string(sorted_keys.size() - 1);
    DEBUG(log).print("adding keybinding: %s -> %s\n", keyspec.c_str(), binding.c_str());
    Core::getInstance().AddKeyBinding(keyspec, binding);
}

static void find_active_keybindings(color_ostream &out, df::viewscreen *screen, bool filtermenu) {
    DEBUG(log).print("scanning for active keybindings\n");
    if (valid)
        cleanupHotkeys(NULL);

    vector<string> valid_keys;

    for (char c = '0'; c <= '9'; c++) {
        valid_keys.push_back(string(&c, 1));
    }

    for (char c = 'A'; c <= 'Z'; c++) {
        valid_keys.push_back(string(&c, 1));
    }

    for (int i = 1; i <= 12; i++) {
        valid_keys.push_back('F' + int_to_string(i));
    }

    valid_keys.push_back("`");

    for (int shifted = 0; shifted < 2; shifted++) {
        for (int alt = 0; alt < 2; alt++) {
            for (int ctrl = 0; ctrl < 2; ctrl++) {
                for (auto it = valid_keys.begin(); it != valid_keys.end(); it++) {
                    string sym;
                    if (ctrl) sym += "Ctrl-";
                    if (alt) sym += "Alt-";
                    if (shifted) sym += "Shift-";
                    sym += *it;

                    auto list = Core::getInstance().ListKeyBindings(sym);
                    for (auto invoke_cmd = list.begin(); invoke_cmd != list.end(); invoke_cmd++) {
                        string::size_type colon_pos = invoke_cmd->find(":");
                        // colons at location 0 are for commands like ":lua"
                        if (colon_pos == string::npos || colon_pos == 0) {
                            add_binding_if_valid(out, sym, *invoke_cmd, screen, filtermenu);
                        }
                        else {
                            vector<string> tokens;
                            split_string(&tokens, *invoke_cmd, ":");
                            string focus = tokens[0].substr(1);
                            if(Gui::matchFocusString(focus)) {
                                auto cmdline = trim(tokens[1]);
                                add_binding_if_valid(out, sym, cmdline, screen, filtermenu);
                            }
                        }
                    }
                }
            }
        }
    }

    valid = true;
}

static int getHotkeys(lua_State *L) {
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    find_active_keybindings(*out, Gui::getCurViewscreen(true), true);
    Lua::PushVector(L, sorted_keys);
    Lua::Push(L, current_bindings);
    return 2;
}

DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(getHotkeys),
    DFHACK_LUA_COMMAND(cleanupHotkeys),
    DFHACK_LUA_END
};

static void list(color_ostream &out) {
    DEBUG(log).print("listing active hotkeys\n");
    bool was_valid = valid;
    if (!valid)
        find_active_keybindings(out, Gui::getCurViewscreen(true), false);

    out.print("Valid keybindings for the current focus:\n %s\n",
              join_strings("\n", Gui::getCurFocus(true)).c_str());
    std::for_each(sorted_keys.begin(), sorted_keys.end(), [&](const string &sym) {
        out.print("%s: %s\n", sym.c_str(), current_bindings[sym].c_str());
    });

    if (!was_valid)
        cleanupHotkeys(NULL);
}

static bool invoke_command(color_ostream &out, const size_t index) {
    auto screen = Core::getTopViewscreen();
    if (sorted_keys.size() <= index ||
            !Gui::matchFocusString(MENU_SCREEN_FOCUS_STRING))
        return false;

    auto cmd = current_bindings[sorted_keys[index]];
    DEBUG(log).print("invoking command: '%s'\n", cmd.c_str());

    {
        Screen::Hide hideGuard(screen, Screen::Hide::RESTORE_AT_TOP);
        Core::getInstance().runCommand(out, cmd);
    }

    Screen::dismiss(screen);
    return true;
}

static command_result hotkeys_cmd(color_ostream &out, vector <string> & parameters) {
    if (!parameters.size()) {
        DEBUG(log).print("invoking command: '%s'\n", INVOKE_MENU_DEFAULT_COMMAND.c_str());
        return Core::getInstance().runCommand(out, INVOKE_MENU_DEFAULT_COMMAND );
    } else if (parameters.size() == 2 && parameters[0] == "menu") {
        string cmd = INVOKE_MENU_BASE_COMMAND + parameters[1];
        DEBUG(log).print("invoking command: '%s'\n", cmd.c_str());
        return Core::getInstance().runCommand(out, cmd);
    }

    if (parameters[0] == "list") {
        list(out);
        return CR_OK;
    }

    // internal command -- intentionally undocumented
    if (parameters.size() != 2 || parameters[0] != "invoke")
        return CR_WRONG_USAGE;

    int index = string_to_int(parameters[1], -1);
    if (index < 0)
        return CR_WRONG_USAGE;
    return invoke_command(out, index) ? CR_OK : CR_WRONG_USAGE;
}

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands) {
    commands.push_back(
        PluginCommand(
        "hotkeys",
        "Invoke hotkeys from the interactive menu.",
        hotkeys_cmd,
        Gui::anywhere_hotkey));

    return CR_OK;
}
