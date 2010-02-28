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
    
    bool validate(char* exe_file, uint32_t pid, std::vector< memory_info* >& known_versions);
    bool waitWhile (DF_PINGPONG state);
    bool DF_TestBridgeVersion(bool & ret);
    bool DF_GetPID(pid_t & ret);
};

bool SHMProcess::Private::waitWhile (DF_PINGPONG state)
{
    uint32_t cnt = 0;
    struct shmid_ds descriptor;
    while (((shm_cmd *)my_shm)->pingpong == state)
    {
        if(cnt == 10000)
        {
            
            shmctl(my_shmid, IPC_STAT, &descriptor); 
            if(descriptor.shm_nattch == 1)// DF crashed?
            {
                gcc_barrier
                ((shm_cmd *)my_shm)->pingpong = DFPP_RUNNING;
                attached = suspended = false;
                return false;
            }
            else
            {
                cnt = 0;
            }
        }
        SCHED_YIELD
        cnt++;
    }
    if(((shm_cmd *)my_shm)->pingpong == DFPP_SV_ERROR)
    {
        ((shm_cmd *)my_shm)->pingpong = DFPP_RUNNING;
        attached = suspended = false;
        cerr << "shm server error!" << endl;
        assert (false);
        return false;
    }
    return true;
}

bool SHMProcess::Private::DF_TestBridgeVersion(bool & ret)
{
    ((shm_cmd *)my_shm)->pingpong = DFPP_VERSION;
    gcc_barrier
    if(!waitWhile(DFPP_VERSION))
        return false;
    gcc_barrier
    ((shm_cmd *)my_shm)->pingpong = DFPP_SUSPENDED;
    ret =( ((shm_retval *)my_shm)->value == PINGPONG_VERSION );
    return true;
}

bool SHMProcess::Private::DF_GetPID(pid_t & ret)
{
    ((shm_cmd *)my_shm)->pingpong = DFPP_PID;
    gcc_barrier
    if(!waitWhile(DFPP_PID))
        return false;
    gcc_barrier
    ((shm_cmd *)my_shm)->pingpong = DFPP_SUSPENDED;
    ret = ((shm_retval *)my_shm)->value;
    return true;
}

SHMProcess::SHMProcess(vector <memory_info *> & known_versions)
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
        d->my_window = new DFWindow(this);
    }
    gcc_barrier
    // at this point, DF is stopped and waiting for commands. make it run again
    ((shm_cmd *)d->my_shm)->pingpong = DFPP_RUNNING;
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
    ((shm_cmd *)d->my_shm)->pingpong = DFPP_SUSPEND;
    if(!d->waitWhile(DFPP_SUSPEND))
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
    if(((shm_cmd *)d->my_shm)->pingpong == DFPP_SUSPENDED)
    {
        d->suspended = true;
        return true;
    }
    else
    {
        ((shm_cmd *)d->my_shm)->pingpong = DFPP_SUSPEND;
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
    ((shm_cmd *)d->my_shm)->pingpong = DFPP_RUNNING;
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
        ((shm_read *)d->my_shm)->address = src_address;
        ((shm_read *)d->my_shm)->length = size;
        gcc_barrier
        ((shm_read *)d->my_shm)->pingpong = DFPP_READ;
        d->waitWhile(DFPP_READ);
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
            ((shm_read *)d->my_shm)->address = src_address;
            ((shm_read *)d->my_shm)->length = to_read;
            gcc_barrier
            ((shm_read *)d->my_shm)->pingpong = DFPP_READ;
            d->waitWhile(DFPP_READ);
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
    ((shm_read_small *)d->my_shm)->address = offset;
    gcc_barrier
    ((shm_read_small *)d->my_shm)->pingpong = DFPP_READ_BYTE;
    d->waitWhile(DFPP_READ_BYTE);
    return ((shm_retval *)d->my_shm)->value;
}

void SHMProcess::readByte (const uint32_t offset, uint8_t &val )
{
    ((shm_read_small *)d->my_shm)->address = offset;
    gcc_barrier
    ((shm_read_small *)d->my_shm)->pingpong = DFPP_READ_BYTE;
    d->waitWhile(DFPP_READ_BYTE);
    val = ((shm_retval *)d->my_shm)->value;
}

uint16_t SHMProcess::readWord (const uint32_t offset)
{
    ((shm_read_small *)d->my_shm)->address = offset;
    gcc_barrier
    ((shm_read_small *)d->my_shm)->pingpong = DFPP_READ_WORD;
    d->waitWhile(DFPP_READ_WORD);
    return ((shm_retval *)d->my_shm)->value;
}

void SHMProcess::readWord (const uint32_t offset, uint16_t &val)
{
    ((shm_read_small *)d->my_shm)->address = offset;
    gcc_barrier
    ((shm_read_small *)d->my_shm)->pingpong = DFPP_READ_WORD;
    d->waitWhile(DFPP_READ_WORD);
    val = ((shm_retval *)d->my_shm)->value;
}

uint32_t SHMProcess::readDWord (const uint32_t offset)
{
    ((shm_read_small *)d->my_shm)->address = offset;
    gcc_barrier
    ((shm_read_small *)d->my_shm)->pingpong = DFPP_READ_DWORD;
    d->waitWhile(DFPP_READ_DWORD);
    return ((shm_retval *)d->my_shm)->value;
}
void SHMProcess::readDWord (const uint32_t offset, uint32_t &val)
{
    ((shm_read_small *)d->my_shm)->address = offset;
    gcc_barrier
    ((shm_read_small *)d->my_shm)->pingpong = DFPP_READ_DWORD;
    d->waitWhile(DFPP_READ_DWORD);
    val = ((shm_retval *)d->my_shm)->value;
}

/*
 * WRITING
 */

void SHMProcess::writeDWord (uint32_t offset, uint32_t data)
{
    ((shm_write_small *)d->my_shm)->address = offset;
    ((shm_write_small *)d->my_shm)->value = data;
    gcc_barrier
    ((shm_write_small *)d->my_shm)->pingpong = DFPP_WRITE_DWORD;
    d->waitWhile(DFPP_WRITE_DWORD);
}

// using these is expensive.
void SHMProcess::writeWord (uint32_t offset, uint16_t data)
{
    ((shm_write_small *)d->my_shm)->address = offset;
    ((shm_write_small *)d->my_shm)->value = data;
    gcc_barrier
    ((shm_write_small *)d->my_shm)->pingpong = DFPP_WRITE_WORD;
    d->waitWhile(DFPP_WRITE_WORD);
}

void SHMProcess::writeByte (uint32_t offset, uint8_t data)
{
    ((shm_write_small *)d->my_shm)->address = offset;
    ((shm_write_small *)d->my_shm)->value = data;
    gcc_barrier
    ((shm_write_small *)d->my_shm)->pingpong = DFPP_WRITE_BYTE;
    d->waitWhile(DFPP_WRITE_BYTE);
}

void SHMProcess::write (uint32_t dst_address, uint32_t size, uint8_t *source_buffer)
{
    // normal write under 1MB
    if(size <= SHM_BODY)
    {
        ((shm_write *)d->my_shm)->address = dst_address;
        ((shm_write *)d->my_shm)->length = size;
        memcpy(d->my_shm+SHM_HEADER,source_buffer, size);
        gcc_barrier
        ((shm_write *)d->my_shm)->pingpong = DFPP_WRITE;
        d->waitWhile(DFPP_WRITE);
    }
    // a big write, we push this over the shm in iterations
    else
    {
        // first write equals the size of the SHM window
        uint32_t to_write = SHM_BODY;
        while (size)
        {
            // write to_write bytes to dst_cursor
            ((shm_write *)d->my_shm)->address = dst_address;
            ((shm_write *)d->my_shm)->length = to_write;
            memcpy(d->my_shm+SHM_HEADER,source_buffer, to_write);
            gcc_barrier
            ((shm_write *)d->my_shm)->pingpong = DFPP_WRITE;
            d->waitWhile(DFPP_WRITE);
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
    ((shm_read_small *)d->my_shm)->address = offset;
    full_barrier
    ((shm_read_small *)d->my_shm)->pingpong = DFPP_READ_STL_STRING;
    d->waitWhile(DFPP_READ_STL_STRING);
    //int length = ((shm_retval *)d->my_shm)->value;
    return(string( (char *)d->my_shm+SHM_HEADER));
}

size_t SHMProcess::readSTLString (uint32_t offset, char * buffer, size_t bufcapacity)
{
    ((shm_read_small *)d->my_shm)->address = offset;
    full_barrier
    ((shm_read_small *)d->my_shm)->pingpong = DFPP_READ_STL_STRING;
    d->waitWhile(DFPP_READ_STL_STRING);
    size_t length = ((shm_retval *)d->my_shm)->value;
    size_t fit = min(bufcapacity - 1, length);
    strncpy(buffer,(char *)d->my_shm+SHM_HEADER,fit);
    buffer[fit] = 0;
    return fit;
}

void SHMProcess::writeSTLString(const uint32_t address, const std::string writeString)
{
    ((shm_write_small *)d->my_shm)->address = address;
    strncpy(d->my_shm+SHM_HEADER,writeString.c_str(),writeString.length()+1); // length + 1 for the null terminator
    full_barrier
    ((shm_write_small *)d->my_shm)->pingpong = DFPP_WRITE_STL_STRING;
    d->waitWhile(DFPP_WRITE_STL_STRING);
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