#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"
#include "MiscUtils.h"

#include "modules/Screen.h"
#include "modules/Gui.h"
#include <algorithm>
#include <map>
#include <set>

#include <VTableInterpose.h>
#include "ColorText.h"
#include "uicommon.h"
#include "df/viewscreen_choose_start_sitest.h"
#include "df/interface_key.h"

using namespace DFHack;
using df::global::enabler;
using df::global::gps;

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

void get_embark_pos (df::viewscreen_choose_start_sitest * screen,
                     int& x1, int& x2, int& y1, int& y2, int& w, int& h)
{
    x1 = screen->location.embark_pos_min.x,
    x2 = screen->location.embark_pos_max.x,
    y1 = screen->location.embark_pos_min.y,
    y2 = screen->location.embark_pos_max.y,
    w  = x2 - x1 + 1,
    h  = y2 - y1 + 1;
}

void set_embark_pos (df::viewscreen_choose_start_sitest * screen,
                     int x1, int x2, int y1, int y2)
{
    screen->location.embark_pos_min.x = x1;
    screen->location.embark_pos_max.x = x2;
    screen->location.embark_pos_min.y = y1;
    screen->location.embark_pos_max.y = y2;
}

#define GET_EMBARK_POS(screen, a, b, c, d, e, f) \
    int a, b, c, d, e, f; \
    get_embark_pos(screen, a, b, c, d, e, f);

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
    virtual void after_mouse_event(start_sitest* screen) { };
};
std::map<std::string, EmbarkTool*> tools;

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

class MouseControl : public EmbarkTool
{
protected:
    // Used for event handling
    int prev_x;
    int prev_y;
    bool prev_lbut;
    // Used for controls
    bool base_max_x;
    bool base_max_y;
    bool in_local_move;
    bool in_local_edge_resize_x;
    bool in_local_edge_resize_y;
    bool in_local_corner_resize;
    // These keep track of where the embark location would be (i.e. if
    // the mouse is dragged out of the local embark area), to prevent
    // the mouse from moving the actual area when it's 20+ tiles away
    int local_overshoot_x1;
    int local_overshoot_x2;
    int local_overshoot_y1;
    int local_overshoot_y2;
    inline bool in_local_adjust()
    {
        return in_local_move || in_local_edge_resize_x || in_local_edge_resize_y ||
            in_local_corner_resize;
    }
    void lbut_press(start_sitest* screen, bool pressed, int x, int y)
    {
        GET_EMBARK_POS(screen, x1, x2, y1, y2, width, height);
        in_local_move = in_local_edge_resize_x = in_local_edge_resize_y =
            in_local_corner_resize = false;
        if (pressed)
        {
            if (x >= 1 && x <= 16 && y >= 2 && y <= 17)
            {
                // Local embark - translate to local map coordinates
                x -= 1;
                y -= 2;
                if ((x == x1 || x == x2) && (y == y1 || y == y2))
                {
                    in_local_corner_resize = true;
                    base_max_x = (x == x2);
                    base_max_y = (y == y2);
                }
                else if (x == x1 || x == x2)
                {
                    in_local_edge_resize_x = true;
                    base_max_x = (x == x2);
                    base_max_y = false;
                }
                else if (y == y1 || y == y2)
                {
                    in_local_edge_resize_y = true;
                    base_max_x = false;
                    base_max_y = (y == y2);
                }
                else if (x > x1 && x < x2 && y > y1 && y < y2)
                {
                    in_local_move = true;
                    base_max_x = base_max_y = false;
                    local_overshoot_x1 = x1;
                    local_overshoot_x2 = x2;
                    local_overshoot_y1 = y1;
                    local_overshoot_y2 = y2;
                }
            }
        }
        update_embark_sidebar(screen);
    }
    void mouse_move(start_sitest* screen, int x, int y)
    {
        GET_EMBARK_POS(screen, x1, x2, y1, y2, width, height);
        if (x == -1 && prev_x > (2 + 16))
        {
            x = gps->dimx;
            gps->mouse_x = x - 1;
        }
        if (y == -1 && prev_y > (1 + 16))
        {
            y = gps->dimy;
            gps->mouse_y = y - 1;
        }
        if (in_local_corner_resize || in_local_edge_resize_x || in_local_edge_resize_y)
        {
            x -= 1;
            y -= 2;
        }
        if (in_local_corner_resize)
        {
            x = std::max(0, std::min(15, x));
            y = std::max(0, std::min(15, y));
            if (base_max_x)
                x2 = x;
            else
                x1 = x;
            if (base_max_y)
                y2 = y;
            else
                y1 = y;
            if (x1 > x2)
            {
                std::swap(x1, x2);
                base_max_x = !base_max_x;
            }
            if (y1 > y2)
            {
                std::swap(y1, y2);
                base_max_y = !base_max_y;
            }
        }
        else if (in_local_edge_resize_x)
        {
            x = std::max(0, std::min(15, x));
            if (base_max_x)
                x2 = x;
            else
                x1 = x;
            if (x1 > x2)
            {
                std::swap(x1, x2);
                base_max_x = !base_max_x;
            }
        }
        else if (in_local_edge_resize_y)
        {
            y = std::max(0, std::min(15, y));
            if (base_max_y)
                y2 = y;
            else
                y1 = y;
            if (y1 > y2)
            {
                std::swap(y1, y2);
                base_max_y = !base_max_y;
            }
        }
        else if (in_local_move)
        {
            int dx = x - prev_x;
            int dy = y - prev_y;
            local_overshoot_x1 += dx;
            local_overshoot_x2 += dx;
            local_overshoot_y1 += dy;
            local_overshoot_y2 += dy;
            if (local_overshoot_x1 < 0)
            {
                x1 = 0;
                x2 = width - 1;
            }
            else if (local_overshoot_x2 > 15)
            {
                x1 = 15 - (width - 1);
                x2 = 15;
            }
            else
            {
                x1 = local_overshoot_x1;
                x2 = local_overshoot_x2;
            }
            if (local_overshoot_y1 < 0)
            {
                y1 = 0;
                y2 = height - 1;
            }
            else if (local_overshoot_y2 > 15)
            {
                y1 = 15 - (height - 1);
                y2 = 15;
            }
            else
            {
                y1 = local_overshoot_y1;
                y2 = local_overshoot_y2;
            }
        }
        set_embark_pos(screen, x1, x2, y1, y2);
    }
public:
    MouseControl()
        :EmbarkTool(),
        prev_x(0),
        prev_y(0),
        prev_lbut(false),
        base_max_x(false),
        base_max_y(false),
        in_local_move(false),
        in_local_edge_resize_x(false),
        in_local_edge_resize_y(false),
        in_local_corner_resize(false),
        local_overshoot_x1(0),
        local_overshoot_x2(0),
        local_overshoot_y1(0),
        local_overshoot_y2(0)
    { }
    virtual std::string getId() { return "mouse"; }
    virtual std::string getName() { return "Mouse control"; }
    virtual std::string getDesc() { return "Implements mouse controls on the embark screen"; }
    virtual df::interface_key getToggleKey() { return df::interface_key::CUSTOM_M; }
    virtual void after_render(start_sitest* screen)
    {
        GET_EMBARK_POS(screen, x1, x2, y1, y2, width, height);
        int mouse_x = gps->mouse_x, mouse_y = gps->mouse_y;
        int local_x = prev_x - 1;
        int local_y = prev_y - 2;
        if (local_x >= x1 && local_x <= x2 && local_y >= y1 && local_y <= y2)
        {
            int screen_x1 = x1 + 1;
            int screen_x2 = x2 + 1;
            int screen_y1 = y1 + 2;
            int screen_y2 = y2 + 2;
            UIColor fg = in_local_adjust() ? COLOR_GREY : COLOR_DARKGREY;
            Screen::Pen corner_ul = Screen::Pen((char)201, fg, COLOR_BLACK);
            Screen::Pen corner_ur = Screen::Pen((char)187, fg, COLOR_BLACK);
            Screen::Pen corner_dl = Screen::Pen((char)200, fg, COLOR_BLACK);
            Screen::Pen corner_dr = Screen::Pen((char)188, fg, COLOR_BLACK);
            Screen::Pen border_ud = Screen::Pen((char)205, fg, COLOR_BLACK);
            Screen::Pen border_lr = Screen::Pen((char)186, fg, COLOR_BLACK);
            if (in_local_corner_resize ||
                ((local_x == x1 || local_x == x2) && (local_y == y1 || local_y == y2)))
            {
                if (local_x == x1 && local_y == y1)
                    Screen::paintTile(corner_ul, screen_x1, screen_y1);
                else if (local_x == x2 && local_y == y1)
                    Screen::paintTile(corner_ur, screen_x2, screen_y1);
                else if (local_x == x1 && local_y == y2)
                    Screen::paintTile(corner_dl, screen_x1, screen_y2);
                else if (local_x == x2 && local_y == y2)
                    Screen::paintTile(corner_dr, screen_x2, screen_y2);
            }
            else if (in_local_edge_resize_x || local_x == x1 || local_x == x2)
            {
                if ((in_local_edge_resize_x && !base_max_x) || local_x == x1)
                {
                    Screen::paintTile(corner_ul, screen_x1, screen_y1);
                    for (int i = screen_y1 + 1; i <= screen_y2 - 1; ++i)
                        Screen::paintTile(border_lr, screen_x1, i);
                    Screen::paintTile(corner_dl, screen_x1, screen_y2);
                }
                else
                {
                    Screen::paintTile(corner_ur, screen_x2, screen_y1);
                    for (int i = screen_y1 + 1; i <= screen_y2 - 1; ++i)
                        Screen::paintTile(border_lr, screen_x2, i);
                    Screen::paintTile(corner_dr, screen_x2, screen_y2);
                }
            }
            else if (in_local_edge_resize_y || local_y == y1 || local_y == y2)
            {
                if ((in_local_edge_resize_y && !base_max_y) || local_y == y1)
                {
                    Screen::paintTile(corner_ul, screen_x1, screen_y1);
                    for (int i = screen_x1 + 1; i <= screen_x2 - 1; ++i)
                        Screen::paintTile(border_ud, i, screen_y1);
                    Screen::paintTile(corner_ur, screen_x2, screen_y1);
                }
                else
                {
                    Screen::paintTile(corner_dl, screen_x1, screen_y2);
                    for (int i = screen_x1 + 1; i <= screen_x2 - 1; ++i)
                        Screen::paintTile(border_ud, i, screen_y2);
                    Screen::paintTile(corner_dr, screen_x2, screen_y2);
                }
            }
            else
            {
                Screen::paintTile(corner_ul, screen_x1, screen_y1);
                Screen::paintTile(corner_ur, screen_x2, screen_y1);
                Screen::paintTile(corner_dl, screen_x1, screen_y2);
                Screen::paintTile(corner_dr, screen_x2, screen_y2);
            }
        }
    }
    virtual void after_mouse_event(start_sitest* screen)
    {
        if (enabler->mouse_lbut != prev_lbut)
        {
            lbut_press(screen, enabler->mouse_lbut, gps->mouse_x, gps->mouse_y);
        }
        if (gps->mouse_x != prev_x || gps->mouse_y != prev_y)
        {
            mouse_move(screen, gps->mouse_x, gps->mouse_y);
        }
        prev_lbut = enabler->mouse_lbut;
        prev_x = gps->mouse_x;
        prev_y = gps->mouse_y;
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
        std::string title = "  Embark tools (DFHack)  ";
        Screen::paintString(Screen::Pen(' ', COLOR_BLACK, COLOR_GREY), min_x + ((max_x - min_x - title.size()) / 2), min_y, title);
        x = min_x + 2;
        y = max_y - 2;
        OutputString(COLOR_LIGHTRED, x, y, Screen::getKeyDisplay(df::interface_key::SELECT));
        OutputString(COLOR_WHITE, x, y, "/");
        OutputString(COLOR_LIGHTRED, x, y, Screen::getKeyDisplay(df::interface_key::LEAVESCREEN));
        OutputString(COLOR_WHITE, x, y, ": Done");
        y = min_y + 2;
        FOR_ITER_TOOLS(iter)
        {
            EmbarkTool* t = iter->second;
            x = min_x + 2;
            OutputString(COLOR_LIGHTRED, x, y, Screen::getKeyDisplay(t->getToggleKey()));
            OutputString(COLOR_WHITE, x, y, ": " + t->getName() + ": ");
            OutputString(t->getEnabled() ? COLOR_GREEN : COLOR_RED, x, y,
                         t->getEnabled() ? "Enabled" : "Disabled");
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
                EmbarkTool* t = iter->second;
                if (t->getToggleKey() == key)
                {
                    t->toggleEnabled();
                }
            }
        }
    };
};

void add_tool (EmbarkTool *t)
{
    tools[t->getId()] = t;
}

bool tool_exists (std::string tool_name)
{
    return tools.find(tool_name) != tools.end();
}

bool tool_enabled (std::string tool_name)
{
    if (tool_exists(tool_name))
        return tools[tool_name]->getEnabled();
    return false;
}

bool tool_enable (std::string tool_name, bool enable_state)
{
    int n = 0;
    FOR_ITER_TOOLS(iter)
    {
        EmbarkTool* tool = iter->second;
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
        std::vector<std::string> parts;
        FOR_ITER_TOOLS(it)
        {
            EmbarkTool *t = it->second;
            if (t->getEnabled())
                parts.push_back(t->getName());
        }
        if (parts.size())
        {
            std::string label = join_strings(", ", parts);
            if (label.size() > dim.x - x - 1)
            {
                label.resize(dim.x - x - 1 - 3);
                label.append("...");
            }
            OutputString(COLOR_LIGHTMAGENTA, x, y, label);
        }
        else
            OutputString(COLOR_LIGHTMAGENTA, x, y, "(none)");
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
            EmbarkTool* tool = iter->second;
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
            EmbarkTool* tool = iter->second;
            if (tool->getEnabled())
                tool->after_feed(this, input);
        }
    }
    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        FOR_ITER_TOOLS(iter)
        {
            EmbarkTool* tool = iter->second;
            if (tool->getEnabled())
                tool->before_render(this);
        }
        INTERPOSE_NEXT(render)();
        display_tool_status();
        FOR_ITER_TOOLS(iter)
        {
            EmbarkTool* tool = iter->second;
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
    add_tool(new EmbarkAnywhere);
    add_tool(new MouseControl);
    add_tool(new SandIndicator);
    add_tool(new StablePosition);
    std::string help = "";
    help += "embark-tools (enable/disable) tool [tool...]\n"
            "Tools:\n";
    FOR_ITER_TOOLS(iter)
    {
        help += ("  " + iter->second->getId() + ": " + iter->second->getDesc() + "\n");
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

DFhackCExport command_result plugin_onstatechange (color_ostream &out, state_change_event evt)
{
    if (evt == SC_BEGIN_UNLOAD)
    {
        if (Gui::getCurFocus() == "dfhack/embark-tools/options")
        {
            out.printerr("Settings screen active.\n");
            return CR_FAILURE;
        }
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate (color_ostream &out)
{
    static int8_t mask = 0;
    static decltype(gps->mouse_x) prev_x = -1;
    static decltype(gps->mouse_y) prev_y = -1;
    df::viewscreen* parent = DFHack::Gui::getCurViewscreen();
    VIRTUAL_CAST_VAR(screen, df::viewscreen_choose_start_sitest, parent);
    if (!screen)
        return CR_OK;
    int8_t new_mask = (enabler->mouse_lbut << 1) |
                      (enabler->mouse_rbut << 2) |
                      (enabler->mouse_lbut_down << 3) |
                      (enabler->mouse_rbut_down << 4) |
                      (enabler->mouse_lbut_lift << 5) |
                      (enabler->mouse_rbut_lift << 6);
    if (mask != new_mask || prev_x != gps->mouse_x || prev_y != gps->mouse_y)
    {
        FOR_ITER_TOOLS(iter)
        {
            if (iter->second->getEnabled())
                iter->second->after_mouse_event(screen);
        }
    }
    mask = new_mask;
    prev_x = gps->mouse_x;
    prev_y = gps->mouse_y;
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
                EmbarkTool* t = iter->second;
                out << "  " << t->getName() << " (" << t->getId() << "): "
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
