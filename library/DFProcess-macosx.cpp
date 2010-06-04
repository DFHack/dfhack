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
#include "dfhack/DFMemInfo.h"
#include "dfhack/DFError.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/sysctl.h>
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
        self = self_;
    };
    ~Private(){};
    Window* my_window;
    memory_info * my_descriptor;
    pid_t my_handle;
    uint32_t my_pid;
    bool attached;
    bool suspended;
    bool identified;
    Process * self;
    bool validate(uint32_t pid, vector <memory_info *> & known_versions);
};

NormalProcess::NormalProcess(uint32_t pid, vector< memory_info* >& known_versions)
: d(new Private(this))
{
    uint32_t sc_name[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, 0, 0 };

    kinfo_proc *kproc = NULL;
    size_t len;

    d->identified = false;
    d->my_descriptor = 0;
    
    sc_name[3] = pid;
    if (sysctl((int *) sc_name, (sizeof(sc_name)/sizeof(*sc_name)) - 1,
	       NULL, &len, NULL, 0) == -1) return;
    kproc = (kinfo_proc *) malloc(len);
    if (sysctl((int *) sc_name, (sizeof(sc_name)/sizeof(*sc_name)) - 1,
	       kproc, &len, NULL, 0) == -1) {
      free(kproc);
      return;
    }
    
    // is this the regular linux DF?
    if (strstr(kproc->kp_proc.p_comm, "dwarfort.exe") != NULL)
    {
        // create linux process, add it to the vector
        d->identified = d->validate(pid,known_versions );
    }
    free(kproc);
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

bool NormalProcess::Private::validate(uint32_t pid, vector <memory_info *> & known_versions)
{
    char lsof_cmd [256];
    char line[1024];
    int n = 0;
    FILE *lpipe;
    char *r;

    // find the binary
    sprintf(lsof_cmd,"/usr/sbin/lsof -F -p %d", pid);
    lpipe = popen(lsof_cmd, "r");
    do {
        r = fgets(line, 1024, lpipe);
	if (r == NULL) {
	    perror("fgets");
	    pclose(lpipe);
	    return false;
	}
	if (*r != 'n') continue;
	n += 1;
    } while (n < 2);
    pclose(lpipe);
    while (*r != '\n') r++;
    *r = 0;

    md5wrapper md5;
    // get hash of the running DF process
    string hash = md5.getHashFromFile(line+1);
    vector<memory_info *>::iterator it;

    // iterate over the list of memory locations
    for ( it=known_versions.begin() ; it < known_versions.end(); it++ )
    {
        try
        {
            if(hash == (*it)->getString("md5")) // are the md5 hashes the same?
            {
                memory_info * m = *it;
                if (memory_info::OS_MACOSX == m->getOS())
                {
                    memory_info *m2 = new memory_info(*m);
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

memory_info * NormalProcess::getDescriptor()
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
    char vmmap_cmd[256];
    char buffer[1024];
    
    sprintf(vmmap_cmd, "/usr/bin/vmmap -w -interleaved %lu", d->my_pid);
    FILE *mapFile = ::popen(vmmap_cmd, "r");
    
    while (fgets(buffer, 1024, mapFile))
    {
        if (strncmp(buffer, "__DATA", 6) == 0) {
	  if (strstr(buffer, "dwarfort.exe") == NULL) continue;
        } else if (strncmp(buffer, "MALLOC_", 7) != 0) continue;
        t_memrange temp;
        temp.name[0] = 0;
        sscanf(buffer+23, "%llx-%llx",
               &temp.start,
               &temp.end);
        temp.read = buffer[50] == 'r';
        temp.write = buffer[51] == 'w';
        temp.execute = buffer[52] == 'x';
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
    if (ptrace(PT_CONTINUE, d->my_handle, NULL, NULL) == -1)
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
    if (ptrace(PT_ATTACH , d->my_handle, NULL, NULL) == -1)
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
    d->attached = true;
    return true; // we are attached
}

bool NormalProcess::detach()
{
    if(!d->attached) return false;
    if(!d->suspended) suspend();
    int result = 0;

    // detach
    result = ptrace(PT_DETACH, d->my_handle, NULL, NULL);
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

void NormalProcess::read (const uint32_t offset_, const uint32_t size_, uint8_t *target)
{
    uint32_t indexptr = 0;
    uint32_t offset = offset_;
    uint32_t size = size_;
    while (size > 0)
    {
        #ifdef HAVE_64_BIT
            // quad!
            if(size >= 8)
            {
                readQuad(offset, *(uint64_t *) (target + indexptr));
                offset +=8;
                indexptr +=8;
                size -=8;
            }
            else
        #endif
        // default: we push 4 bytes
        if(size >= 4)
        {
            readDWord(offset, *(uint32_t *) (target + indexptr));
            offset +=4;
            indexptr +=4;
            size -=4;
        }
        // last is either three or 2 bytes
        else if(size >= 2)
        {
            readWord(offset, *(uint16_t *) (target + indexptr));
            offset +=2;
            indexptr +=2;
            size -=2;
        }
        // finishing move
        else if(size == 1)
        {
            readByte(offset, *(uint8_t *) (target + indexptr));
            return;
        }
    }
}

uint8_t NormalProcess::readByte (const uint32_t offset)
{
    return ptrace(PT_READ_D,d->my_handle, (caddr_t) offset, NULL) & 0xff;
}

void NormalProcess::readByte (const uint32_t offset, uint8_t &val )
{
    val = ptrace(PT_READ_D,d->my_handle, (caddr_t) offset, NULL) & 0xff;
}

uint16_t NormalProcess::readWord (const uint32_t offset)
{
    return ptrace(PT_READ_D,d->my_handle, (caddr_t) offset, NULL) & 0xffff;
}

void NormalProcess::readWord (const uint32_t offset, uint16_t &val)
{
    val = ptrace(PT_READ_D,d->my_handle, (caddr_t) offset, NULL) & 0xffff;
}

uint32_t NormalProcess::readDWord (const uint32_t offset)
{
    uint32_t val;
    #ifdef HAVE_64_BIT
        val = ptrace(PT_READ_D,d->my_handle, (caddr_t) offset, NULL) & 0xffffffff;
    #else
	val = ptrace(PT_READ_D,d->my_handle, (caddr_t) offset, NULL);
    #endif
    return val;
}
void NormalProcess::readDWord (const uint32_t offset, uint32_t &val)
{
    #ifdef HAVE_64_BIT
        val = ptrace(PT_READ_D,d->my_handle, (caddr_t) offset, NULL) & 0xffffffff;
    #else
	val = ptrace(PT_READ_D,d->my_handle, (caddr_t) offset, NULL);
    #endif
}

float NormalProcess::readFloat (const uint32_t offset)
{
    uint32_t v = readDWord(offset);
    return *((float *) (&v));
}

void NormalProcess::readFloat (const uint32_t offset, float &val)
{
    uint32_t v = readDWord(offset);
    val = *((float *) (&v));
}

uint64_t NormalProcess::readQuad (const uint32_t offset)
{
    uint64_t val;
    #ifdef HAVE_64_BIT
        val = ptrace(PT_READ_D,d->my_handle, (caddr_t) offset, NULL);
    #else
	val = ptrace(PT_READ_D,d->my_handle, (caddr_t) offset, NULL) |
	  ptrace(PT_READ_D,d->my_handle, (caddr_t) offset+4, NULL) << 32;
    #endif
    return val;
}
void NormalProcess::readQuad (const uint32_t offset, uint64_t &val)
{
    #ifdef HAVE_64_BIT
        val = ptrace(PT_READ_D,d->my_handle, (caddr_t) offset, NULL);
    #else
	val = ptrace(PT_READ_D,d->my_handle, (caddr_t) offset, NULL) |
	  ptrace(PT_READ_D,d->my_handle, (caddr_t) offset+4, NULL) << 32;
    #endif
}
/*
 * WRITING
 */

void NormalProcess::writeQuad (uint32_t offset, const uint64_t data)
{
    #ifdef HAVE_64_BIT
        ptrace(PT_WRITE_D,d->my_handle, (caddr_t) offset, data);
    #else
        ptrace(PT_WRITE_D,d->my_handle, (caddr_t) offset, (uint32_t) data);
        ptrace(PT_WRITE_D,d->my_handle, (caddr_t) offset+4, (uint32_t) (data >> 32));
    #endif
}

void NormalProcess::writeDWord (uint32_t offset, uint32_t data)
{
    #ifdef HAVE_64_BIT
        uint64_t orig = readQuad(offset);
        orig &= 0xFFFFFFFF00000000;
        orig |= data;
        ptrace(PT_WRITE_D,d->my_handle, (caddr_t) offset, orig);
    #else
        ptrace(PT_WRITE_D,d->my_handle, (caddr_t) offset, data);
    #endif
}

// using these is expensive.
void NormalProcess::writeWord (uint32_t offset, uint16_t data)
{
    #ifdef HAVE_64_BIT
        uint64_t orig = readQuad(offset);
        orig &= 0xFFFFFFFFFFFF0000;
        orig |= data;
        ptrace(PT_WRITE_D,d->my_handle, (caddr_t) offset, orig);
    #else
        uint32_t orig = readDWord(offset);
        orig &= 0xFFFF0000;
        orig |= data;
        ptrace(PT_WRITE_D,d->my_handle, (caddr_t) offset, orig);
    #endif
}

void NormalProcess::writeByte (uint32_t offset, uint8_t data)
{
    #ifdef HAVE_64_BIT
        uint64_t orig = readQuad(offset);
        orig &= 0xFFFFFFFFFFFFFF00;
        orig |= data;
        ptrace(PT_WRITE_D,d->my_handle, (caddr_t) offset, orig);
    #else
        uint32_t orig = readDWord(offset);
        orig &= 0xFFFFFF00;
        orig |= data;
        ptrace(PT_WRITE_D,d->my_handle, (caddr_t) offset, orig);
    #endif
}

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
