#include <map>
#include <string>
#include <vector>

#include "modules/Gui.h"
#include "modules/Screen.h"

#include "LuaTools.h"
#include "PluginManager.h"

DFHACK_PLUGIN("hotkeys");

using std::map;
using std::string;
using std::vector;

using namespace DFHack;

static const std::string MENU_SCREEN_FOCUS_STRING = "dfhack/lua/hotkeys/menu";

static map<string, string> current_bindings;
static vector<string> sorted_keys;

static bool can_invoke(const string &cmdline, df::viewscreen *screen)
{
    vector<string> cmd_parts;
    split_string(&cmd_parts, cmdline, " ");
    if (toLower(cmd_parts[0]) == "hotkeys")
        return false;

    return Core::getInstance().getPluginManager()->CanInvokeHotkey(cmd_parts[0], screen);
}

static void add_binding_if_valid(const string &sym, const string &cmdline, df::viewscreen *screen)
{
    if (!can_invoke(cmdline, screen))
        return;

    current_bindings[sym] = cmdline;
    sorted_keys.push_back(sym);
    string keyspec = sym + "@" + MENU_SCREEN_FOCUS_STRING;
    Core::getInstance().AddKeyBinding(keyspec, "hotkeys invoke " + int_to_string(sorted_keys.size() - 1));
}

static void find_active_keybindings(df::viewscreen *screen)
{
    current_bindings.clear();
    sorted_keys.clear();

    vector<string> valid_keys;

    for (char c = 'A'; c <= 'Z'; c++)
    {
        valid_keys.push_back(string(&c, 1));
    }

    for (int i = 1; i < 10; i++)
    {
        valid_keys.push_back("F" + int_to_string(i));
    }

    valid_keys.push_back("`");

    auto current_focus = Gui::getFocusString(screen);
    for (int shifted = 0; shifted < 2; shifted++)
    {
        for (int ctrl = 0; ctrl < 2; ctrl++)
        {
            for (int alt = 0; alt < 2; alt++)
            {
                for (auto it = valid_keys.begin(); it != valid_keys.end(); it++)
                {
                    string sym;
                    if (shifted) sym += "Shift-";
                    if (ctrl) sym += "Ctrl-";
                    if (alt) sym += "Alt-";
                    sym += *it;

                    auto list = Core::getInstance().ListKeyBindings(sym);
                    for (auto invoke_cmd = list.begin(); invoke_cmd != list.end(); invoke_cmd++)
                    {
                        if (invoke_cmd->find(":") == string::npos)
                        {
                            add_binding_if_valid(sym, *invoke_cmd, screen);
                        }
                        else
                        {
                            vector<string> tokens;
                            split_string(&tokens, *invoke_cmd, ":");
                            string focus = tokens[0].substr(1);
                            if (prefix_matches(focus, current_focus))
                            {
                                auto cmdline = trim(tokens[1]);
                                add_binding_if_valid(sym, cmdline, screen);
                            }
                        }
                    }
                }
            }
        }
    }
}

static bool close_hotkeys_screen()
{
    auto screen = Core::getTopViewscreen();
    if (Gui::getFocusString(screen) != MENU_SCREEN_FOCUS_STRING)
        return false;

    Screen::dismiss(Core::getTopViewscreen());
    std::for_each(sorted_keys.begin(), sorted_keys.end(), [](const string &sym){
        Core::getInstance().ClearKeyBindings(sym + "@" + MENU_SCREEN_FOCUS_STRING);
    });
    sorted_keys.clear();
    return true;
}

static bool invoke_command(const size_t index)
{
    if (sorted_keys.size() <= index)
        return false;

    auto cmd = current_bindings[sorted_keys[index]];
    if (close_hotkeys_screen()) {
        Core::getInstance().setHotkeyCmd(cmd);
        return true;
    }
    return false;
}

static command_result hotkeys_cmd(color_ostream &out, vector <string> & parameters)
{
    if (parameters.size() != 2 || parameters[0] != "invoke")
        return CR_WRONG_USAGE;

    int index;
    std::stringstream index_raw(parameters[1]);
    index_raw >> index;
    return invoke_command(index) ? CR_OK : CR_WRONG_USAGE;
}

static int getHotkeys(lua_State *L) {
    find_active_keybindings(Gui::getCurViewscreen(true));
    Lua::PushVector(L, sorted_keys);
    Lua::Push(L, current_bindings);
    return 2;
}

DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(getHotkeys),
    DFHACK_LUA_END
};


DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(
        PluginCommand(
        "hotkeys",
        "Invoke hotkeys from the interactive menu.",
        hotkeys_cmd));

    return CR_OK;
}
