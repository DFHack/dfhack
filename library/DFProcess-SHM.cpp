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
#include "SHMProcess.h"
#include "ProcessFactory.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/DFError.h"
#include "shms.h"
#include "mod-core.h"

using namespace DFHack;

Process* DFHack::createSHMProcess(uint32_t pid,  VersionInfoFactory * factory)
{
    return new SHMProcess(pid, factory);
}

SHMProcess::SHMProcess(uint32_t PID,  VersionInfoFactory * factory)
: d(new Private(this))
{
    d->process_ID = PID;
    // attach the SHM
    if(!attach())
    {
        return;
    }
    // Test bridge version, get PID, sync Yield
    bool bridgeOK;
    if(!d->Aux_Core_Attach(bridgeOK,d->process_ID))
    {
        detach();
        throw Error::SHMAttachFailure();
    }
    else if(!bridgeOK)
    {
        detach();
        throw Error::SHMVersionMismatch();
    }

    // try to identify the DF version (md5 the binary, compare with known versions)
    d->validate(factory);
    // at this point, DF is attached and suspended, make it run
    detach();
}

SHMProcess::~SHMProcess()
{
    if(d->attached)
    {
        detach();
    }
    // destroy data model. this is assigned by processmanager
    if(d->memdescriptor)
        delete d->memdescriptor;
    delete d;
}

VersionInfo * SHMProcess::getDescriptor()
{
    return d->memdescriptor;
}

int SHMProcess::getPID()
{
    return d->process_ID;
}


bool SHMProcess::isSuspended()
{
    return d->locked;
}
bool SHMProcess::isAttached()
{
    return d->attached;
}

bool SHMProcess::isIdentified()
{
    return d->identified;
}

bool SHMProcess::suspend()
{
    if(!d->attached)
    {
        return false;
    }
    if(d->locked)
    {
        return true;
    }
    //cerr << "suspend" << endl;// FIXME: throw
    // FIXME: this should be controlled on the server side
    // FIXME: IF server got CORE_RUN in this frame, interpret CORE_SUSPEND as CORE_STEP
    // did we just resume a moment ago?
    if(D_SHMCMD == CORE_RUN)
    {
        //fprintf(stderr,"%d invokes step\n",attachmentIdx);
        // wait for the next window
        /*
        if(!d->SetAndWait(CORE_STEP))
        {
            throw Error::SHMLockingError("if(!d->SetAndWait(CORE_STEP))");
        }
        */
        D_SHMCMD = CORE_STEP;
    }
    else
    {
        //fprintf(stderr,"%d invokes suspend\n",attachmentIdx);
        // lock now
        /*
        if(!d->SetAndWait(CORE_SUSPEND))
        {
            throw Error::SHMLockingError("if(!d->SetAndWait(CORE_SUSPEND))");
        }
        */
        D_SHMCMD = CORE_SUSPEND;
    }
    //fprintf(stderr,"waiting for lock\n");
    // we wait for the server to give up our suspend lock (held by default)
    if(acquireSuspendLock())
    {
        d->locked = true;
        return true;
    }
    return false;
}

// FIXME: needs a good think-through
bool SHMProcess::asyncSuspend()
{
    if(!d->attached)
    {
        return false;
    }
    if(d->locked)
    {
        return true;
    }
    //cerr << "async suspend" << endl;// FIXME: throw
    uint32_t cmd = D_SHMCMD;
    if(cmd == CORE_SUSPENDED)
    {
        // we have to hold the lock to be really suspended
        if(acquireSuspendLock())
        {
            d->locked = true;
            return true;
        }
        return false;
    }
    else
    {
        // did we just resume a moment ago?
        if(cmd == CORE_STEP)
        {
            return false;
        }
        else if(cmd == CORE_RUN)
        {
            D_SHMCMD = CORE_STEP;
        }
        else
        {
            D_SHMCMD = CORE_SUSPEND;
        }
        return false;
    }
}

bool SHMProcess::forceresume()
{
    return resume();
}

// FIXME: wait for the server to advance a step!
bool SHMProcess::resume()
{
    if(!d->attached)
        return false;
    if(!d->locked)
        return true;
    //cerr << "resume" << endl;// FIXME: throw
    // unlock the suspend lock
    if(releaseSuspendLock())
    {
        d->locked = false;
        if(d->SetAndWait(CORE_RUN)) // we have to make sure the server responds!
        {
            return true;
        }
        throw Error::SHMLockingError("if(d->SetAndWait(CORE_RUN))");
    }
    throw Error::SHMLockingError("if(releaseSuspendLock())");
    return false;
}

// get module index by name and version. bool 0 = error
bool SHMProcess::getModuleIndex (const char * name, const uint32_t version, uint32_t & OUTPUT)
{
    if(!d->locked) throw Error::MemoryAccessDenied(0xdeadbeef);

    modulelookup * payload = D_SHMDATA(modulelookup);
    payload->version = version;

    strncpy(payload->name,name,255);
    payload->name[255] = 0;

    if(!SetAndWait(CORE_ACQUIRE_MODULE))
    {
        return false; // FIXME: throw a fatal exception instead
    }
    if(D_SHMHDR->error)
    {
        return false;
    }
    //fprintf(stderr,"%s v%d : %d\n", name, version, D_SHMHDR->value);
    OUTPUT = D_SHMHDR->value;
    return true;
}

bool SHMProcess::Private::Aux_Core_Attach(bool & versionOK, pid_t & PID)
{
    if(!locked) throw Error::MemoryAccessDenied(0xdeadbeef);

    SHMDATA(coreattach)->cl_affinity = OS_getAffinity();
    if(!SetAndWait(CORE_ATTACH)) return false;
    /*
    cerr <<"CORE_VERSION" << CORE_VERSION << endl;
    cerr <<"server CORE_VERSION" << SHMDATA(coreattach)->sv_version << endl;
    */
    versionOK =( SHMDATA(coreattach)->sv_version == CORE_VERSION );
    PID = SHMDATA(coreattach)->sv_PID;
    useYield = SHMDATA(coreattach)->sv_useYield;
    #ifdef DEBUG
        if(useYield) cerr << "Using Yield!" << endl;
    #endif
    return true;
}

void SHMProcess::read (uint32_t src_address, uint32_t size, uint8_t *target_buffer)
{
    if(!d->locked) throw Error::MemoryAccessDenied(src_address);

    // normal read under 1MB
    if(size <= SHM_BODY)
    {
        D_SHMHDR->address = src_address;
        D_SHMHDR->length = size;
        full_barrier
        d->SetAndWait(CORE_READ);
        memcpy (target_buffer, D_SHMDATA(void),size);
    }
    // a big read, we pull data over the shm in iterations
    else
    {
        // first read equals the size of the SHM window
        uint32_t to_read = SHM_BODY;
        while (size)
        {
            // read to_read bytes from src_cursor
            D_SHMHDR->address = src_address;
            D_SHMHDR->length = to_read;
            full_barrier
            d->SetAndWait(CORE_READ);
            memcpy (target_buffer, D_SHMDATA(void) ,to_read);
            // decrease size by bytes read
            size -= to_read;
            // move the cursors
            src_address += to_read;
            target_buffer += to_read;
            // check how much to write in the next iteration
            to_read = min(size, (uint32_t) SHM_BODY);
        }
    }
}

void SHMProcess::readByte (const uint32_t offset, uint8_t &val )
{
    if(!d->locked) throw Error::MemoryAccessDenied(offset);

    D_SHMHDR->address = offset;
    full_barrier
    d->SetAndWait(CORE_READ_BYTE);
    val = D_SHMHDR->value;
}

void SHMProcess::readWord (const uint32_t offset, uint16_t &val)
{
    if(!d->locked) throw Error::MemoryAccessDenied(offset);

    D_SHMHDR->address = offset;
    full_barrier
    d->SetAndWait(CORE_READ_WORD);
    val = D_SHMHDR->value;
}

void SHMProcess::readDWord (const uint32_t offset, uint32_t &val)
{
    if(!d->locked) throw Error::MemoryAccessDenied(offset);

    D_SHMHDR->address = offset;
    full_barrier
    d->SetAndWait(CORE_READ_DWORD);
    val = D_SHMHDR->value;
}

void SHMProcess::readQuad (const uint32_t offset, uint64_t &val)
{
    if(!d->locked) throw Error::MemoryAccessDenied(offset);

    D_SHMHDR->address = offset;
    full_barrier
    d->SetAndWait(CORE_READ_QUAD);
    val = D_SHMHDR->Qvalue;
}

void SHMProcess::readFloat (const uint32_t offset, float &val)
{
    if(!d->locked) throw Error::MemoryAccessDenied(offset);

    D_SHMHDR->address = offset;
    full_barrier
    d->SetAndWait(CORE_READ_DWORD);
    val = reinterpret_cast<float&> (D_SHMHDR->value);
}

/*
 * WRITING
 */

void SHMProcess::writeQuad (uint32_t offset, uint64_t data)
{
    if(!d->locked) throw Error::MemoryAccessDenied(offset);

    D_SHMHDR->address = offset;
    D_SHMHDR->Qvalue = data;
    full_barrier
    d->SetAndWait(CORE_WRITE_QUAD);
}

void SHMProcess::writeDWord (uint32_t offset, uint32_t data)
{
    if(!d->locked) throw Error::MemoryAccessDenied(offset);

    D_SHMHDR->address = offset;
    D_SHMHDR->value = data;
    full_barrier
    d->SetAndWait(CORE_WRITE_DWORD);
}

// using these is expensive.
void SHMProcess::writeWord (uint32_t offset, uint16_t data)
{
    if(!d->locked) throw Error::MemoryAccessDenied(offset);

    D_SHMHDR->address = offset;
    D_SHMHDR->value = data;
    full_barrier
    d->SetAndWait(CORE_WRITE_WORD);
}

void SHMProcess::writeByte (uint32_t offset, uint8_t data)
{
    if(!d->locked) throw Error::MemoryAccessDenied(offset);

    D_SHMHDR->address = offset;
    D_SHMHDR->value = data;
    full_barrier
    d->SetAndWait(CORE_WRITE_BYTE);
}

void SHMProcess::write (uint32_t dst_address, uint32_t size, uint8_t *source_buffer)
{
    if(!d->locked) throw Error::MemoryAccessDenied(dst_address);

    // normal write under 1MB
    if(size <= SHM_BODY)
    {
        D_SHMHDR->address = dst_address;
        D_SHMHDR->length = size;
        memcpy(D_SHMDATA(void),source_buffer, size);
        full_barrier
        d->SetAndWait(CORE_WRITE);
    }
    // a big write, we push this over the shm in iterations
    else
    {
        // first write equals the size of the SHM window
        uint32_t to_write = SHM_BODY;
        while (size)
        {
            // write to_write bytes to dst_cursor
            D_SHMHDR->address = dst_address;
            D_SHMHDR->length = to_write;
            memcpy(D_SHMDATA(void),source_buffer, to_write);
            full_barrier
            d->SetAndWait(CORE_WRITE);
            // decrease size by bytes written
            size -= to_write;
            // move the cursors
            source_buffer += to_write;
            dst_address += to_write;
            // check how much to write in the next iteration
            to_write = min(size, (uint32_t) SHM_BODY);
        }
    }
}

const std::string SHMProcess::readCString (uint32_t offset)
{
    std::string temp;
    int counter = 0;
    char r;
    while (1)
    {
        r = Process::readByte(offset+counter);
        if(!r) break;
        counter++;
        temp.append(1,r);
    }
    return temp;
}

const std::string SHMProcess::readSTLString(uint32_t offset)
{
    if(!d->locked) throw Error::MemoryAccessDenied(offset);

    D_SHMHDR->address = offset;
    full_barrier
    d->SetAndWait(CORE_READ_STL_STRING);
    return(string( D_SHMDATA(char) ));
}

size_t SHMProcess::readSTLString (uint32_t offset, char * buffer, size_t bufcapacity)
{
    if(!d->locked) throw Error::MemoryAccessDenied(offset);

    D_SHMHDR->address = offset;
    full_barrier
    d->SetAndWait(CORE_READ_STL_STRING);
    size_t length = D_SHMHDR->value;
    size_t fit = min(bufcapacity - 1, length);
    strncpy(buffer,D_SHMDATA(char),fit);
    buffer[fit] = 0;
    return fit;
}

void SHMProcess::writeSTLString(const uint32_t address, const std::string writeString)
{
    if(!d->locked) throw Error::MemoryAccessDenied(address);

    D_SHMHDR->address = address;
    strncpy(D_SHMDATA(char),writeString.c_str(),writeString.length()+1); // length + 1 for the null terminator
    full_barrier
    d->SetAndWait(CORE_WRITE_STL_STRING);
}
