#ifndef DFCONNECT_H
#define DFCONNECT_H

#define CORE_VERSION 3
#define SHM_KEY 123466
#define SHM_HEADER 1024 // 1kB reserved for a header
#define SHM_BODY 1024*1024 // 1MB reserved for bulk data transfer
#define SHM_SIZE SHM_HEADER+SHM_BODY


// FIXME: add YIELD for linux, add single-core and multi-core compile targets for optimal speed
#ifdef LINUX_BUILD
    // a full memory barrier! better be safe than sorry.
    #define full_barrier asm volatile("" ::: "memory"); __sync_synchronize();
    #define SCHED_YIELD sched_yield(); // slow but allows the SHM to work on single-core
    // #define SCHED_YIELD usleep(0); // extremely slow
    // #define SCHED_YIELD // works only on multi-core
#else
    // we need windows.h for Sleep()
    #define _WIN32_WINNT 0x0501 // needed for INPUT struct
    #define WINVER 0x0501                   // OpenThread(), PSAPI, Toolhelp32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #define SCHED_YIELD Sleep(0); // slow on single-core, but avoids infinite lockup
    // FIXME: detect MSVC here and use the right barrier magic
    #ifdef __MINGW32__
        #define full_barrier asm volatile("" ::: "memory");
    #else
        #include <intrin.h>
        #pragma intrinsic(_ReadWriteBarrier)
        #define full_barrier _ReadWriteBarrier();
    #endif
#endif

enum DF_SHM_ERRORSTATE
{
    SHM_OK, // all OK
    SHM_CANT_GET_SHM, // getting the SHM ID failed for some reason
    SHM_CANT_ATTACH, // we can't attach the shm for some reason
    SHM_SECOND_DF // we are a second DF process, can't use SHM at all
};

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
    
    NUM_CORE_CMDS
};


enum DF_ERROR
{
    DFEE_INVALID_COMMAND,
    DFEE_BUFFER_OVERFLOW
};

typedef union
{
    struct
    {
        volatile uint16_t command;
        volatile uint16_t module;
    } parts;
    volatile uint32_t pingpong;
    inline void set(uint16_t module, uint16_t command)
    {
        pingpong = module + command << 16;
    }
} shm_cmd;

typedef struct
{
    shm_cmd cmd;
    uint32_t address;
    uint32_t value;
    uint32_t length;
} shm_header;

void SHM_Act (void);
void InitModules (void);
bool isValidSHM();
uint32_t OS_getPID();

#endif
