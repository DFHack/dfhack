// This template is appropriate for plugins that simply provide one or more
// commands, but don't need to be "enabled" to function.

#include <string>
#include <vector>
#include <csignal>

#ifdef _WIN32
#include <float.h>
#else
#include <cfenv>

#endif

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

void allow_divby_zero(){
#ifdef _WIN32
    unsigned int old_fp_control_word;
    unsigned int new_fp_control_word;

    _controlfp_s(&old_fp_control_word, 0, 0); // Get the current control word
    new_fp_control_word = old_fp_control_word & ~_EM_ZERODIVIDE; // Clear the "divide by zero" bit
    new_fp_control_word |= _EM_INVALID; // Set the "invalid operation" bit
    _controlfp_s(&old_fp_control_word, new_fp_control_word, _MCW_EM); // Set the new control word
#else
    feenableexcept(FE_DIVBYZERO);
#endif
}

// the non-raise code ends in the process terminating before the signal is handled.. or the signal not being generated
#define USERAISE

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
#ifdef USERAISE
        std::raise(SIGSEGV);
    } else if (parameters[0] == "sigill") {
        std::raise(SIGILL);
    } else if (parameters[0] == "sigfpe") {
        std::raise(SIGFPE);
#else
        auto p = &suspend;
        (++p)->lock();
    } else if (parameters[0] == "sigill") {
        int* x = (int*)allow_divby_zero - 1;
        void (*fp)() = reinterpret_cast<void (*)()>(x);
        fp();
    } else if (parameters[0] == "sigfpe") {
        double d = 0.0;
        int* p = (int*)&d;
        std::cerr << *(double*)p;
        allow_divby_zero();
        double q = 7.0 / *(double*)p;

        std::cerr << q;
#endif
    } else if (parameters[0] == "sigint") {
        std::raise(SIGINT);
    }


    return CR_OK;
}
