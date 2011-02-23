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
#include "Internal.h"
#include "LinuxProcess.h"
#include "ProcessFactory.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/DFError.h"
#include <errno.h>
#include <sys/ptrace.h>
#include <stdio.h>
using namespace DFHack;

namespace {
    class WineProcess : public LinuxProcessBase
    {
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
}

Process* DFHack::createWineProcess(uint32_t pid, vector <VersionInfo *> & known_versions)
{
    return new WineProcess(pid, known_versions);
}

WineProcess::WineProcess(uint32_t pid, vector <VersionInfo *> & known_versions) : LinuxProcessBase(pid)
{
    char dir_name [256];
    char exe_link_name [256];
    char mem_name [256];
    char cwd_name [256];
    char cmdline_name [256];
    char target_name[1024];
    int target_result;

    identified = false;
    my_descriptor = 0;

    sprintf(dir_name,"/proc/%d/", pid);
    sprintf(exe_link_name,"/proc/%d/exe", pid);
    sprintf(mem_name,"/proc/%d/mem", pid);
    sprintf(cwd_name,"/proc/%d/cwd", pid);
    sprintf(cmdline_name,"/proc/%d/cmdline", pid);

    // resolve /proc/PID/exe link
    target_result = readlink(exe_link_name, target_name, sizeof(target_name)-1);
    if (target_result == -1)
    {
        return;
    }
    // make sure we have a null terminated string...
    target_name[target_result] = 0;

    // FIXME: this fails when the wine process isn't started from the 'current working directory'. strip path data from cmdline
    // is this windows version of Df running in wine?
    if(strstr(target_name, "wine-preloader")!= NULL)
    {
        // get working directory
        target_result = readlink(cwd_name, target_name, sizeof(target_name)-1);
        target_name[target_result] = 0;

        // got path to executable, do the same for its name
        ifstream ifs ( cmdline_name , ifstream::in );
        string cmdline;
        getline(ifs,cmdline);
        if (cmdline.find("dwarfort-w.exe") != string::npos || cmdline.find("dwarfort.exe") != string::npos || cmdline.find("Dwarf Fortress.exe") != string::npos)
        {
            char exe_link[1024];
            // put executable name and path together
            sprintf(exe_link,"%s/%s",target_name,cmdline.c_str());

            // create wine process, add it to the vector
            identified = validate(exe_link,pid,mem_name,known_versions);
            return;
        }
    }
}


bool WineProcess::validate(char * exe_file,uint32_t pid, char * memFile, vector <VersionInfo *> & known_versions)
{
    md5wrapper md5;
    // get hash of the running DF process
    string hash = md5.getHashFromFile(exe_file);
    vector<VersionInfo *>::iterator it;

    // iterate over the list of memory locations
    for ( it=known_versions.begin() ; it < known_versions.end(); it++ )
    {
        try
        {
            if (hash == (*it)->getMD5()) // are the md5 hashes the same?
            {
                if (OS_WINDOWS == (*it)->getOS())
                {
                    // keep track of created memory_info object so we can destroy it later
                    my_descriptor = new VersionInfo(**it);
                    my_descriptor->setParentProcess(this);
                    // tell Process about the /proc/PID/mem file
                    memFile = memFile;
                    identified = true;

                    OffsetGroup * strGrp = my_descriptor->getGroup("string")->getGroup("MSVC");
                    STLSTR_buf_off = strGrp->getOffset("buffer");
                    STLSTR_size_off = strGrp->getOffset("size");
                    STLSTR_cap_off = strGrp->getOffset("capacity");
                    return true;
                }
            }
        }
        catch (Error::AllMemdef&)
        {
            continue;
        }
    }
    return false;
}


size_t WineProcess::readSTLString (uint32_t offset, char * buffer, size_t bufcapacity)
{
    uint32_t start_offset = offset + STLSTR_buf_off;
    size_t length = Process::readDWord(offset + STLSTR_size_off);
    size_t capacity = Process::readDWord(offset + STLSTR_cap_off);

    size_t read_real = min(length, bufcapacity-1);// keep space for null termination

    // read data from inside the string structure
    if(capacity < 16)
    {
        read(start_offset, read_real , (uint8_t *)buffer);
    }
    else // read data from what the offset + 4 dword points to
    {
        start_offset = Process::readDWord(start_offset);// dereference the start offset
        read(start_offset, read_real, (uint8_t *)buffer);
    }

    buffer[read_real] = 0;
    return read_real;
}

const string WineProcess::readSTLString (uint32_t offset)
{
    uint32_t start_offset = offset + STLSTR_buf_off;
    size_t length = Process::readDWord(offset + STLSTR_size_off);
    size_t capacity = Process::readDWord(offset + STLSTR_cap_off);

    char * temp = new char[capacity+1];

    // read data from inside the string structure
    if(capacity < 16)
    {
        read(start_offset, capacity, (uint8_t *)temp);
    }
    else // read data from what the offset + 4 dword points to
    {
        start_offset = Process::readDWord(start_offset);// dereference the start offset
        read(start_offset, capacity, (uint8_t *)temp);
    }

    temp[length] = 0;
    string ret = temp;
    delete temp;
    return ret;
}

string WineProcess::readClassName (uint32_t vptr)
{
    int rtti = Process::readDWord(vptr - 0x4);
    int typeinfo = Process::readDWord(rtti + 0xC);
    string raw = readCString(typeinfo + 0xC); // skips the .?AV
    raw.resize(raw.length() - 2);// trim @@ from end
    return raw;
}
