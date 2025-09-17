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
#include <filesystem>

namespace  DFHack
{
    class CommandHistory
    {
    public:
        CommandHistory(std::size_t capacity = 5000)
        {
            this->capacity = capacity;
        }
        bool load (std::filesystem::path filename)
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
        bool save (std::filesystem::path filename)
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

    enum class ConsoleType {
        Base,
        Posix,
        Windows,
        SDL
    };

    class DFHACK_EXPORT Console : public color_ostream
    {
    private:
        bool use_ansi_colors_{false};

    protected:
        ConsoleType con_type{type_tag};

        virtual void begin_batch() {};
        virtual void add_text(color_value color, const std::string &text);
        virtual void end_batch() {};
        virtual void flush_proxy() { std::cout << std::flush; }

    public:
        static const char * getANSIColor(const int c);

        //! \defgroup lineedit_return_values Possible errors from lineedit
        //! \{
        static constexpr int FAILURE = -1;
        static constexpr int SHUTDOWN = -2;
        static constexpr int RETRY = -3;
        //! \}
        static constexpr ConsoleType type_tag = ConsoleType::Base;

        ///ctor, NOT thread-safe
        Console() = default;
        template <typename Derived>
        Console(Derived*) : con_type(Derived::type_tag) {}
        ///dtor, NOT thread-safe
        virtual ~Console() = default;

        /// initialize the console. NOT thread-safe
        virtual bool init( bool dont_redirect ) { return true; };
        /// shutdown the console. NOT thread-safe
        virtual bool shutdown( void ) { return true; };

        /// Clear the console, along with its scrollback
        virtual void clear() {};
        /// Position cursor at x,y. 1,1 = top left corner
        virtual void gotoxy(int x, int y) {};
        /// Enable or disable the caret/cursor
        virtual void cursor(bool enable = true) {};
        /// get the current number of columns
        virtual int  get_columns(void) { return -1; };
        /// get the current number of rows
        virtual int  get_rows(void) { return -1; };
        /// beep. maybe?
        //void beep (void);
        /// A simple line edit (raw mode)
        virtual int lineedit(const std::string& prompt, std::string& output, CommandHistory & history ) { return SHUTDOWN; };
        virtual bool isInited (void) { return true; };

        virtual bool hide() { return false; };
        virtual bool show() { return false; };

        virtual void cleanup() {};

        /// Platform independent. Waits given number of milliseconds before continuing.
        void msleep(unsigned int msec);
        bool is_console() { return true; }
        ConsoleType get_type() const { return con_type; }
        void use_ansi_colors(bool choice) { use_ansi_colors_ = choice; };

        template <typename T>
        T& as() {
            return static_cast<T&>(*this);
        }

        template <typename T>
        T* try_as() {
            return (get_type() == T::type_tag) ? static_cast<T*>(this) : nullptr;
        }

        static std::unique_ptr<Console> makeConsole();
        template <typename T>
        static std::unique_ptr<Console> makeConsole();
    };
}
