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
#include "dfhack/VersionInfo.h"
#include "dfhack/DFError.h"
#include <errno.h>
#include <sys/ptrace.h>
using namespace DFHack;

NormalProcess::NormalProcess(uint32_t pid, vector <VersionInfo *> & known_versions) : LinuxProcessBase(pid)
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

    // is this the regular linux DF?
    if (strstr(target_name, "dwarfort.exe") != 0 || strstr(target_name,"Dwarf_Fortress") != 0)
    {
        // create linux process, add it to the vector
        identified = validate(target_name,pid,mem_name,known_versions);
        return;
    }
}

bool NormalProcess::validate(char * exe_file,uint32_t pid, char * memFile, vector <VersionInfo *> & known_versions)
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
                if (OS_LINUX == (*it)->getOS())
                {
                    // keep track of created memory_info object so we can destroy it later
                    my_descriptor = new VersionInfo(**it);
                    my_descriptor->setParentProcess(this);
                    // tell Process about the /proc/PID/mem file
                    memFile = memFile;
                    identified = true;
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


struct _Rep_base
{
    uint32_t       _M_length;
    uint32_t       _M_capacity;
    uint32_t        _M_refcount;
};

size_t NormalProcess::readSTLString (uint32_t offset, char * buffer, size_t bufcapacity)
{
    _Rep_base header;
    offset = Process::readDWord(offset);
    read(offset - sizeof(_Rep_base),sizeof(_Rep_base),(uint8_t *)&header);
    size_t read_real = min((size_t)header._M_length, bufcapacity-1);// keep space for null termination
    read(offset,read_real,(uint8_t * )buffer);
    buffer[read_real] = 0;
    return read_real;
}

const string NormalProcess::readSTLString (uint32_t offset)
{
    _Rep_base header;

    offset = Process::readDWord(offset);
    read(offset - sizeof(_Rep_base),sizeof(_Rep_base),(uint8_t *)&header);

    // FIXME: use char* everywhere, avoid string
    char * temp = new char[header._M_length+1];
    read(offset,header._M_length+1,(uint8_t * )temp);
    string ret(temp);
    delete temp;
    return ret;
}

string NormalProcess::readClassName (uint32_t vptr)
{
    int typeinfo = Process::readDWord(vptr - 0x4);
    int typestring = Process::readDWord(typeinfo + 0x4);
    string raw = readCString(typestring);
    size_t  start = raw.find_first_of("abcdefghijklmnopqrstuvwxyz");// trim numbers
    size_t end = raw.length();
    return raw.substr(start,end-start);
}
