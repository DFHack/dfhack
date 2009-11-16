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
using namespace DFHack;

Process::Process(DataModel * dm, memory_info* mi, ProcessHandle ph, uint32_t pid)
{
    my_datamodel = dm;
    my_descriptor = mi;
    my_handle = ph;
    my_pid = pid;
    attached = false;
    suspended = false;
}


Process::~Process()
{
    if(attached)
    {
        detach();
    }
    // destroy data model. this is assigned by processmanager
    delete my_datamodel;
    freeResources();
}


DataModel *Process::getDataModel()
{
    return my_datamodel;
}


memory_info * Process::getDescriptor()
{
    return my_descriptor;
}


void Process::setMemFile(const string & memf)
{
    assert(!attached);
    memFile = memf;
}

#ifdef LINUX_BUILD
/*
 *     LINUX PART
 */


bool Process::getThreadIDs(vector<uint32_t> & threads )
{
    return false;
}

void Process::getMemRanges( vector<t_memrange> & ranges )
{
    char buffer[1024];
    char name[1024];
    char permissions[5]; // r/-, w/-, x/-, p/s, 0
    
    sprintf(buffer, "/proc/%lu/maps", my_pid);
    FILE *mapFile = ::fopen(buffer, "r");
    uint64_t begin, end, offset, device1, device2, node;
    
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
    int status;
    if(!attached)
        return false;
    if(suspended)
        return true;
    if (kill(my_handle, SIGSTOP) == -1)
    {
        // no, we got an error
        perror("kill SIGSTOP error");
        return false;
    }
    while(true)
    {
        // we wait on the pid
        pid_t w = waitpid(my_handle, &status, 0);
        if (w == -1)
        {
            // child died
            perror("DF exited during suspend call");
            return false;
        }
        // stopped -> let's continue
        if (WIFSTOPPED(status))
        {
            break;
        }
    }
    suspended = true;
}

bool Process::resume()
{
    int status;
    if(!attached)
        return false;
    if(!suspended)
        return true;
    if (ptrace(PTRACE_CONT, my_handle, NULL, NULL) == -1)
    {
        // no, we got an error
        perror("ptrace resume error");
        return false;
    }
    suspended = false;
}


bool Process::attach()
{
    int status;
    if(g_pProcess != NULL)
    {
        return false;
    }
    // can we attach?
    if (ptrace(PTRACE_ATTACH , my_handle, NULL, NULL) == -1)
    {
        // no, we got an error
        perror("ptrace attach error");
        cerr << "attach failed on pid " << my_handle << endl;
        return false;
    }
    while(true)
    {
        // we wait on the pid
        pid_t w = waitpid(my_handle, &status, 0);
        if (w == -1)
        {
            // child died
            perror("wait inside attach()");
            return false;
        }
        // stopped -> let's continue
        if (WIFSTOPPED(status))
        {
            break;
        }
    }
    suspended = true;
    
    int proc_pid_mem = open(memFile.c_str(),O_RDONLY);
    if(proc_pid_mem == -1)
    {
        ptrace(PTRACE_DETACH, my_handle, NULL, NULL);
        cerr << "couldn't open /proc/" << my_handle << "/mem" << endl;
        perror("open(memFile.c_str(),O_RDONLY)");
        return false;
    }
    else
    {
        attached = true;
        g_pProcess = this;
        g_ProcessHandle = my_handle;
        
        g_ProcessMemFile = proc_pid_mem;
        return true; // we are attached
    }
}

bool Process::detach()
{
    if(!attached) return false;
    if(!suspended) suspend();
    int result = 0;
    // close /proc/PID/mem
    result = close(g_ProcessMemFile);
    if(result == -1)
    {
        cerr << "couldn't close /proc/"<< my_handle <<"/mem" << endl;
        perror("mem file close");
        return false;
    }
    else
    {
        // detach
        g_ProcessMemFile = -1;
        result = ptrace(PTRACE_DETACH, my_handle, NULL, NULL);
        if(result == -1)
        {
            cerr << "couldn't detach from process pid" << my_handle << endl;
            perror("ptrace detach");
            return false;
        }
        else
        {
            attached = false;
            g_pProcess = NULL;
            g_ProcessHandle = 0;
            return true;
        }
    }
}



void Process::freeResources()
{
    // nil
};


#else
/*
 *     WINDOWS PART
 */

bool Process::suspend()
{
    if(!attached)
        return false;
    if(suspended)
    {
        return true;
    }
    vector<uint32_t> threads;
    getThreadIDs( threads );
    for(int i = 0; i < /*threads.size()*/ 1; i++)
    {
        HANDLE thread_handle = OpenThread(THREAD_ALL_ACCESS, FALSE, (DWORD) threads[i]);
        SuspendThread(thread_handle);
        CloseHandle(thread_handle);
    }
    suspended = true;
    return true;
}

bool Process::resume()
{
    if(!attached)
        return false;
    if(!suspended)
    {
        return true;
    }
    vector<uint32_t> threads;
    getThreadIDs( threads );
    for(int i = 0; i < /* threads.size() */ 1; i++)
    {
        HANDLE thread_handle = OpenThread(THREAD_ALL_ACCESS, FALSE, (DWORD) threads[i]);
        ResumeThread(thread_handle);
        CloseHandle(thread_handle);
    }
    suspended = false;
    return true;
}

bool Process::attach()
{
    if(attached)
    {
        return false;
    }
    attached = true;
    g_pProcess = this;
    g_ProcessHandle = my_handle;
    suspend();

    return true;
}


bool Process::detach()
{
    if(!attached)
    {
        return false;
    }
    resume();
    attached = false;
    g_pProcess = NULL;
    g_ProcessHandle = 0;
    return true;
}

bool Process::getThreadIDs(vector<uint32_t> & threads )
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
        if( te32.th32OwnerProcessID == my_pid )
        {
            threads.push_back(te32.th32ThreadID);
        }
    } while( Thread32Next(AllThreads, &te32 ) ); 
    
    CloseHandle( AllThreads );
    return true;
}


void Process::getMemRanges( vector<t_memrange> & ranges )
{
    // code here is taken from hexsearch by Silas Dunmore.
    // As this IMHO isn't a 'sunstantial portion' of anything, I'm not including the MIT license here
    
    // I'm faking this, because there's no way I'm using VirtualQuery
    
    t_memrange temp;
    uint32_t base = this->my_descriptor->getBase();
    temp.start = base + 0x1000; // more fakery.
    temp.end = base + MreadDWord(base+MreadDWord(base+0x3C)+0x50)-1; // yay for magic.
    temp.read = 1;
    temp.write = 1;
    temp.execute = 0; // fake
    strcpy(temp.name,"pants");// that's right. I'm calling it pants. Windows can go to HELL
    ranges.push_back(temp);
}

void Process::freeResources()
{
    // opened by process manager
    CloseHandle(my_handle);
};
#endif

