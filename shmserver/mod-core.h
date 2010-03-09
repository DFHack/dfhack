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

#ifndef SHMS_CORE_H
#define SHMS_CORE_H

// increment on every core change
#define CORE_VERSION 7

typedef struct
{
    shm_cmd cmd;
    uint32_t address;
    uint32_t value;
    uint32_t length;
    uint32_t error;
} shm_core_hdr;

typedef struct
{
    uint32_t version;
    char name[256];
} modulelookup;

typedef struct
{
    uint32_t version;
    char module[256];
    char name[256];
} commandlookup;

typedef struct
{
    uint32_t sv_version; // output
    uint32_t cl_affinity; // input
    uint32_t sv_PID; // output
    uint32_t sv_useYield; // output
} coreattach;

enum CORE_COMMAND
{
    // basic states
    CORE_RUNNING = 0, // no command, normal server execution
    CORE_SUSPEND, // client notifies server to wait for commands (server is stalled in busy wait)
    CORE_SUSPENDED, // response to WAIT, server is stalled in busy wait
    CORE_ERROR, // there was a server error
        
    // utility commands
    CORE_ATTACH, // compare affinity, get core version and process ID
    CORE_ACQUIRE_MODULE, // get index of a loaded module by name and version
    CORE_ACQUIRE_COMMAND, // get module::command callsign by module name, command name and module version

    // raw reads
    CORE_DFPP_READ, // cl -> sv, read some data
    CORE_READ_DWORD, // cl -> sv, read a dword
    CORE_READ_WORD, // cl -> sv, read a word
    CORE_READ_BYTE, // cl -> sv, read a byte

    // raw writes
    CORE_WRITE,// client writes to server
    CORE_WRITE_DWORD,// client writes a DWORD to server
    CORE_WRITE_WORD,// client writes a WORD to server
    CORE_WRITE_BYTE,// client writes a BYTE to server

    // string functions
    CORE_READ_STL_STRING,// client requests contents of STL string at address
    CORE_READ_C_STRING,// client requests contents of a C string at address, max length (0 means zero terminated)
    CORE_WRITE_STL_STRING,// client wants to set STL string at address to something

    // total commands
    NUM_CORE_CMDS
};
#endif