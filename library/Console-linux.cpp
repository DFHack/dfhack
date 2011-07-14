/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

/* 
Parts of this code are based on linenoise:
linenoise.c -- guerrilla line editing library against the idea that a
line editing lib needs to be 20,000 lines of C code.

You can find the latest source code at:

  http://github.com/antirez/linenoise

Does a number of crazy assumptions that happen to be true in 99.9999% of
the 2010 UNIX computers around.

------------------------------------------------------------------------

Copyright (c) 2010, Salvatore Sanfilippo <antirez at gmail dot com>
Copyright (c) 2010, Pieter Noordhuis <pcnoordhuis at gmail dot com>

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

#include "dfhack/Console.h"
#include "dfhack/extra/stdiostream.h"
#include <cstdio>
#include <cstdlib>
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
using namespace DFHack;

#define LINENOISE_DEFAULT_HISTORY_MAX_LEN 100
#define LINENOISE_MAX_LINE 4096

static const char *unsupported_term[] = {"dumb","cons25",NULL};
namespace DFHack
{
    class Private
    {
    public:
        Private()
        {
            dfout_C = 0;
            stream_o = 0;
            rawmode = 0;
        };

        int isUnsupportedTerm(void)
        {
            char *term = getenv("TERM");
            int j;

            if (term == NULL) return 0;
            for (j = 0; unsupported_term[j]; j++)
                if (!strcasecmp(term,unsupported_term[j])) return 1;
                return 0;
        }

        FILE * dfout_C;
        duthomhas::stdiobuf * stream_o;
        termios orig_termios; /* in order to restore at exit */
        int rawmode; /* for atexit() function to check if restore is needed*/
        std::deque <std::string> history;
    };
}
Console::Console():std::ostream(0), std::ios(0)
{
    d = 0;
}
Console::~Console()
{
    if(d)
        delete d;
}

bool Console::init(void)
{
    d = new Private();
    // make our own weird streams so our IO isn't redirected
    d->dfout_C = fopen("/dev/tty", "w");
    d->stream_o = new duthomhas::stdiobuf(d->dfout_C);
    rdbuf(d->stream_o);
    std::cin.tie(this);
    clear();
}

bool Console::shutdown(void)
{
    if(d->rawmode)
        disable_raw();
    print("\n");
}

int Console::print( const char* format, ... )
{
    va_list args;
    va_start( args, format );
    int ret = vfprintf( d->dfout_C, format, args );
    va_end( args );
    return ret;
}

int Console::get_columns(void)
{
    winsize ws;
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1) return 80;
    return ws.ws_col;
}

int Console::get_rows(void)
{
    winsize ws;
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1) return 25;
    return ws.ws_row;
}

void Console::clear()
{
    if(d->rawmode)
    {
        const char * clr = "\033c\033[3J\033[H";
        ::write(STDIN_FILENO,clr,strlen(clr));
    }
    else
    {
        print("\033c\033[3J\033[H");
    }
}

void Console::gotoxy(int x, int y)
{
    print("\033[%d;%dH", y,x);
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

void Console::color(int index)
{
    print(getANSIColor(index));
}

void Console::reset_color( void )
{
    print(RESETCOLOR);
}

void Console::cursor(bool enable)
{
    if(enable)
        print("\033[?25h");
    else
        print("\033[?25l");
}

void Console::msleep (unsigned int msec)
{
    if (msec > 1000) sleep(msec/1000000);
    usleep((msec % 1000000) * 1000);
}

int Console::enable_raw()
{
    struct termios raw;

    if (!isatty(STDIN_FILENO)) goto fatal;
    if (tcgetattr(STDIN_FILENO,&d->orig_termios) == -1) goto fatal;

    raw = d->orig_termios;  /* modify the original mode */
    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* output modes - disable post processing */
    raw.c_oflag &= ~(OPOST);
    /* control modes - set 8 bit chars */
    raw.c_cflag |= (CS8);
    /* local modes - choing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* control chars - set return condition: min number of bytes and timer.
     * We want read to return every single byte, without timeout. */
    raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */

    /* put terminal in raw mode after flushing */
    if (tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw) < 0) goto fatal;
    d->rawmode = 1;
    return 0;

fatal:
    errno = ENOTTY;
    return -1;
}

void Console::disable_raw()
{
    /* Don't even check the return value as it's too late. */
    if (d->rawmode && tcsetattr(STDIN_FILENO,TCSAFLUSH,&d->orig_termios) != -1)
        d->rawmode = 0;
}

void Console::prompt_refresh( const std::string& prompt, const std::string& buffer, size_t pos)
{
    char seq[64];
    int cols = get_columns();
    int plen = prompt.size();
    const char * buf = buffer.c_str();
    int len = buffer.size();
    // Use math! This is silly.
    while((plen+pos) >= cols)
    {
        buf++;
        len--;
        pos--;
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
    snprintf(seq,64,"\x1b[1G\x1b[%dC", (int)(pos+plen));
    if (::write(STDIN_FILENO,seq,strlen(seq)) == -1) return;
}

int Console::prompt_loop(const std::string & prompt, std::string & buffer)
{
    int fd = STDIN_FILENO;
    size_t plen = prompt.size();
    size_t pos = 0;
    size_t cols = get_columns();
    int history_index = 0;

    /* The latest history entry is always our current buffer, that
     * initially is just an empty string. */
    const std::string empty;
    history_add(empty);
    if (::write(fd,prompt.c_str(),plen) == -1) return -1;
    while(1)
    {
        char c;
        int nread;
        char seq[2], seq2[2];

        nread = ::read(fd,&c,1);
        if (nread <= 0) return buffer.size();

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
        case 13:    /* enter */
            d->history.pop_front();
            return buffer.size();
        case 3:     /* ctrl-c */
            errno = EAGAIN;
            return -1;
        case 127:   /* backspace */
        case 8:     /* ctrl-h */
            if (pos > 0 && buffer.size() > 0)
            {
                buffer.erase(pos-1,1);
                pos--;
                prompt_refresh(prompt,buffer,pos);
            }
            break;
        case 27:    /* escape sequence */
            if (::read(fd,seq,2) == -1) break;
            if(seq[0] == '[')
            {
                if (seq[1] == 'D')
                {
                    left_arrow:
                    if (pos > 0)
                    {
                        pos--;
                        prompt_refresh(prompt,buffer,pos);
                    }
                }
                else if ( seq[1] == 'C')
                {
                    right_arrow:
                    /* right arrow */
                    if (pos != buffer.size())
                    {
                        pos++;
                        prompt_refresh(prompt,buffer,pos);
                    }
                }
                else if (seq[1] == 'A' || seq[1] == 'B')
                {
                    /* up and down arrow: history */
                    if (d->history.size() > 1)
                    {
                        /* Update the current history entry before to
                         * overwrite it with tne next one. */
                        d->history[history_index] = buffer;
                        /* Show the new entry */
                        history_index += (seq[1] == 'A') ? 1 : -1;
                        if (history_index < 0)
                        {
                            history_index = 0;
                            break;
                        }
                        else if (history_index >= d->history.size())
                        {
                            history_index = d->history.size()-1;
                            break;
                        }
                        buffer = d->history[history_index];
                        pos = buffer.size();
                        prompt_refresh(prompt,buffer,pos);
                    }
                }
                else if(seq[1] == 'H')
                {
                    // home
                    pos = 0;
                    prompt_refresh(prompt,buffer,pos);
                }
                else if(seq[1] == 'F')
                {
                    // end
                    pos = buffer.size();
                    prompt_refresh(prompt,buffer,pos);
                }
                else if (seq[1] > '0' && seq[1] < '7')
                {
                    // extended escape
                    if (::read(fd,seq2,2) == -1) break;
                    if (seq2[0] == '~' && seq[1] == '3')
                    {
                        // delete
                        if (buffer.size() > 0 && pos < buffer.size())
                        {
                            buffer.erase(pos,1);
                            prompt_refresh(prompt,buffer,pos);
                        }
                    }
                }
            }
            break;
        default:
            if (buffer.size() == pos)
            {
                buffer.append(1,c);
                pos++;
                if (plen+buffer.size() < cols)
                {
                    /* Avoid a full update of the line in the
                     * trivial case. */
                    if (::write(fd,&c,1) == -1) return -1;
                }
                else
                {
                    prompt_refresh(prompt,buffer,pos);
                }
            }
            else
            {
                buffer.insert(pos,1,c);
                pos++;
                prompt_refresh(prompt,buffer,pos);
            }
            break;
        case 21: // Ctrl+u, delete the whole line.
            buffer.clear();
            pos = 0;
            prompt_refresh(prompt,buffer,pos);
            break;
        case 11: // Ctrl+k, delete from current to end of line.
            buffer.erase(pos);
            prompt_refresh(prompt,buffer,pos);
            break;
        case 1: // Ctrl+a, go to the start of the line
            pos = 0;
            prompt_refresh(prompt,buffer,pos);
            break;
        case 5: // ctrl+e, go to the end of the line
            pos = buffer.size();
            prompt_refresh(prompt,buffer,pos);
            break;
        case 12: // ctrl+l, clear screen
            clear();
            prompt_refresh(prompt,buffer,pos);
        }
    }
    return buffer.size();
}
// push to front, remove from back if we are above maximum. ignore immediate duplicates
void Console::history_add(const std::string & command)
{
    if(!d->history.empty() && d->history.front() == command)
        return;
    d->history.push_front(command);
    if(d->history.size() > 100)
        d->history.pop_back();
}

int Console::lineedit(const std::string & prompt, std::string & output)
{
    output.clear();
    int count;
    if (d->isUnsupportedTerm() || !isatty(STDIN_FILENO))
    {
        print(prompt.c_str());
        fflush(d->dfout_C);
        std::getline(std::cin, output);
        return output.size();
    }
    else
    {
        if (enable_raw() == -1) return 0;
        count = prompt_loop(prompt, output);
        disable_raw();
        print("\n");
        return output.size();
    }
}