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

class Process::Private
{
    public:
        Private()
        {
            my_datamodel = NULL;
            my_descriptor = NULL;
            my_handle = NULL;
            my_main_thread = NULL;
            my_pid = 0;
            attached = false;
            suspended = false;
        };
        ~Private(){};
        DataModel* my_datamodel;
        memory_info * my_descriptor;
        ProcessHandle my_handle;
        HANDLE my_main_thread;
        uint32_t my_pid;
        string memFile;
        bool attached;
        bool suspended;
        bool identified;
};

Process::Process(uint32_t pid, vector <memory_info> & known_versions)
: d(new Private())
{
    HMODULE hmod = NULL;
    DWORD junk;
    HANDLE hProcess;
    bool found = false;
    
    IMAGE_NT_HEADERS32 pe_header;
    IMAGE_SECTION_HEADER sections[16];
    d->identified = false;
    // open process
    hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pid );
    if (NULL == hProcess)
        return;
    
    // try getting the first module of the process
    if(EnumProcessModules(hProcess, &hmod, 1 * sizeof(HMODULE), &junk) == 0)
    {
        CloseHandle(hProcess);
        return; //if enumprocessModules fails, give up
    }
    
    // got base ;)
    uint32_t base = (uint32_t)hmod;
    
    // read from this process
    g_ProcessHandle = hProcess;
    uint32_t pe_offset = MreadDWord(base+0x3C);
    Mread(base + pe_offset                   , sizeof(pe_header), (uint8_t *)&pe_header);
    Mread(base + pe_offset+ sizeof(pe_header), sizeof(sections) , (uint8_t *)&sections );
    
    // see if there's a version entry that matches this process
    vector<memory_info>::iterator it;
    for ( it=known_versions.begin() ; it < known_versions.end(); it++ )
    {
        // filter by OS
        if(memory_info::OS_WINDOWS != (*it).getOS())
            continue;
        
        // filter by timestamp
        uint32_t pe_timestamp = (*it).getHexValue("pe_timestamp");
        if (pe_timestamp != pe_header.FileHeader.TimeDateStamp)
            continue;
        
        // all went well
        {
            printf("Match found! Using version %s.\n", (*it).getVersion().c_str());
            d->identified = true;
            // give the process a data model and memory layout fixed for the base of first module
            memory_info *m = new memory_info(*it);
            m->RebaseAll(base);
            // keep track of created memory_info object so we can destroy it later
            d->my_descriptor = m;
            // process is responsible for destroying its data model
            d->my_datamodel = new DMWindows40d();
            d->my_pid = pid;
            d->my_handle = hProcess;
            d->identified = true;
            
            // TODO: detect errors in thread enumeration
            vector<uint32_t> threads;
            getThreadIDs( threads );
            d->my_main_thread = OpenThread(THREAD_ALL_ACCESS, FALSE, (DWORD) threads[0]);
            
            found = true;
            break; // break the iterator loop
        }
    }
    // close handle of processes that aren't DF
    if(!found)
    {
        CloseHandle(hProcess);
    }
}
/*
*/

Process::~Process()
{
    if(d->attached)
    {
        detach();
    }
    // destroy data model. this is assigned by processmanager
    delete d->my_datamodel;
    delete d->my_descriptor;
    if(d->my_handle != NULL)
    {
        CloseHandle(d->my_handle);
    }
    if(d->my_main_thread != NULL)
    {
        CloseHandle(d->my_main_thread);
    }
    delete d;
}


DataModel *Process::getDataModel()
{
    return d->my_datamodel;
}


memory_info * Process::getDescriptor()
{
    return d->my_descriptor;
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

bool Process::suspend()
{
    if(!d->attached)
        return false;
    if(d->suspended)
    {
        return true;
    }
    SuspendThread(d->my_main_thread);
    d->suspended = true;
    return true;
}

bool Process::forceresume()
{
    if(!d->attached)
        return false;
    while (ResumeThread(d->my_main_thread) > 1);
    d->suspended = false;
    return true;
}


bool Process::resume()
{
    if(!d->attached)
        return false;
    if(!d->suspended)
    {
        return true;
    }
    ResumeThread(d->my_main_thread);
    d->suspended = false;
    return true;
}

bool Process::attach()
{
    if(g_pProcess != NULL)
    {
        return false;
    }
    d->attached = true;
    g_pProcess = this;
    g_ProcessHandle = d->my_handle;
    suspend();

    return true;
}


bool Process::detach()
{
    if(!d->attached)
    {
        return false;
    }
    resume();
    d->attached = false;
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
        if( te32.th32OwnerProcessID == d->my_pid )
        {
            threads.push_back(te32.th32ThreadID);
        }
    } while( Thread32Next(AllThreads, &te32 ) ); 
    
    CloseHandle( AllThreads );
    return true;
}

//FIXME: use VirtualQuery to probe for memory ranges, cross-reference with base-corrected PE segment entries
void Process::getMemRanges( vector<t_memrange> & ranges )
{
    // code here is taken from hexsearch by Silas Dunmore.
    // As this IMHO isn't a 'sunstantial portion' of anything, I'm not including the MIT license here
    
    // I'm faking this, because there's no way I'm using VirtualQuery
    
    t_memrange temp;
    uint32_t base = d->my_descriptor->getBase();
    temp.start = base + 0x1000; // more fakery.
    temp.end = base + MreadDWord(base+MreadDWord(base+0x3C)+0x50)-1; // yay for magic.
    temp.read = 1;
    temp.write = 1;
    temp.execute = 0; // fake
    strcpy(temp.name,"pants");// that's right. I'm calling it pants. Windows can go to HELL
    ranges.push_back(temp);
}
