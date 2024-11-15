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

#include "df/renderer_2d.h"
#include <VTableInterpose.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <csignal>
#include <fcntl.h>
#include <SDL_pixels.h>

#include "Console.h"
#include "SDL_console.h"

#include "Hooks.h"
#include "modules/DFSDL.h"
using namespace DFHack;

using namespace sdl_console;

SDLConsole *conPtr = nullptr;
struct con_render_hook : df::renderer_2d {
    typedef df::renderer_2d interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        conPtr->update();
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
        Private() : con(SDLConsole::get_console()) {conPtr = &con;};
        virtual ~Private() {}
    private:

    public:
        bool init()
        {
            con.set_prompt(prompt);
            return con.init();
        }

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
            if (!con.state.is_active()) {
                fputs("SDLConsole is inactive\n", stderr);
                return Console::SHUTDOWN;
            }

            if (prompt != this->prompt) {
                con.set_prompt(prompt);
                this->prompt = prompt;
            }

            int ret = con.get_line(output);
            if (ret == 0)
                return Console::RETRY;

            if (ret == Console::FAILURE) {
                fputs("SDLConsole is inactive (shutting down)\n", stderr);
            }
            return ret;
        }

        void begin_batch()
        {
        }

        void end_batch()
        {
        }

        void flush()
        {
        }

        /// Clear the console, along with its scrollback
        void clear()
        {
            con.clear();
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

        /// Waits given number of milliseconds before continuing.
        void msleep(unsigned int msec);

        int  get_columns(void)
        {
            return con.get_columns();
        }

        int  get_rows(void)
        {
            return con.get_rows();
        }

        SDLConsole& con;
        bool in_batch{false};
        std::string prompt;      // current prompt string
    };
}

Console::Console()
{
    d = 0;
    inited.store(false);
    // we can't create the mutex at this time. the SDL functions aren't hooked yet.
    wlock = new std::recursive_mutex();
}
Console::~Console()
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
 **/
bool Console::init(bool dont_redirect)
{
    if (inited.load()) {
        INTERPOSE_HOOK(con_render_hook,render).apply(true);
        return true;
    }

    d = new Private();
    inited.store(d->init());

    if (!dont_redirect)
    {
        if (freopen("stdout.log", "w", stdout) == nullptr) {
            fputs("Failed to redirect stdout to file\n", stderr);
        }
    }
    return inited;
}

bool Console::shutdown(void)
{
    if(!inited.load())
        return true;
    d->con.shutdown();
    inited = false;
    return true;
}

void Console::begin_batch()
{
    wlock->lock();

    if(inited.load())
        d->begin_batch();
}

void Console::end_batch()
{
    if(inited.load())
        d->end_batch();

    wlock->unlock();
}
/* XXX: Not implemented */
void Console::flush_proxy()
{
    return;
    std::lock_guard <std::recursive_mutex> g(*wlock);
    if (inited)
        d->flush();
}

void Console::add_text(color_value color, const std::string &text)
{
    std::lock_guard <std::recursive_mutex> g(*wlock);
    if(inited.load())
        d->print_text(color, text);
    else
        fwrite(text.data(), 1, text.size(), stderr);
}

int Console::get_columns(void)
{
    int ret = Console::FAILURE;
    if (inited.load()) {
        ret = d->get_columns();
    }
    return ret;
}

int Console::get_rows(void)
{
    auto ret = Console::FAILURE;
    if (inited.load()) {
        ret = d->get_rows();
    }
    return ret;
}

void Console::clear()
{
    if (inited.load())
        d->con.clear();
}
/* XXX: Not implemented */
void Console::gotoxy(int x, int y)
{
}
/* XXX: Not implemented */
void Console::cursor(bool enable)
{
    if (inited.load())
        d->cursor(enable);
}

int Console::lineedit(const std::string & prompt, std::string & output, CommandHistory & ch)
{
    if(!inited.load())
        return Console::SHUTDOWN;

    int ret = d->lineedit(prompt,output,ch);
    if (ret == 0)
        return Console::RETRY;
    return ret;
}

void Console::msleep (unsigned int msec)
{
    if (msec > 1000) sleep(msec/1000000);
    usleep((msec % 1000000) * 1000);
}

bool Console::hide()
{
    if (!inited.load())
        return false;
    d->con.hide_window();
    return true;
}

bool Console::show()
{
    if(!inited.load())
        return false;
    d->con.show_window();
    return true;
}

/*
 * cleanup() here is necessary if the console
 * failed after init, or if commanded to shutdown
 * during run time (not exiting).
 */
bool Console::sdl_event_hook(SDL_Event &e)
{
    if (!d) {
        return false;
    } else if (d->con.state.is_active()) {
        return d->con.sdl_event_hook(e);
    } else if (d->con.state.is_shutdown()) {
        cleanup();
    }
    return false;
}

/*
 * Called by df main thread.
 * 'd' is initialized by main thread.
 * We don't need to check the atomic 'inited' here,
 * as it'll be set to false on shutdown.
 * Cleanup must be done from the main thread.
 * NOTE: This may be achievable in the destructor instead.
 */
void Console::cleanup()
{
    if (!d)
        return;
    INTERPOSE_HOOK(con_render_hook,render).apply(0);
    // destroy() will change its state to inactive
    d->con.destroy();
    std::cerr << "SDLConsole destroyed" << std::endl;
    inited.store(false);
}
