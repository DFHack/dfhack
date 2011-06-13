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
#include "core.h"
#include "DFHack.h"
#include "dfhack/VersionInfoFactory.h"
#include "dfhack/Context.h"
#include "dfhack/modules/Vegetation.h"
#include <iostream>


#define DFhackCExport extern "C" __attribute__ ((visibility("default")))

int errorstate = 0;
bool inited = 0;
DFHack::VersionInfoFactory * vif = 0;
DFHack::Process * p = 0;
DFHack::Context * c = 0;
DFHack::Gui * gui = 0;
DFHack::Maps * maps = 0;
DFHack::Vegetation * veg = 0;

uint32_t OS_getPID()
{
    return getpid();
}

void SHM_Init ( void )
{
    // check that we do this only once per process
    if(inited)
    {
        std::cerr << "SDL_Init was called twice or more!" << std::endl;
        return;
    }
    vif = new DFHack::VersionInfoFactory("Memory.xml");
    p = new DFHack::Process(vif);
    if (!p->isIdentified())
    {
        std::cerr << "Couldn't identify this version of DF." << std::endl;
        errorstate = 1;
    }
    c = new DFHack::Context(p);
    gui = c->getGui();
    gui->Start();
    veg = c->getVegetation();
    veg->Start();
    maps = c->getMaps();
    maps->Start();
    inited = true;
}

int32_t x = 0,y = 0,z = 0;
int32_t xo = 0,yo = 0,zo = 0;

void print_tree( DFHack::df_plant & tree)
{
    //DFHack::Materials * mat = DF->getMaterials();
    printf("%d:%d = ",tree.type,tree.material);
    if(tree.watery)
    {
        std::cout << "near-water ";
    }
    //std::cout << mat->organic[tree.material].id << " ";
    if(!tree.is_shrub)
    {
        std::cout << "tree";
    }
    else
    {
        std::cout << "shrub";
    }
    std::cout << std::endl;
    printf("Grow counter: 0x%08x\n", tree.grow_counter);
    printf("temperature 1: %d\n", tree.temperature_1);
    printf("temperature 2: %d\n", tree.temperature_2);
    printf("On fire: %d\n", tree.is_burning);
    printf("hitpoints: 0x%08x\n", tree.hitpoints);
    printf("update order: %d\n", tree.update_order);
    printf("Address: 0x%x\n", &tree);
    //hexdump(DF,tree.address,13*16);
}

void SHM_Act()
{
    maps->Start();
    gui->getCursorCoords(x,y,z);
    if(x != xo || y!= yo || z != zo)
    {
        xo = x;
        yo = y;
        zo = z;
        std::cout << "Cursor: " << x << "/" << y << "/" << z << std::endl;
        if(x != -30000)
        {
            std::vector <DFHack::df_plant *> * vec;
            if(maps->ReadVegetation(x/16,y/16,z,vec))
            {
                for(size_t i = 0; i < vec->size();i++)
                {
                    DFHack::df_plant * p = vec->at(i);
                    if(p->x == x && p->y == y && p->z == z)
                    {
                        print_tree(*p);
                    }
                }
            }
            else
                std::cout << "No veg vector..." << std::endl;
        }
    }
};

void SHM_Destroy ( void )
{
    if(inited && !errorstate)
    {
        inited = false;
    }
}

/*******************************************************************************
*                           SDL part starts here                               *
*******************************************************************************/
DFhackCExport int SDL_NumJoysticks(void)
{
    if(errorstate)
        return -1;
    if(!inited)
    {
        SHM_Init();
        return -2;
    }
    SHM_Act();
    return -3;
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
    if(!errorstate)
    {
        SHM_Destroy();
        errorstate = true;
    }
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
        errorstate = true;
        exit(1);
    }
    //SHM_Init();
    
    return _SDL_Init(flags);
}
//*/
/*******************************************************************************
*                           NCURSES part starts here                           *
*******************************************************************************/

static int (*_refresh)(void) = 0;
DFhackCExport int refresh (void)
{
    if(_refresh)
    {
        /*
        if(!errorstate)
        {
            SHM_Act();
        }
        counter ++;
        */
        return _refresh();
    }
    return 0;
}

static int (*_endwin)(void) = 0;
DFhackCExport int endwin (void)
{
    if(!errorstate)
    {
        SHM_Destroy();
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
