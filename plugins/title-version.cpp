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
#include "df/viewscreen_optionst.h"
#include "df/viewscreen_titlest.h"
#include "uicommon.h"

using std::vector;
using std::string;
using namespace DFHack;

DFHACK_PLUGIN("title-version");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);
REQUIRE_GLOBAL(gps);

void draw_version(int start_x, int start_y) {
    int x = start_x,
        y = start_y;

    OutputString(COLOR_WHITE, x, y, string("DFHack ") + DFHACK_VERSION);
    if (!DFHACK_IS_RELEASE)
    {
        OutputString(COLOR_WHITE, x, y, " (dev)");
        x = start_x; y++;
        OutputString(COLOR_WHITE, x, y, "Git: ");
        OutputString(COLOR_WHITE, x, y, DFHACK_GIT_DESCRIPTION);
    }
    if (DFHACK_IS_PRERELEASE)
    {
        x = start_x; y++;
        OutputString(COLOR_LIGHTRED, x, y, "Pre-release build");
    }
}

struct title_version_hook : df::viewscreen_titlest {
    typedef df::viewscreen_titlest interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();
        if (!loading)
            draw_version(0, 0);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(title_version_hook, render);

struct options_version_hook : df::viewscreen_optionst {
    typedef df::viewscreen_optionst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();
        if (!msg_quit && !in_retire_adv && !msg_peasant &&
            !in_retire_dwf_abandon_adv && !in_abandon_dwf && !ending_game)
            draw_version(0, 0);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(options_version_hook, render);

DFhackCExport command_result plugin_enable (color_ostream &out, bool enable)
{
    if (!gps)
        return CR_FAILURE;

    if (enable != is_enabled)
    {
        if (!INTERPOSE_HOOK(title_version_hook, render).apply(enable) ||
            !INTERPOSE_HOOK(options_version_hook, render).apply(enable))
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
    INTERPOSE_HOOK(options_version_hook, render).remove();
    return CR_OK;
}
