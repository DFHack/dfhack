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

#define _WIN32_WINNT 0x0501
#define WINVER 0x0501

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <map>
using namespace std;

#include "VersionInfo.h"
#include "VersionInfoFactory.h"
#include "Error.h"
#include "MemAccess.h"
using namespace DFHack;
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
Process::Process(VersionInfoFactory * factory)
{
    HMODULE hmod = NULL;
    DWORD needed;
    bool found = false;
    identified = false;
    my_descriptor = NULL;

    d = new PlatformSpecific();
    // open process
    d->my_pid = GetCurrentProcessId();
    d->my_handle = GetCurrentProcess();
    // try getting the first module of the process
    if(EnumProcessModules(d->my_handle, &hmod, sizeof(hmod), &needed) == 0)
    {
        return; //if enumprocessModules fails, give up
    }

    // got base ;)
    d->base = (char *)hmod;

    // read from this process
    try
    {
        uint32_t pe_offset = readDWord(d->base+0x3C);
        read(d->base + pe_offset, sizeof(d->pe_header), (uint8_t *)&(d->pe_header));
        const size_t sectionsSize = sizeof(IMAGE_SECTION_HEADER) * d->pe_header.FileHeader.NumberOfSections;
        d->sections = (IMAGE_SECTION_HEADER *) malloc(sectionsSize);
        read(d->base + pe_offset + sizeof(d->pe_header), sectionsSize, (uint8_t *)(d->sections));
    }
    catch (exception &)
    {
        return;
    }
    my_pe = d->pe_header.FileHeader.TimeDateStamp;
    VersionInfo* vinfo = factory->getVersionInfoByPETimestamp(my_pe);
    if(vinfo)
    {
        identified = true;
        // give the process a data model and memory layout fixed for the base of first module
        my_descriptor  = new VersionInfo(*vinfo);
        my_descriptor->rebaseTo(getBase());
    }
    else
    {
        fprintf(stderr, "Unable to retrieve version information.\nPE timestamp: 0x%x\n", my_pe);
        fflush(stderr);
    }
}

Process::~Process()
{
    // destroy our rebased copy of the memory descriptor
    delete my_descriptor;
    if(d->sections != NULL)
        free(d->sections);
}

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

static void GetDosNames(std::map<string, string> &table)
{
    // Partially based on example from msdn:
    // Translate path with device name to drive letters.
    TCHAR szTemp[512];
    szTemp[0] = '\0';

    if (GetLogicalDriveStrings(sizeof(szTemp)-1, szTemp))
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

void Process::getMemRanges( vector<t_memrange> & ranges )
{
    MEMORY_BASIC_INFORMATION MBI;
    //map<char *, unsigned int> heaps;
    uint64_t movingStart = 0;
    PVOID LastAllocationBase = 0;
    map <char *, string> nameMap;
    map <string,string> dosDrives;

    // get page size
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    uint64_t PageSize = si.dwPageSize;

    // get dos drive names
    GetDosNames(dosDrives);

    ranges.clear();

    // enumerate heaps
    // HeapNodes(d->my_pid, heaps);
    // go through all the VM regions, convert them to our internal format
    while (VirtualQueryEx(d->my_handle, (const void*) (movingStart), &MBI, sizeof(MBI)) == sizeof(MBI))
    {
        movingStart = ((uint64_t)MBI.BaseAddress + MBI.RegionSize);
        if(movingStart % PageSize != 0)
            movingStart = (movingStart / PageSize + 1) * PageSize;

        // Skip unallocated address space
        if (MBI.State & MEM_FREE)
            continue;

        // Find range and permissions
        t_memrange temp;
        memset(&temp, 0, sizeof(temp));

        temp.start   = (char *) MBI.BaseAddress;
        temp.end     =  ((char *)MBI.BaseAddress + (uint64_t)MBI.RegionSize);
        temp.valid   = true;

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
            auto &last = ranges.back();

            if (last.end == temp.start &&
                last.valid == temp.valid && last.execute == temp.execute &&
                last.read == temp.read && last.write == temp.write)
            {
                last.end = temp.end;
                continue;
            }
        }

#if 1
        // Find the mapped file name
        if (GetMappedFileName(d->my_handle, temp.start, temp.name, 1024))
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
#else
        // Find the executable name
        char *base = (char*)MBI.AllocationBase;

        if(nameMap.count(base))
        {
            strncpy(temp.name, nameMap[base].c_str(), 1023);
        }
        else if(GetModuleBaseName(d->my_handle, (HMODULE)base, temp.name, 1024))
        {
            std::string nm(temp.name);

            nameMap[base] = nm;

            // this is our executable! (could be generalized to pull segments from libs, but whatever)
            if(d->base == base)
            {
                for(int i = 0; i < d->pe_header.FileHeader.NumberOfSections; i++)
                {
                    /*char sectionName[9];
                    memcpy(sectionName,d->sections[i].Name,8);
                    sectionName[8] = 0;
                    string nm;
                    nm.append(temp.name);
                    nm.append(" : ");
                    nm.append(sectionName);*/
                    nameMap[base + d->sections[i].VirtualAddress] = nm;
                }
            }
        }
        else
            temp.name[0] = 0;
#endif

        // Push the entry
        LastAllocationBase = MBI.AllocationBase;
        ranges.push_back(temp);
    }
}

uintptr_t Process::getBase()
{
    if(d)
        return (uintptr_t) d->base;
    return 0x400000;
}

int Process::adjustOffset(int offset, bool to_file)
{
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
}


string Process::doReadClassName (void * vptr)
{
    char * rtti = readPtr((char *)vptr - 0x4);
    char * typeinfo = readPtr(rtti + 0xC);
    string raw = readCString(typeinfo + 0xC); // skips the .?AV
    raw.resize(raw.length() - 2);// trim @@ from end
    return raw;
}

uint32_t Process::getTickCount()
{
    return GetTickCount();
}

string Process::getPath()
{
    HMODULE hmod;
    DWORD junk;
    char String[255];
    EnumProcessModules(d->my_handle, &hmod, 1 * sizeof(HMODULE), &junk); //get the module from the handle
    GetModuleFileNameEx(d->my_handle,hmod,String,sizeof(String)); //get the filename from the module
    string out(String);
    return(out.substr(0,out.find_last_of("\\")));
}

bool Process::setPermisions(const t_memrange & range,const t_memrange &trgrange)
{
    DWORD newprotect=0;
    if(trgrange.read && !trgrange.write && !trgrange.execute)newprotect=PAGE_READONLY;
    if(trgrange.read && trgrange.write && !trgrange.execute)newprotect=PAGE_READWRITE;
    if(!trgrange.read && !trgrange.write && trgrange.execute)newprotect=PAGE_EXECUTE;
    if(trgrange.read && !trgrange.write && trgrange.execute)newprotect=PAGE_EXECUTE_READ;
    if(trgrange.read && trgrange.write && trgrange.execute)newprotect=PAGE_EXECUTE_READWRITE;
    DWORD oldprotect=0;
    bool result;
    result=VirtualProtect((LPVOID)range.start,(char *)range.end-(char *)range.start,newprotect,&oldprotect);

    return result;
}

void* Process::memAlloc(const int length)
{
    void *ret;
    // returns 0 on error
    ret = VirtualAlloc(0, length, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    if (!ret)
        ret = (void*)-1;
    return ret;
}

int Process::memDealloc(void *ptr, const int length)
{
    // can only free the whole region at once
    // vfree returns 0 on error
    return !VirtualFree(ptr, 0, MEM_RELEASE);
}

int Process::memProtect(void *ptr, const int length, const int prot)
{
    int prot_native = 0;
    DWORD old_prot = 0;

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

    return !VirtualProtect(ptr, length, prot_native, &old_prot);
}
