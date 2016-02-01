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

#include "Internal.h"
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <mach-o/dyld.h>

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdio>
#include <cstring>
using namespace std;

#include <md5wrapper.h>
#include "MemAccess.h"
#include "VersionInfoFactory.h"
#include "VersionInfo.h"
#include "Error.h"
#include <string.h>
using namespace DFHack;

Process::Process(VersionInfoFactory * known_versions)
{
    int target_result;

    char path[1024];
    char *real_path;
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        real_path = realpath(path, NULL);
    }

    identified = false;
    my_descriptor = 0;
    my_pe = 0;

    md5wrapper md5;
    uint32_t length;
    uint8_t first_kb [1024];
    memset(first_kb, 0, sizeof(first_kb));
    // get hash of the running DF process
    my_md5 = md5.getHashFromFile(real_path, length, (char *) first_kb);
    // create linux process, add it to the vector
    VersionInfo * vinfo = known_versions->getVersionInfoByMD5(my_md5);
    if(vinfo)
    {
        my_descriptor = new VersionInfo(*vinfo);
        identified = true;
    }
    else
    {
        char * wd = getcwd(NULL, 0);
        cerr << "Unable to retrieve version information.\n";
        cerr << "File: " << real_path << endl;
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
    // destroy our copy of the memory descriptor
    delete my_descriptor;
}

string Process::doReadClassName (void * vptr)
{
    //FIXME: BAD!!!!!
    char * typeinfo = Process::readPtr(((char *)vptr - 0x4));
    char * typestring = Process::readPtr(typeinfo + 0x4);
    string raw = readCString(typestring);
    size_t  start = raw.find_first_of("abcdefghijklmnopqrstuvwxyz");// trim numbers
    size_t end = raw.length();
    return raw.substr(start,end-start);
}

#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/vm_region.h>
#include <mach/vm_statistics.h>
#include <dlfcn.h>

const char *
inheritance_strings[] = {
    "SHARE", "COPY", "NONE", "DONATE_COPY",
};

const char *
behavior_strings[] = {
    "DEFAULT", "RANDOM", "SEQUENTIAL", "RESQNTL", "WILLNEED", "DONTNEED",
};

void Process::getMemRanges( vector<t_memrange> & ranges )
{
    static bool log_ranges = (getenv("DFHACK_LOG_MEM_RANGES") != NULL);

    kern_return_t kr;
    task_t the_task;

    the_task = mach_task_self();

    vm_size_t vmsize;
    vm_address_t address;
    vm_region_basic_info_data_t info;
    mach_msg_type_number_t info_count;
    vm_region_flavor_t flavor;
    memory_object_name_t object;

    kr = KERN_SUCCESS;
    address = 0;

    do {
        flavor = VM_REGION_BASIC_INFO;
        info_count = VM_REGION_BASIC_INFO_COUNT;
        kr = vm_region(the_task, &address, &vmsize, flavor,
                       (vm_region_info_t)&info, &info_count, &object);
        if (kr == KERN_SUCCESS) {
            if (info.reserved==1) {
                address += vmsize;
                continue;
            }
            Dl_info dlinfo;
            int dlcheck;
            dlcheck = dladdr((const void*)address, &dlinfo);
            if (dlcheck==0) {
                dlinfo.dli_fname = "";
            }

            t_memrange temp;
            strncpy( temp.name, dlinfo.dli_fname, 1023 );
            temp.name[1023] = 0;
            temp.start = (void *) address;
            temp.end = (void *) (address+vmsize);
            temp.read = (info.protection & VM_PROT_READ);
            temp.write = (info.protection & VM_PROT_WRITE);
            temp.execute = (info.protection & VM_PROT_EXECUTE);
            temp.shared = info.shared;
            temp.valid = true;
            ranges.push_back(temp);

            if (log_ranges)
            {
                fprintf(stderr,
                "%08x-%08x %8uK %c%c%c/%c%c%c %11s %6s %10s uwir=%hu sub=%u dlname: %s\n",
                            address, (address + vmsize), (vmsize >> 10),
                            (info.protection & VM_PROT_READ)        ? 'r' : '-',
                            (info.protection & VM_PROT_WRITE)       ? 'w' : '-',
                            (info.protection & VM_PROT_EXECUTE)     ? 'x' : '-',
                            (info.max_protection & VM_PROT_READ)    ? 'r' : '-',
                            (info.max_protection & VM_PROT_WRITE)   ? 'w' : '-',
                            (info.max_protection & VM_PROT_EXECUTE) ? 'x' : '-',
                            inheritance_strings[info.inheritance],
                            (info.shared) ? "shared" : "-",
                            behavior_strings[info.behavior],
                            info.user_wired_count,
                            info.reserved,
                            dlinfo.dli_fname);
            }

            address += vmsize;
        } else if (kr != KERN_INVALID_ADDRESS) {

            /*if (the_task != MACH_PORT_NULL) {
                mach_port_deallocate(mach_task_self(), the_task);
            }*/
            return;
        }
    } while (kr != KERN_INVALID_ADDRESS);


/*    if (the_task != MACH_PORT_NULL) {
        mach_port_deallocate(mach_task_self(), the_task);
    }*/
}

uintptr_t Process::getBase()
{
    return 0x1000;
}

int Process::adjustOffset(int offset, bool /*to_file*/)
{
    return offset;
}

static int getdir (string dir, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL)
    {
        cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }
    while ((dirp = readdir(dp)) != NULL) {
    files.push_back(string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}

uint32_t Process::getTickCount()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}

string Process::getPath()
{
    static string cached_path = "";
    if (cached_path.size())
        return cached_path;
    char path[1024];
    char *real_path;
    uint32_t size = sizeof(path);
    if (getcwd(path, size))
    {
        cached_path = string(path);
        return cached_path;
    }
    if (_NSGetExecutablePath(path, &size) == 0) {
        real_path = realpath(path, NULL);
    }
    std::string path_string(real_path);
    int last_slash = path_string.find_last_of("/");
    cached_path = path_string.substr(0,last_slash);
    return cached_path;
}

int Process::getPID()
{
    return getpid();
}

bool Process::setPermisions(const t_memrange & range,const t_memrange &trgrange)
{
    int result;
    int protect=0;
    if(trgrange.read)protect|=PROT_READ;
    if(trgrange.write)protect|=PROT_WRITE;
    if(trgrange.execute)protect|=PROT_EXEC;
    result=mprotect((void *)range.start, (size_t)range.end-(size_t)range.start,protect);

    return result==0;
}

// returns -1 on error
void* Process::memAlloc(const int length)
{
    return mmap(0, length, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
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
