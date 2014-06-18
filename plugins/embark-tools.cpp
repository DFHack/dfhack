#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/Screen.h"
#include "modules/Gui.h"
#include <algorithm>
#include <set>

#include <VTableInterpose.h>
#include "ColorText.h"
#include "df/viewscreen_choose_start_sitest.h"
#include "df/interface_key.h"

using namespace DFHack;

struct EmbarkTool
{
    std::string id;
    std::string name;
    std::string desc;
    bool enabled;
    df::interface_key toggle_key;
};

EmbarkTool embark_tools[] = {
    {"anywhere", "Embark anywhere", "Allows embarking anywhere on the world map",
        false, df::interface_key::CUSTOM_A},
    {"nano", "Nano embark", "Allows the embark size to be decreased below 2x2",
        false, df::interface_key::CUSTOM_N},
    {"sand", "Sand indicator", "Displays an indicator when sand is present on the given embark site",
        false, df::interface_key::CUSTOM_S},
    {"sticky", "Stable position", "Maintains the selected local area while navigating the world map",
        false, df::interface_key::CUSTOM_P},
};
#define NUM_TOOLS int(sizeof(embark_tools) / sizeof(EmbarkTool))

command_result embark_tools_cmd (color_ostream &out, std::vector <std::string> & parameters);

void OutputString (int8_t color, int &x, int y, const std::string &text);

bool tool_exists (std::string tool_name);
bool tool_enabled (std::string tool_name);
bool tool_enable (std::string tool_name, bool enable_state);
void tool_update (std::string tool_name);

class embark_tools_settings : public dfhack_viewscreen
{
public:
    embark_tools_settings () { };
    ~embark_tools_settings () { };
    void help () { };
    std::string getFocusString () { return "embark-tools/options"; };
    void render ()
    {
        int x;
        auto dim = Screen::getWindowSize();
        int width = 50,
            height = 4 + 1 + NUM_TOOLS,  // Padding + lower row
            min_x = (dim.x - width) / 2,
            max_x = (dim.x + width) / 2,
            min_y = (dim.y - height) / 2,
            max_y = min_y + height;
        Screen::fillRect(Screen::Pen(' ', COLOR_BLACK, COLOR_DARKGREY), min_x, min_y, max_x, max_y);
        Screen::fillRect(Screen::Pen(' ', COLOR_BLACK, COLOR_BLACK), min_x + 1, min_y + 1, max_x - 1, max_y - 1);
        x = min_x + 2;
        OutputString(COLOR_LIGHTRED, x, max_y - 2, Screen::getKeyDisplay(df::interface_key::SELECT));
        OutputString(COLOR_WHITE, x, max_y - 2, "/");
        OutputString(COLOR_LIGHTRED, x, max_y - 2, Screen::getKeyDisplay(df::interface_key::LEAVESCREEN));
        OutputString(COLOR_WHITE, x, max_y - 2, ": Done");
        for (int i = 0, y = min_y + 2; i < NUM_TOOLS; i++, y++)
        {
            EmbarkTool t = embark_tools[i];
            x = min_x + 2;
            OutputString(COLOR_LIGHTRED, x, y, Screen::getKeyDisplay(t.toggle_key));
            OutputString(COLOR_WHITE, x, y, ": " + t.name + (t.enabled ? ": Enabled" : ": Disabled"));
        }
    };
    void feed (std::set<df::interface_key> * input)
    {
        if (input->count(df::interface_key::SELECT) || input->count(df::interface_key::LEAVESCREEN))
        {
            Screen::dismiss(this);
            return;
        }
        for (auto iter = input->begin(); iter != input->end(); iter++)
        {
            df::interface_key key = *iter;
            for (int i = 0; i < NUM_TOOLS; i++)
            {
                if (embark_tools[i].toggle_key == key)
                {
                    embark_tools[i].enabled = !embark_tools[i].enabled;
                }
            }
        }
    };
};

/*
 * Logic
 */

void update_embark_sidebar (df::viewscreen_choose_start_sitest * screen)
{
    bool is_top = false;
    if (screen->embark_pos_min.y == 0)
        is_top = true;
    std::set<df::interface_key> keys;
    keys.insert(df::interface_key::SETUP_LOCAL_Y_MUP);
    screen->feed(&keys);
    if (!is_top)
    {
        keys.insert(df::interface_key::SETUP_LOCAL_Y_MDOWN);
        screen->feed(&keys);
    }
}

void resize_embark (df::viewscreen_choose_start_sitest * screen, int dx, int dy)
{
    /* Reproduces DF's embark resizing functionality
     * Local area resizes up and to the right, unless it's already touching the edge
     */
    int x1 = screen->embark_pos_min.x,
        x2 = screen->embark_pos_max.x,
        y1 = screen->embark_pos_min.y,
        y2 = screen->embark_pos_max.y,
        width = x2 - x1 + dx,
        height = y2 - y1 + dy;
    if (x1 == x2 && dx == -1)
        dx = 0;
    if (y1 == y2 && dy == -1)
        dy = 0;

    x2 += dx;  // Resize right
    while (x2 > 15)
    {
        x2--;
        x1--;
    }
    x1 = std::max(0, x1);

    y1 -= dy;  // Resize up
    while (y1 < 0)
    {
        y1++;
        y2++;
    }
    y2 = std::min(15, y2);

    screen->embark_pos_min.x = x1;
    screen->embark_pos_max.x = x2;
    screen->embark_pos_min.y = y1;
    screen->embark_pos_max.y = y2;

    update_embark_sidebar(screen);
}

std::string sand_indicator = "";
bool sand_dirty = true;  // Flag set when update is needed
void sand_update (df::viewscreen_choose_start_sitest * screen)
{
    CoreSuspendClaimer suspend;
    buffered_color_ostream out;
    Core::getInstance().runCommand(out, "prospect");
    auto fragments = out.fragments();
    sand_indicator = "";
    for (auto iter = fragments.begin(); iter != fragments.end(); iter++)
    {
        std::string fragment = iter->second;
        if (fragment.find("SAND_") != std::string::npos)
        {
            sand_indicator = "Sand";
            break;
        }
    }
    sand_dirty = false;
}

int sticky_pos[] = {0, 0, 3, 3};
bool sticky_moved = false;
void sticky_save (df::viewscreen_choose_start_sitest * screen)
{
    sticky_pos[0] = screen->embark_pos_min.x;
    sticky_pos[1] = screen->embark_pos_max.x;
    sticky_pos[2] = screen->embark_pos_min.y;
    sticky_pos[3] = screen->embark_pos_max.y;
}

void sticky_apply (df::viewscreen_choose_start_sitest * screen)
{
    screen->embark_pos_min.x = sticky_pos[0];
    screen->embark_pos_max.x = sticky_pos[1];
    screen->embark_pos_min.y = sticky_pos[2];
    screen->embark_pos_max.y = sticky_pos[3];
    update_embark_sidebar(screen);
}

/*
 * Viewscreen hooks
 */

void OutputString (int8_t color, int &x, int y, const std::string &text)
{
    Screen::paintString(Screen::Pen(' ', color, 0), x, y, text);
    x += text.length();
}

struct choose_start_site_hook : df::viewscreen_choose_start_sitest
{
    typedef df::viewscreen_choose_start_sitest interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key> *input))
    {
        bool prevent_default = false;
        if (tool_enabled("anywhere"))
        {
            for (auto iter = input->begin(); iter != input->end(); iter++)
            {
                df::interface_key key = *iter;
                if (key == df::interface_key::SETUP_EMBARK)
                {
                    prevent_default = true;
                    this->in_embark_normal = 1;
                }
            }
        }

        if (input->count(df::interface_key::CUSTOM_S))
        {
            Screen::show(new embark_tools_settings);
            return;
        }

        if (tool_enabled("nano"))
        {
            for (auto iter = input->begin(); iter != input->end(); iter++)
            {
                df::interface_key key = *iter;
                bool is_resize = true;
                int dx = 0, dy = 0;
                switch (key)
                {
                    case df::interface_key::SETUP_LOCAL_Y_UP:
                        dy = 1;
                        break;
                    case df::interface_key::SETUP_LOCAL_Y_DOWN:
                        dy = -1;
                        break;
                    case df::interface_key::SETUP_LOCAL_X_UP:
                        dx = 1;
                        break;
                    case df::interface_key::SETUP_LOCAL_X_DOWN:
                        dx = -1;
                        break;
                    default:
                        is_resize = false;
                }
                if (is_resize)
                {
                    prevent_default = true;
                    resize_embark(this, dx, dy);
                }
            }
        }

        if (tool_enabled("sticky"))
        {
            for (auto iter = input->begin(); iter != input->end(); iter++)
            {
                df::interface_key key = *iter;
                bool is_motion = false;
                int dx = 0, dy = 0;
                switch (key)
                {
                    case df::interface_key::CURSOR_UP:
                    case df::interface_key::CURSOR_DOWN:
                    case df::interface_key::CURSOR_LEFT:
                    case df::interface_key::CURSOR_RIGHT:
                    case df::interface_key::CURSOR_UPLEFT:
                    case df::interface_key::CURSOR_UPRIGHT:
                    case df::interface_key::CURSOR_DOWNLEFT:
                    case df::interface_key::CURSOR_DOWNRIGHT:
                    case df::interface_key::CURSOR_UP_FAST:
                    case df::interface_key::CURSOR_DOWN_FAST:
                    case df::interface_key::CURSOR_LEFT_FAST:
                    case df::interface_key::CURSOR_RIGHT_FAST:
                    case df::interface_key::CURSOR_UPLEFT_FAST:
                    case df::interface_key::CURSOR_UPRIGHT_FAST:
                    case df::interface_key::CURSOR_DOWNLEFT_FAST:
                    case df::interface_key::CURSOR_DOWNRIGHT_FAST:
                        is_motion = true;
                        break;
                    default:
                        is_motion = false;
                }
                if (is_motion && !sticky_moved)
                {
                    sticky_save(this);
                    sticky_moved = true;
                }
            }
        }

        if (tool_enabled("sand"))
        {
            sand_dirty = true;
        }
        if (!prevent_default)
            INTERPOSE_NEXT(feed)(input);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        if (tool_enabled("sticky") && sticky_moved)
        {
            sticky_apply(this);
            sticky_moved = false;
        }

        INTERPOSE_NEXT(render)();

        auto dim = Screen::getWindowSize();
        int x = 1,
            y = dim.y - 5;
        OutputString(COLOR_LIGHTRED, x, y, Screen::getKeyDisplay(df::interface_key::CUSTOM_S));
        OutputString(COLOR_WHITE, x, y, ": Enabled: ");
        std::list<std::string> parts;
        for (int i = 0; i < NUM_TOOLS; i++)
        {
            if (embark_tools[i].enabled)
            {
                parts.push_back(embark_tools[i].name);
                parts.push_back(", ");
            }
        }
        if (parts.size())
        {
            parts.pop_back();  // Remove trailing comma
            for (auto iter = parts.begin(); iter != parts.end(); iter++)
            {
                OutputString(COLOR_LIGHTMAGENTA, x, y, *iter);
            }
        }
        else
        {
            OutputString(COLOR_LIGHTMAGENTA, x, y, "(none)");
        }

        if (tool_enabled("anywhere"))
        {
            x = 20; y = dim.y - 2;
            if (this->page >= 0 && this->page <= 4)
            {
                // Only display on five map pages, not on site finder or notes
                OutputString(COLOR_WHITE, x, y, ": Embark!");
            }
        }
        if (tool_enabled("sand"))
        {
            if (sand_dirty)
            {
                sand_update(this);
            }
            x = dim.x - 28; y = 13;
            if (this->page == 0)
            {
                OutputString(COLOR_YELLOW, x, y, sand_indicator);
            }
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(choose_start_site_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(choose_start_site_hook, render);

/*
 * Tool management
 */

bool tool_exists (std::string tool_name)
{
    for (int i = 0; i < NUM_TOOLS; i++)
    {
        if (embark_tools[i].id == tool_name)
            return true;
    }
    return false;
}

bool tool_enabled (std::string tool_name)
{
    for (int i = 0; i < NUM_TOOLS; i++)
    {
        if (embark_tools[i].id == tool_name)
            return embark_tools[i].enabled;
    }
    return false;
}

bool tool_enable (std::string tool_name, bool enable_state)
{
    int n = 0;
    for (int i = 0; i < NUM_TOOLS; i++)
    {
        if (embark_tools[i].id == tool_name || tool_name == "all")
        {
            embark_tools[i].enabled = enable_state;
            tool_update(tool_name);
            n++;
        }
    }
    return (bool)n;
}

void tool_update (std::string tool_name)
{
    // Called whenever a tool is enabled/disabled
    if (tool_name == "sand")
    {
        sand_dirty = true;
    }
}

/*
 * Plugin management
 */

DFHACK_PLUGIN("embark-tools");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    std::string help = "";
    help += "embark-tools (enable/disable) tool [tool...]\n"
            "Tools:\n";
    for (int i = 0; i < NUM_TOOLS; i++)
    {
        help += ("  " + embark_tools[i].id + ": " + embark_tools[i].desc + "\n");
    }
    commands.push_back(PluginCommand(
        "embark-tools",
        "A collection of embark tools",
        embark_tools_cmd,
        false,
        help.c_str()
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    INTERPOSE_HOOK(choose_start_site_hook, feed).remove();
    INTERPOSE_HOOK(choose_start_site_hook, render).remove();
    return CR_OK;
}

DFhackCExport command_result plugin_enable (color_ostream &out, bool enable)
{
    if (is_enabled != enable)
    {
        if (!INTERPOSE_HOOK(choose_start_site_hook, feed).apply(enable) ||
            !INTERPOSE_HOOK(choose_start_site_hook, render).apply(enable))
            return CR_FAILURE;
        is_enabled = enable;
    }
    return CR_OK;
}

command_result embark_tools_cmd (color_ostream &out, std::vector <std::string> & parameters)
{
    CoreSuspender suspend;
    if (parameters.size())
    {
        // Set by "enable"/"disable" - allows for multiple commands, e.g. "enable nano disable anywhere"
        bool enable_state = true;
        for (size_t i = 0; i < parameters.size(); i++)
        {
            if (parameters[i] == "enable")
            {
                enable_state = true;
                plugin_enable(out, true);  // Enable plugin
            }
            else if (parameters[i] == "disable")
                enable_state = false;
            else if (tool_exists(parameters[i]) || parameters[i] == "all")
            {
                tool_enable(parameters[i], enable_state);
            }
            else
                return CR_WRONG_USAGE;
        }
    }
    else
    {
        if (is_enabled)
        {
            out << "Tool status:" << std::endl;
            for (int i = 0; i < NUM_TOOLS; i++)
            {
                EmbarkTool t = embark_tools[i];
                out << t.name << " (" << t.id << "): " << (t.enabled ? "Enabled" : "Disabled") << std::endl;
            }
        }
        else
        {
            out << "Plugin not enabled" << std::endl;
        }
    }
    return CR_OK;
}
