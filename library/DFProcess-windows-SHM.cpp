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
#include "SHMProcess.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/DFError.h"
#include "shms.h"
#include "mod-core.h"
using namespace DFHack;

SHMProcess::Private::Private(SHMProcess * self_)
{
    memdescriptor = NULL;
    process_ID = 0;
    attached = false;
    locked = false;
    identified = false;
    useYield = 0;
    DFSVMutex = 0;
    DFCLMutex = 0;
    DFCLSuspendMutex = 0;
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
                UnmapViewOfFile(shm_addr);
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

bool SHMProcess::SetAndWait (uint32_t state)
{
    return d->SetAndWait(state);
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

bool SHMProcess::Private::validate(VersionInfoFactory * factory)
{
    // try to identify the DF version
    IMAGE_NT_HEADERS pe_header;
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
    uint32_t pe_offset = self->Process::readDWord(base+0x3C);
    self->read(base + pe_offset                   , sizeof(pe_header), (uint8_t *)&pe_header);
    self->read(base + pe_offset+ sizeof(pe_header), sizeof(sections) , (uint8_t *)&sections );

    VersionInfo* vinfo = factory->getVersionInfoByPETimestamp(pe_header.FileHeader.TimeDateStamp);
    if(vinfo)
    {
        VersionInfo *m = new VersionInfo(*vinfo);
        m->RebaseAll(base);
        memdescriptor = m;
        m->setParentProcess(self);
        identified = true;
        vector_start = memdescriptor->getGroup("vector")->getOffset("start");
        CloseHandle(hProcess);
        return true;
    }
    return false;
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
    // BLAH
    ranges.clear();
}

bool SHMProcess::acquireSuspendLock()
{
    return ( WaitForSingleObject(d->DFCLSuspendMutex,INFINITE) == 0 );
}

bool SHMProcess::releaseSuspendLock()
{
    return ( ReleaseMutex(d->DFCLSuspendMutex) != 0);
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
    UnmapViewOfFile(d->shm_addr);
    // release it for some other client
    //ReleaseMutex(d->DFCLMutex); // we keep the mutex handles
    d->FreeLocks();
    d->attached = false;
    d->locked = false;
    d->shm_addr = false;
    return true;
}

void SHMProcess::readSTLVector(const uint32_t address, t_vecTriplet & triplet)
{
    read(address + d->vector_start, sizeof(triplet), (uint8_t *) &triplet);
}

string SHMProcess::readClassName (uint32_t vptr)
{
    int rtti = Process::readDWord(vptr - 0x4);
    int typeinfo = Process::readDWord(rtti + 0xC);
    string raw = readCString(typeinfo + 0xC); // skips the .?AV
    raw.resize(raw.length() - 2);// trim @@ from end
    return raw;
}

string SHMProcess::getPath()
{
    HMODULE hmod;
    DWORD junk;
    char String[255];
    HANDLE hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, d->process_ID ); //get the handle from the process ID
    EnumProcessModules(hProcess, &hmod, 1 * sizeof(HMODULE), &junk); //get the module from the handle
    GetModuleFileNameEx(hProcess,hmod,String,sizeof(String)); //get the filename from the module
    string out(String);
    return(out.substr(0,out.find_last_of("\\")));
}

char * SHMProcess::getSHMStart (void)
{
    if(!d->locked) throw Error::MemoryAccessDenied(0xdeadbeef);

    return d->shm_addr;
}
