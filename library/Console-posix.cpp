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

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string.h>
#include <string>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <errno.h>
#include <deque>
#ifdef HAVE_CUCHAR
#include <cuchar>
#else
#include <cwchar>
#endif

// George Vulov for MacOSX
#ifndef __LINUX__
#define TMP_FAILURE_RETRY(expr) \
    ({ long int _res; \
        do _res = (long int) (expr); \
        while (_res == -1L && errno == EINTR); \
        _res; })
#endif

#include "PosixConsole.h"
#include "Hooks.h"
using namespace DFHack;

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


#ifdef HAVE_CUCHAR
// Use u32string for GCC 6 and later and msvc to allow potable implementation
using u32string = std::u32string;
using std::c32rtomb;
using std::mbrtoc32;
#else
// Fallback for gcc 4 and 5 that don't have cuchar header
// But wchar_t is 4 bytes that is a good fallback implementation
using u32string = std::wstring;
size_t mbrtoc32(u32string::value_type* c,
        const char* s,
        std::size_t n,
        std::mbstate_t* ps)
{
    return std::mbrtowc(c, s, n, ps);
}

size_t c32rtomb(char* mb,
        u32string::value_type c,
        std::mbstate_t* ps)
{
    return std::wcrtomb(mb, c, ps);
}
#endif

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

namespace DFHack
{
    class Private
    {
    public:
        Private()
        {
            dfout_C = NULL;
            rawmode = false;
            in_batch = false;
            supported_terminal = false;
            state = con_unclaimed;
        };
        virtual ~Private()
        {
            //sync();
        }
    private:
        bool read_char(unsigned char & out)
        {
            FD_ZERO(&descriptor_set);
            FD_SET(STDIN_FILENO, &descriptor_set);
            FD_SET(exit_pipe[0], &descriptor_set);
            int ret = TMP_FAILURE_RETRY(
                select (std::max(STDIN_FILENO,exit_pipe[0])+1,&descriptor_set, NULL, NULL, NULL)
            );
            if(ret == -1)
                return false;
            if (FD_ISSET(exit_pipe[0], &descriptor_set))
                return false;
            if (FD_ISSET(STDIN_FILENO, &descriptor_set))
            {
                // read byte from stdin
                ret = TMP_FAILURE_RETRY(
                    read(STDIN_FILENO, &out, 1)
                );
                if(ret == -1)
                    return false;
                return true;
            }
            return false;
        }

    public:
        void print(const char *data)
        {
            fputs(data, dfout_C);
        }

        void print_text(color_ostream::color_value clr, const std::string &chunk)
        {
            if(!in_batch && state == con_lineedit)
            {
                disable_raw();
                fprintf(dfout_C,"\x1b[1G");
                fprintf(dfout_C,"\x1b[0K");

                color(clr);
                print(chunk.c_str());

                reset_color();
                enable_raw();
                prompt_refresh();
            }
            else
            {
                color(clr);
                print(chunk.c_str());
            }
        }

        void begin_batch()
        {
            assert(!in_batch);

            in_batch = true;

            if (state == con_lineedit)
            {
                disable_raw();
                fprintf(dfout_C,"\x1b[1G");
                fprintf(dfout_C,"\x1b[0K");
            }
        }

        void end_batch()
        {
            assert(in_batch);

            flush();

            in_batch = false;

            if (state == con_lineedit)
            {
                reset_color();
                enable_raw();
                prompt_refresh();
            }
        }

        void flush()
        {
            if (!rawmode)
                fflush(dfout_C);
        }

        /// Clear the console, along with its scrollback
        void clear()
        {
            if(rawmode)
            {
                const char * clr = "\033c\033[3J\033[H";
                if (::write(STDIN_FILENO,clr,strlen(clr)) == -1)
                    ;
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
            char tmp[64];
            sprintf(tmp,"\033[%d;%dH", y,x);
            print(tmp);
        }
        /// Set color (ANSI color number)
        void color(Console::color_value index)
        {
            if(!rawmode)
                fprintf(dfout_C, "%s", getANSIColor(index));
            else
            {
                const char * colstr = getANSIColor(index);
                int lstr = strlen(colstr);
                if (::write(STDIN_FILENO,colstr,lstr) == -1)
                    ;
            }
        }
        /// Reset color to default
        void reset_color(void)
        {
            color(COLOR_RESET);
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
        void back_word()
        {
            if (raw_cursor == 0)
                return;
            raw_cursor--;
            while (raw_cursor > 0 && !isalnum(raw_buffer[raw_cursor]))
                raw_cursor--;
            while (raw_cursor > 0 && isalnum(raw_buffer[raw_cursor]))
                raw_cursor--;
            if (!isalnum(raw_buffer[raw_cursor]) && raw_cursor != 0)
                raw_cursor++;
            prompt_refresh();
        }
        void forward_word()
        {
            int len = raw_buffer.size();
            if (raw_cursor == len)
                return;
            raw_cursor++;
            while (raw_cursor <= len && !isalnum(raw_buffer[raw_cursor]))
                raw_cursor++;
            while (raw_cursor <= len && isalnum(raw_buffer[raw_cursor]))
                raw_cursor++;
            if (raw_cursor > len)
                raw_cursor = len;
            prompt_refresh();
        }
        /// A simple line edit (raw mode)
        int lineedit(const std::string& prompt, std::string& output, std::recursive_mutex * lock, CommandHistory & ch)
        {
            if(state == con_lineedit)
                return Console::FAILURE;
            output.clear();
            reset_color();
            this->prompt = prompt;
            if (!supported_terminal)
            {
                print(prompt.c_str());
                fflush(dfout_C);
                // FIXME: what do we do here???
                //SDL_recursive_mutexV(lock);
                state = con_lineedit;
                std::getline(std::cin, output);
                state = con_unclaimed;
                //SDL_recursive_mutexP(lock);
                return output.size();
            }
            else
            {
                int count;
                if (enable_raw() == -1) return 0;
                state = con_lineedit;
                count = prompt_loop(lock,ch);
                state = con_unclaimed;
                disable_raw();
                print("\n");
                if(count > Console::FAILURE)
                {
                    output = toLocaleMB(raw_buffer);
                }
                return count;
            }
        }
        int enable_raw()
        {
            struct termios raw;

            if (!supported_terminal)
                return Console::FAILURE;
            if (tcgetattr(STDIN_FILENO,&orig_termios) == -1)
                return Console::FAILURE;

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
#ifdef CONSOLE_NO_CATCH
            raw.c_lflag &= ~( ECHO | ICANON | IEXTEN );
#else
            raw.c_lflag &= ~( ECHO | ICANON | IEXTEN | ISIG );
#endif
            // control chars - set return condition: min number of bytes and timer.
            // We want read to return every single byte, without timeout.
            raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0;// 1 byte, no timer
            // put terminal in raw mode
            if (tcsetattr(STDIN_FILENO, TCSADRAIN, &raw) < 0)
                return Console::FAILURE;
            rawmode = 1;
            return 0;
        }

        void disable_raw()
        {
            /* Don't even check the return value as it's too late. */
            if (rawmode && tcsetattr(STDIN_FILENO, TCSADRAIN, &orig_termios) != -1)
                rawmode = 0;
        }
        void prompt_refresh()
        {
            char seq[64];
            int cols = get_columns();
            int plen = prompt.size() % cols;
            int len = raw_buffer.size();
            int begin = 0;
            int cooked_cursor = raw_cursor;
            if ((plen+cooked_cursor) >= cols)
            {
                const int text_over = plen + cooked_cursor + 1 - cols;
                begin = text_over;
                len -= text_over;
                cooked_cursor -= text_over;
            }
            if (plen+len > cols)
                len -= plen+len - cols;
            std::string mbstr;
            try {
                mbstr = toLocaleMB(raw_buffer.substr(begin,len));
            }
            catch (std::out_of_range&) {
                // fallback check in case begin is still out of range
                // (this behaves badly but at least doesn't crash)
                mbstr = toLocaleMB(raw_buffer);
            }
            /* Cursor to left edge */
            snprintf(seq,64,"\x1b[1G");
            if (::write(STDIN_FILENO,seq,strlen(seq)) == -1) return;
            /* Write the prompt and the current buffer content */
            if (::write(STDIN_FILENO,prompt.c_str(),plen) == -1) return;
            if (::write(STDIN_FILENO,mbstr.c_str(),mbstr.length()) == -1) return;
            /* Erase to right */
            snprintf(seq,64,"\x1b[0K");
            if (::write(STDIN_FILENO,seq,strlen(seq)) == -1) return;
            /* Move cursor to original position. */
            snprintf(seq,64,"\x1b[1G\x1b[%dC", (int)(cooked_cursor+plen));
            if (::write(STDIN_FILENO,seq,strlen(seq)) == -1) return;
        }

        int prompt_loop(std::recursive_mutex * lock, CommandHistory & history)
        {
            int fd = STDIN_FILENO;
            size_t plen = prompt.size();
            int history_index = 0;
            raw_buffer.clear();
            raw_cursor = 0;
            /* The latest history entry is always our current buffer, that
             * initially is just an empty string. */
            const std::string empty;
            history.add(empty);
            if (::write(fd,prompt.c_str(),prompt.size()) == -1)
                return Console::FAILURE;
            while(1)
            {
                unsigned char c;
                unsigned char seq[2], seq2;
                lock->unlock();
                if(!read_char(c))
                {
                    lock->lock();
                    return Console::SHUTDOWN;
                }
                lock->lock();
                const int old_cursor = raw_cursor;
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
                case 10:
                    history.remove();
                    return raw_buffer.size();
                case 3:     // ctrl-c
                    return Console::RETRY;
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
                    if (!read_char(seq[0]))
                    {
                        lock->lock();
                        return Console::SHUTDOWN;
                    }
                    lock->lock();
                    if (seq[0] == 'b')
                    {
                        back_word();
                    }
                    else if (seq[0] == 'f')
                    {
                        forward_word();
                    }
                    else if (seq[0] == 127 || seq[0] == 8) // backspace || ctrl-h
                    {
                        // delete word
                        back_word();
                        if (old_cursor > raw_cursor)
                        {
                            yank_buffer = raw_buffer.substr(raw_cursor, old_cursor - raw_cursor);
                            raw_buffer.erase(raw_cursor, old_cursor - raw_cursor);
                            prompt_refresh();
                        }
                    }
                    else if (seq[0] == 'd')
                    {
                        // delete word forward
                        forward_word();
                        if (old_cursor < raw_cursor)
                        {
                            yank_buffer = raw_buffer.substr(old_cursor, raw_cursor - old_cursor);
                            raw_buffer.erase(old_cursor, raw_cursor - old_cursor);
                            raw_cursor = old_cursor;
                            prompt_refresh();
                        }
                    }
                    else if(seq[0] == '[')
                    {
                        if (!read_char(seq[1]))
                        {
                            lock->lock();
                            return Console::SHUTDOWN;
                        }
                        if (seq[1] == 'D')
                        {
                            /* left arrow */
                            if (raw_cursor > 0)
                            {
                                raw_cursor--;
                                prompt_refresh();
                            }
                        }
                        else if ( seq[1] == 'C')
                        {
                            /* right arrow */
                            if (size_t(raw_cursor) != raw_buffer.size())
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
                                history[history_index] = toLocaleMB(raw_buffer);
                                /* Show the new entry */
                                history_index += (seq[1] == 'A') ? 1 : -1;
                                if (history_index < 0)
                                {
                                    history_index = 0;
                                    break;
                                }
                                else if (size_t(history_index) >= history.size())
                                {
                                    history_index = history.size()-1;
                                    break;
                                }
                                raw_buffer = fromLocaleMB(history[history_index]);
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
                            unsigned char seq3[3];
                            lock->unlock();
                            if(!read_char(seq2))
                            {
                                lock->lock();
                                return Console::SHUTDOWN;
                            }
                            lock->lock();
                            if (seq[1] == '3' && seq2 == '~' )
                            {
                                // delete
                                if (raw_buffer.size() > 0 && size_t(raw_cursor) < raw_buffer.size())
                                {
                                    raw_buffer.erase(raw_cursor,1);
                                    prompt_refresh();
                                }
                                break;
                            }
                            if (!read_char(seq3[0]) || !read_char(seq3[1]))
                            {
                                lock->lock();
                                return Console::SHUTDOWN;
                            }
                            if (seq2 == ';')
                            {
                                // Format: esc [ n ; n DIRECTION
                                // Ignore first character (second "n")
                                if (seq3[1] == 'C')
                                {
                                    forward_word();
                                }
                                else if (seq3[1] == 'D')
                                {
                                    back_word();
                                }
                            }
                        }
                    }
                    break;
                case 21: // Ctrl+u, delete from current to beginning of line.
                    if (raw_cursor > 0)
                        yank_buffer = raw_buffer.substr(0, raw_cursor);
                    raw_buffer.erase(0, raw_cursor);
                    raw_cursor = 0;
                    prompt_refresh();
                    break;
                case 11: // Ctrl+k, delete from current to end of line.
                    if (size_t(raw_cursor) < raw_buffer.size())
                        yank_buffer = raw_buffer.substr(raw_cursor);
                    raw_buffer.erase(raw_cursor);
                    prompt_refresh();
                    break;
                case 25: // Ctrl+y, paste last text deleted with Ctrl+u/k
                    if (yank_buffer.size())
                    {
                        raw_buffer.insert(raw_cursor, yank_buffer);
                        raw_cursor += yank_buffer.size();
                        prompt_refresh();
                    }
                    break;
                case 20: // Ctrl+t, transpose current and previous characters
                    if (raw_buffer.size() >= 2 && raw_cursor > 0)
                    {
                        if (size_t(raw_cursor) == raw_buffer.size())
                            raw_cursor--;
                        std::swap(raw_buffer[raw_cursor - 1], raw_buffer[raw_cursor]);
                        raw_cursor++;
                        prompt_refresh();
                    }
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
                default:
                    if (c >= 32)  // Space
                    {
                        u32string::value_type c32;
                        char mb[MB_CUR_MAX];
                        size_t count = 1;
                        mb[0] = c;
                        ssize_t sz;
                        std::mbstate_t state{};
                        // Read all bytes belonging to a multi byte
                        // character starting from the first bye already red
                        while ((sz = mbrtoc32(&c32,&mb[count-1],1, &state)) < 0) {
                            if (sz == -1 || sz == -3)
                                return Console::FAILURE; /* mbrtoc32 error (not valid utf-32 character */
                            if(!read_char(c))
                                return Console::SHUTDOWN;
                            mb[count++] = c;
                        }
                        if (raw_buffer.size() == size_t(raw_cursor))
                        {
                            raw_buffer.append(1,c32);
                            raw_cursor++;
                            if (plen+raw_buffer.size() < size_t(get_columns()))
                            {
                                /* Avoid a full update of the line in the
                                 * trivial case. */
                                if (::write(fd,mb,count) == -1)
                                    return Console::FAILURE;
                            }
                            else
                            {
                                prompt_refresh();
                            }
                        }
                        else
                        {
                            raw_buffer.insert(raw_cursor,1,c32);
                            raw_cursor++;
                            prompt_refresh();
                        }
                        break;
                    }
                }
            }
            return raw_buffer.size();
        }
        FILE * dfout_C;
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
        bool in_batch;
        std::string prompt;      // current prompt string
        u32string raw_buffer;  // current raw mode buffer
        u32string yank_buffer; // last text deleted with Ctrl-K/Ctrl-U
        int raw_cursor;          // cursor position in the buffer
        // thread exit mechanism
        int exit_pipe[2];
        fd_set descriptor_set;
    };
}

PosixConsole::PosixConsole()
{
    d = nullptr;
    inited = false;
    // we can't create the mutex at this time. the SDL functions aren't hooked yet.
    wlock = new std::recursive_mutex();
}
PosixConsole::~PosixConsole()
{
    assert(!inited);
    if(wlock)
        delete wlock;
    if(d)
        delete d;
}

bool PosixConsole::init(bool dont_redirect)
{
    d = new Private();
    // make our own weird streams so our IO isn't redirected
    if (dont_redirect)
    {
        d->dfout_C = fopen("/dev/stdout", "w");
    }
    else
    {
        if (!freopen("stdout.log", "w", stdout))
            ;
        d->dfout_C = fopen("/dev/tty", "w");
        if (!d->dfout_C)
        {
            fprintf(stderr, "could not open tty\n");
            d->dfout_C = fopen("/dev/stdout", "w");
            return false;
        }
    }
    std::cin.tie(this);
    clear();
    d->supported_terminal = !isUnsupportedTerm() && isatty(STDIN_FILENO);
    // init the exit mechanism
    if (pipe(d->exit_pipe) == -1)
        ;
    FD_ZERO(&d->descriptor_set);
    FD_SET(STDIN_FILENO, &d->descriptor_set);
    FD_SET(d->exit_pipe[0], &d->descriptor_set);
    inited = true;
    return true;
}

bool PosixConsole::is_supported()
{
    return !isUnsupportedTerm() && isatty(STDIN_FILENO);
}

bool PosixConsole::shutdown(void)
{
    if(!d)
        return true;
    d->reset_color();
    std::lock_guard<std::recursive_mutex> lock{*wlock};
    close(d->exit_pipe[1]);
    if (d->state != Private::con_lineedit)
        inited = false;
    return true;
}

void PosixConsole::begin_batch()
{
    //color_ostream::begin_batch();

    wlock->lock();

    if (inited)
        d->begin_batch();
}

void PosixConsole::end_batch()
{
    if (inited)
        d->end_batch();

    wlock->unlock();
}

void PosixConsole::flush_proxy()
{
    std::lock_guard<std::recursive_mutex> lock{*wlock};
    if (inited)
        d->flush();
}

void PosixConsole::add_text(color_value color, const std::string &text)
{
    std::lock_guard<std::recursive_mutex> lock{*wlock};
    if (inited)
        d->print_text(color, text);
    else
        fwrite(text.data(), 1, text.size(), stderr);
}

int PosixConsole::get_columns(void)
{
    std::lock_guard<std::recursive_mutex> lock{*wlock};
    int ret = Console::FAILURE;
    if(inited)
        ret = d->get_columns();
    return ret;
}

int PosixConsole::get_rows(void)
{
    std::lock_guard<std::recursive_mutex> lock{*wlock};
    int ret = Console::FAILURE;
    if(inited)
        ret = d->get_rows();
    return ret;
}

void PosixConsole::clear()
{
    std::lock_guard<std::recursive_mutex> lock{*wlock};
    if(inited)
        d->clear();
}

void PosixConsole::gotoxy(int x, int y)
{
    std::lock_guard<std::recursive_mutex> lock{*wlock};
    if(inited)
        d->gotoxy(x,y);
}

void PosixConsole::cursor(bool enable)
{
    std::lock_guard<std::recursive_mutex> lock{*wlock};
    if(inited)
        d->cursor(enable);
}

int PosixConsole::lineedit(const std::string & prompt, std::string & output, CommandHistory & ch)
{
    std::lock_guard<std::recursive_mutex> lock{*wlock};
    int ret = Console::SHUTDOWN;
    if(inited) {
        ret = d->lineedit(prompt,output,wlock,ch);
        if (ret == Console::SHUTDOWN) {
            // kill the thing
            if(d->rawmode)
                d->disable_raw();
            d->print("\n");
            inited = false;
        }
    }
    return ret;
}

void PosixConsole::msleep (unsigned int msec)
{
    if (msec > 1000) sleep(msec/1000000);
    usleep((msec % 1000000) * 1000);
}

bool PosixConsole::hide()
{
    //Warmist: don't know if it's possible...
    return false;
}

bool PosixConsole::show()
{
    //Warmist: don't know if it's possible...
    return false;
}
