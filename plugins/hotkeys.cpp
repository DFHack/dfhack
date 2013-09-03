#include "uicommon.h"

#include "df/viewscreen_dwarfmodest.h"
#include "df/ui.h"
#include "modules/Maps.h"
#include "modules/World.h"
#include "modules/Gui.h"

//using df::global::ui;

DFHACK_PLUGIN("hotkeys");
#define PLUGIN_VERSION 0.1

static bool display_active = false;

struct hotkeys_hook : public df::viewscreen_dwarfmodest
{
    typedef df::viewscreen_dwarfmodest interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        INTERPOSE_NEXT(feed)(input);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();
        if (!display_active)
            return;

        auto dims = Gui::getDwarfmodeViewDims();
        if (dims.menu_x1 <= 0)
            return;

    }
};

IMPLEMENT_VMETHOD_INTERPOSE(hotkeys_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(hotkeys_hook, render);


static command_result autotrade_cmd(color_ostream &out, vector <string> & parameters)
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
            display_active = true;
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
        autotrade_cmd, false, ""));

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
        break;
    default:
        break;
    }

    return CR_OK;
}
