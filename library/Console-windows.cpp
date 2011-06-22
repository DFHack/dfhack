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
using namespace DFHack;

// FIXME: maybe make configurable with an ini option?
#define MAX_CONSOLE_LINES 250;

duthomhas::stdiostream dfout;
FILE * dfout_C = 0;
duthomhas::stdiobuf * stream_o = 0;

HANDLE  g_hConsoleOut;                   // Handle to debug console
HWND ConsoleWindow;

// FIXME: prime candidate for being a singleton... indeed.
Console::Console()
{
    int                        hConHandle;
    long                       lStdHandle;
    CONSOLE_SCREEN_BUFFER_INFO coninfo;
    FILE                       *fp;
    DWORD  oldMode, newMode;

    // Allocate a console!
    AllocConsole();
    ConsoleWindow = GetConsoleWindow();
    HMENU  hm = GetSystemMenu(ConsoleWindow,false);
    DeleteMenu(hm, SC_CLOSE, MF_BYCOMMAND);

    // set the screen buffer to be big enough to let us scroll text
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
    coninfo.dwSize.Y = MAX_CONSOLE_LINES;  // How many lines do you want to have in the console buffer
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

    // redirect unbuffered STDOUT to the console
    g_hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
    lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    dfout_C = _fdopen( hConHandle, "w" );
    setvbuf( dfout_C, NULL, _IONBF, 0 );

    // redirect unbuffered STDIN to the console
    lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen( hConHandle, "r" );
    *stdin = *fp;
    setvbuf( stdin, NULL, _IONBF, 0 );
    GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),&oldMode);
    newMode = oldMode | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT;
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),newMode);
    std::ios::sync_with_stdio();

    // make our own weird streams so our IO isn't redirected
    stream_o = new duthomhas::stdiobuf(dfout_C);
    dfout.rdbuf(stream_o);
    std::cin.tie(&dfout);
    clear();
    // result is a terminal controlled by the parasitic code!
}
Console::~Console()
{
    FreeConsole();
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