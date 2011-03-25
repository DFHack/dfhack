/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf

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

#ifndef SHM_PROCESS_H_INCLUDED
#define SHM_PROCESS_H_INCLUDED

#include "dfhack/DFProcess.h"
#include "dfhack/DFIntegers.h"
#include "dfhack/VersionInfoFactory.h"

namespace DFHack
{
    class DFHACK_EXPORT SHMProcess : public Process
    {
    private:
        class Private;
        Private * const d;

    public:
        SHMProcess(uint32_t PID, VersionInfoFactory * factory);
        ~SHMProcess();
        // Set up stuff so we can read memory
        bool attach();
        bool detach();

        bool suspend();
        bool asyncSuspend();
        bool resume();
        bool forceresume();

        void readQuad(const uint32_t address, uint64_t & value);
        void writeQuad(const uint32_t address, const uint64_t value);

        void readDWord(const uint32_t address, uint32_t & value);
        void writeDWord(const uint32_t address, const uint32_t value);

        void readFloat(const uint32_t address, float & value);

        void readWord(const uint32_t address, uint16_t & value);
        void writeWord(const uint32_t address, const uint16_t value);

        void readByte(const uint32_t address, uint8_t & value);
        void writeByte(const uint32_t address, const uint8_t value);

        void read( uint32_t address, uint32_t length, uint8_t* buffer);
        void write(uint32_t address, uint32_t length, uint8_t* buffer);

        const std::string readSTLString (uint32_t offset);
        size_t readSTLString (uint32_t offset, char * buffer, size_t bufcapacity);
        void writeSTLString(const uint32_t address, const std::string writeString);

        void readSTLVector(const uint32_t address, t_vecTriplet & triplet);
        // get class name of an object with rtti/type info
        std::string readClassName(uint32_t vptr);

        const std::string readCString (uint32_t offset);

        bool isSuspended();
        bool isAttached();
        bool isIdentified();

        bool getThreadIDs(std::vector<uint32_t> & threads );
        void getMemRanges(std::vector<t_memrange> & ranges );
        VersionInfo *getDescriptor();
        int getPID();
        std::string getPath();
        // get module index by name and version. bool 1 = error
        bool getModuleIndex (const char * name, const uint32_t version, uint32_t & OUTPUT);
        // get the SHM start if available
        char * getSHMStart (void);
        bool SetAndWait (uint32_t state);
    private:
        bool acquireSuspendLock();
        bool releaseSuspendLock();
    };

    class SHMProcess::Private
    {
    public:
        Private(SHMProcess * self_);
        ~Private(){}
        VersionInfo * memdescriptor;
        SHMProcess * self;
        char *shm_addr;
        int attachmentIdx;

        bool attached;
        bool locked;
        bool identified;
        bool useYield;
        
        uint8_t vector_start;

#ifdef LINUX_BUILD
        pid_t process_ID;
        int shm_ID;
        int server_lock;
        int client_lock;
        int suspend_lock;
#else
        typedef uint32_t pid_t;
        uint32_t process_ID;
        HANDLE DFSVMutex;
        HANDLE DFCLMutex;
        HANDLE DFCLSuspendMutex;
#endif

        bool validate(VersionInfoFactory * factory);

        bool Aux_Core_Attach(bool & versionOK, pid_t& PID);
        bool SetAndWait (uint32_t state);
        bool GetLocks();
        bool AreLocksOk();
        void FreeLocks();
    };
}

// some helpful macros to keep the code bloat in check
#define SHMCMD ( (uint32_t *) shm_addr)[attachmentIdx]
#define D_SHMCMD ( (uint32_t *) (d->shm_addr))[d->attachmentIdx]

#define SHMHDR ((shm_core_hdr *)shm_addr)
#define D_SHMHDR ((shm_core_hdr *)(d->shm_addr))

#define SHMDATA(type) ((type *)(shm_addr + SHM_HEADER))
#define D_SHMDATA(type) ((type *)(d->shm_addr + SHM_HEADER))

#endif
