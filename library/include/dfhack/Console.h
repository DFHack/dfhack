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
#include <deque>
#include <fstream>
#include <llimits.h>
#include <assert.h>
#include <iostream>
#include <string>
namespace tthread
{
    class mutex;
    class condition_variable;
    class thread;
}
namespace  DFHack
{
    class CommandHistory
    {
    public:
        CommandHistory(std::size_t capacity = 100)
        {
            this->capacity = capacity;
        }
        bool load (const char * filename)
        {
            std::string reader;
            std::ifstream infile(filename);
            if(infile.bad())
                return false;
            std::string s;
            while(std::getline(infile, s))
            {
                if(s.empty())
                    continue;
                history.push_back(s);
            }
            return true;
        }
        bool save (const char * filename)
        {
            std::ofstream outfile (filename);
            if(outfile.bad())
                return false;
            for(auto iter = history.begin();iter < history.end(); iter++)
            {
                outfile << *iter << std::endl;
            }
            outfile.close();
            return true;
        }
        /// add a command to the history
        void add(const std::string& command)
        {
            // if current command = last in history -> do not add. Always add if history is empty.
            if(!history.empty() && history.front() == command)
                return;
            history.push_front(command);
            if(history.size() > capacity)
                history.pop_back();
        }
        /// clear the command history
        void clear()
        {
            history.clear();
        }
        /// get current history size
        std::size_t size()
        {
            return history.size();
        }
        /// get pointer to a particular history item
        std::string & operator[](std::size_t index)
        {
            assert(index < history.size());
            return history[index];
        }
        void remove( void )
        {
            history.pop_front();
        }
    private:
        std::size_t capacity;
        std::deque <std::string> history;
    };
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
        bool init( bool sharing );
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
        int lineedit(const std::string& prompt, std::string& output, CommandHistory & history );
    private:
        Private * d;
        tthread::mutex * wlock;
        bool inited;
    };
}
