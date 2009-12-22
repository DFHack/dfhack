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
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <time.h>
#include "shmserver/dfconnect.h"

using namespace DFHack;

/// HACK: global variables (only one process can be attached at the same time.)
Process * DFHack::g_pProcess; ///< current process. non-NULL when picked

bool waitWhile (char *shm, int shmid, uint32_t state)
{
    uint32_t cnt = 0;
    struct shmid_ds descriptor;
    while (((shm_cmd *)shm)->pingpong == state)
    {
        if(cnt == 10000)
        {
            shmctl(shmid, IPC_STAT, &descriptor); 
            if(descriptor.shm_nattch == 1)// DF crashed?
            {
                ((shm_cmd *)shm)->pingpong = DFPP_RUNNING;
                fprintf(stderr,"dfhack: Broke out of loop, other process disappeared.\n");
                return false;
            }
            else
            {
                cnt = 0;
            }
        }
        cnt++;
    }
    if(((shm_cmd *)shm)->pingpong == DFPP_SV_ERROR) return false;
    return true;
}

bool DF_TestBridgeVersion(char *shm, int shmid, bool & ret)
{
    ((shm_cmd *)shm)->pingpong = DFPP_VERSION;
    if(!waitWhile(shm, shmid, DFPP_VERSION))
        return false;
    ((shm_cmd *)shm)->pingpong = DFPP_SUSPENDED;
    ret =( ((shm_retval *)shm)->value == PINGPONG_VERSION );
    return true;
}

bool DF_GetPID(char *shm, int shmid, pid_t & ret)
{
    ((shm_cmd *)shm)->pingpong = DFPP_PID;
    if(!waitWhile(shm, shmid, DFPP_PID))
        return false;
    ((shm_cmd *)shm)->pingpong = DFPP_SUSPENDED;
    ret = ((shm_retval *)shm)->value;
    return true;
}

class DFHack::ProcessEnumerator::Private
{
    public:
        Private(){};
        MemInfoManager *meminfo;
        std::vector<Process *> processes;
};

bool ProcessEnumerator::findProcessess()
{
    DIR *dir_p;
    struct dirent *dir_entry_p;

    int errorcount;
    int result;
    
    char *shm = 0;
    int shmid;
    Process *p = 0;
    /*
     * Locate the segment.
     */
    if ((shmid = shmget(SHM_KEY, SHM_SIZE, 0666)) < 0)
    {
        perror("shmget");
    }
    else
    {
        if ((shm = (char *) shmat(shmid, NULL, 0)) == (char *) -1)
        {
            perror("shmat");
        }
        else
        {
            struct shmid_ds descriptor;
            shmctl(shmid, IPC_STAT, &descriptor); 
            /*
            * Now we attach the segment to our data space.
            */
            
            if(descriptor.shm_nattch != 2)// badness
            {
                fprintf(stderr,"dfhack: WTF\n");
            }
            else
            {
                bool bridgeOK;
                if(!DF_TestBridgeVersion(shm, shmid, bridgeOK))
                {
                    fprintf(stderr,"DF terminated during reading\n");
                }
                else
                {
                    if(!bridgeOK)
                    {
                        fprintf(stderr,"SHM bridge version mismatch\n");
                    }
                    else
                    {
                        pid_t DFPID;
                        if(DF_GetPID(shm, shmid, DFPID))
                        {
                            printf("shm: DF PID: %d\n",DFPID);
                            p = new SHMProcess(DFPID,shmid,shm,d->meminfo->meminfo);
                        }
                    }
                }
            }
            if(p && p->isIdentified())
            {
                cerr << "added process to vector" << endl;
                d->processes.push_back(p);
            }
            else
            {
                // couldn't use the hooked process, make sure it's running fine
                ((shm_cmd *)shm)->pingpong = DFPP_RUNNING;
                delete p;
            }
        }
    }
    
    // Open /proc/ directory
    dir_p = opendir("/proc/");
    // Reading /proc/ entries
    while(NULL != (dir_entry_p = readdir(dir_p)))
    {
        // Only PID folders (numbers)
        if (strspn(dir_entry_p->d_name, "0123456789") != strlen(dir_entry_p->d_name))
        {
            continue;
        }
        Process *p2 = new NormalProcess(atoi(dir_entry_p->d_name),d->meminfo->meminfo);
        if(p2->isIdentified())
        {
            d->processes.push_back(p2);
        }
        else
        {
            delete p2;
        }
    }
    closedir(dir_p);
    // return value depends on if we found some DF processes
    if(d->processes.size())
    {
        return true;
    }
    return false;
}

uint32_t ProcessEnumerator::size()
{
    return d->processes.size();
}


Process * ProcessEnumerator::operator[](uint32_t index)
{
    assert(index < d->processes.size());
    return d->processes[index];
}


ProcessEnumerator::ProcessEnumerator( string path_to_xml )
: d(new Private())
{
    d->meminfo = new MemInfoManager(path_to_xml);
}


ProcessEnumerator::~ProcessEnumerator()
{
    // delete all processes
    for(uint32_t i = 0;i < d->processes.size();i++)
    {
        delete d->processes[i];
    }
    delete d->meminfo;
    delete d;
}
