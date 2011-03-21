// Dwarf fortress names are a complicated beast, in objects they are displayed
// as indexes into the language vectors
//
// use this tool if you are trying to find what the indexes are for a displayed
// name, so you can then search for it in your object

#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <sstream>
using namespace std;

#include <DFHack.h>

// returns a lower case version of the string 
string tolower (const string & s)
{
    string d (s);

    transform (d.begin (), d.end (), d.begin (), (int(*)(int)) tolower);
    return d;
}
string groupBy2(const string & s)
{
    string d;
    for(int i =2;i<s.length();i++){
        if(i%2==0)
        {
            d+= s.substr(i-2,2) + " ";
        }
    }
    d+=s.substr(s.length()-2,2);
    return(d);
}

uint32_t endian_swap(uint32_t x)
{
    x = (x>>24) | 
        ((x<<8) & 0x00FF0000) |
        ((x>>8) & 0x0000FF00) |
        (x<<24);
    return x;
}


int main (void)
{
    DFHack::ContextManager DF("Memory.xml");
    DFHack::Context * C;
    DFHack::Translation * Tran;
    try
    {
        C = DF.getSingleContext();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }

    Tran = C->getTranslation();

    if(!Tran->Start())
    {
        cerr << "Could not get Names" << endl;
        return 1;
    }
    DFHack::Dicts dicts = *(Tran->getDicts());
    DFHack::DFDict & englishWords = dicts.translations;
    DFHack::DFDict & foreignWords = dicts.foreign_languages;

    C->Detach();
    string input;

    cout << "\nSelect Name to search or q to Quit" << endl;
    getline (cin, input);
    while(input != "q"){
        for( uint32_t i = 0; i < englishWords.size();i++){
            for( uint32_t j = 0;j < englishWords[i].size();j++){
                if(englishWords[i][j] != ""){
                    uint32_t found = tolower(input).find(tolower(englishWords[i][j]));
                    if(found != string::npos){
                        stringstream value;
                        value << setfill('0') << setw(8) << hex << endian_swap(j);
                        cout << englishWords[i][j] << " "  << groupBy2(value.str()) << endl;
                    }
                }
            }
        }
        for( uint32_t i = 0; i < foreignWords.size();i++){
            for( uint32_t j = 0;j < foreignWords[i].size();j++){
                uint32_t found = tolower(input).find(tolower(foreignWords[i][j]));
                if(found != string::npos){
                    stringstream value;
                    value << setfill('0') << setw(8) << hex << endian_swap(j);
                    cout << foreignWords[i][j] << " "  << groupBy2(value.str()) << endl;
                }
            }
        }
        getline(cin,input);
    }
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    return 0;
}
