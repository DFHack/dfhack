/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

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

#pragma once

#include <iostream>
#include <iomanip>
#include <climits>
#include <stdint.h>
#include <vector>
#include <sstream>
#include <ctime>
#include <cstdio>

using namespace std;
/*
#include <dfhack/Process.h>
#include <dfhack/VersionInfo.h>
#include <dfhack/Vector.h>

void DumpObjStr0Vector (const char * name, DFHack::Process *p, uint32_t addr)
{
    cout << "----==== " << name << " ====----" << endl;
    DFHack::DfVector <uint32_t> vect(p,addr);
    for(uint32_t i = 0; i < vect.size();i++)
    {
        uint32_t addr = vect[i];
        cout << p->readSTLString(addr) << endl;
    }
    cout << endl;
}
void DumpObjVtables (const char * name, DFHack::Process *p, uint32_t addr)
{
    cout << "----==== " << name << " ====----" << endl;
    DFHack::DfVector <uint32_t> vect(p,addr);
    for(uint32_t i = 0; i < vect.size();i++)
    {
        uint32_t addr = vect[i];
        uint32_t vptr = p->readDWord(addr);
        cout << p->readClassName(vptr) << endl;
    }
    cout << endl;
}
void DumpDWordVector (const char * name, DFHack::Process *p, uint32_t addr)
{
    cout << "----==== " << name << " ====----" << endl;
    DFHack::DfVector <uint32_t> vect(p,addr);
    for(uint32_t i = 0; i < vect.size();i++)
    {
        uint32_t number = vect[i];
        cout << number << endl;
    }
    cout << endl;
}
*/
/*
address = absolute address of dump start
length = length in lines. 1 line = 16 bytes
*/
/*
void hexdump (DFHack::Context *DF, uint32_t address, uint32_t length)
{
    char *buf = new char[length];
    DF->ReadRaw(address, length, (uint8_t *) buf);
    uint32_t i = 0;
    while (i < length)
    {
        // leading offset
        if(i%16 == 0)
            cout << "0x" << hex << setw(8) << address + i << "| ";
        // bytes
        for(int k = 0; k < 4; k++)
        {
            cout << hex << setw(2) << int(static_cast<unsigned char>(buf[i])) << " ";
            i++;
            if(i == length) break;
        }
        if(i%16 == 0 || i>= length)
        {
            cout << endl;
        }
        else if(i%4 == 0)
        {
            cout << " ";
        }
    }
    delete buf;
}

void interleave_hex (DFHack::Context* DF, vector < uint32_t > & addresses, uint32_t length)
{
    vector <char * > bufs;

    for(uint32_t counter = 0; counter < addresses.size(); counter ++)
    {
        char * buf = new char[length * 16];
        DF->ReadRaw(addresses[counter], length * 16, (uint8_t *) buf);
        bufs.push_back(buf);
    }
    cout << setfill('0');

    // output a header
    cout << "line offset ";
    for (uint32_t obj = 0; obj < addresses.size(); obj++)
    {
        cout << "0x" << hex << setw(9) << addresses[obj] << "  ";
    }
    cout << endl;

    for(uint32_t offs = 0 ; offs < length * 16; offs += 4)
    {
        if((!(offs % 16)) && offs != 0)
        {
            cout << endl;
        }
        cout << setfill(' ');
        cout << dec << setw(4) << offs/4 << " ";
        cout << setfill('0');
        cout << "0x" << hex << setw(4) << offs << " ";
        for (uint32_t object = 0; object < bufs.size(); object++)
        {
            // bytes
            for(int k = 0; k < 4; k++)
            {
                uint8_t data = bufs[object][offs + k];
                cout << hex << setw(2) << int(static_cast<unsigned char>(data)) << " ";
            }
            cout << " ";
        }
        cout << endl;
    }
    for(uint32_t counter = 0; counter < addresses.size(); counter ++)
    {
        delete bufs[counter];
    }
}
*/
template <typename T>
void print_bits ( T val, DFHack::Console& out )
{
    stringstream strs;
    T n_bits = sizeof ( val ) * CHAR_BIT;
    int cnt;
    for ( unsigned i = 0; i < n_bits; ++i )
    {
        cnt = i/10;
        strs << cnt << " ";
    }
    strs << endl;
    for ( unsigned i = 0; i < n_bits; ++i )
    {
        cnt = i%10;
        strs << cnt << " ";
    }
    strs << endl;
    for ( unsigned i = 0; i < n_bits; ++i )
    {
        strs << "--";
    }
    strs << endl;
    for ( unsigned i = 0; i < n_bits; ++i )
    {
        strs<< !!( val & 1 ) << " ";
        val >>= 1;
    }
    strs << endl;
    out.print(strs.str().c_str());
}
/*
// this is probably completely bogus
std::string PrintSplatterType (int16_t mat1, int32_t mat2, vector<DFHack::t_matgloss> &creature_types)
{
    std::string ret;
    switch (mat1)
    {
        case 0:
            return "Rock";
        case 1:
            return "Amber";
        case 2:
            return "Coral";
        case 3:
            return "Green Glass";
        case 4:
            return "Clear Glass";
        case 5:
            return "Crystal Glass";
        case 6:
            return "Water";
        case 7:
            return "Coal";
        case 8:
            return "Potash";
        case 9:
            return "Ash";
        case 10:
            return "Pearlash";
        case 11:
            return "Lye";
        case 12:
            return "Mud";
        case 13:
            return "Vomit";
        case 14:
            return "Salt";
        case 15:
            return "Filth";
        case 16:
            return "Frozen? Filth";
        case 18:
            return "Grime";
        case 0xF2:
            return  "Very Specific Blood (references a named creature)";
        case 0x2A:
        case 0x2B:
            if(mat2 != -1)
            {
                ret += creature_types[mat2].id;
                ret += " ";
            }
            ret += "Blood";
            return ret;
        default:
            return "Unknown";
    }
}
*/


