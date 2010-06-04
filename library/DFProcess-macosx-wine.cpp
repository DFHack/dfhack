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
#include <sys/ptrace.h>
#include <stdio.h>
#include <sys/sysctl.h>
using namespace DFHack;

class WineProcess::Private
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
    memory_info * my_descriptor;
    Process * self;
    pid_t my_handle;
    uint32_t my_pid;
    bool attached;
    bool suspended;
    bool identified;
    bool validate(uint32_t pid, vector <memory_info *> & known_versions);
};

WineProcess::WineProcess(uint32_t pid, vector <memory_info *> & known_versions)
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
        
    // FIXME: this fails when the wine process isn't started from the 'current working directory'. strip path data from cmdline
    // is this windows version of Df running in wine?
    if(strstr(kproc->kp_proc.p_comm, "wine")!= NULL)
    {
        char ps_cmd[256], buf[1024];
	sprintf(ps_cmd, "/bin/ps -ww %d", pid);
	FILE *f = popen(ps_cmd, "r");
	fgets(buf, 1024, f);
	fgets(buf, 1024, f);
	pclose(f);

        if (strstr(buf, "dwarfort.exe") != NULL ||
	    strstr(buf, "dwarfort_w.exe") != NULL ||
	    strstr(buf, "Dwarf Fortress.exe") != NULL)
	{
            // create wine process, add it to the vector
            d->identified = d->validate(pid,known_versions);
        }
    }
    free(kproc);
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

bool WineProcess::Private::validate(uint32_t pid, std::vector< memory_info* >& known_versions)
{
    char lsof_cmd [256];
    char line[1024];
    int target_result;
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
    delete d;
}

memory_info * WineProcess::getDescriptor()
{
    return d->my_descriptor;
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
    char vmmap_cmd[256];
    char buffer[1024];
    
    sprintf(vmmap_cmd, "/usr/bin/vmmap -w -interleaved %lu", d->my_pid);
    FILE *mapFile = ::popen(vmmap_cmd, "r");
    
    while (fgets(buffer, 1024, mapFile))
    {
        if (strncmp(buffer, "WINE_DOS", 6) == 0) {
	  if (strstr(buffer, "wine") == NULL) continue;
        } else /* if (strncmp(buffer, "MALLOC_", 7) != 0) */ continue;
        t_memrange temp;
        temp.name[0] = 0;
        sscanf(buffer, "%llx-%llx",
               &temp.start,
               &temp.end);
        temp.read = buffer[50] == 'r';
        temp.write = buffer[51] == 'w';
        temp.execute = buffer[52] == 'x';
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
    if (ptrace(PT_CONTINUE, d->my_handle, NULL, NULL) == -1)
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

bool WineProcess::detach()
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


// danger: uses recursion!
void WineProcess::read (const uint32_t offset_, const uint32_t size_, uint8_t *target)
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

uint8_t WineProcess::readByte (const uint32_t offset)
{
    return ptrace(PT_READ_D,d->my_handle, (caddr_t) offset, NULL) & 0xff;
}

void WineProcess::readByte (const uint32_t offset, uint8_t &val )
{
    val = ptrace(PT_READ_D,d->my_handle, (caddr_t) offset, NULL) & 0xff;
}

uint16_t WineProcess::readWord (const uint32_t offset)
{
    return ptrace(PT_READ_D,d->my_handle, (caddr_t) offset, NULL) & 0xffff;
}

void WineProcess::readWord (const uint32_t offset, uint16_t &val)
{
    val = ptrace(PT_READ_D,d->my_handle, (caddr_t) offset, NULL) & 0xffff;
}

uint32_t WineProcess::readDWord (const uint32_t offset)
{
    uint32_t val;
    #ifdef HAVE_64_BIT
        val = ptrace(PT_READ_D,d->my_handle, (caddr_t) offset, NULL) & 0xffffffff;
    #else
	val = ptrace(PT_READ_D,d->my_handle, (caddr_t) offset, NULL);
    #endif
    return val;
}
void WineProcess::readDWord (const uint32_t offset, uint32_t &val)
{
    #ifdef HAVE_64_BIT
        val = ptrace(PT_READ_D,d->my_handle, (caddr_t) offset, NULL) & 0xffffffff;
    #else
	val = ptrace(PT_READ_D,d->my_handle, (caddr_t) offset, NULL);
    #endif
}

float WineProcess::readFloat (const uint32_t offset)
{
    uint32_t v = readDWord(offset);
    return *((float *) (&v));
}

void WineProcess::readFloat (const uint32_t offset, float &val)
{
    uint32_t v = readDWord(offset);
    val = *((float *) (&v));
}

uint64_t WineProcess::readQuad (const uint32_t offset)
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
void WineProcess::readQuad (const uint32_t offset, uint64_t &val)
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

void WineProcess::writeQuad (uint32_t offset, const uint64_t data)
{
    #ifdef HAVE_64_BIT
        ptrace(PT_WRITE_D,d->my_handle, (caddr_t) offset, data);
    #else
        ptrace(PT_WRITE_D,d->my_handle, (caddr_t) offset, (uint32_t) data);
        ptrace(PT_WRITE_D,d->my_handle, (caddr_t) offset+4, (uint32_t) (data >> 32));
    #endif
}

void WineProcess::writeDWord (uint32_t offset, uint32_t data)
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
void WineProcess::writeWord (uint32_t offset, uint16_t data)
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

void WineProcess::writeByte (uint32_t offset, uint8_t data)
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

void WineProcess::write (uint32_t offset, uint32_t size, uint8_t *source)
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
