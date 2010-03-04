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
 * This is the source for the DF <-> dfhack shm bridge's core module.
 */

#include <stdio.h>
#include "../library/integers.h"
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include "shms.h"
#include "shms-core.h"

std::vector <DFPP_module> module_registry;

// various crud
extern int errorstate;
extern char *shm;
extern int shmid;
bool useYield = 0;

#define SHMHDR ((shm_core_hdr *)shm)
#define SHMCMD ((shm_cmd *)shm)->pingpong

void GetCoreVersion (void)
{
    SHMHDR->value = module_registry[0].version;
}

void GetPID (void)
{
    SHMHDR->value = OS_getPID();
}

void ReadRaw (void)
{
    memcpy(shm + SHM_HEADER, (void *) SHMHDR->address,SHMHDR->length);
}

void ReadDWord (void)
{
    SHMHDR->value = *((uint32_t*) SHMHDR->address);
}

void ReadWord (void)
{
    SHMHDR->value = *((uint16_t*) SHMHDR->address);
}

void ReadByte (void)
{
    SHMHDR->value = *((uint8_t*) SHMHDR->address);
}

void WriteRaw (void)
{
    memcpy((void *)SHMHDR->address, shm + SHM_HEADER,SHMHDR->length);
}

void WriteDWord (void)
{
    (*(uint32_t*)SHMHDR->address) = SHMHDR->value;
}

void WriteWord (void)
{
    (*(uint16_t*)SHMHDR->address) = SHMHDR->value;
}

void WriteByte (void)
{
    (*(uint8_t*)SHMHDR->address) = SHMHDR->value;
}

void ReadSTLString (void)
{
    std::string * myStringPtr = (std::string *) SHMHDR->address;
    unsigned int l = myStringPtr->length();
    SHMHDR->value = l;
    // FIXME: there doesn't have to be a null terminator!
    strncpy(shm+SHM_HEADER,myStringPtr->c_str(),l+1);
}

void WriteSTLString (void)
{
    std::string * myStringPtr = (std::string *) SHMHDR->address;
    // here we DO expect a 0 terminator
    myStringPtr->assign((const char *) (shm + SHM_HEADER));
}

// MIT HAKMEM bitcount
int bitcount(uint32_t n)
{
    register uint32_t tmp;
    
    tmp = n - ((n >> 1) & 033333333333) - ((n >> 2) & 011111111111);
    return ((tmp + (tmp >> 3)) & 030707070707) % 63;
}

// get local and remote affinity, set up yield if required (single core available)
void SyncYield (void)
{
    uint32_t local = OS_getAffinity();
    uint32_t remote = SHMHDR->value;
    uint32_t pool = local | remote;
    if(bitcount(pool) == 1)
    {
        SHMHDR->value = useYield = 1;
    }
    else
    {
        SHMHDR->value = useYield = 0;
    }
}

void InitCore(void)
{
    DFPP_module core;
    core.name = "Core";
    core.version = CORE_VERSION;
    core.modulestate = 0; // this one is dumb and has no real state
    
    core.reserve(NUM_CORE_CMDS);
    core.set_command(CORE_RUNNING, CANCELLATION, "Running");
    
    core.set_command(CORE_GET_VERSION, FUNCTION,"Get core version",GetCoreVersion, CORE_RET_VERSION);
    core.set_command(CORE_RET_VERSION, CLIENT_WAIT,"Core version return");
    
    core.set_command(CORE_GET_PID, FUNCTION, "Get PID", GetPID, CORE_RET_PID);
    core.set_command(CORE_RET_PID, CLIENT_WAIT, "PID return");
    
    core.set_command(CORE_DFPP_READ, FUNCTION,"Raw read",ReadRaw, CORE_RET_DATA);
    core.set_command(CORE_RET_DATA, CLIENT_WAIT,"Raw read return");

    core.set_command(CORE_READ_DWORD, FUNCTION,"Read DWORD",ReadDWord, CORE_RET_DWORD);
    core.set_command(CORE_RET_DWORD, CLIENT_WAIT,"Read DWORD return");

    core.set_command(CORE_READ_WORD, FUNCTION,"Read WORD",ReadWord, CORE_RET_WORD);
    core.set_command(CORE_RET_WORD, CLIENT_WAIT,"Read WORD return");
    
    core.set_command(CORE_READ_BYTE, FUNCTION,"Read BYTE",ReadByte, CORE_RET_BYTE);
    core.set_command(CORE_RET_BYTE, CLIENT_WAIT,"Read BYTE return");
    
    core.set_command(CORE_SV_ERROR, CANCELLATION, "Server error");
    core.set_command(CORE_CL_ERROR, CANCELLATION, "Client error");
    
    core.set_command(CORE_WRITE, FUNCTION, "Raw write", WriteRaw, CORE_SUSPENDED);
    core.set_command(CORE_WRITE_DWORD, FUNCTION, "Write DWORD", WriteDWord, CORE_SUSPENDED);
    core.set_command(CORE_WRITE_WORD, FUNCTION, "Write WORD", WriteWord, CORE_SUSPENDED);
    core.set_command(CORE_WRITE_BYTE, FUNCTION, "Write BYTE", WriteByte, CORE_SUSPENDED);
    
    core.set_command(CORE_SUSPEND, CLIENT_WAIT, "Suspend", 0 , CORE_SUSPENDED);
    core.set_command(CORE_SUSPENDED, CLIENT_WAIT, "Suspended");
    
    core.set_command(CORE_READ_STL_STRING, FUNCTION, "Read STL string", ReadSTLString, CORE_RET_STRING);
    core.set_command(CORE_READ_C_STRING, CLIENT_WAIT, "RESERVED");
    core.set_command(CORE_RET_STRING, CLIENT_WAIT, "Return string");
    core.set_command(CORE_WRITE_STL_STRING, FUNCTION, "Write STL string", WriteSTLString, CORE_SUSPENDED);
    core.set_command(CORE_SYNC_YIELD, FUNCTION, "Synchronize affinity/yield", SyncYield, CORE_SYNC_YIELD_RET);
    module_registry.push_back(core);
}

void InitModules (void)
{
    // create the core module
    InitCore();
    // TODO: dynamic module init
}

void SHM_Act (void)
{
    if(errorstate)
    {
        return;
    }
    uint32_t numwaits = 0;
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
            // full_barrier
            SHMCMD = CORE_RUNNING;
            fprintf(stderr,"dfhack: Broke out of loop, other process disappeared.\n");
        }
    }
    DFPP_module & mod = module_registry[((shm_cmd *)shm)->parts.module];
    DFPP_command & cmd = mod.commands[((shm_cmd *)shm)->parts.command];
    //fprintf(stderr, "Client invoked  %s\n", cmd.name.c_str() );
    if(cmd._function)
    {
        cmd._function();
    }
    if(cmd.nextState != -1)
    {
        SHMCMD = cmd.nextState;
    }
    if(cmd.type != CANCELLATION)
    {
        if(useYield)
        {
            SCHED_YIELD
        }
        numwaits ++; // watchdog timeout
        goto check_again;
    }
}

