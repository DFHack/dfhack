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

#ifndef WIN32
#ifndef _DARWIN
#endif /* ! WIN32 */
#include <cstdlib>
#ifndef WIN32
#endif /* ! _DARWIN */
#endif /* ! WIN32 */
#include <cstring>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <filesystem>

#ifndef WIN32
#include <dirent.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>
#ifdef _DARWIN
#include <string.h>

#include <mach-o/dyld.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/vm_region.h>
#include <mach/vm_statistics.h>
#include <dlfcn.h>

#endif /* _DARWIN */

#endif /* ! WIN32 */
#include "Error.h"
#include "Internal.h"
#include "MemAccess.h"
#include "Memory.h"
#include "MiscUtils.h"
#include "VersionInfo.h"
#include "VersionInfoFactory.h"
#include "modules/Filesystem.h"
#ifndef WIN32
#include "md5wrapper.h"
#else /* WIN32 */

#include <format>

#define _WIN32_WINNT 0x0600
#define WINVER 0x0600
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#endif /* WIN32 */

using namespace DFHack;

using std::string;
using std::map;
using std::vector;
using std::endl;
using std::cerr;

#ifdef WIN32
namespace DFHack
{
    class PlatformSpecific
    {
    public:
        PlatformSpecific()
        {
            base = 0;
            sections = 0;
        };
        HANDLE my_handle;
        uint32_t my_pid;
        IMAGE_NT_HEADERS pe_header;
        IMAGE_SECTION_HEADER * sections;
        char * base;
    };
}

#endif /* WIN32 */
Process::Process(const VersionInfoFactory& known_versions) : identified(false)
{
#ifndef WIN32
    std::string self_exe_name;
#ifndef _DARWIN
    const char * exe_link_name = "/proc/self/exe";

    // valgrind replaces readlink for /proc/self/exe, but not open.
    char self_exe[1024];
    memset(self_exe, 0, sizeof(self_exe));
    if (readlink(exe_link_name, self_exe, sizeof(self_exe) - 1) < 0)
        self_exe_name = exe_link_name;
    else
        self_exe_name = self_exe;
#else /* _DARWIN */
    char path[1024];
    char *exec_link_name;
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        exec_link_name = realpath(path, NULL);
        self_exe_name = std::string(exec_link_name);
    }
#endif /* _DARWIN */
#else /* WIN32 */
    HMODULE hmod = NULL;
    DWORD needed;
    bool found = false;

    d = new PlatformSpecific();
    // open process
    d->my_pid = GetCurrentProcessId();
    d->my_handle = GetCurrentProcess();
    // try getting the first module of the process
    if (EnumProcessModules(d->my_handle, &hmod, sizeof(hmod), &needed) == 0)
    {
        return; //if enumprocessModules fails, give up
    }
    // got base ;)
    d->base = (char*)hmod;

    // read from this process
    try
    {
        uint32_t pe_offset = readDWord(d->base + 0x3C);
        read(d->base + pe_offset, sizeof(d->pe_header), (uint8_t*)&(d->pe_header));
        const size_t sectionsSize = sizeof(IMAGE_SECTION_HEADER) * d->pe_header.FileHeader.NumberOfSections;
        d->sections = (IMAGE_SECTION_HEADER*)malloc(sectionsSize);
        read(d->base + pe_offset + sizeof(d->pe_header), sectionsSize, (uint8_t*)(d->sections));
    }
    catch (std::exception&)
    {
        return;
    }
#endif /* WIN32 */

#ifndef WIN32
    my_pe = 0;
    md5wrapper md5;
    uint32_t length;
    uint8_t first_kb [1024];
    memset(first_kb, 0, sizeof(first_kb));
    // get hash of the running DF process
    my_md5 = md5.getHashFromFile(self_exe_name, length, (char *) first_kb);
    // create linux process, add it to the vector
    auto vinfo = known_versions.getVersionInfoByMD5(my_md5);
#else /* WIN32 */
    my_pe = d->pe_header.FileHeader.TimeDateStamp;
    auto vinfo = known_versions.getVersionInfoByPETimestamp(my_pe);
#endif /* WIN32 */
    if(vinfo)
    {
        identified = true;
        my_descriptor = std::make_shared<VersionInfo>(*vinfo);
#ifdef WIN32
        // give the process a data model and memory layout fixed for the base of first module
        my_descriptor->rebaseTo(getBase());
#endif /* WIN32 */
    }
    else
    {
#ifndef WIN32
        char * wd = getcwd(NULL, 0);
#endif /* ! WIN32 */
        cerr << "Unable to retrieve version information.\n";
#ifndef WIN32
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
#else /* WIN32 */
        cerr << "PE timestamp: " << std::format("{:#0x}", my_pe) << endl;
#endif /* WIN32 */
    }
}

Process::~Process()
{
#ifndef WIN32
    // Nothing to do here
#else /* WIN32 */
    // destroy our rebased copy of the memory descriptor
    if(d->sections != NULL)
        free(d->sections);
#endif /* WIN32 */
}

string Process::doReadClassName (void * vptr)
{
    char* rtti = Process::readPtr(((char*)vptr - sizeof(void*)));
#ifndef WIN32
    char* typestring = Process::readPtr(rtti + sizeof(void*));
#else /* WIN32 */
#ifdef DFHACK64
    void* base;
    if (!RtlPcToFileHeader(rtti, &base))
        return "dummy";
    char* typeinfo = (char *)base + readDWord(rtti + 0xC);
    char* typestring = typeinfo + 0x10;
#else
    char* typeinfo = readPtr(rtti + 0xC);
    char* typestring = typeinfo + 0x8;
#endif
#endif /* WIN32 */
    std::string raw = readCString(typestring);
    if (raw.length() == 0)
        return "dummy";

    string status;
    string demangled = cxx_demangle(raw, &status);

    if (demangled.length() == 0) {
        return "dummy";
    }

    return demangled;
}

#ifndef WIN32
#ifndef _DARWIN
//FIXME: cross-reference with ELF segment entries?
#else /* _DARWIN */
const char*
inheritance_strings[] = {
    "SHARE", "COPY", "NONE", "DONATE_COPY",
};

const char*
behavior_strings[] = {
    "DEFAULT", "RANDOM", "SEQUENTIAL", "RESQNTL", "WILLNEED", "DONTNEED",
};

#endif /* _DARWIN */
void Process::getMemRanges(vector<t_memrange>& ranges)
{
#ifndef _DARWIN
    char buffer[1024];
    char permissions[5]; // r/-, w/-, x/-, p/s, 0
#else /* _DARWIN */
    static bool log_ranges = (getenv("DFHACK_LOG_MEM_RANGES") != NULL);

    kern_return_t kr;
    task_t the_task;
#endif /* _DARWIN */

#ifndef _DARWIN
    FILE* mapFile = ::fopen("/proc/self/maps", "r");
    if (!mapFile)
        return;
#else /* _DARWIN */
    the_task = mach_task_self();
#endif /* _DARWIN */

#ifndef _DARWIN
    size_t start, end, offset, device1, device2, node;
#else /* _DARWIN */
#ifdef DFHACK64
    mach_vm_size_t vmsize;
    mach_vm_address_t address;
    vm_region_basic_info_data_64_t info;
#else
    vm_size_t vmsize;
    vm_address_t address;
    vm_region_basic_info_data_t info;
#endif
    mach_msg_type_number_t info_count;
    vm_region_flavor_t flavor;
    memory_object_name_t object;

    kr = KERN_SUCCESS;
    address = 0;
#endif /* _DARWIN */

    string cur_name;
    void* cur_base = nullptr;

#ifndef _DARWIN
    while (fgets(buffer, 1024, mapFile))
    {
#else /* _DARWIN */
    do {
#ifdef DFHACK64
        flavor = VM_REGION_BASIC_INFO_64;
        info_count = VM_REGION_BASIC_INFO_COUNT_64;
        kr = mach_vm_region(the_task, &address, &vmsize, flavor,
            (vm_region_info_64_t)&info, &info_count, &object);
#else
        flavor = VM_REGION_BASIC_INFO;
        info_count = VM_REGION_BASIC_INFO_COUNT;
        kr = vm_region(the_task, &address, &vmsize, flavor,
            (vm_region_info_t)&info, &info_count, &object);
#endif

        if (kr == KERN_INVALID_ADDRESS)
            break;
        if (kr != KERN_SUCCESS)
        {

            /*if (the_task != MACH_PORT_NULL) {
            mach_port_deallocate(mach_task_self(), the_task);
            }*/
            return;
        }
        if (info.reserved == 1) {
            address += vmsize;
            continue;
        }
        Dl_info dlinfo;
        int dlcheck;
        dlcheck = dladdr((const void*)address, &dlinfo);
        if (dlcheck == 0) {
            dlinfo.dli_fname = "";
        }

#endif /* _DARWIN */
        t_memrange temp;
#ifndef _DARWIN
        temp.name[0] = 0;
        sscanf(buffer, "%zx-%zx %s %zx %zx:%zx %zu %[^\n]",
            &start,
            &end,
            (char*)&permissions,
            &offset, &device1, &device2, &node,
            (char*)temp.name);
#else /* _DARWIN */
        strncpy(temp.name, dlinfo.dli_fname, 1023);
        temp.name[1023] = 0;
#endif /* _DARWIN */
        if (cur_name != temp.name) {
            cur_name = temp.name;
#ifndef _DARWIN
            cur_base = (void*)start;
#else /* _DARWIN */
            cur_base = (void*)address;
#endif /* _DARWIN */
        }
        temp.base = cur_base;
#ifndef _DARWIN
        temp.start = (void*)start;
        temp.end = (void*)end;
        temp.read = permissions[0] == 'r';
        temp.write = permissions[1] == 'w';
        temp.execute = permissions[2] == 'x';
        temp.shared = permissions[3] == 's';
#else /* _DARWIN */
        temp.start = (void*)address;
        temp.end = (void*)(address + vmsize);
        temp.read = (info.protection & VM_PROT_READ);
        temp.write = (info.protection & VM_PROT_WRITE);
        temp.execute = (info.protection & VM_PROT_EXECUTE);
        temp.shared = info.shared;
#endif /* _DARWIN */
        temp.valid = true;
        ranges.push_back(temp);
#ifndef _DARWIN
    }
#endif /* ! _DARWIN */

#ifndef _DARWIN
    fclose(mapFile);
#else /* _DARWIN */
    if (log_ranges)
    {
        fprintf(stderr,
            "%p-%p %8zuK %c%c%c/%c%c%c %11s %6s %10s uwir=%hu sub=%u dlname: %s\n",
            (void*)address,
            (void*)(address + vmsize),
            size_t(vmsize >> 10),
            (info.protection & VM_PROT_READ) ? 'r' : '-',
            (info.protection & VM_PROT_WRITE) ? 'w' : '-',
            (info.protection & VM_PROT_EXECUTE) ? 'x' : '-',
            (info.max_protection & VM_PROT_READ) ? 'r' : '-',
            (info.max_protection & VM_PROT_WRITE) ? 'w' : '-',
            (info.max_protection & VM_PROT_EXECUTE) ? 'x' : '-',
            inheritance_strings[info.inheritance],
            (info.shared) ? "shared" : "-",
            behavior_strings[info.behavior],
            info.user_wired_count,
            info.reserved,
            dlinfo.dli_fname);
    }

    address += vmsize;
    } while (kr != KERN_INVALID_ADDRESS);


/*    if (the_task != MACH_PORT_NULL) {
        mach_port_deallocate(mach_task_self(), the_task);
    }*/
#endif /* _DARWIN */
}
#else /* WIN32 */
/*
typedef struct _MEMORY_BASIC_INFORMATION
{
  void *  BaseAddress;
  void *  AllocationBase;
  uint32_t  AllocationProtect;
  size_t RegionSize;
  uint32_t  State;
  uint32_t  Protect;
  uint32_t  Type;
} MEMORY_BASIC_INFORMATION, *PMEMORY_BASIC_INFORMATION;
*/
/*
//Internal structure used to store heap block information.
struct HeapBlock
{
      PVOID dwAddress;
      DWORD dwSize;
      DWORD dwFlags;
      ULONG reserved;
};
*/

static void GetDosNames(std::map<string, string>& table)
{
    // Partially based on example from msdn:
    // Translate path with device name to drive letters.
    TCHAR szTemp[512];
    szTemp[0] = '\0';

    if (GetLogicalDriveStrings(sizeof(szTemp) - 1, szTemp))
    {
        TCHAR szName[MAX_PATH];
        TCHAR szDrive[3] = " :";
        BOOL bFound = FALSE;
        TCHAR* p = szTemp;

        do
        {
            // Copy the drive letter to the template string
            *szDrive = *p;

            // Look up each device name
            if (QueryDosDevice(szDrive, szName, MAX_PATH))
                table[szName] = szDrive;

            // Go to the next NULL character.
            while (*p++);
        } while (*p); // end of string
    }
}

void Process::getMemRanges(vector<t_memrange>& ranges)
{
    MEMORY_BASIC_INFORMATION MBI;
    //map<char *, unsigned int> heaps;
    uint64_t movingStart = 0;
    PVOID LastAllocationBase = 0;
    map <char*, string> nameMap;
    map <string, string> dosDrives;

    // get page size
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    uint64_t PageSize = si.dwPageSize;

    // get dos drive names
    GetDosNames(dosDrives);

    ranges.clear();

    HANDLE my_handle = GetCurrentProcess();

    // enumerate heaps
    // HeapNodes(d->my_pid, heaps);
    // go through all the VM regions, convert them to our internal format
    while (VirtualQueryEx(my_handle, (const void*)(movingStart), &MBI, sizeof(MBI)) == sizeof(MBI))
    {
        t_memrange temp;
        movingStart = ((uint64_t)MBI.BaseAddress + MBI.RegionSize);
        if (movingStart % PageSize != 0)
            movingStart = (movingStart / PageSize + 1) * PageSize;

        // Skip unallocated address space
        if (MBI.State & MEM_FREE)
            continue;

        // Find range and permissions
        memset(&temp, 0, sizeof(temp));

        temp.start = (char*)MBI.BaseAddress;
        temp.end = ((char*)MBI.BaseAddress + (uint64_t)MBI.RegionSize);
        temp.valid = true;

        if (!(MBI.State & MEM_COMMIT))
            temp.valid = false; // reserved address space
        else if (MBI.Protect & PAGE_EXECUTE)
            temp.execute = true;
        else if (MBI.Protect & PAGE_EXECUTE_READ)
            temp.execute = temp.read = true;
        else if (MBI.Protect & PAGE_EXECUTE_READWRITE)
            temp.execute = temp.read = temp.write = true;
        else if (MBI.Protect & PAGE_EXECUTE_WRITECOPY)
            temp.execute = temp.read = temp.write = true;
        else if (MBI.Protect & PAGE_READONLY)
            temp.read = true;
        else if (MBI.Protect & PAGE_READWRITE)
            temp.read = temp.write = true;
        else if (MBI.Protect & PAGE_WRITECOPY)
            temp.read = temp.write = true;

        // Merge areas with the same properties
        if (!ranges.empty() && LastAllocationBase == MBI.AllocationBase)
        {
            auto& last = ranges.back();

            if (last.end == temp.start &&
                last.valid == temp.valid && last.execute == temp.execute &&
                last.read == temp.read && last.write == temp.write)
            {
                last.end = temp.end;
                continue;
            }
        }

        // Find the mapped file name
        if (GetMappedFileName(my_handle, temp.start, temp.name, 1024))
        {
            int vsize = strlen(temp.name);

            // Translate NT name to DOS name
            for (auto it = dosDrives.begin(); it != dosDrives.end(); ++it)
            {
                int ksize = it->first.size();
                if (strncmp(temp.name, it->first.data(), ksize) != 0)
                    continue;

                memcpy(temp.name, it->second.data(), it->second.size());
                memmove(temp.name + it->second.size(), temp.name + ksize, vsize + 1 - ksize);
                break;
            }
        }
        else
            temp.name[0] = 0;

        // Push the entry
        LastAllocationBase = MBI.AllocationBase;
        ranges.push_back(temp);
    }
}
#endif

uintptr_t Process::getBase()
{
#if WIN32
    if(d)
        return (uintptr_t) d->base;
#endif /* WIN32 */
    return DEFAULT_BASE_ADDR; // Memory.h
}

int Process::adjustOffset(int offset, [[maybe_unused]] bool to_file)
{
#ifndef WIN32
    return offset;
#else /* WIN32 */
    if (!d)
        return -1;

    for(int i = 0; i < d->pe_header.FileHeader.NumberOfSections; i++)
    {
        auto &section = d->sections[i];

        if (to_file)
        {
            unsigned delta = offset - section.VirtualAddress;
            if (delta >= section.Misc.VirtualSize)
                continue;
            if (!section.PointerToRawData || delta >= section.SizeOfRawData)
                return -1;
            return (int)(section.PointerToRawData + delta);
        }
        else
        {
            unsigned delta = offset - section.PointerToRawData;
            if (!section.PointerToRawData || delta >= section.SizeOfRawData)
                continue;
            if (delta >= section.Misc.VirtualSize)
                return -1;
            return (int)(section.VirtualAddress + delta);
        }
    }

    return -1;
#endif /* WIN32 */
}

uint32_t Process::getTickCount()
{
#ifndef WIN32
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
#else /* WIN32 */
    return GetTickCount();
#endif /* WIN32 */
}

std::filesystem::path Process::getPath()
{
#if defined(WIN32) || !defined(_DARWIN)
    return Filesystem::get_initial_cwd();
#else /* _DARWIN */
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
    else {
        fprintf(stderr, "_NSGetExecutablePath failed!\n");
        cached_path = ".";
        return cached_path;
    }
    std::string path_string(real_path);
    int last_slash = path_string.find_last_of("/");
    cached_path = path_string.substr(0,last_slash);
    return cached_path;
#endif /* _DARWIN */
}

int Process::getPID()
{
#ifndef WIN32
    return getpid();
#else /* WIN32 */
    return (int) GetCurrentProcessId();
#endif /* WIN32 */
}

#ifdef WIN32

#endif /* WIN32 */
bool Process::setPermissions(const t_memrange & range,const t_memrange &trgrange)
{
#ifndef WIN32
    int result;
    int protect=0;
    if(trgrange.read)protect|=PROT_READ;
    if(trgrange.write)protect|=PROT_WRITE;
    if(trgrange.execute)protect|=PROT_EXEC;
    result=mprotect((void *)range.start, (size_t)range.end-(size_t)range.start,protect);
#else /* WIN32 */
    DWORD newprotect=0;
    if(trgrange.read && !trgrange.write && !trgrange.execute)newprotect=PAGE_READONLY;
    if(trgrange.read && trgrange.write && !trgrange.execute)newprotect=PAGE_READWRITE;
    if(!trgrange.read && !trgrange.write && trgrange.execute)newprotect=PAGE_EXECUTE;
    if(trgrange.read && !trgrange.write && trgrange.execute)newprotect=PAGE_EXECUTE_READ;
    if(trgrange.read && trgrange.write && trgrange.execute)newprotect=PAGE_EXECUTE_READWRITE;
    DWORD oldprotect=0;
    bool result;
    result=VirtualProtect((LPVOID)range.start,(char *)range.end-(char *)range.start,newprotect,&oldprotect);
#endif /* WIN32 */

#ifndef WIN32
    return result==0;
#else /* WIN32 */
    return result;
#endif /* WIN32 */
}

bool Process::flushCache(const void* target, size_t count)
{
#ifndef WIN32
#ifndef _DARWIN
    __builtin___clear_cache((char*)target, (char*)target + count - 1);
    return true; /* assume always succeeds, as the builtin has no return type */
#else /* _DARWIN */
    // FIXME: implement cache flush for MacOS (???)
    return false;
#endif /* _DARWIN */
#else /* WIN32 */
    return 0 != FlushInstructionCache(d->my_handle, (LPCVOID)target, count);
#endif /* WIN32 */
}

#ifndef WIN32
// returns -1 on error
#endif /* ! WIN32 */
void* Process::memAlloc(const int length)
{
#ifndef WIN32
#ifndef _DARWIN
    return mmap(0, length, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
#else /* _DARWIN */
    return mmap(0, length, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
#endif /* _DARWIN */
#else /* WIN32 */
    void *ret;
    // returns 0 on error
    ret = VirtualAlloc(0, length, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    if (!ret)
        ret = (void*)-1;
    return ret;
#endif /* WIN32 */
}

int Process::memDealloc(void *ptr, const int length)
{
#ifndef WIN32
    return munmap(ptr, length);
#else /* WIN32 */
    // can only free the whole region at once
    // vfree returns 0 on error
    return !VirtualFree(ptr, 0, MEM_RELEASE);
#endif /* WIN32 */
}

int Process::memProtect(void *ptr, const int length, const int prot)
{
    int prot_native = 0;

#ifndef WIN32
    if (prot & Process::MemProt::READ)
        prot_native |= PROT_READ;
    if (prot & Process::MemProt::WRITE)
        prot_native |= PROT_WRITE;
    if (prot & Process::MemProt::EXEC)
        prot_native |= PROT_EXEC;
#else /* WIN32 */
    // only support a few constant combinations
    if (prot == 0)
        prot_native = PAGE_NOACCESS;
    else if (prot == Process::MemProt::READ)
        prot_native = PAGE_READONLY;
    else if (prot == (Process::MemProt::READ | Process::MemProt::WRITE))
        prot_native = PAGE_READWRITE;
    else if (prot == (Process::MemProt::READ | Process::MemProt::WRITE | Process::MemProt::EXEC))
        prot_native = PAGE_EXECUTE_READWRITE;
    else if (prot == (Process::MemProt::READ | Process::MemProt::EXEC))
        prot_native = PAGE_EXECUTE_READ;
    else
        return -1;
#endif /* WIN32 */

#ifndef WIN32
    return mprotect(ptr, length, prot_native);
#else /* WIN32 */
    DWORD old_prot = 0;
    return !VirtualProtect(ptr, length, prot_native, &old_prot);
#endif /* WIN32 */
}
