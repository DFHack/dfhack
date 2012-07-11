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

typedef struct interpose_s
{
  void *new_func;
  void *orig_func;
} interpose_t;

#include "DFHack.h"
#include "Core.h"
#include "Hooks.h"
#include <iostream>

/*static const interpose_t interposers[] __attribute__ ((section("__DATA, __interpose"))) = 
{
     { (void *)DFH_SDL_Init,  (void *)SDL_Init  },
     { (void *)DFH_SDL_PollEvent, (void *)SDL_PollEvent },
     { (void *)DFH_SDL_Quit, (void *)SDL_Quit },
     { (void *)DFH_SDL_NumJoysticks, (void *)SDL_NumJoysticks },
     
};*/

#define DYLD_INTERPOSE(_replacment,_replacee) __attribute__((used)) static struct{ const void* replacment; const void* replacee; } _interpose_##_replacee __attribute__ ((section ("__DATA,__interpose"))) = { (const void*)(unsigned long)&_replacment, (const void*)(unsigned long)&_replacee }; 

DYLD_INTERPOSE(DFH_SDL_Init,SDL_Init);
DYLD_INTERPOSE(DFH_SDL_PollEvent,SDL_PollEvent);
DYLD_INTERPOSE(DFH_SDL_Quit,SDL_Quit);
DYLD_INTERPOSE(DFH_SDL_NumJoysticks,SDL_NumJoysticks);

/*******************************************************************************
*                           SDL part starts here                               *
*******************************************************************************/
// hook - called for each game tick (or more often)
DFhackCExport int DFH_SDL_NumJoysticks(void)
{
    DFHack::Core & c = DFHack::Core::getInstance();
    return c.Update();
}

// hook - called at program exit
static void (*_SDL_Quit)(void) = 0;
DFhackCExport void DFH_SDL_Quit(void)
{
    DFHack::Core & c = DFHack::Core::getInstance();
    c.Shutdown();
    /*if(_SDL_Quit)
    {
        _SDL_Quit();
    }*/
    
    SDL_Quit();
}

// called by DF to check input events
static int (*_SDL_PollEvent)(SDL::Event* event) = 0;
DFhackCExport int DFH_SDL_PollEvent(SDL::Event* event)
{
    pollevent_again:
    // if SDL returns 0 here, it means there are no more events. return 0
    int orig_return = SDL_PollEvent(event);
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
DFhackCExport int DFH_SDL_Init(uint32_t flags)
{
    // reroute stderr
    fprintf(stderr,"dfhack: attempting to hook in\n");
    freopen("stderr.log", "w", stderr);
    // we don't reroute stdout until  we figure out if this should be done at all
    // See: Console-linux.cpp

    // find real functions
    fprintf(stderr,"dfhack: saving real SDL functions\n");
    _SDL_Init = (int (*)( uint32_t )) dlsym(RTLD_NEXT, "SDL_Init");
    _SDL_Quit = (void (*)( void )) dlsym(RTLD_NEXT, "SDL_Quit");
    _SDL_PollEvent = (int (*)(SDL::Event*))dlsym(RTLD_NEXT,"SDL_PollEvent");

    fprintf(stderr,"dfhack: saved real SDL functions\n");
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
    
    DFHack::Core & c = DFHack::Core::getInstance();
    //c.Init();
    
    //int ret = _SDL_Init(flags);
    int ret = SDL_Init(flags);
    return ret;
}