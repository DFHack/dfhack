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
#include "DFCommonInternal.h"
#include "../shmserver/shms.h"
#include "../shmserver/mod-core.h"
using namespace DFHack;

// a full memory barrier! better be safe than sorry.
class Process::Private
{
    public:
    Private()
    {
        my_descriptor = NULL;
        my_pid = 0;
        my_shm = 0;
        my_window = NULL;
        attached = false;
        suspended = false;
        identified = false;
        useYield = 0;
        DFSVMutex = 0;
        DFCLMutex = 0;
    };
    ~Private(){};
    memory_info * my_descriptor;
    DFWindow * my_window;
    uint32_t my_pid;
    char *my_shm;
    HANDLE DFSVMutex;
    HANDLE DFCLMutex;
    
    bool attached;
    bool suspended;
    bool identified;
    bool useYield;
    
    bool waitWhile (uint32_t state);
    bool isValidSV();
    bool Aux_Core_Attach(bool & versionOK, uint32_t & PID);
};

// some helpful macros to keep the code bloat in check
#define SHMCMD ((shm_cmd *)my_shm)->pingpong
#define D_SHMCMD ((shm_cmd *)d->my_shm)->pingpong

#define SHMHDR ((shm_core_hdr *)my_shm)
#define D_SHMHDR ((shm_core_hdr *)d->my_shm)

#define SHMDATA(type) ((type *)(my_shm + SHM_HEADER))
#define D_SHMDATA(type) ((type *)(d->my_shm + SHM_HEADER))

// is the other side still there?
bool Process::Private::isValidSV()
{
    // try if CL mutex is free
    uint32_t result = WaitForSingleObject(DFSVMutex,0);
    
    switch (result)
    {
        case WAIT_ABANDONED:
        case WAIT_OBJECT_0:
        {
            ReleaseMutex(DFSVMutex);
            return false;
        }
        case WAIT_TIMEOUT:
        {
            // mutex is held by DF
            return true;
        }   
        default:
        case WAIT_FAILED:
        {
            // TODO: now how do I respond to this?
            return false;
        }
    }
}

bool Process::waitWhile (uint32_t state)
{
    return d->waitWhile(state);
}

bool Process::Private::waitWhile (uint32_t state)
{
    uint32_t cnt = 0;
    while (SHMCMD == state)
    {
        // yield the CPU, only on single-core CPUs
        if(useYield)
        {
            SCHED_YIELD
        }
        if(cnt == 10000)
        {
            if(!isValidSV())// DF not there anymore?
            {
                attached = suspended = false;
                ReleaseMutex(DFCLMutex);
                UnmapViewOfFile(my_shm);
                throw Error::SHMServerDisappeared();
                return false;
            }
            else
            {
                cnt = 0;
            }
        }
        cnt++;
    }
    if(SHMCMD == CORE_ERROR)
    {
        SHMCMD = CORE_RUNNING;
        attached = suspended = false;
        cerr << "shm server error!" << endl;
        return false;
    }
    return true;
}

uint32_t OS_getAffinity()
{
    HANDLE hProcess = GetCurrentProcess();
    DWORD dwProcessAffinityMask, dwSystemAffinityMask;
    GetProcessAffinityMask( hProcess, &dwProcessAffinityMask, &dwSystemAffinityMask );
    return dwProcessAffinityMask;
}

bool Process::Private::Aux_Core_Attach(bool & versionOK, uint32_t & PID)
{
    SHMDATA(coreattach)->cl_affinity = OS_getAffinity();
    full_barrier
    SHMCMD = CORE_ATTACH;
    if(!waitWhile(CORE_ATTACH))
        return false;
    full_barrier
    versionOK =( SHMDATA(coreattach)->sv_version == CORE_VERSION );
    PID = SHMDATA(coreattach)->sv_PID;
    useYield = SHMDATA(coreattach)->sv_useYield;
    #ifdef DEBUG
        if(useYield) cerr << "Using Yield!" << endl;
    #endif
    return true;
}

Process::Process(uint32_t PID, vector <memory_info *> & known_versions)
: d(new Private())
{
    char svmutexname [256];
    sprintf(svmutexname,"DFSVMutex-%d",PID);
    char clmutexname [256];
    sprintf(clmutexname,"DFCLMutex-%d",PID);
    
    // get server and client mutex
    d->DFSVMutex = OpenMutex(SYNCHRONIZE,false, svmutexname);
    if(d->DFSVMutex == 0)
    {
        return;
    }
    d->DFCLMutex = OpenMutex(SYNCHRONIZE,false, clmutexname);
    if(d->DFCLMutex == 0)
    {
        return;
    }
    d->my_pid = PID;
    
    // attach the SHM
    if(!attach())
    {
        return;
    }
    
    // Test bridge version, get PID, sync Yield
    bool bridgeOK;
    bool error = 0;
    if(!d->Aux_Core_Attach(bridgeOK,d->my_pid))
    {
        fprintf(stderr,"DF terminated during reading\n");
        error = 1;
    }
    else if(!bridgeOK)
    {
        fprintf(stderr,"SHM bridge version mismatch\n");
        error = 1;
    }
    if(error)
    {
        D_SHMCMD = CORE_RUNNING;
        UnmapViewOfFile(d->my_shm);
        ReleaseMutex(d->DFCLMutex);
        CloseHandle(d->DFSVMutex);
        d->DFSVMutex = 0;
        CloseHandle(d->DFCLMutex);
        d->DFCLMutex = 0;
        return;
    }

    // try to identify the DF version
    do // glorified goto
    {  
        IMAGE_NT_HEADERS32 pe_header;
        IMAGE_SECTION_HEADER sections[16];
        HMODULE hmod = NULL;
        DWORD junk;
        HANDLE hProcess;
        bool found = false;
        d->identified = false;
        // open process, we only need the process open 
        hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, d->my_pid );
        if (NULL == hProcess)
            break;
        
        // try getting the first module of the process
        if(EnumProcessModules(hProcess, &hmod, 1 * sizeof(HMODULE), &junk) == 0)
        {
            CloseHandle(hProcess);
            cout << "EnumProcessModules fail'd" << endl;
            break;
        }
        // got base ;)
        uint32_t base = (uint32_t)hmod;
        
        // read from this process
        uint32_t pe_offset = readDWord(base+0x3C);
        read(base + pe_offset                   , sizeof(pe_header), (uint8_t *)&pe_header);
        read(base + pe_offset+ sizeof(pe_header), sizeof(sections) , (uint8_t *)&sections );
        
        // iterate over the list of memory locations
        vector<memory_info *>::iterator it;
        for ( it=known_versions.begin() ; it < known_versions.end(); it++ )
        {
            uint32_t pe_timestamp;
            try
            {
                pe_timestamp = (*it)->getHexValue("pe_timestamp");
            }
            catch(Error::MissingMemoryDefinition& e)
            {
                continue;
            }
            if (pe_timestamp == pe_header.FileHeader.TimeDateStamp)
            {
                memory_info *m = new memory_info(**it);
                m->RebaseAll(base);
                d->my_descriptor = m;
                d->identified = true;
                cerr << "identified " << m->getVersion() << endl;
                break;
            }
        }
        CloseHandle(hProcess);
    } while (0); // glorified goto end
    
    if(d->identified)
    {
        d->my_window = new DFWindow(this);
    }
    else
    {
        D_SHMCMD = CORE_RUNNING;
        UnmapViewOfFile(d->my_shm);
        d->my_shm = 0;
        ReleaseMutex(d->DFCLMutex);
        CloseHandle(d->DFSVMutex);
        d->DFSVMutex = 0;
        CloseHandle(d->DFCLMutex);
        d->DFCLMutex = 0;
        return;
    }
    full_barrier
    // at this point, DF is attached and suspended, make it run
    detach();
}

bool Process::isSuspended()
{
    return d->suspended;
}
bool Process::isAttached()
{
    return d->attached;
}

bool Process::isIdentified()
{
    return d->identified;
}

Process::~Process()
{
    if(d->attached)
    {
        detach();
    }
    // destroy data model. this is assigned by processmanager
    if(d->my_descriptor)
    {
        delete d->my_descriptor;
    }
    if(d->my_window)
    {
        delete d->my_window;
    }
    // release mutex handles we have
    if(d->DFCLMutex)
    {
        CloseHandle(d->DFCLMutex);
    }
    if(d->DFSVMutex)
    {
        CloseHandle(d->DFSVMutex);
    }
    delete d;
}

memory_info * Process::getDescriptor()
{
    return d->my_descriptor;
}

DFWindow * Process::getWindow()
{
    return d->my_window;
}

int Process::getPID()
{
    return d->my_pid;
}

//FIXME: implement
bool Process::getThreadIDs(vector<uint32_t> & threads )
{
    return false;
}

//FIXME: cross-reference with ELF segment entries?
void Process::getMemRanges( vector<t_memrange> & ranges )
{
    char buffer[1024];
    char permissions[5]; // r/-, w/-, x/-, p/s, 0
    
    sprintf(buffer, "/proc/%lu/maps", d->my_pid);
    FILE *mapFile = ::fopen(buffer, "r");
    uint64_t offset, device1, device2, node;
    
    while (fgets(buffer, 1024, mapFile))
    {
        t_memrange temp;
        temp.name[0] = 0;
        sscanf(buffer, "%llx-%llx %s %llx %2llu:%2llu %llu %s",
               &temp.start,
               &temp.end,
               (char*)&permissions,
               &offset, &device1, &device2, &node,
               (char*)&temp.name);
        temp.read = permissions[0] == 'r';
        temp.write = permissions[1] == 'w';
        temp.execute = permissions[2] == 'x';
        ranges.push_back(temp);
    }
}

bool Process::suspend()
{
    if(!d->attached)
    {
        cerr << "couldn't suspend, not attached" << endl;
        return false;
    }
    if(d->suspended)
    {
        cerr << "couldn't suspend, already suspended" << endl;
        return true;
    }
    D_SHMCMD = CORE_SUSPEND;
    if(!d->waitWhile(CORE_SUSPEND))
    {
        cerr << "couldn't suspend, DF not responding to commands" << endl;
        return false;
    }
    d->suspended = true;
    return true;
}

bool Process::asyncSuspend()
{
    if(!d->attached)
    {
        return false;
    }
    if(d->suspended)
    {
        return true;
    }
    if(D_SHMCMD == CORE_SUSPENDED)
    {
        d->suspended = true;
        return true;
    }
    else
    {
        D_SHMCMD = CORE_SUSPEND;
        return false;
    }
}

bool Process::forceresume()
{
    return resume();
}

bool Process::resume()
{
    if(!d->attached)
    {
        cerr << "couldn't resume because of no attachment" << endl;
        return false;
    }
    if(!d->suspended)
    {
        cerr << "couldn't resume because of not being suspended" << endl;
        return true;
    }
    D_SHMCMD = CORE_RUNNING;
    d->suspended = false;
    return true;
}


bool Process::attach()
{
    if(g_pProcess != 0)
    {
        cerr << "there's already a different process attached" << endl;
        return false;
    }
    if(d->attached)
    {
        cerr << "already attached" << endl;
        return false;
    }
    // check if DF is there
    if(!d->isValidSV())
    {
        return false; // NOT
    }
    // try locking client mutex
    uint32_t result = WaitForSingleObject(d->DFCLMutex,0);
    if( result != WAIT_OBJECT_0 && result != WAIT_ABANDONED)
    {
        return false; // we couldn't lock it
    }

    char shmname [256];
    sprintf(shmname,"DFShm-%d",d->my_pid);

    // now try getting and attaching the shared memory
    HANDLE shmHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS,false,shmname);
    if(!shmHandle)
    {
        ReleaseMutex(d->DFCLMutex);
        return false; // we couldn't lock it
    }

    // attempt to attach the opened mapping
    d->my_shm = (char *) MapViewOfFile(shmHandle,FILE_MAP_ALL_ACCESS, 0,0, SHM_SIZE);
    if(!d->my_shm)
    {
        CloseHandle(shmHandle);
        ReleaseMutex(d->DFCLMutex);
        return false; // we couldn't attach the mapping
    }
    // we close the handle right here so we don't have to keep track of it
    CloseHandle(shmHandle);
    d->attached = true;
    suspend();
    g_pProcess = this;
    return true;
}

bool Process::detach()
{
    if(!d->attached)
    {
        return false;
    }
    // detach segment
    UnmapViewOfFile(d->my_shm);
    // release it for some other client
    ReleaseMutex(d->DFCLMutex); // we keep the mutex handles
    d->attached = false;
    d->suspended = false;
    g_pProcess = 0;
    return true;
}

void Process::read (uint32_t src_address, uint32_t size, uint8_t *target_buffer)
{
    // normal read under 1MB
    if(size <= SHM_BODY)
    {
        D_SHMHDR->address = src_address;
        D_SHMHDR->length = size;
        full_barrier
        D_SHMCMD = CORE_DFPP_READ;
        d->waitWhile(CORE_DFPP_READ);
        memcpy (target_buffer, d->my_shm + SHM_HEADER,size);
    }
    // a big read, we pull data over the shm in iterations
    else
    {
        // first read equals the size of the SHM window
        uint32_t to_read = SHM_BODY;
        while (size)
        {
            // read to_read bytes from src_cursor
            D_SHMHDR->address = src_address;
            D_SHMHDR->length = to_read;
            full_barrier
            D_SHMCMD = CORE_DFPP_READ;
            d->waitWhile(CORE_DFPP_READ);
            memcpy (target_buffer, d->my_shm + SHM_HEADER,size);
            // decrease size by bytes read
            size -= to_read;
            // move the cursors
            src_address += to_read;
            target_buffer += to_read;
            // check how much to write in the next iteration
            to_read = min(size, (uint32_t) SHM_BODY);
        }
    }
}

uint8_t Process::readByte (const uint32_t offset)
{
    D_SHMHDR->address = offset;
    full_barrier
    D_SHMCMD = CORE_READ_BYTE;
    d->waitWhile(CORE_READ_BYTE);
    return D_SHMHDR->value;
}

void Process::readByte (const uint32_t offset, uint8_t &val )
{
    D_SHMHDR->address = offset;
    full_barrier
    D_SHMCMD = CORE_READ_BYTE;
    d->waitWhile(CORE_READ_BYTE);
    val = D_SHMHDR->value;
}

uint16_t Process::readWord (const uint32_t offset)
{
    D_SHMHDR->address = offset;
    full_barrier
    D_SHMCMD = CORE_READ_WORD;
    d->waitWhile(CORE_READ_WORD);
    return D_SHMHDR->value;
}

void Process::readWord (const uint32_t offset, uint16_t &val)
{
    D_SHMHDR->address = offset;
    full_barrier
    D_SHMCMD = CORE_READ_WORD;
    d->waitWhile(CORE_READ_WORD);
    val = D_SHMHDR->value;
}

uint32_t Process::readDWord (const uint32_t offset)
{
    D_SHMHDR->address = offset;
    full_barrier
    D_SHMCMD = CORE_READ_DWORD;
    d->waitWhile(CORE_READ_DWORD);
    return D_SHMHDR->value;
}
void Process::readDWord (const uint32_t offset, uint32_t &val)
{
    D_SHMHDR->address = offset;
    full_barrier
    D_SHMCMD = CORE_READ_DWORD;
    d->waitWhile(CORE_READ_DWORD);
    val = D_SHMHDR->value;
}

/*
 * WRITING
 */

void Process::writeDWord (uint32_t offset, uint32_t data)
{
    D_SHMHDR->address = offset;
    D_SHMHDR->value = data;
    full_barrier
    D_SHMCMD = CORE_WRITE_DWORD;
    d->waitWhile(CORE_WRITE_DWORD);
}

// using these is expensive.
void Process::writeWord (uint32_t offset, uint16_t data)
{
    D_SHMHDR->address = offset;
    D_SHMHDR->value = data;
    full_barrier
    D_SHMCMD = CORE_WRITE_WORD;
    d->waitWhile(CORE_WRITE_WORD);
}

void Process::writeByte (uint32_t offset, uint8_t data)
{
    D_SHMHDR->address = offset;
    D_SHMHDR->value = data;
    full_barrier
    D_SHMCMD = CORE_WRITE_BYTE;
    d->waitWhile(CORE_WRITE_BYTE);
}

void Process::write (uint32_t dst_address, uint32_t size, uint8_t *source_buffer)
{
    // normal write under 1MB
    if(size <= SHM_BODY)
    {
        D_SHMHDR->address = dst_address;
        D_SHMHDR->length = size;
        memcpy(d->my_shm+SHM_HEADER,source_buffer, size);
        full_barrier
        D_SHMCMD = CORE_WRITE;
        d->waitWhile(CORE_WRITE);
    }
    // a big write, we push this over the shm in iterations
    else
    {
        // first write equals the size of the SHM window
        uint32_t to_write = SHM_BODY;
        while (size)
        {
            // write to_write bytes to dst_cursor
            D_SHMHDR->address = dst_address;
            D_SHMHDR->length = to_write;
            memcpy(d->my_shm+SHM_HEADER,source_buffer, to_write);
            full_barrier
            D_SHMCMD = CORE_WRITE;
            d->waitWhile(CORE_WRITE);
            // decrease size by bytes written
            size -= to_write;
            // move the cursors
            source_buffer += to_write;
            dst_address += to_write;
            // check how much to write in the next iteration
            to_write = min(size, (uint32_t) SHM_BODY);
        }
    }
}

// FIXME: butt-fugly
const std::string Process::readCString (uint32_t offset)
{
    std::string temp;
    char temp_c[256];
    int counter = 0;
    char r;
    do
    {
        r = readByte(offset+counter);
        temp_c[counter] = r;
        counter++;
    } while (r && counter < 255);
    temp_c[counter] = 0;
    temp = temp_c;
    return temp;
}

DfVector Process::readVector (uint32_t offset, uint32_t item_size)
{
    /*
        MSVC++ vector is four pointers long
        ptr allocator
        ptr start
        ptr end
        ptr alloc_end
     
        we don't care about alloc_end because we don't try to add stuff
        we also don't care about the allocator thing in front
    */
    uint32_t start = g_pProcess->readDWord(offset+4);
    uint32_t end = g_pProcess->readDWord(offset+8);
    uint32_t size = (end - start) /4;
    return DfVector(start,size,item_size);
}

const std::string Process::readSTLString(uint32_t offset)
{
    //offset -= 4; //msvc std::string pointers are 8 bytes ahead of their data, not 4
    D_SHMHDR->address = offset;
    full_barrier
    D_SHMCMD = CORE_READ_STL_STRING;
    d->waitWhile(CORE_READ_STL_STRING);
    int length = D_SHMHDR->value;
//    char temp_c[256];
//    strncpy(temp_c, d->my_shm+SHM_HEADER,length+1); // length + 1 for the null terminator
    return(string(d->my_shm+SHM_HEADER));
}

size_t Process::readSTLString (uint32_t offset, char * buffer, size_t bufcapacity)
{
    //offset -= 4; //msvc std::string pointers are 8 bytes ahead of their data, not 4
    D_SHMHDR->address = offset;
    full_barrier
    D_SHMCMD = CORE_READ_STL_STRING;
    d->waitWhile(CORE_READ_STL_STRING);
    size_t length = D_SHMHDR->value;
    size_t real = min(length, bufcapacity - 1);
    strncpy(buffer, d->my_shm+SHM_HEADER,real); // length + 1 for the null terminator
    buffer[real] = 0;
    return real;
}

void Process::writeSTLString(const uint32_t address, const std::string writeString)
{
    D_SHMHDR->address = address/*-4*/;
    strncpy(d->my_shm+SHM_HEADER,writeString.c_str(),writeString.length()+1); // length + 1 for the null terminator
    full_barrier
    D_SHMCMD = CORE_WRITE_STL_STRING;
    d->waitWhile(CORE_WRITE_STL_STRING);
}

string Process::readClassName (uint32_t vptr)
{
    int rtti = readDWord(vptr - 0x4);
    int typeinfo = readDWord(rtti + 0xC);
    string raw = readCString(typeinfo + 0xC); // skips the .?AV
    raw.resize(raw.length() - 4);// trim st@@ from end
    return raw;
}

// get module index by name and version. bool 1 = error
bool Process::getModuleIndex (const char * name, const uint32_t version, uint32_t & OUTPUT)
{
    modulelookup * payload = (modulelookup *) (d->my_shm + SHM_HEADER);
    payload->version = version;
    strcpy(payload->name,name);
    full_barrier
    D_SHMCMD = CORE_ACQUIRE_MODULE;
    d->waitWhile(CORE_ACQUIRE_MODULE);
    if(D_SHMHDR->error) return false;
    OUTPUT = D_SHMHDR->value;
    return true;
}

char * Process::getSHMStart (void)
{
    return d->my_shm;
}