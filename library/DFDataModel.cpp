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
using namespace DFHack;

DfVector DMWindows40d::readVector (uint32_t offset, uint32_t item_size)
{
    /*
        MSVC++ vector is four pointers long
        ptr allocator
        ptr start
        ptr end
        ptr alloc_end
     
        we don't care about alloc_end because we don't try to add stuff
        we also don't care about the allocator thing in front
    */
    uint32_t start = MreadDWord(offset+4);
    uint32_t end = MreadDWord(offset+8);
    uint32_t size = (end - start) /4;
    return DfVector(start,size,item_size);
};


size_t DMWindows40d::readSTLString (uint32_t offset, char * buffer, size_t bufcapacity)
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
    size_t length = MreadDWord(offset + 20);
    
    size_t capacity = MreadDWord(offset + 24);
    size_t read_real = min(length, bufcapacity-1);// keep space for null termination
    
    // read data from inside the string structure
    if(capacity < 16)
    {
        Mread(start_offset, read_real , (uint8_t *)buffer);
    }
    else // read data from what the offset + 4 dword points to
    {
        start_offset = MreadDWord(start_offset);// dereference the start offset
        Mread(start_offset, read_real, (uint8_t *)buffer);
    }
    
    buffer[read_real] = 0;
    return read_real;
};

const string DMWindows40d::readSTLString (uint32_t offset)
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
    uint32_t length = MreadDWord(offset + 20);
    uint32_t capacity = MreadDWord(offset + 24);
    char * temp = new char[capacity+1];
    
    // read data from inside the string structure
    if(capacity < 16)
    {
        Mread(start_offset, capacity, (uint8_t *)temp);
    }
    else // read data from what the offset + 4 dword points to
    {
        start_offset = MreadDWord(start_offset);// dereference the start offset
        Mread(start_offset, capacity, (uint8_t *)temp);
    }
    
    temp[length] = 0;
    string ret = temp;
    delete temp;
    return ret;
};


DfVector DMLinux40d::readVector (uint32_t offset, uint32_t item_size)
{
    /*
        GNU libstdc++ vector is three pointers long
        ptr start
        ptr end
        ptr alloc_end

        we don't care about alloc_end because we don't try to add stuff
    */
    uint32_t start = MreadDWord(offset);
    uint32_t end = MreadDWord(offset+4);
    uint32_t size = (end - start) /4;
    return DfVector(start,size,item_size);
};

struct _Rep_base
{
    uint32_t       _M_length;
    uint32_t       _M_capacity;
    uint32_t        _M_refcount;
};

size_t DMLinux40d::readSTLString (uint32_t offset, char * buffer, size_t bufcapacity)
{
    _Rep_base header;
    offset = MreadDWord(offset);
    Mread(offset - sizeof(_Rep_base),sizeof(_Rep_base),(uint8_t *)&header);
    size_t read_real = min((size_t)header._M_length, bufcapacity-1);// keep space for null termination
    Mread(offset,read_real,(uint8_t * )buffer);
    buffer[read_real] = 0;
    return read_real;
};

const string DMLinux40d::readSTLString (uint32_t offset)
{
    _Rep_base header;
    
    offset = MreadDWord(offset);
    Mread(offset - sizeof(_Rep_base),sizeof(_Rep_base),(uint8_t *)&header);
    
    // FIXME: use char* everywhere, avoid string
    char * temp = new char[header._M_length+1];
    Mread(offset,header._M_length+1,(uint8_t * )temp);
    string ret(temp);
    delete temp;
    return ret;
};
