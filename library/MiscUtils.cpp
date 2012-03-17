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

#include "Internal.h"
#include "Export.h"
#include "MiscUtils.h"

#ifndef LINUX_BUILD
    #include <Windows.h>
#else
    #include <sys/time.h>
    #include <ctime>
#endif

#include <ctype.h>
#include <stdarg.h>

#include <sstream>

std::string stl_sprintf(const char *fmt, ...) {
    va_list lst;
    va_start(lst, fmt);
    std::string rv = stl_vsprintf(fmt, lst);
    va_end(lst);
    return rv;
}

std::string stl_vsprintf(const char *fmt, va_list args) {
    std::vector<char> buf;
    buf.resize(4096);
    for (;;) {
        int rsz = vsnprintf(&buf[0], buf.size(), fmt, args);

        if (rsz < 0)
            buf.resize(buf.size()*2);
        else if (unsigned(rsz) > buf.size())
            buf.resize(rsz+1);
        else
            return std::string(&buf[0], rsz);
    }
}

bool split_string(std::vector<std::string> *out,
                  const std::string &str, const std::string &separator, bool squash_empty)
{
    out->clear();

    size_t start = 0, pos;

    if (!separator.empty())
    {
        while ((pos = str.find(separator,start)) != std::string::npos)
        {
            if (pos > start || !squash_empty)
                out->push_back(str.substr(start, pos-start));
            start = pos + separator.size();
        }
    }

    if (start < str.size() || !squash_empty)
        out->push_back(str.substr(start));

    return out->size() > 1;
}

std::string join_strings(const std::string &separator, const std::vector<std::string> &items)
{
    std::stringstream ss;

    for (size_t i = 0; i < items.size(); i++)
    {
        if (i)
            ss << separator;
        ss << items[i];
    }

    return ss.str();
}

std::string toUpper(const std::string &str)
{
    std::string rv(str.size(),' ');
    for (unsigned i = 0; i < str.size(); ++i)
        rv[i] = toupper(str[i]);
    return rv;
}

std::string toLower(const std::string &str)
{
    std::string rv(str.size(),' ');
    for (unsigned i = 0; i < str.size(); ++i)
        rv[i] = tolower(str[i]);
    return rv;
}

#ifdef LINUX_BUILD // Linux
uint64_t GetTimeMs64()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t ret = tv.tv_usec;

    // Convert from micro seconds (10^-6) to milliseconds (10^-3)
    ret /= 1000;
    // Adds the seconds (10^0) after converting them to milliseconds (10^-3)
    ret += (tv.tv_sec * 1000);
    return ret;
}


#else // Windows
uint64_t GetTimeMs64()
{
    FILETIME ft;
    LARGE_INTEGER li;

    // Get the amount of 100 nano seconds intervals elapsed since January 1, 1601 (UTC)
    // and copy it to a LARGE_INTEGER structure.
    GetSystemTimeAsFileTime(&ft);
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;

    uint64_t ret = li.QuadPart;
    // Convert from file time to UNIX epoch time.
    ret -= 116444736000000000LL;
    // From 100 nano seconds (10^-7) to 1 millisecond (10^-3) intervals
    ret /= 10000;

    return ret;
}
#endif