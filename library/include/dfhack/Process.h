/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

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

#include "dfhack/Pragma.h"
#include "dfhack/Export.h"
#include <iostream>
#include <cstring>
#include <map>

namespace DFHack
{
    class VersionInfo;
    class Process;
    class Window;
    class DFVector;
    class VersionInfoFactory;
    class PlatformSpecific;

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

    /**
     * Allows low-level access to the memory of an OS process.
     * \ingroup grp_context
     */
    class DFHACK_EXPORT Process
    {
        public:
            /// this is the single most important destructor ever. ~px
            Process(VersionInfoFactory * known_versions);
            ~Process();
            /// read a 8-byte integer
            uint64_t readQuad(const uint32_t address)
            {
                return *(uint64_t *)address;
            }
            /// read a 8-byte integer
            void readQuad(const uint32_t address, uint64_t & value)
            {
                value = *(uint64_t *)address;
            };
            /// write a 8-byte integer
            void writeQuad(const uint32_t address, const uint64_t value)
            {
                (*(uint64_t *)address) = value;
            };

            /// read a 4-byte integer
            uint32_t readDWord(const uint32_t address)
            {
                return *(uint32_t *)address;
            }
            /// read a 4-byte integer
            void readDWord(const uint32_t address, uint32_t & value)
            {
                value = *(uint32_t *)address;
            };
            /// write a 4-byte integer
            void writeDWord(const uint32_t address, const uint32_t value)
            {
                (*(uint32_t *)address) = value;
            };

            /// read a float
            float readFloat(const uint32_t address)
            {
                return *(float*)address;
            }
            /// write a float
            void readFloat(const uint32_t address, float & value)
            {
                value = *(float*)address;
            };

            /// read a 2-byte integer
            uint16_t readWord(const uint32_t address)
            {
                return *(uint16_t *)address;
            }
            /// read a 2-byte integer
            void readWord(const uint32_t address, uint16_t & value)
            {
                value = *(uint16_t *)address;
            };
            /// write a 2-byte integer
            void writeWord(const uint32_t address, const uint16_t value)
            {
                (*(uint16_t *)address) = value;
            };

            /// read a byte
            uint8_t readByte(const uint32_t address)
            {
                return *(uint8_t *)address;
            }
            /// read a byte
            void readByte(const uint32_t address, uint8_t & value)
            {
                value = *(uint8_t *)address;
            };
            /// write a byte
            void writeByte(const uint32_t address, const uint8_t value)
            {
                (*(uint8_t *)address) = value;
            };

            /// read an arbitrary amount of bytes
            void read( uint32_t address, uint32_t length, uint8_t* buffer)
            {
                memcpy(buffer, (void *) address, length);
            };
            /// write an arbitrary amount of bytes
            void write(uint32_t address, uint32_t length, uint8_t* buffer)
            {
                memcpy((void *) address, buffer, length);
            };

            /// read an STL string
            const std::string readSTLString (uint32_t offset)
            {
                std::string * str = (std::string *) offset;
                return *str;
            };
            /// read an STL string
            size_t readSTLString (uint32_t offset, char * buffer, size_t bufcapacity)
            {
                if(!bufcapacity || bufcapacity == 1)
                    return 0;
                std::string * str = (std::string *) offset;
                size_t copied = str->copy(buffer,bufcapacity-1);
                buffer[copied] = 0;
                return copied;
            };
            /**
             * write an STL string
             * @return length written
             */
            size_t writeSTLString(const uint32_t address, const std::string writeString)
            {
                std::string * str = (std::string *) address;
                str->assign(writeString);
                return writeString.size();
            };
            /**
             * attempt to copy a string from source address to target address. may truncate or leak, depending on platform
             * @return length copied
             */
            size_t copySTLString(const uint32_t address, const uint32_t target)
            {
                std::string * strsrc = (std::string *) address;
                std::string * str = (std::string *) target;
                str->assign(*strsrc);
                return str->size();
            }

            /// get class name of an object with rtti/type info
            std::string doReadClassName(void * vptr);

            std::string readClassName(void * vptr)
            {
                std::map<void *, std::string>::iterator it = classNameCache.find(vptr);
                if (it != classNameCache.end())
                    return it->second;
                return classNameCache[vptr] = doReadClassName(vptr);
            }

            /// read a null-terminated C string
            const std::string readCString (uint32_t offset)
            {
                return std::string((char *) offset);
            };

            /// @return true if the process is suspended
            bool isSuspended()
            {
                return true;
            };
            /// @return true if the process is identified -- has a Memory.xml entry
            bool isIdentified()
            {
                return identified;
            };
            /// find the thread IDs of the process
            bool getThreadIDs(std::vector<uint32_t> & threads );
            /// get virtual memory ranges of the process (what is mapped where)
            void getMemRanges(std::vector<t_memrange> & ranges );

            /// get the flattened Memory.xml entry of this process
            VersionInfo *getDescriptor()
            {
                return my_descriptor;
            };
            uint32_t getBase();
            /// get the DF Process ID
            int getPID();
            /// get the DF Process FilePath
            std::string getPath();
    private:
        VersionInfo * my_descriptor;
        PlatformSpecific *d;
        bool identified;
        uint32_t my_pid;
        uint32_t base;
        std::map<void *, std::string> classNameCache;
    };

    class DFHACK_EXPORT ClassNameCheck
    {
        std::string name;
        mutable void * vptr;

    public:
        ClassNameCheck() : vptr(0) {}
        ClassNameCheck(std::string _name);
        ClassNameCheck &operator= (const ClassNameCheck &b);

        // Is the class name of the given virtual table pointer the same as the
        // name for thei ClassNameCheck object?
        bool operator() (Process *p, void * ptr) const;

        // Get list of names given to ClassNameCheck constructors.
        static void getKnownClassNames(std::vector<std::string> &names);
    };
}
#endif
