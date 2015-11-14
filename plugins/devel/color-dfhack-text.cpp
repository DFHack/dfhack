#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/Screen.h"

using namespace DFHack;

DFHACK_PLUGIN("color-dfhack-text");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

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
    pen2.fg = color;
    pen2.bg = color;
    pen2.bold = true;
    return color_text_hook.next()(pen2, x, y, map);
}

DFhackCExport command_result plugin_enable (color_ostream &out, bool enable)
{
    color_text_hook.apply(enable);
    is_enabled = enable;
    return CR_OK;
}

command_result color(color_ostream &out, std::vector<std::string> &params)
{
    plugin_enable(out, true);
    std::string p0 = toLower(params[0]);
#define CHECK_COLOR(color_name) else if (p0 == toLower(std::string(#color_name))) \
        { config.flicker = false; config.color = COLOR_##color_name % 8; }
    if (params.empty())
        return CR_OK;
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
    else if (p0 == "flicker")
    {
        config.flicker = true;
    }
    else if (p0 == "disable")
    {
        plugin_enable(out, false);
    }
    else if (p0 != "enable")
    {
        out.printerr("Unrecognized option: %s\n", p0.c_str());
        return CR_WRONG_USAGE;
    }
    return CR_OK;
}

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "color-dfhack-text",
        "color <color>|flicker|enable|disable",
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
