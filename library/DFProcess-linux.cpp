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
#include "Internal.h"
#include "dfhack/DFProcess.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/DFError.h"
#include <errno.h>
#include <sys/ptrace.h>
using namespace DFHack;

class NormalProcess::Private
{
    public:
    Private(Process * self_)
    {
        my_descriptor = NULL;
        my_handle = NULL;
        my_pid = 0;
        attached = false;
        suspended = false;
        memFileHandle = 0;
        self = self_;
    };
    ~Private(){};
    Window* my_window;
    VersionInfo * my_descriptor;
    pid_t my_handle;
    uint32_t my_pid;
    string memFile;
    int memFileHandle;
    bool attached;
    bool suspended;
    bool identified;
    Process * self;
    bool validate(char * exe_file, uint32_t pid, char * mem_file, vector <VersionInfo *> & known_versions);
};

NormalProcess::NormalProcess(uint32_t pid, vector< VersionInfo* >& known_versions)
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

    // is this the regular linux DF?
    if (strstr(target_name, "dwarfort.exe") != 0 || strstr(target_name,"Dwarf_Fortress") != 0)
    {
        // create linux process, add it to the vector
        d->identified = d->validate(target_name,pid,mem_name,known_versions );
        return;
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

bool NormalProcess::Private::validate(char * exe_file,uint32_t pid, char * memFile, vector <VersionInfo *> & known_versions)
{
    md5wrapper md5;
    // get hash of the running DF process
    string hash = md5.getHashFromFile(exe_file);
    vector<VersionInfo *>::iterator it;

    // iterate over the list of memory locations
    for ( it=known_versions.begin() ; it < known_versions.end(); it++ )
    {
        try
        {
            //cout << hash << " ?= " << (*it)->getMD5() << endl;
            if(hash == (*it)->getMD5()) // are the md5 hashes the same?
            {
                VersionInfo * m = *it;
                if (VersionInfo::OS_LINUX == m->getOS())
                {
                    VersionInfo *m2 = new VersionInfo(*m);
                    my_descriptor = m2;
                    m2->setParentProcess(dynamic_cast<Process *>( self ));
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
        catch (Error::MissingMemoryDefinition&)
        {
            continue;
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
    // destroy our copy of the memory descriptor
    if(d->my_descriptor)
        delete d->my_descriptor;
    delete d;
}

VersionInfo * NormalProcess::getDescriptor()
{
    return d->my_descriptor;
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

bool NormalProcess::asyncSuspend()
{
    return suspend();
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

bool NormalProcess::detach()
{
    if(!d->attached) return true;
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
void NormalProcess::read (const uint32_t offset, const uint32_t size, uint8_t *target)
{
    if(size == 0) return;

    ssize_t result;
    result = pread(d->memFileHandle, target,size,offset);
    if(result != size)
    {
        if(result == -1)
        {
            cerr << "pread failed: can't read 0x" << hex << size << " bytes at address 0x" << offset << endl;
            cerr << "errno: " << errno << endl;
            errno = 0;
            throw Error::MemoryAccessDenied();
        }
        else
        {
            this->read(offset + result, size - result, target + result);
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

float NormalProcess::readFloat (const uint32_t offset)
{
    float val;
    read(offset, 4, (uint8_t *) &val);
    return val;
}
void NormalProcess::readFloat (const uint32_t offset, float &val)
{
    read(offset, 4, (uint8_t *) &val);
}

uint64_t NormalProcess::readQuad (const uint32_t offset)
{
    uint64_t val;
    read(offset, 8, (uint8_t *) &val);
    return val;
}
void NormalProcess::readQuad (const uint32_t offset, uint64_t &val)
{
    read(offset, 8, (uint8_t *) &val);
}
/*
 * WRITING
 */

void NormalProcess::writeQuad (uint32_t offset, const uint64_t data)
{
    #ifdef HAVE_64_BIT
        ptrace(PTRACE_POKEDATA,d->my_handle, offset, data);
    #else
        ptrace(PTRACE_POKEDATA,d->my_handle, offset, (uint32_t) data);
        ptrace(PTRACE_POKEDATA,d->my_handle, offset+4, (uint32_t) (data >> 32));
    #endif
}

void NormalProcess::writeDWord (uint32_t offset, uint32_t data)
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
void NormalProcess::writeWord (uint32_t offset, uint16_t data)
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

void NormalProcess::writeByte (uint32_t offset, uint8_t data)
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
void NormalProcess::write (uint32_t offset, uint32_t size, uint8_t *source)
{
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

struct _Rep_base
{
    uint32_t       _M_length;
    uint32_t       _M_capacity;
    uint32_t        _M_refcount;
};

size_t NormalProcess::readSTLString (uint32_t offset, char * buffer, size_t bufcapacity)
{
    _Rep_base header;
    offset = readDWord(offset);
    read(offset - sizeof(_Rep_base),sizeof(_Rep_base),(uint8_t *)&header);
    size_t read_real = min((size_t)header._M_length, bufcapacity-1);// keep space for null termination
    read(offset,read_real,(uint8_t * )buffer);
    buffer[read_real] = 0;
    return read_real;
}

const string NormalProcess::readSTLString (uint32_t offset)
{
    _Rep_base header;

    offset = readDWord(offset);
    read(offset - sizeof(_Rep_base),sizeof(_Rep_base),(uint8_t *)&header);

    // FIXME: use char* everywhere, avoid string
    char * temp = new char[header._M_length+1];
    read(offset,header._M_length+1,(uint8_t * )temp);
    string ret(temp);
    delete temp;
    return ret;
}

string NormalProcess::readClassName (uint32_t vptr)
{
    int typeinfo = readDWord(vptr - 0x4);
    int typestring = readDWord(typeinfo + 0x4);
    string raw = readCString(typestring);
    size_t  start = raw.find_first_of("abcdefghijklmnopqrstuvwxyz");// trim numbers
    size_t end = raw.length();
    return raw.substr(start,end-start);
}
string NormalProcess::getPath()
{
    char cwd_name[256];
    char target_name[1024];
    int target_result;

    sprintf(cwd_name,"/proc/%d/cwd", getPID());
    // resolve /proc/PID/exe link
    target_result = readlink(cwd_name, target_name, sizeof(target_name));
    target_name[target_result] = '\0';
    return(string(target_name));
}
