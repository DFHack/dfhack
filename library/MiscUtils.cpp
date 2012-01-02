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
#include "Core.h"
#include "MiscUtils.h"



#ifndef LINUX_BUILD
    #include <Windows.h>
#else
    #include <sys/time.h>
    #include <ctime>
#endif

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