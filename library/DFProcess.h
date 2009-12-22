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

#include "Export.h"

namespace DFHack
{
    class memory_info;
    class DataModel;
    class Process;
    class DFWindow;
    
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
        public:
            virtual bool attach() = 0;
            virtual bool detach() = 0;
            
            virtual bool suspend() = 0;
            virtual bool resume() = 0;
            virtual bool forceresume() = 0;
            
            virtual uint32_t readDWord(const uint32_t address) = 0;
            virtual void readDWord(const uint32_t address, uint32_t & value) = 0;
            virtual uint16_t readWord(const uint32_t address) = 0;
            virtual void readWord(const uint32_t address, uint16_t & value) = 0;
            virtual uint8_t readByte(const uint32_t address) = 0;
            virtual void readByte(const uint32_t address, uint8_t & value) = 0;
            virtual void read( const uint32_t address, const uint32_t length, uint8_t* buffer) = 0;
            
            virtual void writeDWord(const uint32_t address, const uint32_t value) = 0;
            virtual void writeWord(const uint32_t address, const uint16_t value) = 0;
            virtual void writeByte(const uint32_t address, const uint8_t value) = 0;
            virtual void write(const uint32_t address,const uint32_t length, const uint8_t* buffer) = 0;

            virtual const std::string readCString (uint32_t offset) = 0;
            
            virtual bool isSuspended() = 0;
            virtual bool isAttached() = 0;
            virtual bool isIdentified() = 0;
            
            virtual bool getThreadIDs(vector<uint32_t> & threads ) = 0;
            virtual void getMemRanges( vector<t_memrange> & ranges ) = 0;
            
            virtual memory_info *getDescriptor() = 0;
            virtual DataModel *getDataModel() = 0;
            virtual DFWindow * getWindow() = 0;
            virtual int getPID() = 0;
    };

    class DFHACK_EXPORT NormalProcess : virtual public Process
    {
        friend class ProcessEnumerator;
        class Private;
        private:
            Private * const d;
            NormalProcess(uint32_t pid, vector <memory_info> & known_versions);
            ~NormalProcess();
        public:
            // Set up stuff so we can read memory
            bool attach();
            bool detach();
            
            bool suspend();
            bool resume();
            bool forceresume();
            
            uint32_t readDWord(const uint32_t address);
            void readDWord(const uint32_t address, uint32_t & value);
            uint16_t readWord(const uint32_t address);
            void readWord(const uint32_t address, uint16_t & value);
            uint8_t readByte(const uint32_t address);
            void readByte(const uint32_t address, uint8_t & value);
            void read( const uint32_t address, const uint32_t length, uint8_t* buffer);
            
            void writeDWord(const uint32_t address, const uint32_t value);
            void writeWord(const uint32_t address, const uint16_t value);
            void writeByte(const uint32_t address, const uint8_t value);
            void write(const uint32_t address,const uint32_t length, const uint8_t* buffer);

            const std::string readCString (uint32_t offset);
            
            bool isSuspended();
            bool isAttached();
            bool isIdentified();
            
            bool getThreadIDs(vector<uint32_t> & threads );
            void getMemRanges( vector<t_memrange> & ranges );
            memory_info *getDescriptor();
            DataModel *getDataModel();
            DFWindow * getWindow();
            int getPID();
    };
    
    class DFHACK_EXPORT SHMProcess : virtual public Process
    {
        friend class ProcessEnumerator;
        class Private;
        private:
            Private * const d;
            SHMProcess(uint32_t pid, int shmid, char * shm, vector <memory_info> & known_versions);
            ~SHMProcess();
        public:
            // Set up stuff so we can read memory
            bool attach();
            bool detach();
            
            bool suspend();
            bool resume();
            bool forceresume();
            
            uint32_t readDWord(const uint32_t address);
            void readDWord(const uint32_t address, uint32_t & value);
            uint16_t readWord(const uint32_t address);
            void readWord(const uint32_t address, uint16_t & value);
            uint8_t readByte(const uint32_t address);
            void readByte(const uint32_t address, uint8_t & value);
            void read( const uint32_t address, const uint32_t length, uint8_t* buffer);
            
            void writeDWord(const uint32_t address, const uint32_t value);
            void writeWord(const uint32_t address, const uint16_t value);
            void writeByte(const uint32_t address, const uint8_t value);
            void write(const uint32_t address,const uint32_t length, const uint8_t* buffer);

            const std::string readCString (uint32_t offset);
            
            bool isSuspended();
            bool isAttached();
            bool isIdentified();
            
            bool getThreadIDs(vector<uint32_t> & threads );
            void getMemRanges( vector<t_memrange> & ranges );
            memory_info *getDescriptor();
            DataModel *getDataModel();
            DFWindow * getWindow();
            int getPID();
    };
}
#endif
