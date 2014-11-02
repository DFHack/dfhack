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
#include "uicommon.h"
#include "df/viewscreen_choose_start_sitest.h"
#include "df/interface_key.h"

using namespace DFHack;

#define FOR_ITER_TOOLS(iter) for(auto iter = tools.begin(); iter != tools.end(); iter++)

void update_embark_sidebar (df::viewscreen_choose_start_sitest * screen)
{
    bool is_top = false;
    if (screen->location.embark_pos_min.y == 0)
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
    int x1 = screen->location.embark_pos_min.x,
        x2 = screen->location.embark_pos_max.x,
        y1 = screen->location.embark_pos_min.y,
        y2 = screen->location.embark_pos_max.y,
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

    screen->location.embark_pos_min.x = x1;
    screen->location.embark_pos_max.x = x2;
    screen->location.embark_pos_min.y = y1;
    screen->location.embark_pos_max.y = y2;

    update_embark_sidebar(screen);
}

typedef df::viewscreen_choose_start_sitest start_sitest;
typedef std::set<df::interface_key> ikey_set;

class EmbarkTool
{
protected:
    bool enabled;
public:
    EmbarkTool()
        :enabled(false)
    { }
    virtual bool getEnabled() { return enabled; }
    virtual void setEnabled(bool state) { enabled = state; }
    virtual void toggleEnabled() { setEnabled(!enabled); }
    virtual std::string getId() = 0;
    virtual std::string getName() = 0;
    virtual std::string getDesc() = 0;
    virtual df::interface_key getToggleKey() = 0;
    virtual void before_render(start_sitest* screen) { };
    virtual void after_render(start_sitest* screen) { };
    virtual void before_feed(start_sitest* screen, ikey_set* input, bool &cancel) { };
    virtual void after_feed(start_sitest* screen, ikey_set* input) { };
};
std::vector<EmbarkTool*> tools;

/*

class SampleTool : public EmbarkTool
{
    virtual std::string getId() { return "id"; }
    virtual std::string getName() { return "Name"; }
    virtual std::string getDesc() { return "Description"; }
    virtual df::interface_key getToggleKey() { return df::interface_key::KEY; }
    virtual void before_render(start_sitest* screen) { }
    virtual void after_render(start_sitest* screen) { }
    virtual void before_feed(start_sitest* screen, ikey_set* input, bool &cancel) { };
    virtual void after_feed(start_sitest* screen, ikey_set* input) { };
};

*/

class EmbarkAnywhere : public EmbarkTool
{
public:
    virtual std::string getId() { return "anywhere"; }
    virtual std::string getName() { return "Embark anywhere"; }
    virtual std::string getDesc() { return "Allows embarking anywhere on the world map"; }
    virtual df::interface_key getToggleKey() { return df::interface_key::CUSTOM_A; }
    virtual void after_render(start_sitest* screen)
    {
        auto dim = Screen::getWindowSize();
        int x = 20, y = dim.y - 2;
        if (screen->page >= 0 && screen->page <= 4)
        {
            OutputString(COLOR_WHITE, x, y, ": Embark!");
        }
    }
    virtual void before_feed(start_sitest* screen, ikey_set *input, bool &cancel)
    {
        if (input->count(df::interface_key::SETUP_EMBARK))
        {
            cancel = true;
            screen->in_embark_normal = 1;
        }
    };
};

class NanoEmbark : public EmbarkTool
{
public:
    virtual std::string getId() { return "nano"; }
    virtual std::string getName() { return "Nano embark"; }
    virtual std::string getDesc() { return "Allows the embark size to be decreased below 2x2"; }
    virtual df::interface_key getToggleKey() { return df::interface_key::CUSTOM_N; }
    virtual void before_feed(start_sitest* screen, ikey_set* input, bool &cancel)
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
                cancel = true;
                resize_embark(screen, dx, dy);
                return;
            }
        }
    };
};

class SandIndicator : public EmbarkTool
{
protected:
    bool dirty;
    std::string indicator;
    void update_indicator()
    {
        CoreSuspendClaimer suspend;
        buffered_color_ostream out;
        Core::getInstance().runCommand(out, "prospect");
        auto fragments = out.fragments();
        indicator = "";
        for (auto iter = fragments.begin(); iter != fragments.end(); iter++)
        {
            std::string fragment = iter->second;
            if (fragment.find("SAND_") != std::string::npos)
            {
                indicator = "Sand";
                break;
            }
        }
        dirty = false;
    }
public:
    SandIndicator()
        :EmbarkTool(),
        dirty(true),
        indicator("")
    { }
    virtual void setEnabled(bool state)
    {
        EmbarkTool::setEnabled(state);
        dirty = true;
    }
    virtual std::string getId() { return "sand"; }
    virtual std::string getName() { return "Sand indicator"; }
    virtual std::string getDesc() { return "Displays an indicator when sand is present on the given embark site"; }
    virtual df::interface_key getToggleKey() { return df::interface_key::CUSTOM_S; }
    virtual void after_render(start_sitest* screen)
    {
        if (dirty)
            update_indicator();
        auto dim = Screen::getWindowSize();
        int x = dim.x - 28,
            y = 13;
        if (screen->page == 0)
        {
            OutputString(COLOR_YELLOW, x, y, indicator);
        }
    }
    virtual void after_feed(start_sitest* screen, ikey_set* input)
    {
        dirty = true;
    };
};

class StablePosition : public EmbarkTool
{
protected:
    int prev_position[4];
    bool moved_position;
    void save_position(start_sitest* screen)
    {
        prev_position[0] = screen->location.embark_pos_min.x;
        prev_position[1] = screen->location.embark_pos_max.x;
        prev_position[2] = screen->location.embark_pos_min.y;
        prev_position[3] = screen->location.embark_pos_max.y;
    }
    void restore_position(start_sitest* screen)
    {
        if (screen->finder.finder_state != -1)
        {
            // Site finder is active - don't override default local position
            return;
        }
        screen->location.embark_pos_min.x = prev_position[0];
        screen->location.embark_pos_max.x = prev_position[1];
        screen->location.embark_pos_min.y = prev_position[2];
        screen->location.embark_pos_max.y = prev_position[3];
        update_embark_sidebar(screen);
    }
public:
    StablePosition()
        :EmbarkTool(),
        moved_position(false)
    {
        prev_position[0] = 0;
        prev_position[1] = 0;
        prev_position[2] = 3;
        prev_position[3] = 3;
    }
    virtual std::string getId() { return "sticky"; }
    virtual std::string getName() { return "Stable position"; }
    virtual std::string getDesc() { return "Maintains the selected local area while navigating the world map"; }
    virtual df::interface_key getToggleKey() { return df::interface_key::CUSTOM_P; }
    virtual void before_render(start_sitest* screen) {
        if (moved_position)
        {
            restore_position(screen);
            moved_position = false;
        }
    }
    virtual void before_feed(start_sitest* screen, ikey_set* input, bool &cancel) {
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
            }
            if (is_motion && !moved_position)
            {
                save_position(screen);
                moved_position = true;
            }
        }
    };
};

class embark_tools_settings : public dfhack_viewscreen
{
public:
    embark_tools_settings () { };
    ~embark_tools_settings () { };
    void help () { };
    std::string getFocusString () { return "embark-tools/options"; };
    void render ()
    {
        parent->render();
        int x, y;
        auto dim = Screen::getWindowSize();
        int width = 50,
            height = 4 + 1 + tools.size(),  // Padding + lower row
            min_x = (dim.x - width) / 2,
            max_x = (dim.x + width) / 2,
            min_y = (dim.y - height) / 2,
            max_y = min_y + height;
        Screen::fillRect(Screen::Pen(' ', COLOR_BLACK, COLOR_DARKGREY), min_x, min_y, max_x, max_y);
        Screen::fillRect(Screen::Pen(' ', COLOR_BLACK, COLOR_BLACK), min_x + 1, min_y + 1, max_x - 1, max_y - 1);
        x = min_x + 2;
        y = max_y - 2;
        OutputString(COLOR_LIGHTRED, x, y, Screen::getKeyDisplay(df::interface_key::SELECT));
        OutputString(COLOR_WHITE, x, y, "/");
        OutputString(COLOR_LIGHTRED, x, y, Screen::getKeyDisplay(df::interface_key::LEAVESCREEN));
        OutputString(COLOR_WHITE, x, y, ": Done");
        y = min_y + 2;
        FOR_ITER_TOOLS(iter)
        {
            EmbarkTool* t = *iter;
            x = min_x + 2;
            OutputString(COLOR_LIGHTRED, x, y, Screen::getKeyDisplay(t->getToggleKey()));
            OutputString(COLOR_WHITE, x, y, ": " + t->getName() +
                         (t->getEnabled() ? ": Enabled" : ": Disabled"));
            y++;
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
            FOR_ITER_TOOLS(iter)
            {
                EmbarkTool* t = *iter;
                if (t->getToggleKey() == key)
                {
                    t->toggleEnabled();
                }
            }
        }
    };
};

bool tool_exists (std::string tool_name)
{
    FOR_ITER_TOOLS(iter)
    {
        EmbarkTool* tool = *iter;
        if (tool->getId() == tool_name)
            return true;
    }
    return false;
}

bool tool_enabled (std::string tool_name)
{
    FOR_ITER_TOOLS(iter)
    {
        EmbarkTool* tool = *iter;
        if (tool->getId() == tool_name)
            return tool->getEnabled();
    }
    return false;
}

bool tool_enable (std::string tool_name, bool enable_state)
{
    int n = 0;
    FOR_ITER_TOOLS(iter)
    {
        EmbarkTool* tool = *iter;
        if (tool->getId() == tool_name || tool_name == "all")
        {
            tool->setEnabled(enable_state);
            n++;
        }
    }
    return (bool)n;
}

struct choose_start_site_hook : df::viewscreen_choose_start_sitest
{
    typedef df::viewscreen_choose_start_sitest interpose_base;

    void display_tool_status()
    {
        auto dim = Screen::getWindowSize();
        int x = 1,
            y = dim.y - 5;
        OutputString(COLOR_LIGHTRED, x, y, Screen::getKeyDisplay(df::interface_key::CUSTOM_S));
        OutputString(COLOR_WHITE, x, y, ": Enabled: ");
        std::list<std::string> parts;
        FOR_ITER_TOOLS(iter)
        {
            EmbarkTool* tool = *iter;
            if (tool->getEnabled())
            {
                parts.push_back(tool->getName());
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
    }

    void display_settings()
    {
        Screen::show(new embark_tools_settings);
    }

    inline bool is_valid_page()
    {
        return (page >= 0 && page <= 4);
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key> *input))
    {
        bool cancel = false;
        FOR_ITER_TOOLS(iter)
        {
            EmbarkTool* tool = *iter;
            if (tool->getEnabled())
                tool->before_feed(this, input, cancel);
        }
        if (cancel)
            return;
        INTERPOSE_NEXT(feed)(input);
        if (input->count(df::interface_key::CUSTOM_S) && is_valid_page())
            display_settings();
        FOR_ITER_TOOLS(iter)
        {
            EmbarkTool* tool = *iter;
            if (tool->getEnabled())
                tool->after_feed(this, input);
        }
    }
    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        FOR_ITER_TOOLS(iter)
        {
            EmbarkTool* tool = *iter;
            if (tool->getEnabled())
                tool->before_render(this);
        }
        INTERPOSE_NEXT(render)();
        display_tool_status();
        FOR_ITER_TOOLS(iter)
        {
            EmbarkTool* tool = *iter;
            if (tool->getEnabled())
                tool->after_render(this);
        }
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(choose_start_site_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(choose_start_site_hook, render);

DFHACK_PLUGIN("embark-tools");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

command_result embark_tools_cmd (color_ostream &out, std::vector <std::string> & parameters);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    tools.push_back(new EmbarkAnywhere);
    tools.push_back(new NanoEmbark);
    tools.push_back(new SandIndicator);
    tools.push_back(new StablePosition);
    std::string help = "";
    help += "embark-tools (enable/disable) tool [tool...]\n"
            "Tools:\n";
    FOR_ITER_TOOLS(iter)
    {
        help += ("  " + (*iter)->getId() + ": " + (*iter)->getDesc() + "\n");
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
            FOR_ITER_TOOLS(iter)
            {
                EmbarkTool* t = *iter;
                out << t->getName() << " (" << t->getId() << "): "
                    << (t->getEnabled() ? "Enabled" : "Disabled") << std::endl;
            }
        }
        else
        {
            out << "Plugin not enabled" << std::endl;
        }
    }
    return CR_OK;
}
