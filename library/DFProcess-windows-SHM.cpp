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
#include "../shmserver/shms.h"
using namespace DFHack;

// a full memory barrier! better be safe than sorry.
class SHMProcess::Private
{
    public:
    Private()
    {
        my_datamodel = NULL;
        my_descriptor = NULL;
        my_pid = 0;
        my_shm = 0;
        my_window = NULL;
        attached = false;
        suspended = false;
        identified = false;
        DFSVMutex = 0;
        DFCLMutex = 0;
    };
    ~Private(){};
    DataModel* my_datamodel;
    memory_info * my_descriptor;
    DFWindow * my_window;
    uint32_t my_pid;
    char *my_shm;
    HANDLE DFSVMutex;
    HANDLE DFCLMutex;
    
    bool attached;
    bool suspended;
    bool identified;
    
    bool waitWhile (DF_PINGPONG state);
    bool isValidSV();
    bool DF_TestBridgeVersion(bool & ret);
    bool DF_GetPID(uint32_t & ret);
};

// is the other side still there?
bool SHMProcess::Private::isValidSV()
{
    // try if CL mutex is free
    uint32_t result = WaitForSingleObject(DFSVMutex,0);
    
    switch (result)
    {
        case WAIT_ABANDONED:
        case WAIT_OBJECT_0:
        {
            ReleaseMutex(DFSVMutex);
            return false;
        }
        case WAIT_TIMEOUT:
        {
            // mutex is held by DF
            return true;
        }   
        default:
        case WAIT_FAILED:
        {
            // TODO: now how do I respond to this?
            return false;
        }
    }
}

bool SHMProcess::Private::waitWhile (DF_PINGPONG state)
{
    uint32_t cnt = 0;
    SCHED_YIELD // yield the CPU, valid only on single-core CPUs
    while (((shm_cmd *)my_shm)->pingpong == state)
    {
        if(cnt == 10000)
        {
            if(!isValidSV())// DF not there anymore?
            {
                full_barrier
                ((shm_cmd *)my_shm)->pingpong = DFPP_RUNNING;
                attached = suspended = false;
                ReleaseMutex(DFCLMutex);
                return false;
            }
            else
            {
                cnt = 0;
            }
        }
        cnt++;
    }
    if(((shm_cmd *)my_shm)->pingpong == DFPP_SV_ERROR)
    {
        ((shm_cmd *)my_shm)->pingpong = DFPP_RUNNING;
        attached = suspended = false;
        cerr << "shm server error!" << endl;
        assert (false);
        return false;
    }
    return true;
}

bool SHMProcess::Private::DF_TestBridgeVersion(bool & ret)
{
    ((shm_cmd *)my_shm)->pingpong = DFPP_VERSION;
    full_barrier
    if(!waitWhile(DFPP_VERSION))
        return false;
    full_barrier
    ((shm_cmd *)my_shm)->pingpong = DFPP_SUSPENDED;
    ret =( ((shm_retval *)my_shm)->value == PINGPONG_VERSION );
    return true;
}

bool SHMProcess::Private::DF_GetPID(uint32_t & ret)
{
    ((shm_cmd *)my_shm)->pingpong = DFPP_PID;
    full_barrier
    if(!waitWhile(DFPP_PID))
        return false;
    full_barrier
    ((shm_cmd *)my_shm)->pingpong = DFPP_SUSPENDED;
    ret = ((shm_retval *)my_shm)->value;
    return true;
}

SHMProcess::SHMProcess(vector <memory_info> & known_versions)
: d(new Private())
{
    char exe_link_name [256];
    char target_name[1024];
    int target_result;
    do
    {
        // get server and client mutex
        d->DFSVMutex = OpenMutex(SYNCHRONIZE,false, "DFSVMutex");
        if(d->DFSVMutex == 0)
        {
            break;
        }
        d->DFCLMutex = OpenMutex(SYNCHRONIZE,false, "DFCLMutex");
        if(d->DFCLMutex == 0)
        {
            break;
        }
        if(!attach())
        {
            break;
        }
        
        // All seems to be OK so far. Attached and connected to something that looks like DF
        
        // Test bridge version, will also detect when we connect to something that doesn't respond
        bool bridgeOK;
        if(!d->DF_TestBridgeVersion(bridgeOK))
        {
            fprintf(stderr,"DF terminated during reading\n");
            UnmapViewOfFile(d->my_shm);
            ReleaseMutex(d->DFCLMutex);
            CloseHandle(d->DFSVMutex);
            d->DFSVMutex = 0;
            CloseHandle(d->DFCLMutex);
            d->DFCLMutex = 0;
            break;
        }
        if(!bridgeOK)
        {
            fprintf(stderr,"SHM bridge version mismatch\n");
            ((shm_cmd *)d->my_shm)->pingpong = DFPP_RUNNING;
            UnmapViewOfFile(d->my_shm);
            ReleaseMutex(d->DFCLMutex);
            CloseHandle(d->DFSVMutex);
            d->DFSVMutex = 0;
            CloseHandle(d->DFCLMutex);
            d->DFCLMutex = 0;
            break;
        }
        /*
         * get the PID from DF
         */
        if(d->DF_GetPID(d->my_pid))
        {
            // try to identify the DF version
            do // glorified goto
            {  
                IMAGE_NT_HEADERS32 pe_header;
                IMAGE_SECTION_HEADER sections[16];
                HMODULE hmod = NULL;
                DWORD junk;
                HANDLE hProcess;
                bool found = false;
                d->identified = false;
                // open process, we only need the process open 
                hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, d->my_pid );
                if (NULL == hProcess)
                    break;
                
                // try getting the first module of the process
                if(EnumProcessModules(hProcess, &hmod, 1 * sizeof(HMODULE), &junk) == 0)
                {
                    CloseHandle(hProcess);
                    cout << "EnumProcessModules fail'd" << endl;
                    break;
                }
                // got base ;)
                uint32_t base = (uint32_t)hmod;
                
                // read from this process
                uint32_t pe_offset = readDWord(base+0x3C);
                read(base + pe_offset                   , sizeof(pe_header), (uint8_t *)&pe_header);
                read(base + pe_offset+ sizeof(pe_header), sizeof(sections) , (uint8_t *)&sections );
                
                // iterate over the list of memory locations
                vector<memory_info>::iterator it;
                for ( it=known_versions.begin() ; it < known_versions.end(); it++ )
                {
                    uint32_t pe_timestamp = (*it).getHexValue("pe_timestamp");
                    if (pe_timestamp == pe_header.FileHeader.TimeDateStamp)
                    {
                        memory_info *m = new memory_info(*it);
                        m->RebaseAll(base);
                        d->my_datamodel = new DMWindows40d();
                        d->my_descriptor = m;
                        d->identified = true;
                        cerr << "identified " << m->getVersion() << endl;
                        break;
                    }
                }
                CloseHandle(hProcess);
            } while (0); // glorified goto end
            
            if(d->identified)
            {
                d->my_window = new DFWindow(this);
            }
            else
            {
                ((shm_cmd *)d->my_shm)->pingpong = DFPP_RUNNING;
                UnmapViewOfFile(d->my_shm);
                ReleaseMutex(d->DFCLMutex);
                CloseHandle(d->DFSVMutex);
                d->DFSVMutex = 0;
                CloseHandle(d->DFCLMutex);
                d->DFCLMutex = 0;
                break;
            }
        }
    } while (0);
    full_barrier
    // at this point, DF is attached and suspended, make it run
    detach();
}

bool SHMProcess::isSuspended()
{
    return d->suspended;
}
bool SHMProcess::isAttached()
{
    return d->attached;
}

bool SHMProcess::isIdentified()
{
    return d->identified;
}

SHMProcess::~SHMProcess()
{
    if(d->attached)
    {
        detach();
    }
    // destroy data model. this is assigned by processmanager
    if(d->my_datamodel)
    {
        delete d->my_datamodel;
    }
    if(d->my_descriptor)
    {
        delete d->my_descriptor;
    }
    if(d->my_window)
    {
        delete d->my_window;
    }
    // release mutex handles we have
    if(d->DFCLMutex)
    {
        CloseHandle(d->DFCLMutex);
    }
    if(d->DFSVMutex)
    {
        CloseHandle(d->DFSVMutex);
    }
    delete d;
}


DataModel *SHMProcess::getDataModel()
{
    return d->my_datamodel;
}

memory_info * SHMProcess::getDescriptor()
{
    return d->my_descriptor;
}

DFWindow * SHMProcess::getWindow()
{
    return d->my_window;
}

int SHMProcess::getPID()
{
    return d->my_pid;
}

//FIXME: implement
bool SHMProcess::getThreadIDs(vector<uint32_t> & threads )
{
    return false;
}

//FIXME: cross-reference with ELF segment entries?
void SHMProcess::getMemRanges( vector<t_memrange> & ranges )
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

bool SHMProcess::suspend()
{
    if(!d->attached)
    {
        cerr << "couldn't suspend, not attached" << endl;
        return false;
    }
    if(d->suspended)
    {
        cerr << "couldn't suspend, already suspended" << endl;
        return true;
    }
    ((shm_cmd *)d->my_shm)->pingpong = DFPP_SUSPEND;
    if(!d->waitWhile(DFPP_SUSPEND))
    {
        cerr << "couldn't suspend, DF not responding to commands" << endl;
        return false;
    }
    d->suspended = true;
    return true;
}

bool SHMProcess::asyncSuspend()
{
    if(!d->attached)
    {
        return false;
    }
    if(d->suspended)
    {
        return true;
    }
    if(((shm_cmd *)d->my_shm)->pingpong == DFPP_SUSPENDED)
    {
        d->suspended = true;
        return true;
    }
    else
    {
        ((shm_cmd *)d->my_shm)->pingpong = DFPP_SUSPEND;
        return false;
    }
}

bool SHMProcess::forceresume()
{
    return resume();
}

bool SHMProcess::resume()
{
    if(!d->attached)
    {
        cerr << "couldn't resume because of no attachment" << endl;
        return false;
    }
    if(!d->suspended)
    {
        cerr << "couldn't resume because of not being suspended" << endl;
        return true;
    }
    ((shm_cmd *)d->my_shm)->pingpong = DFPP_RUNNING;
    d->suspended = false;
    return true;
}


bool SHMProcess::attach()
{
    int status;
    if(g_pProcess != 0)
    {
        cerr << "there's already a different process attached" << endl;
        return false;
    }
    if(d->attached)
    {
        cerr << "already attached" << endl;
        return false;
    }
    // check if DF is there
    if(!d->isValidSV())
    {
        return false; // NOT
    }
    // try locking client mutex
    uint32_t result = WaitForSingleObject(d->DFCLMutex,0);
    if( result != WAIT_OBJECT_0 && result != WAIT_ABANDONED)
    {
        return false; // we couldn't lock it
    }

    // now try getting and attaching the shared memory
    HANDLE shmHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS,false,"DFShm");
    if(!shmHandle)
    {
        ReleaseMutex(d->DFCLMutex);
        return false; // we couldn't lock it
    }

    // attempt to attach the opened mapping
    d->my_shm = (char *) MapViewOfFile(shmHandle,FILE_MAP_ALL_ACCESS, 0,0, SHM_SIZE);
    if(!d->my_shm)
    {
        CloseHandle(shmHandle);
        ReleaseMutex(d->DFCLMutex);
        return false; // we couldn't attach the mapping
    }
    // we close the handle right here so we don't have to keep track of it
    CloseHandle(shmHandle);
    d->attached = true;
    suspend();
    g_pProcess = this;
    return true;
}

bool SHMProcess::detach()
{
    if(!d->attached)
    {
        return false;
    }
    if(d->suspended)
    {
        resume();
    }
    // detach segment
    UnmapViewOfFile(d->my_shm);
    // release it for some other client
    ReleaseMutex(d->DFCLMutex); // we keep the mutex handles
    d->attached = false;
    d->suspended = false;
    g_pProcess = 0;
    return true;
}

void SHMProcess::read (uint32_t src_address, uint32_t size, uint8_t *target_buffer)
{
    // normal read under 1MB
    if(size <= SHM_BODY)
    {
        ((shm_read *)d->my_shm)->address = src_address;
        ((shm_read *)d->my_shm)->length = size;
        full_barrier
        ((shm_read *)d->my_shm)->pingpong = DFPP_READ;
        d->waitWhile(DFPP_READ);
        memcpy (target_buffer, d->my_shm + SHM_HEADER,size);
    }
    // a big read, we pull data over the shm in iterations
    else
    {
        // first read equals the size of the SHM window
        uint32_t to_read = SHM_BODY;
        while (size)
        {
            // read to_read bytes from src_cursor
            ((shm_read *)d->my_shm)->address = src_address;
            ((shm_read *)d->my_shm)->length = to_read;
            full_barrier
            ((shm_read *)d->my_shm)->pingpong = DFPP_READ;
            d->waitWhile(DFPP_READ);
            memcpy (target_buffer, d->my_shm + SHM_HEADER,size);
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

uint8_t SHMProcess::readByte (const uint32_t offset)
{
    ((shm_read_small *)d->my_shm)->address = offset;
    full_barrier
    ((shm_read_small *)d->my_shm)->pingpong = DFPP_READ_BYTE;
    d->waitWhile(DFPP_READ_BYTE);
    return ((shm_retval *)d->my_shm)->value;
}

void SHMProcess::readByte (const uint32_t offset, uint8_t &val )
{
    ((shm_read_small *)d->my_shm)->address = offset;
    full_barrier
    ((shm_read_small *)d->my_shm)->pingpong = DFPP_READ_BYTE;
    d->waitWhile(DFPP_READ_BYTE);
    val = ((shm_retval *)d->my_shm)->value;
}

uint16_t SHMProcess::readWord (const uint32_t offset)
{
    ((shm_read_small *)d->my_shm)->address = offset;
    full_barrier
    ((shm_read_small *)d->my_shm)->pingpong = DFPP_READ_WORD;
    d->waitWhile(DFPP_READ_WORD);
    return ((shm_retval *)d->my_shm)->value;
}

void SHMProcess::readWord (const uint32_t offset, uint16_t &val)
{
    ((shm_read_small *)d->my_shm)->address = offset;
    full_barrier
    ((shm_read_small *)d->my_shm)->pingpong = DFPP_READ_WORD;
    d->waitWhile(DFPP_READ_WORD);
    val = ((shm_retval *)d->my_shm)->value;
}

uint32_t SHMProcess::readDWord (const uint32_t offset)
{
    ((shm_read_small *)d->my_shm)->address = offset;
    full_barrier
    ((shm_read_small *)d->my_shm)->pingpong = DFPP_READ_DWORD;
    d->waitWhile(DFPP_READ_DWORD);
    return ((shm_retval *)d->my_shm)->value;
}
void SHMProcess::readDWord (const uint32_t offset, uint32_t &val)
{
    ((shm_read_small *)d->my_shm)->address = offset;
    full_barrier
    ((shm_read_small *)d->my_shm)->pingpong = DFPP_READ_DWORD;
    d->waitWhile(DFPP_READ_DWORD);
    val = ((shm_retval *)d->my_shm)->value;
}

/*
 * WRITING
 */

void SHMProcess::writeDWord (uint32_t offset, uint32_t data)
{
    ((shm_write_small *)d->my_shm)->address = offset;
    ((shm_write_small *)d->my_shm)->value = data;
    full_barrier
    ((shm_write_small *)d->my_shm)->pingpong = DFPP_WRITE_DWORD;
    d->waitWhile(DFPP_WRITE_DWORD);
}

// using these is expensive.
void SHMProcess::writeWord (uint32_t offset, uint16_t data)
{
    ((shm_write_small *)d->my_shm)->address = offset;
    ((shm_write_small *)d->my_shm)->value = data;
    full_barrier
    ((shm_write_small *)d->my_shm)->pingpong = DFPP_WRITE_WORD;
    d->waitWhile(DFPP_WRITE_WORD);
}

void SHMProcess::writeByte (uint32_t offset, uint8_t data)
{
    ((shm_write_small *)d->my_shm)->address = offset;
    ((shm_write_small *)d->my_shm)->value = data;
    full_barrier
    ((shm_write_small *)d->my_shm)->pingpong = DFPP_WRITE_BYTE;
    d->waitWhile(DFPP_WRITE_BYTE);
}

void SHMProcess::write (uint32_t dst_address, uint32_t size, uint8_t *source_buffer)
{
    // normal write under 1MB
    if(size <= SHM_BODY)
    {
        ((shm_write *)d->my_shm)->address = dst_address;
        ((shm_write *)d->my_shm)->length = size;
        memcpy(d->my_shm+SHM_HEADER,source_buffer, size);
        full_barrier
        ((shm_write *)d->my_shm)->pingpong = DFPP_WRITE;
        d->waitWhile(DFPP_WRITE);
    }
    // a big write, we push this over the shm in iterations
    else
    {
        // first write equals the size of the SHM window
        uint32_t to_write = SHM_BODY;
        while (size)
        {
            // write to_write bytes to dst_cursor
            ((shm_write *)d->my_shm)->address = dst_address;
            ((shm_write *)d->my_shm)->length = to_write;
            memcpy(d->my_shm+SHM_HEADER,source_buffer, to_write);
            full_barrier
            ((shm_write *)d->my_shm)->pingpong = DFPP_WRITE;
            d->waitWhile(DFPP_WRITE);
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

// FIXME: butt-fugly
const std::string SHMProcess::readCString (uint32_t offset)
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

