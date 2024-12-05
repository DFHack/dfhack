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

#include "df/renderer_2d.h"
#include <VTableInterpose.h>

#include "SDLConsoleDriver.h"
#include "SDLConsole.h"

using namespace DFHack;

using namespace sdl_console;

struct con_render_hook : df::renderer_2d {
    typedef df::renderer_2d interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        SDLConsole::get_console().update();
        INTERPOSE_NEXT(render)();
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(con_render_hook, render);

namespace DFHack
{
#if 0
    //! Convert a locale defined multibyte coding to UTF-32 string for easier
    //! character processing.
    static u32string fromLocaleMB(const std::string& str)
    {
        u32string rv;
        u32string::value_type ch;
        size_t pos = 0;
        ssize_t sz;
        std::mbstate_t state{};
        while ((sz = mbrtoc32(&ch,&str[pos], str.size() - pos, &state)) != 0) {
            if (sz == -1 || sz == -2)
                break;
            rv.push_back(ch);
            if (sz == -3) /* multi value character */
                continue;
            pos += sz;
        }
        return rv;
    }

    //! Convert a UTF-32 string back to locale defined multibyte coding.
    static std::string toLocaleMB(const u32string& wstr)
    {
        std::stringstream ss{};
        char mb[MB_CUR_MAX];
        std::mbstate_t state{};
        const size_t err = -1;
        for (auto ch: wstr) {
            size_t sz = c32rtomb(mb, ch, &state);
            if (sz == err)
                break;
            ss.write(mb, sz);
        }
        return ss.str();
    }
#endif
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

    // Function to get the color
    SDL_Color getANSIColor(int c)
    {
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
        Private() : con(SDLConsole::get_console()) {};
        virtual ~Private() = default;
    private:

    public:

        void print(const char *data)
        {
            con.write_line(data);
        }

        void print_text(color_ostream::color_value clr, const std::string &chunk)
        {
            con.write_line(chunk, getANSIColor(clr));
        }

        int lineedit(const std::string& prompt, std::string& output, CommandHistory & ch)
        {
            static bool did_set_history = false;

            // I don't believe this check is necessary.
            // unless, somwhow, fiothread is inited before the console.
            if (con.state.is_inactive()) {
                return Console::RETRY;
            }
            // kludge. This is the only place to set it?
            if (!did_set_history) {
                std::vector<std::string> hist;
                ch.getEntries(hist);
                con.set_command_history(hist);
                did_set_history = true;
            }

            if (prompt != this->prompt) {
                con.set_prompt(prompt);
                this->prompt = prompt;
            }

            int ret = con.get_line(output);
            if (ret == 0)
                return Console::RETRY;
            else if (ret == -1)
                return Console::SHUTDOWN;

            return ret;
        }
        /// Position cursor at x,y. 1,1 = top left corner
        void gotoxy(int x, int y)
        {
        }

        /// Set color (ANSI color number)
        /// Note: unimplemented because widgets share the same font atlas,
        /// which will change color for all widgets.
        void color(Console::color_value index)
        {
        }

        /// Set color (ANSI color number)
        /// Note: unimplemented because widgets share the same font atlas,
        /// which will change color for all widgets.
        void reset_color(void)
        {
        }

        /// Enable or disable the caret/cursor
        void cursor(bool enable = true)
        {
        }

        SDLConsole &con;
        std::string prompt;      // current prompt string
    };
}

SDLConsoleDriver::SDLConsoleDriver() : Console(Console::Type::SDL)
{
    d = new Private();
    inited.store(false);
    // we can't create the mutex at this time. the SDL functions aren't hooked yet.
    wlock = new std::recursive_mutex();
}
SDLConsoleDriver::~SDLConsoleDriver()
{
    assert(!inited);
    if(wlock)
        delete wlock;
    if(d)
        delete d;
}

/**
 * FIXME: Two-stage init because we need to initialize on
 * the main thread, but interpose isn't available until later.
 */
bool SDLConsoleDriver::init(bool dont_redirect)
{
    if (inited)
        INTERPOSE_HOOK(con_render_hook,render).apply(true);

    if (!dont_redirect)
    {
        if (freopen("stdout.log", "w", stdout) == nullptr) {
            fputs("Failed to redirect stdout to file\n", stderr);
        }
    }
    return inited.load();
}

bool SDLConsoleDriver::init_sdl()
{
    inited.store(d->con.init());
    return inited.load();
}

bool SDLConsoleDriver::shutdown(void)
{
    if (!inited.load()) return true;
    d->con.shutdown();
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
        d->print_text(color, text);
    else
        fwrite(text.data(), 1, text.size(), stderr);
}

int SDLConsoleDriver::get_columns(void)
{
    // returns Console::FAILURE if inactive
    return d->con.get_columns();
}

int SDLConsoleDriver::get_rows(void)
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

int SDLConsoleDriver::lineedit(const std::string & prompt, std::string & output, CommandHistory & ch)
{
    // Tell fiothread we are done.
    if(!inited.load())
        return Console::SHUTDOWN;

    return d->lineedit(prompt,output,ch);
}

void SDLConsoleDriver::msleep (unsigned int msec)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(msec));
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
 * or if commanded to shutdown during run time (but df not exiting).
 *
 * NOTE: We do not absolutely have to clean up here. It can be done at exit.
 * This is for the ability to shutdown and restart the console at run time.
 */
bool SDLConsoleDriver::sdl_event_hook(SDL_Event &e)
{
    auto& con = d->con;
    if (con.state.is_active()) [[likely]] {
        return con.sdl_event_hook(e);
    } else if (con.state.is_shutdown()) {
        cleanup();
    }
    return false;
}

/*
 * Cleanup must be done from the main thread.
 * NOTE: may be able to do this in the destructor instead.
 */
void SDLConsoleDriver::cleanup()
{
    INTERPOSE_HOOK(con_render_hook,render).apply(false);
    // destroy() will change console's state to inactive
    d->con.destroy();
    inited.store(false);
}
