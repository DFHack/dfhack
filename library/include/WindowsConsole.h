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
#include "Console.h"
#include <atomic>
#include <assert.h>
#include <mutex>
#include <string>
#include <memory>

namespace  DFHack
{
    class Private;
    class DFHACK_EXPORT WindowsConsole : public Console
    {
    protected:
        void begin_batch() override;
        void add_text(color_value color, const std::string &text) override;
        void end_batch() override;
        void flush_proxy() override;
    public:
        WindowsConsole();
        ~WindowsConsole();

        bool init( bool dont_redirect ) override;
        /// shutdown the console. NOT thread-safe
        bool shutdown( void ) override;
        void clear() override;
        /// Position cursor at x,y. 1,1 = top left corner
        void gotoxy(int x, int y) override;
        /// Enable or disable the caret/cursor
        void cursor(bool enable = true) override;
        /// get the current number of columns
        int  get_columns(void) override;
        /// get the current number of rows
        int  get_rows(void) override;

        int lineedit(const std::string& prompt, std::string& output, CommandHistory & history ) override;

        bool hide() override;
        bool show() override;

        static bool is_supported() { return true; };

        static constexpr ConsoleType type_tag = ConsoleType::Windows;
    private:
        Private * d;
        std::recursive_mutex * wlock;
        std::atomic<bool> inited;
    };
}
