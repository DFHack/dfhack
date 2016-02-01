/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

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

#include "Pragma.h"
#include "Export.h"
#include <iostream>
#include <cstring>
#include <map>

namespace DFHack
{
    struct VersionInfo;
    class Process;
    //class Window;
    class DFVector;
    class VersionInfoFactory;
    class PlatformSpecific;

    /**
     * Structure describing a section of virtual memory inside a process
     * \ingroup grp_context
     */
    struct DFHACK_EXPORT t_memrange
    {
        void * start;
        void * end;
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
        inline bool isInRange( void * address)
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
            uint64_t readQuad(const void * address)
            {
                return *(uint64_t *)address;
            }
            /// read a 8-byte integer
            void readQuad(const void * address, uint64_t & value)
            {
                value = *(uint64_t *)address;
            };
            /// write a 8-byte integer
            void writeQuad(const void * address, const uint64_t value)
            {
                (*(uint64_t *)address) = value;
            };

            /// read a 4-byte integer
            uint32_t readDWord(const void * address)
            {
                return *(uint32_t *)address;
            }
            /// read a 4-byte integer
            void readDWord(const void * address, uint32_t & value)
            {
                value = *(uint32_t *)address;
            };
            /// write a 4-byte integer
            void writeDWord(const void * address, const uint32_t value)
            {
                (*(uint32_t *)address) = value;
            };

            /// read a pointer
            char * readPtr(const void * address)
            {
                return *(char **)address;
            }
            /// read a pointer
            void readPtr(const void * address, char * & value)
            {
                value = *(char **)address;
            };

            /// read a float
            float readFloat(const void * address)
            {
                return *(float*)address;
            }
            /// write a float
            void readFloat(const void * address, float & value)
            {
                value = *(float*)address;
            };

            /// read a 2-byte integer
            uint16_t readWord(const void * address)
            {
                return *(uint16_t *)address;
            }
            /// read a 2-byte integer
            void readWord(const void * address, uint16_t & value)
            {
                value = *(uint16_t *)address;
            };
            /// write a 2-byte integer
            void writeWord(const void * address, const uint16_t value)
            {
                (*(uint16_t *)address) = value;
            };

            /// read a byte
            uint8_t readByte(const void * address)
            {
                return *(uint8_t *)address;
            }
            /// read a byte
            void readByte(const void * address, uint8_t & value)
            {
                value = *(uint8_t *)address;
            };
            /// write a byte
            void writeByte(const void * address, const uint8_t value)
            {
                (*(uint8_t *)address) = value;
            };

            /// read an arbitrary amount of bytes
            void read(void * address, uint32_t length, uint8_t* buffer)
            {
                memcpy(buffer, (void *) address, length);
            };
            /// write an arbitrary amount of bytes
            void write(void * address, uint32_t length, uint8_t* buffer)
            {
                memcpy((void *) address, buffer, length);
            };

            /// read an STL string
            const std::string readSTLString (void * offset)
            {
                std::string * str = (std::string *) offset;
                return *str;
            };
            /// read an STL string
            size_t readSTLString (void * offset, char * buffer, size_t bufcapacity)
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
            size_t writeSTLString(const void * address, const std::string writeString)
            {
                std::string * str = (std::string *) address;
                str->assign(writeString);
                return writeString.size();
            };
            /**
             * attempt to copy a string from source address to target address. may truncate or leak, depending on platform
             * @return length copied
             */
            size_t copySTLString(const void * address, const uint32_t target)
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
            const std::string readCString (void * offset)
            {
                return std::string((char *) offset);
            };

            /// @return true if the process is suspended
            bool isSuspended()
            {
                return true;
            };
            /// @return true if the process is identified -- has a symbol table extension
            bool isIdentified()
            {
                return identified;
            };
            /// get virtual memory ranges of the process (what is mapped where)
            void getMemRanges(std::vector<t_memrange> & ranges );

            /// get the symbol table extension of this process
            VersionInfo *getDescriptor()
            {
                return my_descriptor;
            };
            uintptr_t getBase();
            /// get the DF Process ID
            int getPID();
            /// get the DF Process FilePath
            std::string getPath();
            /// Adjust between in-memory and in-file image offset
            int adjustOffset(int offset, bool to_file = false);

            /// millisecond tick count, exactly as DF uses
            uint32_t getTickCount();

            /// modify permisions of memory range
            bool setPermisions(const t_memrange & range,const t_memrange &trgrange);

            /// write a possibly read-only memory area
            bool patchMemory(void *target, const void* src, size_t count);

            /// allocate new memory pages for code or stuff
            /// returns -1 on error (0 is a valid address)
            void* memAlloc(const int length);

            /// free memory pages from memAlloc
            /// should have length = alloced length for portability
            /// returns 0 on success
            int memDealloc(void *ptr, const int length);

            /// change memory page permissions
            /// prot is a bitwise OR of the MemProt enum
            /// returns 0 on success
            int memProtect(void *ptr, const int length, const int prot);

            enum MemProt {
                READ = 1,
                WRITE = 2,
                EXEC = 4
            };

            uint32_t getPE() { return my_pe; }
            std::string getMD5() { return my_md5; }

    private:
        VersionInfo * my_descriptor;
        PlatformSpecific *d;
        bool identified;
        uint32_t my_pid;
        uint32_t base;
        std::map<void *, std::string> classNameCache;
        uint32_t my_pe;
        std::string my_md5;
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

    class DFHACK_EXPORT MemoryPatcher
    {
        Process *p;
        std::vector<t_memrange> ranges, save;
    public:
        MemoryPatcher(Process *p = NULL);
        ~MemoryPatcher();

        bool verifyAccess(void *target, size_t size, bool write = false);
        bool makeWritable(void *target, size_t size) {
            return verifyAccess(target, size, true);
        }
        bool write(void *target, const void *src, size_t size);

        void close();
    };
}
#endif
