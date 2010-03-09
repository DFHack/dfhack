/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix)

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

/**
 * This is the source for the DF <-> dfhack shm bridge
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
#include "shms.h"
#include "mod-core.h"
#include <sched.h>

#define DFhackCExport extern "C" __attribute__ ((visibility("default")))

// various crud
int counter = 0;
int errorstate = 0;
char *shm = 0;
int shmid = 0;
bool inited = 0;

int fd_svlock = 0;
int fd_cllock = 0;


/*******************************************************************************
*                           SHM part starts here                               *
*******************************************************************************/
/*
// FIXME: add error checking?
bool isValidSHM()
{
    shmid_ds descriptor;
    shmctl(shmid, IPC_STAT, &descriptor);
    //fprintf(stderr,"ID %d, attached: %d\n",shmid, descriptor.shm_nattch);
    return (descriptor.shm_nattch == 2);
}
*/

// is the other side still there?
bool isValidSHM()
{
    // try if CL lock file is free
    int result = lockf(fd_cllock,F_TEST,0);
    /*
    F_TEST: Test the lock:
    return 0 if the specified section is unlocked or locked by this process;
    return -1, set errno to EAGAIN (EACCES on some other systems), if another process holds a lock.
    */
    
    // if nobody holds the lock, the SHM isn't valid. file locks are unlocked when the owner closes or crashes.
    if(result == 0) return false;
    
    return true;
}

uint32_t OS_getPID()
{
    return getpid();
}

uint32_t OS_getAffinity()
{
    cpu_set_t mask;
    sched_getaffinity(0,sizeof(cpu_set_t),&mask);
    // FIXME: truncation
    uint32_t affinity = *(uint32_t *) &mask;
    return affinity;
}

void SHM_Init ( void )
{
    // check that we do this only once per process
    if(inited)
    {
        fprintf(stderr, "SDL_Init was called twice or more!\n");
        return;
    }
    inited = true;
    char name[256];
    char name2[256];
    
    // make folder structure for our lock files
    sprintf(name, "/tmp/DFHack/%d",OS_getPID());
    mode_t createmode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
    mkdir("/tmp/DFHack", createmode);
    mkdir(name, createmode);
    
    // create and lock the server lock file
    sprintf(name2, "%s/SVlock",name);
    fd_svlock = open(name2,O_WRONLY | O_CREAT, createmode);
    lockf( fd_svlock, F_LOCK, 0 );
    
    // create the client lock file
    sprintf(name2, "%s/CLlock",name);
    fd_cllock = open(name2,O_WRONLY | O_CREAT, createmode);
    
    // name for the segment, an accident waiting to happen
    key_t key = SHM_KEY + OS_getPID();
    
    // find previous segment, check if it's used by some processes.
    // if it isn't, kill it with fire
    if ((shmid = shmget(key, SHM_SIZE, 0600)) != -1)
    {
        shmid_ds descriptor;
        shmctl(shmid, IPC_STAT, &descriptor);
        if(descriptor.shm_nattch == 0)
        {
            shmctl(shmid,IPC_RMID,NULL);
            fprintf(stderr,"dfhack: killed dangling resources from crashed DF.\n");
        }
    }
    // create the segment, make sure only ww are really creating it
    if ((shmid = shmget(key, SHM_SIZE, IPC_CREAT | IPC_EXCL | 0600)) < 0)
    {
        perror("shmget");
        errorstate = 1;
        return;
    }
    
    // attach the segment
    if ((shm = (char *) shmat(shmid, 0, 0)) == (char *) -1)
    {
        perror("shmat");
        errorstate = 1;
        return;
    }
    full_barrier
    // make sure we don't stall or do crazy stuff
    ((shm_cmd *)shm)->pingpong = CORE_RUNNING;
    InitModules();
}

void SHM_Destroy ( void )
{
    if(inited && !errorstate)
    {
        KillModules();
        shmid_ds descriptor;
        shmctl(shmid, IPC_STAT, &descriptor);
        shmdt(shm);
        while(descriptor.shm_nattch != 0)
        {
            shmctl(shmid, IPC_STAT, &descriptor);
        }
        shmctl(shmid,IPC_RMID,NULL);
        
        // unlock and close server lock, close client lock
        lockf(fd_svlock,F_ULOCK,0);
        close(fd_svlock);
        close(fd_cllock);
        fd_svlock = 0;
        fd_cllock = 0;

        // destroy lock files
        char name[256];
        char name2[256];
        sprintf(name, "/tmp/DFHack/%d",OS_getPID());
        sprintf(name2, "%s/SVlock",name);
        unlink(name2);
        sprintf(name2, "%s/CLlock",name);
        unlink(name2);
        rmdir(name);
        fprintf(stderr,"dfhack: destroyed shared segment.\n");
        inited = false;
    }
}

/*******************************************************************************
*                           SDL part starts here                               *
*******************************************************************************/

// ptr to the real functions
static void (*_SDL_GL_SwapBuffers)(void) = 0;
static void (*_SDL_Quit)(void) = 0;
static int (*_SDL_Init)(uint32_t flags) = 0;
static int (*_SDL_Flip)(void * some_ptr) = 0;

// hook - called every tick in OpenGL mode of DF
DFhackCExport void SDL_GL_SwapBuffers(void)
{
    if(_SDL_GL_SwapBuffers)
    {
        if(!errorstate && ((shm_cmd *)shm)->pingpong != CORE_RUNNING)
        {
            SHM_Act();
        }
        counter ++;
        _SDL_GL_SwapBuffers();
    }
}

// hook - called every tick in the 2D mode of DF
DFhackCExport int SDL_Flip(void * some_ptr)
{
    if(_SDL_Flip)
    {
        if(!errorstate && ((shm_cmd *)shm)->pingpong != CORE_RUNNING)
        {
            SHM_Act();
        }
        counter ++;
        return _SDL_Flip(some_ptr);
    }
    return 0;
}

// hook - called at program exit
DFhackCExport void SDL_Quit(void)
{
    if(!errorstate)
    {
        SHM_Destroy();
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
    _SDL_GL_SwapBuffers = (void (*)( void )) dlsym(RTLD_NEXT, "SDL_GL_SwapBuffers");
    _SDL_Init = (int (*)( uint32_t )) dlsym(RTLD_NEXT, "SDL_Init");
    _SDL_Flip = (int (*)( void * )) dlsym(RTLD_NEXT, "SDL_Flip");
    _SDL_Quit = (void (*)( void )) dlsym(RTLD_NEXT, "SDL_Quit");

    // check if we got them
    if(_SDL_GL_SwapBuffers && _SDL_Init && _SDL_Quit)
    {
        fprintf(stderr,"dfhack: hooking successful\n");
    }
    else
    {
        // bail, this would be a disaster otherwise
        fprintf(stderr,"dfhack: something went horribly wrong\n");
        exit(1);
    }
    SHM_Init();
    
    return _SDL_Init(flags);
}

/*******************************************************************************
*                           NCURSES part starts here                           *
*******************************************************************************/

static int (*_refresh)(void) = 0;
//extern NCURSES_EXPORT(int) refresh (void);
DFhackCExport int refresh (void)
{
    if(_refresh)
    {
        if(!errorstate && ((shm_cmd *)shm)->pingpong != CORE_RUNNING)
        {
            SHM_Act();
        }
        counter ++;
        return _refresh();
    }
    return 0;
}

static int (*_endwin)(void) = 0;
//extern NCURSES_EXPORT(int) endwin (void);
DFhackCExport int endwin (void)
{
    if(!errorstate)
    {
        SHM_Destroy();
    }
    if(_endwin)
    {
        return _endwin();
    }
}

typedef void WINDOW;
//extern NCURSES_EXPORT(WINDOW *) initscr (void);
static WINDOW * (*_initscr)(void) = 0;
DFhackCExport WINDOW * initscr (void)
{
    // find real functions
    _refresh = (int (*)( void )) dlsym(RTLD_NEXT, "refresh");
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
    SHM_Init();
    return _initscr();
}