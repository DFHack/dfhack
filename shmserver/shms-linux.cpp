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
#include <unistd.h>
#include "shms.h"
#include <sys/time.h>
#include <time.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <signal.h>

/*
 * wait for futex
 * futex has to be aligned to 4 bytes
 * futex has to be equal to val (returns EWOULDBLOCK otherwise)
 * wait can be broken by arriving signals (returns EINTR)
 * returns 0 when broken by futex_wake
 */
inline int futex_wait(int * futex, int val)
{
    return syscall(SYS_futex, futex, FUTEX_WAIT, val, 0, 0, 0);
}
/*
 * wait for futex
 * futex has to be aligned to 4 bytes
 * futex has to be equal to val (returns EWOULDBLOCK otherwise)
 * wait can be broken by arriving signals (returns EINTR)
 * returns 0 when broken by futex_wake
 * returns ETIMEDOUT on timeout
 */
inline int futex_wait_timed(int * futex, int val, const struct timespec *timeout)
{
    return syscall(SYS_futex, futex, FUTEX_WAIT, val, timeout, 0, 0);
}
/*
 * wake up futex. returns number of waked processes
 */
inline int futex_wake(int * futex)
{
    return syscall(SYS_futex, futex, FUTEX_WAKE, 1, 0, 0, 0);
}
static timespec one_second = { 1,0 };
static timespec five_second = { 5,0 };


// ptr to the real functions
static void (*_SDL_GL_SwapBuffers)(void) = 0;
static void (*_SDL_Quit)(void) = 0;
static int (*_SDL_Init)(uint32_t flags) = 0;
static int (*_SDL_Flip)(void * some_ptr) = 0;
// various crud
int counter = 0;
int errorstate = 0;
char *shm = 0;
int shmid = 0;
bool inited = 0;

void SHM_Init ( void )
{
    // check that we do this only once per process
    if(inited)
    {
        fprintf(stderr, "SDL_Init was called twice or more!\n");
        return;
    }
    inited = true;
    
    // name for the segment
    key_t key = 123466;
    
    // find previous segment, check if it's used by some processes. if it isn't, kill it with fire
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
    ((shm_cmd *)shm)->pingpong = DFPP_RUNNING; // make sure we don't stall or do crazy stuff
}

void SHM_Destroy ( void )
{
    shmid_ds descriptor;
    shmctl(shmid, IPC_STAT, &descriptor);
    shmdt(shm);
    while(descriptor.shm_nattch != 0)
    {
        shmctl(shmid, IPC_STAT, &descriptor);
    }
    shmctl(shmid,IPC_RMID,NULL);
    fprintf(stderr,"dfhack: destroyed shared segment.\n");
}

// hook - called every tick in OpenGL mode of DF
extern "C" void SDL_GL_SwapBuffers(void)
{
    if(_SDL_GL_SwapBuffers)
    {
        if(!errorstate && ((shm_cmd *)shm)->pingpong != DFPP_RUNNING)
        {
            SHM_Act();
        }
        counter ++;
        _SDL_GL_SwapBuffers();
    }
}

// hook - called every tick in the 2D mode of DF
extern "C" int SDL_Flip(void * some_ptr)
{
    if(_SDL_Flip)
    {
        if(!errorstate && ((shm_cmd *)shm)->pingpong != DFPP_RUNNING)
        {
            SHM_Act();
        }
        counter ++;
        return _SDL_Flip(some_ptr);
    }
}

// hook - called at program exit
extern "C" void SDL_Quit(void)
{
    if(_SDL_Quit)
    {
        _SDL_Quit();
    }
    if(!errorstate)
    {
        fprintf(stderr,"dfhack: DF called SwapBuffers %d times\n", counter);
        SHM_Destroy();
    }
}

// hook - called at program start, initialize some stuffs we'll use later
extern "C" int SDL_Init(uint32_t flags)
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

// FIXME: add error checking?
bool isValidSHM()
{
    shmid_ds descriptor;
    shmctl(shmid, IPC_STAT, &descriptor);
    fprintf(stderr,"ID %d, attached: %d\n",shmid, descriptor.shm_nattch);
    return (descriptor.shm_nattch == 2);
}
uint32_t getPID()
{
    return getpid();
}