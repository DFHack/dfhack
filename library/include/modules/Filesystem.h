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
/* Based on luafilesystem
Copyright © 2003-2014 Kepler Project.

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once
#include "Export.h"
#include <map>
#include <string>
#include <vector>

#ifndef _WIN32
    #ifndef _AIX
        #define _FILE_OFFSET_BITS 64 /* Linux, Solaris and HP-UX */
    #else
        #define _LARGE_FILES 1 /* AIX */
    #endif
#endif

#ifndef _LARGEFILE64_SOURCE
    #define _LARGEFILE64_SOURCE
#endif

#include <cstdio>
#include <cstdint>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>

#ifdef _WIN32
    #include <direct.h>
    #define NOMINMAX
    #include <windows.h>
    #include <io.h>
    #include <sys/locking.h>
    #ifdef __BORLANDC__
        #include <utime.h>
    #else
        #include <sys/utime.h>
    #endif
    #include <fcntl.h>
    #include "wdirent.h"
#else
    #include <unistd.h>
    #include <dirent.h>
    #include <fcntl.h>
    #include <sys/types.h>
    #include <utime.h>
#endif

#ifdef _WIN32
    #ifdef __BORLANDC__
        #define lfs_setmode(L,file,m)   ((void)L, setmode(_fileno(file), m))
        #define STAT_STRUCT struct stati64
    #else
        #define lfs_setmode(L,file,m)   ((void)L, _setmode(_fileno(file), m))
        #define STAT_STRUCT struct _stati64
    #endif
    #define STAT_FUNC _stati64
    #define LSTAT_FUNC STAT_FUNC
#else
    #define _O_TEXT 0
    #define _O_BINARY 0
    #define lfs_setmode(L,file,m)   ((void)L, (void)file, (void)m, 0)
    #define STAT_STRUCT struct stat
    #define STAT_FUNC stat
    #define LSTAT_FUNC lstat
#endif

#ifdef _WIN32
    #ifndef S_ISDIR
        #define S_ISDIR(mode)  (mode&_S_IFDIR)
    #endif
    #ifndef S_ISREG
        #define S_ISREG(mode)  (mode&_S_IFREG)
    #endif
    #ifndef S_ISLNK
        #define S_ISLNK(mode)  (0)
    #endif
    #ifndef S_ISSOCK
        #define S_ISSOCK(mode)  (0)
    #endif
    #ifndef S_ISCHR
        #define S_ISCHR(mode)  (mode&_S_IFCHR)
    #endif
    #ifndef S_ISBLK
        #define S_ISBLK(mode)  (0)
    #endif
#endif

enum _filetype {
    FILETYPE_NONE = -2,
    FILETYPE_UNKNOWN = -1,
    FILETYPE_FILE = 1,
    FILETYPE_DIRECTORY,
    FILETYPE_LINK,
    FILETYPE_SOCKET,
    FILETYPE_NAMEDPIPE,
    FILETYPE_CHAR_DEVICE,
    FILETYPE_BLOCK_DEVICE
};

namespace DFHack {
    namespace Filesystem {
        DFHACK_EXPORT bool chdir (std::string path);
        DFHACK_EXPORT std::string getcwd ();
        DFHACK_EXPORT bool mkdir (std::string path);
        DFHACK_EXPORT bool rmdir (std::string path);
        DFHACK_EXPORT bool stat (std::string path, STAT_STRUCT &info);
        DFHACK_EXPORT bool exists (std::string path);
        DFHACK_EXPORT _filetype filetype (std::string path);
        DFHACK_EXPORT bool isfile (std::string path);
        DFHACK_EXPORT bool isdir (std::string path);
        DFHACK_EXPORT int64_t atime (std::string path);
        DFHACK_EXPORT int64_t ctime (std::string path);
        DFHACK_EXPORT int64_t mtime (std::string path);
        DFHACK_EXPORT int listdir (std::string dir, std::vector<std::string> &files);
        DFHACK_EXPORT int listdir_recursive (std::string dir, std::map<std::string, bool> &files,
            int depth = 10, std::string prefix = "");
    }
}
