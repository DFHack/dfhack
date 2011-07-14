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
        Console();
        ~Console();
        /// initialize the console
        bool init( void );
        /// shutdown the console
        bool shutdown( void );
        /// Print a formatted string, like printf
        int  print(const char * format, ...);
        /// Clear the console, along with its scrollback
        void clear();
        /// Position cursor at x,y. 1,1 = top left corner
        void gotoxy(int x, int y);
        /// Set color (ANSI color number)
        void color(int index);
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
    private:
        int prompt_loop(const std::string & prompt, std::string & buffer);
        void prompt_refresh( const std::string & prompt, const std::string & buffer, size_t pos);
        int enable_raw();
        void disable_raw();
        void history_clear();
        Private * d;
    };
}