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

#define DFhackCExport extern "C" __declspec(dllexport)

#include <windows.h>
#include <stdint.h>
#include <vector>
#include <string>
#include "Core.h"
#include "PluginManager.h"
#include "Hooks.h"
#include <stdio.h>

#include "tinythread.h"
#include "modules/Graphic.h"

/*
 * Plugin loading functions
 */
namespace DFHack
{
    DFLibrary * OpenPlugin (const char * filename)
    {
        return (DFLibrary *) LoadLibrary(filename);
    }
    void * LookupPlugin (DFLibrary * plugin ,const char * function)
    {
        return (void *) GetProcAddress((HMODULE)plugin, function);
    }
    void ClosePlugin (DFLibrary * plugin)
    {
        FreeLibrary((HMODULE) plugin);
    }
}