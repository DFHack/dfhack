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
#include <errno.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <time.h>
#include "../shmserver/shms.h"
#include "../shmserver/shms-core.h"
#include <sys/time.h>
#include <time.h>
#include <sched.h>

using namespace DFHack;

// a full memory barrier! better be safe than sorry.
#define gcc_barrier asm volatile("" ::: "memory"); __sync_synchronize();

class Process::Private
{
    public:
    Private()
    {
        my_descriptor = NULL;
        my_pid = 0;
        my_shm = 0;
        my_shmid = -1;
        my_window = NULL;
        attached = false;
        suspended = false;
        identified = false;
        useYield = false;
    };
    ~Private(){};
    memory_info * my_descriptor;
    DFWindow * my_window;
    pid_t my_pid;
    char *my_shm;
    int my_shmid;
    
    bool attached;
    bool suspended;
    bool identified;
    bool useYield;
    
    bool validate(char* exe_file, uint32_t pid, std::vector< memory_info* >& known_versions);
    bool waitWhile (CORE_COMMAND state);
    bool DF_TestBridgeVersion(bool & ret);
    bool DF_GetPID(pid_t & ret);
    void DF_SyncAffinity(void);
};

// some helpful macros to keep the code bloat in check
#define SHMCMD ((shm_cmd *)my_shm)->pingpong
#define D_SHMCMD ((shm_cmd *)d->my_shm)->pingpong

#define SHMHDR ((shm_core_hdr *)my_shm)
#define D_SHMHDR ((shm_core_hdr *)d->my_shm)

/*
Yeah. with no way to synchronize things (locks are slow, the OS doesn't give us enough control over scheduling)
we end up with this silly thing
*/
bool Process::Private::waitWhile (CORE_COMMAND state)
{
    uint32_t cnt = 0;
    struct shmid_ds descriptor;
    while (SHMCMD == state)
    {
        if(cnt == 10000)// check if the other process is still there
        {
            
            shmctl(my_shmid, IPC_STAT, &descriptor); 
            if(descriptor.shm_nattch == 1)// DF crashed?
            {
                SHMCMD = CORE_RUNNING;
                attached = suspended = false;
                return false;
            }
            else
            {
                cnt = 0;
            }
        }
        if(useYield)
        {
            SCHED_YIELD
        }
        cnt++;
    }
    if(SHMCMD == CORE_SV_ERROR)
    {
        SHMCMD = CORE_RUNNING;
        attached = suspended = false;
        cerr << "shm server error!" << endl;
        assert (false);
        return false;
    }
    return true;
}

bool Process::Private::DF_TestBridgeVersion(bool & ret)
{
    SHMCMD = CORE_GET_VERSION;
    gcc_barrier
    if(!waitWhile(CORE_GET_VERSION))
        return false;
    gcc_barrier
    SHMCMD = CORE_SUSPENDED;
    ret =( SHMHDR->value == CORE_VERSION );
    return true;
}

bool Process::Private::DF_GetPID(pid_t & ret)
{
    SHMCMD = CORE_GET_PID;
    gcc_barrier
    if(!waitWhile(CORE_GET_PID))
        return false;
    gcc_barrier
    SHMCMD = CORE_SUSPENDED;
    ret = SHMHDR->value;
    return true;
}

uint32_t OS_getAffinity()
{
    cpu_set_t mask;
    sched_getaffinity(0,sizeof(cpu_set_t),&mask);
    // FIXME: truncation
        uint32_t affinity = *(uint32_t *) &mask;
        return affinity;
}

void Process::Private::DF_SyncAffinity( void )
{
    SHMHDR->value = OS_getAffinity();
    gcc_barrier
    SHMCMD = CORE_SYNC_YIELD;
    gcc_barrier
    if(!waitWhile(CORE_SYNC_YIELD))
        return;
    gcc_barrier
    SHMCMD = CORE_SUSPENDED;
    useYield = SHMHDR->value;
    #ifdef DEBUG
    if(useYield) cerr << "Using Yield!" << endl;
    #endif
}

Process::Process(vector <memory_info *> & known_versions)
: d(new Private())
{
    char exe_link_name [256];
    char target_name[1024];
    int target_result;
    
    /*
     * Locate the segment.
     */
    if ((d->my_shmid = shmget(SHM_KEY, SHM_SIZE, 0666)) < 0)
    {
        return;
    }
    
    /*
     * Attach the segment
     */
    if ((d->my_shm = (char *) shmat(d->my_shmid, NULL, 0)) == (char *) -1)
    {
        return;
    }
    
    /*
     * Check if there are two processes connected to the segment
     */
    shmid_ds descriptor;
    shmctl(d->my_shmid, IPC_STAT, &descriptor); 
    if(descriptor.shm_nattch != 2)// badness
    {
        fprintf(stderr,"dfhack: %d : invalid no. of processes connected\n", (int) descriptor.shm_nattch);
        fprintf(stderr,"detach: %d",shmdt(d->my_shm));
        return;
    }
    
    /*
     * Test bridge version, will also detect when we connect to something that doesn't respond
     */
    bool bridgeOK;
    if(!d->DF_TestBridgeVersion(bridgeOK))
    {
        fprintf(stderr,"DF terminated during reading\n");
        return;
    }
    if(!bridgeOK)
    {
        fprintf(stderr,"SHM bridge version mismatch\n");
        return;
    }
    /*
     * get the PID from DF
     */
    if(d->DF_GetPID(d->my_pid))
    {
        // find its binary
        sprintf(exe_link_name,"/proc/%d/exe", d->my_pid);
        target_result = readlink(exe_link_name, target_name, sizeof(target_name)-1);
        if (target_result == -1)
        {
            perror("readlink");
            return;
        }
        // make sure we have a null terminated string...
        // see http://www.opengroup.org/onlinepubs/000095399/functions/readlink.html
        target_name[target_result] = 0;
        
        // try to identify the DF version
        d->validate(target_name, d->my_pid, known_versions);
        d->DF_SyncAffinity();
        d->my_window = new DFWindow(this);
    }
    gcc_barrier
    // at this point, DF is stopped and waiting for commands. make it run again
    D_SHMCMD = CORE_RUNNING;
    shmdt(d->my_shm); // detach so we don't attach twice when attach() is called
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

bool Process::Private::validate(char * exe_file, uint32_t pid, vector <memory_info *> & known_versions)
{
    md5wrapper md5;
    // get hash of the running DF process
    string hash = md5.getHashFromFile(exe_file);
    vector<memory_info *>::iterator it;
    cerr << exe_file << " " << hash <<  endl;
    // iterate over the list of memory locations
    for ( it=known_versions.begin() ; it < known_versions.end(); it++ )
    {
        try{
            if(hash == (*it)->getString("md5")) // are the md5 hashes the same?
            {
                memory_info * m = *it;
                my_descriptor = m;
                my_pid = pid;
                identified = true;
                cerr << "identified " << m->getVersion() << endl;
                return true;
            }
        }
        catch (Error::MissingMemoryDefinition&)
        {
            continue;
        }
    }
    return false;
}

Process::~Process()
{
    if(d->attached)
    {
        detach();
    }
    // destroy data model. this is assigned by processmanager
    if(d->my_window)
    {
        delete d->my_window;
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
        return false;
    }
    if(d->suspended)
    {
        return true;
    }
    D_SHMCMD = CORE_SUSPEND;
    if(!d->waitWhile(CORE_SUSPEND))
    {
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
        return false;
    if(!d->suspended)
        return true;
    D_SHMCMD = CORE_RUNNING;
    d->suspended = false;
    return true;
}


bool Process::attach()
{
    int status;
    if(g_pProcess != 0)
    {
        cerr << "there's already a different process attached" << endl;
        return false;
    }
    /*
    * Attach the segment
    */
    if ((d->my_shm = (char *) shmat(d->my_shmid, NULL, 0)) != (char *) -1)
    {
        d->attached = true;
        if(suspend())
        {
            d->suspended = true;
            g_pProcess = this;
            return true;
        }
        d->attached = false;
        cerr << "unable to suspend" << endl;
        // FIXME: detach sehment here
        return false;
    }
    cerr << "unable to attach" << endl;
    return false;
}

bool Process::detach()
{
    if(!d->attached)
    {
        return false;
    }
    if(d->suspended)
    {
        resume();
    }
    // detach segment
    if(shmdt(d->my_shm) != -1)
    {
        d->attached = false;
        d->suspended = false;
        d->my_shm = 0;
        g_pProcess = 0;
        return true;
    }
    // fail if we can't detach
    perror("failed to detach shared segment");
    return false;
}

void Process::read (uint32_t src_address, uint32_t size, uint8_t *target_buffer)
{
    // normal read under 1MB
    if(size <= SHM_BODY)
    {
        D_SHMHDR->address = src_address;
        D_SHMHDR->length = size;
        gcc_barrier
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
            gcc_barrier
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
    gcc_barrier
    D_SHMCMD = CORE_READ_BYTE;
    d->waitWhile(CORE_READ_BYTE);
    return D_SHMHDR->value;
}

void Process::readByte (const uint32_t offset, uint8_t &val )
{
    D_SHMHDR->address = offset;
    gcc_barrier
    D_SHMCMD = CORE_READ_BYTE;
    d->waitWhile(CORE_READ_BYTE);
    val = D_SHMHDR->value;
}

uint16_t Process::readWord (const uint32_t offset)
{
    D_SHMHDR->address = offset;
    gcc_barrier
    D_SHMCMD = CORE_READ_WORD;
    d->waitWhile(CORE_READ_WORD);
    return D_SHMHDR->value;
}

void Process::readWord (const uint32_t offset, uint16_t &val)
{
    D_SHMHDR->address = offset;
    gcc_barrier
    D_SHMCMD = CORE_READ_WORD;
    d->waitWhile(CORE_READ_WORD);
    val = D_SHMHDR->value;
}

uint32_t Process::readDWord (const uint32_t offset)
{
    D_SHMHDR->address = offset;
    gcc_barrier
    D_SHMCMD = CORE_READ_DWORD;
    d->waitWhile(CORE_READ_DWORD);
    return D_SHMHDR->value;
}
void Process::readDWord (const uint32_t offset, uint32_t &val)
{
    D_SHMHDR->address = offset;
    gcc_barrier
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
    gcc_barrier
    D_SHMCMD = CORE_WRITE_DWORD;
    d->waitWhile(CORE_WRITE_DWORD);
}

// using these is expensive.
void Process::writeWord (uint32_t offset, uint16_t data)
{
    D_SHMHDR->address = offset;
    D_SHMHDR->value = data;
    gcc_barrier
    D_SHMCMD = CORE_WRITE_WORD;
    d->waitWhile(CORE_WRITE_WORD);
}

void Process::writeByte (uint32_t offset, uint8_t data)
{
    D_SHMHDR->address = offset;
    D_SHMHDR->value = data;
    gcc_barrier
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
        gcc_barrier
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
            gcc_barrier
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
        GNU libstdc++ vector is three pointers long
        ptr start
        ptr end
        ptr alloc_end

        we don't care about alloc_end because we don't try to add stuff
    */
    uint32_t start = g_pProcess->readDWord(offset);
    uint32_t end = g_pProcess->readDWord(offset+4);
    uint32_t size = (end - start) /4;
    return DfVector(start,size,item_size);
}

const std::string Process::readSTLString(uint32_t offset)
{
    D_SHMHDR->address = offset;
    full_barrier
    D_SHMCMD = CORE_READ_STL_STRING;
    d->waitWhile(CORE_READ_STL_STRING);
    //int length = ((shm_retval *)d->my_shm)->value;
    return(string( (char *)d->my_shm+SHM_HEADER));
}

size_t Process::readSTLString (uint32_t offset, char * buffer, size_t bufcapacity)
{
    D_SHMHDR->address = offset;
    full_barrier
    D_SHMCMD = CORE_READ_STL_STRING;
    d->waitWhile(CORE_READ_STL_STRING);
    size_t length = D_SHMHDR->value;
    size_t fit = min(bufcapacity - 1, length);
    strncpy(buffer,(char *)d->my_shm+SHM_HEADER,fit);
    buffer[fit] = 0;
    return fit;
}

void Process::writeSTLString(const uint32_t address, const std::string writeString)
{
    D_SHMHDR->address = address;
    strncpy(d->my_shm+SHM_HEADER,writeString.c_str(),writeString.length()+1); // length + 1 for the null terminator
    full_barrier
    D_SHMCMD = CORE_WRITE_STL_STRING;
    d->waitWhile(CORE_WRITE_STL_STRING);
}

string Process::readClassName (uint32_t vptr)
{
    int typeinfo = readDWord(vptr - 0x4);
    int typestring = readDWord(typeinfo + 0x4);
    string raw = readCString(typestring);
    size_t  start = raw.find_first_of("abcdefghijklmnopqrstuvwxyz");// trim numbers
    size_t end = raw.length();
    return raw.substr(start,end-start - 2); // trim the 'st' from the end
}