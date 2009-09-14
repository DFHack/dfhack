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

#include "DFCommon.h"
#ifdef LINUX_BUILD
#include <sys/wait.h>
#endif


Process::Process(DataModel * dm, memory_info* mi, ProcessHandle ph, uint32_t pid)
{
    my_datamodel = dm;
    my_descriptor = mi;
    my_handle = ph;
    my_pid = pid;
    attached = false;
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
bool Process::attach()
{
    // TODO: check for errors!
    if(g_pProcess != NULL)
    {
        return false;
    }

    ptrace(PTRACE_ATTACH , my_handle, NULL, NULL);
    wait(NULL); // wait for DF to be stopped.
    attached = true;

    // HACK: Set the global process variables
    g_pProcess = this;
    g_ProcessHandle = my_handle;
    g_ProcessMemFile = fopen(memFile.c_str(),"rb");
    return true; // we are attached
}


bool Process::detach()
{
    // TODO: check for errors.
    ptrace(PTRACE_DETACH, my_handle, NULL, NULL);
    attached = false;

    g_pProcess = NULL;
    g_ProcessHandle = 0;
    fclose(g_ProcessMemFile);// close /proc/PID/mem
    g_ProcessMemFile = NULL;
    return true;
}


void Process::freeResources()
{
    // nil
};


#else
/*
 *     WINDOWS PART
 */

//FIXME: should support stopping and resuming the process

bool Process::attach()
{
    if(attached)
    {
        return false;
    }
    /*
    if(DebugActiveProcess(my_pid))
    {
    */
        attached = true;
        g_pProcess = this;
        g_ProcessHandle = my_handle;

        return true;
    /*}
    return false;*/
}


bool Process::detach()
{
    if(!attached)
    {
        return false;
    }
    //TODO: find a safer way to suspend processes
    /*if(DebugActiveProcessStop(my_pid))
    {*/
        attached = false;
        g_pProcess = NULL;
        g_ProcessHandle = 0;
        return true;
    /*}
    return false;*/
}


void Process::freeResources()
{
    // opened by process manager
    CloseHandle(my_handle);
};
#endif

