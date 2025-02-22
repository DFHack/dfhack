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

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <dirent.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

#include "Error.h"
#include "Internal.h"
#include "MemAccess.h"
#include "Memory.h"
#include "MiscUtils.h"
#include "VersionInfo.h"
#include "VersionInfoFactory.h"
#include "modules/Filesystem.h"
#include "md5wrapper.h"

using namespace DFHack;

using std::string;
using std::map;
using std::vector;
using std::endl;
using std::cerr;

Process::Process(const VersionInfoFactory& known_versions) : identified(false), my_pe(0)
{
    const char * exe_link_name = "/proc/self/exe";

    // valgrind replaces readlink for /proc/self/exe, but not open.
    char self_exe[1024];
    memset(self_exe, 0, sizeof(self_exe));
    std::string self_exe_name;
    if (readlink(exe_link_name, self_exe, sizeof(self_exe) - 1) < 0)
        self_exe_name = exe_link_name;
    else
        self_exe_name = self_exe;

    md5wrapper md5;
    uint32_t length;
    uint8_t first_kb [1024];
    memset(first_kb, 0, sizeof(first_kb));
    // get hash of the running DF process
    my_md5 = md5.getHashFromFile(self_exe_name, length, (char *) first_kb);
    // create linux process, add it to the vector
    auto vinfo = known_versions.getVersionInfoByMD5(my_md5);
    if(vinfo)
    {
        identified = true;
        my_descriptor = std::make_shared<VersionInfo>(*vinfo);
    }
    else
    {
        char * wd = getcwd(NULL, 0);
        cerr << "Unable to retrieve version information.\n";
        cerr << "File: " << exe_link_name << endl;
        cerr << "MD5: " << my_md5 << endl;
        cerr << "working dir: " << wd << endl;
        cerr << "length:" << length << endl;
        cerr << "1KB hexdump follows:" << endl;
        for(int i = 0; i < 64; i++)
        {
            fprintf(stderr, "%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x\n",
                    first_kb[i*16],
                    first_kb[i*16+1],
                    first_kb[i*16+2],
                    first_kb[i*16+3],
                    first_kb[i*16+4],
                    first_kb[i*16+5],
                    first_kb[i*16+6],
                    first_kb[i*16+7],
                    first_kb[i*16+8],
                    first_kb[i*16+9],
                    first_kb[i*16+10],
                    first_kb[i*16+11],
                    first_kb[i*16+12],
                    first_kb[i*16+13],
                    first_kb[i*16+14],
                    first_kb[i*16+15]
                   );
        }
        free(wd);
    }
}

Process::~Process()
{
    // Nothing to do here
}

string Process::doReadClassName (void * vptr)
{
    char* typeinfo = Process::readPtr(((char *)vptr - sizeof(void*)));
    char* typestring = Process::readPtr(typeinfo + sizeof(void*));
    string raw = readCString(typestring);

    string status;
    string demangled = cxx_demangle(raw, &status);

    if (demangled.length() == 0) {
        return "dummy";
    }

    return demangled;
}

//FIXME: cross-reference with ELF segment entries?
void Process::getMemRanges( vector<t_memrange> & ranges )
{
    char buffer[1024];
    char permissions[5]; // r/-, w/-, x/-, p/s, 0

    FILE *mapFile = ::fopen("/proc/self/maps", "r");
    if (!mapFile)
        return;

    size_t start, end, offset, device1, device2, node;

    string cur_name;
    void * cur_base = nullptr;

    while (fgets(buffer, 1024, mapFile))
    {
        t_memrange temp;
        temp.name[0] = 0;
        sscanf(buffer, "%zx-%zx %s %zx %zx:%zx %zu %[^\n]",
               &start,
               &end,
               (char*)&permissions,
               &offset, &device1, &device2, &node,
               (char*)temp.name);
        if (cur_name != temp.name) {
            cur_name = temp.name;
            cur_base = (void *) start;
        }
        temp.base = cur_base;
        temp.start = (void *) start;
        temp.end = (void *) end;
        temp.read = permissions[0] == 'r';
        temp.write = permissions[1] == 'w';
        temp.execute = permissions[2] == 'x';
        temp.shared = permissions[3] == 's';
        temp.valid = true;
        ranges.push_back(temp);
    }

    fclose(mapFile);
}

uintptr_t Process::getBase()
{
    return DEFAULT_BASE_ADDR;  // Memory.h
}

int Process::adjustOffset(int offset, bool /*to_file*/)
{
    return offset;
}

uint32_t Process::getTickCount()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}

string Process::getPath()
{
    return Filesystem::get_initial_cwd();
}

int Process::getPID()
{
    return getpid();
}

bool Process::setPermissions(const t_memrange & range,const t_memrange &trgrange)
{
    int result;
    int protect=0;
    if(trgrange.read)protect|=PROT_READ;
    if(trgrange.write)protect|=PROT_WRITE;
    if(trgrange.execute)protect|=PROT_EXEC;
    result=mprotect((void *)range.start, (size_t)range.end-(size_t)range.start,protect);

    return result==0;
}

bool Process::flushCache(const void* target, size_t count)
{
    __builtin___clear_cache((char*)target, (char*)target + count - 1);
    return true; /* assume always succeeds, as the builtin has no return type */
}


// returns -1 on error
void* Process::memAlloc(const int length)
{
    return mmap(0, length, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
}

int Process::memDealloc(void *ptr, const int length)
{
    return munmap(ptr, length);
}

int Process::memProtect(void *ptr, const int length, const int prot)
{
    int prot_native = 0;

    if (prot & Process::MemProt::READ)
        prot_native |= PROT_READ;
    if (prot & Process::MemProt::WRITE)
        prot_native |= PROT_WRITE;
    if (prot & Process::MemProt::EXEC)
        prot_native |= PROT_EXEC;

    return mprotect(ptr, length, prot_native);
}
