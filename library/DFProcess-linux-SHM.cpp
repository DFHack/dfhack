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
#include "../shmserver/mod-core.h"
#include <sys/time.h>
#include <time.h>
#include <sched.h>

using namespace DFHack;

// a full memory barrier! better be safe than sorry.
#define gcc_barrier asm volatile("" ::: "memory"); __sync_synchronize();

class SHMProcess::Private
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
    Process* q;
    
    bool attached;
    bool suspended;
    bool identified;
    bool useYield;
    
    bool validate(char* exe_file, uint32_t pid, std::vector< memory_info* >& known_versions);
    
    bool Aux_Core_Attach(bool & versionOK, pid_t & PID);
    bool waitWhile (uint32_t state);
};

// some helpful macros to keep the code bloat in check
#define SHMCMD ((shm_cmd *)my_shm)->pingpong
#define D_SHMCMD ((shm_cmd *)d->my_shm)->pingpong

#define SHMHDR ((shm_core_hdr *)my_shm)
#define D_SHMHDR ((shm_core_hdr *)d->my_shm)

#define SHMDATA(type) ((type *)(my_shm + SHM_HEADER))
#define D_SHMDATA(type) ((type *)(d->my_shm + SHM_HEADER))

/*
Yeah. with no way to synchronize things (locks are slow, the OS doesn't give us enough control over scheduling)
we end up with this silly thing
*/
bool SHMProcess::Private::waitWhile (uint32_t state)
{
    uint32_t cnt = 0;
    struct shmid_ds descriptor;
    while (SHMCMD == state)
    {
        if(cnt == 10000)// check if the other process is still there
        {
            
            shmctl(my_shmid, IPC_STAT, &descriptor); 
            if(descriptor.shm_nattch == 1)// DF crashed or exited - no way to tell?
            {
                //detach the shared memory
                shmdt(my_shm);
                attached = suspended = false;
                
                // we aren't the current process anymore
                g_pProcess = NULL;
                
                throw Error::SHMServerDisappeared();
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
    if(SHMCMD == CORE_ERROR)
    {
        SHMCMD = CORE_RUNNING;
        attached = suspended = false;
        cerr << "shm server error!" << endl;
        assert (false);
        return false;
    }
    return true;
}

/*
Yeah. with no way to synchronize things (locks are slow, the OS doesn't give us enough control over scheduling)
we end up with this silly thing
*/
bool SHMProcess::waitWhile (uint32_t state)
{
    return d->waitWhile(state);
}

uint32_t OS_getAffinity()
{
    cpu_set_t mask;
    sched_getaffinity(0,sizeof(cpu_set_t),&mask);
    // FIXME: truncation
    uint32_t affinity = *(uint32_t *) &mask;
    return affinity;
}


bool SHMProcess::Private::Aux_Core_Attach(bool & versionOK, pid_t & PID)
{
    SHMDATA(coreattach)->cl_affinity = OS_getAffinity();
    gcc_barrier
    SHMCMD = CORE_ATTACH;
    if(!waitWhile(CORE_ATTACH))
        return false;
    gcc_barrier
    versionOK =( SHMDATA(coreattach)->sv_version == CORE_VERSION );
    PID = SHMDATA(coreattach)->sv_PID;
    useYield = SHMDATA(coreattach)->sv_useYield;
    #ifdef DEBUG
        if(useYield) cerr << "Using Yield!" << endl;
    #endif
    return true;
}

SHMProcess::SHMProcess(uint32_t PID, vector< memory_info* >& known_versions)
: d(new Private())
{
    char exe_link_name [256];
    char target_name[1024];
    int target_result;
    
    /*
     * Locate the segment.
     */
    if ((d->my_shmid = shmget(SHM_KEY + PID, SHM_SIZE, 0666)) < 0)
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
     * Test bridge version, get PID, sync Yield
     */
    bool bridgeOK;
    if(!d->Aux_Core_Attach(bridgeOK,d->my_pid))
    {
        fprintf(stderr,"DF terminated during reading\n");
        shmdt(d->my_shm);
        return;
    }
    if(!bridgeOK)
    {
        fprintf(stderr,"SHM bridge version mismatch\n");
        shmdt(d->my_shm);
        return;
    }
    
    // find the binary
    sprintf(exe_link_name,"/proc/%d/exe", d->my_pid);
    target_result = readlink(exe_link_name, target_name, sizeof(target_name)-1);
    if (target_result == -1)
    {
        perror("readlink");
        shmdt(d->my_shm);
        return;
    }
    
    // make sure we have a null terminated string...
    // see http://www.opengroup.org/onlinepubs/000095399/functions/readlink.html
    target_name[target_result] = 0;
    
    // try to identify the DF version
    d->validate(target_name, d->my_pid, known_versions);
    d->my_window = new DFWindow(this);

    gcc_barrier
    // at this point, DF is stopped and waiting for commands. make it run again
    D_SHMCMD = CORE_RUNNING;
    shmdt(d->my_shm); // detach so we don't attach twice when attach() is called
}

bool SHMProcess::isSuspended()
{
    return d->suspended;
}
bool SHMProcess::isAttached()
{
    return d->attached;
}

bool SHMProcess::isIdentified()
{
    return d->identified;
}

bool SHMProcess::Private::validate(char * exe_file, uint32_t pid, vector <memory_info *> & known_versions)
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

SHMProcess::~SHMProcess()
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

memory_info * SHMProcess::getDescriptor()
{
    return d->my_descriptor;
}

DFWindow * SHMProcess::getWindow()
{
    return d->my_window;
}

int SHMProcess::getPID()
{
    return d->my_pid;
}

//FIXME: implement
bool SHMProcess::getThreadIDs(vector<uint32_t> & threads )
{
    return false;
}

//FIXME: cross-reference with ELF segment entries?
void SHMProcess::getMemRanges( vector<t_memrange> & ranges )
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

bool SHMProcess::suspend()
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
    if(!waitWhile(CORE_SUSPEND))
    {
        return false;
    }
    d->suspended = true;
    return true;
}

bool SHMProcess::asyncSuspend()
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

bool SHMProcess::forceresume()
{
    return resume();
}

bool SHMProcess::resume()
{
    if(!d->attached)
        return false;
    if(!d->suspended)
        return true;
    D_SHMCMD = CORE_RUNNING;
    d->suspended = false;
    return true;
}


bool SHMProcess::attach()
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

bool SHMProcess::detach()
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

void SHMProcess::read (uint32_t src_address, uint32_t size, uint8_t *target_buffer)
{
    // normal read under 1MB
    if(size <= SHM_BODY)
    {
        D_SHMHDR->address = src_address;
        D_SHMHDR->length = size;
        gcc_barrier
        D_SHMCMD = CORE_DFPP_READ;
        waitWhile(CORE_DFPP_READ);
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
            waitWhile(CORE_DFPP_READ);
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

uint8_t SHMProcess::readByte (const uint32_t offset)
{
    D_SHMHDR->address = offset;
    gcc_barrier
    D_SHMCMD = CORE_READ_BYTE;
    waitWhile(CORE_READ_BYTE);
    return D_SHMHDR->value;
}

void SHMProcess::readByte (const uint32_t offset, uint8_t &val )
{
    D_SHMHDR->address = offset;
    gcc_barrier
    D_SHMCMD = CORE_READ_BYTE;
    waitWhile(CORE_READ_BYTE);
    val = D_SHMHDR->value;
}

uint16_t SHMProcess::readWord (const uint32_t offset)
{
    D_SHMHDR->address = offset;
    gcc_barrier
    D_SHMCMD = CORE_READ_WORD;
    waitWhile(CORE_READ_WORD);
    return D_SHMHDR->value;
}

void SHMProcess::readWord (const uint32_t offset, uint16_t &val)
{
    D_SHMHDR->address = offset;
    gcc_barrier
    D_SHMCMD = CORE_READ_WORD;
    waitWhile(CORE_READ_WORD);
    val = D_SHMHDR->value;
}

uint32_t SHMProcess::readDWord (const uint32_t offset)
{
    D_SHMHDR->address = offset;
    gcc_barrier
    D_SHMCMD = CORE_READ_DWORD;
    waitWhile(CORE_READ_DWORD);
    return D_SHMHDR->value;
}
void SHMProcess::readDWord (const uint32_t offset, uint32_t &val)
{
    D_SHMHDR->address = offset;
    gcc_barrier
    D_SHMCMD = CORE_READ_DWORD;
    waitWhile(CORE_READ_DWORD);
    val = D_SHMHDR->value;
}

/*
 * WRITING
 */

void SHMProcess::writeDWord (uint32_t offset, uint32_t data)
{
    D_SHMHDR->address = offset;
    D_SHMHDR->value = data;
    gcc_barrier
    D_SHMCMD = CORE_WRITE_DWORD;
    waitWhile(CORE_WRITE_DWORD);
}

// using these is expensive.
void SHMProcess::writeWord (uint32_t offset, uint16_t data)
{
    D_SHMHDR->address = offset;
    D_SHMHDR->value = data;
    gcc_barrier
    D_SHMCMD = CORE_WRITE_WORD;
    waitWhile(CORE_WRITE_WORD);
}

void SHMProcess::writeByte (uint32_t offset, uint8_t data)
{
    D_SHMHDR->address = offset;
    D_SHMHDR->value = data;
    gcc_barrier
    D_SHMCMD = CORE_WRITE_BYTE;
    waitWhile(CORE_WRITE_BYTE);
}

void SHMProcess::write (uint32_t dst_address, uint32_t size, uint8_t *source_buffer)
{
    // normal write under 1MB
    if(size <= SHM_BODY)
    {
        D_SHMHDR->address = dst_address;
        D_SHMHDR->length = size;
        memcpy(d->my_shm+SHM_HEADER,source_buffer, size);
        gcc_barrier
        D_SHMCMD = CORE_WRITE;
        waitWhile(CORE_WRITE);
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
            waitWhile(CORE_WRITE);
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

DfVector SHMProcess::readVector (uint32_t offset, uint32_t item_size)
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

const std::string SHMProcess::readSTLString(uint32_t offset)
{
    D_SHMHDR->address = offset;
    full_barrier
    D_SHMCMD = CORE_READ_STL_STRING;
    waitWhile(CORE_READ_STL_STRING);
    //int length = ((shm_retval *)d->my_shm)->value;
    return(string( (char *)d->my_shm+SHM_HEADER));
}

size_t SHMProcess::readSTLString (uint32_t offset, char * buffer, size_t bufcapacity)
{
    D_SHMHDR->address = offset;
    full_barrier
    D_SHMCMD = CORE_READ_STL_STRING;
    waitWhile(CORE_READ_STL_STRING);
    size_t length = D_SHMHDR->value;
    size_t fit = min(bufcapacity - 1, length);
    strncpy(buffer,(char *)d->my_shm+SHM_HEADER,fit);
    buffer[fit] = 0;
    return fit;
}

void SHMProcess::writeSTLString(const uint32_t address, const std::string writeString)
{
    D_SHMHDR->address = address;
    strncpy(d->my_shm+SHM_HEADER,writeString.c_str(),writeString.length()+1); // length + 1 for the null terminator
    full_barrier
    D_SHMCMD = CORE_WRITE_STL_STRING;
    waitWhile(CORE_WRITE_STL_STRING);
}

string SHMProcess::readClassName (uint32_t vptr)
{
    int typeinfo = readDWord(vptr - 0x4);
    int typestring = readDWord(typeinfo + 0x4);
    string raw = readCString(typestring);
    size_t  start = raw.find_first_of("abcdefghijklmnopqrstuvwxyz");// trim numbers
    size_t end = raw.length();
    return raw.substr(start,end-start - 2); // trim the 'st' from the end
}

// FIXME: having this around could lead to bad things in the hands of unsuspecting fools
// *!!DON'T BE AN UNSUSPECTING FOOL!!*
// the whole SHM thing works only because copying DWORDS is an atomic operation on i386 and x86_64 archs
// get module index by name and version. bool 1 = error
bool SHMProcess::getModuleIndex (const char * name, const uint32_t version, uint32_t & OUTPUT)
{
    modulelookup * payload = (modulelookup *) (d->my_shm + SHM_HEADER);
    payload->version = version;
    
    strncpy(payload->name,name,255);
    payload->name[255] = 0;
    
    full_barrier
    
    D_SHMCMD = CORE_ACQUIRE_MODULE;
    if(!waitWhile(CORE_ACQUIRE_MODULE))
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
    return d->my_shm;
}