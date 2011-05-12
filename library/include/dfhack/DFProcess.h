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

#pragma once

#ifndef PROCESS_H_INCLUDED
#define PROCESS_H_INCLUDED

#include "DFPragma.h"
#include "DFExport.h"
#include <iostream>
#include <map>

namespace DFHack
{
    class VersionInfo;
    class Process;
    class Window;
    class DFVector;
    
    /**
     * A type for storing an extended OS Process ID (combines PID and the time the process was started for unique identification)
     * \ingroup grp_context
     */
    struct ProcessID
    {
        ProcessID(const uint64_t _time, const uint64_t _pid): time(_time), pid(_pid){};
        bool operator==(const ProcessID &other) const
        {
            return (other.time == time && other.pid == pid);
        }
        bool operator< (const ProcessID& ms) const
        {
            if (time < ms.time)
                return true;
            else if(time == ms.time)
                return pid < ms.pid ;
            return false;
        }
        uint64_t time;
        uint64_t pid;
    };
    
    /**
     * Structure describing a section of virtual memory inside a process
     * \ingroup grp_context
     */
    struct DFHACK_EXPORT t_memrange
    {
        uint64_t start;
        uint64_t end;
        // memory range name (if any)
        char name[1024];
        // permission to read
        bool read : 1;
        // permission to write
        bool write : 1;
        // permission to execute
        bool execute : 1;
        // is a shared region
        bool shared : 1;
        inline bool isInRange( uint64_t address)
        {
            if (address >= start && address < end) return true;
            return false;
        }
        bool valid;
        uint8_t * buffer;
    };
    struct t_vecTriplet
    {
        uint32_t start;
        uint32_t end;
        uint32_t alloc_end;
    };

    /**
     * Allows low-level access to the memory of an OS process. OS processes can be enumerated by \ref ProcessEnumerator
     * \ingroup grp_context
     */
    class DFHACK_EXPORT Process
    {
        protected:
            std::map<uint32_t, std::string> classNameCache;

        public:
            /// this is the single most important destructor ever. ~px
            virtual ~Process(){};
            /// Set up stuff so we can read memory, suspends synchronously
            virtual bool attach() = 0;
            /// detach from DF, resume its execution if it's suspended
            virtual bool detach() = 0;
            /**
             * synchronous suspend
             * waits for DF to be actually suspended,
             * this might take a while depending on implementation
             */
            virtual bool suspend() = 0;
            /// asynchronous suspend to use together with polling and timers
            virtual bool asyncSuspend() = 0;
            /// resume DF execution
            virtual bool resume() = 0;
            /// force-resume DF execution
            virtual bool forceresume() = 0;

            /// read a 8-byte integer
            uint64_t readQuad(const uint32_t address) { uint64_t result; readQuad(address, result); return result; }
            /// read a 8-byte integer
            virtual void readQuad(const uint32_t address, uint64_t & value) = 0;
            /// write a 8-byte integer
            virtual void writeQuad(const uint32_t address, const uint64_t value) = 0;

            /// read a 4-byte integer
            uint32_t readDWord(const uint32_t address) { uint32_t result; readDWord(address, result); return result; }
            /// read a 4-byte integer
            virtual void readDWord(const uint32_t address, uint32_t & value) = 0;
            /// write a 4-byte integer
            virtual void writeDWord(const uint32_t address, const uint32_t value) = 0;

            /// read a float
            float readFloat(const uint32_t address) { float result; readFloat(address, result); return result; }
            /// write a float
            virtual void readFloat(const uint32_t address, float & value) = 0;

            /// read a 2-byte integer
            uint16_t readWord(const uint32_t address) { uint16_t result; readWord(address, result); return result; }
            /// read a 2-byte integer
            virtual void readWord(const uint32_t address, uint16_t & value) = 0;
            /// write a 2-byte integer
            virtual void writeWord(const uint32_t address, const uint16_t value) = 0;

            /// read a byte
            uint8_t readByte(const uint32_t address) { uint8_t result; readByte(address, result); return result; }
            /// read a byte
            virtual void readByte(const uint32_t address, uint8_t & value) = 0;
            /// write a byte
            virtual void writeByte(const uint32_t address, const uint8_t value) = 0;

            /// read an arbitrary amount of bytes
            virtual void read( uint32_t address, uint32_t length, uint8_t* buffer) = 0;
            /// write an arbitrary amount of bytes
            virtual void write(uint32_t address, uint32_t length, uint8_t* buffer) = 0;

            /// read an STL string
            virtual const std::string readSTLString (uint32_t offset) = 0;
            /// read an STL string
            virtual size_t readSTLString (uint32_t offset, char * buffer, size_t bufcapacity) = 0;
            /**
             * write an STL string
             * @return length written
             */
            virtual size_t writeSTLString(const uint32_t address, const std::string writeString) = 0;
            /**
             * attempt to copy a string from source address to target address. may truncate or leak, depending on platform
             * @return length copied
             */
            virtual size_t copySTLString(const uint32_t address, const uint32_t target)
            {
                return writeSTLString(target, readSTLString(address));
            }

            /// read a STL vector
            virtual void readSTLVector(const uint32_t address, t_vecTriplet & triplet) = 0;
            virtual void writeSTLVector(const uint32_t address, t_vecTriplet & triplet) = 0;
            /// get class name of an object with rtti/type info
            virtual std::string doReadClassName(uint32_t vptr) = 0;

            std::string readClassName(uint32_t vptr) {
                std::map<uint32_t, std::string>::iterator it = classNameCache.find(vptr);
                if (it != classNameCache.end())
                    return it->second;
                return classNameCache[vptr] = doReadClassName(vptr);
            }

            /// read a null-terminated C string
            virtual const std::string readCString (uint32_t offset) = 0;

            /// @return true if the process is suspended
            virtual bool isSuspended() = 0;
            /// @return true if the process is attached
            virtual bool isAttached() = 0;
            /// @return true if the process is identified -- has a Memory.xml entry
            virtual bool isIdentified() = 0;
            /// @return true if this is a Process snapshot
            virtual bool isSnapshot() { return false; };

            /// find the thread IDs of the process
            virtual bool getThreadIDs(std::vector<uint32_t> & threads ) = 0;
            /// get virtual memory ranges of the process (what is mapped where)
            virtual void getMemRanges(std::vector<t_memrange> & ranges ) = 0;

            /// get the flattened Memory.xml entry of this process
            virtual VersionInfo *getDescriptor() = 0;
            /// get the DF Process ID
            virtual int getPID() = 0;
            /// get the DF Process FilePath
            virtual std::string getPath() = 0;
            /// get module index by name and version. bool 1 = error
            virtual bool getModuleIndex (const char * name, const uint32_t version, uint32_t & OUTPUT) = 0;
            /// get the SHM start if available
            virtual char * getSHMStart (void) = 0;
            /// set a SHM command and wait for a response, return 0 on error or throw exception
            virtual bool SetAndWait (uint32_t state) = 0;
    };

    class DFHACK_EXPORT ClassNameCheck {
        std::string name;
        mutable uint32_t vptr;
    public:
        ClassNameCheck() : vptr(0) {}
        ClassNameCheck(std::string _name) : name(_name), vptr(0) {}
        ClassNameCheck &operator= (const ClassNameCheck &b) {
            name = b.name; vptr = b.vptr; return *this;
        }
        bool operator() (Process *p, uint32_t ptr) const {
            if (vptr == 0 && p->readClassName(ptr) == name)
                vptr = ptr;
            return (vptr && vptr == ptr);
        }
    };
}
#endif
