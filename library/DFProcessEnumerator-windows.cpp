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

/// HACK: global variables (only one process can be attached at the same time.)
Process * DFHack::g_pProcess; ///< current process. non-NULL when picked
ProcessHandle DFHack::g_ProcessHandle; ///< cache of handle to current process. used for speed reasons
int DFHack::g_ProcessMemFile; ///< opened /proc/PID/mem, valid when attached

class DFHack::ProcessEnumerator::Private
{
    public:
        Private(){};
        MemInfoManager *meminfo;
        std::vector<Process *> processes;
};

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

// WINDOWS version of the process finder
bool ProcessEnumerator::findProcessess()
{
    // Get the list of process identifiers.
    DWORD ProcArray[2048], memoryNeeded, numProccesses;

    EnableDebugPriv();
    if ( !EnumProcesses( ProcArray, sizeof(ProcArray), &memoryNeeded ) )
    {
        return false;
    }

    // Calculate how many process identifiers were returned.
    numProccesses = memoryNeeded / sizeof(DWORD);

    // iterate through processes
    for ( int i = 0; i < (int)numProccesses; i++ )
    {
        Process *p = new Process(ProcArray[i],d->meminfo->meminfo);
        if(p->isIdentified())
        {
            d->processes.push_back(p);
        }
        else
        {
            delete p;
        }
    }
    if(d->processes.size())
        return true;
    return false;
}

uint32_t ProcessEnumerator::size()
{
    return d->processes.size();
};


Process * ProcessEnumerator::operator[](uint32_t index)
{
    assert(index < d->processes.size());
    return d->processes[index];
};


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
