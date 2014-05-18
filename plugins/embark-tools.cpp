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
};

EmbarkTool embark_tools[] = {
    {"anywhere", "Embark anywhere", "Allows embarking anywhere on the world map", false},
    {"nano", "Nano embark", "Allows the embark size to be decreased below 2x2", false},
};
#define NUM_TOOLS sizeof(embark_tools) / sizeof(EmbarkTool)

command_result embark_tools_cmd (color_ostream &out, std::vector <std::string> & parameters);

bool tool_exists (std::string tool_name);
bool tool_enabled (std::string tool_name);
bool tool_enable (std::string tool_name, bool enable_state);

/*
 * Logic
 */

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
        df::viewscreen * top = Gui::getCurViewscreen();
        VIRTUAL_CAST_VAR(screen, df::viewscreen_choose_start_sitest, top);
        if (tool_enabled("anywhere"))
        {
            for (auto iter = input->begin(); iter != input->end(); iter++)
            {
                df::interface_key key = *iter;
                if (key == df::interface_key::SETUP_EMBARK)
                {
                    prevent_default = true;
                    screen->in_embark_normal = 1;
                }
            }
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
                if (is_resize) {
                    prevent_default = true;
                    resize_embark(screen, dx, dy);
                }
            }
        }
        if (!prevent_default)
            INTERPOSE_NEXT(feed)(input);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();

        df::viewscreen * top = Gui::getCurViewscreen();
        VIRTUAL_CAST_VAR(screen, df::viewscreen_choose_start_sitest, top);

        auto dim = Screen::getWindowSize();
        int x = 1,
            y = dim.y - 5;
        OutputString(COLOR_LIGHTMAGENTA, x, y, "Enabled: ");
        std::list<std::string> tools;
        for (int i = 0; i < NUM_TOOLS; i++)
        {
            if (embark_tools[i].enabled)
            {
                tools.push_back(embark_tools[i].name);
                tools.push_back(", ");
            }
        }
        if (tools.size())
        {
            tools.pop_back();  // Remove last ,
            for (auto iter = tools.begin(); iter != tools.end(); iter++)
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
            if (screen->page == 0)
            {
                // Only display on main page (not site finder, notes, etc.)
                OutputString(COLOR_WHITE, x, y, ": Embark!");
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
    for (int i = 0; i < NUM_TOOLS; i++)
    {
        if (embark_tools[i].id == tool_name)
        {
            embark_tools[i].enabled = enable_state;
            return true;
        }
    }
    return false;
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
            else if (tool_exists(parameters[i]))
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
