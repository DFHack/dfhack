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
#include "WindowsProcess.h"
#include "ProcessFactory.h"
#include "MicrosoftSTL.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/DFError.h"
#include <string.h>
using namespace DFHack;

namespace
{
    class NormalProcess : public Process
    {
        private:
            VersionInfo * my_descriptor;
            HANDLE my_handle;
            HANDLE my_main_thread;
            uint32_t my_pid;
            string memFile;
            bool attached;
            bool suspended;
            bool identified;
            IMAGE_NT_HEADERS pe_header;
            IMAGE_SECTION_HEADER * sections;
            uint32_t base;
            MicrosoftSTL stl;
        public:
            NormalProcess(uint32_t pid, std::vector <VersionInfo *> & known_versions);
            ~NormalProcess();
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
            VersionInfo *getDescriptor();
            int getPID();
            std::string getPath();
            // get module index by name and version. bool 1 = error
            bool getModuleIndex (const char * name, const uint32_t version, uint32_t & OUTPUT) { OUTPUT=0; return false;};
            // get the SHM start if available
            char * getSHMStart (void){return 0;};
            // set a SHM command and wait for a response
            bool SetAndWait (uint32_t state){return false;};
    };

}

Process* DFHack::createNormalProcess(uint32_t pid, vector <VersionInfo *> & known_versions)
{
    return new NormalProcess(pid, known_versions);
}

NormalProcess::NormalProcess(uint32_t pid, vector <VersionInfo *> & known_versions)
: my_pid(pid)
{
    my_descriptor = NULL;
    my_main_thread = NULL;
    attached = false;
    suspended = false;
    base = 0;
    sections = 0;

    HMODULE hmod = NULL;
    DWORD needed;
    bool found = false;

    identified = false;
    // open process
    my_handle = OpenProcess( PROCESS_ALL_ACCESS, FALSE, my_pid );
    if (NULL == my_handle)
        return;

    // try getting the first module of the process
    if(EnumProcessModules(my_handle, &hmod, sizeof(hmod), &needed) == 0)
    {
        CloseHandle(my_handle);
        // cout << "EnumProcessModules fail'd" << endl;
        return; //if enumprocessModules fails, give up
    }

    // got base ;)
    base = (uint32_t)hmod;

    my_main_thread = 0;
    // read from this process
    try
    {
        uint32_t pe_offset = Process::readDWord(base+0x3C);
        read(base + pe_offset                       , sizeof(pe_header), (uint8_t *)&pe_header);
        const size_t sectionsSize = sizeof(IMAGE_SECTION_HEADER) * pe_header.FileHeader.NumberOfSections;
        sections = (IMAGE_SECTION_HEADER *) malloc(sectionsSize);
        read(base + pe_offset + sizeof(pe_header), sectionsSize, (uint8_t *)sections);
        my_handle = 0;
    }
    catch (exception &)
    {
        CloseHandle(my_handle);
        my_handle = 0;
        return;
    }

    // see if there's a version entry that matches this process
    vector<VersionInfo*>::iterator it;
    for ( it=known_versions.begin() ; it < known_versions.end(); it++ )
    {
        // filter by OS
        if(OS_WINDOWS != (*it)->getOS())
            continue;
        uint32_t pe_timestamp;
        // filter by timestamp, skip entries without a timestamp
        try
        {
            pe_timestamp = (*it)->getPE();
        }
        catch(Error::AllMemdef&)
        {
            continue;
        }
        if (pe_timestamp != pe_header.FileHeader.TimeDateStamp)
            continue;

        // all went well
        {
            printf("Match found! Using version %s.\n", (*it)->getVersion().c_str());
            identified = true;
            // give the process a data model and memory layout fixed for the base of first module
            my_descriptor  = new VersionInfo(**it);
            my_descriptor->RebaseAll(base);
            // keep track of created memory_info object so we can destroy it later
            my_descriptor->setParentProcess(this);
            // process is responsible for destroying its data model
            my_handle = my_handle;
            identified = true;

            // TODO: detect errors in thread enumeration
            vector<uint32_t> threads;
            getThreadIDs( threads );
            my_main_thread = OpenThread(THREAD_ALL_ACCESS, FALSE, (DWORD) threads[0]);
            stl.init(this);
            found = true;
            break; // break the iterator loop
        }
    }
    // close handle of processes that aren't DF
    if(!found)
    {
        CloseHandle(my_handle);
    }
}
/*
*/

NormalProcess::~NormalProcess()
{
    if(attached)
    {
        detach();
    }
    // destroy our rebased copy of the memory descriptor
    delete my_descriptor;
    if(my_handle != NULL)
    {
        CloseHandle(my_handle);
    }
    if(my_main_thread != NULL)
    {
        CloseHandle(my_main_thread);
    }
    if(sections != NULL)
        free(sections);
    delete d;
}

VersionInfo * NormalProcess::getDescriptor()
{
    return my_descriptor;
}

int NormalProcess::getPID()
{
    return my_pid;
}

bool NormalProcess::isSuspended()
{
    return suspended;
}
bool NormalProcess::isAttached()
{
    return attached;
}

bool NormalProcess::isIdentified()
{
    return identified;
}

bool NormalProcess::asyncSuspend()
{
    return suspend();
}

bool NormalProcess::suspend()
{
    if(!attached)
        return false;
    if(suspended)
    {
        return true;
    }
    SuspendThread(my_main_thread);
    suspended = true;
    return true;
}

bool NormalProcess::forceresume()
{
    if(!attached)
        return false;
    while (ResumeThread(my_main_thread) > 1);
    suspended = false;
    return true;
}


bool NormalProcess::resume()
{
    if(!attached)
        return false;
    if(!suspended)
    {
        return true;
    }
    ResumeThread(my_main_thread);
    suspended = false;
    return true;
}

bool NormalProcess::attach()
{
    if(attached)
    {
        if(!suspended)
            return suspend();
        return true;
    }
    attached = true;
    suspend();

    return true;
}


bool NormalProcess::detach()
{
    if(!attached) return true;
    resume();
    attached = false;
    return true;
}

bool NormalProcess::getThreadIDs(vector<uint32_t> & threads )
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
        if( te32.th32OwnerProcessID == my_pid )
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
void HeapNodes(DWORD pid, map<uint64_t, unsigned int> & heaps)
{
    // Create debug buffer
    PDEBUG_BUFFER db = RtlCreateQueryDebugBuffer(0, FALSE); 
    // Get process heap data
    RtlQueryProcessDebugInformation( pid, PDI_HEAPS/* | PDI_HEAP_BLOCKS*/, db);
    ULONG heapNodeCount = db->HeapInformation ? *PULONG(db->HeapInformation):0;
    PDEBUG_HEAP_INFORMATION heapInfo = PDEBUG_HEAP_INFORMATION(PULONG(db-> HeapInformation) + 1);
    // Go through each of the heap nodes and dispaly the information
    for (unsigned int i = 0; i < heapNodeCount; i++) 
    {
        heaps[heapInfo[i].Base] = i;
    }
    // Clean up the buffer
    RtlDestroyQueryDebugBuffer( db );
}

// FIXME: NEEDS TESTING!
void NormalProcess::getMemRanges( vector<t_memrange> & ranges )
{
    MEMORY_BASIC_INFORMATION MBI;
    map<uint64_t, unsigned int> heaps;
    uint64_t movingStart = 0;
    map <uint64_t, string> nameMap;

    // get page size
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    uint64_t PageSize = si.dwPageSize;
    // enumerate heaps
    HeapNodes(my_pid, heaps);
    // go through all the VM regions, convert them to our internal format
    while (VirtualQueryEx(this->my_handle, (const void*) (movingStart), &MBI, sizeof(MBI)) == sizeof(MBI))
    {
        movingStart = ((uint64_t)MBI.BaseAddress + MBI.RegionSize);
        if(movingStart % PageSize != 0)
            movingStart = (movingStart / PageSize + 1) * PageSize;
        // skip empty regions and regions we share with other processes (DLLs)
        if( !(MBI.State & MEM_COMMIT) /*|| !(MBI.Type & MEM_PRIVATE)*/ )
            continue;
        t_memrange temp;
        temp.start   = (uint64_t) MBI.BaseAddress;
        temp.end     =  ((uint64_t)MBI.BaseAddress + (uint64_t)MBI.RegionSize);
        temp.read    = MBI.Protect & PAGE_EXECUTE_READ || MBI.Protect & PAGE_EXECUTE_READWRITE || MBI.Protect & PAGE_READONLY || MBI.Protect & PAGE_READWRITE;
        temp.write   = MBI.Protect & PAGE_EXECUTE_READWRITE || MBI.Protect & PAGE_READWRITE;
        temp.execute = MBI.Protect & PAGE_EXECUTE_READ || MBI.Protect & PAGE_EXECUTE_READWRITE || MBI.Protect & PAGE_EXECUTE;
        temp.valid = true;
        if(!GetModuleBaseName(this->my_handle, (HMODULE) temp.start, temp.name, 1024))
        {
            if(nameMap.count(temp.start))
            {
                // potential buffer overflow...
                strcpy(temp.name, nameMap[temp.start].c_str());
            }
            else
            {
                // filter away shared segments without a name.
                if( !(MBI.Type & MEM_PRIVATE) )
                    continue;
                else
                {
                    // could be a heap?
                    if(heaps.count(temp.start))
                    {
                        sprintf(temp.name,"HEAP %d",heaps[temp.start]);
                    }
                    else temp.name[0]=0;
                }
                

                
            }
        }
        else
        {
            // this is our executable! (could be generalized to pull segments from libs, but whatever)
            if(base == temp.start)
            {
                for(int i = 0; i < pe_header.FileHeader.NumberOfSections; i++)
                {
                    char sectionName[9];
                    memcpy(sectionName,sections[i].Name,8);
                    sectionName[8] = 0;
                    string nm;
                    nm.append(temp.name);
                    nm.append(" : ");
                    nm.append(sectionName);
                    nameMap[temp.start + sections[i].VirtualAddress] = nm;
                }
            }
            else
                continue;
        }
        ranges.push_back(temp);
    }
}

void NormalProcess::readByte (const uint32_t offset,uint8_t &result)
{
    if(!ReadProcessMemory(my_handle, (int*) offset, &result, sizeof(uint8_t), NULL))
        throw Error::MemoryAccessDenied();
}

void NormalProcess::readWord (const uint32_t offset, uint16_t &result)
{
    if(!ReadProcessMemory(my_handle, (int*) offset, &result, sizeof(uint16_t), NULL))
        throw Error::MemoryAccessDenied();
}

void NormalProcess::readDWord (const uint32_t offset, uint32_t &result)
{
    if(!ReadProcessMemory(my_handle, (int*) offset, &result, sizeof(uint32_t), NULL))
        throw Error::MemoryAccessDenied();
}

void NormalProcess::readQuad (const uint32_t offset, uint64_t &result)
{
    if(!ReadProcessMemory(my_handle, (int*) offset, &result, sizeof(uint64_t), NULL))
        throw Error::MemoryAccessDenied();
}

void NormalProcess::readFloat (const uint32_t offset, float &result)
{
    if(!ReadProcessMemory(my_handle, (int*) offset, &result, sizeof(float), NULL))
        throw Error::MemoryAccessDenied();
}

void NormalProcess::read (const uint32_t offset, uint32_t size, uint8_t *target)
{
    if(!ReadProcessMemory(my_handle, (int*) offset, target, size, NULL))
        throw Error::MemoryAccessDenied();
}

// WRITING
void NormalProcess::writeQuad (const uint32_t offset, uint64_t data)
{
    if(!WriteProcessMemory(my_handle, (int*) offset, &data, sizeof(data), NULL))
        throw Error::MemoryAccessDenied();
}

void NormalProcess::writeDWord (const uint32_t offset, uint32_t data)
{
    if(!WriteProcessMemory(my_handle, (int*) offset, &data, sizeof(data), NULL))
        throw Error::MemoryAccessDenied();
}

// using these is expensive.
void NormalProcess::writeWord (uint32_t offset, uint16_t data)
{
    if(!WriteProcessMemory(my_handle, (int*) offset, &data, sizeof(data), NULL))
        throw Error::MemoryAccessDenied();
}

void NormalProcess::writeByte (uint32_t offset, uint8_t data)
{
    if(!WriteProcessMemory(my_handle, (int*) offset, &data, sizeof(data), NULL))
        throw Error::MemoryAccessDenied();
}

void NormalProcess::write (uint32_t offset, uint32_t size, uint8_t *source)
{
    if(!WriteProcessMemory(my_handle, (int*) offset, source, size, NULL))
        throw Error::MemoryAccessDenied();
}

///FIXME: reduce use of temporary objects
const string NormalProcess::readCString (const uint32_t offset)
{
    string temp;
    char temp_c[256];
    SIZE_T read;
    if(!ReadProcessMemory(my_handle, (int *) offset, temp_c, 254, &read))
        throw Error::MemoryAccessDenied();
    // needs to be 254+1 byte for the null term
    temp_c[read+1] = 0;
    temp.assign(temp_c);
    return temp;
}

size_t NormalProcess::readSTLString (uint32_t offset, char * buffer, size_t bufcapacity)
{
    return stl.readSTLString(offset, buffer, bufcapacity);
}

const string NormalProcess::readSTLString (uint32_t offset)
{
    return stl.readSTLString(offset);
}

string NormalProcess::readClassName (uint32_t vptr)
{
    stl.readClassName(vptr);
}

string NormalProcess::getPath()
{
    HMODULE hmod;
    DWORD junk;
    char String[255];
    EnumProcessModules(my_handle, &hmod, 1 * sizeof(HMODULE), &junk); //get the module from the handle
    GetModuleFileNameEx(my_handle,hmod,String,sizeof(String)); //get the filename from the module
    string out(String);
    return(out.substr(0,out.find_last_of("\\")));
}
