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

bool ProcessEnumerator::findProcessess()
{
    DIR *dir_p;
    struct dirent *dir_entry_p;
    string dir_name;
    string exe_link;
    string cwd_link;
    string cmdline_path;
    string cmdline;

    // ALERT: buffer overrun potential

    int errorcount;
    int result;

    errorcount=0;
    result=0;
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
        Process *p = new Process(atoi(dir_entry_p->d_name),d->meminfo->meminfo);
        if(p->isIdentified())
        {
            d->processes.push_back(p);
        }
        else
        {
            delete p;
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
