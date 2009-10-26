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

#ifndef PROCESSMANAGER_H_INCLUDED
#define PROCESSMANAGER_H_INCLUDED

#ifdef LINUX_BUILD
typedef pid_t ProcessHandle;
#else
typedef HANDLE ProcessHandle;
#endif

class memory_info;
class DataModel;
class TiXmlElement;
class Process;

/*
* Currently attached process and its handle
*/
extern Process * g_pProcess; ///< current process. non-NULL when picked
extern ProcessHandle g_ProcessHandle; ///< cache of handle to current process. used for speed reasons
extern int g_ProcessMemFile; ///< opened /proc/PID/mem, valid when attached

class Process
{
    friend class ProcessManager;
    protected:
        Process(DataModel * dm, memory_info* mi, ProcessHandle ph, uint32_t pid);
        ~Process();
        DataModel* my_datamodel;
        memory_info * my_descriptor;
        ProcessHandle my_handle;
        uint32_t my_pid;
        string memFile;
        bool attached;
        void freeResources();
        void setMemFile(const string & memf);
    public:
        // Set up stuff so we can read memory
        bool attach();
        bool detach();
        // is the process still there?
        memory_info *getDescriptor();
        DataModel *getDataModel();
};

/*
 * Process manager
 */
class ProcessManager
{
public:
    ProcessManager( string path_to_xml);
    ~ProcessManager();
    bool findProcessess();
    uint32_t size();
    Process * operator[](uint32_t index);

private:
    // memory info entries loaded from a file
    std::vector<memory_info> meminfo;
    // vector to keep track of dynamically created memory_info entries
    std::vector<memory_info *> destroy_meminfo;
    Process * currentProcess;
    ProcessHandle currentProcessHandle;
    std::vector<Process *> processes;
    bool loadDescriptors( string path_to_xml);
    void ParseVTable(TiXmlElement* vtable, memory_info& mem);
    void ParseEntry (TiXmlElement* entry, memory_info& mem, map <string ,TiXmlElement *>& knownEntries);
#ifdef LINUX_BUILD
    Process* addProcess(const string & exe,ProcessHandle PH,const string & memFile);
#endif
};

#endif // PROCESSMANAGER_H_INCLUDED
