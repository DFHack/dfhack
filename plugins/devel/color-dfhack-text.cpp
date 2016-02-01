#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/Gui.h"
#include "modules/Screen.h"

using namespace DFHack;

DFHACK_PLUGIN("color-dfhack-text");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);
REQUIRE_GLOBAL(gps);

struct {
    bool flicker;
    uint8_t color;
    short tick;
} config;

void color_text_tile(const Screen::Pen &pen, int x, int y, bool map);
GUI_HOOK_CALLBACK(Screen::Hooks::set_tile, color_text_hook, color_text_tile);
void color_text_tile(const Screen::Pen &pen, int x, int y, bool map)
{
    Screen::Pen pen2 = pen;
    uint8_t color = config.flicker ? config.tick % 8 : config.color;
    if (map)
    {
        pen2.fg = color % 8;
        pen2.bg = (color % 8) + 8;
        pen2.bold = false;
    }
    else
    {
        pen2.fg = color;
        pen2.bg = color;
        pen2.bold = true;
    }
    return color_text_hook.next()(pen2, x, y, map);
}

void aaaaa_set_tile(const Screen::Pen &pen, int x, int y, bool map);
GUI_HOOK_CALLBACK(Screen::Hooks::set_tile, aaaaa_set_tile_hook, aaaaa_set_tile);
void aaaaa_set_tile(const Screen::Pen &pen, int x, int y, bool map)
{
    Screen::Pen pen2 = pen;
    if ((pen.ch >= 'A' && pen.ch <= 'Z') || (pen.ch >= '0' && pen.ch <= '9'))
        pen2.ch = 'A';
    else if (pen.ch >= 'a' && pen.ch <= 'z')
        pen2.ch = 'a';
    aaaaa_set_tile_hook.next()(pen2, x, y, map);
}

void shift_set_tile(const Screen::Pen &pen, int x, int y, bool map);
GUI_HOOK_CALLBACK(Screen::Hooks::set_tile, shift_set_tile_hook, shift_set_tile);
void shift_set_tile(const Screen::Pen &pen, int x, int y, bool map)
{
    x = (x + 1) % gps->dimx;
    shift_set_tile_hook.next()(pen, x, y, map);
}

DFhackCExport command_result plugin_enable (color_ostream &out, bool enable)
{
    color_text_hook.apply(enable);
    if (!enable)
    {
        shift_set_tile_hook.disable();
        aaaaa_set_tile_hook.disable();
    }
    is_enabled = enable;
    return CR_OK;
}

command_result color(color_ostream &out, std::vector<std::string> &params)
{
    if (params.empty())
        return plugin_enable(out, true);
    for (auto it = params.begin(); it != params.end(); ++it)
    {
        std::string p = toLower(*it);
        if (!p.size())
            continue;
        #define CHECK_COLOR(color_name) else if (p == toLower(std::string(#color_name))) \
            { config.flicker = false; config.color = COLOR_##color_name % 8; plugin_enable(out, true); }
        CHECK_COLOR(RED)
        CHECK_COLOR(GREEN)
        CHECK_COLOR(BLUE)
        CHECK_COLOR(YELLOW)
        CHECK_COLOR(BROWN)
        CHECK_COLOR(CYAN)
        CHECK_COLOR(MAGENTA)
        CHECK_COLOR(WHITE)
        CHECK_COLOR(GREY)
        CHECK_COLOR(BLACK)
        #undef CHECK_COLOR
        else if (p == "flicker")
        {
            config.flicker = true;
            plugin_enable(out, true);
        }
        else if (p.size() >= 3 && p.substr(0, 3) == "aaa")
        {
            aaaaa_set_tile_hook.toggle();
        }
        else if (p == "shift")
        {
            shift_set_tile_hook.toggle();
        }
        else if (p == "disable")
        {
            plugin_enable(out, false);
        }
        else if (p != "enable")
        {
            out.printerr("Unrecognized option: %s\n", p.c_str());
            return CR_WRONG_USAGE;
        }
    }
    return CR_OK;
}

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "color-dfhack-text",
        "color <color>|flicker|enable|disable|shift|aaaaa",
        color,
        false
    ));
    config.flicker = false;
    config.color = COLOR_RED;
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate (color_ostream &out)
{
    ++config.tick;
    return CR_OK;
}
