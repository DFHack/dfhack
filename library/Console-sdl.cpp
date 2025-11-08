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
#include "df/renderer_2d.h"
#include <VTableInterpose.h>

using namespace DFHack;

using namespace sdl_console;

namespace DFHack
{
    struct con_render_hook : df::renderer_2d {
        using interpose_base = df::renderer_2d;

        DEFINE_VMETHOD_INTERPOSE(void, render, ())
        {
            SDLConsole::get_console().update();
            INTERPOSE_NEXT(render)();
        }
    };

    IMPLEMENT_VMETHOD_INTERPOSE(con_render_hook, render);

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

    class Private
    {
    public:
        Private() : con(SDLConsole::get_console()) { };
        virtual ~Private() = default;

        int lineedit(const std::string_view prompt, std::string& output, CommandHistory & ch)
        {
            static bool did_set_history = false;

            // I don't believe this check is necessary.
            // unless, somwhow, fiothread is inited before the console.
            if (!con.is_active()) {
                return Console::RETRY;
            }
            // kludge. This is the only place to set it?
            if (!did_set_history) {
                std::vector<std::string> hist;
                ch.getEntries(hist);
                con.set_command_history(hist);
                did_set_history = true;
            }

            if (prompt != con.get_prompt()) {
                con.set_prompt(prompt);
            }

            int ret = con.get_line(output);
            if (ret == 0)
                return Console::RETRY;
            if (ret == -1)
                return Console::SHUTDOWN;

            return ret;
        }

        /// Enable or disable the caret/cursor
        void cursor(bool enable = true)
        {
        }

        SDLConsole& con;
    };
}

SDLConsoleDriver::SDLConsoleDriver()
    : Console(this)
    , inited(false)
{
    d = new Private();

    // we can't create the mutex at this time. the S DL functions aren't hooked yet.
    wlock = new std::recursive_mutex();
}
SDLConsoleDriver::~SDLConsoleDriver()
{
    delete wlock;
    delete d;
}

// Must be called before init()
// init_sdl() must be called from the main renderer thread.
bool SDLConsoleDriver::init_sdl()
{
    inited.store(d->con.init_session());
    return inited.load();
}

/**
 * FIXME: Two-stage init because we need to initialize on
 * the main thread, but interpose isn't available until later.
 */
bool SDLConsoleDriver::init(bool dont_redirect)
{
    // inited is true after init_sdl() succeeds.
    if (inited) {
        // Not sure about this. Useful if the console shuts down on its own
        // after init.
        //d->con.set_destroy_session_callback([this]() {
        //  cleanup();
        //});
        hook_df_renderer(true);
    }

    if (!dont_redirect)
    {
        if (freopen("stdout.log", "w", stdout) == nullptr) {
            fputs("Failed to redirect stdout to file\n", stderr);
        }
    }
    return inited.load();
}

bool SDLConsoleDriver::shutdown()
{
    if (!inited.load())
        return true;
    hook_df_renderer(false);
    d->con.shutdown_session();
    inited.store(false);
    return true;
}

/*
 * This should be for guarding against interleaving prints to the console.
 * The begin_batch() and end_batch() pair does the job on its own.
 */
void SDLConsoleDriver::begin_batch()
{
    wlock->lock();
}

void SDLConsoleDriver::end_batch()
{
    wlock->unlock();
}

/* Don't think we need this? */
void SDLConsoleDriver::flush_proxy()
{
}

void SDLConsoleDriver::add_text(color_value color, const std::string &text)
{
    // I don't think this lock is needed, unless to prevent
    // interleaving prints. But we have batch for that?
    std::lock_guard <std::recursive_mutex> g(*wlock);
    if(inited.load())
        d->con.write_line(text, getSDLColor(color));
    else
        fwrite(text.data(), 1, text.size(), stderr);
}

int SDLConsoleDriver::get_columns()
{
    // returns Console::FAILURE if inactive
    return d->con.get_columns();
}

int SDLConsoleDriver::get_rows()
{
    // returns Console::FAILURE if inactive
    return d->con.get_rows();
}

void SDLConsoleDriver::clear()
{
    d->con.clear();
}
/* XXX: Not implemented */
void SDLConsoleDriver::gotoxy(int x, int y)
{
}
/* XXX: Not implemented */
void SDLConsoleDriver::cursor(bool enable)
{
    d->cursor(enable);
}

int SDLConsoleDriver::lineedit(const std::string& prompt, std::string& output, CommandHistory& ch)
{
    // Tell fiothread we are done.
    if(!inited.load())
        return Console::SHUTDOWN;

    return d->lineedit(prompt,output,ch);
}

bool SDLConsoleDriver::hide()
{
    d->con.hide_window();
    return true;
}

bool SDLConsoleDriver::show()
{
    d->con.show_window();
    return true;
}

/*
 * We should cleanup() if the console failed after init (unlikely to happen),
 * or if commanded to shutdown during run time.
 *
 */
bool SDLConsoleDriver::sdl_event_hook(const SDL_Event &e)
{
    return d->con.sdl_event_hook(e);
}

void SDLConsoleDriver::hook_df_renderer(bool enabled)
{
    INTERPOSE_HOOK(DFHack::con_render_hook,render).apply(enabled);
}

/*
 * Cleanup must be done from the main thread.
 * NOTE: may be able to do this in the destructor instead.
 */
void SDLConsoleDriver::cleanup()
{
    hook_df_renderer(false);
    d->con.destroy_session();
    inited.store(false);
}


