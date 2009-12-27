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
#include "dfconnect.h"
#include <sys/time.h>
#include <time.h>
#include <linux/futex.h>
#include <sys/syscall.h>

// a full memory barrier! better be safe than sorry.
#define gcc_barrier asm volatile("" ::: "memory"); __sync_synchronize();

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
char *shm;
int shmid;

void SHM_Init ( void )
{
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
    gcc_barrier
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

void SHM_Act (void)
{
    if(errorstate)
    {
        return;
    }
    shmid_ds descriptor;
    uint32_t numwaits = 0;
    uint32_t length;
    uint32_t address;
    check_again: // goto target!!!
    if(numwaits == 10000)
    {
        shmctl(shmid, IPC_STAT, &descriptor);
        if(descriptor.shm_nattch == 1)// other guy crashed
        {
            gcc_barrier
            ((shm_cmd *)shm)->pingpong = DFPP_RUNNING;
            fprintf(stderr,"dfhack: Broke out of loop, other process disappeared.\n");
            return;
        }
        else
        {
            numwaits = 0;
        }
    }
    switch (((shm_cmd *)shm)->pingpong)
    {
        case DFPP_RET_VERSION:
        case DFPP_RET_DATA:
        case DFPP_RET_DWORD:
        case DFPP_RET_WORD:
        case DFPP_RET_BYTE:
        case DFPP_SUSPENDED:
        case DFPP_RET_PID:
        case DFPP_SV_ERROR:
            numwaits++;
            goto check_again;
        case DFPP_SUSPEND:
            gcc_barrier
            ((shm_cmd *)shm)->pingpong = DFPP_SUSPENDED;
            goto check_again;
            /*
        case DFPP_BOUNCE:
            length = ((shm_bounce *)shm)->length;
            memcpy(BigFat,shm + sizeof(shm_bounce),length);
            memcpy(shm + sizeof(shm_ret_data),BigFat,length);
            ((shm_cmd *)shm)->pingpong = DFPP_RET_DATA;
            goto check_again;
            */
        case DFPP_PID:
            ((shm_retval *)shm)->value = getpid();
            gcc_barrier
            ((shm_retval *)shm)->pingpong = DFPP_RET_PID;
            goto check_again;
            
        case DFPP_VERSION:
            ((shm_retval *)shm)->value = PINGPONG_VERSION;
            gcc_barrier
            ((shm_retval *)shm)->pingpong = DFPP_RET_VERSION;
            goto check_again;
            
        case DFPP_READ:
            length = ((shm_read *)shm)->length;
            address = ((shm_read *)shm)->address;
            memcpy(shm + sizeof(shm_ret_data), (void *) address,length);
            gcc_barrier
            ((shm_cmd *)shm)->pingpong = DFPP_RET_DATA;
            goto check_again;
            
        case DFPP_READ_DWORD:
            address = ((shm_read_small *)shm)->address;
            ((shm_retval *)shm)->value = *((uint32_t*) address);
            gcc_barrier
            ((shm_retval *)shm)->pingpong = DFPP_RET_DWORD;
            goto check_again;

        case DFPP_READ_WORD:
            address = ((shm_read_small *)shm)->address;
            ((shm_retval *)shm)->value = *((uint16_t*) address);
            gcc_barrier
            ((shm_retval *)shm)->pingpong = DFPP_RET_WORD;
            goto check_again;
            
        case DFPP_READ_BYTE:
            address = ((shm_read_small *)shm)->address;
            ((shm_retval *)shm)->value = *((uint8_t*) address);
            gcc_barrier
            ((shm_retval *)shm)->pingpong = DFPP_RET_BYTE;
            goto check_again;
            
        case DFPP_WRITE:
            address = ((shm_write *)shm)->address;
            length = ((shm_write *)shm)->length;
            memcpy((void *)address, shm + sizeof(shm_write),length);
            gcc_barrier
            ((shm_cmd *)shm)->pingpong = DFPP_SUSPENDED;
            goto check_again;
            
        case DFPP_WRITE_DWORD:
            (*(uint32_t*)((shm_write_small *)shm)->address) = ((shm_write_small *)shm)->value;
            gcc_barrier
            ((shm_cmd *)shm)->pingpong = DFPP_SUSPENDED;
            goto check_again;

        case DFPP_WRITE_WORD:
            (*(uint16_t*)((shm_write_small *)shm)->address) = ((shm_write_small *)shm)->value;
            gcc_barrier
            ((shm_cmd *)shm)->pingpong = DFPP_SUSPENDED;
            goto check_again;
        
        case DFPP_WRITE_BYTE:
            (*(uint8_t*)((shm_write_small *)shm)->address) = ((shm_write_small *)shm)->value;
            gcc_barrier
            ((shm_cmd *)shm)->pingpong = DFPP_SUSPENDED;
            goto check_again;
            
        case DFPP_CL_ERROR:
        case DFPP_RUNNING:
            fprintf(stderr, "no. of waits: %d\n", numwaits);
            break;
            
        default:
            ((shm_retval *)shm)->value = DFEE_INVALID_COMMAND;
            gcc_barrier
            ((shm_retval *)shm)->pingpong = DFPP_SV_ERROR;
            break;
    }
}

// hook - called every tick in OpenGL mode of DF
extern "C" void SDL_GL_SwapBuffers(void)
{
    if(_SDL_GL_SwapBuffers)
    {
        if(((shm_cmd *)shm)->pingpong != DFPP_RUNNING)
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
        if(((shm_cmd *)shm)->pingpong != DFPP_RUNNING)
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
    fprintf(stderr,"dfhack: DF called SwapBuffers %d times\n", counter);
    SHM_Destroy();
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

