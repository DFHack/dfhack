// Fast Startup tweak

#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <MiscUtils.h>
#include <VTableInterpose.h>

#include "df/viewscreen_initial_prepst.h"
#include <vector>

using namespace DFHack;
using namespace df::enums;
using std::vector;

// Uncomment this to make the Loading screen as fast as possible
// This has the side effect of removing the dwarf face animation
// (and briefly making the game become unresponsive)

//#define REALLY_FAST

DFHACK_PLUGIN("faststart");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

struct prep_hook : df::viewscreen_initial_prepst
{
    typedef df::viewscreen_initial_prepst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, logic, ())
    {
#ifdef REALLY_FAST
        while (breakdown_level != interface_breakdown_types::STOPSCREEN)
        {
            render_count++;
            INTERPOSE_NEXT(logic)();
        }
#else
        render_count = 4;
        INTERPOSE_NEXT(logic)();
#endif
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(prep_hook, logic);

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    if (enable != is_enabled)
    {
        if (!INTERPOSE_HOOK(prep_hook, logic).apply(enable))
            return CR_FAILURE;

        is_enabled = enable;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands)
{
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    INTERPOSE_HOOK(prep_hook, logic).remove();
    return CR_OK;
}
