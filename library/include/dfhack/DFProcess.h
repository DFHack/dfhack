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

#include "DFPragma.h"
#include "DFExport.h"
#include <iostream>

namespace DFHack
{
    class memory_info;
    class Process;
    class Window;
    
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
        uint8_t * buffer;
    };

    class DFHACK_EXPORT Process
    {
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
            virtual uint64_t readQuad(const uint32_t address) = 0;
            /// read a 8-byte integer
            virtual void readQuad(const uint32_t address, uint64_t & value) = 0;
            /// write a 8-byte integer
            virtual void writeQuad(const uint32_t address, const uint64_t value) = 0;

            /// read a 4-byte integer
            virtual uint32_t readDWord(const uint32_t address) = 0;
            /// read a 4-byte integer
            virtual void readDWord(const uint32_t address, uint32_t & value) = 0;
            /// write a 4-byte integer
            virtual void writeDWord(const uint32_t address, const uint32_t value) = 0;

            /// read a float
            virtual float readFloat(const uint32_t address) = 0;
            /// write a float
            virtual void readFloat(const uint32_t address, float & value) = 0;

            /// read a 2-byte integer
            virtual uint16_t readWord(const uint32_t address) = 0;
            /// read a 2-byte integer
            virtual void readWord(const uint32_t address, uint16_t & value) = 0;
            /// write a 2-byte integer
            virtual void writeWord(const uint32_t address, const uint16_t value) = 0;

            /// read a byte
            virtual uint8_t readByte(const uint32_t address) = 0;
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
            /// write an STL string
            virtual void writeSTLString(const uint32_t address, const std::string writeString) = 0;
            /// get class name of an object with rtti/type info
            virtual std::string readClassName(uint32_t vptr) = 0;

            /// read a null-terminated C string
            virtual const std::string readCString (uint32_t offset) = 0;

            /// @return true if the process is suspended
            virtual bool isSuspended() = 0;
            /// @return true if the process is attached
            virtual bool isAttached() = 0;
            /// @return true if the process is identified -- has a Memory.xml entry
            virtual bool isIdentified() = 0;

            /// find the thread IDs of the process
            virtual bool getThreadIDs(std::vector<uint32_t> & threads ) = 0;
            /// get virtual memory ranges of the process (what is mapped where)
            virtual void getMemRanges(std::vector<t_memrange> & ranges ) = 0;

            /// get the flattened Memory.xml entry of this process
            virtual memory_info *getDescriptor() = 0;
            /// get the DF Process ID
            virtual int getPID() = 0;
            /// get module index by name and version. bool 1 = error
            virtual bool getModuleIndex (const char * name, const uint32_t version, uint32_t & OUTPUT) = 0;
            /// get the SHM start if available
            virtual char * getSHMStart (void) = 0;
            /// set a SHM command and wait for a response, return 0 on error or throw exception
            virtual bool SetAndWait (uint32_t state) = 0;
    };

    ////////////////////////////////////////////////////////////////////////////
    //         Compiler appeasement area. Not worth a look really...          //
    ////////////////////////////////////////////////////////////////////////////

    class DFHACK_EXPORT NormalProcess : virtual public Process
    {
        friend class ProcessEnumerator;
        class Private;
        private:
            Private * const d;
        public:
            NormalProcess(uint32_t pid, std::vector <memory_info *> & known_versions);
            ~NormalProcess();
            bool attach();
            bool detach();
            
            bool suspend();
            bool asyncSuspend();
            bool resume();
            bool forceresume();
            
            uint64_t readQuad(const uint32_t address);
            void readQuad(const uint32_t address, uint64_t & value);
            void writeQuad(const uint32_t address, const uint64_t value);
            
            uint32_t readDWord(const uint32_t address);
            void readDWord(const uint32_t address, uint32_t & value);
            void writeDWord(const uint32_t address, const uint32_t value);
            
            float readFloat(const uint32_t address);
            void readFloat(const uint32_t address, float & value);
            
            uint16_t readWord(const uint32_t address);
            void readWord(const uint32_t address, uint16_t & value);
            void writeWord(const uint32_t address, const uint16_t value);
            
            uint8_t readByte(const uint32_t address);
            void readByte(const uint32_t address, uint8_t & value);
            void writeByte(const uint32_t address, const uint8_t value);
            
            void read( uint32_t address, uint32_t length, uint8_t* buffer);
            void write(uint32_t address, uint32_t length, uint8_t* buffer);
            
            const std::string readSTLString (uint32_t offset);
            size_t readSTLString (uint32_t offset, char * buffer, size_t bufcapacity);
            void writeSTLString(const uint32_t address, const std::string writeString){};
            // get class name of an object with rtti/type info
            std::string readClassName(uint32_t vptr);
            
            const std::string readCString (uint32_t offset);
            
            bool isSuspended();
            bool isAttached();
            bool isIdentified();
            
            bool getThreadIDs(std::vector<uint32_t> & threads );
            void getMemRanges(std::vector<t_memrange> & ranges );
            memory_info *getDescriptor();
            int getPID();
            // get module index by name and version. bool 1 = error
            bool getModuleIndex (const char * name, const uint32_t version, uint32_t & OUTPUT) { OUTPUT=0; return false;};
            // get the SHM start if available
            char * getSHMStart (void){return 0;};
            // set a SHM command and wait for a response
            bool SetAndWait (uint32_t state){return false;};
    };
    
    class DFHACK_EXPORT SHMProcess : virtual public Process
    {
        friend class ProcessEnumerator;
        class Private;
        private:
            Private * const d;
            
        public:
            SHMProcess(uint32_t PID, std::vector <memory_info *> & known_versions);
            ~SHMProcess();
            // Set up stuff so we can read memory
            bool attach();
            bool detach();
            
            bool suspend();
            bool asyncSuspend();
            bool resume();
            bool forceresume();
            
            uint64_t readQuad(const uint32_t address);
            void readQuad(const uint32_t address, uint64_t & value);
            void writeQuad(const uint32_t address, const uint64_t value);
            
            uint32_t readDWord(const uint32_t address);
            void readDWord(const uint32_t address, uint32_t & value);
            void writeDWord(const uint32_t address, const uint32_t value);
            
            float readFloat(const uint32_t address);
            void readFloat(const uint32_t address, float & value);
            
            uint16_t readWord(const uint32_t address);
            void readWord(const uint32_t address, uint16_t & value);
            void writeWord(const uint32_t address, const uint16_t value);
            
            uint8_t readByte(const uint32_t address);
            void readByte(const uint32_t address, uint8_t & value);
            void writeByte(const uint32_t address, const uint8_t value);
            
            void read( uint32_t address, uint32_t length, uint8_t* buffer);
            void write(uint32_t address, uint32_t length, uint8_t* buffer);
            
            const std::string readSTLString (uint32_t offset);
            size_t readSTLString (uint32_t offset, char * buffer, size_t bufcapacity);
            void writeSTLString(const uint32_t address, const std::string writeString);
            // get class name of an object with rtti/type info
            std::string readClassName(uint32_t vptr);
            
            const std::string readCString (uint32_t offset);
            
            bool isSuspended();
            bool isAttached();
            bool isIdentified();
            
            bool getThreadIDs(std::vector<uint32_t> & threads );
            void getMemRanges(std::vector<t_memrange> & ranges );
            memory_info *getDescriptor();
            int getPID();
            // get module index by name and version. bool 1 = error
            bool getModuleIndex (const char * name, const uint32_t version, uint32_t & OUTPUT);
            // get the SHM start if available
            char * getSHMStart (void);
            bool SetAndWait (uint32_t state);
    };

#ifdef LINUX_BUILD
    class DFHACK_EXPORT WineProcess : virtual public Process
    {
        friend class ProcessEnumerator;
        class Private;
        private:
            Private * const d;
            
        public:
            WineProcess(uint32_t pid, std::vector <memory_info *> & known_versions);
            ~WineProcess();
            bool attach();
            bool detach();
            
            bool suspend();
            bool asyncSuspend();
            bool resume();
            bool forceresume();
            
            uint64_t readQuad(const uint32_t address);
            void readQuad(const uint32_t address, uint64_t & value);
            void writeQuad(const uint32_t address, const uint64_t value);
            
            uint32_t readDWord(const uint32_t address);
            void readDWord(const uint32_t address, uint32_t & value);
            void writeDWord(const uint32_t address, const uint32_t value);
            
            float readFloat(const uint32_t address);
            void readFloat(const uint32_t address, float & value);
            
            uint16_t readWord(const uint32_t address);
            void readWord(const uint32_t address, uint16_t & value);
            void writeWord(const uint32_t address, const uint16_t value);
            
            uint8_t readByte(const uint32_t address);
            void readByte(const uint32_t address, uint8_t & value);
            void writeByte(const uint32_t address, const uint8_t value);
            
            void read( uint32_t address, uint32_t length, uint8_t* buffer);
            void write(uint32_t address, uint32_t length, uint8_t* buffer);

            const std::string readSTLString (uint32_t offset);
            size_t readSTLString (uint32_t offset, char * buffer, size_t bufcapacity);
            void writeSTLString(const uint32_t address, const std::string writeString){};
            // get class name of an object with rtti/type info
            std::string readClassName(uint32_t vptr);
            
            const std::string readCString (uint32_t offset);
            
            bool isSuspended();
            bool isAttached();
            bool isIdentified();
            
            bool getThreadIDs(std::vector<uint32_t> & threads );
            void getMemRanges(std::vector<t_memrange> & ranges );
            memory_info *getDescriptor();
            int getPID();
            // get module index by name and version. bool 1 = error
            bool getModuleIndex (const char * name, const uint32_t version, uint32_t & OUTPUT) {OUTPUT=0; return false;};
            // get the SHM start if available
            char * getSHMStart (void){return 0;};
            bool SetAndWait (uint32_t state){return false;};
    };
#endif
}
#endif
