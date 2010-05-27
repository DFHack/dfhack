/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf

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

#ifndef DFCOMMONINTERNAL_H_INCLUDED
#define DFCOMMONINTERNAL_H_INCLUDED

// this makes everything that includes this file export symbols when using DFHACK_EXPORT (see DFExport.h)
#ifndef BUILD_DFHACK_LIB
    #define BUILD_DFHACK_LIB
#endif

// wizardry for the cmake-generated config.h
#define _QUOTEME(x) #x
#define QUOT(x) _QUOTEME(x)

#ifdef USE_CONFIG_H // set by cmake for Linux builds
    #include "config.h"
#else
    #define MEMXML_DATA_PATH .
#endif

// one file for globals
#include "dfhack/DFGlobal.h"

// one file for telling the MSVC compiler where it can shove its pointless warnings
#include "dfhack/DFPragma.h"

// basic stl containers and IO stuff
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
using namespace std;

// C99 integer types
#include "dfhack/DFIntegers.h"
// C includes
#include <assert.h>
#include <string.h>
#include <stdlib.h>

// platform includes and defines
#ifdef LINUX_BUILD
    #include <sys/types.h>
    #include <sys/ptrace.h>
    #include <dirent.h>
    #define __USE_FILE_OFFSET64
    #define _FILE_OFFSET_BITS 64
    #include <unistd.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <sys/wait.h>
#else
    // WINDOWS
    #define _WIN32_WINNT 0x0501 // needed for INPUT struct
    #define WINVER 0x0501       // OpenThread(), PSAPI, Toolhelp32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <winnt.h>
    #include <psapi.h>
    #include <tlhelp32.h>
#endif

// dfhack dependencies
#include <tinyxml.h>
#include <md5wrapper.h>

/*
#if defined(_MSC_VER) && _MSC_VER >= 1400
#define fill_char_buf(buf, str) strcpy_s((buf), sizeof(buf) / sizeof((buf)[0]), (str).c_str())
#else
#define fill_char_buf(buf, str) strncpy((buf), (str).c_str(), sizeof(buf) / sizeof((buf)[0]))
#endif
*/

#endif // DFCOMMONINTERNAL_H_INCLUDED

