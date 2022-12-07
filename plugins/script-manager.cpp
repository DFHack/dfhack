#include "Debug.h"
#include "LuaTools.h"
#include "PluginManager.h"

using std::string;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("script-manager");

namespace DFHack {
    DBG_DECLARE(script_manager, log, DebugCategory::LINFO);
}

DFhackCExport command_result plugin_init(color_ostream &, std::vector<PluginCommand> &) {
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event != SC_CORE_INITIALIZED)
        return CR_OK;

    DEBUG(log,out).print("scanning scripts for onStateChange() functions\n");

    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);
    Lua::CallLuaModuleFunction(out, L, "plugins.script-manager", "reload");

    return CR_OK;
}
