#ifdef BUILD_DFHACK_CLIENT
    #include "Console.h"
#else
    #if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
        #include "PosixConsole.h"
    #elif defined(_WIN32) || defined(_WIN64)
        #include "WindowsConsole.h"
    #endif
    #include "SDLConsoleDriver.h"
#endif

using namespace DFHack;

std::unique_ptr<Console> Console::makeConsole() {
    #ifdef BUILD_DFHACK_CLIENT
        return std::make_unique<Console>();
    #else
        // Return the Windows console
        #if defined(_WIN32) || defined(_WIN64)
            return std::make_unique<WindowsConsole>();

        // Return Posix console if launched from a supported terminal
        #elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
            if (PosixConsole::is_supported())
                return std::make_unique<PosixConsole>();
        #endif

        return std::make_unique<SDLConsoleDriver>();
    #endif
}
