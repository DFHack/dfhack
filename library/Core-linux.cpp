/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2011 Petr Mrázek (peterix)

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
#include <iostream>

#define DFhackCExport extern "C" __attribute__ ((visibility("default")))

/*******************************************************************************
*                           SDL part starts here                               *
*******************************************************************************/
DFhackCExport int SDL_NumJoysticks(void)
{
    DFHack::Core & c = DFHack::Core::getInstance();
    return c.Update();
}

// ptr to the real functions
//static void (*_SDL_GL_SwapBuffers)(void) = 0;
static void (*_SDL_Quit)(void) = 0;
static int (*_SDL_Init)(uint32_t flags) = 0;
//static int (*_SDL_Flip)(void * some_ptr) = 0;
/*
// hook - called every tick in OpenGL mode of DF
DFhackCExport void SDL_GL_SwapBuffers(void)
{
    if(_SDL_GL_SwapBuffers)
    {
        if(!errorstate)
        {
            SHM_Act();
        }
        counter ++;
        _SDL_GL_SwapBuffers();
    }
}
*/
/*
// hook - called every tick in the 2D mode of DF
DFhackCExport int SDL_Flip(void * some_ptr)
{
    if(_SDL_Flip)
    {
        if(!errorstate)
        {
            SHM_Act();
        }
        counter ++;
        return _SDL_Flip(some_ptr);
    }
    return 0;
}
*/

// hook - called at program exit
DFhackCExport void SDL_Quit(void)
{
    DFHack::Core & c = DFHack::Core::getInstance();
    c.Shutdown();
    if(_SDL_Quit)
    {
        _SDL_Quit();
    }
}

// hook - called at program start, initialize some stuffs we'll use later
DFhackCExport int SDL_Init(uint32_t flags)
{
    // find real functions
    //_SDL_GL_SwapBuffers = (void (*)( void )) dlsym(RTLD_NEXT, "SDL_GL_SwapBuffers");
    _SDL_Init = (int (*)( uint32_t )) dlsym(RTLD_NEXT, "SDL_Init");
    //_SDL_Flip = (int (*)( void * )) dlsym(RTLD_NEXT, "SDL_Flip");
    _SDL_Quit = (void (*)( void )) dlsym(RTLD_NEXT, "SDL_Quit");

    // check if we got them
    if(/*_SDL_GL_SwapBuffers &&*/ _SDL_Init && _SDL_Quit)
    {
        fprintf(stderr,"dfhack: hooking successful\n");
    }
    else
    {
        // bail, this would be a disaster otherwise
        fprintf(stderr,"dfhack: something went horribly wrong\n");
        exit(1);
    }
    return _SDL_Init(flags);
}


/*******************************************************************************
*                           NCURSES part starts here                           *
*******************************************************************************/
/*
static int (*_refresh)(void) = 0;
DFhackCExport int refresh (void)
{
    if(_refresh)
    {
        return _refresh();
    }
    return 0;
}

static int (*_endwin)(void) = 0;
DFhackCExport int endwin (void)
{
    if(!errorstate)
    {
        HOOK_Shutdown();
        errorstate = true;
    }
    if(_endwin)
    {
        return _endwin();
    }
    return 0;
}

typedef void WINDOW;
static WINDOW * (*_initscr)(void) = 0;
DFhackCExport WINDOW * initscr (void)
{
    // find real functions
    //_refresh = (int (*)( void )) dlsym(RTLD_NEXT, "refresh");
    _endwin = (int (*)( void )) dlsym(RTLD_NEXT, "endwin");
    _initscr = (WINDOW * (*)( void )) dlsym(RTLD_NEXT, "initscr");
    // check if we got them
    if(_refresh && _endwin && _initscr)
    {
        fprintf(stderr,"dfhack: hooking successful\n");
    }
    else
    {
        // bail, this would be a disaster otherwise
        fprintf(stderr,"dfhack: something went horribly wrong\n");
        exit(1);
    }
    //SHM_Init();
    return _initscr();
}
*/