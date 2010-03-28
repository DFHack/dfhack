#ifndef DFCONNECT_H
#define DFCONNECT_H

#define SHM_KEY 123466
#define SHM_MAX_CLIENTS 4
#define SHM_HEADER 1024 // 1kB reserved for a header
#define SHM_BODY 1024*1024 // 4MB reserved for bulk data transfer
#define SHM_SIZE SHM_HEADER+SHM_BODY
//#define SHM_ALL_CLIENTS SHM_MAX_CLIENTS*(SHM_SIZE)
//#define SHM_CL(client_idx) client_idx*(SHM_SIZE)

// FIXME: add YIELD for linux, add single-core and multi-core compile targets for optimal speed
#ifdef LINUX_BUILD
    // a full memory barrier! better be safe than sorry.
    #define full_barrier asm volatile("" ::: "memory"); __sync_synchronize();
    #define SCHED_YIELD sched_yield(); // a requirement for single-core
#else
    // we need windows.h for Sleep()
    #define _WIN32_WINNT 0x0501 // needed for INPUT struct
    #define WINVER 0x0501                   // OpenThread(), PSAPI, Toolhelp32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #define SCHED_YIELD Sleep(0); // avoids infinite lockup on single core
    // FIXME: detect MSVC here and use the right barrier magic
    #ifdef __MINGW32__
        #define full_barrier asm volatile("" ::: "memory");
    #else
        #include <intrin.h>
        #pragma intrinsic(_ReadWriteBarrier)
        #define full_barrier _ReadWriteBarrier();
    #endif
#endif

enum DFPP_Locking
{
    LOCKING_BUSY = 0,
    LOCKING_LOCKS = 1
};

enum DFPP_CmdType
{
    CANCELLATION, // we should jump out of the Act()
    CLIENT_WAIT, // we are waiting for the client
    FUNCTION, // we call a function as a result of the command
};

struct DFPP_command
{
    void (*_function)(void *);
    DFPP_CmdType type:32; // force the enum to 32 bits for compatibility reasons
    std::string name;
    uint32_t nextState;
    DFPP_Locking locking;
};

struct DFPP_module
{
    DFPP_module()
    {
        name = "Uninitialized module";
        version = 0;
        modulestate = 0;
    }
    // ALERT: the structures share state
    DFPP_module(const DFPP_module & orig)
    {
        commands = orig.commands;
        name = orig.name;
        modulestate = orig.modulestate;
        version = orig.version;
    }
    inline void set_command(const unsigned int index, const DFPP_CmdType type, const char * name, void (*_function)(void *) = 0,uint32_t nextState = -1, DFPP_Locking locking = LOCKING_BUSY)
    {
        commands[index].type = type;
        commands[index].name = name;
        commands[index]._function = _function;
        commands[index].nextState = nextState;
        commands[index].locking = locking;
    }
    inline void reserve (unsigned int numcommands)
    {
        commands.clear();
        DFPP_command cmd = {0,CANCELLATION,"",0};
        commands.resize(numcommands,cmd);
    }
    std::string name;
    uint32_t version; // version
    std::vector <DFPP_command> commands;
    void * modulestate;
};

union shm_cmd
{
    struct
        {
            volatile uint16_t command;
            volatile uint16_t module;
        } parts;
    volatile uint32_t pingpong;
    shm_cmd(volatile uint32_t z)
    {
        pingpong = z;
    }
};

void SHM_Act (void);
void InitModules (void);
void KillModules (void);
bool isValidSHM(int current);
uint32_t OS_getPID();
uint32_t OS_getAffinity(); // limited to 32 processors. Silly, eh?
void OS_releaseSuspendLock(int currentClient);
void OS_lockSuspendLock(int currentClient);

#endif
