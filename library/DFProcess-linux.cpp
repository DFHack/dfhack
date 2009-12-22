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
#include <errno.h>
using namespace DFHack;

class NormalProcess::Private
{
    public:
    Private()
    {
        my_datamodel = NULL;
        my_descriptor = NULL;
        my_handle = NULL;
        my_window = NULL;
        my_pid = 0;
        attached = false;
        suspended = false;
        memFileHandle = 0;
    };
    ~Private(){};
    DataModel* my_datamodel;
    DFWindow* my_window;
    memory_info * my_descriptor;
    ProcessHandle my_handle;
    uint32_t my_pid;
    string memFile;
    int memFileHandle;
    bool attached;
    bool suspended;
    bool identified;
    bool validate(char * exe_file, uint32_t pid, char * mem_file, vector <memory_info> & known_versions);
};

NormalProcess::NormalProcess(uint32_t pid, vector <memory_info> & known_versions)
: d(new Private())
{
    char dir_name [256];
    char exe_link_name [256];
    char mem_name [256];
    char cwd_name [256];
    char cmdline_name [256];
    char target_name[1024];
    int target_result;
    
    d->identified = false;
    
    sprintf(dir_name,"/proc/%d/", pid);
    sprintf(exe_link_name,"/proc/%d/exe", pid);
    sprintf(mem_name,"/proc/%d/mem", pid);
    sprintf(cwd_name,"/proc/%d/cwd", pid);
    sprintf(cmdline_name,"/proc/%d/cmdline", pid);
    
    // resolve /proc/PID/exe link
    target_result = readlink(exe_link_name, target_name, sizeof(target_name)-1);
    if (target_result == -1)
    {
        return;
    }
    // make sure we have a null terminated string...
    target_name[target_result] = 0;
    
    // is this the regular linux DF?
    if (strstr(target_name, "dwarfort.exe") != NULL)
    {
        // create linux process, add it to the vector
        d->identified = d->validate(target_name,pid,mem_name,known_versions );
        d->my_window = new DFWindow(this);
        return;
    }
    
    // FIXME: this fails when the wine process isn't started from the 'current working directory'. strip path data from cmdline
    // is this windows version of Df running in wine?
    if(strstr(target_name, "wine-preloader")!= NULL)
    {
        // get working directory
        target_result = readlink(cwd_name, target_name, sizeof(target_name)-1);
        target_name[target_result] = 0;
        
        // got path to executable, do the same for its name
        ifstream ifs ( cmdline_name , ifstream::in );
        string cmdline;
        getline(ifs,cmdline);
        if (cmdline.find("dwarfort-w.exe") != string::npos || cmdline.find("dwarfort.exe") != string::npos || cmdline.find("Dwarf Fortress.exe") != string::npos)
        {
            char exe_link[1024];
            // put executable name and path together
            sprintf(exe_link,"%s/%s",target_name,cmdline.c_str());
            
            // create wine process, add it to the vector
            d->identified = d->validate(exe_link,pid,mem_name,known_versions);
            d->my_window = new DFWindow(this);
            return;
        }
    }
}

bool NormalProcess::isSuspended()
{
    return d->suspended;
}
bool NormalProcess::isAttached()
{
    return d->attached;
}

bool NormalProcess::isIdentified()
{
    return d->identified;
}

bool NormalProcess::Private::validate(char * exe_file,uint32_t pid, char * memFile, vector <memory_info> & known_versions)
{
    md5wrapper md5;
    // get hash of the running DF process
    string hash = md5.getHashFromFile(exe_file);
    vector<memory_info>::iterator it;
    
    // iterate over the list of memory locations
    for ( it=known_versions.begin() ; it < known_versions.end(); it++ )
    {
        if(hash == (*it).getString("md5")) // are the md5 hashes the same?
        {
            memory_info * m = &*it;
            // df can run under wine on Linux
            if(memory_info::OS_WINDOWS == (*it).getOS())
            {
                my_datamodel =new DMWindows40d();
                my_descriptor = m;
                my_handle = my_pid = pid;
            }
            else if (memory_info::OS_LINUX == (*it).getOS())
            {
                my_datamodel =new DMLinux40d();
                my_descriptor = m;
                my_handle = my_pid = pid;
            }
            else
            {
                // some error happened, continue with next process
                continue;
            }
            // tell NormalProcess about the /proc/PID/mem file
            this->memFile = memFile;
            identified = true;
            return true;
        }
    }
    return false;
}

NormalProcess::~NormalProcess()
{
    if(d->attached)
    {
        detach();
    }
    // destroy data model. this is assigned by processmanager
    if(d->my_datamodel)
        delete d->my_datamodel;
    if(d->my_window)
        delete d->my_window;
    delete d;
}


DataModel *NormalProcess::getDataModel()
{
    return d->my_datamodel;
}

memory_info * NormalProcess::getDescriptor()
{
    return d->my_descriptor;
}

DFWindow * NormalProcess::getWindow()
{
    return d->my_window;
}

int NormalProcess::getPID()
{
    return d->my_pid;
}

//FIXME: implement
bool NormalProcess::getThreadIDs(vector<uint32_t> & threads )
{
    return false;
}

//FIXME: cross-reference with ELF segment entries?
void NormalProcess::getMemRanges( vector<t_memrange> & ranges )
{
    char buffer[1024];
    char permissions[5]; // r/-, w/-, x/-, p/s, 0
    
    sprintf(buffer, "/proc/%lu/maps", d->my_pid);
    FILE *mapFile = ::fopen(buffer, "r");
    uint64_t offset, device1, device2, node;
    
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

bool NormalProcess::suspend()
{
    int status;
    if(!d->attached)
        return false;
    if(d->suspended)
        return true;
    if (kill(d->my_handle, SIGSTOP) == -1)
    {
        // no, we got an error
        perror("kill SIGSTOP error");
        return false;
    }
    while(true)
    {
        // we wait on the pid
        pid_t w = waitpid(d->my_handle, &status, 0);
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
    d->suspended = true;
    return true;
}

bool NormalProcess::forceresume()
{
    return resume();
}

bool NormalProcess::resume()
{
    if(!d->attached)
        return false;
    if(!d->suspended)
        return true;
    if (ptrace(PTRACE_CONT, d->my_handle, NULL, NULL) == -1)
    {
        // no, we got an error
        perror("ptrace resume error");
        return false;
    }
    d->suspended = false;
    return true;
}


bool NormalProcess::attach()
{
    int status;
    if(g_pProcess != NULL)
    {
        return false;
    }
    // can we attach?
    if (ptrace(PTRACE_ATTACH , d->my_handle, NULL, NULL) == -1)
    {
        // no, we got an error
        perror("ptrace attach error");
        cerr << "attach failed on pid " << d->my_handle << endl;
        return false;
    }
    while(true)
    {
        // we wait on the pid
        pid_t w = waitpid(d->my_handle, &status, 0);
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
    d->suspended = true;
    
    int proc_pid_mem = open(d->memFile.c_str(),O_RDONLY);
    if(proc_pid_mem == -1)
    {
        ptrace(PTRACE_DETACH, d->my_handle, NULL, NULL);
        cerr << "couldn't open /proc/" << d->my_handle << "/mem" << endl;
        perror("open(memFile.c_str(),O_RDONLY)");
        return false;
    }
    else
    {
        d->attached = true;
        g_pProcess = this;
        
        d->memFileHandle = proc_pid_mem;
        return true; // we are attached
    }
}

bool NormalProcess::detach()
{
    if(!d->attached) return false;
    if(!d->suspended) suspend();
    int result = 0;
    // close /proc/PID/mem
    result = close(d->memFileHandle);
    if(result == -1)
    {
        cerr << "couldn't close /proc/"<< d->my_handle <<"/mem" << endl;
        perror("mem file close");
        return false;
    }
    else
    {
        // detach
        result = ptrace(PTRACE_DETACH, d->my_handle, NULL, NULL);
        if(result == -1)
        {
            cerr << "couldn't detach from process pid" << d->my_handle << endl;
            perror("ptrace detach");
            return false;
        }
        else
        {
            d->attached = false;
            g_pProcess = NULL;
            return true;
        }
    }
}


// danger: uses recursion!
void NormalProcess::read (const uint32_t offset, const uint32_t size, uint8_t *target)
{
    if(size == 0) return;
    
    ssize_t result;
    result = pread(d->memFileHandle, target,size,offset);
    if(result != size)
    {
        if(result == -1)
        {
            cerr << "pread failed: can't read " << size << " bytes at addres " << offset << endl;
            cerr << "errno: " << errno << endl;
            errno = 0;
        }
        else
        {
            read(offset + result, size - result, target + result);
        }
    }
}

uint8_t NormalProcess::readByte (const uint32_t offset)
{
    uint8_t val;
    read(offset, 1, &val);
    return val;
}

void NormalProcess::readByte (const uint32_t offset, uint8_t &val )
{
    read(offset, 1, &val);
}

uint16_t NormalProcess::readWord (const uint32_t offset)
{
    uint16_t val;
    read(offset, 2, (uint8_t *) &val);
    return val;
}

void NormalProcess::readWord (const uint32_t offset, uint16_t &val)
{
    read(offset, 2, (uint8_t *) &val);
}

uint32_t NormalProcess::readDWord (const uint32_t offset)
{
    uint32_t val;
    read(offset, 4, (uint8_t *) &val);
    return val;
}
void NormalProcess::readDWord (const uint32_t offset, uint32_t &val)
{
    read(offset, 4, (uint8_t *) &val);
}

/*
 * WRITING
 */

void NormalProcess::writeDWord (uint32_t offset, uint32_t data)
{
    ptrace(PTRACE_POKEDATA,d->my_handle, offset, data);
}

// using these is expensive.
void NormalProcess::writeWord (uint32_t offset, uint16_t data)
{
    uint32_t orig = readDWord(offset);
    orig &= 0xFFFF0000;
    orig |= data;
    /*
    orig |= 0x0000FFFF;
    orig &= data;
    */
    ptrace(PTRACE_POKEDATA,d->my_handle, offset, orig);
}

void NormalProcess::writeByte (uint32_t offset, uint8_t data)
{
    uint32_t orig = readDWord(offset);
    orig &= 0xFFFFFF00;
    orig |= data;
    /*
    orig |= 0x000000FF;
    orig &= data;
    */
    ptrace(PTRACE_POKEDATA,d->my_handle, offset, orig);
}

// blah. I hate the kernel devs for crippling /proc/PID/mem. THIS IS RIDICULOUS
void NormalProcess::write (uint32_t offset, uint32_t size, const uint8_t *source)
{
    uint32_t indexptr = 0;
    while (size > 0)
    {
        // default: we push 4 bytes
        if(size >= 4)
        {
            writeDWord(offset, *(uint32_t *) (source + indexptr));
            offset +=4;
            indexptr +=4;
            size -=4;
        }
        // last is either three or 2 bytes
        else if(size >= 2)
        {
            writeWord(offset, *(uint16_t *) (source + indexptr));
            offset +=2;
            indexptr +=2;
            size -=2;
        }
        // finishing move
        else if(size == 1)
        {
            writeByte(offset, *(uint8_t *) (source + indexptr));
            return;
        }
    }
}

const std::string NormalProcess::readCString (uint32_t offset)
{
    std::string temp;
    char temp_c[256];
    int counter = 0;
    char r;
    do
    {
        r = readByte(offset+counter);
        temp_c[counter] = r;
        counter++;
    } while (r && counter < 255);
    temp_c[counter] = 0;
    temp = temp_c;
    return temp;
}

