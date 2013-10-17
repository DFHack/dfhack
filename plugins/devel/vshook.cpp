#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <modules/Gui.h>
#include <modules/Screen.h>
#include <vector>
#include <cstdio>
#include <stack>
#include <string>
#include <cmath>

#include <VTableInterpose.h>
#include "df/graphic.h"
#include "df/viewscreen_titlest.h"

using std::vector;
using std::string;
using std::stack;
using namespace DFHack;

using df::global::gps;

DFHACK_PLUGIN("vshook");

DFHACK_PLUGIN_IS_ENABLED(is_enabled);

struct title_hook : df::viewscreen_titlest {
    typedef df::viewscreen_titlest interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();

        Screen::Pen pen(' ',COLOR_WHITE,COLOR_BLACK);
        Screen::paintString(pen,0,0,"DFHack " DFHACK_VERSION);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(title_hook, render);

DFhackCExport command_result plugin_enable ( color_ostream &out, bool enable)
{
    if (!gps)
        return CR_FAILURE;

    if (enable != is_enabled)
    {
        if (!INTERPOSE_HOOK(title_hook, render).apply(enable))
            return CR_FAILURE;

        is_enabled = enable;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    // DON'T DO THIS IN NON-EXAMPLE PLUGINS
    plugin_enable(out, true);

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    INTERPOSE_HOOK(title_hook, render).remove();
    return CR_OK;
}
