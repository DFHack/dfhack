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
#include "dfhack/DFProcess.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/DFError.h"

#include <errno.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <time.h>
#include <shms.h>
#include <mod-core.h>
#include <sys/time.h>
#include <time.h>
#include <sched.h>

using namespace DFHack;

// a full memory barrier! better be safe than sorry.
#define gcc_barrier asm volatile("" ::: "memory"); __sync_synchronize();

class SHMProcess::Private
{
    public:
    Private(Process * self_)
    {
        memdescriptor = NULL;
        process_ID = 0;
        shm_addr = 0;
        //shm_addr_with_cl_idx = 0;
        shm_ID = -1;
        attached = false;
        identified = false;
        useYield = false;
        server_lock = -1;
        client_lock = -1;
        suspend_lock = -1;
        attachmentIdx = 0;
        locked = false;
        self = self_;
    };
    ~Private(){};
    VersionInfo * memdescriptor;
    Process * self;
    pid_t process_ID;
    char *shm_addr;
    int shm_ID;
    Process* q;
    int server_lock;
    int client_lock;
    int suspend_lock;
    int attachmentIdx;

    bool attached;
    bool locked;
    bool identified;
    bool useYield;

    bool validate(std::vector< VersionInfo* >& known_versions);

    bool Aux_Core_Attach(bool & versionOK, pid_t & PID);
    //bool waitWhile (uint32_t state);
    bool SetAndWait (uint32_t state);
    bool GetLocks();
    bool AreLocksOk();
    void FreeLocks();
};

#define SHMCMD ( (uint32_t *) shm_addr)[attachmentIdx]
#define D_SHMCMD ( (uint32_t *) (d->shm_addr))[d->attachmentIdx]

#define SHMHDR ((shm_core_hdr *)shm_addr)
#define D_SHMHDR ((shm_core_hdr *)(d->shm_addr))

#define SHMDATA(type) ((type *)(shm_addr + SHM_HEADER))
#define D_SHMDATA(type) ((type *)(d->shm_addr + SHM_HEADER))

bool SHMProcess::Private::SetAndWait (uint32_t state)
{
    uint32_t cnt = 0;
    if(!attached) return false;
    SHMCMD = state;
    while (SHMCMD == state)
    {
        // check if the other process is still there
        if(cnt == 10000)
        {
            if(!AreLocksOk())
            {
                //detach the shared memory
                shmdt(shm_addr);
                FreeLocks();
                attached = locked = identified = false;
                // we aren't the current process anymore
                throw Error::SHMServerDisappeared();
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
    // server returned a generic error
    if(SHMCMD == CORE_ERROR)
    {
        return false;
    }
    return true;
}

/*
Yeah. with no way to synchronize things (locks are slow, the OS doesn't give us
enough control over scheduling)
we end up with this silly thing
*/
bool SHMProcess::SetAndWait (uint32_t state)
{
    return d->SetAndWait(state);
}

uint32_t OS_getAffinity()
{
    cpu_set_t mask;
    sched_getaffinity(0,sizeof(cpu_set_t),&mask);
    // FIXME: truncation
    uint32_t affinity = *(uint32_t *) &mask;
    return affinity;
}

// test if we have client and server locks and the server is present
bool SHMProcess::Private::AreLocksOk()
{
    // both locks are inited (we hold our lock)
    if(client_lock != -1 && server_lock != -1)
    {
        if(lockf(server_lock,F_TEST,0) == -1) // and server holds its lock
        {
            return true; // OK, locks are good
        }
    }
    // locks are bad
    return false;
}

void SHMProcess::Private::FreeLocks()
{
    attachmentIdx = -1;
    if(client_lock != -1)
    {
        lockf(client_lock,F_ULOCK,0);
        close(client_lock);
        client_lock = -1;
    }
    if(server_lock != -1)
    {
        close(server_lock);
        server_lock = -1;
    }
    if(suspend_lock != -1)
    {
        close(suspend_lock);
        locked = false;
        suspend_lock = -1;
    }
}

bool SHMProcess::Private::GetLocks()
{
    char name[256];
    // try to acquire locks
    // look at the server lock, if it's locked, the server is present
    sprintf(name, "/tmp/DFHack/%d/SVlock",process_ID);
    server_lock = open(name,O_WRONLY);
    if(server_lock == -1)
    {
        // cerr << "can't open sv lock" << endl;
        return false;
    }

    if(lockf( server_lock, F_TEST, 0 ) != -1)
    {
        cerr << "sv lock not locked" << endl;
        close(server_lock);
        server_lock = -1;
        return false;
    }

    for(int i = 0; i < SHM_MAX_CLIENTS; i++)
    {
        // open the client suspend locked
        sprintf(name, "/tmp/DFHack/%d/CLSlock%d",process_ID,i);
        suspend_lock = open(name,O_WRONLY);
        if(suspend_lock == -1)
        {
            cerr << "can't open cl S-lock " << i << endl;
            // couldn't open lock
            continue;
        }

        // open the client lock, try to lock it
        sprintf(name, "/tmp/DFHack/%d/CLlock%d",process_ID,i);
        client_lock = open(name,O_WRONLY);
        if(client_lock == -1)
        {
            cerr << "can't open cl lock " << i << endl;
            close(suspend_lock);
            locked = false;
            suspend_lock = -1;
            // couldn't open lock
            continue;
        }
        if(lockf(client_lock,F_TLOCK, 0) == -1)
        {
            // couldn't acquire lock
            cerr << "can't acquire cl lock " << i << endl;
            close(suspend_lock);
            locked = false;
            suspend_lock = -1;
            close(client_lock);
            client_lock = -1;
            continue;
        }
        // ok, we have all the locks we need!
        attachmentIdx = i;
        return true;
    }
    close(server_lock);
    server_lock = -1;
    cerr << "can't get any client locks" << endl;
    return false;
}

SHMProcess::SHMProcess(uint32_t PID, vector< VersionInfo* >& known_versions)
: d(new Private(this))
{
    d->process_ID = PID;
    d->memdescriptor = 0;
    if(!attach())
    {
        // couldn't attach to process
        return;
    }
    /*
     * Test bridge version, get PID, sync Yield
     */
    bool bridgeOK;
    if(!d->Aux_Core_Attach(bridgeOK,d->process_ID))
    {
        detach();
        throw Error::SHMAttachFailure();
    }
    if(!bridgeOK)
    {
        detach();
        throw Error::SHMVersionMismatch();
    }

    // try to identify the DF version (md5 the binary, compare with known versions)
    d->validate(known_versions);
    // detach
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

bool SHMProcess::Private::validate(vector <VersionInfo *> & known_versions)
{
    char exe_link_name [256];
    char target_name[1024];
    int target_result;
    // find the binary
    sprintf(exe_link_name,"/proc/%d/exe", process_ID);
    target_result = readlink(exe_link_name, target_name, sizeof(target_name)-1);
    if (target_result == -1)
    {
        perror("readlink");
        return false;
    }
    // make sure we have a null terminated string...
    // see http://www.opengroup.org/onlinepubs/000095399/functions/readlink.html
    target_name[target_result] = 0;

    md5wrapper md5;
    // get hash of the running DF process
    string hash = md5.getHashFromFile(target_name);
    vector<VersionInfo *>::iterator it;
    // cerr << exe_file << " " << hash <<  endl;
    // iterate over the list of memory locations
    for ( it=known_versions.begin() ; it < known_versions.end(); it++ )
    {
        try{
            if(hash == (*it)->getMD5()) // are the md5 hashes the same?
            {
                VersionInfo *m = new VersionInfo(**it);
                memdescriptor = m;
                m->setParentProcess(dynamic_cast<Process *>( self ));
                identified = true;
                // cerr << "identified " << m->getVersion() << endl;
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
    if(d->memdescriptor)
        delete d->memdescriptor;
    delete d;
}

VersionInfo * SHMProcess::getDescriptor()
{
    return d->memdescriptor;
}

int SHMProcess::getPID()
{
    return d->process_ID;
}

// there is only one we care about.
bool SHMProcess::getThreadIDs(vector<uint32_t> & threads )
{
    if(d->attached)
    {
        threads.clear();
        threads.push_back(d->process_ID);
        return true;
    }
    return false;
}

//FIXME: cross-reference with ELF segment entries?
void SHMProcess::getMemRanges( vector<t_memrange> & ranges )
{
    char buffer[1024];
    char permissions[5]; // r/-, w/-, x/-, p/s, 0

    sprintf(buffer, "/proc/%lu/maps", d->process_ID);
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
    if(d->locked)
    {
        return true;
    }

    // FIXME: this should be controlled on the server side
    // FIXME: IF server got CORE_RUN in this frame, interpret CORE_SUSPEND as CORE_STEP
    // did we just resume a moment ago?
    if(D_SHMCMD == CORE_RUN)
    {
        //fprintf(stderr,"%d invokes step\n",d->attachmentIdx);
        /*
        // wait for the next window
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
    if(lockf(d->suspend_lock,F_LOCK,0) == 0)
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
    uint32_t cmd = D_SHMCMD;
    if(cmd == CORE_SUSPENDED)
    {
        // we have to hold the lock to be really suspended
        if(lockf(d->suspend_lock,F_LOCK,0) == 0)
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
    // unlock the suspend lock
    if(lockf(d->suspend_lock,F_ULOCK,0) == 0)
    {
        d->locked = false;
        if(d->SetAndWait(CORE_RUN)) // we have to make sure the server responds!
        {
            return true;
        }
        throw Error::SHMLockingError("if(d->SetAndWait(CORE_RUN))");
    }
    throw Error::SHMLockingError("if(lockf(d->suspend_lock,F_ULOCK,0) == 0)");
    return false;
}


bool SHMProcess::attach()
{
    if(d->attached)
    {
        if(!d->locked)
            return suspend();
        return true;
    }
    if(!d->GetLocks())
    {
        //cerr << "server is full or not really there!" << endl;
        return false;
    }

    /*
    * Locate the segment.
    */
    if ((d->shm_ID = shmget(SHM_KEY + d->process_ID, SHM_SIZE, 0666)) < 0)
    {
        d->FreeLocks();
        cerr << "can't find segment" << endl; // FIXME: throw
        return false;
    }

    /*
    * Attach the segment
    */
    if ((d->shm_addr = (char *) shmat(d->shm_ID, NULL, 0)) == (char *) -1)
    {
        d->FreeLocks();
        cerr << "can't attach segment" << endl; // FIXME: throw
        return false;
    }
    d->attached = true;
    if(!suspend())
    {
        shmdt(d->shm_addr);
        d->FreeLocks();
        cerr << "unable to suspend" << endl;
        return false;
    }
    return true;
}

bool SHMProcess::detach()
{
    if(!d->attached) return true;
    if(d->locked)
    {
        resume();
    }
    // detach segment
    if(shmdt(d->shm_addr) != -1)
    {
        d->FreeLocks();
        d->locked = false;
        d->attached = false;
        d->shm_addr = 0;
        return true;
    }
    // fail if we can't detach
    // FIXME: throw exception here??
    perror("failed to detach shared segment");
    return false;
}

void SHMProcess::read (uint32_t src_address, uint32_t size, uint8_t *target_buffer)
{
    if(!d->locked) throw Error::MemoryAccessDenied();

    // normal read under 1MB
    if(size <= SHM_BODY)
    {
        D_SHMHDR->address = src_address;
        D_SHMHDR->length = size;
        gcc_barrier
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
            gcc_barrier
            d->SetAndWait(CORE_READ);
            memcpy (target_buffer, D_SHMDATA(void) ,to_read);
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
    gcc_barrier
    d->SetAndWait(CORE_READ_BYTE);
    return D_SHMHDR->value;
}

void SHMProcess::readByte (const uint32_t offset, uint8_t &val )
{
    if(!d->locked) throw Error::MemoryAccessDenied();

    D_SHMHDR->address = offset;
    gcc_barrier
    d->SetAndWait(CORE_READ_BYTE);
    val = D_SHMHDR->value;
}

uint16_t SHMProcess::readWord (const uint32_t offset)
{
    if(!d->locked) throw Error::MemoryAccessDenied();

    D_SHMHDR->address = offset;
    gcc_barrier
    d->SetAndWait(CORE_READ_WORD);
    return D_SHMHDR->value;
}

void SHMProcess::readWord (const uint32_t offset, uint16_t &val)
{
    if(!d->locked) throw Error::MemoryAccessDenied();

    D_SHMHDR->address = offset;
    gcc_barrier
    d->SetAndWait(CORE_READ_WORD);
    val = D_SHMHDR->value;
}

uint32_t SHMProcess::readDWord (const uint32_t offset)
{
    if(!d->locked) throw Error::MemoryAccessDenied();

    D_SHMHDR->address = offset;
    gcc_barrier
    d->SetAndWait(CORE_READ_DWORD);
    return D_SHMHDR->value;
}
void SHMProcess::readDWord (const uint32_t offset, uint32_t &val)
{
    if(!d->locked) throw Error::MemoryAccessDenied();

    D_SHMHDR->address = offset;
    gcc_barrier
    d->SetAndWait(CORE_READ_DWORD);
    val = D_SHMHDR->value;
}

uint64_t SHMProcess::readQuad (const uint32_t offset)
{
    if(!d->locked) throw Error::MemoryAccessDenied();

    D_SHMHDR->address = offset;
    gcc_barrier
    d->SetAndWait(CORE_READ_QUAD);
    return D_SHMHDR->Qvalue;
}
void SHMProcess::readQuad (const uint32_t offset, uint64_t &val)
{
    if(!d->locked) throw Error::MemoryAccessDenied();

    D_SHMHDR->address = offset;
    gcc_barrier
    d->SetAndWait(CORE_READ_QUAD);
    val = D_SHMHDR->Qvalue;
}

float SHMProcess::readFloat (const uint32_t offset)
{
    if(!d->locked) throw Error::MemoryAccessDenied();

    D_SHMHDR->address = offset;
    gcc_barrier
    d->SetAndWait(CORE_READ_DWORD);
    return reinterpret_cast<float&> (D_SHMHDR->value);
}
void SHMProcess::readFloat (const uint32_t offset, float &val)
{
    if(!d->locked) throw Error::MemoryAccessDenied();

    D_SHMHDR->address = offset;
    gcc_barrier
    d->SetAndWait(CORE_READ_DWORD);
    val = reinterpret_cast<float&> (D_SHMHDR->value);
}

/*
 * WRITING
 */

void SHMProcess::writeQuad (const uint32_t offset, const uint64_t data)
{
    if(!d->locked) throw Error::MemoryAccessDenied();

    D_SHMHDR->address = offset;
    D_SHMHDR->Qvalue = data;
    gcc_barrier
    d->SetAndWait(CORE_WRITE_QUAD);
}

void SHMProcess::writeDWord (uint32_t offset, uint32_t data)
{
    if(!d->locked) throw Error::MemoryAccessDenied();

    D_SHMHDR->address = offset;
    D_SHMHDR->value = data;
    gcc_barrier
    d->SetAndWait(CORE_WRITE_DWORD);
}

// using these is expensive.
void SHMProcess::writeWord (uint32_t offset, uint16_t data)
{
    if(!d->locked) throw Error::MemoryAccessDenied();

    D_SHMHDR->address = offset;
    D_SHMHDR->value = data;
    gcc_barrier
    d->SetAndWait(CORE_WRITE_WORD);
}

void SHMProcess::writeByte (uint32_t offset, uint8_t data)
{
    if(!d->locked) throw Error::MemoryAccessDenied();

    D_SHMHDR->address = offset;
    D_SHMHDR->value = data;
    gcc_barrier
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
        gcc_barrier
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
            gcc_barrier
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
    if(!d->locked) throw Error::MemoryAccessDenied();

    int typeinfo = readDWord(vptr - 0x4);
    int typestring = readDWord(typeinfo + 0x4);
    string raw = readCString(typestring);
    size_t  start = raw.find_first_of("abcdefghijklmnopqrstuvwxyz");// trim numbers
    size_t end = raw.length();
    return raw.substr(start,end-start);
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
    if(!d->locked) return 0; //THROW HERE!

    return /*d->shm_addr_with_cl_idx*/ d->shm_addr;
}

bool SHMProcess::Private::Aux_Core_Attach(bool & versionOK, pid_t & PID)
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
string SHMProcess::getPath()
{
  char cwd_name[256];
    char target_name[1024];
    int target_result;

    sprintf(cwd_name,"/proc/%d/cwd", getPID());
    // resolve /proc/PID/exe link
    target_result = readlink(cwd_name, target_name, sizeof(target_name));
    return(string(target_name));
}
