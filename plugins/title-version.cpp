#include <vector>
#include <cstdio>
#include <stack>
#include <string>
#include <cmath>

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Gui.h"
#include "modules/Screen.h"
#include "VTableInterpose.h"
#include "DFHackVersion.h"

#include "df/graphic.h"
#include "df/viewscreen_titlest.h"
#include "uicommon.h"

using std::vector;
using std::string;
using namespace DFHack;

DFHACK_PLUGIN("title-version");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);
REQUIRE_GLOBAL(gps);

struct title_version_hook : df::viewscreen_titlest {
    typedef df::viewscreen_titlest interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();
        int x = 0, y = 0;
        OutputString(COLOR_WHITE, x, y, string("DFHack ") + DFHACK_VERSION);
        if (!DFHACK_IS_RELEASE)
            OutputString(COLOR_WHITE, x, y, " (dev)");
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(title_version_hook, render);

DFhackCExport command_result plugin_enable (color_ostream &out, bool enable)
{
    if (!gps)
        return CR_FAILURE;

    if (enable != is_enabled)
    {
        if (!INTERPOSE_HOOK(title_version_hook, render).apply(enable))
            return CR_FAILURE;

        is_enabled = enable;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init (color_ostream &out, vector<PluginCommand> &commands)
{
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    INTERPOSE_HOOK(title_version_hook, render).remove();
    return CR_OK;
}
