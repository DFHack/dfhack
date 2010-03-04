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
#define CORE_VERSION 6

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

enum CORE_COMMAND
{
    CORE_RUNNING = 0, // no command, normal server execution
    
    CORE_GET_VERSION, // protocol version query
    CORE_RET_VERSION, // return the protocol version
    
    CORE_GET_PID, // query for the process ID
    CORE_RET_PID, // return process ID
    
    // version 1 stuff below
    CORE_DFPP_READ, // cl -> sv, read some data
    CORE_RET_DATA, // sv -> cl, returned data
    
    CORE_READ_DWORD, // cl -> sv, read a dword
    CORE_RET_DWORD, // sv -> cl, returned dword

    CORE_READ_WORD, // cl -> sv, read a word
    CORE_RET_WORD, // sv -> cl, returned word

    CORE_READ_BYTE, // cl -> sv, read a byte
    CORE_RET_BYTE, // sv -> cl, returned byte
    
    CORE_SV_ERROR, // there was a server error
    CORE_CL_ERROR, // there was a client error
    
    CORE_WRITE,// client writes to server
    CORE_WRITE_DWORD,// client writes a DWORD to server
    CORE_WRITE_WORD,// client writes a WORD to server
    CORE_WRITE_BYTE,// client writes a BYTE to server
    
    CORE_SUSPEND, // client notifies server to wait for commands (server is stalled in busy wait)
    CORE_SUSPENDED, // response to WAIT, server is stalled in busy wait
    
    // all strings capped at 1MB
    CORE_READ_STL_STRING,// client requests contents of STL string at address
    CORE_READ_C_STRING,// client requests contents of a C string at address, max length (0 means zero terminated)
    CORE_RET_STRING, // sv -> cl length + string contents
    CORE_WRITE_STL_STRING,// client wants to set STL string at address to something
    
    // compare affinity and determine if using yield is required
    CORE_SYNC_YIELD,// cl sends affinity to sv, sv sets yield
    CORE_SYNC_YIELD_RET,// sv returns yield bool
    
    // get index of a loaded module by name and version
    CORE_ACQUIRE_MODULE, 
    // returning module index
    CORE_RET_MODULE,
    NUM_CORE_CMDS
};
#endif