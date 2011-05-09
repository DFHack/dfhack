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
#include "PlatformInternal.h"

#include <string>
#include <vector>
#include <map>
#include <cstdio>
using namespace std;

#include "SHMProcess.h"
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
#include <md5wrapper.h>
using namespace DFHack;

SHMProcess::Private::Private(SHMProcess * self_)
{
    memdescriptor = NULL;
    process_ID = 0;
    shm_ID = -1;
    attached = false;
    identified = false;
    useYield = false;
    server_lock = -1;
    client_lock = -1;
    suspend_lock = -1;
    locked = false;
    self = self_;
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


bool SHMProcess::Private::validate(VersionInfoFactory * factory)
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

    // get hash of the running DF process
    md5wrapper md5;
    string hash = md5.getHashFromFile(target_name);

    // create linux process, add it to the vector
    VersionInfo * vinfo = factory->getVersionInfoByMD5(hash);
    if(vinfo)
    {
        memdescriptor = vinfo;
        // FIXME: BIG BAD BUG RIGHT HERE!!!!
        memdescriptor->setParentProcess(self);
        identified = true;
        vector_start = memdescriptor->getGroup("vector")->getOffset("start");
        return true;
    }
    return false;
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

    sprintf(buffer, "/proc/%lu/maps", (long unsigned)d->process_ID);
    FILE *mapFile = ::fopen(buffer, "r");
    uint64_t offset, device1, device2, node;

    while (fgets(buffer, 1024, mapFile))
    {
        t_memrange temp;
        temp.name[0] = 0;
        sscanf(buffer, "%zx-%zx %s %zx %2zu:%2zu %zu %s",
               &temp.start,
               &temp.end,
               (char*)&permissions,
               &offset, &device1, &device2, &node,
               (char*)&temp.name);
        temp.read = permissions[0] == 'r';
        temp.write = permissions[1] == 'w';
        temp.execute = permissions[2] == 'x';
        temp.valid = true;
        ranges.push_back(temp);
    }
}

bool SHMProcess::acquireSuspendLock()
{
    return (lockf(d->suspend_lock,F_LOCK,0) == 0);
}

bool SHMProcess::releaseSuspendLock()
{
    return (lockf(d->suspend_lock,F_ULOCK,0) == 0);
}


bool SHMProcess::attach()
{
    if(d->attached)
    {
        if(!d->locked)
            return suspend();
        return true;
    }
    //cerr << "attach" << endl;// FIXME: throw
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

void SHMProcess::readSTLVector(const uint32_t address, t_vecTriplet & triplet)
{
    read(address + d->vector_start, sizeof(triplet), (uint8_t *) &triplet);
}


string SHMProcess::doReadClassName (uint32_t vptr)
{
    if(!d->locked) throw Error::MemoryAccessDenied(vptr);

    int typeinfo = Process::readDWord(vptr - 0x4);
    int typestring = Process::readDWord(typeinfo + 0x4);
    string raw = readCString(typestring);
    size_t  start = raw.find_first_of("abcdefghijklmnopqrstuvwxyz");// trim numbers
    size_t end = raw.length();
    return raw.substr(start,end-start);
}

string SHMProcess::getPath()
{
  char cwd_name[256];
    char target_name[1024];
    int target_result;

    sprintf(cwd_name,"/proc/%d/cwd", getPID());
    // resolve /proc/PID/exe link
    target_result = readlink(cwd_name, target_name, sizeof(target_name));
    target_name[target_result] = '\0';
    return(string(target_name));
}

char * SHMProcess::getSHMStart (void)
{
    if(!d->locked) return 0; //THROW HERE!

    return d->shm_addr;
}
