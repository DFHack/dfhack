// This template is appropriate for plugins that simply provide one or more
// commands, but don't need to be "enabled" to function.

#include <string>
#include <vector>
#include <csignal>

#include "Debug.h"
#include "PluginManager.h"

using std::string;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("signals");

namespace DFHack {
    DBG_DECLARE(signals, log, DebugCategory::LERROR);
}

static command_result send_signal(color_ostream &out, vector<string> &parameters);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    DEBUG(log, out).print("initializing %s\n", plugin_name);

    commands.emplace_back(plugin_name, "", send_signal);
    return CR_OK;
}

static command_result send_signal(color_ostream &out, vector<string> &parameters) {
    // be sure to suspend the core if any DF state is read or modified
    CoreSuspender suspend;

    if (parameters.empty()) {
        return DFHack::CR_FAILURE;
    } else if (parameters[0] == "sigterm") {
        std::terminate();
    } else if (parameters[0] == "sigabrt") {
        std::abort();
    } else if (parameters[0] == "sigsegv") {
        auto p = (CoreSuspender*)1337;
        p->lock();
    } else if (parameters[0] == "sigill") {
        __asm__ ("ud2");
    } else if (parameters[0] == "sigfpe") {
        double q = 7.0 / 0.0;
        std::cerr << q;
    } else if (parameters[0] == "sigint") {
        std::raise(SIGINT);
    }

    return CR_OK;
}
