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

#ifndef BUILD_DFHACK_LIB
#   define BUILD_DFHACK_LIB
#endif

#include "DFCommon.h"

#include "DFDataModel.h"
#include "DFMemInfo.h"

#include "tinyxml/tinyxml.h"
#include <iostream>

/// HACK: global variables (only one process can be attached at the same time.)
Process * g_pProcess; ///< current process. non-NULL when picked
ProcessHandle g_ProcessHandle; ///< cache of handle to current process. used for speed reasons
int g_ProcessMemFile; ///< opened /proc/PID/mem, valid when attached


#ifdef LINUX_BUILD
/*
 *  LINUX version of the process finder.
 */

#include "md5/md5wrapper.h"

Process* ProcessManager::addProcess(const string & exe,ProcessHandle PH, const string & memFile)
{
    md5wrapper md5;
    // get hash of the running DF process
    string hash = md5.getHashFromFile(exe);
    vector<memory_info>::iterator it;

    // iterate over the list of memory locations
    for ( it=meminfo.begin() ; it < meminfo.end(); it++ )
    {
        if(hash == (*it).getString("md5")) // are the md5 hashes the same?
        {
            memory_info * m = &*it;
            Process * ret;
            //cout <<"Found process " << PH <<  ". It's DF version " << m->getVersion() << "." << endl;

            // df can run under wine on Linux
            if(memory_info::OS_WINDOWS == (*it).getOS())
            {
                ret= new Process(new DMWindows40d(),m,PH, PH);
            }
            else if (memory_info::OS_LINUX == (*it).getOS())
            {
                ret= new Process(new DMLinux40d(),m,PH, PH);
            }
            else
            {
                // some error happened, continue with next process
                continue;
            }
            // tell Process about the /proc/PID/mem file
            ret->setMemFile(memFile);
            processes.push_back(ret);
            return ret;
        }
    }
    return NULL;
}

bool ProcessManager::findProcessess()
{
    DIR *dir_p;
    struct dirent *dir_entry_p;
    string dir_name;
    string exe_link;
    string cwd_link;
    string cmdline_path;
    string cmdline;

    // ALERT: buffer overrun potential
    char target_name[1024];
    int target_result;
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

        // string manipulation - get /proc/PID/exe link and /proc/PID/mem names
        dir_name = "/proc/";
        dir_name += dir_entry_p->d_name;
        dir_name += "/";
        exe_link = dir_name + "exe";
        string mem_name = dir_name + "mem";

        // resolve /proc/PID/exe link
        target_result = readlink(exe_link.c_str(), target_name, sizeof(target_name)-1);
        if (target_result == -1)
        {
            // bad result from link resolution, continue with another processed
            continue;
        }
        // make sure we have a null terminated string...
        target_name[target_result] = 0;

        // is this the regular linux DF?
        if (strstr(target_name, "dwarfort.exe") != NULL)
        {
            exe_link = target_name;
            // get PID
            result = atoi(dir_entry_p->d_name);
            // create linux process, add it to the vector
            addProcess(exe_link,result,mem_name);
            // continue with next process
            continue;
        }

        // FIXME: this fails when the wine process isn't started from the 'current working directory'. strip path data from cmdline
        // is this windows version of Df running in wine?
        if(strstr(target_name, "wine-preloader")!= NULL)
        {
            // get working directory
            cwd_link = dir_name + "cwd";
            target_result = readlink(cwd_link.c_str(), target_name, sizeof(target_name)-1);
            target_name[target_result] = 0;

            // got path to executable, do the same for its name
            cmdline_path = dir_name + "cmdline";
            ifstream ifs ( cmdline_path.c_str() , ifstream::in );
            getline(ifs,cmdline);
            if (cmdline.find("dwarfort.exe") != string::npos || cmdline.find("Dwarf Fortress.exe") != string::npos)
            {
                // put executable name and path together
                exe_link = target_name;
                exe_link += "/";
                exe_link += cmdline;

                // get PID
                result = atoi(dir_entry_p->d_name);

                // create wine process, add it to the vector
                addProcess(exe_link,result,mem_name);
            }
        }
    }
    closedir(dir_p);
    // return value depends on if we found some DF processes
    if(processes.size())
    {
        return true;
    }
    return false;
}

#else

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
bool ProcessManager::findProcessess()
{
    // Get the list of process identifiers.
    //TODO: make this dynamic. (call first to get the array size and second to really get process handles)
    DWORD ProcArray[512], memoryNeeded, numProccesses;
    HMODULE hmod = NULL;
    DWORD junk;
    HANDLE hProcess;
    bool found = false;

    IMAGE_NT_HEADERS32 pe_header;
    IMAGE_SECTION_HEADER sections[16];

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
        found = false;
        
        // open process
        hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, ProcArray[i] );
        if (NULL == hProcess)
            continue;
        
        // try getting the first module of the process
        if(EnumProcessModules(hProcess, &hmod, 1 * sizeof(HMODULE), &junk) == 0)
        {
            CloseHandle(hProcess);
            continue;
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
        for ( it=meminfo.begin() ; it < meminfo.end(); it++ )
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
                // give the process a data model and memory layout fixed for the base of first module
                memory_info *m = new memory_info(*it);
                m->RebaseAll(base);
                // keep track of created memory_info objects so we can destroy them later
                destroy_meminfo.push_back(m);
                // process is responsible for destroying its data model
                Process *ret= new Process(new DMWindows40d(),m,hProcess, ProcArray[i]);
                processes.push_back(ret);
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
	if(processes.size())
        return true;
    return false;
}
#endif


void ProcessManager::ParseVTable(TiXmlElement* vtable, memory_info& mem)
{
    TiXmlElement* pClassEntry;
    TiXmlElement* pClassSubEntry;
    // check for rebase, do rebase if check positive
    const char * rebase = vtable->Attribute("rebase");
    if(rebase)
    {
        int32_t rebase_offset = strtol(rebase, NULL, 16);
        mem.RebaseVTable(rebase_offset);
    }
    // parse vtable entries
    pClassEntry = vtable->FirstChildElement();
    for(;pClassEntry;pClassEntry=pClassEntry->NextSiblingElement())
    {
        string type = pClassEntry->Value();
        const char *cstr_name = pClassEntry->Attribute("name");
        const char *cstr_vtable = pClassEntry->Attribute("vtable");
        // it's a simple class
        if(type== "class")
        {
            mem.setClass(cstr_name, cstr_vtable);
        }
        // it's a multi-type class
        else if (type == "multiclass")
        {
            // get offset of the type variable
            const char *cstr_typeoffset = pClassEntry->Attribute("typeoffset");
            int mclass = mem.setMultiClass(cstr_name, cstr_vtable, cstr_typeoffset);
            // parse class sub-entries
            pClassSubEntry = pClassEntry->FirstChildElement();
            for(;pClassSubEntry;pClassSubEntry=pClassSubEntry->NextSiblingElement())
            {
                type = pClassSubEntry->Value();
                if(type== "class")
                {
                    // type is a value loaded from type offset
                    cstr_name = pClassSubEntry->Attribute("name");
                    const char *cstr_value = pClassSubEntry->Attribute("type");
                    mem.setMultiClassChild(mclass,cstr_name,cstr_value);
                }
            }
        }
    }
}



void ProcessManager::ParseEntry (TiXmlElement* entry, memory_info& mem, map <string ,TiXmlElement *>& knownEntries)
{
    TiXmlElement* pMemEntry;
    const char *cstr_version = entry->Attribute("version");
    const char *cstr_os = entry->Attribute("os");
    const char *cstr_base = entry->Attribute("base");
    const char *cstr_rebase = entry->Attribute("rebase");
    if(cstr_base)
    {
        string base = cstr_base;
        ParseEntry(knownEntries[base], mem, knownEntries);
    }
    
    // mandatory attributes missing?
    if(!(cstr_version && cstr_os))
    {
        cerr << "Bad entry in memory.xml detected, version or os attribute is missing.";
        // skip if we don't have valid attributes
        return;
    }
    string os = cstr_os;
    mem.setVersion(cstr_version);
    mem.setOS(cstr_os);

    // offset inherited addresses by 'rebase'.
    int32_t rebase = 0;
    if(cstr_rebase)
    {
        rebase = mem.getBase() + strtol(cstr_rebase, NULL, 16);
        mem.RebaseAddresses(rebase);
    }

    //set base to default, we're overwriting this because the previous rebase could cause havoc on Vista/7
    if(os == "windows")
    {
        // set default image base. this is fixed for base relocation later
        mem.setBase(0x400000);
    }
    else if(os == "linux")
    {
        // this is wrong... I'm not going to do base image relocation on linux though.
        // users are free to use a sane kernel that doesn't do this kind of **** by default
        mem.setBase(0x0);
    }
    else
    {
        cerr << "unknown operating system " << os << endl;
        return;
    }
    
    // process additional entries
    //cout << "Entry " << cstr_version << " " <<  cstr_os << endl;
    pMemEntry = entry->FirstChildElement()->ToElement();
    for(;pMemEntry;pMemEntry=pMemEntry->NextSiblingElement())
    {
        // only elements get processed
        const char *cstr_type = pMemEntry->Value();
        const char *cstr_name = pMemEntry->Attribute("name");
        const char *cstr_value = pMemEntry->GetText();
        // check for missing parts
        string type, name, value;
        type = cstr_type;
        if(type == "VTable")
        {
            ParseVTable(pMemEntry, mem);
            continue;
        }
        if( !(cstr_name && cstr_value))
        {
            cerr << "underspecified MemInfo entry" << endl;
            continue;
        }
        name = cstr_name;
        value = cstr_value;
        if (type == "HexValue")
        {
            mem.setHexValue(name, value);
        }
        else if (type == "Address")
        {
            mem.setAddress(name, value);
        }
        else if (type == "Offset")
        {
            mem.setOffset(name, value);
        }
        else if (type == "String")
        {
            mem.setString(name, value);
        }
        else
        {
            cerr << "Unknown MemInfo type: " << type << endl;
        }
    } // for
} // method


// load the XML file with offsets
bool ProcessManager::loadDescriptors(string path_to_xml)
{
    TiXmlDocument doc( path_to_xml.c_str() );
    bool loadOkay = doc.LoadFile();
    TiXmlHandle hDoc(&doc);
    TiXmlElement* pElem;
    TiXmlHandle hRoot(0);
    memory_info mem;

    if ( loadOkay )
    {
        // block: name
        {
            pElem=hDoc.FirstChildElement().Element();
            // should always have a valid root but handle gracefully if it does
            if (!pElem)
            {
                cerr << "no pElem found" << endl;
                return false;
            }
            string m_name=pElem->Value();
            if(m_name != "DFExtractor")
            {
                cerr << "DFExtractor != " << m_name << endl;
                return false;
            }
            //cout << "got DFExtractor XML!" << endl;
            // save this for later
            hRoot=TiXmlHandle(pElem);
        }
        // transform elements
        {
            // trash existing list
            meminfo.clear();
            TiXmlElement* pMemInfo=hRoot.FirstChild( "MemoryDescriptors" ).FirstChild( "Entry" ).Element();
            map <string ,TiXmlElement *> map_pNamedEntries;
            vector <TiXmlElement *> v_pEntries;
            for( ; pMemInfo; pMemInfo=pMemInfo->NextSiblingElement("Entry"))
            {
                v_pEntries.push_back(pMemInfo);
                const char *id;
                if(id= pMemInfo->Attribute("id"))
                {
                    string str_id = id;
                    map_pNamedEntries[str_id] = pMemInfo;
                }
            }
            for(uint32_t i = 0; i< v_pEntries.size();i++)
            {
                memory_info mem;
                //FIXME: add a set of entries processed in a step of this cycle, use it to check for infinite loops
                /* recursive */ParseEntry( v_pEntries[i] , mem , map_pNamedEntries);
                meminfo.push_back(mem);
            }
            // process found things here
        }
        return true;
    }
    else
    {
        // load failed
        cerr << "Can't load memory offsets from memory.xml" << endl;
        return false;
    }
}


uint32_t ProcessManager::size()
{
    return processes.size();
};


Process * ProcessManager::operator[](uint32_t index)
{
    assert(index < processes.size());
    return processes[index];
};


ProcessManager::ProcessManager( string path_to_xml )
{
    currentProcess = NULL;
    currentProcessHandle = 0;
    loadDescriptors( path_to_xml );
}


ProcessManager::~ProcessManager()
{
    // delete all processes
    for(uint32_t i = 0;i < processes.size();i++)
    {
        delete processes[i];
    }
    //delete all generated memory_info stuff
    for(uint32_t i = 0;i < destroy_meminfo.size();i++)
    {
        delete destroy_meminfo[i];
    }
}
