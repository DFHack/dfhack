#ifndef DFCONNECT_H
#define DFCONNECT_H

#define PINGPONG_VERSION 2
#define SHM_KEY 123466
#define SHM_HEADER 1024
#define SHM_BODY 1024*1024
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


/*
    * read - parameters are address and length
    * write - parameters are address, length and the actual data to write
    * wait - sent to DF so that it waits for more commands
    * end - sent to DF for breaking out of the wait 
*/

enum DF_SHM_ERRORSTATE
{
    SHM_OK, // all OK
    SHM_CANT_GET_SHM, // getting the SHM ID failed for some reason
    SHM_CANT_ATTACH, // we can't attach the shm for some reason
    SHM_SECOND_DF // we are a second DF process, can't use SHM at all
};

enum DF_PINGPONG
{
    DFPP_RUNNING = 0, // no command, normal server execution
    
    DFPP_VERSION, // protocol version query
    DFPP_RET_VERSION, // return the protocol version
    
    DFPP_PID, // query for the process ID
    DFPP_RET_PID, // return process ID
    
    // version 1 stuff below
    DFPP_READ, // cl -> sv, read some data
    DFPP_RET_DATA, // sv -> cl, returned data
    
    DFPP_READ_DWORD, // cl -> sv, read a dword
    DFPP_RET_DWORD, // sv -> cl, returned dword

    DFPP_READ_WORD, // cl -> sv, read a word
    DFPP_RET_WORD, // sv -> cl, returned word

    DFPP_READ_BYTE, // cl -> sv, read a byte
    DFPP_RET_BYTE, // sv -> cl, returned byte
    
    DFPP_SV_ERROR, // there was a server error
    DFPP_CL_ERROR, // there was a client error
    
    DFPP_WRITE,// client writes to server
    DFPP_WRITE_DWORD,// client writes a DWORD to server
    DFPP_WRITE_WORD,// client writes a WORD to server
    DFPP_WRITE_BYTE,// client writes a BYTE to server
    
    DFPP_SUSPEND, // client notifies server to wait for commands (server is stalled in busy wait)
    DFPP_SUSPENDED, // response to WAIT, server is stalled in busy wait
    
    // all strings capped at 1MB
    DFPP_READ_STL_STRING,// client requests contents of STL string at address
    DFPP_READ_C_STRING,// client requests contents of a C string at address, max length (0 means zero terminated)
    DFPP_RET_STRING, // sv -> cl length + string contents
    DFPP_WRITE_STL_STRING,// client wants to set STL string at address to something
    
    // vector elements > 1MB are not supported because they don't fit into the shared memory
    DFPP_READ_ENTIRE_VECTOR, // read an entire vector (parameters are address of vector object and size of items)
    DFPP_RET_VECTOR_BODY, // a part of a vector is returned - no. of elements returned, no. of elements total, elements
    
    NUM_DFPP
};


enum DF_ERROR
{
    DFEE_INVALID_COMMAND,
    DFEE_BUFFER_OVERFLOW
};

typedef struct
{
    volatile uint32_t pingpong; // = 0
} shm_cmd;

typedef struct
{
    volatile uint32_t pingpong;
    uint32_t address;
    uint32_t length;
} shm_read;

typedef shm_read shm_write;
typedef shm_read shm_bounce;

typedef struct
{
    volatile uint32_t pingpong;
} shm_ret_data;

typedef struct
{
    volatile uint32_t pingpong;
    uint32_t address;
} shm_read_small;

typedef struct
{
    volatile uint32_t pingpong;
    uint32_t address;
    uint32_t value;
} shm_write_small;

typedef struct
{
    volatile uint32_t pingpong;
    uint32_t value;
} shm_retval;

typedef struct
{
    volatile uint32_t pingpong;
    uint32_t length;
} shm_retstr;


void SHM_Act (void);
bool isValidSHM();
uint32_t getPID();

#endif
