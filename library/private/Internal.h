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

// wizardry for adding quotes to macros
#define _QUOTEME(x) #x
#define QUOT(x) _QUOTEME(x)

#ifdef LINUX_BUILD
    #define __USE_FILE_OFFSET64
    #define _FILE_OFFSET_BITS 64
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
    //#include <Dbghelp.h> // NOT present in mingw32
    typedef LONG NTSTATUS;
    #define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

    // FIXME: it is uncertain how these map to 64bit
    typedef struct _DEBUG_BUFFER
    {
        HANDLE SectionHandle;
        PVOID  SectionBase;
        PVOID  RemoteSectionBase;
        ULONG  SectionBaseDelta;
        HANDLE  EventPairHandle;
        ULONG  Unknown[2];
        HANDLE  RemoteThreadHandle;
        ULONG  InfoClassMask;
        ULONG  SizeOfInfo;
        ULONG  AllocatedSize;
        ULONG  SectionSize;
        PVOID  ModuleInformation;
        PVOID  BackTraceInformation;
        PVOID  HeapInformation;
        PVOID  LockInformation;
        PVOID  Reserved[8];
    } DEBUG_BUFFER, *PDEBUG_BUFFER;

    typedef struct _DEBUG_HEAP_INFORMATION
    {
        ULONG Base; // 0×00
        ULONG Flags; // 0×04
        USHORT Granularity; // 0×08
        USHORT Unknown; // 0x0A
        ULONG Allocated; // 0x0C
        ULONG Committed; // 0×10
        ULONG TagCount; // 0×14
        ULONG BlockCount; // 0×18
        ULONG Reserved[7]; // 0x1C
        PVOID Tags; // 0×38
        PVOID Blocks; // 0x3C
    } DEBUG_HEAP_INFORMATION, *PDEBUG_HEAP_INFORMATION;

    // RtlQueryProcessDebugInformation.DebugInfoClassMask constants
    #define PDI_MODULES                       0x01
    #define PDI_BACKTRACE                     0x02
    #define PDI_HEAPS                         0x04
    #define PDI_HEAP_TAGS                     0x08
    #define PDI_HEAP_BLOCKS                   0x10
    #define PDI_LOCKS                         0x20

    extern "C" __declspec(dllimport) NTSTATUS __stdcall RtlQueryProcessDebugInformation( IN ULONG  ProcessId, IN ULONG  DebugInfoClassMask, IN OUT PDEBUG_BUFFER  DebugBuffer);
    extern "C" __declspec(dllimport) PDEBUG_BUFFER __stdcall RtlCreateQueryDebugBuffer( IN ULONG  Size, IN BOOLEAN  EventPair);
    extern "C" __declspec(dllimport) NTSTATUS __stdcall RtlDestroyQueryDebugBuffer( IN PDEBUG_BUFFER  DebugBuffer);
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

