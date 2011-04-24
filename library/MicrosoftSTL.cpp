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

#include <string>
#include <vector>
#include <map>
using namespace std;

#include "MicrosoftSTL.h"
#include "dfhack/DFProcess.h"
#include "dfhack/VersionInfo.h"

using namespace DFHack;

void MicrosoftSTL::init(Process* self)
{
    p = self;
    OffsetGroup * strGrp = p->getDescriptor()->getGroup("string")->getGroup("MSVC");
    STLSTR_buf_off = strGrp->getOffset("buffer");
    STLSTR_size_off = strGrp->getOffset("size");
    STLSTR_cap_off = strGrp->getOffset("capacity");
}

size_t MicrosoftSTL::readSTLString (uint32_t offset, char * buffer, size_t bufcapacity)
{
    uint32_t start_offset = offset + STLSTR_buf_off;
    size_t length = p->readDWord(offset + STLSTR_size_off);
    size_t capacity = p->readDWord(offset + STLSTR_cap_off);

    size_t read_real = min(length, bufcapacity-1);// keep space for null termination

    // read data from inside the string structure
    if(capacity < 16)
    {
        p->read(start_offset, read_real , (uint8_t *)buffer);
    }
    else // read data from what the offset + 4 dword points to
    {
        start_offset = p->readDWord(start_offset);// dereference the start offset
        p->read(start_offset, read_real, (uint8_t *)buffer);
    }

    buffer[read_real] = 0;
    return read_real;
}

const string MicrosoftSTL::readSTLString (uint32_t offset)
{
    uint32_t start_offset = offset + STLSTR_buf_off;
    size_t length = p->readDWord(offset + STLSTR_size_off);
    size_t capacity = p->readDWord(offset + STLSTR_cap_off);

    char * temp = new char[capacity+1];

    // read data from inside the string structure
    if(capacity < 16)
    {
        p->read(start_offset, capacity, (uint8_t *)temp);
    }
    else // read data from what the offset + 4 dword points to
    {
        start_offset = p->readDWord(start_offset);// dereference the start offset
        p->read(start_offset, capacity, (uint8_t *)temp);
    }

    temp[length] = 0;
    string ret = temp;
    delete temp;
    return ret;
}

string MicrosoftSTL::readClassName (uint32_t vptr)
{
    int rtti = p->readDWord(vptr - 0x4);
    int typeinfo = p->readDWord(rtti + 0xC);
    string raw = p->readCString(typeinfo + 0xC); // skips the .?AV
    raw.resize(raw.length() - 2);// trim @@ from end
    return raw;
}

// FIXME: really, fix this.
size_t MicrosoftSTL::writeSTLString(const uint32_t address, const std::string writeString)
{
    return 0;
}