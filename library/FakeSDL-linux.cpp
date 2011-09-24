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


#include <stdio.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <map>

#include "DFHack.h"
#include "dfhack/Core.h"
#include "dfhack/FakeSDL.h"
#include <iostream>

/*
 * Plugin loading functions
 */
namespace DFHack
{
    DFLibrary * OpenPlugin (const char * filename)
    {
        dlerror();
        DFLibrary * ret =  (DFLibrary *) dlopen(filename, RTLD_NOW);
        if(!ret)
        {
            std::cerr << dlerror() << std::endl;
        }
        return ret;
    }
    void * LookupPlugin (DFLibrary * plugin ,const char * function)
    {
        return (DFLibrary *) dlsym((void *)plugin, function);
    }
    void ClosePlugin (DFLibrary * plugin)
    {
        dlclose((void *) plugin);
    }
}

/*******************************************************************************
*                           SDL part starts here                               *
*******************************************************************************/
// hook - called for each game tick (or more often)
DFhackCExport int SDL_NumJoysticks(void)
{
    DFHack::Core & c = DFHack::Core::getInstance();
    return c.Update();
}

// hook - called at program exit
static void (*_SDL_Quit)(void) = 0;
DFhackCExport void SDL_Quit(void)
{
    DFHack::Core & c = DFHack::Core::getInstance();
    c.Shutdown();
    if(_SDL_Quit)
    {
        _SDL_Quit();
    }
}

// called by DF to check input events
static int (*_SDL_PollEvent)(SDL::Event* event) = 0;
DFhackCExport int SDL_PollEvent(SDL::Event* event)
{
    int orig_return = _SDL_PollEvent(event);
    // if the event is valid, intercept
    if( event != 0 )
    {
        DFHack::Core & c = DFHack::Core::getInstance();
        return c.SDL_Event(event, orig_return);
    }
    return orig_return;
}

struct WINDOW;
DFhackCExport int wgetch(WINDOW *win)
{
    static int (*_wgetch)(WINDOW * win) = (int (*)( WINDOW * )) dlsym(RTLD_NEXT, "wgetch");
    if(!_wgetch)
    {
        exit(EXIT_FAILURE);
    }
    DFHack::Core & c = DFHack::Core::getInstance();
    wgetch_again:
    int in = _wgetch(win);
    int out;
    if(c.ncurses_wgetch(in, out))
    {
        // not consumed, give to DF
        return out;
    }
    else
    {
        // consumed, repeat
        goto wgetch_again;
    }
}

// hook - called at program start, initialize some stuffs we'll use later
static int (*_SDL_Init)(uint32_t flags) = 0;
DFhackCExport int SDL_Init(uint32_t flags)
{
    // find real functions
    _SDL_Init = (int (*)( uint32_t )) dlsym(RTLD_NEXT, "SDL_Init");
    _SDL_Quit = (void (*)( void )) dlsym(RTLD_NEXT, "SDL_Quit");
    _SDL_PollEvent = (int (*)(SDL::Event*))dlsym(RTLD_NEXT,"SDL_PollEvent");

    // check if we got them
    if(_SDL_Init && _SDL_Quit && _SDL_PollEvent)
    {
        fprintf(stderr,"dfhack: hooking successful\n");
    }
    else
    {
        // bail, this would be a disaster otherwise
        fprintf(stderr,"dfhack: something went horribly wrong\n");
        exit(1);
    }
    /*
    DFHack::Core & c = DFHack::Core::getInstance();
    c.Init();
    */
    int ret = _SDL_Init(flags);
    return ret;
}

#ifdef MALLOC_FILL
DFhackCExport void * malloc(size_t size)
{
    static void* (*real_malloc)(size_t) = NULL;
    if (!real_malloc)
        real_malloc = (void * (*)( size_t )) dlsym(RTLD_NEXT, "malloc");
    // check if we got them
    if(!real_malloc)
    {
        // bail, this would be a disaster otherwise
        exit(1);
    }
    void * ret = real_malloc(size);
    if(ret)
    {
        memset(ret, 0xCC, size);
    }
    return ret;
}
#endif
