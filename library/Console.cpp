#include "Console.h"
#ifndef BUILD_DFHACK_CLIENT
    #include "SDLConsoleDriver.h"
    #if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
        #define PLATFORM_CONSOLE PosixConsole
        #include "PosixConsole.h"
    #elif defined(_WIN32)
        #define PLATFORM_CONSOLE WindowsConsole
        #include "WindowsConsole.h"
    #endif
#endif

#include <chrono>
#include <thread>

using namespace DFHack;

constexpr const char * ANSI_CLS = "\033[2J";
constexpr const char * ANSI_BLACK = "\033[22;30m";
constexpr const char * ANSI_RED = "\033[22;31m";
constexpr const char * ANSI_GREEN = "\033[22;32m";
constexpr const char * ANSI_BROWN = "\033[22;33m";
constexpr const char * ANSI_BLUE = "\033[22;34m";
constexpr const char * ANSI_MAGENTA = "\033[22;35m";
constexpr const char * ANSI_CYAN = "\033[22;36m";
constexpr const char * ANSI_GREY = "\033[22;37m";
constexpr const char * ANSI_DARKGREY = "\033[01;30m";
constexpr const char * ANSI_LIGHTRED = "\033[01;31m";
constexpr const char * ANSI_LIGHTGREEN = "\033[01;32m";
constexpr const char * ANSI_YELLOW = "\033[01;33m";
constexpr const char * ANSI_LIGHTBLUE = "\033[01;34m";
constexpr const char * ANSI_LIGHTMAGENTA = "\033[01;35m";
constexpr const char * ANSI_LIGHTCYAN = "\033[01;36m";
constexpr const char * ANSI_WHITE = "\033[01;37m";
constexpr const char * RESETCOLOR = "\033[0m";

const char * Console::getANSIColor(const int c) noexcept
{
    switch (c)
    {
        case -1: return RESETCOLOR; // HACK! :P
        case 0 : return ANSI_BLACK;
        case 1 : return ANSI_BLUE; // non-ANSI
        case 2 : return ANSI_GREEN;
        case 3 : return ANSI_CYAN; // non-ANSI
        case 4 : return ANSI_RED; // non-ANSI
        case 5 : return ANSI_MAGENTA;
        case 6 : return ANSI_BROWN;
        case 7 : return ANSI_GREY;
        case 8 : return ANSI_DARKGREY;
        case 9 : return ANSI_LIGHTBLUE; // non-ANSI
        case 10: return ANSI_LIGHTGREEN;
        case 11: return ANSI_LIGHTCYAN; // non-ANSI;
        case 12: return ANSI_LIGHTRED; // non-ANSI;
        case 13: return ANSI_LIGHTMAGENTA;
        case 14: return ANSI_YELLOW; // non-ANSI
        case 15: return ANSI_WHITE;
        default: return "";
    }
}

std::unique_ptr<Console> Console::makeConsole() {
#ifdef BUILD_DFHACK_CLIENT
    return std::make_unique<Console>();
#else
    if (PLATFORM_CONSOLE::is_enabled())
        return std::make_unique<PLATFORM_CONSOLE>();

    return std::make_unique<SDLConsoleDriver>();
#endif
}

void Console::add_text(color_value color, const std::string &text) {
    if (use_ansi_colors_) {
        std::cout << getANSIColor(color);
        std::cout << text;
        std::cout << RESETCOLOR;
    } else {
        std::cout << text;
    }
}

void Console::msleep (unsigned int msec)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(msec));
}
