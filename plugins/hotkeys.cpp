#include "uicommon.h"
#include "listcolumn.h"

#include "df/viewscreen_dwarfmodest.h"
#include "df/ui.h"

#include "modules/Maps.h"
#include "modules/World.h"
#include "modules/Gui.h"

#include "PluginManager.h"

DFHACK_PLUGIN("hotkeys");
#define PLUGIN_VERSION 0.1

static map<string, string> current_bindings;
static vector<string> sorted_keys;
static bool show_usage = false;

static void send_key(const df::interface_key &key)
{
    set< df::interface_key > keys;
    keys.insert(key);
    Gui::getCurViewscreen(true)->feed(&keys);
}

static bool can_invoke(string cmdline, df::viewscreen *screen)
{
    vector<string> cmd_parts;
    split_string(&cmd_parts, cmdline, " ");
    if (toLower(cmd_parts[0]) == "hotkeys")
        return false;

    return Core::getInstance().getPluginManager()->CanInvokeHotkey(cmd_parts[0], screen);
}

static void add_binding_if_valid(string sym, string cmdline, df::viewscreen *screen)
{
    if (!can_invoke(cmdline, screen))
        return;

    current_bindings[sym] = cmdline;
    sorted_keys.push_back(sym);
    string keyspec = sym + "@dfhack/viewscreen_hotkeys";
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
                        bool add_temp_binding = false;
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
    if (Gui::getFocusString(screen) != "dfhack/viewscreen_hotkeys")
        return false;

    Screen::dismiss(Core::getTopViewscreen());
    for_each_(sorted_keys, [] (const string &sym)
        { Core::getInstance().ClearKeyBindings(sym + "@dfhack/viewscreen_hotkeys"); });
    sorted_keys.clear();
    return true;
}


static void invoke_command(const int index)
{
    if (sorted_keys.size() <= index)
        return;

    auto cmd = current_bindings[sorted_keys[index]];
    if (close_hotkeys_screen())
    {
        Core::getInstance().setHotkeyCmd(cmd);
    }
}

class ViewscreenHotkeys : public dfhack_viewscreen
{
public:
    ViewscreenHotkeys(df::viewscreen *top_screen) : top_screen(top_screen)
    {
        hotkeys_column.multiselect = false;
        hotkeys_column.auto_select = true;
        hotkeys_column.setTitle("Key Binding");
        hotkeys_column.bottom_margin = 4;
        hotkeys_column.allow_search = false;

        focus = Gui::getFocusString(top_screen);
        populateColumns();
    }

    void populateColumns()
    {
        hotkeys_column.clear();

        int max_key_length = 0;
        for_each_(sorted_keys, [&] (const string &sym)
        { if (sym.length() > max_key_length) { max_key_length = sym.length(); } });
        int padding = max_key_length + 2;

        for (int i = 0; i < sorted_keys.size(); i++)
        {
            string text = pad_string(sorted_keys[i], padding, false);
            text += current_bindings[sorted_keys[i]];
            hotkeys_column.add(text, i+1);

        }

        help_start = hotkeys_column.fixWidth() + 2;
        hotkeys_column.filterDisplay();
    }

    void feed(set<df::interface_key> *input)
    {
        if (hotkeys_column.feed(input))
            return;

        if (input->count(interface_key::LEAVESCREEN))
        {
            close_hotkeys_screen();
        }
        else if (input->count(interface_key::SELECT))
        {
            invoke_command(hotkeys_column.highlighted_index);
        }
        else if (input->count(interface_key::CUSTOM_U))
        {
            show_usage = !show_usage;
        }
    }

    void render()
    {
        if (Screen::isDismissed(this))
            return;

        dfhack_viewscreen::render();

        Screen::clear();
        Screen::drawBorder("  Hotkeys  ");

        hotkeys_column.display(true);

        int32_t y = gps->dimy - 3;
        int32_t x = 2;
        OutputHotkeyString(x, y, "Leave", "Esc");

        x += 3;
        OutputHotkeyString(x, y, "Invoke", "Enter or Hotkey");

        x += 3;
        OutputToggleString(x, y, "Show Usage", "u", show_usage);

        y = gps->dimy - 4;
        x = 2;
        OutputHotkeyString(x, y, focus.c_str(), "Context", false, help_start, COLOR_WHITE, COLOR_BROWN);

        if (sorted_keys.size() == 0)
            return;

        y = 2;
        x = help_start;

        auto width = gps->dimx - help_start - 2;
        vector <string> parts;
        Core::cheap_tokenise(current_bindings[sorted_keys[hotkeys_column.highlighted_index]], parts);
        if(parts.size() == 0)
            return;

        string first = parts[0];
        parts.erase(parts.begin());

        if (first[0] == '#')
            return;

        Plugin *plugin = Core::getInstance().getPluginManager()->getPluginByCommand(first);
        if (plugin)
        {
            for (auto i = 0; i < plugin->size(); i++)
            {
                auto pc = plugin->operator[](i);
                if (pc.name == first)
                {
                    OutputString(COLOR_BROWN, x, y, "Help", true, help_start);
                    vector <string> lines;
                    string help_text = pc.description;
                    if (show_usage)
                        help_text += "\n\n" + pc.usage;

                    split_string(&lines, help_text, "\n");
                    for (auto it = lines.begin(); it != lines.end() && y < gps->dimy - 4; it++)
                    {
                        auto wrapped_lines = wrapString(*it, width);
                        for (auto wit = wrapped_lines.begin(); wit != wrapped_lines.end() && y < gps->dimy - 4; wit++)
                        {
                            OutputString(COLOR_WHITE, x, y, *wit, true, help_start);
                        }
                    }
                    break;
                }
            }
        }
    }

    virtual std::string getFocusString()
    {
        return "viewscreen_hotkeys";
    }

private:
    ListColumn<int> hotkeys_column;
    df::viewscreen *top_screen;
    string focus;

    int32_t help_start;

    void resize(int32_t x, int32_t y)
    {
        dfhack_viewscreen::resize(x, y);
        hotkeys_column.resize();
    }

    static vector<string> wrapString(string str, int width)
    {
        vector<string> result;
        string excess;
        if (str.length() > width)
        {
            auto cut_space = str.rfind(' ', width-1);
            int excess_start;
            if (cut_space == string::npos)
            {
                cut_space = width-1;
                excess_start = cut_space;
            }
            else
            {
                excess_start = cut_space + 1;
            }

            string line = str.substr(0, cut_space);
            excess = str.substr(excess_start);
            result.push_back(line);
            auto excess_lines = wrapString(excess, width);
            result.insert(result.end(), excess_lines.begin(), excess_lines.end());
        }
        else
        {
            result.push_back(str);
        }

        return result;
    }
};


static command_result hotkeys_cmd(color_ostream &out, vector <string> & parameters)
{
    bool show_help = false;
    if (parameters.empty())
    {
        if (Maps::IsValid())
        {
            auto top_screen = Core::getTopViewscreen();
            if (Gui::getFocusString(top_screen) != "dfhack/viewscreen_hotkeys")
            {
                find_active_keybindings(top_screen);
                Screen::show(new ViewscreenHotkeys(top_screen));
            }
        }
    }
    else
    {
        auto cmd = parameters[0][0];
        if (cmd == 'v')
        {
            out << "Hotkeys" << endl << "Version: " << PLUGIN_VERSION << endl;
        }
        else if (cmd == 'i')
        {
            int index;
            stringstream index_raw(parameters[1]);
            index_raw >> index;
            invoke_command(index);
        }
        else
        {
            return CR_WRONG_USAGE;
        }
    }

    return CR_OK;
}


DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (!gps)
        out.printerr("Could not insert hotkeys hooks!\n");

    commands.push_back(
        PluginCommand(
        "hotkeys", "Shows ingame viewscreen with all dfhack keybindings active in current mode.",
        hotkeys_cmd, false, ""));

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        sorted_keys.clear();
        break;
    default:
        break;
    }

    return CR_OK;
}
