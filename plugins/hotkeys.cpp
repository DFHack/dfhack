#include "uicommon.h"

#include "df/viewscreen_dwarfmodest.h"
#include "df/ui.h"

#include "modules/Maps.h"
#include "modules/World.h"
#include "modules/Gui.h"

DFHACK_PLUGIN("hotkeys");
#define PLUGIN_VERSION 0.1

static bool display_active = false;
static int skip_frames = 0, page = 0, pageSize = 0, maxPages = 0;
static map<string, string> current_bindings;
static vector<string> sorted_keys;

static bool is_menu_active()
{
    return Gui::dwarfmode_hotkey(Core::getTopViewscreen());
}

static void resize()
{
    auto dims = Gui::getDwarfmodeViewDims();
    int display_height = dims.y2 - 4;
    pageSize = display_height / 3;
    maxPages = current_bindings.size() / pageSize - 1;
}

static void enable_display()
{
    current_bindings.clear();
    sorted_keys.clear();
    display_active = true;
    skip_frames = 2;
    page = 0;
    vector<string> generic_keys;

    auto focus_string = "@" + Gui::getFocusString(Core::getTopViewscreen());
    for (int shifted = 0; shifted < 2; shifted++)
    {
        for (int ctrl = 0; ctrl < 2; ctrl++)
        {
            for (int alt = 0; alt < 2; alt++)
            {
                for (char c = 'A'; c <= 'Z'; c++)
                {
                    string sym;
                    if (shifted) sym += "Shift-";
                    if (ctrl) sym += "Ctrl-";
                    if (alt) sym += "Alt-";
                    sym += c;

                    auto list = Core::getInstance().ListKeyBindings(sym);
                    for (auto mode = list.begin(); mode != list.end(); mode++)
                    {
                        if (mode->find(":") == string::npos)
                        {
                            current_bindings[sym] = *mode;
                            generic_keys.push_back(sym);
                        }
                        else
                        {
                            vector<string> tokens;
                            split_string(&tokens, *mode, ":");
                            if (tokens[0] == focus_string)
                            {
                                current_bindings[sym] = tokens[1];
                                sorted_keys.push_back(sym);
                            }
                        }
                    }
                }
            }
        }
    }
    sorted_keys.insert(sorted_keys.end(), generic_keys.begin(), generic_keys.end());
    resize();
}

struct hotkeys_hook : public df::viewscreen_dwarfmodest
{
    typedef df::viewscreen_dwarfmodest interpose_base;

    bool is_display_active()
    {
        if (!is_menu_active())
            display_active = false;

        return display_active;
    }

    bool handle_input(set<df::interface_key> *input)
    {
        bool result = false;
        if (is_display_active())
        {
            if (input->count(interface_key::LEAVESCREEN_ALL))
            {
                result = true;
            }
            
            if (skip_frames)
            {
                skip_frames = 0;
                result = true;
            }
            else
                display_active = false;
        }

        return result;
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        if (!handle_input(input))
            INTERPOSE_NEXT(feed)(input);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();

        if (!is_display_active())
            return;

        if (skip_frames)
            --skip_frames;
        Screen::Pen pen(' ',COLOR_BLACK);
        auto dims = Gui::getDwarfmodeViewDims();
        int y = dims.y1 + 1;
        int left_margin = dims.menu_x1 + 1;
        int x = left_margin;
        Screen::fillRect(pen, x, y, dims.menu_x2, y + 30);

        OutputString(COLOR_BROWN, x, y, "DFHack Hotkeys", true, left_margin);
        OutputHotkeyString(x, y, "Leave", "Shift-Esc", true, left_margin);
        ++y;

        for (int i = page * pageSize, start = page * pageSize; 
            i < sorted_keys.size() && i - start < pageSize;
            i++)
        {
            string hotkey = sorted_keys[i] + ":";
            string cmd = current_bindings[sorted_keys[i]];
            OutputString(COLOR_LIGHTGREEN, x, y, hotkey.c_str(), true, left_margin);
            OutputString(COLOR_WHITE, x, y, cmd.c_str(), true, left_margin);
            ++y;
        }

        /*auto it = current_bindings.begin();
        for (int i = 1; i < (page * pageSize); i++) { ++it; }
        int i = 0;
        while (it != current_bindings.end() && i < pageSize)
        {
            string cmd = it->second;
            string hotkey = it->first + ":";
            OutputString(COLOR_LIGHTGREEN, x, y, hotkey.c_str(), true, left_margin);
            OutputString(COLOR_WHITE, x, y, cmd.c_str(), true, left_margin);
            ++y;
            ++it;
            ++i;
        }*/

        if (maxPages > 0)
            OutputHotkeyString(x, y, "Previous/Next Page", "/*", true, left_margin);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(hotkeys_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(hotkeys_hook, render);


static command_result hotkeys_cmd(color_ostream &out, vector <string> & parameters)
{
    bool show_help = false;
    if (parameters.empty())
    {
        show_help = true;
    }
    else
    {
        auto cmd = parameters[0][0];
        if (cmd == 'v')
        {
            out << "Hotkeys" << endl << "Version: " << PLUGIN_VERSION << endl;
        }
        else if (cmd == 'h')
        {
            return CR_WRONG_USAGE;
        }
        else
        {
            if (is_menu_active())
                enable_display();
        }
    }

    return CR_OK;
}


DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (!gps || !INTERPOSE_HOOK(hotkeys_hook, feed).apply() || !INTERPOSE_HOOK(hotkeys_hook, render).apply())
        out.printerr("Could not insert hotkeys hooks!\n");

    commands.push_back(
        PluginCommand(
        "hotkeys", "List all keybindings active in current mode",
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
        display_active = false;
        skip_frames = 0;
        page = 0;
        break;
    default:
        break;
    }

    return CR_OK;
}
