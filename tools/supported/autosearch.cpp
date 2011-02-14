// this is an incremental search tool. It only works on Linux.
// here be dragons... and ugly code :P
#include <iostream>
#include <climits>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <ctime>
#include <string.h>
#include <stdio.h>
#include <algorithm>
using namespace std;

#ifndef LINUX_BUILD
    #define WINVER 0x0500
    // this one prevents windows from infecting the global namespace with filth
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

#include <DFHack.h>
#include "SegmentedFinder.h"
class Token
{
public:
    Token(uint64_t _offset)
    {
        offset = _offset;
        offset_valid = 1;
        value_valid = 0;
        parent = 0;
    }
    Token(const std::string & offsetn)
    {
        full_offset_name = offsetn;
        offset_valid = 0;
        value_valid = 0;
        parent = 0;
    }
    Token()
    {
        offset_valid = 0;
        value_valid = 0;
        parent = 0;
    }
    virtual ~Token(){};
    virtual bool LoadData(SegmentedFinder * s) = 0;
    virtual void EmptyData()
    {
        value_valid = false;
    };
    virtual bool Match(SegmentedFinder * s, uint64_t offset) = 0;
    virtual void EmptyOffset()
    {
        offset_valid = false;
    };
    virtual bool AcquireOffset(DFHack::VersionInfo * vinfo)
    {
        vinfo->getOffset(full_offset_name);
    }
    virtual uint32_t Length() = 0;
    virtual uint64_t getAbsolute(){if(parent) return parent->getAbsolute() + offset; else return offset;};
    void setParent( Token *par )
    {
        par = parent;
    }
protected:
    uint64_t offset;// offset from the start of the parent token
    std::string full_offset_name;
    Token * parent;
    bool offset_valid :1;
    bool value_valid :1;
};

class Byte: virtual public Token
{
public:
    Byte(uint64_t _offset):Token(_offset){};
    Byte():Token(){};
    ~Byte();
    virtual bool LoadData(SegmentedFinder * s)
    {
        if(offset_valid)
        {
            char * ptr = s->Translate<char>(getAbsolute());
            if(ptr)
            {
                value = *ptr;
                value_valid = true;
                return true;
            }
        }
        return false;
    };
    // is the loaded data same as data at offset? yes -> set our offset to that.
    virtual bool Match(SegmentedFinder * s, uint64_t offs)
    {
        if(value_valid && (*s->Translate<char>(parent->getAbsolute() + offset)) == value )
        {
            if(parent)
                offset = offs - parent->getAbsolute();
            else
                offset = offs;
            return true;
        }
        return false;
    };
    virtual uint32_t Length()
    {
        return 1;
    };
private:
    char value;
};

class Short: virtual public Token
{
public:
    Short(uint64_t _offset):Token(_offset){};
    Short():Token(){};
    ~Short();
    virtual bool LoadData(SegmentedFinder * s)
    {
        if(offset_valid)
        {
            uint16_t * ptr = s->Translate<uint16_t>(getAbsolute());
            if(ptr)
            {
                value = *ptr;
                value_valid = true;
                return true;
            }
        }
        return false;
    };
    // is the loaded data same as data at offset? yes -> set our offset to that.
    virtual bool Match(SegmentedFinder * s, uint64_t offs)
    {
        if(value_valid && (*s->Translate<uint16_t>(parent->getAbsolute() + offset)) == value )
        {
            if(parent)
                offset = offs - parent->getAbsolute();
            else
                offset = offs;
            return true;
        }
        return false;
    };
    virtual uint32_t Length()
    {
        return 2;
    };
private:
    uint16_t value;
};

class Long: virtual public Token
{
public:
    Long(uint64_t _offset):Token(_offset){};
    Long():Token(){};
    ~Long();
    virtual bool LoadData(SegmentedFinder * s)
    {
        if(offset_valid)
        {
            uint32_t * ptr = s->Translate<uint32_t>(getAbsolute());
            if(ptr)
            {
                value = *ptr;
                value_valid = true;
                return true;
            }
        }
        return false;
    };
    // is the loaded data same as data at offset? yes -> set our offset to that.
    virtual bool Match(SegmentedFinder * s, uint64_t offs)
    {
        if(value_valid && (*s->Translate<uint32_t>(offs)) == value )
        {
            if(parent)
                offset = offs - parent->getAbsolute();
            else
                offset = offs;
            return true;
        }
        return false;
            
    };
    virtual uint32_t Length(){return 4;};
private:
    uint32_t value;
};

class PtrVector : virtual public Token
{
public:
    PtrVector(uint64_t _offset):Token(_offset){};
    PtrVector():Token(){};
    ~PtrVector();
    virtual uint32_t Length(){return 12;};
private:
    vector <uint64_t> value;
};

class Pointer: virtual public Token
{
public:
    Pointer(uint64_t _offset):Token(_offset){};
    Pointer():Token(){};
    ~Pointer();
    virtual uint32_t Length(){return 4;};
private:
    uint64_t value;
};

class String: virtual public Token
{
protected:
    string value;
};

class Struct: virtual public Token
{
public:
    Struct(uint64_t _offset):Token(_offset){};
    Struct():Token(){};
    ~Struct(){};
    void Add( Token * t ){members.push_back(t);};
    virtual uint32_t Length(){return 0;}; // FIXME: temporary solution, should be the minimal length of all the contents combined
    virtual bool LoadData(SegmentedFinder* s)
    {
        bool OK = true;
        for(int i = 0; i < members.size() && OK; i++)
            OK &= members[i]->LoadData(s);
        return OK;
    };
    // TODO: IMPLEMENT!
    virtual bool Match(SegmentedFinder* s, uint64_t offset)
    {
        return false;
    }
private:
    vector<Token*> members;
};

class LinuxString: virtual public String
{
public:
    LinuxString(uint64_t _offset):Token(_offset){};
    LinuxString():Token(){};
    ~LinuxString(){};
    virtual uint32_t Length(){return 4;};
    virtual bool LoadData(SegmentedFinder* s)
    {
        return false;
    }
    virtual bool Match(SegmentedFinder* s, uint64_t offset)
    {
        return false;
    }
    /*
        // read string pointer, translate to local scheme
        char *str = sf->Translate<char>(*offset);
        // verify
        if(!str)
            return false;
        uint32_t length = *(uint32_t *)(offset - 12);
        uint32_t capacity = *(uint32_t *)(offset - 8);
        if(length > capacity)
            return false;
        //char * temp = new char[length+1];
        // read data from inside the string structure
        //memcpy(temp, str,length + 1);
        output = str;
        return true;
 */
};

class WindowsString: virtual public String
{
public:
    WindowsString(uint64_t _offset):Token(_offset){};
    WindowsString():Token(){};
    ~WindowsString(){};
    virtual uint32_t Length(){return 0x1C;}; // FIXME: pouzivat Memory.xml?
    virtual bool LoadData(SegmentedFinder* s)
    {
        return false;
    }
    virtual bool Match(SegmentedFinder* s, uint64_t offset)
    {
        return false;
    }
    string rdWinString( char * offset, SegmentedFinder & sf )
    {
        char * start_offset = offset + 4; // FIXME: pouzivat Memory.xml?
        uint32_t length = *(uint32_t *)(offset + 20); // FIXME: pouzivat Memory.xml?
        uint32_t capacity = *(uint32_t *)(offset + 24); // FIXME: pouzivat Memory.xml?
        char * temp = new char[capacity+1];

        // read data from inside the string structure
        if(capacity < 16)
        {
            memcpy(temp, start_offset,capacity);
            //read(start_offset, capacity, (uint8_t *)temp);
        }
        else // read data from what the offset + 4 dword points to
        {
            start_offset = sf.Translate<char>(*(uint32_t*)start_offset);
            memcpy(temp, start_offset,capacity);
        }

        temp[length] = 0;
        string ret = temp;
        delete temp;
        return ret;
    }
};

inline void printRange(DFHack::t_memrange * tpr)
{
    std::cout << std::hex << tpr->start << " - " << tpr->end << "|" << (tpr->read ? "r" : "-") << (tpr->write ? "w" : "-") << (tpr->execute ? "x" : "-") << "|" << tpr->name << std::endl;
}

bool getRanges(DFHack::Process * p, vector <DFHack::t_memrange>& selected_ranges)
{
    vector <DFHack::t_memrange> ranges;
    selected_ranges.clear();
    p->getMemRanges(ranges);
    cout << "Which range to search? (default is 1-4)" << endl;
    for(int i = 0; i< ranges.size();i++)
    {
        cout << dec << "(" << i << ") ";
        printRange(&(ranges[i]));
    }
    int start, end;
    while(1)
    {
        string select;
        cout << ">>";
        std::getline(cin, select);
        if(select.empty())
        {
            // empty input, assume default. observe the length of the memory range vector
            // these are hardcoded values, intended for my convenience only
            if(p->getDescriptor()->getOS() == DFHack::OS_WINDOWS)
            {
                start = min(11, (int)ranges.size());
                end = min(14, (int)ranges.size());
            }
            else if(p->getDescriptor()->getOS() == DFHack::OS_LINUX)
            {
                start = min(2, (int)ranges.size());
                end = min(4, (int)ranges.size());
            }
            else
            {
                start = 1;
                end = 1;
            }
            break;
        }
        // I like the C variants here. much less object clutter
        else if(sscanf(select.c_str(), "%d-%d", &start, &end) == 2)
        {
            start = min(start, (int)ranges.size());
            end = min(end, (int)ranges.size());
            break;
        }
        else
        {
            continue;
        }
        break;
    }
    end++;
    cout << "selected ranges:" <<endl;
    vector <DFHack::t_memrange>::iterator it;
    it = ranges.begin() + start;
    while (it != ranges.begin() + end)
    {
        // check if readable
        if((*it).read)
        {
            selected_ranges.push_back(*it);
            printRange(&*it);
        }
        it++;
    }
    return true;
}

bool getNumber (string prompt, int & output, int def, bool pdef = true)
{
    cout << prompt;
    if(pdef)
        cout << " default=" << def << endl;
    while (1)
    {
        string select;
        cout << ">>";
        std::getline(cin, select);
        if(select.empty())
        {
            output = def;
            break;
        }
        else if( sscanf(select.c_str(), "%d", &output) == 1 )
        {
            break;
        }
        else
        {
            continue;
        }
    }
    return true;
}

bool getString (string prompt, string & output)
{
    cout << prompt;
    cout << ">>";
    string select;
    std::getline(cin, select);
    if(select.empty())
    {
        return false;
    }
    else
    {
        output = select;
        return true;
    }
}

// meh
#pragma pack(1)
struct tilecolors
{
    uint16_t fore;
    uint16_t back;
    uint16_t bright;
};
#pragma pack()

void printFound(vector <uint64_t> &found, const char * what)
{
    cout << what << ":" << endl;
    for(int i = 0; i < found.size();i++)
    {
        cout << hex << "0x" << found[i] << endl;
    }
}

void printFoundStrVec(vector <uint64_t> &found, const char * what, SegmentedFinder & s)
{
    cout << what << ":" << endl;
    for(int i = 0; i < found.size();i++)
    {
        cout << hex << "0x" << found[i] << endl;
        cout << "--------------------------" << endl;
        vecTriplet * vt = s.Translate<vecTriplet>(found[i]);
        if(vt)
        {
            int j = 0;
            for(uint32_t idx = vt->start; idx < vt->finish; idx += sizeof(uint32_t))
            {
                uint32_t object_ptr;
                // deref ptr idx, get ptr to object
                if(!s.Read(idx,object_ptr))
                {
                    cout << "BAD!" << endl;
                    break;
                }
                // deref ptr to first object, get ptr to string
                uint32_t string_ptr;
                if(!s.Read(object_ptr,string_ptr))
                {
                    cout << "BAD!" << endl;
                    break;
                }
                // get string location in our local cache
                char * str = s.Translate<char>(string_ptr);
                if(!str)
                {
                    cout << "BAD!" << endl;
                    break;
                }
                cout << dec << j << ":" << hex << "0x" << object_ptr << " : " << str << endl;
                j++;
            }
        }
        else
        {
            cout << "BAD!" << endl;
            break;
        }
        cout << "--------------------------" << endl;
    }
}

class TokenFactory
{
    DFHack::OSType platform;
public:
    TokenFactory(DFHack::OSType platform_in)
    {
        platform = platform_in;
    }
    template <class T>
    T * Build()
    {
        return new T;
    }
    template <class T>
    T * Build(uint64_t offset)
    {
        return new T(offset);
    }
};
template <>
String * TokenFactory::Build()
{
    switch(platform)
    {
        case DFHack::OS_WINDOWS:
            return new WindowsString();
        case DFHack::OS_LINUX:
        case DFHack::OS_APPLE:
            return new LinuxString();
    }
    return 0;
};
template <>
String * TokenFactory::Build(uint64_t offset)
{
    switch(platform)
    {
        case DFHack::OS_WINDOWS:
            return new WindowsString(offset);
        case DFHack::OS_LINUX:
        case DFHack::OS_APPLE:
            return new LinuxString(offset);
    }
    return 0;
};

void autoSearch(DFHack::Context * DF, vector <DFHack::t_memrange>& ranges, DFHack::OSType platform)
{
    cout << "stealing memory..." << endl;
    SegmentedFinder sf(ranges, DF);
    TokenFactory tf(platform);
    cout << "done!" << endl;
    Struct maps;
    maps.Add(tf.Build<String>());
    maps.Add(tf.Build<String>());
    /*
    vector <uint64_t> allVectors;
    vector <uint64_t> filtVectors;
    vector <uint64_t> to_filter;

    cout << "stealing memory..." << endl;
    SegmentedFinder sf(ranges, DF);
    cout << "looking for vectors..." << endl;
    sf.Find<int ,vecTriplet>(0,4,allVectors, vectorAll);

    filtVectors = allVectors;
    cout << "-------------------" << endl;
    cout << "!!LANGUAGE TABLES!!" << endl;
    cout << "-------------------" << endl;

    uint64_t kulet_vector;
    uint64_t word_table_offset;
    uint64_t DWARF_vector;
    uint64_t DWARF_object;

    // find lang vector (neutral word table)
    to_filter = filtVectors;
    sf.Filter<const char * ,vecTriplet>("ABBEY",to_filter, vectorStringFirst);
    uint64_t lang_addr = to_filter[0];

    // find dwarven language word table
    to_filter = filtVectors;
    sf.Filter<const char * ,vecTriplet>("kulet",to_filter, vectorStringFirst);
    kulet_vector = to_filter[0];

    // find vector of languages
    to_filter = filtVectors;
    sf.Filter<const char * ,vecTriplet>("DWARF",to_filter, vectorStringFirst);

    // verify
    for(int i = 0; i < to_filter.size(); i++)
    {
        vecTriplet * vec = sf.Translate<vecTriplet>(to_filter[i]);
        if(((vec->finish - vec->start) / 4) == 4) // verified
        {
            DWARF_vector = to_filter[i];
            DWARF_object = sf.Read<uint32_t>(vec->start);
            // compute word table offset from dwarf word table and dwarf language object addresses
            word_table_offset = kulet_vector - DWARF_object;
            break;
        }
    }
    cout << "translation vector: " << hex << "0x" << DWARF_vector << endl;
    cout << "lang vector: " << hex << "0x" << lang_addr << endl;
    cout << "word table offset: " << hex << "0x" << word_table_offset << endl;

    cout << "-------------" << endl;
    cout << "!!MATERIALS!!" << endl;
    cout << "-------------" << endl;
    // inorganics vector
    to_filter = filtVectors;
    //sf.Find<uint32_t,vecTriplet>(257 * 4,4,to_filter,vectorLength<uint32_t>);
    sf.Filter<const char * ,vecTriplet>("IRON",to_filter, vectorString);
    sf.Filter<const char * ,vecTriplet>("ONYX",to_filter, vectorString);
    sf.Filter<const char * ,vecTriplet>("RAW_ADAMANTINE",to_filter, vectorString);
    sf.Filter<const char * ,vecTriplet>("BLOODSTONE",to_filter, vectorString);
    printFound(to_filter,"inorganics");

    // organics vector
    to_filter = filtVectors;
    sf.Filter<uint32_t,vecTriplet>(52 * 4,to_filter,vectorLength<uint32_t>);
    sf.Filter<const char * ,vecTriplet>("MUSHROOM_HELMET_PLUMP",to_filter, vectorStringFirst);
    printFound(to_filter,"organics");

    // tree vector
    to_filter = filtVectors;
    sf.Filter<uint32_t,vecTriplet>(31 * 4,to_filter,vectorLength<uint32_t>);
    sf.Filter<const char * ,vecTriplet>("MANGROVE",to_filter, vectorStringFirst);
    printFound(to_filter,"trees");

    // plant vector
    to_filter = filtVectors;
    sf.Filter<uint32_t,vecTriplet>(21 * 4,to_filter,vectorLength<uint32_t>);
    sf.Filter<const char * ,vecTriplet>("MUSHROOM_HELMET_PLUMP",to_filter, vectorStringFirst);
    printFound(to_filter,"plants");

    // color descriptors
    //AMBER, 112
    to_filter = filtVectors;
    sf.Filter<uint32_t,vecTriplet>(112 * 4,to_filter,vectorLength<uint32_t>);
    sf.Filter<const char * ,vecTriplet>("AMBER",to_filter, vectorStringFirst);
    printFound(to_filter,"color descriptors");
    if(!to_filter.empty())
    {
        uint64_t vec = to_filter[0];
        vecTriplet *vtColors = sf.Translate<vecTriplet>(vec);
        uint32_t colorObj = sf.Read<uint32_t>(vtColors->start);
        cout << "Amber color:" << hex << "0x" << colorObj << endl;
        // TODO: find string 'amber', the floats
    }

    // all descriptors
    //AMBER, 338
    to_filter = filtVectors;
    sf.Filter<uint32_t,vecTriplet>(338 * 4,to_filter,vectorLength<uint32_t>);
    sf.Filter<const char * ,vecTriplet>("AMBER",to_filter, vectorStringFirst);
    printFound(to_filter,"all descriptors");

    // creature type
    //ELEPHANT, ?? (demons abound)
    to_filter = filtVectors;
    //sf.Find<uint32_t,vecTriplet>(338 * 4,4,to_filter,vectorLength<uint32_t>);
    sf.Filter<const char * ,vecTriplet>("ELEPHANT",to_filter, vectorString);
    sf.Filter<const char * ,vecTriplet>("CAT",to_filter, vectorString);
    sf.Filter<const char * ,vecTriplet>("DWARF",to_filter, vectorString);
    sf.Filter<const char * ,vecTriplet>("WAMBLER_FLUFFY",to_filter, vectorString);
    sf.Filter<const char * ,vecTriplet>("TOAD",to_filter, vectorString);
    sf.Filter<const char * ,vecTriplet>("DEMON_1",to_filter, vectorString);

    vector <uint64_t> toad_first = to_filter;
    vector <uint64_t> elephant_first = to_filter;
    sf.Filter<const char * ,vecTriplet>("TOAD",toad_first, vectorStringFirst);
    sf.Filter<const char * ,vecTriplet>("ELEPHANT",elephant_first, vectorStringFirst);
    printFoundStrVec(toad_first,"toad-first creature types",sf);
    printFound(elephant_first,"elephant-first creature types");
    printFound(to_filter,"all creature types");
    
    uint64_t to_use = 0;
    if(!elephant_first.empty())
    {
        to_use = elephant_first[0];
        vecTriplet *vtCretypes = sf.Translate<vecTriplet>(to_use);
        uint32_t elephant = sf.Read<uint32_t>(vtCretypes->start);
        uint64_t Eoffset;
        cout << "Elephant: 0x" << hex << elephant << endl;
        cout << "Elephant: rawname = 0x0" << endl;
        uint8_t letter_E = 'E';
        Eoffset = sf.FindInRange<uint8_t,uint8_t> (letter_E,equalityP<uint8_t>, elephant, 0x300 );
        if(Eoffset)
        {
            cout << "Elephant: big E = 0x" << hex << Eoffset - elephant << endl;
        }
        Eoffset = sf.FindInRange<const char *,vecTriplet> ("FEMALE",vectorStringFirst, elephant, 0x300 );
        if(Eoffset)
        {
            cout << "Elephant: caste vector = 0x" << hex << Eoffset - elephant << endl;
        }
        Eoffset = sf.FindInRange<const char *,vecTriplet> ("SKIN",vectorStringFirst, elephant, 0x2000 );
        if(Eoffset)
        {
            cout << "Elephant: extract? vector = 0x" << hex << Eoffset - elephant << endl;
        }
        tilecolors eletc = {7,0,0};
        Bytestream bs_eletc(&eletc, sizeof(tilecolors));
        cout << bs_eletc;
        Eoffset = sf.FindInRange<Bytestream,tilecolors> (bs_eletc, findBytestream, elephant, 0x300 );
        if(Eoffset)
        {
            cout << "Elephant: colors = 0x" << hex << Eoffset - elephant << endl;
        }
        //cout << "Amber color:" << hex << "0x" << colorObj << endl;
        // TODO: find string 'amber', the floats
    }
    if(!toad_first.empty())
    {
        to_use = toad_first[0];
        vecTriplet *vtCretypes = sf.Translate<vecTriplet>(to_use);
        uint32_t toad = sf.Read<uint32_t>(vtCretypes->start);
        uint64_t Eoffset;
        cout << "Toad: 0x" << hex << toad << endl;
        cout << "Toad: rawname = 0x0" << endl;
        Eoffset = sf.FindInRange<uint8_t,uint8_t> (0xF9,equalityP<uint8_t>, toad, 0x300 );
        if(Eoffset)
        {
            cout << "Toad: character (not reliable) = 0x" << hex << Eoffset - toad << endl;
        }
        Eoffset = sf.FindInRange<const char *,vecTriplet> ("FEMALE",vectorStringFirst, toad, 0x300 );
        if(Eoffset)
        {
            cout << "Toad: caste vector = 0x" << hex << Eoffset - toad << endl;
        }
        Eoffset = sf.FindInRange<const char *,vecTriplet> ("SKIN",vectorStringFirst, toad, 0x2000 );
        if(Eoffset)
        {
            cout << "Toad: extract? vector = 0x" << hex << Eoffset - toad << endl;
        }
        tilecolors toadtc = {2,0,0};
        Bytestream bs_toadc(&toadtc, sizeof(tilecolors));
        Eoffset = sf.FindInRange<Bytestream,tilecolors> (bs_toadc, findBytestream, toad, 0x300 );
        if(Eoffset)
        {
            cout << "Toad: colors = 0x" << hex << Eoffset - toad << endl;
        }
    }*/
}

int main (void)
{
    string select;
    DFHack::ContextManager DFMgr("Memory.xml");
    DFHack::Context * DF = DFMgr.getSingleContext();
    try
    {
        DF->Attach();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        #ifndef LINUX_BUILD
            cin.ignore();
        #endif
        return 1;
    }
    DFHack::Process * p = DF->getProcess();
    vector <DFHack::t_memrange> selected_ranges;
    getRanges(p,selected_ranges);

    DFHack::VersionInfo *minfo = DF->getMemoryInfo();
    autoSearch(DF,selected_ranges, minfo->getOS());
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    return 0;
}
