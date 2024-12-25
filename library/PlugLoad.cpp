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

#include "Core.h"
#include "DFHack.h"
#include "Debug.h"
#include "Export.h"
#include "PluginManager.h"
#include "Hooks.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <stdio.h>
#include <stdint.h>

#ifdef WIN32
#define NOMINMAX
#include <windows.h>
#define global_search_handle() GetModuleHandle(nullptr)
#define get_function_address(plugin, function) GetProcAddress((HMODULE)plugin, function)
#define clear_error()
#define load_library(fn) LoadLibrary(fn)
#define close_library(handle) (!(FreeLibrary((HMODULE)handle)))
#else
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#define global_search_handle() (RTLD_DEFAULT)
#define get_function_address(plugin, function) dlsym((void*)plugin, function)
#define clear_error() dlerror()
#define load_library(fn) dlopen(fn, RTLD_NOW | RTLD_LOCAL);
#define close_library(handle) dlclose((void*)handle)
#endif

/*
 * Plugin loading functions
 */
namespace DFHack
{
    DBG_DECLARE(core, plugins, DebugCategory::LINFO);

    DFHack::DFLibrary* GLOBAL_NAMES = (DFLibrary*)global_search_handle();

    namespace {
        std::string get_error()
        {
#ifdef WIN32
            DWORD error = GetLastError();
            LPSTR buf = NULL;
            DWORD len = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buf, 0, NULL);
            if (len > 0) {
                std::string message{ buf };
                LocalFree(buf);
                return message;
            }
            std::ostringstream b{};
            b << "unknown error " << error;
            return b.str();
#else
            char* error = dlerror();
            if (error)
                return std::string{ error };
            return std::string{};
#endif
        }
    }

    DFLibrary * OpenPlugin (const char * filename)
    {
        clear_error();
        DFLibrary* ret = (DFLibrary*)load_library(filename);
        if (!ret) {
            auto error = get_error();
            WARN(plugins).print("OpenPlugin on %s failed: %s\n", filename, error.c_str());
        }
        return ret;
    }
    void * LookupPlugin (DFLibrary * plugin ,const char * function)
    {
        return (void *) get_function_address(plugin, function);
    }
    bool ClosePlugin (DFLibrary * plugin)
    {
        int res = close_library(plugin);
        if (res != 0)
        {
            auto error = get_error();
            WARN(plugins).print("ClosePlugin failed: %s\n", error.c_str());
        }
        return (res == 0);

    }
}
