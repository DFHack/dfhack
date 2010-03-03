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

enum DFPP_CmdType
{
    CANCELLATION, // we should jump out of the Act()
    CLIENT_WAIT, // we are waiting for the client
    FUNCTION, // we call a function as a result of the command
};

struct DFPP_command
{
    DFPP_CmdType type:32; // force the enum to 32 bits for compatibility reasons
    std::string name;
    void (*_function)(void);
};

struct DFPP_module
{
    inline void push_command(DFPP_CmdType type, const char * name, void (*_function)(void))
    {
        DFPP_command cmd;
        cmd.type = type;
        cmd.name = name;
        cmd._function = _function;
        commands.push_back(cmd);
    }
    inline void set_command(unsigned int index, DFPP_CmdType type, const char * name, void (*_function)(void))
    {
        DFPP_command cmd;
        cmd.type = type;
        cmd.name = name;
        cmd._function = _function;
        commands[index] = cmd;
    }
    inline void reserve (unsigned int numcommands)
    {
        commands.clear();
        DFPP_command cmd = {CANCELLATION,"",0};
        commands.resize(numcommands,cmd);
    }
    std::string name;
    uint32_t version; // version
    std::vector <DFPP_command> commands;
    void * modulestate;
};

std::vector <DFPP_module> module_registry;

// various crud
extern int errorstate;
extern char *shm;
extern int shmid;

#define SHMHDR ((shm_header *)shm)
#define SHMCMD ((shm_cmd *)shm)->pingpong

void GetCoreVersion (void)
{
    SHMHDR->value = module_registry[0].version;
    full_barrier
    SHMCMD = CORE_RET_VERSION;
}

void GetPID (void)
{
    SHMHDR->value = OS_getPID();
    full_barrier
    SHMCMD = CORE_RET_PID;
}

void ReadRaw (void)
{
    memcpy(shm + SHM_HEADER, (void *) SHMHDR->address,SHMHDR->length);
    full_barrier
    SHMCMD = CORE_RET_DATA;
}

void ReadDWord (void)
{
    SHMHDR->value = *((uint32_t*) SHMHDR->address);
    full_barrier
    SHMCMD = CORE_RET_DWORD;
}

void ReadWord (void)
{
    SHMHDR->value = *((uint16_t*) SHMHDR->address);
    full_barrier
    SHMCMD = CORE_RET_WORD;
}

void ReadByte (void)
{
    SHMHDR->value = *((uint8_t*) SHMHDR->address);
    full_barrier
    SHMCMD = CORE_RET_BYTE;
}

void WriteRaw (void)
{
    memcpy((void *)SHMHDR->address, shm + SHM_HEADER,SHMHDR->length);
    full_barrier
    SHMCMD = CORE_SUSPENDED;
}

void WriteDWord (void)
{
    (*(uint32_t*)SHMHDR->address) = SHMHDR->value;
    full_barrier
    SHMCMD = CORE_SUSPENDED;
}

void WriteWord (void)
{
    (*(uint16_t*)SHMHDR->address) = SHMHDR->value;
    full_barrier
    SHMCMD = CORE_SUSPENDED;
}

void WriteByte (void)
{
    (*(uint8_t*)SHMHDR->address) = SHMHDR->value;
    full_barrier
    SHMCMD = CORE_SUSPENDED;
}

void ReadSTLString (void)
{
    std::string * myStringPtr = (std::string *) SHMHDR->address;
    unsigned int l = myStringPtr->length();
    SHMHDR->value = l;
    // there doesn't have to be a null terminator!
    strncpy(shm+SHM_HEADER,myStringPtr->c_str(),l+1);
    full_barrier
    SHMCMD = CORE_RET_STRING;
}

void WriteSTLString (void)
{
    std::string * myStringPtr = (std::string *) SHMHDR->address;
    // here we DO expect a 0 terminator
    myStringPtr->assign((const char *) (shm + SHM_HEADER));
    full_barrier
    SHMCMD = CORE_SUSPENDED;    
}

void Suspend (void)
{
    SHMCMD = CORE_SUSPENDED;    
}

void InitCore(void)
{
    DFPP_module core;
    core.name = "Core";
    core.version = CORE_VERSION;
    core.modulestate = 0; // this one is dumb and has no real state
    
    core.reserve(NUM_CORE_CMDS);
    core.set_command(CORE_RUNNING, CANCELLATION, "Running", NULL);
    
    core.set_command(CORE_GET_VERSION, FUNCTION,"Get core version",GetCoreVersion);
    core.set_command(CORE_RET_VERSION, CLIENT_WAIT,"Core version return",0);
    
    core.set_command(CORE_GET_PID, FUNCTION, "Get PID", GetPID);
    core.set_command(CORE_RET_PID, CLIENT_WAIT, "PID return", 0);
    
    core.set_command(CORE_DFPP_READ, FUNCTION,"Raw read",ReadRaw);
    core.set_command(CORE_RET_DATA, CLIENT_WAIT,"Raw read return",0);

    core.set_command(CORE_READ_DWORD, FUNCTION,"Read DWORD",ReadDWord);
    core.set_command(CORE_RET_DWORD, CLIENT_WAIT,"Read DWORD return",0);

    core.set_command(CORE_READ_WORD, FUNCTION,"Read WORD",ReadWord);
    core.set_command(CORE_RET_WORD, CLIENT_WAIT,"Read WORD return",0);
    
    core.set_command(CORE_READ_BYTE, FUNCTION,"Read BYTE",ReadByte);
    core.set_command(CORE_RET_BYTE, CLIENT_WAIT,"Read BYTE return",0);
    
    core.set_command(CORE_SV_ERROR, CANCELLATION, "Server error", 0);
    core.set_command(CORE_CL_ERROR, CANCELLATION, "Client error", 0);
    
    core.set_command(CORE_WRITE, FUNCTION, "Raw write", WriteRaw);
    core.set_command(CORE_WRITE_DWORD, FUNCTION, "Write DWORD", WriteDWord);
    core.set_command(CORE_WRITE_WORD, FUNCTION, "Write WORD", WriteWord);
    core.set_command(CORE_WRITE_BYTE, FUNCTION, "Write BYTE", WriteByte);
    
    core.set_command(CORE_SUSPEND, FUNCTION, "Suspend", Suspend);
    core.set_command(CORE_SUSPENDED, CLIENT_WAIT, "Suspended", 0);
    
    core.set_command(CORE_READ_STL_STRING, FUNCTION, "Read STL string", ReadSTLString);
    core.set_command(CORE_READ_C_STRING, CLIENT_WAIT, "RESERVED", 0);
    core.set_command(CORE_RET_STRING, CLIENT_WAIT, "Return string", 0);
    core.set_command(CORE_WRITE_STL_STRING, FUNCTION, "Write STL string", WriteSTLString);
    module_registry.push_back(core);
}

void InitModules (void)
{
    // create the core module
    InitCore();
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
            full_barrier
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
    if(cmd.type != CANCELLATION)
    {
        SCHED_YIELD
        numwaits ++; // watchdog timeout
        goto check_again;
    }
}

