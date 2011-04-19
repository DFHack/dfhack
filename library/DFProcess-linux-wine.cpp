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
#include "PlatformInternal.h"

#include <string>
#include <vector>
#include <map>
#include <cstdio>
#include <cstring>
#include <fstream>
using namespace std;

#include "LinuxProcess.h"
#include "ProcessFactory.h"
#include "MicrosoftSTL.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/DFError.h"
using namespace DFHack;

#include <errno.h>
#include <sys/ptrace.h>

#include <md5wrapper.h>


namespace {
    class WineProcess : public LinuxProcessBase
    {
        private:
            uint8_t vector_start;
            MicrosoftSTL stl;
        public:
            WineProcess(uint32_t pid, VersionInfoFactory * factory);
            ~WineProcess()
            {
                if(attached)
                {
                    detach();
                }
            }
            bool attach();
            bool detach();

            bool suspend();
            bool asyncSuspend();
            bool resume();
            bool forceresume();

            void readSTLVector(const uint32_t address, t_vecTriplet & triplet);
            const std::string readSTLString (uint32_t offset);
            size_t readSTLString (uint32_t offset, char * buffer, size_t bufcapacity);
            size_t writeSTLString(const uint32_t address, const std::string writeString);
            // get class name of an object with rtti/type info
            std::string readClassName(uint32_t vptr);
    };
}

Process* DFHack::createWineProcess(uint32_t pid, VersionInfoFactory * factory)
{
    return new WineProcess(pid, factory);
}

WineProcess::WineProcess(uint32_t pid, VersionInfoFactory * factory) : LinuxProcessBase(pid)
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
    memFile = mem_name;
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

            md5wrapper md5;
            // get hash of the running DF process
            string hash = md5.getHashFromFile(exe_link);
            // create linux process, add it to the vector
            VersionInfo * vinfo = factory->getVersionInfoByMD5(hash);
            if(vinfo)
            {
                my_descriptor = new VersionInfo(*vinfo);
                my_descriptor->setParentProcess(this);
                vector_start = my_descriptor->getGroup("vector")->getOffset("start");
                stl.init(this);
                identified = true;
            }
            return;
        }
    }
}

void WineProcess::readSTLVector(const uint32_t address, t_vecTriplet & triplet)
{
    read(address + vector_start, sizeof(triplet), (uint8_t *) &triplet);
}

size_t WineProcess::readSTLString (uint32_t offset, char * buffer, size_t bufcapacity)
{
    return stl.readSTLString(offset, buffer, bufcapacity);
}

size_t WineProcess::writeSTLString(const uint32_t address, const std::string writeString)
{
    return stl.writeSTLString(address,writeString);
}


const string WineProcess::readSTLString (uint32_t offset)
{
    return stl.readSTLString(offset);
}

string WineProcess::readClassName (uint32_t vptr)
{
    return stl.readClassName(vptr);
}

bool WineProcess::asyncSuspend()
{
    return suspend();
}

bool WineProcess::suspend()
{
    int status;
    if(suspended)
        return true;
    // can we attach?
    if (ptrace(PTRACE_ATTACH , my_pid, NULL, NULL) == -1)
    {
        // no, we got an error
        perror("ptrace attach error");
        cerr << "attach failed on pid " << my_pid << endl;
        return false;
    }
    while(true)
    {
        // we wait on the pid
        pid_t w = waitpid(my_pid, &status, 0);
        if (w == -1)
        {
            // child died
            perror("wait inside attach()");
            return false;
        }
        // stopped -> let's continue
        if (WIFSTOPPED(status))
        {
            break;
        }
    }
    int proc_pid_mem = open(memFile.c_str(),O_RDONLY);
    if(proc_pid_mem == -1)
    {
        ptrace(PTRACE_DETACH, my_pid, NULL, NULL);
        cerr << memFile << endl;
        cerr << "couldn't open /proc/" << my_pid << "/mem" << endl;
        perror("open(memFile.c_str(),O_RDONLY)");
        return false;
    }
    else
    {
        attached = suspended = true;
        memFileHandle = proc_pid_mem;
        return true; // we are attached
    }
}

bool WineProcess::forceresume()
{
    return resume();
}

bool WineProcess::resume()
{
    if(!suspended)
        return true;
    int result = 0;
    // close /proc/PID/mem
    result = close(memFileHandle);
    if(result == -1)
    {
        cerr << "couldn't close /proc/"<< my_pid <<"/mem" << endl;
        perror("mem file close");
        return false;
    }
    else
    {
        // detach
        result = ptrace(PTRACE_DETACH, my_pid, NULL, NULL);
        if(result == -1)
        {
            cerr << "couldn't detach from process pid" << my_pid << endl;
            perror("ptrace detach");
            return false;
        }
        else
        {
            attached = suspended = false;
            return true;
        }
    }
}


bool WineProcess::attach()
{
    if(suspended) return true;
    return suspend();
}

bool WineProcess::detach()
{
    if(!suspended) return true;
    return resume();
}
