/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

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

#include "Export.h"
#include "ColorText.h"
#include <deque>
#include <fstream>
#include <assert.h>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

namespace  DFHack
{
    class CommandHistory
    {
    public:
        CommandHistory(std::size_t capacity = 5000)
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
            if (!history.size())
                return true;
            std::ofstream outfile (filename);
            //fprintf(stderr,"Save: Initialized stream\n");
            if(outfile.bad())
                return false;
            //fprintf(stderr,"Save: Iterating...\n");
            for(auto iter = history.begin();iter < history.end(); iter++)
            {
                //fprintf(stderr,"Save: Dumping %s\n",(*iter).c_str());
                outfile << *iter << std::endl;
                //fprintf(stderr,"Save: Flushing\n");
                outfile.flush();
            }
            //fprintf(stderr,"Save: Closing\n");
            outfile.close();
            //fprintf(stderr,"Save: Done\n");
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
        /// adds the current list of entries to the given vector
        void getEntries(std::vector<std::string> &entries)
        {
            for (auto &entry : history)
                entries.push_back(entry);
        }
    private:
        std::size_t capacity;
        std::deque <std::string> history;
    };

    class DFHACK_EXPORT Console : public color_ostream
    {
    public:
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


        enum class Type {
            Posix,
            SDL,
            Windows,
            DUMMY
        };

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

    protected:
        Type con_type{Type::DUMMY};

        virtual void begin_batch() {};
        virtual void add_text(color_value color, const std::string &text) {
            std::cout << getANSIColor(color);
            std::cout << text;
            std::cout << RESETCOLOR;
        };
        virtual void end_batch() {};
        virtual void flush_proxy() {};

    public:
        ///ctor, NOT thread-safe
        Console() = default;
        Console(Type type) : con_type(type) {}
        ///dtor, NOT thread-safe
        virtual ~Console() = default;
        /// initialize the console. NOT thread-safe
        virtual bool init( bool dont_redirect ) { return false; };
        /// shutdown the console. NOT thread-safe
        virtual bool shutdown( void ) { return true; };

        /// Clear the console, along with its scrollback
        virtual void clear() {};
        /// Position cursor at x,y. 1,1 = top left corner
        virtual void gotoxy(int x, int y) {};
        /// Enable or disable the caret/cursor
        virtual void cursor(bool enable = true) {};
        /// Waits given number of milliseconds before continuing.
        virtual void msleep(unsigned int msec) {};
        /// get the current number of columns
        virtual int  get_columns(void) { return FAILURE; };
        /// get the current number of rows
        virtual int  get_rows(void) { return FAILURE; };
        /// beep. maybe?
        //void beep (void);
        //! \defgroup lineedit_return_values Possible errors from lineedit
        //! \{
        static constexpr int FAILURE = -1;
        static constexpr int SHUTDOWN = -2;
        static constexpr int RETRY = -3;
        //! \}
        /// A simple line edit (raw mode)
        virtual int lineedit(const std::string& prompt, std::string& output, CommandHistory & history ) { return SHUTDOWN; };
        virtual bool isInited (void) { return false; };

        bool is_console() { return true; }

        virtual bool hide() { return false; };
        virtual bool show() { return false; };

        virtual void cleanup() {};
        Type get_type() const { return con_type; }

        template <typename T>
        T& as() {
            return static_cast<T&>(*this);
        }

        static std::unique_ptr<Console> makeConsole();
    };
}
