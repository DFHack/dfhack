#include "Core.h"
#include "Export.h"

#include "df/gamest.h"

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#   include <libloaderapi.h>
#else
#   include <dlfcn.h>
#endif

static bool disabled = false;

DFhackCExport const int32_t dfhooks_priority = 100;

static std::filesystem::path getModulePath()
{
#ifdef _WIN32
    HMODULE module = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)getModulePath, &module);
    if (!module) return std::filesystem::path(); // should never happen, but just in case, return an empty path instead of crashing

    wchar_t path[MAX_PATH];
    GetModuleFileNameW(module, path, MAX_PATH);
    return std::filesystem::path(path);
#else
    DL_info info;
    dladdr(getModulePath, &info);
    return std::filesystem::path(info.dli_fname);
#endif
}

static std::filesystem::path basepath{getModulePath()};

// called by the chainloader before the main thread is initialized and before any other hooks are called.
DFhackCExport void dfhooks_preinit(std::filesystem::path dllpath)
{
    basepath = dllpath.parent_path();
}

// called from the main thread before the simulation thread is started
// and the main event loop is initiated
DFhackCExport void dfhooks_init() {
    if (getenv("DFHACK_DISABLE")) {
        fprintf(stderr, "dfhack: DFHACK_DISABLE detected in environment; disabling\n");
        disabled = true;
        return;
    }

    // we need to init DF globals before we can check the commandline
    if (!DFHack::Core::getInstance().InitMainThread(std::filesystem::canonical(basepath)) || !df::global::game) {
        // we don't set disabled to true here so symbol generation can work
        return;
    }

    const std::string & cmdline = df::global::game->command_line.original;
    if (cmdline.find("--disable-dfhack") != std::string::npos) {
        fprintf(stderr, "dfhack: --disable-dfhack specified on commandline; disabling\n");
        disabled = true;
        return;
    }

    fprintf(stderr, "DFHack pre-init successful.\n");
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
    // TODO: potentially render overlay elements that are not attached to a viewscreen
}

// called from the main thread for each SDL event. if true is returned, then
// the event has been consumed and further processing shouldn't happen
DFhackCExport bool dfhooks_sdl_event(SDL_Event* event) {
    if (disabled)
        return false;
    return DFHack::Core::getInstance().DFH_SDL_Event(event);
}

// called from the main thread just after setting mouse state in gps and just
// before rendering the screen buffer to the screen.
DFhackCExport void dfhooks_sdl_loop() {
    if (disabled)
        return;
    // TODO: wire this up to the new SDL-based console once it is merged
    DFHack::Core::getInstance().DFH_SDL_Loop();
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
