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
#include "DFProcess.h"
#include "DFWindow.h"
#include "DFMemInfo.h"
#include "DFError.h"
#include <errno.h>
#include <DFError.h>
#include <sys/ptrace.h>
#include <stdio.h>
using namespace DFHack;

class WineProcess::Private
{
    public:
    Private(Process * self_)
    {
        my_descriptor = NULL;
        my_handle = NULL;
        my_window = NULL;
        my_pid = 0;
        attached = false;
        suspended = false;
        memFileHandle = 0;
        self = self_;
    };
    ~Private(){};
    DFWindow* my_window;
    memory_info * my_descriptor;
    Process * self;
    pid_t my_handle;
    uint32_t my_pid;
    string memFile;
    int memFileHandle;
    bool attached;
    bool suspended;
    bool identified;
    bool validate(char * exe_file, uint32_t pid, char * mem_file, vector <memory_info *> & known_versions);
};

WineProcess::WineProcess(uint32_t pid, vector <memory_info *> & known_versions)
: d(new Private(this))
{
    char dir_name [256];
    char exe_link_name [256];
    char mem_name [256];
    char cwd_name [256];
    char cmdline_name [256];
    char target_name[1024];
    int target_result;
    
    d->identified = false;
    d->my_descriptor = 0;
    
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

bool WineProcess::isSuspended()
{
    return d->suspended;
}
bool WineProcess::isAttached()
{
    return d->attached;
}

bool WineProcess::isIdentified()
{
    return d->identified;
}

bool WineProcess::Private::validate(char* exe_file, uint32_t pid, char* mem_file, std::vector< memory_info* >& known_versions)
{
    md5wrapper md5;
    // get hash of the running DF process
    string hash = md5.getHashFromFile(exe_file);
    vector<memory_info *>::iterator it;
    
    // iterate over the list of memory locations
    for ( it=known_versions.begin() ; it < known_versions.end(); it++ )
    {
        string thishash;
        try
        {
            thishash = (*it)->getString("md5");
        }
        catch (Error::MissingMemoryDefinition& e)
        {
            continue;
        }
        // are the md5 hashes the same?
        if(memory_info::OS_WINDOWS == (*it)->getOS() && hash == thishash)
        {
            
            // keep track of created memory_info object so we can destroy it later
            memory_info *m = new memory_info(**it);
            my_descriptor = m;
            m->setParentProcess(dynamic_cast<Process *>( self ));
            my_handle = my_pid = pid;
            // tell WineProcess about the /proc/PID/mem file
            memFile = mem_file;
            identified = true;
            return true;
        }
    }
    return false;
}

WineProcess::~WineProcess()
{
    if(d->attached)
    {
        detach();
    }
    // destroy our copy of the memory descriptor
    if(d->my_descriptor)
        delete d->my_descriptor;
    if(d->my_window)
        delete d->my_window;
    delete d;
}

memory_info * WineProcess::getDescriptor()
{
    return d->my_descriptor;
}

DFWindow * WineProcess::getWindow()
{
    return d->my_window;
}

int WineProcess::getPID()
{
    return d->my_pid;
}

//FIXME: implement
bool WineProcess::getThreadIDs(vector<uint32_t> & threads )
{
    return false;
}

//FIXME: cross-reference with ELF segment entries?
void WineProcess::getMemRanges( vector<t_memrange> & ranges )
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

bool WineProcess::asyncSuspend()
{
    return suspend();
}

bool WineProcess::suspend()
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

bool WineProcess::forceresume()
{
    return resume();
}

bool WineProcess::resume()
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


bool WineProcess::attach()
{
    int status;
    if(d->attached)
    {
        if(!d->suspended)
            return suspend();
        return true;
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
        cerr << d->memFile << endl;
        cerr << "couldn't open /proc/" << d->my_handle << "/mem" << endl;
        perror("open(memFile.c_str(),O_RDONLY)");
        return false;
    }
    else
    {
        d->attached = true;
        
        d->memFileHandle = proc_pid_mem;
        return true; // we are attached
    }
}

bool WineProcess::detach()
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
            return true;
        }
    }
}


// danger: uses recursion!
void WineProcess::read (const uint32_t offset, const uint32_t size, uint8_t *target)
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
            throw Error::MemoryAccessDenied();
        }
        else
        {
            read(offset + result, size - result, target + result);
        }
    }
}

uint8_t WineProcess::readByte (const uint32_t offset)
{
    uint8_t val;
    read(offset, 1, &val);
    return val;
}

void WineProcess::readByte (const uint32_t offset, uint8_t &val )
{
    read(offset, 1, &val);
}

uint16_t WineProcess::readWord (const uint32_t offset)
{
    uint16_t val;
    read(offset, 2, (uint8_t *) &val);
    return val;
}

void WineProcess::readWord (const uint32_t offset, uint16_t &val)
{
    read(offset, 2, (uint8_t *) &val);
}

uint32_t WineProcess::readDWord (const uint32_t offset)
{
    uint32_t val;
    read(offset, 4, (uint8_t *) &val);
    return val;
}
void WineProcess::readDWord (const uint32_t offset, uint32_t &val)
{
    read(offset, 4, (uint8_t *) &val);
}

float WineProcess::readFloat (const uint32_t offset)
{
    float val;
    read(offset, 4, (uint8_t *) &val);
    return val;
}
void WineProcess::readFloat (const uint32_t offset, float &val)
{
    read(offset, 4, (uint8_t *) &val);
}

void WineProcess::readQuad (const uint32_t offset, uint64_t &val)
{
    read(offset, 8, (uint8_t *) &val);
}

uint64_t WineProcess::readQuad (const uint32_t offset)
{
    uint64_t val;
    read(offset, 8, (uint8_t *) &val);
    return val;
}

/*
 * WRITING
 */

void WineProcess::writeQuad (uint32_t offset, const uint64_t data)
{
    #ifdef HAVE_64_BIT
        ptrace(PTRACE_POKEDATA,d->my_handle, offset, data);
    #else
        ptrace(PTRACE_POKEDATA,d->my_handle, offset, (uint32_t) data);
        ptrace(PTRACE_POKEDATA,d->my_handle, offset+4, (uint32_t) (data >> 32));
    #endif
}

void WineProcess::writeDWord (uint32_t offset, uint32_t data)
{
    #ifdef HAVE_64_BIT
        uint64_t orig = readQuad(offset);
        orig &= 0xFFFFFFFF00000000;
        orig |= data;
        ptrace(PTRACE_POKEDATA,d->my_handle, offset, orig);
    #else
        ptrace(PTRACE_POKEDATA,d->my_handle, offset, data);
    #endif
}

// using these is expensive.
void WineProcess::writeWord (uint32_t offset, uint16_t data)
{
    #ifdef HAVE_64_BIT
        uint64_t orig = readQuad(offset);
        orig &= 0xFFFFFFFFFFFF0000;
        orig |= data;
        ptrace(PTRACE_POKEDATA,d->my_handle, offset, orig);
    #else
        uint32_t orig = readDWord(offset);
        orig &= 0xFFFF0000;
        orig |= data;
        ptrace(PTRACE_POKEDATA,d->my_handle, offset, orig);
    #endif
}

void WineProcess::writeByte (uint32_t offset, uint8_t data)
{
    #ifdef HAVE_64_BIT
        uint64_t orig = readQuad(offset);
        orig &= 0xFFFFFFFFFFFFFF00;
        orig |= data;
        ptrace(PTRACE_POKEDATA,d->my_handle, offset, orig);
    #else
        uint32_t orig = readDWord(offset);
        orig &= 0xFFFFFF00;
        orig |= data;
        ptrace(PTRACE_POKEDATA,d->my_handle, offset, orig);
    #endif
}

// blah. I hate the kernel devs for crippling /proc/PID/mem. THIS IS RIDICULOUS
void WineProcess::write (uint32_t offset, uint32_t size, uint8_t *source)
{
    uint32_t count = 0;
    uint32_t indexptr = 0;
    while (size > 0)
    {
        #ifdef HAVE_64_BIT
            // quad!
            if(size >= 8)
            {
                writeQuad(offset, *(uint64_t *) (source + indexptr));
                offset +=8;
                indexptr +=8;
                size -=8;
            }
            else
        #endif
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

const std::string WineProcess::readCString (uint32_t offset)
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

size_t WineProcess::readSTLString (uint32_t offset, char * buffer, size_t bufcapacity)
{
    /*
    MSVC++ string
    ptr allocator
    union
    {
        char[16] start;
        char * start_ptr
    }
    Uint32 length
    Uint32 capacity
    */
    uint32_t start_offset = offset + 4;
    size_t length = readDWord(offset + 20);
    
    size_t capacity = readDWord(offset + 24);
    size_t read_real = min(length, bufcapacity-1);// keep space for null termination
    
    // read data from inside the string structure
    if(capacity < 16)
    {
        read(start_offset, read_real , (uint8_t *)buffer);
    }
    else // read data from what the offset + 4 dword points to
    {
        start_offset = readDWord(start_offset);// dereference the start offset
        read(start_offset, read_real, (uint8_t *)buffer);
    }
    
    buffer[read_real] = 0;
    return read_real;
}

const string WineProcess::readSTLString (uint32_t offset)
{
    /*
        MSVC++ string
        ptr allocator
        union
        {
            char[16] start;
            char * start_ptr
        }
        Uint32 length
        Uint32 capacity
    */
    uint32_t start_offset = offset + 4;
    uint32_t length = readDWord(offset + 20);
    uint32_t capacity = readDWord(offset + 24);
    char * temp = new char[capacity+1];
    
    // read data from inside the string structure
    if(capacity < 16)
    {
        read(start_offset, capacity, (uint8_t *)temp);
    }
    else // read data from what the offset + 4 dword points to
    {
        start_offset = readDWord(start_offset);// dereference the start offset
        read(start_offset, capacity, (uint8_t *)temp);
    }
    
    temp[length] = 0;
    string ret = temp;
    delete temp;
    return ret;
}

string WineProcess::readClassName (uint32_t vptr)
{
    int rtti = readDWord(vptr - 0x4);
    int typeinfo = readDWord(rtti + 0xC);
    string raw = readCString(typeinfo + 0xC); // skips the .?AV
    raw.resize(raw.length() - 2);// trim @@ from end
    return raw;
}
