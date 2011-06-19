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

#pragma once

/*
 *  Some much needed SDL fakery.
 */

#include "dfhack/Pragma.h"
#include "dfhack/Export.h"

#ifdef LINUX_BUILD
    #define DFhackCExport extern "C" __attribute__ ((visibility("default")))
#else
    #define DFhackCExport extern "C" __declspec(dllexport)
#endif

// function and variable pointer... we don't try to understand what SDL does here
typedef void * fPtr;
typedef void * vPtr;
struct DFMutex;
struct DFThread;
struct DFLibrary;

// mutex and thread functions. We can call these.
DFhackCExport DFMutex * SDL_CreateMutex(void);
DFhackCExport int SDL_mutexP(DFMutex *);
DFhackCExport int SDL_mutexV(DFMutex *);
DFhackCExport void SDL_DestroyMutex(DFMutex *);
DFhackCExport DFThread *SDL_CreateThread(int (*fn)(void *), void *data);

// Functions for loading libraries and looking up exported symbols
DFhackCExport void * SDL_LoadFunction(DFLibrary *handle, const char *name);
DFhackCExport DFLibrary * SDL_LoadObject(const char *sofile);
DFhackCExport void SDL_UnloadObject(DFLibrary * handle);

// these functions are here because they call into DFHack::Core and therefore need to
// be declared as friend functions/known
DFhackCExport int SDL_NumJoysticks(void);
DFhackCExport void SDL_Quit(void);
/*
// not yet.
DFhackCExport int SDL_Init(uint32_t flags);
DFhackCExport int SDL_PollEvent(vPtr event);
*/

// Other crud is in the OS-specific core files.
