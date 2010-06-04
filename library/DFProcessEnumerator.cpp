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
#include "DFMemInfoManager.h"

#include "dfhack/DFProcessEnumerator.h"
#include "dfhack/DFProcess.h"
#include "dfhack/DFMemInfo.h"


using namespace DFHack;

typedef std::vector<Process *> PROC_V;
typedef std::map<ProcessID, Process*> PID2PROC;

class DFHack::ProcessEnumerator::Private
{
    public:
        Private(){};
        MemInfoManager *meminfo;
        PROC_V Processes;
        PID2PROC ProcMap;
        Process *GetProcessObject(ProcessID ID);
        void EnumPIDs (vector <ProcessID> &PIDs);
};

class DFHack::BadProcesses::Private
{
    public:
        Private(){};
        PROC_V bad;
};

BadProcesses::BadProcesses():d(new Private()){}

BadProcesses::~BadProcesses()
{
    clear();
    delete d;
}

bool BadProcesses::Contains(Process* p)
{
    for(int i = 0; i < d->bad.size(); i++)
    {
        if(d->bad[i] == p)
            return true;
    }
    return false;
}

bool BadProcesses::excise(Process* p)
{
    vector<Process*>::iterator it = d->bad.begin();
    while(it != d->bad.end())
    {
        if((*it) == p)
        {
            d->bad.erase(it);
            return true;
        }
        else
        {
            it++;
        }
    }
    return false;
}

uint32_t BadProcesses::size()
{
    return d->bad.size();
}

void BadProcesses::clear()
{
    for(int i = 0; i < d->bad.size(); i++)
    {
        delete d->bad[i];
    }
    d->bad.clear();
}

void BadProcesses::push_back(Process* p)
{
    if(p)
        d->bad.push_back(p);
}

Process * BadProcesses::operator[](uint32_t index)
{
    if(index < d->bad.size())
        return d->bad[index];
    return 0;
}

//FIXME: wasteful
Process *ProcessEnumerator::Private::GetProcessObject(ProcessID ID)
{
    
    Process *p1 = new SHMProcess(ID.pid,meminfo->meminfo);
    if(p1->isIdentified())
        return p1;
    else
        delete p1;
    
    Process *p2 = new NormalProcess(ID.pid,meminfo->meminfo);
    if(p2->isIdentified())
        return p2;
    else
        delete p2;
#ifdef LINUX_BUILD
    Process *p3 = new WineProcess(ID.pid,meminfo->meminfo);
    if(p3->isIdentified())
        return p3;
    else
        delete p3;
#endif
    return 0;
}

#ifdef LINUX_BUILD
void ProcessEnumerator::Private::EnumPIDs (vector <ProcessID> &PIDs)
{
    DIR *dir_p;
    struct dirent *dir_entry_p;
    struct stat st;
    char fullname[512];
    fullname[0] = 0;
    PIDs.clear(); // make sure the vector is clear

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
        sprintf(fullname, "/proc/%s", dir_entry_p->d_name);
        int ierr = stat (fullname, &st);
        if (ierr != 0)
        {
            printf("Cannot stat %s: ierr= %d\n", fullname, ierr);
            continue;
        }
        uint64_t Pnum = atoi(dir_entry_p->d_name);
        uint64_t ctime = st.st_ctime;
        PIDs.push_back(ProcessID(ctime,Pnum));
    }
    closedir(dir_p);
}
#endif

#ifndef LINUX_BUILD
// some magic - will come in handy when we start doing debugger stuff on Windows
bool EnableDebugPriv()
{
    bool               bRET = FALSE;
    TOKEN_PRIVILEGES   tp;
    HANDLE             hToken;

    if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid))
    {
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
        {
            if (hToken != INVALID_HANDLE_VALUE)
            {
                tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
                tp.PrivilegeCount = 1;
                if (AdjustTokenPrivileges(hToken, FALSE, &tp, 0, 0, 0))
                {
                    bRET = TRUE;
                }
                CloseHandle(hToken);
            }
        }
    }
    return bRET;
}

// Convert Windows FileTime structs to POSIX timestamp
// from http://frenk.wordpress.com/2009/12/14/convert-filetime-to-unix-timestamp/
uint64_t FileTime_to_POSIX(FILETIME ft)
{
    // takes the last modified date
    LARGE_INTEGER date, adjust;
    date.HighPart = ft.dwHighDateTime;
    date.LowPart = ft.dwLowDateTime;

    // 100-nanoseconds = milliseconds * 10000
    adjust.QuadPart = 11644473600000 * 10000;

    // removes the diff between 1970 and 1601
    date.QuadPart -= adjust.QuadPart;

    // converts back from 100-nanoseconds to seconds
    return date.QuadPart / 10000000;
}

void ProcessEnumerator::Private::EnumPIDs (vector <ProcessID> &PIDs)
{
    FILETIME ftCreate, ftExit, ftKernel, ftUser;

    PIDs.clear(); // make sure the vector is clear

    // Get the list of process identifiers.
    DWORD ProcArray[2048], memoryNeeded, numProccesses;
    //EnableDebugPriv();
    if ( !EnumProcesses( ProcArray, sizeof(ProcArray), &memoryNeeded ) )
    {
        cout << "EnumProcesses fail'd" << endl;
        return;
    }
    // Calculate how many process identifiers were returned.
    numProccesses = memoryNeeded / sizeof(DWORD);
    EnableDebugPriv();
    // iterate through processes
    for ( int i = 0; i < (int)numProccesses; i++ )
    {
        HANDLE proc = OpenProcess (PROCESS_QUERY_INFORMATION, false, ProcArray[i]);
        if(!proc)
            continue;
        if(GetProcessTimes(proc, &ftCreate, &ftExit, &ftKernel, &ftUser))
        {
            uint64_t ctime = FileTime_to_POSIX(ftCreate);
            uint64_t Pnum = ProcArray[i];
            PIDs.push_back(ProcessID(ctime,Pnum));
        }
        CloseHandle(proc);
    }
}
#endif

bool ProcessEnumerator::Refresh( BadProcesses* invalidated_processes )
{
    // PIDs to process
    vector <ProcessID> PIDs;
    // this will be the new process map
    PID2PROC temporary;
    // clear the vector
    d->Processes.clear();
    if(invalidated_processes)
        invalidated_processes->clear();

    d->EnumPIDs(PIDs);

    for(uint64_t i = 0; i < PIDs.size();i++)
    {
        ProcessID & PID = PIDs[i];
        // check if we know about the OS process already
        PID2PROC::iterator found= d->ProcMap.find(PID);
        if( found != d->ProcMap.end())
        {
            // we do
            // check if it does have a DFHack Process object associated with it
            Process * p = (*found).second;
            if(p)
            {
                // add it back to the vector we export
                d->Processes.push_back(p);
            }
            // remove the OS Process from ProcMap
            d->ProcMap.erase(found);
            // add the OS Process to what will be the new ProcMap
            temporary[PID] = p;
        }
        else
        {
            // an OS process we don't know yet!
            // try to make a DFHack Process object for it
            if(Process*p = d->GetProcessObject(PID))
            {
                // allright. this is something that can be used
                d->Processes.push_back(p);
                temporary[PID] = p;
            }
            else
            {
                // just a process. we track it anyway. Why not.
                temporary[PID] = 0;
            }
        }
    }
    // now the vector we export is filled again and a temporary map with valid processes is created.
    // we iterate over the old Process map and destroy all the processes that are dead.
    for(PID2PROC::const_iterator idx = d->ProcMap.begin(); idx != d->ProcMap.end();++idx)
    {
        Process * p = (*idx).second;
        if(p)
        {
            if(invalidated_processes)
            {
                invalidated_processes->push_back(p);
            }
            else
            {
                delete p;
            }
        }
    }
    d->ProcMap.swap(temporary);
    // return value depends on if we found some DF processes
    if(d->Processes.size())
    {
        return true;
    }
    return false;
}

uint32_t ProcessEnumerator::size()
{
    return d->Processes.size();
}


Process * ProcessEnumerator::operator[](uint32_t index)
{
    assert(index < d->Processes.size());
    return d->Processes[index];
}


ProcessEnumerator::ProcessEnumerator( string path_to_xml )
: d(new Private())
{
    d->meminfo = new MemInfoManager(path_to_xml);
}

void ProcessEnumerator::purge()
{
    for(uint32_t i = 0;i < d->Processes.size();i++)
    {
        delete d->Processes[i];
    }
    d->ProcMap.clear();
    d->Processes.clear();
}

ProcessEnumerator::~ProcessEnumerator()
{
    // delete all processes
    purge();
    delete d->meminfo;
    delete d;
}
