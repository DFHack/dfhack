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
#include "Core.h"
#include "Hooks.h"
#include <iostream>

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
    pollevent_again:
    // if SDL returns 0 here, it means there are no more events. return 0
    int orig_return = _SDL_PollEvent(event);
    if(!orig_return)
        return 0;
    // otherwise we have an event to filter
    else if( event != 0 )
    {
        DFHack::Core & c = DFHack::Core::getInstance();
        // if we consume the event, ask SDL for more.
        if(!c.DFH_SDL_Event(event))
            goto pollevent_again;
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
    // reroute stderr
    if (!freopen("stderr.log", "w", stderr))
        fprintf(stderr, "dfhack: failed to reroute stderr\n");
    // we don't reroute stdout until  we figure out if this should be done at all
    // See: Console-linux.cpp

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
