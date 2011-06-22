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

#include "dfhack/Console.h"
#include <cstdio>
#include <cstdlib>
#include <sstream>
using namespace DFHack;

duthomhas::stdiostream dfout;
FILE * dfout_C = 0;
duthomhas::stdiobuf * stream_o = 0;

// FIXME: prime candidate for being a singleton...
Console::Console()
{
    // make our own weird streams so our IO isn't redirected
    dfout_C = fopen("/dev/tty", "w");
    stream_o = new duthomhas::stdiobuf(dfout_C);
    dfout.rdbuf(stream_o);
    std::cin.tie(&dfout);
    clear();
    // result is a terminal controlled by the parasitic code!
}
Console::~Console()
{
    
}
void Console::clear()
{
    dfout << "\033c";
    dfout << "\033[3J\033[H";
}
void Console::gotoxy(int x, int y)
{
    std::ostringstream oss;
    oss << "\033[" << y << ";" << x << "H";
    dfout << oss.str();
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
    dfout << getANSIColor(index);
}

void Console::cursor(bool enable)
{
    if(enable)
    {
        dfout <<"\033[?25h";
    }
    else
    {
        dfout <<"\033[?25l";
    }
}