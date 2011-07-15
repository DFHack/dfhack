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

#pragma once
#include "dfhack/Pragma.h"
#include "dfhack/Export.h"
#include <ostream>
namespace  DFHack
{
    class Private;
    class DFHACK_EXPORT Console : public std::ostream
    {
    public:
        enum color_value
        {
            COLOR_RESET = -1,
            COLOR_BLACK = 0,
            COLOR_BLUE,
            COLOR_GREEN,
            COLOR_CYAN,
            COLOR_RED,
            COLOR_MAGENTA,
            COLOR_BROWN,
            COLOR_GREY,
            COLOR_DARKGREY,
            COLOR_LIGHTBLUE,
            COLOR_LIGHTGREEN,
            COLOR_LIGHTCYAN,
            COLOR_LIGHTRED,
            COLOR_LIGHTMAGENTA,
            COLOR_YELLOW,
            COLOR_WHITE,
            COLOR_MAX = COLOR_WHITE
        };
        ///ctor, NOT thread-safe
        Console();
        ///dtor, NOT thread-safe
        ~Console();
        /// initialize the console. NOT thread-safe
        bool init( void );
        /// shutdown the console. NOT thread-safe
        bool shutdown( void );

        /// Print a formatted string, like printf
        int  print(const char * format, ...);
        /// Print a formatted string, like printf, in red
        int  printerr(const char * format, ...);
        /// Clear the console, along with its scrollback
        void clear();
        /// Position cursor at x,y. 1,1 = top left corner
        void gotoxy(int x, int y);
        /// Set color (ANSI color number)
        void color(color_value c);
        /// Reset color to default
        void reset_color(void);
        /// Enable or disable the caret/cursor
        void cursor(bool enable = true);
        /// Waits given number of milliseconds before continuing.
        void msleep(unsigned int msec);
        /// get the current number of columns
        int  get_columns(void);
        /// get the current number of rows
        int  get_rows(void);
        /// beep. maybe?
        //void beep (void);
        /// A simple line edit (raw mode)
        int lineedit(const std::string& prompt, std::string& output);
        /// add a command to the history
        void history_add(const std::string& command);
        /// clear the command history
        void history_clear();
    private:
        Private * d;
    };
}