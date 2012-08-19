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

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    if (gps)
    {
        if (!INTERPOSE_HOOK(title_hook, render).apply())
            out.printerr("Could not interpose viewscreen_titlest::render\n");
    }

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    INTERPOSE_HOOK(title_hook, render).remove();
    return CR_OK;
}
