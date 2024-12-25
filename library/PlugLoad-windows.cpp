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

#define DFhackCExport extern "C" __declspec(dllexport)

#define NOMINMAX
#include <windows.h>
#include <stdint.h>
#include <vector>
#include <string>
#include "Core.h"
#include "Debug.h"
#include "PluginManager.h"
#include "Hooks.h"
#include <stdio.h>

/*
 * Plugin loading functions
 */
namespace DFHack
{
    DBG_DECLARE(core, plugins, DebugCategory::LINFO);

    DFLibrary* GLOBAL_NAMES = (DFLibrary*)GetModuleHandle(nullptr);
    DFLibrary * OpenPlugin (const char * filename)
    {
        DFLibrary *ret = (DFLibrary *) LoadLibrary(filename);
        if (!ret) {
            DWORD error = GetLastError();
            LPSTR buf = NULL;
            DWORD len = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buf, 0, NULL);
            if (len > 0) {
                std::string message = buf;
                std::cerr << "LoadLibrary " << filename << ": " << rtrim(message) << " (" << error << ")" << std::endl;
                LocalFree(buf);
            }
            else {
                std::cerr << "LoadLibrary " << filename << ": unknown error " << error << std::endl;
            }
        }
        return ret;
    }
    void * LookupPlugin (DFLibrary * plugin ,const char * function)
    {
        return (void *) GetProcAddress((HMODULE)plugin, function);
    }
    bool ClosePlugin (DFLibrary * plugin)
    {
        // FreeLibrary returns nonzero on success, zero on failure
        int res = FreeLibrary((HMODULE)plugin);
        if (res == 0)
        {
            DWORD err = GetLastError();
            WARN(plugins).print("FreeLibrary failed with error code %x\n", err);
        }
        return (res != 0);

    }
}
