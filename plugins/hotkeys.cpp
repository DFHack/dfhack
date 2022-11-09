#include <map>
#include <string>
#include <vector>

#include "modules/Gui.h"
#include "modules/Screen.h"

#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"

DFHACK_PLUGIN("hotkeys");

namespace DFHack {
    DBG_DECLARE(hotkeys, log, DebugCategory::LINFO);
}

using std::map;
using std::string;
using std::vector;

using namespace DFHack;

static const string INVOKE_MENU_COMMAND = "overlay trigger hotkeys.menu";
static const std::string MENU_SCREEN_FOCUS_STRING = "dfhack/lua/hotkeys/menu";

static bool valid = false; // whether the following two vars contain valid data
static string current_focus;
static map<string, string> current_bindings;
static vector<string> sorted_keys;

static bool can_invoke(const string &cmdline, df::viewscreen *screen) {
    vector<string> cmd_parts;
    split_string(&cmd_parts, cmdline, " ");
    if (toLower(cmd_parts[0]) == "hotkeys")
        return false;

    return Core::getInstance().getPluginManager()->CanInvokeHotkey(cmd_parts[0], screen);
}

static int cleanupHotkeys(lua_State *) {
    DEBUG(log).print("cleaning up old stub keybindings for %s\n", current_focus.c_str());
    std::for_each(sorted_keys.begin(), sorted_keys.end(), [](const string &sym) {
        string keyspec = sym + "@" + MENU_SCREEN_FOCUS_STRING;
        DEBUG(log).print("clearing keybinding: %s\n", keyspec.c_str());
        Core::getInstance().ClearKeyBindings(keyspec);
    });
    valid = false;
    current_focus = "";
    sorted_keys.clear();
    current_bindings.clear();
    return 0;
}

static void add_binding_if_valid(const string &sym, const string &cmdline, df::viewscreen *screen) {
    if (!can_invoke(cmdline, screen))
        return;

    if (cmdline == INVOKE_MENU_COMMAND) {
        DEBUG(log).print("filtering out hotkey menu keybinding\n");
        return;
    }

    current_bindings[sym] = cmdline;
    sorted_keys.push_back(sym);
    string keyspec = sym + "@" + MENU_SCREEN_FOCUS_STRING;
    string binding = "hotkeys invoke " + int_to_string(sorted_keys.size() - 1);
    DEBUG(log).print("adding keybinding: %s -> %s\n", keyspec.c_str(), binding.c_str());
    Core::getInstance().AddKeyBinding(keyspec, binding);
}

static void find_active_keybindings(df::viewscreen *screen) {
    DEBUG(log).print("scanning for active keybindings\n");
    if (valid)
        cleanupHotkeys(NULL);

    vector<string> valid_keys;

    for (char c = 'A'; c <= 'Z'; c++) {
        valid_keys.push_back(string(&c, 1));
    }

    for (int i = 1; i < 10; i++) {
        valid_keys.push_back("F" + int_to_string(i));
    }

    valid_keys.push_back("`");

    current_focus = Gui::getFocusString(screen);
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
                        if (invoke_cmd->find(":") == string::npos) {
                            add_binding_if_valid(sym, *invoke_cmd, screen);
                        }
                        else {
                            vector<string> tokens;
                            split_string(&tokens, *invoke_cmd, ":");
                            string focus = tokens[0].substr(1);
                            if (prefix_matches(focus, current_focus)) {
                                auto cmdline = trim(tokens[1]);
                                add_binding_if_valid(sym, cmdline, screen);
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
    find_active_keybindings(Gui::getCurViewscreen(true));
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
        find_active_keybindings(Gui::getCurViewscreen(true));

    out.print("Valid keybindings for the current screen (%s)\n",
              current_focus.c_str());
    std::for_each(sorted_keys.begin(), sorted_keys.end(), [&](const string &sym) {
        out.print("%s: %s\n", sym.c_str(), current_bindings[sym].c_str());
    });

    if (!was_valid)
        cleanupHotkeys(NULL);
}

static bool invoke_command(color_ostream &out, const size_t index) {
    auto screen = Core::getTopViewscreen();
    if (sorted_keys.size() <= index ||
            Gui::getFocusString(screen) != MENU_SCREEN_FOCUS_STRING)
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
        DEBUG(log).print("invoking command: '%s'\n", INVOKE_MENU_COMMAND.c_str());
        return Core::getInstance().runCommand(out, INVOKE_MENU_COMMAND );
    }

    if (parameters[0] == "list") {
        list(out);
        return CR_OK;
    }

    if (parameters.size() != 2 || parameters[0] != "invoke")
        return CR_WRONG_USAGE;

    CoreSuspender guard;

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
