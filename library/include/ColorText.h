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

#include <list>
#include <fstream>
#include <assert.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <sstream>

namespace dfproto
{
    class CoreTextNotification;
}

namespace  DFHack
{
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

    class DFHACK_EXPORT color_ostream : public std::ostream
    {
    public:
        using color_value = DFHack::color_value;

    private:
        color_value cur_color;

        class buffer : public std::stringbuf
        {
        public:
            color_ostream *owner;

            buffer(color_ostream *owner) : owner(owner) {}
            virtual ~buffer() { }

        protected:
            virtual int sync() {
                owner->flush_buffer(true);
                return 0;
            }
        };

        buffer *buf() { return (buffer*)rdbuf(); }

        void flush_buffer(bool flush);

    protected:
        // These must be strictly balanced, because
        // they might grab and hold mutexes.
        virtual void begin_batch();
        virtual void end_batch();

        virtual void add_text(color_value color, const std::string &text) = 0;

        virtual void flush_proxy() {};

        friend class color_ostream_proxy;
    public:
        color_ostream();
        virtual ~color_ostream();

        /// Print a formatted string, like printf
        void print(const char *format, ...) Wformat(printf,2,3);
        void vprint(const char *format, va_list args) Wformat(printf,2,0);

        /// Print a formatted string, like printf, in red
        void printerr(const char *format, ...) Wformat(printf,2,3);
        void vprinterr(const char *format, va_list args) Wformat(printf,2,0);

        /// Get color
        color_value color() { return cur_color; }
        /// Set color (ANSI color number)
        void color(color_value c);
        /// Reset color to default
        void reset_color(void);

        virtual bool is_console() { return false; }
        virtual color_ostream *proxy_target() { return NULL; }

        static bool log_errors_to_stderr;
    };

    inline color_ostream &operator << (color_ostream &out, color_ostream::color_value clr)
    {
        out.color(clr);
        return out;
    }

    class DFHACK_EXPORT color_ostream_wrapper : public color_ostream
    {
        std::ostream &out;

    protected:
        virtual void add_text(color_value color, const std::string &text);
        virtual void flush_proxy();

    public:
        color_ostream_wrapper(std::ostream &os) : out(os) {}
    };

    class DFHACK_EXPORT buffered_color_ostream : public color_ostream
    {
    protected:
        virtual void add_text(color_value color, const std::string &text);

    public:
        typedef std::pair<color_value,std::string> fragment_type;

        buffered_color_ostream() {}
        ~buffered_color_ostream() {}

        const std::list<fragment_type> &fragments() { return buffer; }

    protected:
        std::list<fragment_type> buffer;
    };

    class DFHACK_EXPORT color_ostream_proxy : public buffered_color_ostream
    {
    protected:
        color_ostream *target;

        virtual void flush_proxy();

    public:
        color_ostream_proxy(color_ostream &target) : target(&target) {}
        ~color_ostream_proxy();

        virtual color_ostream *proxy_target() { return target; }

        void decode(dfproto::CoreTextNotification *data);
    };
}
