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
#include "DFProcess.h"
#include "DFWindow.h"
#include "DFMemInfo.h"
#include "DFError.h"
#include "shms.h"
#include "mod-core.h"
using namespace DFHack;

// a full memory barrier! better be safe than sorry.
class SHMProcess::Private
{
    public:
    Private()
    {
        memdescriptor = NULL;
        process_ID = 0;
        shm_addr = 0;
        window = NULL;
        attached = false;
        locked = false;
        identified = false;
        useYield = 0;
        DFSVMutex = 0;
        DFCLMutex = 0;
        DFCLSuspendMutex = 0;
        attachmentIdx = -1;
    };
    ~Private(){};
    memory_info * memdescriptor;
    DFWindow * window;
    SHMProcess * q;
    uint32_t process_ID;
    char *shm_addr;
    HANDLE DFSVMutex;
    HANDLE DFCLMutex;
    HANDLE DFCLSuspendMutex;
    int attachmentIdx;
    
    bool attached;
    bool locked;
    bool identified;
    bool useYield;
    
    bool validate(std::vector< memory_info* >& known_versions);
    
    bool Aux_Core_Attach(bool & versionOK, uint32_t & PID);
    bool SetAndWait (uint32_t state);
    bool GetLocks();
    bool AreLocksOk();
    void FreeLocks();
};

// some helpful macros to keep the code bloat in check
#define SHMCMD ( (uint32_t *) shm_addr)[attachmentIdx]
#define D_SHMCMD ( (uint32_t *) (d->shm_addr))[d->attachmentIdx]

#define SHMHDR ((shm_core_hdr *)shm_addr)
#define D_SHMHDR ((shm_core_hdr *)(d->shm_addr))

#define SHMDATA(type) ((type *)(shm_addr + SHM_HEADER))
#define D_SHMDATA(type) ((type *)(d->shm_addr + SHM_HEADER))

bool SHMProcess::SetAndWait (uint32_t state)
{
    return d->SetAndWait(state);
}

bool SHMProcess::Private::SetAndWait (uint32_t state)
{
    uint32_t cnt = 0;
    if(!attached) return false;
    SHMCMD = state;
    
    while (SHMCMD == state)
    {
        // yield the CPU, only on single-core CPUs
        if(useYield)
        {
            SCHED_YIELD
        }
        if(cnt == 10000)
        {
            if(!AreLocksOk())// DF not there anymore?
            {
                UnmapViewOfFile(shm_addr);
                FreeLocks();
                attached = locked = identified = false;
                // we aren't the current process anymore
                g_pProcess = NULL;
                throw Error::SHMServerDisappeared();
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

void SHMProcess::Private::FreeLocks()
{
    attachmentIdx = -1;
    if(DFCLMutex != 0)
    {
        ReleaseMutex(DFCLMutex);
        CloseHandle(DFCLMutex);
        DFCLMutex = 0;
    }
    if(DFSVMutex != 0)
    {
        CloseHandle(DFSVMutex);
        DFSVMutex = 0;
    }
    if(DFCLSuspendMutex != 0)
    {
        ReleaseMutex(DFCLSuspendMutex);
        CloseHandle(DFCLSuspendMutex);
        // FIXME: maybe also needs ReleaseMutex!
        DFCLSuspendMutex = 0;
        locked = false;
    }
}

bool SHMProcess::Private::GetLocks()
{
    char name[256];
    // try to acquire locks
    // look at the server lock, if it's locked, the server is present
    sprintf(name,"DFSVMutex-%d",process_ID);
    DFSVMutex = OpenMutex(SYNCHRONIZE,0, name);
    if(DFSVMutex == 0)
    {
        // cerr << "can't open sv lock" << endl;
        return false;
    }
    // unlike the F_TEST of lockf, this one actually locks. we have to release
    if(WaitForSingleObject(DFSVMutex,0) == 0)
    {
        ReleaseMutex(DFSVMutex);
        // cerr << "sv lock not locked" << endl;
        CloseHandle(DFSVMutex);
        DFSVMutex = 0;
        return false;
    }
                
    for(int i = 0; i < SHM_MAX_CLIENTS; i++)
    {
        // open the client suspend locked
        sprintf(name, "DFCLSuspendMutex-%d-%d",process_ID,i);
        DFCLSuspendMutex = OpenMutex(SYNCHRONIZE,0, name);
        if(DFCLSuspendMutex == 0)
        {
            //cerr << "can't open cl S-lock " << i << endl;
            // couldn't open lock
            continue;
        }

        // open the client lock, try to lock it
        
        sprintf(name,"DFCLMutex-%d-%d",process_ID,i);
        DFCLMutex =  OpenMutex(SYNCHRONIZE,0,name);
        if(DFCLMutex == 0)
        {
            //cerr << "can't open cl lock " << i << endl;
            CloseHandle(DFCLSuspendMutex);
            locked = false;
            DFCLSuspendMutex = 0;
            continue;
        }
        uint32_t waitstate = WaitForSingleObject(DFCLMutex,0);
        if(waitstate == WAIT_FAILED || waitstate == WAIT_TIMEOUT )
        {
            //cerr << "can't acquire cl lock " << i << endl;
            CloseHandle(DFCLSuspendMutex);
            locked = false;
            DFCLSuspendMutex = 0;
            CloseHandle(DFCLMutex);
            DFCLMutex = 0;
            continue;
        }
        // ok, we have all the locks we need!
        attachmentIdx = i;
        return true;
    }
    CloseHandle(DFSVMutex);
    DFSVMutex = 0;
    // cerr << "can't get any client locks" << endl;
    return false;
}

// is the other side still there?
bool SHMProcess::Private::AreLocksOk()
{
    // both locks are inited (we hold our lock)
    if(DFCLMutex != 0 && DFSVMutex != 0) 
    {
        // try if CL mutex is free
        switch (WaitForSingleObject(DFSVMutex,0))
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
    return false;
}



    /*
    char svmutexname [256];
    
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
    */

SHMProcess::SHMProcess(uint32_t PID, vector <memory_info *> & known_versions)
: d(new Private())
{
    d->process_ID = PID;
    d->q = this;
    // attach the SHM
    if(!attach())
    {
        return;
    }
    // Test bridge version, get PID, sync Yield
    bool bridgeOK;
    if(!d->Aux_Core_Attach(bridgeOK,d->process_ID))
    {
        detach();
        throw Error::SHMAttachFailure();
    }
    else if(!bridgeOK)
    {
        detach();
        throw Error::SHMVersionMismatch();
    }
    d->validate(known_versions);
    d->window = new DFWindow(this);
    // at this point, DF is attached and suspended, make it run
    detach();
}

bool SHMProcess::isSuspended()
{
    return d->locked;
}
bool SHMProcess::isAttached()
{
    return d->attached;
}

bool SHMProcess::isIdentified()
{
    return d->identified;
}
bool SHMProcess::Private::validate(vector <memory_info *> & known_versions)
{
    // try to identify the DF version
    IMAGE_NT_HEADERS32 pe_header;
    IMAGE_SECTION_HEADER sections[16];
    HMODULE hmod = NULL;
    DWORD junk;
    HANDLE hProcess;
    bool found = false;
    identified = false;
    // open process, we only need the process open 
    hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, process_ID );
    if (NULL == hProcess)
        return false;
    
    // try getting the first module of the process
    if(EnumProcessModules(hProcess, &hmod, 1 * sizeof(HMODULE), &junk) == 0)
    {
        CloseHandle(hProcess);
        // cout << "EnumProcessModules fail'd" << endl;
        return false;
    }
    // got base ;)
    uint32_t base = (uint32_t)hmod;
    
    // read from this process
    uint32_t pe_offset = q->readDWord(base+0x3C);
    q->read(base + pe_offset                   , sizeof(pe_header), (uint8_t *)&pe_header);
    q->read(base + pe_offset+ sizeof(pe_header), sizeof(sections) , (uint8_t *)&sections );
    
    // iterate over the list of memory locations
    vector<memory_info *>::iterator it;
    for ( it=known_versions.begin() ; it < known_versions.end(); it++ )
    {
        uint32_t pe_timestamp;
        try
        {
            pe_timestamp = (*it)->getHexValue("pe_timestamp");
        }
        catch(Error::MissingMemoryDefinition&)
        {
            continue;
        }
        if (pe_timestamp == pe_header.FileHeader.TimeDateStamp)
        {
            memory_info *m = new memory_info(**it);
            m->RebaseAll(base);
            memdescriptor = m;
            identified = true;
            cerr << "identified " << m->getVersion() << endl;
            CloseHandle(hProcess);
            return true;
        }
    }
    return false;
}
SHMProcess::~SHMProcess()
{
    if(d->attached)
    {
        detach();
    }
    // destroy data model. this is assigned by processmanager
    if(d->memdescriptor)
    {
        delete d->memdescriptor;
    }
    if(d->window)
    {
        delete d->window;
    }
    delete d;
}

memory_info * SHMProcess::getDescriptor()
{
    return d->memdescriptor;
}

DFWindow * SHMProcess::getWindow()
{
    return d->window;
}

int SHMProcess::getPID()
{
    return d->process_ID;
}

bool SHMProcess::getThreadIDs(vector<uint32_t> & threads )
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
        if( te32.th32OwnerProcessID == d->process_ID )
        {
            threads.push_back(te32.th32ThreadID);
        }
    } while( Thread32Next(AllThreads, &te32 ) ); 
    
    CloseHandle( AllThreads );
    return true;
}

//FIXME: use VirtualQuery to probe for memory ranges, cross-reference with base-corrected PE segment entries
void SHMProcess::getMemRanges( vector<t_memrange> & ranges )
{
    // code here is taken from hexsearch by Silas Dunmore.
    // As this IMHO isn't a 'sunstantial portion' of anything, I'm not including the MIT license here
    
    // I'm faking this, because there's no way I'm using VirtualQuery
    
    t_memrange temp;
    uint32_t base = d->memdescriptor->getBase();
    temp.start = base + 0x1000; // more fakery.
    temp.end = base + readDWord(base+readDWord(base+0x3C)+0x50)-1; // yay for magic.
    temp.read = 1;
    temp.write = 1;
    temp.execute = 0; // fake
    strcpy(temp.name,"pants");
    ranges.push_back(temp);
}

bool SHMProcess::suspend()
{
    if(!d->attached)
    {
        return false;
    }
    if(d->locked)
    {
        return true;
    }
    //cerr << "suspend" << endl;// FIXME: throw
    // FIXME: this should be controlled on the server side
    // FIXME: IF server got CORE_RUN in this frame, interpret CORE_SUSPEND as CORE_STEP
    // did we just resume a moment ago?
    if(D_SHMCMD == CORE_RUN)
    {
        //fprintf(stderr,"%d invokes step\n",d->attachmentIdx);
        // wait for the next window
        /*
        if(!d->SetAndWait(CORE_STEP))
        {
            throw Error::SHMLockingError("if(!d->SetAndWait(CORE_STEP))");
        }
        */
        D_SHMCMD = CORE_STEP;
    }
    else
    {
        //fprintf(stderr,"%d invokes suspend\n",d->attachmentIdx);
        // lock now
        /*
        if(!d->SetAndWait(CORE_SUSPEND))
        {
            throw Error::SHMLockingError("if(!d->SetAndWait(CORE_SUSPEND))");
        }
        */
        D_SHMCMD = CORE_SUSPEND;
    }
    //fprintf(stderr,"waiting for lock\n");
    // we wait for the server to give up our suspend lock (held by default)
    if( WaitForSingleObject(d->DFCLSuspendMutex,INFINITE) == 0 )
    {
        d->locked = true;
        return true;
    }
    return false;
}

// FIXME: needs a good think-through
bool SHMProcess::asyncSuspend()
{
    if(!d->attached)
    {
        return false;
    }
    if(d->locked)
    {
        return true;
    }
    //cerr << "async suspend" << endl;// FIXME: throw
    uint32_t cmd = D_SHMCMD;
    if(cmd == CORE_SUSPENDED)
    {
        // we have to hold the lock to be really suspended
        if( WaitForSingleObject(d->DFCLSuspendMutex,INFINITE) == 0 )
        {
            d->locked = true;
            return true;
        }
        return false;
    }
    else
    {
        // did we just resume a moment ago?
        if(cmd == CORE_STEP)
        {
            return false;
        }
        else if(cmd == CORE_RUN)
        {
            D_SHMCMD = CORE_STEP;
        }
        else
        {
            D_SHMCMD = CORE_SUSPEND;
        }
        return false;
    }
}

bool SHMProcess::forceresume()
{
    return resume();
}

// FIXME: wait for the server to advance a step!
bool SHMProcess::resume()
{
    if(!d->attached)
        return false;
    if(!d->locked)
        return true;
    //cerr << "resume" << endl;// FIXME: throw
    // unlock the suspend lock
    if( ReleaseMutex(d->DFCLSuspendMutex) != 0)
    {
        d->locked = false;
        if(d->SetAndWait(CORE_RUN)) // we have to make sure the server responds!
        {
            return true;
        }
        throw Error::SHMLockingError("if(d->SetAndWait(CORE_RUN))");
    }
    throw Error::SHMLockingError("if( ReleaseMutex(d->DFCLSuspendMutex) != 0)");
    return false;
}


bool SHMProcess::attach()
{
    if(g_pProcess != 0)
    {
        cerr << "there's already a process attached" << endl;
        return false;
    }
    //cerr << "attach" << endl;// FIXME: throw
    if(!d->GetLocks())
    {
        //cerr << "server is full or not really there!" << endl;
        return false;
    }
    /*
    // check if DF is there
    if(!d->isValidSV())
    {
        return false; // NOT
    }
    */
    /*
    // try locking client mutex
    uint32_t result = WaitForSingleObject(d->DFCLMutex,0);
    if( result != WAIT_OBJECT_0 && result != WAIT_ABANDONED)
    {
        return false; // we couldn't lock it
    }
    */
    
    /*
    * Locate the segment.
    */
    char shmname [256];
    sprintf(shmname,"DFShm-%d",d->process_ID);
    HANDLE shmHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS,false,shmname);
    if(!shmHandle)
    {
        d->FreeLocks();
        //ReleaseMutex(d->DFCLMutex);
        return false; // we couldn't lock it
    }

    /*
    * Attach the segment
    */
    d->shm_addr = (char *) MapViewOfFile(shmHandle,FILE_MAP_ALL_ACCESS, 0,0, SHM_SIZE);
    if(!d->shm_addr)
    {
        CloseHandle(shmHandle);
        //ReleaseMutex(d->DFCLMutex);
        d->FreeLocks(); 
        return false; // we couldn't attach the mapping // FIXME: throw
    }
    // we close the handle right here - it's not needed anymore
    CloseHandle(shmHandle);
    
    d->attached = true;
    if(!suspend())
    {
        UnmapViewOfFile(d->shm_addr);
        d->FreeLocks();
        //cerr << "unable to suspend" << endl;// FIXME: throw
        return false;
    }
    g_pProcess = this;
    return true;
}

bool SHMProcess::detach()
{
    if(!d->attached)
    {
        return false;
    }
    //cerr << "detach" << endl;// FIXME: throw
    if(d->locked)
    {
        resume();
    }
    //cerr << "detach after resume" << endl;// FIXME: throw
    // detach segment
    UnmapViewOfFile(d->shm_addr);
    // release it for some other client
    //ReleaseMutex(d->DFCLMutex); // we keep the mutex handles
    d->FreeLocks();
    d->attached = false;
    d->locked = false;
    d->shm_addr = false;
    g_pProcess = 0;
    return true;
}

void SHMProcess::read (uint32_t src_address, uint32_t size, uint8_t *target_buffer)
{
    if(!d->locked) throw Error::MemoryAccessDenied();
    
    // normal read under 1MB
    if(size <= SHM_BODY)
    {
        D_SHMHDR->address = src_address;
        D_SHMHDR->length = size;
        full_barrier
        d->SetAndWait(CORE_READ);
        memcpy (target_buffer, D_SHMDATA(void),size);
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
            d->SetAndWait(CORE_READ);
            memcpy (target_buffer, D_SHMDATA(void) ,size);
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

uint8_t SHMProcess::readByte (const uint32_t offset)
{
    if(!d->locked) throw Error::MemoryAccessDenied();
    
    D_SHMHDR->address = offset;
    full_barrier
    d->SetAndWait(CORE_READ_BYTE);
    return D_SHMHDR->value;
}

void SHMProcess::readByte (const uint32_t offset, uint8_t &val )
{
    if(!d->locked) throw Error::MemoryAccessDenied();
    
    D_SHMHDR->address = offset;
    full_barrier
    d->SetAndWait(CORE_READ_BYTE);
    val = D_SHMHDR->value;
}

uint16_t SHMProcess::readWord (const uint32_t offset)
{
    if(!d->locked) throw Error::MemoryAccessDenied();
    
    D_SHMHDR->address = offset;
    full_barrier
    d->SetAndWait(CORE_READ_WORD);
    return D_SHMHDR->value;
}

void SHMProcess::readWord (const uint32_t offset, uint16_t &val)
{
    if(!d->locked) throw Error::MemoryAccessDenied();
    
    D_SHMHDR->address = offset;
    full_barrier
    d->SetAndWait(CORE_READ_WORD);
    val = D_SHMHDR->value;
}

uint32_t SHMProcess::readDWord (const uint32_t offset)
{
    if(!d->locked) throw Error::MemoryAccessDenied();
    
    D_SHMHDR->address = offset;
    full_barrier
    d->SetAndWait(CORE_READ_DWORD);
    return D_SHMHDR->value;
}
void SHMProcess::readDWord (const uint32_t offset, uint32_t &val)
{
    if(!d->locked) throw Error::MemoryAccessDenied();
    
    D_SHMHDR->address = offset;
    full_barrier
    d->SetAndWait(CORE_READ_DWORD);
    val = D_SHMHDR->value;
}

/*
 * WRITING
 */

void SHMProcess::writeDWord (uint32_t offset, uint32_t data)
{
    if(!d->locked) throw Error::MemoryAccessDenied();
    
    D_SHMHDR->address = offset;
    D_SHMHDR->value = data;
    full_barrier
    d->SetAndWait(CORE_WRITE_DWORD);
}

// using these is expensive.
void SHMProcess::writeWord (uint32_t offset, uint16_t data)
{
    if(!d->locked) throw Error::MemoryAccessDenied();
    
    D_SHMHDR->address = offset;
    D_SHMHDR->value = data;
    full_barrier
    d->SetAndWait(CORE_WRITE_WORD);
}

void SHMProcess::writeByte (uint32_t offset, uint8_t data)
{
    if(!d->locked) throw Error::MemoryAccessDenied();
    
    D_SHMHDR->address = offset;
    D_SHMHDR->value = data;
    full_barrier
    d->SetAndWait(CORE_WRITE_BYTE);
}

void SHMProcess::write (uint32_t dst_address, uint32_t size, uint8_t *source_buffer)
{
    if(!d->locked) throw Error::MemoryAccessDenied();
    
    // normal write under 1MB
    if(size <= SHM_BODY)
    {
        D_SHMHDR->address = dst_address;
        D_SHMHDR->length = size;
        memcpy(D_SHMDATA(void),source_buffer, size);
        full_barrier
        d->SetAndWait(CORE_WRITE);
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
            memcpy(D_SHMDATA(void),source_buffer, to_write);
            full_barrier
            d->SetAndWait(CORE_WRITE);
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
const std::string SHMProcess::readCString (uint32_t offset)
{
    if(!d->locked) throw Error::MemoryAccessDenied();
    
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

const std::string SHMProcess::readSTLString(uint32_t offset)
{
    if(!d->locked) throw Error::MemoryAccessDenied();
        
    D_SHMHDR->address = offset;
    full_barrier
    d->SetAndWait(CORE_READ_STL_STRING);
    return(string( D_SHMDATA(char) ));
}

size_t SHMProcess::readSTLString (uint32_t offset, char * buffer, size_t bufcapacity)
{
    if(!d->locked) throw Error::MemoryAccessDenied();
        
    D_SHMHDR->address = offset;
    full_barrier
    d->SetAndWait(CORE_READ_STL_STRING);
    size_t length = D_SHMHDR->value;
    size_t fit = min(bufcapacity - 1, length);
    strncpy(buffer,D_SHMDATA(char),fit);
    buffer[fit] = 0;
    return fit;
}

void SHMProcess::writeSTLString(const uint32_t address, const std::string writeString)
{
    if(!d->locked) throw Error::MemoryAccessDenied();
    
    D_SHMHDR->address = address;
    strncpy(D_SHMDATA(char),writeString.c_str(),writeString.length()+1); // length + 1 for the null terminator
    full_barrier
    d->SetAndWait(CORE_WRITE_STL_STRING);
}

string SHMProcess::readClassName (uint32_t vptr)
{
    int rtti = readDWord(vptr - 0x4);
    int typeinfo = readDWord(rtti + 0xC);
    string raw = readCString(typeinfo + 0xC); // skips the .?AV
    raw.resize(raw.length() - 2);// trim @@ from end
    return raw;
}

// get module index by name and version. bool 0 = error
bool SHMProcess::getModuleIndex (const char * name, const uint32_t version, uint32_t & OUTPUT)
{
    if(!d->locked) throw Error::MemoryAccessDenied();
        
    modulelookup * payload = D_SHMDATA(modulelookup);
    payload->version = version;
    
    strncpy(payload->name,name,255);
    payload->name[255] = 0;
    
    if(!SetAndWait(CORE_ACQUIRE_MODULE))
    {
        return false; // FIXME: throw a fatal exception instead
    }
    if(D_SHMHDR->error)
    {
        return false;
    }
    //fprintf(stderr,"%s v%d : %d\n", name, version, D_SHMHDR->value);
    OUTPUT = D_SHMHDR->value;
    return true;
}

char * SHMProcess::getSHMStart (void)
{
    if(!d->locked) throw Error::MemoryAccessDenied();
    return /*d->shm_addr_with_cl_idx*/ d->shm_addr;
}

bool SHMProcess::Private::Aux_Core_Attach(bool & versionOK,  uint32_t & PID)
{
    if(!locked) throw Error::MemoryAccessDenied();
    
    SHMDATA(coreattach)->cl_affinity = OS_getAffinity();
    if(!SetAndWait(CORE_ATTACH)) return false;
    /*
    cerr <<"CORE_VERSION" << CORE_VERSION << endl;
    cerr <<"server CORE_VERSION" << SHMDATA(coreattach)->sv_version << endl;
    */
    versionOK =( SHMDATA(coreattach)->sv_version == CORE_VERSION );
    PID = SHMDATA(coreattach)->sv_PID;
    useYield = SHMDATA(coreattach)->sv_useYield;
    #ifdef DEBUG
        if(useYield) cerr << "Using Yield!" << endl;
    #endif
    return true;
}