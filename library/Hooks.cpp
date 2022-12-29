#include "Core.h"
#include "Export.h"

// called before main event loop starts
DFhackCExport void dfhooks_init() {
    DFHack::Core::getInstance().Init();
}

// called after main event loops exits
DFhackCExport void dfhooks_shutdown() {
    DFHack::Core::getInstance().Shutdown();
}

// called in the main event loop
DFhackCExport void dfhooks_update() {
    DFHack::Core::getInstance().Update();
}

// called just before adding the macro recording/playback overlay
DFhackCExport void dfhooks_prerender() {
    // TODO: render overlay widgets that are not attached to a viewscreen
}

// called for each SDL event, if true is returned, then the event has been
// consumed and further processing shouldn't happen
DFhackCExport bool dfhooks_sdl_event(SDL::Event* event) {
    return DFHack::Core::getInstance().DFH_SDL_Event(event);
}
// called for each utf-8 char read from the ncurses input
// key is positive for ncurses keys and negative for everything else
DFhackCExport bool dfhooks_ncurses_key(int key) {
    return DFHack::Core::getInstance().DFH_ncurses_key(key);
}
