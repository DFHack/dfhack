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
#define _GNU_SOURCE

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

// ptr to the real functions
static void (*_SDL_GL_SwapBuffers)(void) = 0;
static void (*_SDL_Quit)(void) = 0;
static int (*_SDL_Init)(uint32_t flags) = 0;
// various crud
int counter = 0;
int errorstate = 0;
char *shm;
int shmid;

static unsigned char * BigFat;

void SHM_Init ( void )
{
    // name for the segment
    key_t key = 123466;
    BigFat = (unsigned char *) malloc(SHM_SIZE);
    
    // create the segment
    if ((shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0600)) < 0)
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
    ((shm_cmd *)shm)->pingpong = DFPP_RUNNING; // make sure we don't stall or do crazy stuff
}

void SHM_Destroy ( void )
{
    // blah, I don't care
}

void SHM_Act (void)
{
    struct shmid_ds descriptor;
    uint32_t numwaits = 0;
    if(errorstate)
    {
        return;
    }
    uint32_t length;
    uint32_t address;
    check_again: // goto target!!!
    if(numwaits == 10000)
    {
        shmctl(shmid, IPC_STAT, &descriptor); 
        if(descriptor.shm_nattch == 1)// other guy crashed
        {
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
            ((shm_cmd *)shm)->pingpong = DFPP_SUSPENDED;
            goto check_again;
            
        case DFPP_BOUNCE:
            length = ((shm_bounce *)shm)->length;
            memcpy(BigFat,shm + sizeof(shm_bounce),length);
            memcpy(shm + sizeof(shm_ret_data),BigFat,length);
            ((shm_cmd *)shm)->pingpong = DFPP_RET_DATA;
            goto check_again;
            
        case DFPP_PID:
            ((shm_retval *)shm)->value = getpid();
            ((shm_retval *)shm)->pingpong = DFPP_RET_PID;
            goto check_again;
            
        case DFPP_VERSION:
            ((shm_retval *)shm)->value = PINGPONG_VERSION;
            ((shm_retval *)shm)->pingpong = DFPP_RET_VERSION;
            goto check_again;
            
        case DFPP_READ:
            length = ((shm_read *)shm)->length;
            address = ((shm_read *)shm)->address;
            memcpy(shm + sizeof(shm_ret_data), (void *) address,length);
            ((shm_cmd *)shm)->pingpong = DFPP_RET_DATA;
            goto check_again;
            
        case DFPP_READ_DWORD:
            address = ((shm_read_small *)shm)->address;
            ((shm_retval *)shm)->value = *((uint32_t*) address);
            ((shm_retval *)shm)->pingpong = DFPP_RET_DWORD;
            goto check_again;

        case DFPP_READ_WORD:
            address = ((shm_read_small *)shm)->address;
            ((shm_retval *)shm)->value = *((uint16_t*) address);
            ((shm_retval *)shm)->pingpong = DFPP_RET_WORD;
            goto check_again;
            
        case DFPP_READ_BYTE:
            address = ((shm_read_small *)shm)->address;
            ((shm_retval *)shm)->value = *((uint8_t*) address);
            ((shm_retval *)shm)->pingpong = DFPP_RET_BYTE;
            goto check_again;
            
        case DFPP_WRITE:
            address = ((shm_write *)shm)->address;
            length = ((shm_write *)shm)->length;
            memcpy((void *)address, shm + sizeof(shm_write),length);
            ((shm_cmd *)shm)->pingpong = DFPP_SUSPENDED;
            goto check_again;
            
        case DFPP_WRITE_DWORD:
            (*(uint32_t*)((shm_write_small *)shm)->address) = ((shm_write_small *)shm)->value;
            ((shm_cmd *)shm)->pingpong = DFPP_SUSPENDED;
            goto check_again;

        case DFPP_WRITE_WORD:
            (*(uint16_t*)((shm_write_small *)shm)->address) = ((shm_write_small *)shm)->value;
            ((shm_cmd *)shm)->pingpong = DFPP_SUSPENDED;
            goto check_again;
        
        case DFPP_WRITE_BYTE:
            (*(uint8_t*)((shm_write_small *)shm)->address) = ((shm_write_small *)shm)->value;
            ((shm_cmd *)shm)->pingpong = DFPP_SUSPENDED;
            goto check_again;
            
        case DFPP_CL_ERROR:
        case DFPP_RUNNING:
            fprintf(stderr, "no. of waits: %d\n", numwaits);
            break;
            
        default:
            ((shm_retval *)shm)->value = DFEE_INVALID_COMMAND;
            ((shm_retval *)shm)->pingpong = DFPP_SV_ERROR;
            break;
    }
}

// hook - called every tick
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

// hook - called at program exit
extern "C" void SDL_Quit(void)
{
    if(_SDL_Quit)
    {
        _SDL_Quit();
    }
    fprintf(stderr,"dfhack: DF called SwapBuffers %d times, lol\n", counter);
}

// hook - called at program start, initialize some stuffs we'll use later
extern "C" int SDL_Init(uint32_t flags)
{
    // find real functions
    _SDL_GL_SwapBuffers = (void (*)( void )) dlsym(RTLD_NEXT, "SDL_GL_SwapBuffers");
    _SDL_Init = (int (*)( uint32_t )) dlsym(RTLD_NEXT, "SDL_Init");
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

