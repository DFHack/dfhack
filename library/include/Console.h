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
#include <atomic>
#include <deque>
#include <fstream>
#include <assert.h>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

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

    class Private;
    class DFHACK_EXPORT Console : public color_ostream
    {
    protected:
        virtual void begin_batch();
        virtual void add_text(color_value color, const std::string &text);
        virtual void end_batch();

        virtual void flush_proxy();

    public:
        ///ctor, NOT thread-safe
        Console();
        ///dtor, NOT thread-safe
        ~Console();
        /// initialize the console. NOT thread-safe
        bool init( bool dont_redirect );
        /// shutdown the console. NOT thread-safe
        bool shutdown( void );

        /// Clear the console, along with its scrollback
        void clear();
        /// Position cursor at x,y. 1,1 = top left corner
        void gotoxy(int x, int y);
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
        //! \defgroup lineedit_return_values Possible errors from lineedit
        //! \{
        static constexpr int FAILURE = -1;
        static constexpr int SHUTDOWN = -2;
        static constexpr int RETRY = -3;
        //! \}
        /// A simple line edit (raw mode)
        int lineedit(const std::string& prompt, std::string& output, CommandHistory & history );
        bool isInited (void) { return inited; };

        bool is_console() { return true; }

        bool hide();
        bool show();
    private:
        Private * d;
        std::recursive_mutex * wlock;
        std::atomic<bool> inited;
    };
}
