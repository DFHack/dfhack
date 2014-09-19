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
DYLD_INTERPOSE(DFH_SDL_CreateRGBSurface,SDL_CreateRGBSurface);
DYLD_INTERPOSE(DFH_SDL_CreateRGBSurfaceFrom,SDL_CreateRGBSurfaceFrom);
DYLD_INTERPOSE(DFH_SDL_UnlockSurface,SDL_UnlockSurface);
DYLD_INTERPOSE(DFH_SDL_LockSurface,SDL_LockSurface);
DYLD_INTERPOSE(DFH_SDL_ConvertSurface,SDL_ConvertSurface);
DYLD_INTERPOSE(DFH_SDL_FreeSurface,SDL_FreeSurface);
DYLD_INTERPOSE(DFH_SDL_GetMouseState,SDL_GetMouseState);
DYLD_INTERPOSE(DFH_SDL_GetVideoSurface,SDL_GetVideoSurface);
DYLD_INTERPOSE(DFH_SDL_UpperBlit,SDL_UpperBlit);

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

// New SDL functions starting in r5
static int (*_SDL_CreateRGBSurface)(uint32_t flags, int width, int height, int depth,
                                     uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) = 0;
DFhackCExport vPtr DFH_SDL_CreateRGBSurface(uint32_t flags, int width, int height, int depth,
                                     uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask)
{
    return SDL_CreateRGBSurface(flags, width, height, depth, Rmask, Gmask, Bmask, Amask);
}

static vPtr (*_SDL_CreateRGBSurfaceFrom)(vPtr pixels, int width, int height, int depth, int pitch,
                                         uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) = 0;
DFhackCExport vPtr DFH_SDL_CreateRGBSurfaceFrom(vPtr pixels, int width, int height, int depth, int pitch,
                                         uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask)
{
    return SDL_CreateRGBSurfaceFrom(pixels, width, height, depth, pitch, Rmask, Gmask, Bmask, Amask);
}

static void (*_SDL_FreeSurface)(vPtr surface) = 0;
DFhackCExport void DFH_SDL_FreeSurface(vPtr surface)
{
    SDL_FreeSurface(surface);
}

static vPtr (*_SDL_ConvertSurface)(vPtr surface, vPtr format, uint32_t flags) = 0;
DFhackCExport vPtr DFH_SDL_ConvertSurface(vPtr surface, vPtr format, uint32_t flags)
{
    return SDL_ConvertSurface(surface, format, flags);
}

static int (*_SDL_LockSurface)(vPtr surface) = 0;
DFhackCExport int DFH_SDL_LockSurface(vPtr surface)
{
    return SDL_LockSurface(surface);
}

static void (*_SDL_UnlockSurface)(vPtr surface) = 0;
DFhackCExport void DFH_SDL_UnlockSurface(vPtr surface)
{
    SDL_UnlockSurface(surface);
}

static uint8_t (*_SDL_GetMouseState)(int *, int *) = 0;
DFhackCExport uint8_t DFH_SDL_GetMouseState(int *x, int *y)
{
    return SDL_GetMouseState(x,y);
}

static void * (*_SDL_GetVideoSurface)( void ) = 0;
DFhackCExport void * DFH_SDL_GetVideoSurface(void)
{
    return SDL_GetVideoSurface();
}

static int (*_SDL_UpperBlit)(DFHack::DFSDL_Surface* src, DFHack::DFSDL_Rect* srcrect, DFHack::DFSDL_Surface* dst, DFHack::DFSDL_Rect* dstrect) = 0;
DFhackCExport int DFH_SDL_UpperBlit(DFHack::DFSDL_Surface* src, DFHack::DFSDL_Rect* srcrect, DFHack::DFSDL_Surface* dst, DFHack::DFSDL_Rect* dstrect)
{
    if ( dstrect != NULL && dstrect->h != 0 && dstrect->w != 0 )
    {
        DFHack::Core & c = DFHack::Core::getInstance();
        DFHack::Graphic* g = c.getGraphic();
        DFHack::DFTileSurface* ov = g->Call(dstrect->x/dstrect->w, dstrect->y/dstrect->h);

        if ( ov != NULL )
        {
            if ( ov->paintOver )
            {
                SDL_UpperBlit(src, srcrect, dst, dstrect);
            }

            DFHack::DFSDL_Rect* dstrect2 = new DFHack::DFSDL_Rect;
            dstrect2->x = dstrect->x;
            dstrect2->y = dstrect->y;
            dstrect2->w = dstrect->w;
            dstrect2->h = dstrect->h;

            if ( ov->dstResize != NULL )
            {
                DFHack::DFSDL_Rect* r = (DFHack::DFSDL_Rect*)ov->dstResize;
                dstrect2->x += r->x;
                dstrect2->y += r->y;
                dstrect2->w += r->w;
                dstrect2->h += r->h;
            }

            int result = SDL_UpperBlit(ov->surface, ov->rect, dst, dstrect2);
            delete dstrect2;
            return result;
        }
    }

    return SDL_UpperBlit(src, srcrect, dst, dstrect);
}