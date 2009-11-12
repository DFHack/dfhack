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


/*
//TODO: rewrite this. really. It's ugly as hell.
bool isStopped(pid_t pid)
{
    char filename[256];
    sprintf(filename, "/proc/%d/status", pid);
    
    // evil mess, that's a fitting name for this thing
    FILE* evil = fopen(filename,"rb");
    if(evil)
    {
        // zlo means evil in czech.
        char zlo[64];
        char test;
        // read first line, ignore
        fgets(zlo,64,evil);
        // read second line
        fgets(zlo,64,evil);
        sscanf(zlo,"State: %c",&test);
        fclose(evil);
        if(test == 'T')
        {
            return true;
        }
        return false;
    }
    cerr << "couldn't open file " << filename << "assuming process stopped" << endl;
    return true;
}
*/
bool Process::attach()
{
    int status;
    //cout << "Attach: start" << endl;
    // check if another process is attached
    if(g_pProcess != NULL)
    {
        return false;
    }
    /*
    if(!isStopped(my_handle))
    {
        // kill (SIGSTOP)
        status = kill(my_handle,SIGSTOP);
        //cout << "sent SIGSTOP" << endl;
        if(status == -1)
        {
            perror("kill(SIGSTOP)");
            return false;
        }
        // wait for the process to stop
        while (!isStopped(my_handle))
        {
            usleep(5000);
            //cout << "wait step" << endl;
        }
    }
    */
    //usleep(10000);
    //cout << "Attach: after conditional stop" << endl;
    // can we attach?
    if (ptrace(PTRACE_ATTACH , my_handle, NULL, NULL) == -1)
    {
        // no, we got an error
        perror("ptrace attach error");
        cerr << "attach failed on pid " << my_handle << endl;
        return false;
    }
    //usleep(10000);
    //cout << "Attach: after ptrace" << endl;
    while(true)
    {
        // we wait on the pid
        pid_t w = waitpid(my_handle, &status, 0);
        if (w == -1)
        {
            // child died
            perror("wait inside attach()");
            //LOGC << "child died?";
            //exit(-1);
            return false;
        }
        // stopped -> let's continue
        if (WIFSTOPPED(status))
        {
            break;
        }
    }
    //  cout << "Managed to attach to pid " << my_handle << endl;
    
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
        //cout << "Attach: after opening /proc/"<< my_handle <<"/mem" << endl;
        return true; // we are attached
    }
}

bool Process::detach()
{
    if(!attached) return false;
    int result = 0;
    //cout << "detach: start" << endl;
    result = close(g_ProcessMemFile);// close /proc/PID/mem
    if(result == -1)
    {
        cerr << "couldn't close /proc/"<< my_handle <<"/mem" << endl;
        perror("mem file close");
        return false;
    }
    else
    {
        //cout << "detach: after closing /proc/"<< my_handle <<"/mem" << endl;
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
         //   cout << "detach: after detaching from "<< my_handle << endl;
            attached = false;
            g_pProcess = NULL;
            g_ProcessHandle = 0;
            // continue, wait for it to recover
    //        kill(my_handle,SIGCONT);
      //      while (isStopped(my_handle));
            //usleep(10000);
            // we finish
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

