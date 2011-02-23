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

#ifndef LINUX_PROCESS_H_INCLUDED
#define LINUX_PROCESS_H_INCLUDED

#ifdef LINUX_BUILD

#include "dfhack/DFProcess.h"

namespace DFHack
{
    class LinuxProcessBase : public Process 
    {
    public:
        class Private;
        protected:
            Private * const d;
	    bool readProgramName(char *target_name, char *mem_name, char *cmdline_name);
        public:
            LinuxProcessBase(uint32_t pid);
            ~LinuxProcessBase();

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

            const std::string readCString (uint32_t offset);

            bool isSuspended();
            bool isAttached();
            bool isIdentified();

            VersionInfo *getDescriptor();
            int getPID();
            std::string getPath();

            bool getThreadIDs(std::vector<uint32_t> & threads );
            void getMemRanges(std::vector<t_memrange> & ranges );
            // get module index by name and version. bool 1 = error
            bool getModuleIndex (const char * name, const uint32_t version, uint32_t & OUTPUT) { OUTPUT=0; return false;};
            // get the SHM start if available
            char * getSHMStart (void){return 0;};
            // set a SHM command and wait for a response
            bool SetAndWait (uint32_t state){return false;};
    };

    class DFHACK_EXPORT NormalProcess : public LinuxProcessBase
    {
        friend class ProcessEnumerator;
        public:
            NormalProcess(uint32_t pid, std::vector <VersionInfo *> & known_versions);

            const std::string readSTLString (uint32_t offset);
            size_t readSTLString (uint32_t offset, char * buffer, size_t bufcapacity);
            void writeSTLString(const uint32_t address, const std::string writeString){};
            // get class name of an object with rtti/type info
            std::string readClassName(uint32_t vptr);
	private:
	    bool validate(char * exe_file,uint32_t pid, char * memFile, vector <VersionInfo *> & known_versions);
    };

    class DFHACK_EXPORT WineProcess : public LinuxProcessBase
    {
        friend class ProcessEnumerator;
        private:
            uint32_t STLSTR_buf_off;
            uint32_t STLSTR_size_off;
            uint32_t STLSTR_cap_off;
        public:
            WineProcess(uint32_t pid, std::vector <VersionInfo *> & known_versions);

            const std::string readSTLString (uint32_t offset);
            size_t readSTLString (uint32_t offset, char * buffer, size_t bufcapacity);
            void writeSTLString(const uint32_t address, const std::string writeString){};
            // get class name of an object with rtti/type info
            std::string readClassName(uint32_t vptr);
	private:
	    bool validate(char * exe_file,uint32_t pid, char * memFile, vector <VersionInfo *> & known_versions);
    };

    class LinuxProcessBase::Private
    {
    public:
        Private(LinuxProcessBase * self_, pid_t);
        ~Private(){};
        VersionInfo * my_descriptor;
        LinuxProcessBase * self;
        pid_t my_pid;
        string memFile;
        int memFileHandle;
        bool attached;
        bool suspended;
        bool identified;
    };
}

#endif
#endif
