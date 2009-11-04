// Creature dump

#include <iostream>
#include <iomanip>
#include <climits>
#include <integers.h>
#include <vector>
using namespace std;

#include <DFTypes.h>
#include <DFHackAPI.h>
/*
oh. fsck it. I'll do the hexdump now and add the things I found in a research/ folder
groups of four bytes, 4 per line
1 space between bytes
2 between groups
offset from the object start on the left
and length set from command line
default 256
*/

/*
address = absolute address of dump start
length = length in lines. 1 line = 16 bytes
*/
void hexdump (DFHackAPI& DF, uint32_t address, uint32_t length)
{
    char *buf = new char[length * 16];
    
    DF.ReadRaw(address, length * 16, (uint8_t *) buf);
    for (int i = 0; i < length; i++)
    {
        // leading offset
        cout << "0x" << hex << setw(4) << i*16 << " ";
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

void interleave_hex (DFHackAPI& DF, vector < uint32_t > & addresses, uint32_t length)
{
    vector <char * > bufs;
    
    for(int counter = 0; counter < addresses.size(); counter ++)
    {
        char * buf = new char[length * 16];
        DF.ReadRaw(addresses[counter], length * 16, (uint8_t *) buf);
        bufs.push_back(buf);
    }
    cout << setfill('0');

    // output a header
    cout << "       ";
    for (int obj = 0; obj < addresses.size(); obj++)
    {
        cout << "0x" << hex << setw(9) << addresses[obj] << "  ";
    }
    cout << endl;
    
    for(int offs = 0 ; offs < length * 16; offs += 4)
    {
        if((!(offs % 16)) && offs != 0)
        {
            cout << endl;
        }
        cout << "0x" << hex << setw(4) << offs << " ";
        for (int object = 0; object < bufs.size(); object++)
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
    for(int counter = 0; counter < addresses.size(); counter ++)
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


int main (int argc,const char* argv[])
{
    if (argc != 2) return 1;
    vector<t_matgloss> creaturestypes;
    
    DFHackAPI *pDF = CreateDFHackAPI("Memory.xml");
    DFHackAPI &DF = *pDF;
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
    
    vector <string> buildingtypes;
    uint32_t numBuildings = DF.InitReadBuildings(buildingtypes);
    vector < uint32_t > addresses;
    for(uint32_t i = 0; i < numBuildings; i++)
    {
        t_building temp;
        DF.ReadBuilding(i, temp);
        if(buildingtypes[temp.type] == argv[1])
        {
            //cout << buildingtypes[temp.type] << " 0x" << hex << temp.origin << endl;
            //hexdump(DF, temp.origin, 16);
            addresses.push_back(temp.origin);
        }
    }
    interleave_hex(DF,addresses,16);
    DF.FinishReadBuildings();
    DF.Detach();
    delete pDF;
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    return 0;
}