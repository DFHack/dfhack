#include "Console.h"
#include "PosixConsole.h"
#include "WindowsConsole.h"
#include "SDLConsoleDriver.h"

using namespace DFHack;

std::unique_ptr<Console> Console::makeConsole() {

// Return the Windows console
#if defined(_WIN32) || defined(_WIN64)
    return std::make_unique<WindowsConsole>(); 

// Return Posix console if launched from a supported terminal
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
    if (PosixConsole::is_supported())
        return std::make_unique<PosixConsole>();
#endif

// Return the SDL console if not launched from dfhack_run
#ifndef DISABLE_SDL_CONSOLE
    return std::make_unique<SDLConsoleDriver>();
#endif

// Return the DUMMY do nothing console
// Should not get here when the SDL console is available
    return std::make_unique<Console>();
}
