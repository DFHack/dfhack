/*
https://github.com/peterix/dfhack
Copyright (c) 2011 Petr Mrázek <peterix@gmail.com>

A thread-safe logging console with a line editor for windows.

Based on linenoise win32 port,
copyright 2010, Jon Griffiths <jon_p_griffiths at yahoo dot com>.
All rights reserved.
Based on linenoise, copyright 2010, Salvatore Sanfilippo <antirez at gmail dot com>.
The original linenoise can be found at: http://github.com/antirez/linenoise

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
  * Neither the name of Redis nor the names of its contributors may be used
    to endorse or promote products derived from this software without
    specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/


#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <istream>
#include <string>

#include "ColorText.h"
#include "MiscUtils.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <sstream>

using namespace DFHack;

#include "tinythread.h"
using namespace tthread;

bool color_ostream::log_errors_to_stderr = false;

void color_ostream::flush_buffer(bool flush)
{
    auto buffer = buf();
    auto str = buffer->str();

    if (!str.empty()) {
        add_text(cur_color, buffer->str());
        buffer->str(std::string());
    }

    if (flush)
        flush_proxy();
}

void color_ostream::begin_batch()
{
    flush_buffer(false);
}

void color_ostream::end_batch()
{
    flush_proxy();
}

color_ostream::color_ostream() : ostream(new buffer(this)), cur_color(COLOR_RESET)
{
    //
}

color_ostream::~color_ostream()
{
    delete buf();
}

void color_ostream::print(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprint(format, args);
    va_end(args);
}

void color_ostream::vprint(const char *format, va_list args)
{
    std::string str = stl_vsprintf(format, args);

    if (!str.empty()) {
        flush_buffer(false);
        add_text(cur_color, str);
        if (str[str.size()-1] == '\n')
            flush_proxy();
    }
}

void color_ostream::printerr(const char * format, ...)
{
    va_list args;
    va_start(args, format);
    vprinterr(format, args);
    va_end(args);
}

void color_ostream::vprinterr(const char *format, va_list args)
{
    color_value save = cur_color;

    if (log_errors_to_stderr)
    {
        va_list args1;
        va_copy(args1, args);
        vfprintf(stderr, format, args1);
        va_end(args1);
    }

    color(COLOR_LIGHTRED);
    va_list args2;
    va_copy(args2, args);
    vprint(format, args2);
    va_end(args2);
    color(save);
}

void color_ostream::color(color_value c)
{
    if (c == cur_color)
        return;

    flush_buffer(false);
    cur_color = c;
}

void color_ostream::reset_color(void)
{
    color(COLOR_RESET);
}

void color_ostream_wrapper::add_text(color_value, const std::string &text)
{
    out << text;
}

void color_ostream_wrapper::flush_proxy()
{
    out << std::flush;
}

void buffered_color_ostream::add_text(color_value color, const std::string &text)
{
    if (text.empty())
        return;

    if (buffer.empty())
    {
        buffer.push_back(fragment_type(color, text));
    }
    else
    {
        auto &back = buffer.back();

        if (back.first != color || std::max(back.second.size(), text.size()) > 128)
            buffer.push_back(fragment_type(color, text));
        else
            buffer.back().second += text;
    }
}

void color_ostream_proxy::flush_proxy()
{
    if (buffer.empty())
        return;

    if (target)
    {
        target->begin_batch();

        for (auto it = buffer.begin(); it != buffer.end(); ++it)
            target->add_text(it->first, it->second);

        target->end_batch();
    }

    buffer.clear();
}

color_ostream_proxy::~color_ostream_proxy()
{
    *this << std::flush;
}
