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
#include <windows.h>
#include <conio.h>
#include <stdarg.h>

#include "dfhack/extra/stdiostream.h"
#include < process.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>
#include <istream>
#include <string>

#include "dfhack/Console.h"
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <deque>
using namespace DFHack;

// FIXME: maybe make configurable with an ini option?
#define MAX_CONSOLE_LINES 999;

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
            console_in = 0;
            console_out = 0;
            ConsoleWindow = 0;
            default_attributes = 0;
        };
        void output(const char* str, size_t len, int x, int y)
        {
            COORD pos = { (SHORT)x, (SHORT)y };
            DWORD count = 0;
            WriteConsoleOutputCharacterA(console_out, str, len, pos, &count);
        }

        FILE * dfout_C;
        duthomhas::stdiobuf * stream_o;
        int rawmode; /* for atexit() function to check if restore is needed*/
        std::deque <std::string> history;

        HANDLE console_in;
        HANDLE console_out;
        HWND ConsoleWindow;
        WORD default_attributes;
    };
}


Console::Console():std::ostream(0), std::ios(0)
{
    d = new Private();
}

Console::~Console()
{
}

bool Console::init(void)
{
    int                        hConHandle;
    long                       lStdHandle;
    CONSOLE_SCREEN_BUFFER_INFO coninfo;
    FILE                       *fp;
    DWORD  oldMode, newMode;

    // Allocate a console!
    AllocConsole();
    d->ConsoleWindow = GetConsoleWindow();
    HMENU  hm = GetSystemMenu(d->ConsoleWindow,false);
    DeleteMenu(hm, SC_CLOSE, MF_BYCOMMAND);

    // set the screen buffer to be big enough to let us scroll text
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
    d->default_attributes = coninfo.wAttributes;
    coninfo.dwSize.Y = MAX_CONSOLE_LINES;  // How many lines do you want to have in the console buffer
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

    // redirect unbuffered STDOUT to the console
    d->console_out = GetStdHandle(STD_OUTPUT_HANDLE);
    lStdHandle = (long)d->console_out;
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    d->dfout_C = _fdopen( hConHandle, "w" );
    setvbuf( d->dfout_C, NULL, _IONBF, 0 );

    // redirect unbuffered STDIN to the console
    d->console_in = GetStdHandle(STD_INPUT_HANDLE);
    lStdHandle = (long)d->console_in;
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen( hConHandle, "r" );
    *stdin = *fp;
    setvbuf( stdin, NULL, _IONBF, 0 );
    GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),&oldMode);
    newMode = oldMode | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT;
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),newMode);
    std::ios::sync_with_stdio();

    // make our own weird streams so our IO isn't redirected
    d->stream_o = new duthomhas::stdiobuf(d->dfout_C);
    rdbuf(d->stream_o);
    std::cin.tie(this);
    clear();
    return true;
}

bool Console::shutdown(void)
{
    FreeConsole();
    return true;
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
    CONSOLE_SCREEN_BUFFER_INFO inf = { 0 };
    GetConsoleScreenBufferInfo(d->console_out, &inf);
    return (size_t)inf.dwSize.X;
}

int Console::get_rows(void)
{
    CONSOLE_SCREEN_BUFFER_INFO inf = { 0 };
    GetConsoleScreenBufferInfo(d->console_out, &inf);
    return (size_t)inf.dwSize.Y;
}

void Console::clear()
{
    system("cls");
}

void Console::gotoxy(int x, int y)
{
    COORD coord = {x-1, y-1}; // Windows uses 0-based coordinates
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void Console::color(int index)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, index);
}

void Console::reset_color( void )
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, d->default_attributes);
}


void Console::cursor(bool enable)
{
    if(enable)
    {
        HANDLE hConsoleOutput;
        CONSOLE_CURSOR_INFO structCursorInfo;
        hConsoleOutput = GetStdHandle( STD_OUTPUT_HANDLE );
        GetConsoleCursorInfo( hConsoleOutput, &structCursorInfo ); // Get current cursor size
        structCursorInfo.bVisible = TRUE;
        SetConsoleCursorInfo( hConsoleOutput, &structCursorInfo );
    }
    else
    {
        HANDLE hConsoleOutput;
        CONSOLE_CURSOR_INFO structCursorInfo;
        hConsoleOutput = GetStdHandle( STD_OUTPUT_HANDLE );
        GetConsoleCursorInfo( hConsoleOutput, &structCursorInfo ); // Get current cursor size
        structCursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo( hConsoleOutput, &structCursorInfo );
    }
}

void Console::msleep (unsigned int msec)
{
    Sleep(msec);
}

int Console::enable_raw()
{
    return 0;
}

void Console::disable_raw()
{
    /* Nothing to do yet */
}

void Console::prompt_refresh( const std::string& prompt, const std::string& buffer, size_t pos)
{
    size_t cols = get_columns();
    size_t plen = prompt.size();
    const char * buf = buffer.c_str();
    size_t len = buffer.size();

    while ((plen + pos) >= cols)
    {
        buf++;
        len--;
        pos--;
    }
    while (plen + len > cols)
    {
        len--;
    }

    CONSOLE_SCREEN_BUFFER_INFO inf = { 0 };
    GetConsoleScreenBufferInfo(d->console_out, &inf);
    d->output(prompt.c_str(), plen, 0, inf.dwCursorPosition.Y);
    d->output(buf, len, plen, inf.dwCursorPosition.Y);
    if (plen + len < (size_t)inf.dwSize.X)
    {
        /* Blank to EOL */
        char* tmp = (char*)malloc(inf.dwSize.X - (plen + len));
        memset(tmp, ' ', inf.dwSize.X - (plen + len));
        d->output(tmp, inf.dwSize.X - (plen + len), len + plen, inf.dwCursorPosition.Y);
        free(tmp);
    }
    inf.dwCursorPosition.X = (SHORT)(pos + plen);
    SetConsoleCursorPosition(d->console_out, inf.dwCursorPosition);
}

int Console::prompt_loop(const std::string & prompt, std::string & buffer)
{
    buffer.clear(); // make sure the buffer is empty!
    size_t plen = prompt.size();
    size_t pos = 0;
    int history_index = 0;

    /* The latest history entry is always our current buffer, that
     * initially is just an empty string. */
    history_add("");

    CONSOLE_SCREEN_BUFFER_INFO inf = { 0 };
    GetConsoleScreenBufferInfo(d->console_out, &inf);
    size_t cols = inf.dwSize.X;
    d->output(prompt.c_str(), plen, 0, inf.dwCursorPosition.Y);
    inf.dwCursorPosition.X = (SHORT)plen;
    SetConsoleCursorPosition(d->console_out, inf.dwCursorPosition);

    while (1)
    {
        INPUT_RECORD rec;
        DWORD count;
        ReadConsoleInputA(d->console_in, &rec, 1, &count);
        if (rec.EventType != KEY_EVENT || !rec.Event.KeyEvent.bKeyDown)
            continue;
        switch (rec.Event.KeyEvent.wVirtualKeyCode)
        {
        case VK_RETURN: // enter
            d->history.pop_front();
            return buffer.size();
        case VK_BACK:   // backspace
            if (pos > 0 && buffer.size() > 0)
            {
                buffer.erase(pos-1,1);
                pos--;
                prompt_refresh(prompt,buffer,pos);
            }
            break;
        case VK_LEFT: // left arrow
            if (pos > 0)
            {
                pos--;
                prompt_refresh(prompt,buffer,pos);
            }
            break;
        case VK_RIGHT: // right arrow
            if (pos != buffer.size())
            {
                pos++;
                prompt_refresh(prompt,buffer,pos);
            }
            break;
        case VK_UP:
        case VK_DOWN:
            /* up and down arrow: history */
            if (d->history.size() > 1)
            {
                /* Update the current history entry before to
                 * overwrite it with tne next one. */
                d->history[history_index] = buffer;
                /* Show the new entry */
                history_index += (rec.Event.KeyEvent.wVirtualKeyCode == VK_UP) ? 1 : -1;
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
            break;
        case VK_DELETE:
            // delete
            if (buffer.size() > 0 && pos < buffer.size())
            {
                buffer.erase(pos,1);
                prompt_refresh(prompt,buffer,pos);
            }
            break;
        case VK_HOME:
            pos = 0;
            prompt_refresh(prompt,buffer,pos);
            break;
        case VK_END:
            pos = buffer.size();
            prompt_refresh(prompt,buffer,pos);
            break;
        default:
            if (rec.Event.KeyEvent.uChar.AsciiChar < ' ' ||
                rec.Event.KeyEvent.uChar.AsciiChar > '~')
                continue;
            if (buffer.size() == pos)
                buffer.append(1,rec.Event.KeyEvent.uChar.AsciiChar);
            else
                buffer.insert(pos,1,rec.Event.KeyEvent.uChar.AsciiChar);
            pos++;
            prompt_refresh(prompt,buffer,pos);
            break;
        }
    }
}

// push to front, remove from back if we are above maximum. ignore immediate duplicates
void Console::history_add(const std::string & command)
{
    if(d->history.front() == command)
        return;
    d->history.push_front(command);
    if(d->history.size() > 100)
        d->history.pop_back();
}

int Console::lineedit(const std::string & prompt, std::string & output)
{
    output.clear();
    int count;
    if (enable_raw() == -1)
        return -1;
    count = prompt_loop(prompt,output);
    disable_raw();
    *this << std::endl;
    return count;
}