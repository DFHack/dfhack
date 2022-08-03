// This template is appropriate for plugins that can be enabled to make some
// specific persistent change to the game, but don't need a world to be loaded
// before they are enabled. These types of plugins typically register some sort
// of hook on enable and clear the hook on disable. They are generally enabled
// from dfhack.init and do not need to persist and reload their enabled state.

#include <string>
#include <vector>

#include "df/viewscreen_titlest.h"

#include "Debug.h"
#include "PluginManager.h"
#include "VTableInterpose.h"

using std::string;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("ui_addition_example");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

namespace DFHack {
    DBG_DECLARE(ui_addition_example, log);
}

// example of hooking a screen so the plugin code will run whenever the screen
// is visible
struct title_version_hook : df::viewscreen_titlest {
    typedef df::viewscreen_titlest interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, render, ()) {
        INTERPOSE_NEXT(render)();

        // TODO: injected render logic here
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(title_version_hook, render);

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    DEBUG(log,out).print("shutting down %s\n", plugin_name);
    INTERPOSE_HOOK(title_version_hook, render).remove();
    return CR_OK;
}

DFhackCExport command_result plugin_enable (color_ostream &out, bool enable) {
    if (enable != is_enabled) {
        DEBUG(log,out).print("%s %s\n", plugin_name,
                             is_enabled ? "enabled" : "disabled");
        if (!INTERPOSE_HOOK(title_version_hook, render).apply(enable))
            return CR_FAILURE;

        is_enabled = enable;
    }
    return CR_OK;
}
