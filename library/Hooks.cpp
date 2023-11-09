#include "Core.h"
#include "Export.h"

#include "df/gamest.h"

static bool disabled = false;

// called from the main thread before the simulation thread is started
// and the main event loop is initiated
DFhackCExport void dfhooks_init() {
    if (getenv("DFHACK_DISABLE")) {
        fprintf(stdout, "dfhack: DFHACK_DISABLE detected in environment; disabling\n");
        disabled = true;
        return;
    }

    // we need to init DF globals before we can check the commandline
    if (!DFHack::Core::getInstance().InitMainThread() || !df::global::game)
        return;
    const std::string & cmdline = df::global::game->command_line.original;
    if (cmdline.find("--disable-dfhack") != std::string::npos) {
        fprintf(stdout, "dfhack: --disable-dfhack specified on commandline; disabling\n");
        disabled = true;
    }
}

// called from the main thread after the main event loops exits
DFhackCExport void dfhooks_shutdown() {
    if (disabled)
        return;
    DFHack::Core::getInstance().Shutdown();
}

// called from the simulation thread in the main event loop
DFhackCExport void dfhooks_update() {
    if (disabled)
        return;
    DFHack::Core::getInstance().Update();
}

// called from the simulation thread just before adding the macro
// recording/playback overlay
DFhackCExport void dfhooks_prerender() {
    if (disabled)
        return;
    // TODO: render overlay widgets that are not attached to a viewscreen
}

// called from the main thread for each SDL event. if true is returned, then
// the event has been consumed and further processing shouldn't happen
DFhackCExport bool dfhooks_sdl_event(SDL_Event* event) {
    if (disabled)
        return false;
    return DFHack::Core::getInstance().DFH_SDL_Event(event);
}

// called from the main thread for each utf-8 char read from the ncurses input
// key is positive for ncurses keys and negative for everything else
// if true is returned, then the event has been consumed and further processing
// shouldn't happen
DFhackCExport bool dfhooks_ncurses_key(int key) {
    if (disabled)
        return false;
    return DFHack::Core::getInstance().DFH_ncurses_key(key);
}
