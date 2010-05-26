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
#include "dfhack/DFIntegers.h"
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#define SHM_INTERNAL // for things only visible to the SHM

#include "shms.h"
#include "mod-core.h"
#include "mod-maps.h"
#include "mod-creature40d.h"

std::vector <DFPP_module> module_registry;

// shared by shms_OS
extern int errorstate;
extern char *shm;
extern int shmid;

// file-globals
bool useYield = 0;
int currentClient = -1;

#define SHMHDR ((shm_core_hdr *)shm)
#define SHMCMD ((uint32_t *)shm )[currentClient]
#define SHMDATA(type) ((type *)(shm + SHM_HEADER))

void ReadRaw (void * data)
{
    memcpy(SHMDATA(void), (void *) SHMHDR->address,SHMHDR->length);
}

void ReadQuad (void * data)
{
    SHMHDR->Qvalue = *((uint64_t*) SHMHDR->address);
}

void ReadDWord (void * data)
{
    SHMHDR->value = *((uint32_t*) SHMHDR->address);
}

void ReadWord (void * data)
{
    SHMHDR->value = *((uint16_t*) SHMHDR->address);
}

void ReadByte (void * data)
{
    SHMHDR->value = *((uint8_t*) SHMHDR->address);
}

void WriteRaw (void * data)
{
    memcpy((void *)SHMHDR->address, SHMDATA(void),SHMHDR->length);
}

void WriteQuad (void * data)
{
    (*(uint64_t*)SHMHDR->address) = SHMHDR->Qvalue;
}

void WriteDWord (void * data)
{
    (*(uint32_t*)SHMHDR->address) = SHMHDR->value;
}

void WriteWord (void * data)
{
    (*(uint16_t*)SHMHDR->address) = SHMHDR->value;
}

void WriteByte (void * data)
{
    (*(uint8_t*)SHMHDR->address) = SHMHDR->value;
}

void ReadSTLString (void * data)
{
    std::string * myStringPtr = (std::string *) SHMHDR->address;
    unsigned int l = myStringPtr->length();
    SHMHDR->value = l;
    // FIXME: there doesn't have to be a null terminator!
    strncpy( SHMDATA(char),myStringPtr->c_str(),l+1);
}

void WriteSTLString (void * data)
{
    std::string * myStringPtr = (std::string *) SHMHDR->address;
    // here we DO expect a 0 terminator
    myStringPtr->assign( SHMDATA(const char) );
}

// MIT HAKMEM bitcount
int bitcount(uint32_t n)
{
    register uint32_t tmp;
    
    tmp = n - ((n >> 1) & 033333333333) - ((n >> 2) & 011111111111);
    return ((tmp + (tmp >> 3)) & 030707070707) % 63;
}

// get local and remote affinity, set up yield if required (single core available)
void CoreAttach (void * data)
{
    // sync affinity
    uint32_t local = OS_getAffinity();
    uint32_t remote = SHMDATA(coreattach)->cl_affinity;
    uint32_t pool = local | remote;
    SHMDATA(coreattach)->sv_useYield = useYield = (bitcount(pool) == 1);
    // return our PID
    SHMDATA(coreattach)->sv_PID = OS_getPID();
    // return core version
    SHMDATA(coreattach)->sv_version = module_registry[0].version;
}

void FindModule (void * data)
{
    bool found = false;
    modulelookup * payload =  SHMDATA(modulelookup);
    std::string test = payload->name;
    uint32_t version = payload->version;
    for(unsigned int i = 0; i < module_registry.size();i++)
    {
        if(module_registry[i].name == test && module_registry[i].version == version)
        {
            // gotcha
            SHMHDR->value = i;
            found = true;
            break;
        }
    }
    SHMHDR->error = !found;
}

void FindCommand (void * data)
{
    bool found = false;
    commandlookup * payload = SHMDATA(commandlookup);
    std::string modname = payload->module;
    std::string cmdname = payload->name;
    uint32_t version = payload->version;
    for(unsigned int i = 0; i < module_registry.size();i++)
    {
        if(module_registry[i].name == modname && module_registry[i].version == version)
        {
            for(unsigned int j = 0 ; j < module_registry[i].commands.size();j++)
            {
                if(module_registry[i].commands[j].name == cmdname)
                {
                    // gotcha
                    SHMHDR->value = j + (i << 16);
                    SHMHDR->error = false;
                    return;
                }
            }
        }
    }
    SHMHDR->error = true;
}

void ReleaseSuspendLock( void * data )
{
    OS_releaseSuspendLock(currentClient);
}

void AcquireSuspendLock( void * data )
{
    OS_lockSuspendLock(currentClient);
}


DFPP_module InitCore(void)
{
    DFPP_module core;
    core.name = "Core";
    core.version = CORE_VERSION;
    core.modulestate = 0; // this one is dumb and has no real state
    
    core.reserve(NUM_CORE_CMDS);
    // basic states
    core.set_command(CORE_RUNNING, CANCELLATION, "Running");
    //core.set_command(CORE_RUN, FUNCTION, "Run!",AcquireSuspendLock,CORE_RUNNING);
    core.set_command(CORE_RUN, CANCELLATION, "Run!",0,CORE_RUNNING);
    core.set_command(CORE_STEP, CANCELLATION, "Suspend on next step",0,CORE_SUSPEND);// set command to CORE_SUSPEND, check next client
    core.set_command(CORE_SUSPEND, FUNCTION, "Suspend", ReleaseSuspendLock , CORE_SUSPENDED, LOCKING_LOCKS);
    core.set_command(CORE_SUSPENDED, CLIENT_WAIT, "Suspended");
    core.set_command(CORE_ERROR, CANCELLATION, "Error");
    
    // utility commands
    core.set_command(CORE_ATTACH, FUNCTION,"Core attach",CoreAttach, CORE_SUSPENDED);
    core.set_command(CORE_ACQUIRE_MODULE, FUNCTION, "Module lookup", FindModule, CORE_SUSPENDED);
    core.set_command(CORE_ACQUIRE_COMMAND, FUNCTION, "Command lookup", FindCommand, CORE_SUSPENDED);
    
    // raw reads
    core.set_command(CORE_READ, FUNCTION,"Raw read",ReadRaw, CORE_SUSPENDED);
    core.set_command(CORE_READ_QUAD, FUNCTION,"Read QUAD",ReadQuad, CORE_SUSPENDED);
    core.set_command(CORE_READ_DWORD, FUNCTION,"Read DWORD",ReadDWord, CORE_SUSPENDED);
    core.set_command(CORE_READ_WORD, FUNCTION,"Read WORD",ReadWord, CORE_SUSPENDED);
    core.set_command(CORE_READ_BYTE, FUNCTION,"Read BYTE",ReadByte, CORE_SUSPENDED);
    
    // raw writes
    core.set_command(CORE_WRITE, FUNCTION, "Raw write", WriteRaw, CORE_SUSPENDED);
    core.set_command(CORE_WRITE_QUAD, FUNCTION, "Write QUAD", WriteQuad, CORE_SUSPENDED);
    core.set_command(CORE_WRITE_DWORD, FUNCTION, "Write DWORD", WriteDWord, CORE_SUSPENDED);
    core.set_command(CORE_WRITE_WORD, FUNCTION, "Write WORD", WriteWord, CORE_SUSPENDED);
    core.set_command(CORE_WRITE_BYTE, FUNCTION, "Write BYTE", WriteByte, CORE_SUSPENDED);
    
    // stl string commands
    core.set_command(CORE_READ_STL_STRING, FUNCTION, "Read STL string", ReadSTLString, CORE_SUSPENDED);
    core.set_command(CORE_READ_C_STRING, CLIENT_WAIT, "RESERVED");
    core.set_command(CORE_WRITE_STL_STRING, FUNCTION, "Write STL string", WriteSTLString, CORE_SUSPENDED);
    return core;
}

void InitModules (void)
{
    // create the core module
    module_registry.push_back(InitCore());
    module_registry.push_back(DFHack::Server::Maps::Init());
    //module_registry.push_back(DFHack::Server::Creatures::Init());
    for(int i = 0; i < module_registry.size();i++)
    {
        fprintf(stderr,"Initialized module %s, version %d\n",module_registry[i].name.c_str(),module_registry[i].version);
    }
    // TODO: dynamic module init
}

void KillModules (void)
{
    for(unsigned int i = 0; i < module_registry.size();i++)
    {
        if(module_registry[i].modulestate)
            free(module_registry[i].modulestate);
    }
    module_registry.clear();
}

void SHM_Act (void)
{
    volatile uint32_t atomic = 0;
    if(errorstate)
    {
        return;
    }
    //static uint oldcl = 88;
    for(currentClient = 0; currentClient < SHM_MAX_CLIENTS;currentClient++)
    {
        // set the offset for the shared memory used for the client
        uint32_t numwaits = 0;
        check_again: // goto target!!!
        if(numwaits == 10000)
        {
            // this tests if there's a process on the other side
            if(isValidSHM(currentClient))
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
        full_barrier // I don't want the compiler to reorder my code.


        //fprintf(stderr,"%d: %x %x\n",currentClient, (uint) SHMHDR, (uint) &(SHMHDR->cmd[currentClient]));
        
        // this is very important! copying two words separately from the command variable leads to inconsistency.
        // Always copy the thing in one go.
        // Also, this whole SHM thing probably only works on intel processors
        atomic  = SHMCMD;
        full_barrier
        
        DFPP_module & mod = module_registry[ ((shm_cmd)atomic).parts.module ];
        DFPP_command & cmd = mod.commands[ ((shm_cmd)atomic).parts.command ];
        /*
        if(atomic == CORE_RUNNING)
        {
            // we are running again for this process
            // reaquire the suspend lock
            OS_lockSuspendLock(currentClient);
            continue;
        }
        full_barrier
        */
        // set next state BEFORE we act on the command - good for locks
        if(cmd.locking == LOCKING_LOCKS)
        {
            if(cmd.nextState != -1) SHMCMD = cmd.nextState;
        }
        
        if(cmd._function)
        {
            cmd._function(mod.modulestate);
        }
        full_barrier
        
        // set next state AFTER we act on the command - good for busy waits
        if(cmd.locking == LOCKING_BUSY)
        {
            /*
            char text [512];
            char text2 [512];
            sprintf (text,"Client %d invoked %d:%d = %x = %s\n",currentClient,((shm_cmd)atomic).parts.module,((shm_cmd)atomic).parts.command, cmd._function,cmd.name.c_str());
            sprintf(text2, "Server set %d\n",cmd.nextState);
            */
            // FIXME: WHAT HAPPENS WHEN A 'NEXTSTATE' IS FROM A DIFFERENT MODULE THAN 'CORE'? Yeah. It doesn't work.
            if(cmd.nextState != -1) SHMCMD = cmd.nextState;
            //MessageBox(0,text,text2, MB_OK);
            
            //fflush(stderr); // make sure this finds its way to the terminal!
            
        }
        full_barrier
        
        if(cmd.type != CANCELLATION)
        {
            if(useYield)
            {
                SCHED_YIELD
            }
            numwaits ++; // watchdog timeout
            goto check_again;
        }
        full_barrier
        
        // we are running again for this process
        // reaquire the suspend lock
        OS_lockSuspendLock(currentClient);
    }
}
