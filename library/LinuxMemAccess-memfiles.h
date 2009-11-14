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

/**
 * DO NOT USE THIS FILE DIRECTLY! USE MemAccess.h INSTEAD!
 */
#include "integers.h"
#include <iostream>
#include <string>
#include <errno.h>
using namespace std;

// danger: uses recursion!
inline
void Mread (const uint32_t &offset, const uint32_t &size, uint8_t *target)
{
    if(size == 0) return;
    
    int result;
    result = pread(DFHack::g_ProcessMemFile, target,size,offset);
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
            Mread(offset + result, size - result, target + result);
        }
    }
}

inline
uint8_t MreadByte (const uint32_t &offset)
{
    uint8_t val;
    Mread(offset, 1, &val);
    return val;
}

inline
void MreadByte (const uint32_t &offset, uint8_t &val )
{
    Mread(offset, 1, &val);
}

inline
uint16_t MreadWord (const uint32_t &offset)
{
    uint16_t val;
    Mread(offset, 2, (uint8_t *) &val);
    return val;
}

inline
void MreadWord (const uint32_t &offset, uint16_t &val)
{
    Mread(offset, 2, (uint8_t *) &val);
}

inline
uint32_t MreadDWord (const uint32_t &offset)
{
    uint32_t val;
    Mread(offset, 4, (uint8_t *) &val);
    return val;
}
inline
void MreadDWord (const uint32_t &offset, uint32_t &val)
{
    Mread(offset, 4, (uint8_t *) &val);
}

/*
 * WRITING
 */

inline
void MwriteDWord (uint32_t offset, uint32_t data)
{
    ptrace(PTRACE_POKEDATA,DFHack::g_ProcessHandle, offset, data);
}

// using these is expensive.
inline
void MwriteWord (uint32_t offset, uint16_t data)
{
    uint32_t orig = MreadDWord(offset);
    orig &= 0xFFFF0000;
    orig |= data;
    /*
    orig |= 0x0000FFFF;
    orig &= data;
    */
    ptrace(PTRACE_POKEDATA,DFHack::g_ProcessHandle, offset, orig);
}

inline
void MwriteByte (uint32_t offset, uint8_t data)
{
    uint32_t orig = MreadDWord(offset);
    orig &= 0xFFFFFF00;
    orig |= data;
    /*
    orig |= 0x000000FF;
    orig &= data;
    */
    ptrace(PTRACE_POKEDATA,DFHack::g_ProcessHandle, offset, orig);
}

// blah. I hate the kernel devs for crippling /proc/PID/mem. THIS IS RIDICULOUS
inline
bool Mwrite (uint32_t offset, uint32_t size, uint8_t *source)
{
    uint32_t indexptr = 0;
    while (size > 0)
    {
        // default: we push 4 bytes
        if(size >= 4)
        {
            MwriteDWord(offset, *(uint32_t *) (source + indexptr));
            offset +=4;
            indexptr +=4;
            size -=4;
        }
        // last is either three or 2 bytes
        else if(size >= 2)
        {
            MwriteWord(offset, *(uint16_t *) (source + indexptr));
            offset +=2;
            indexptr +=2;
            size -=2;
        }
        // finishing move
        else if(size == 1)
        {
            MwriteByte(offset, *(uint8_t *) (source + indexptr));
            return true;
        }
    }
}

inline
const std::string MreadCString (uint32_t offset)
{
    std::string temp;
    char temp_c[256];
    int counter = 0;
    char r;
    do
    {
        r = MreadByte(offset+counter);
        temp_c[counter] = r;
        counter++;
    } while (r && counter < 255);
    temp_c[counter] = 0;
    temp = temp_c;
    return temp;
}
