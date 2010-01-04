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
 * This is the source for the DF <-> dfhack shm bridge, server protocol part
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "shms.h"

// various crud
extern int errorstate;
extern char *shm;
extern int shmid;

void SHM_Act (void)
{
    if(errorstate)
    {
        return;
    }
    uint32_t numwaits = 0;
    uint32_t length;
    uint32_t address;
    check_again: // goto target!!!
    if(numwaits == 10000)
    {
        // this tests if there's a process on the other side
        if(isValidSHM())
        {
            numwaits = 0;
        }
        else
        {
            full_barrier
            ((shm_cmd *)shm)->pingpong = DFPP_RUNNING;
            fprintf(stderr,"dfhack: Broke out of loop, other process disappeared.\n");
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
            full_barrier
            ((shm_cmd *)shm)->pingpong = DFPP_SUSPENDED;
            goto check_again;
            /*
        case DFPP_BOUNCE:
            length = ((shm_bounce *)shm)->length;
            memcpy(BigFat,shm + SHM_HEADER,length);
            memcpy(shm + SHM_HEADER,BigFat,length);
            ((shm_cmd *)shm)->pingpong = DFPP_RET_DATA;
            goto check_again;
            */
        case DFPP_PID:
            ((shm_retval *)shm)->value = getPID();
            full_barrier
            ((shm_retval *)shm)->pingpong = DFPP_RET_PID;
            goto check_again;
            
        case DFPP_VERSION:
            ((shm_retval *)shm)->value = PINGPONG_VERSION;
            full_barrier
            ((shm_retval *)shm)->pingpong = DFPP_RET_VERSION;
            goto check_again;
            
        case DFPP_READ:
            length = ((shm_read *)shm)->length;
            address = ((shm_read *)shm)->address;
            memcpy(shm + SHM_HEADER, (void *) address,length);
            full_barrier
            ((shm_cmd *)shm)->pingpong = DFPP_RET_DATA;
            goto check_again;
            
        case DFPP_READ_DWORD:
            address = ((shm_read_small *)shm)->address;
            ((shm_retval *)shm)->value = *((uint32_t*) address);
            full_barrier
            ((shm_retval *)shm)->pingpong = DFPP_RET_DWORD;
            goto check_again;

        case DFPP_READ_WORD:
            address = ((shm_read_small *)shm)->address;
            ((shm_retval *)shm)->value = *((uint16_t*) address);
            full_barrier
            ((shm_retval *)shm)->pingpong = DFPP_RET_WORD;
            goto check_again;
            
        case DFPP_READ_BYTE:
            address = ((shm_read_small *)shm)->address;
            ((shm_retval *)shm)->value = *((uint8_t*) address);
            full_barrier
            ((shm_retval *)shm)->pingpong = DFPP_RET_BYTE;
            goto check_again;
            
        case DFPP_WRITE:
            address = ((shm_write *)shm)->address;
            length = ((shm_write *)shm)->length;
            memcpy((void *)address, shm + SHM_HEADER,length);
            full_barrier
            ((shm_cmd *)shm)->pingpong = DFPP_SUSPENDED;
            goto check_again;
            
        case DFPP_WRITE_DWORD:
            (*(uint32_t*)((shm_write_small *)shm)->address) = ((shm_write_small *)shm)->value;
            full_barrier
            ((shm_cmd *)shm)->pingpong = DFPP_SUSPENDED;
            goto check_again;

        case DFPP_WRITE_WORD:
            (*(uint16_t*)((shm_write_small *)shm)->address) = ((shm_write_small *)shm)->value;
            full_barrier
            ((shm_cmd *)shm)->pingpong = DFPP_SUSPENDED;
            goto check_again;
        
        case DFPP_WRITE_BYTE:
            (*(uint8_t*)((shm_write_small *)shm)->address) = ((shm_write_small *)shm)->value;
            full_barrier
            ((shm_cmd *)shm)->pingpong = DFPP_SUSPENDED;
            goto check_again;
            
        case DFPP_CL_ERROR:
        case DFPP_RUNNING:
            fprintf(stderr, "no. of waits: %d\n", numwaits);
            break;
            
        default:
            ((shm_retval *)shm)->value = DFEE_INVALID_COMMAND;
            full_barrier
            ((shm_retval *)shm)->pingpong = DFPP_SV_ERROR;
            break;
    }
}
