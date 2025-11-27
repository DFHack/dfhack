/*
https://github.com/peterix/dfhack

A thread-safe logging console with a line editor.

Based on linenoise:
linenoise -- guerrilla line editing library against the idea that a
line editing lib needs to be 20,000 lines of C code.

You can find the latest source code at:

  http://github.com/antirez/linenoise

Does a number of crazy assumptions that happen to be true in 99.9999% of
the 2010 UNIX computers around.

------------------------------------------------------------------------

Copyright (c) 2010, Salvatore Sanfilippo <antirez at gmail dot com>
Copyright (c) 2010, Pieter Noordhuis <pcnoordhuis at gmail dot com>
Copyright (c) 2011, Petr Mrázek <peterix@gmail.com>

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <cstdio>
#include <string>
#include <SDL_pixels.h>

#include "SDLConsoleDriver.h"
#include "SDLConsole.h"

using namespace DFHack;

using namespace sdl_console;

namespace DFHack
{
    static SDL_Color getSDLColor(int c)
    {
        constexpr SDL_Color ANSI_BLACK        = {0, 0, 0, 255};
        constexpr SDL_Color ANSI_BLUE         = {0, 0, 128, 255};      // non-ANSI
        constexpr SDL_Color ANSI_GREEN        = {0, 128, 0, 255};
        constexpr SDL_Color ANSI_CYAN         = {0, 128, 128, 255};    // non-ANSI
        constexpr SDL_Color ANSI_RED          = {128, 0, 0, 255};      // non-ANSI
        constexpr SDL_Color ANSI_MAGENTA      = {128, 0, 128, 255};
        constexpr SDL_Color ANSI_BROWN        = {128, 128, 0, 255};
        constexpr SDL_Color ANSI_GREY         = {192, 192, 192, 255};
        constexpr SDL_Color ANSI_DARKGREY     = {128, 128, 128, 255};
        constexpr SDL_Color ANSI_LIGHTBLUE    = {0, 0, 255, 255};      // non-ANSI
        constexpr SDL_Color ANSI_LIGHTGREEN   = {0, 255, 0, 255};
        constexpr SDL_Color ANSI_LIGHTCYAN    = {0, 255, 255, 255};    // non-ANSI
        constexpr SDL_Color ANSI_LIGHTRED     = {255, 0, 0, 255};      // non-ANSI
        constexpr SDL_Color ANSI_LIGHTMAGENTA = {255, 0, 255, 255};
        constexpr SDL_Color ANSI_YELLOW       = {255, 255, 0, 255};    // non-ANSI
        constexpr SDL_Color ANSI_WHITE        = {255, 255, 255, 255};

        switch (c)
        {
            case -1: return ANSI_WHITE;
            case 0 : return ANSI_BLACK;
            case 1 : return ANSI_BLUE; // non-ANSI
            case 2 : return ANSI_GREEN;
            case 3 : return ANSI_CYAN; // non-ANSI
            case 4 : return ANSI_RED;  // non-ANSI
            case 5 : return ANSI_MAGENTA;
            case 6 : return ANSI_BROWN;
            case 7 : return ANSI_GREY;
            case 8 : return ANSI_DARKGREY;
            case 9 : return ANSI_LIGHTBLUE; // non-ANSI
            case 10: return ANSI_LIGHTGREEN;
            case 11: return ANSI_LIGHTCYAN; // non-ANSI
            case 12: return ANSI_LIGHTRED;  // non-ANSI
            case 13: return ANSI_LIGHTMAGENTA;
            case 14: return ANSI_YELLOW;    // non-ANSI
            case 15: // WHITE
            default: return ANSI_WHITE;
        }
    }
}

SDLConsoleDriver::SDLConsoleDriver()
    : Console(this)
    , con_impl(SDLConsole::get_console())
{
}

SDLConsoleDriver::~SDLConsoleDriver() = default;

// Must be called before init()
// init_sdl() must be called from the main renderer thread.
bool SDLConsoleDriver::init_sdl()
{
    return con_impl.init_session();
}

/**
 * FIXME: Two-stage init because we need to initialize on
 * the main thread, but interpose isn't available until later.
 */
bool SDLConsoleDriver::init(bool dont_redirect)
{
    if (!dont_redirect)
    {
        if (freopen("stdout.log", "w", stdout) == nullptr) {
            fputs("Failed to redirect stdout to file\n", stderr);
        }
    }
    return con_impl.is_active();
}

bool SDLConsoleDriver::shutdown()
{
    con_impl.shutdown_session();
    return true;
}

/*
 * This should be for guarding against interleaving prints to the console.
 * The begin_batch() and end_batch() pair does the job on its own.
 */
void SDLConsoleDriver::begin_batch()
{
    mutex_.lock();
}

void SDLConsoleDriver::end_batch()
{
    mutex_.unlock();
}

/* Don't think we need this? */
void SDLConsoleDriver::flush_proxy()
{
}

void SDLConsoleDriver::add_text(color_value color, const std::string &text)
{
    // I don't think this lock is needed, unless to prevent
    // interleaving prints. But we have batch for that?
    std::scoped_lock l(mutex_);
    if(con_impl.is_active())
        con_impl.write_line(text, getSDLColor(color));
    else
        fwrite(text.data(), 1, text.size(), stderr);
}

int SDLConsoleDriver::get_columns()
{
    // returns Console::FAILURE if inactive
    return con_impl.get_columns();
}

int SDLConsoleDriver::get_rows()
{
    // returns Console::FAILURE if inactive
    return con_impl.get_rows();
}

void SDLConsoleDriver::clear()
{
    con_impl.clear();
}
/* XXX: Not implemented */
void SDLConsoleDriver::gotoxy(int x, int y)
{
}
/* XXX: Not implemented */
void SDLConsoleDriver::cursor(bool enable)
{
    //con.cursor(enable);
}

int SDLConsoleDriver::lineedit(const std::string& prompt, std::string& output, CommandHistory& ch)
{
    if(con_impl.was_shutdown())
        return Console::SHUTDOWN;

    static bool did_set_history = false;

    // I don't believe this check is necessary.
    // unless, somwhow, fiothread is inited before the console.
    if (!con_impl.is_active()) {
        // fiothread doesn't wait before retrying
        msleep(100);
        return Console::RETRY;
    }
    // kludge. This is the only place to set it?
    if (!did_set_history) {
        std::vector<std::string> hist;
        ch.getEntries(hist);
        con_impl.set_command_history(hist);
        did_set_history = true;
    }

    if (prompt != con_impl.get_prompt()) {
        con_impl.set_prompt(prompt);
    }

    int ret = con_impl.get_line(output);
    if (ret == 0)
        return Console::RETRY;
    if (ret == -1)
        return Console::SHUTDOWN;

    return ret;
}

bool SDLConsoleDriver::hide()
{
    con_impl.hide_window();
    return true;
}

bool SDLConsoleDriver::show()
{
    con_impl.show_window();
    return true;
}

/*
 * We should cleanup() if the console failed after init (unlikely to happen),
 * or if commanded to shutdown during run time.
 *
 */
bool SDLConsoleDriver::sdl_event_hook(const SDL_Event &e)
{
    return con_impl.sdl_event_hook(e);
}

void SDLConsoleDriver::update()
{
    con_impl.update();
}

/*
 * Cleanup must be done from the main thread (or the thread that called init_sdl()).
 */
void SDLConsoleDriver::cleanup()
{
    con_impl.destroy_session();
}
