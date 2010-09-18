#ifndef DF_MISCUTILS
#define DF_MISCUTILS
#include <iostream>
#include <iomanip>
#include <climits>
#include <dfhack/DFIntegers.h>
#include <vector>
#include <sstream>
#include <ctime>
#include <cstdio>

using namespace std;

#include <dfhack/DFProcess.h>
#include <dfhack/VersionInfo.h>
#include <dfhack/DFVector.h>

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

/*
address = absolute address of dump start
length = length in lines. 1 line = 16 bytes
*/
void hexdump (DFHack::Context *DF, uint32_t address, uint32_t length)
{
    char *buf = new char[length * 16];
    DF->ReadRaw(address, length * 16, (uint8_t *) buf);
    for (uint32_t i = 0; i < length; i++)
    {
        // leading offset
        cout << "0x" << hex << setw(8) << address + i*16 << "| ";
        // groups
        for(int j = 0; j < 4; j++)
        {
            // bytes
            for(int k = 0; k < 4; k++)
            {
                int idx = i * 16 + j * 4 + k;
                cout << hex << setw(2) << int(static_cast<unsigned char>(buf[idx])) << " ";
            }
            cout << " ";
        }
        cout << endl;
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

template <typename T>
void print_bits ( T val, std::ostream& out )
{
    T n_bits = sizeof ( val ) * CHAR_BIT;

    for ( unsigned i = 0; i < n_bits; ++i ) {
        out<< !!( val & 1 ) << " ";
        val >>= 1;
    }
}

// this is probably completely bogus
std::string PrintSplatterType (int16_t mat1, int32_t mat2, vector<DFHack::t_matgloss> &creature_types)
{
    std::string ret;
    switch (mat1)
    {
        case 0:
            return "Rock";
        break;
        case 1:
            return "Amber";
            break;
        case 2:
            return "Coral";
            break;
        case 3:
            return "Green Glass";
            break;
        case 4:
            return "Clear Glass";
            break;
        case 5:
            return "Crystal Glass";
            break;
        case 6:
            return "Ice";
            break;
        case 7:
            return "Coal";
            break;
        case 8:
            return "Potash";
            break;
        case 9:
            return "Ash";
            break;
        case 10:
            return "Pearlash";
            break;
        case 11:
            return "Lye";
            break;
        case 12:
            return "Mud";
            break;
        case 13:
            return "Vomit";
            break;
        case 14:
            return "Salt";
            break;
        case 15:
            return "Filth";
            break;
        case 16:
            return "Frozen? Filth";
            break;
        case 18:
            return "Grime";
            break;
        case 0xF2:
            return  "Very Specific Blood (references a named creature)";
            break;
        case 0x2A:
        case 0x2B:
            if(mat2 != -1)
            {
                ret += creature_types[mat2].id;
                ret += " ";
            }
            ret += "Blood";
            return ret;
            break;
        default:
            return "Unknown";
            break;
    }
}
#endif

