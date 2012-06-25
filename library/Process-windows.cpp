/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

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

#define _WIN32_WINNT 0x0501 // needed for INPUT struct
#define WINVER 0x0501       // OpenThread(), PSAPI, Toolhelp32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

typedef LONG NTSTATUS;
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

// FIXME: it is uncertain how these map to 64bit
typedef struct _DEBUG_BUFFER
{
    HANDLE SectionHandle;
    PVOID  SectionBase;
    PVOID  RemoteSectionBase;
    ULONG  SectionBaseDelta;
    HANDLE  EventPairHandle;
    ULONG  Unknown[2];
    HANDLE  RemoteThreadHandle;
    ULONG  InfoClassMask;
    ULONG  SizeOfInfo;
    ULONG  AllocatedSize;
    ULONG  SectionSize;
    PVOID  ModuleInformation;
    PVOID  BackTraceInformation;
    PVOID  HeapInformation;
    PVOID  LockInformation;
    PVOID  Reserved[8];
} DEBUG_BUFFER, *PDEBUG_BUFFER;

typedef struct _DEBUG_HEAP_INFORMATION
{
    ULONG Base; // 0×00
    ULONG Flags; // 0×04
    USHORT Granularity; // 0×08
    USHORT Unknown; // 0x0A
    ULONG Allocated; // 0x0C
    ULONG Committed; // 0×10
    ULONG TagCount; // 0×14
    ULONG BlockCount; // 0×18
    ULONG Reserved[7]; // 0x1C
    PVOID Tags; // 0×38
    PVOID Blocks; // 0x3C
} DEBUG_HEAP_INFORMATION, *PDEBUG_HEAP_INFORMATION;

// RtlQueryProcessDebugInformation.DebugInfoClassMask constants
#define PDI_MODULES                       0x01
#define PDI_BACKTRACE                     0x02
#define PDI_HEAPS                         0x04
#define PDI_HEAP_TAGS                     0x08
#define PDI_HEAP_BLOCKS                   0x10
#define PDI_LOCKS                         0x20

extern "C" __declspec(dllimport) NTSTATUS __stdcall RtlQueryProcessDebugInformation( IN ULONG  ProcessId, IN ULONG  DebugInfoClassMask, IN OUT PDEBUG_BUFFER  DebugBuffer);
extern "C" __declspec(dllimport) PDEBUG_BUFFER __stdcall RtlCreateQueryDebugBuffer( IN ULONG  Size, IN BOOLEAN  EventPair);
extern "C" __declspec(dllimport) NTSTATUS __stdcall RtlDestroyQueryDebugBuffer( IN PDEBUG_BUFFER  DebugBuffer);

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
        vector <HANDLE> threads;
        vector <HANDLE> stoppedthreads;
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
        uint32_t pe_offset = Process::readDWord(d->base+0x3C);
        read(d->base + pe_offset, sizeof(d->pe_header), (uint8_t *)&(d->pe_header));
        const size_t sectionsSize = sizeof(IMAGE_SECTION_HEADER) * d->pe_header.FileHeader.NumberOfSections;
        d->sections = (IMAGE_SECTION_HEADER *) malloc(sectionsSize);
        read(d->base + pe_offset + sizeof(d->pe_header), sectionsSize, (uint8_t *)(d->sections));
    }
    catch (exception &)
    {
        return;
    }
    VersionInfo* vinfo = factory->getVersionInfoByPETimestamp(d->pe_header.FileHeader.TimeDateStamp);
    if(vinfo)
    {
        vector<uint32_t> threads_ids;
        if(!getThreadIDs( threads_ids ))
        {
            // thread enumeration failed.
            return;
        }
        identified = true;
        // give the process a data model and memory layout fixed for the base of first module
        my_descriptor  = new VersionInfo(*vinfo);
        my_descriptor->rebaseTo((uint32_t)d->base);
        for(size_t i = 0; i < threads_ids.size();i++)
        {
            HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, (DWORD) threads_ids[i]);
            if(hThread)
                d->threads.push_back(hThread);
            else
                cerr << "Unable to open thread :" << hex << (DWORD) threads_ids[i] << endl;
        }
    }
}

Process::~Process()
{
    // destroy our rebased copy of the memory descriptor
    delete my_descriptor;
    for(size_t i = 0; i < d->threads.size(); i++)
        CloseHandle(d->threads[i]);
    if(d->sections != NULL)
        free(d->sections);
}

bool Process::getThreadIDs(vector<uint32_t> & threads )
{
    HANDLE AllThreads = INVALID_HANDLE_VALUE;
    THREADENTRY32 te32;

    AllThreads = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 );
    if( AllThreads == INVALID_HANDLE_VALUE )
    {
        return false;
    }
    te32.dwSize = sizeof(THREADENTRY32 );

    if( !Thread32First( AllThreads, &te32 ) )
    {
        CloseHandle( AllThreads );
        return false;
    }

    do
    {
        if( te32.th32OwnerProcessID == d->my_pid )
        {
            threads.push_back(te32.th32ThreadID);
        }
    } while( Thread32Next(AllThreads, &te32 ) );

    CloseHandle( AllThreads );
    return true;
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

uint32_t Process::getBase()
{
    if(d)
        return (uint32_t) d->base;
    return 0x400000;
}

string Process::doReadClassName (void * vptr)
{
    char * rtti = readPtr((char *)vptr - 0x4);
    char * typeinfo = readPtr(rtti + 0xC);
    string raw = readCString(typeinfo + 0xC); // skips the .?AV
    raw.resize(raw.length() - 2);// trim @@ from end
    return raw;
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
