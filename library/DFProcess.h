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

#ifndef PROCESS_H_INCLUDED
#define PROCESS_H_INCLUDED

#include "Tranquility.h"
#include "Export.h"

namespace DFHack
{
    class memory_info;
    class Process;
    class DFWindow;
    class DfVector;
    
    // structure describing a memory range
    struct DFHACK_EXPORT t_memrange
    {
        uint64_t start;
        uint64_t end;
        // memory range name (if any)
        char name[1024];
        // permission to read
        bool read;
        // permission to write
        bool write;
        // permission to execute
        bool execute;
        inline bool isInRange( uint64_t address)
        {
            if (address >= start && address <= end) return true;
            return false;
        }
        inline void print()
        {
            cout << hex << start << " - " << end << "|" << (read ? "r" : "-") << (write ? "w" : "-") << (execute ? "x" : "-") << "|" << name << endl;
        }
    };

    class DFHACK_EXPORT Process
    {
        private:
            class Private;
            Private *d;
        public:
            // this is the single most important destructor ever. ~px
            Process(vector <memory_info *> & known_versions);
            ~Process();

            // Set up stuff so we can read memory
            bool attach();
            // detach from DF, resume its execution if it's suspended
            bool detach();
            
            // synchronous suspend
            // waits for DF to be actually suspended,
            // this might take a while depending on implementation
            bool suspend();
            // asynchronous suspend to use together with polling and timers
            bool asyncSuspend();
            // resume DF execution
            bool resume();
            // force-resume DF execution - maybe nonsense in this branch? :P
            bool forceresume();
            
            uint32_t readDWord(const uint32_t address);
            void readDWord(const uint32_t address, uint32_t & value);
            uint16_t readWord(const uint32_t address);
            void readWord(const uint32_t address, uint16_t & value);
            uint8_t readByte(const uint32_t address);
            void readByte(const uint32_t address, uint8_t & value);
            void read( uint32_t address, uint32_t length, uint8_t* buffer);
            
            void writeDWord(const uint32_t address, const uint32_t value);
            void writeWord(const uint32_t address, const uint16_t value);
            void writeByte(const uint32_t address, const uint8_t value);
            void write(uint32_t address, uint32_t length, uint8_t* buffer);
            
            const string readSTLString (uint32_t offset);
            size_t readSTLString (uint32_t offset, char * buffer, size_t bufcapacity);
            void writeSTLString(const uint32_t address, const std::string writeString);
            // read a vector from memory
            DfVector readVector (uint32_t offset, uint32_t item_size);
            // get class name of an object with rtti/type info
            string readClassName(uint32_t vptr);
            const std::string readCString (uint32_t offset);
            
            bool isSuspended();
            bool isAttached();
            bool isIdentified();
            
            // find the thread IDs of the process
            bool getThreadIDs(vector<uint32_t> & threads );
            // get virtual memory ranges of the process (what is mapped where)
            void getMemRanges( vector<t_memrange> & ranges );
            // get the flattened Memory.xml entry of this process
            memory_info *getDescriptor();
            // get the DF's window (first that can be found ~_~)
            DFWindow * getWindow();
            // get the DF Process ID
            int getPID();
            // get module index by name and version. bool 1 = error
            bool getModuleIndex (const char * name, const uint32_t version, uint32_t & OUTPUT);
            // get the SHM start if available
            char * getSHMStart (void);
            // wait for a SHM state. returns 0 without the SHM
            bool waitWhile (uint32_t state);
    };
}
#endif
