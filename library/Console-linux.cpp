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
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <errno.h>
#include <deque>

#include "dfhack/Console.h"
#include "dfhack/FakeSDL.h"
using namespace DFHack;

#include "tinythread.h"
using namespace tthread;

static int isUnsupportedTerm(void)
{
    static const char *unsupported_term[] = {"dumb","cons25",NULL};
    char *term = getenv("TERM");
    int j;

    if (term == NULL) return 0;
    for (j = 0; unsupported_term[j]; j++)
        if (!strcasecmp(term,unsupported_term[j])) return 1;
        return 0;
}

const char * ANSI_CLS = "\033[2J";
const char * ANSI_BLACK = "\033[22;30m";
const char * ANSI_RED = "\033[22;31m";
const char * ANSI_GREEN = "\033[22;32m";
const char * ANSI_BROWN = "\033[22;33m";
const char * ANSI_BLUE = "\033[22;34m";
const char * ANSI_MAGENTA = "\033[22;35m";
const char * ANSI_CYAN = "\033[22;36m";
const char * ANSI_GREY = "\033[22;37m";
const char * ANSI_DARKGREY = "\033[01;30m";
const char * ANSI_LIGHTRED = "\033[01;31m";
const char * ANSI_LIGHTGREEN = "\033[01;32m";
const char * ANSI_YELLOW = "\033[01;33m";
const char * ANSI_LIGHTBLUE = "\033[01;34m";
const char * ANSI_LIGHTMAGENTA = "\033[01;35m";
const char * ANSI_LIGHTCYAN = "\033[01;36m";
const char * ANSI_WHITE = "\033[01;37m";
const char * RESETCOLOR = "\033[0m";

const char * getANSIColor(const int c)
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

namespace DFHack
{
    class Private : public std::stringbuf
    {
    public:
        Private()
        {
            dfout_C = NULL;
            rawmode = false;
            supported_terminal = false;
            state = con_unclaimed;
        };
        virtual ~Private()
        {
            //sync();
        }
    protected:
        int sync()
        {
            print(str().c_str());
            str(std::string());    // Clear the string buffer
            return 0;
        }
    public:
        /// Print a formatted string, like printf
        int  print(const char * format, ...)
        {
            va_list args;
            va_start( args, format );
            int ret = vprint( format, args );
            va_end( args );
            return ret;
        }
        int  vprint(const char * format, va_list vl)
        {
            if(state == con_lineedit)
            {
                disable_raw();
                fprintf(dfout_C,"\x1b[1G");
                fprintf(dfout_C,"\x1b[0K");
                int ret = vfprintf( dfout_C, format, vl );
                enable_raw();
                prompt_refresh();
                return ret;
            }
            else return vfprintf( dfout_C, format, vl );
        }
        int  vprinterr(const char * format, va_list vl)
        {
            if(state == con_lineedit)
            {
                disable_raw();
                color(Console::COLOR_LIGHTRED);
                fprintf(dfout_C,"\x1b[1G");
                fprintf(dfout_C,"\x1b[0K");
                int ret = vfprintf( dfout_C, format, vl );
                reset_color();
                enable_raw();
                prompt_refresh();
                return ret;
            }
            else
            {
                color(Console::COLOR_LIGHTRED);
                int ret = vfprintf( dfout_C, format, vl );
                reset_color();
                return ret;
            }
        }
        /// Print a formatted string, like printf, in red
        int  printerr(const char * format, ...)
        {
            va_list args;
            va_start( args, format );
            int ret = vprinterr( format, args );
            va_end( args );
            return ret;
        }
        /// Clear the console, along with its scrollback
        void clear()
        {
            if(rawmode)
            {
                const char * clr = "\033c\033[3J\033[H";
                ::write(STDIN_FILENO,clr,strlen(clr));
            }
            else
            {
                print("\033c\033[3J\033[H");
                fflush(dfout_C);
            }
        }
        /// Position cursor at x,y. 1,1 = top left corner
        void gotoxy(int x, int y)
        {
            print("\033[%d;%dH", y,x);
        }
        /// Set color (ANSI color number)
        void color(Console::color_value index)
        {
            if(!rawmode)
                fprintf(dfout_C,getANSIColor(index));
            else
            {
                const char * colstr = getANSIColor(index);
                int lstr = strlen(colstr);
                ::write(STDIN_FILENO,colstr,lstr);
            }
        }
        /// Reset color to default
        void reset_color(void)
        {
            color(Console::COLOR_RESET);
            if(!rawmode)
                fflush(dfout_C);
        }
        /// Enable or disable the caret/cursor
        void cursor(bool enable = true)
        {
            if(enable)
                print("\033[?25h");
            else
                print("\033[?25l");
        }
        /// Waits given number of milliseconds before continuing.
        void msleep(unsigned int msec);
        /// get the current number of columns
        int  get_columns(void)
        {
            winsize ws;
            if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1) return 80;
            return ws.ws_col;
        }
        /// get the current number of rows
        int  get_rows(void)
        {
            winsize ws;
            if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1) return 25;
            return ws.ws_row;
        }
        /// beep. maybe?
        //void beep (void);
        /// A simple line edit (raw mode)
        int lineedit(const std::string& prompt, std::string& output, mutex * lock)
        {
            output.clear();
            this->prompt = prompt;
            if (!supported_terminal)
            {
                print(prompt.c_str());
                fflush(dfout_C);
                // FIXME: what do we do here???
                //SDL_mutexV(lock);
                std::getline(std::cin, output);
                //SDL_mutexP(lock);
                return output.size();
            }
            else
            {
                int count;
                if (enable_raw() == -1) return 0;
                if(state == con_lineedit)
                    return -1;
                state = con_lineedit;
                count = prompt_loop(lock);
                state = con_unclaimed;
                disable_raw();
                print("\n");
                if(count != -1)
                {
                    output = raw_buffer;
                }
                return count;
            }
        }
        /// add a command to the history
        void history_add(const std::string& command)
        {
            // if current command = last in history -> do not add. Always add if history is empty.
            if(!history.empty() && history.front() == command)
                return;
            history.push_front(command);
            if(history.size() > 100)
                history.pop_back();
        }
        /// clear the command history
        void history_clear();

        int enable_raw()
        {
            struct termios raw;

            if (!supported_terminal)
                return -1;
            if (tcgetattr(STDIN_FILENO,&orig_termios) == -1)
                return -1;

            raw = orig_termios; //modify the original mode
            // input modes: no break, no CR to NL, no parity check, no strip char,
            // no start/stop output control.
            raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
            // output modes - disable post processing
            raw.c_oflag &= ~(OPOST);
            // control modes - set 8 bit chars
            raw.c_cflag |= (CS8);
            // local modes - choing off, canonical off, no extended functions,
            // no signal chars (^Z,^C)
            raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
            // control chars - set return condition: min number of bytes and timer.
            // We want read to return every single byte, without timeout.
            raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0;// 1 byte, no timer
            // put terminal in raw mode after flushing
            if (tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw) < 0)
                return -1;
            rawmode = 1;
            return 0;
        }

        void disable_raw()
        {
            /* Don't even check the return value as it's too late. */
            if (rawmode && tcsetattr(STDIN_FILENO,TCSAFLUSH,&orig_termios) != -1)
                rawmode = 0;
        }
        void prompt_refresh()
        {
            char seq[64];
            int cols = get_columns();
            int plen = prompt.size();
            const char * buf = raw_buffer.c_str();
            int len = raw_buffer.size();
            int cooked_cursor = raw_cursor;
            // Use math! This is silly.
            while((plen+cooked_cursor) >= cols)
            {
                buf++;
                len--;
                cooked_cursor--;
            }
            while (plen+len > cols)
            {
                len--;
            }
            /* Cursor to left edge */
            snprintf(seq,64,"\x1b[1G");
            if (::write(STDIN_FILENO,seq,strlen(seq)) == -1) return;
            /* Write the prompt and the current buffer content */
            if (::write(STDIN_FILENO,prompt.c_str(),plen) == -1) return;
            if (::write(STDIN_FILENO,buf,len) == -1) return;
            /* Erase to right */
            snprintf(seq,64,"\x1b[0K");
            if (::write(STDIN_FILENO,seq,strlen(seq)) == -1) return;
            /* Move cursor to original position. */
            snprintf(seq,64,"\x1b[1G\x1b[%dC", (int)(cooked_cursor+plen));
            if (::write(STDIN_FILENO,seq,strlen(seq)) == -1) return;
        }

        int prompt_loop(mutex * lock)
        {
            int fd = STDIN_FILENO;
            size_t plen = prompt.size();
            int history_index = 0;
            raw_buffer.clear();
            raw_cursor = 0;
            /* The latest history entry is always our current buffer, that
             * initially is just an empty string. */
            const std::string empty;
            history_add(empty);
            if (::write(fd,prompt.c_str(),prompt.size()) == -1) return -1;
            while(1)
            {
                char c;
                int nread;
                char seq[2], seq2;
                lock->unlock();
                nread = ::read(fd,&c,1);
                lock->lock();
                if (nread <= 0) return raw_buffer.size();

                /* Only autocomplete when the callback is set. It returns < 0 when
                 * there was an error reading from fd. Otherwise it will return the
                 * character that should be handled next. */
                if (c == 9)
                {
                    /*
                    if( completionCallback != NULL) {
                        c = completeLine(fd,prompt,buf,buflen,&len,&pos,cols);
                        // Return on errors
                        if (c < 0) return len;
                        // Read next character when 0
                        if (c == 0) continue;
                    }
                    else
                    {
                        // ignore tab
                        continue;
                    }
                    */
                    // just ignore tabs
                    continue;
                }

                switch(c)
                {
                case 13:    // enter
                    history.pop_front();
                    return raw_buffer.size();
                case 3:     // ctrl-c
                    errno = EAGAIN;
                    return -1;
                case 127:   // backspace
                case 8:     // ctrl-h
                    if (raw_cursor > 0 && raw_buffer.size() > 0)
                    {
                        raw_buffer.erase(raw_cursor-1,1);
                        raw_cursor--;
                        prompt_refresh();
                    }
                    break;
                case 27:    // escape sequence
                    lock->unlock();
                    if (::read(fd,seq,2) == -1)
                    {
                        lock->lock();
                        break;
                    }
                    lock->lock();
                    if(seq[0] == '[')
                    {
                        if (seq[1] == 'D')
                        {
                            left_arrow:
                            if (raw_cursor > 0)
                            {
                                raw_cursor--;
                                prompt_refresh();
                            }
                        }
                        else if ( seq[1] == 'C')
                        {
                            right_arrow:
                            /* right arrow */
                            if (raw_cursor != raw_buffer.size())
                            {
                                raw_cursor++;
                                prompt_refresh();
                            }
                        }
                        else if (seq[1] == 'A' || seq[1] == 'B')
                        {
                            /* up and down arrow: history */
                            if (history.size() > 1)
                            {
                                /* Update the current history entry before to
                                 * overwrite it with tne next one. */
                                history[history_index] = raw_buffer;
                                /* Show the new entry */
                                history_index += (seq[1] == 'A') ? 1 : -1;
                                if (history_index < 0)
                                {
                                    history_index = 0;
                                    break;
                                }
                                else if (history_index >= history.size())
                                {
                                    history_index = history.size()-1;
                                    break;
                                }
                                raw_buffer = history[history_index];
                                raw_cursor = raw_buffer.size();
                                prompt_refresh();
                            }
                        }
                        else if(seq[1] == 'H')
                        {
                            // home
                            raw_cursor = 0;
                            prompt_refresh();
                        }
                        else if(seq[1] == 'F')
                        {
                            // end
                            raw_cursor = raw_buffer.size();
                            prompt_refresh();
                        }
                        else if (seq[1] > '0' && seq[1] < '7')
                        {
                            // extended escape
                            lock->unlock();
                            if (::read(fd,&seq2,1) == -1)
                            {
                                lock->lock();
                                return -1;
                            }
                            lock->lock();
                            if (seq[1] == '3' && seq2 == '~' )
                            {
                                // delete
                                if (raw_buffer.size() > 0 && raw_cursor < raw_buffer.size())
                                {
                                    raw_buffer.erase(raw_cursor,1);
                                    prompt_refresh();
                                }
                            }
                        }
                    }
                    break;
                default:
                    if (raw_buffer.size() == raw_cursor)
                    {
                        raw_buffer.append(1,c);
                        raw_cursor++;
                        if (plen+raw_buffer.size() < get_columns())
                        {
                            /* Avoid a full update of the line in the
                             * trivial case. */
                            if (::write(fd,&c,1) == -1) return -1;
                        }
                        else
                        {
                            prompt_refresh();
                        }
                    }
                    else
                    {
                        raw_buffer.insert(raw_cursor,1,c);
                        raw_cursor++;
                        prompt_refresh();
                    }
                    break;
                case 21: // Ctrl+u, delete the whole line.
                    raw_buffer.clear();
                    raw_cursor = 0;
                    prompt_refresh();
                    break;
                case 11: // Ctrl+k, delete from current to end of line.
                    raw_buffer.erase(raw_cursor);
                    prompt_refresh();
                    break;
                case 1: // Ctrl+a, go to the start of the line
                    raw_cursor = 0;
                    prompt_refresh();
                    break;
                case 5: // ctrl+e, go to the end of the line
                    raw_cursor = raw_buffer.size();
                    prompt_refresh();
                    break;
                case 12: // ctrl+l, clear screen
                    clear();
                    prompt_refresh();
                }
            }
            return raw_buffer.size();
        }
        FILE * dfout_C;
        std::deque <std::string> history;
        bool supported_terminal;
        // state variables
        bool rawmode;           // is raw mode active?
        termios orig_termios;   // saved/restored by raw mode
        // current state
        enum console_state
        {
            con_unclaimed,
            con_lineedit
        } state;
        std::string prompt;     // current prompt string
        std::string raw_buffer; // current raw mode buffer
        int raw_cursor;         // cursor position in the buffer
    };
}

Console::Console():std::ostream(0), std::ios(0)
{
    d = 0;
    inited = false;
    // we can't create the mutex at this time. the SDL functions aren't hooked yet.
    wlock = new mutex();
}
Console::~Console()
{
    if(inited)
        shutdown();
    if(wlock)
        delete wlock;
    if(d)
        delete d;
}

bool Console::init(bool sharing)
{
    if(sharing)
    {
        inited = false;
        return false;
    }
    freopen("stdout.log", "w", stdout);
    freopen("stderr.log", "w", stderr);
    d = new Private();
    // make our own weird streams so our IO isn't redirected
    d->dfout_C = fopen("/dev/tty", "w");
    rdbuf(d);
    std::cin.tie(this);
    clear();
    d->supported_terminal = !isUnsupportedTerm() &&  isatty(STDIN_FILENO);
    inited = true;
}

bool Console::shutdown(void)
{
    if(!d)
        return true;
    lock_guard <mutex> g(*wlock);
    if(d->rawmode)
        d->disable_raw();
    d->print("\n");
    inited = false;
    return true;
}

int Console::print( const char* format, ... )
{
    va_list args;
    lock_guard <mutex> g(*wlock);
    int ret;
    if(!inited) ret = -1;
    else
    {
        va_start( args, format );
        ret = d->vprint(format, args);
        va_end(args);
    }
    return ret;
}

int Console::printerr( const char* format, ... )
{
    va_list args;
    lock_guard <mutex> g(*wlock);
    int ret;
    if(!inited) ret = -1;
    else
    {
        va_start( args, format );
        ret = d->vprinterr(format, args);
        va_end(args);
    }
    return ret;
}

int Console::get_columns(void)
{
    lock_guard <mutex> g(*wlock);
    int ret = -1;
    if(inited)
        ret = d->get_columns();
    return ret;
}

int Console::get_rows(void)
{
    lock_guard <mutex> g(*wlock);
    int ret = -1;
    if(inited)
        ret = d->get_rows();
    return ret;
}

void Console::clear()
{
    lock_guard <mutex> g(*wlock);
    if(inited)
        d->clear();
}

void Console::gotoxy(int x, int y)
{
    lock_guard <mutex> g(*wlock);
    if(inited)
        d->gotoxy(x,y);
}

void Console::color(color_value index)
{
    lock_guard <mutex> g(*wlock);
    if(inited)
        d->color(index);
}

void Console::reset_color( void )
{
    lock_guard <mutex> g(*wlock);
    if(inited)
        d->reset_color();
}

void Console::cursor(bool enable)
{
    lock_guard <mutex> g(*wlock);
    if(inited)
        d->cursor(enable);
}

// push to front, remove from back if we are above maximum. ignore immediate duplicates
void Console::history_add(const std::string & command)
{
    lock_guard <mutex> g(*wlock);
    if(inited)
        d->history_add(command);
}

int Console::lineedit(const std::string & prompt, std::string & output)
{
    lock_guard <mutex> g(*wlock);
    int ret = -2;
    if(inited)
        ret = d->lineedit(prompt,output,wlock);
    return ret;
}

void Console::msleep (unsigned int msec)
{
    if (msec > 1000) sleep(msec/1000000);
    usleep((msec % 1000000) * 1000);
}