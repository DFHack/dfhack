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
bool FirstCall(void);
bool inited = false;

DFhackCExport int SDL_NumJoysticks(void)
{
    DFHack::Core & c = DFHack::Core::getInstance();
    // the 'inited' variable should be normally protected by a lock. It isn't
    // this is harmless enough. only thing this can cause is a slight delay before
    // DF input events start to be processed by Core
    int ret = c.Update();
    if(ret == 0)
        inited = true;
    return ret;
}

// ptr to the real functions
//static void (*_SDL_GL_SwapBuffers)(void) = 0;
static void (*_SDL_Quit)(void) = 0;
static int (*_SDL_Init)(uint32_t flags) = 0;
static SDL::Thread * (*_SDL_CreateThread)(int (*fn)(void *), void *data) = 0;
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

static SDL::Mutex * (*_SDL_CreateMutex)(void) = 0;
DFhackCExport SDL::Mutex * SDL_CreateMutex(void)
{
    return _SDL_CreateMutex();
}

static int (*_SDL_mutexP)(SDL::Mutex * mutex) = 0;
DFhackCExport int SDL_mutexP(SDL::Mutex * mutex)
{
    return _SDL_mutexP(mutex);
}

static int (*_SDL_mutexV)(SDL::Mutex * mutex) = 0;
DFhackCExport int SDL_mutexV(SDL::Mutex * mutex)
{
    return _SDL_mutexV(mutex);
}

static void (*_SDL_DestroyMutex)(SDL::Mutex * mutex) = 0;
DFhackCExport void SDL_DestroyMutex(SDL::Mutex * mutex)
{
    _SDL_DestroyMutex(mutex);
}

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

// called by DF to check input events
static int (*_SDL_PollEvent)(SDL::Event* event) = 0;
DFhackCExport int SDL_PollEvent(SDL::Event* event)
{
    int orig_return = _SDL_PollEvent(event);
    // only send events to Core after we get first SDL_NumJoysticks call
    // DF event loop is possibly polling for SDL events before things get inited properly
    // SDL handles it. We don't, because we use some other parts of SDL too.

    // possible data race. whatever. it's a flag, we don't mind all that much
    if(inited && event != 0)
    {
        DFHack::Core & c = DFHack::Core::getInstance();
        return c.SDL_Event(event, orig_return);
    }
    return orig_return;
}

static uint32_t (*_SDL_ThreadID)(void) = 0;
DFhackCExport uint32_t SDL_ThreadID()
{
    return _SDL_ThreadID();
}

static SDL::Cond * (*_SDL_CreateCond)(void) = 0;
DFhackCExport SDL::Cond *SDL_CreateCond(void)
{
    return _SDL_CreateCond();
}
static void (*_SDL_DestroyCond)(SDL::Cond *) = 0;
DFhackCExport void SDL_DestroyCond(SDL::Cond *cond)
{
    _SDL_DestroyCond(cond);
}
static int (*_SDL_CondSignal)(SDL::Cond *) = 0;
DFhackCExport int SDL_CondSignal(SDL::Cond *cond)
{
    return _SDL_CondSignal(cond);
}
static int (*_SDL_CondWait)(SDL::Cond *, SDL::Mutex *) = 0;
DFhackCExport int SDL_CondWait(SDL::Cond *cond, SDL::Mutex * mut)
{
    return _SDL_CondWait(cond, mut);
}

// hook - called at program start, initialize some stuffs we'll use later
DFhackCExport int SDL_Init(uint32_t flags)
{
    freopen("stdout.log", "w", stdout);
    freopen("stderr.log", "w", stderr);
    // horrible casts not supported by the C or C++ standards. Only POSIX. Damn you, POSIX.
    // find real functions
    //_SDL_GL_SwapBuffers = (void (*)( void )) dlsym(RTLD_NEXT, "SDL_GL_SwapBuffers");
    _SDL_Init = (int (*)( uint32_t )) dlsym(RTLD_NEXT, "SDL_Init");
    //_SDL_Flip = (int (*)( void * )) dlsym(RTLD_NEXT, "SDL_Flip");
    _SDL_Quit = (void (*)( void )) dlsym(RTLD_NEXT, "SDL_Quit");
    _SDL_CreateThread = (SDL::Thread* (*)(int (*fn)(void *), void *data))dlsym(RTLD_NEXT, "SDL_CreateThread");
    _SDL_CreateMutex = (SDL::Mutex*(*)())dlsym(RTLD_NEXT,"SDL_CreateMutex");
    _SDL_DestroyMutex = (void (*)(SDL::Mutex*))dlsym(RTLD_NEXT,"SDL_DestroyMutex");
    _SDL_mutexP = (int (*)(SDL::Mutex*))dlsym(RTLD_NEXT,"SDL_mutexP");
    _SDL_mutexV = (int (*)(SDL::Mutex*))dlsym(RTLD_NEXT,"SDL_mutexV");
    _SDL_PollEvent = (int (*)(SDL::Event*))dlsym(RTLD_NEXT,"SDL_PollEvent");
    _SDL_ThreadID = (uint32_t (*)())dlsym(RTLD_NEXT,"SDL_ThreadID");
    
    _SDL_CreateCond = (SDL::Cond * (*)())dlsym(RTLD_NEXT,"SDL_CreateCond");
    _SDL_DestroyCond = (void(*)(SDL::Cond *))dlsym(RTLD_NEXT,"SDL_DestroyCond");
    _SDL_CondSignal = (int (*)(SDL::Cond *))dlsym(RTLD_NEXT,"SDL_CondSignal");
    _SDL_CondWait = (int (*)(SDL::Cond *, SDL::Mutex *))dlsym(RTLD_NEXT,"SDL_CondWait");

    // check if we got them
    if(_SDL_Init && _SDL_Quit && _SDL_CreateThread
        && _SDL_CreateMutex && _SDL_DestroyMutex && _SDL_mutexP
         && _SDL_mutexV && _SDL_PollEvent && _SDL_ThreadID
      && _SDL_CondSignal && _SDL_CondWait && _SDL_CreateCond && _SDL_DestroyCond)
    {
        fprintf(stderr,"dfhack: hooking successful\n");
    }
    else
    {
        // bail, this would be a disaster otherwise
        fprintf(stderr,"dfhack: something went horribly wrong\n");
        exit(1);
    }
    int ret = _SDL_Init(flags);
    return ret;
}
