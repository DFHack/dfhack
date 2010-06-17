// this is an incremental search tool. It only works on Linux.
// here be dragons... and ugly code :P
#include <iostream>
#include <climits>
#include <vector>
#include <map>
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
template <class T>
class holder
{
    public:
        vector <T> values;
        SegmentedFinder & sf;
        holder(SegmentedFinder& sff):sf(sff){};
        bool isValid(size_t idx)
        {
            
        };
};

class address
{
    public:
        uint64_t addr_;
        unsigned int valid : 1;
        virtual void print(SegmentedFinder& sff)
        {
            cout << hex << "0x" << addr_ << endl;
        };
        address(const uint64_t addr)
        {
            addr_ = addr;
            valid = false;
        }
        address & operator=(const uint64_t in)
        {
            addr_ = in;
            valid = false;
            return *this;
        }
        bool isValid(SegmentedFinder& sff)
        {
            if(valid) return true;
            if(sff.getSegmentForAddress(addr_))
            {
                valid = 1;
            }
        }
};

class Cstr: public address
{
    void print(SegmentedFinder & sf)
    {
        cout << hex << "0x" << addr_ << ": \"" << sf.Translate<char>(addr_) << "\"" << endl;
    }
};
// STL STRING
#ifdef LINUX_BUILD
class STLstr: public address
{
    
};
#endif
#ifndef LINUX_BUILD
class STLstr: public address
{
    
};
#endif

// STL VECTOR
#ifdef LINUX_BUILD
class Vector: public address
{
    
};
#endif
#ifndef LINUX_BUILD
class Vector: public address
{
    
};
#endif
class Int64: public address{};
class Int32: public address{};
class Int16: public address{};
class Int8: public address{};

inline void printRange(DFHack::t_memrange * tpr)
{
    std::cout << std::hex << tpr->start << " - " << tpr->end << "|" << (tpr->read ? "r" : "-") << (tpr->write ? "w" : "-") << (tpr->execute ? "x" : "-") << "|" << tpr->name << std::endl;
}

string rdWinString( char * offset, SegmentedFinder & sf )
{
    char * start_offset = offset + 4;
    uint32_t length = *(uint32_t *)(offset + 20);
    uint32_t capacity = *(uint32_t *)(offset + 24);
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
            if(p->getDescriptor()->getOS() == DFHack::memory_info::OS_WINDOWS)
            {
                start = min(11, (int)ranges.size());
                end = min(14, (int)ranges.size());
            }
            else if(p->getDescriptor()->getOS() == DFHack::memory_info::OS_LINUX)
            {
                start = min(11, (int)ranges.size());
                end = min(14, (int)ranges.size());
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

template <class T>
bool Incremental ( vector <uint64_t> &found, const char * what, T& output,
                   const char *singular = "address", const char *plural = "addresses" )
{
    string select;
    if(found.empty())
    {
        cout << "search ready - insert " << what << ", 'p' for results, 'p #' to limit number of results" << endl;
    }
    else if( found.size() == 1)
    {
        cout << "Found single "<< singular <<"!" << endl;
        cout << hex << "0x" << found[0] << endl;
    }
    else
    {
        cout << "Found " << dec << found.size() << " " << plural <<"." << endl;
    }
    incremental_more:
    cout << ">>";
    std::getline(cin, select);
    size_t num = 0;
    if( sscanf(select.c_str(),"p %d", &num) && num > 0)
    {
        cout << "Found "<< plural <<":" << endl;
        for(int i = 0; i < min(found.size(), num);i++)
        {
            cout << hex << "0x" << found[i] << endl;
        }
        goto incremental_more;
    }
    else if(select == "p")
    {
        cout << "Found "<< plural <<":" << endl;
        for(int i = 0; i < found.size();i++)
        {
            cout << hex << "0x" << found[i] << endl;
        }
        goto incremental_more;
    }
    else if(select.empty())
    {
        return false;
    }
    else
    {
        if( sscanf(select.c_str(),"0x%x", &output) == 1 )
        {
            //cout << dec << output << endl;
            return true;
        }
        if( sscanf(select.c_str(),"%d", &output) == 1 )
        {
            //cout << dec << output << endl;
            return true;
        }

        stringstream ss (stringstream::in | stringstream::out);
        ss << select;
        ss >> output;
        if(!ss.fail())
        {
            return true;
        }
        cout << "not a valid value for type: " << what << endl;
        goto incremental_more;
    }
}

void FindIntegers(DFHack::ContextManager & DFMgr, vector <DFHack::t_memrange>& ranges)
{
    // input / validation of variable size
    int size;
    do
    {
        getNumber("Select variable size (1,2,4 bytes)",size, 4);
    } while (size != 1 && size != 2 && size != 4);
    // input / validation of variable alignment (default is to use the same alignment as size)
    int alignment;
    do
    {
        getNumber("Select variable alignment (1,2,4 bytes)",alignment, size);
    } while (alignment != 1 && alignment != 2 && alignment != 4);

    uint32_t test1;
    vector <uint64_t> found;
    found.reserve(100);
    while(Incremental(found, "integer",test1))
    {
        DFMgr.Refresh();
        DFHack::Context * DF = DFMgr.getSingleContext();
        DF->Attach();
        SegmentedFinder sf(ranges,DF);
        switch(size)
        {
            case 1:
                sf.Find<uint8_t,uint8_t>(test1,alignment,found, equalityP<uint8_t>);
                break;
            case 2:
                sf.Find<uint16_t,uint16_t>(test1,alignment,found, equalityP<uint16_t>);
                break;
            case 4:
                sf.Find<uint32_t,uint32_t>(test1,alignment,found, equalityP<uint32_t>);
                break;
        }
        DF->Detach();
    }
}

void FindVectorByLength(DFHack::ContextManager & DFMgr, vector <DFHack::t_memrange>& ranges )
{
    int element_size;
    do
    {
        getNumber("Select searched vector item size in bytes",element_size, 4);
    } while (element_size < 1);

    uint32_t length;
    vector <uint64_t> found;
    found.reserve(100);
    while (Incremental(found, "vector length",length,"vector","vectors"))
    {
        DFMgr.Refresh();
        DFHack::Context * DF = DFMgr.getSingleContext();
        DF->Attach();
        SegmentedFinder sf(ranges,DF);
        sf.Find<int ,vecTriplet>(0,4,found,vectorAll);
        sf.Find<uint32_t,vecTriplet>(length * element_size,4,found,vectorLength<uint32_t>);
        DF->Detach();
    }
}

void FindVectorByObjectRawname(DFHack::ContextManager & DFMgr, vector <DFHack::t_memrange>& ranges)
{
    vector <uint64_t> found;
    string select;
    while (Incremental(found, "raw name",select,"vector","vectors"))
    {
        // clear the list of found addresses -- this is a one-shot
        found.clear();
        DFMgr.Refresh();
        DFHack::Context * DF = DFMgr.getSingleContext();
        DF->Attach();
        SegmentedFinder sf(ranges,DF);
        sf.Find<int ,vecTriplet>(0,4,found, vectorAll);
        sf.Find<const char * ,vecTriplet>(select.c_str(),4,found, vectorString);
        DF->Detach();
    }
}

void FindVectorByFirstObjectRawname(DFHack::ContextManager & DFMgr, vector <DFHack::t_memrange>& ranges)
{
    vector <uint64_t> found;
    string select;
    while (Incremental(found, "raw name",select,"vector","vectors"))
    {
        // clear the list of found addresses -- this is a one-shot
        found.clear();
        DFMgr.Refresh();
        DFHack::Context * DF = DFMgr.getSingleContext();
        DF->Attach();
        SegmentedFinder sf(ranges,DF);
        sf.Find<int ,vecTriplet>(0,4,found, vectorAll);
        sf.Find<const char * ,vecTriplet>(select.c_str(),4,found, vectorStringFirst);
        DF->Detach();
    }
}

struct VectorSizeFunctor : public binary_function<uint64_t, uint64_t, bool>
{
    VectorSizeFunctor(SegmentedFinder & sf):sf_(sf){}
    bool operator()( uint64_t lhs, uint64_t rhs)
    {
        vecTriplet* left = sf_.Translate<vecTriplet>(lhs);
        vecTriplet* right = sf_.Translate<vecTriplet>(rhs);
        return ((left->finish - left->start) < (right->finish - right->start));
    }
    SegmentedFinder & sf_;
};

void FindVectorByBounds(DFHack::ContextManager & DFMgr, vector <DFHack::t_memrange>& ranges)
{
    vector <uint64_t> found;
    uint32_t select;
    while (Incremental(found, "address between vector.start and vector.end",select,"vector","vectors"))
    {
        // clear the list of found addresses -- this is a one-shot
        found.clear();
        DFMgr.Refresh();
        DFHack::Context * DF = DFMgr.getSingleContext();
        DF->Attach();
        SegmentedFinder sf(ranges,DF);
        sf.Find<int ,vecTriplet>(0,4,found, vectorAll);
        sf.Find<uint32_t ,vecTriplet>(select,4,found, vectorAddrWithin);
        // sort by size of vector
        std::sort(found.begin(), found.end(), VectorSizeFunctor(sf));
        DF->Detach();
    }
}

void FindPtrVectorsByObjectAddress(DFHack::ContextManager & DFMgr, vector <DFHack::t_memrange>& ranges)
{
    vector <uint64_t> found;
    uint32_t select;
    while (Incremental(found, "object address",select,"vector","vectors"))
    {
        // clear the list of found addresses -- this is a one-shot
        found.clear();
        DFMgr.Refresh();
        DFHack::Context * DF = DFMgr.getSingleContext();
        DF->Attach();
        SegmentedFinder sf(ranges,DF);
        sf.Find<int ,vecTriplet>(0,4,found, vectorAll);
        sf.Find<uint32_t ,vecTriplet>(select,4,found, vectorOfPtrWithin);
        DF->Detach();
    }
}


void FindStrings(DFHack::ContextManager & DFMgr, vector <DFHack::t_memrange>& ranges)
{
    vector <uint64_t> found;
    string select;
    while (Incremental(found,"string",select,"string","strings"))
    {
        DFMgr.Refresh();
        DFHack::Context * DF = DFMgr.getSingleContext();
        DF->Attach();
        SegmentedFinder sf(ranges,DF);
        sf.Find< const char * ,uint32_t>(select.c_str(),1,found, findString);
        DF->Detach();
    }
}

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


void automatedLangtables(DFHack::Context * DF, vector <DFHack::t_memrange>& ranges)
{
    vector <uint64_t> allVectors;
    vector <uint64_t> filtVectors;
    vector <uint64_t> to_filter;

    cout << "stealing memory..." << endl;
    SegmentedFinder sf(ranges, DF);
    cout << "looking for vectors..." << endl;
    sf.Find<int ,vecTriplet>(0,4,allVectors, vectorAll);
/*
    // trim vectors. anything with > 10000 entries is not interesting
    for(uint64_t i = 0; i < allVectors.size();i++)
    {
        vecTriplet* vtrip = sf.Translate<vecTriplet>(allVectors[i]);
        if(vtrip)
        {
            uint64_t length = (vtrip->finish - vtrip->start) / 4;
            if(length < 10000 )
            {
                filtVectors.push_back(allVectors[i]);
            }
        }
    }
*/
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
    sf.Find<const char * ,vecTriplet>("ABBEY",4,to_filter, vectorStringFirst);
    uint64_t lang_addr = to_filter[0];

    // find dwarven language word table
    to_filter = filtVectors;
    sf.Find<const char * ,vecTriplet>("kulet",4,to_filter, vectorStringFirst);
    kulet_vector = to_filter[0];

    // find vector of languages
    to_filter = filtVectors;
    sf.Find<const char * ,vecTriplet>("DWARF",4,to_filter, vectorStringFirst);

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
    sf.Find<const char * ,vecTriplet>("IRON",4,to_filter, vectorString);
    sf.Find<const char * ,vecTriplet>("ONYX",4,to_filter, vectorString);
    sf.Find<const char * ,vecTriplet>("RAW_ADAMANTINE",4,to_filter, vectorString);
    sf.Find<const char * ,vecTriplet>("BLOODSTONE",4,to_filter, vectorString);
    printFound(to_filter,"inorganics");

    // organics vector
    to_filter = filtVectors;
    sf.Find<uint32_t,vecTriplet>(52 * 4,4,to_filter,vectorLength<uint32_t>);
    sf.Find<const char * ,vecTriplet>("MUSHROOM_HELMET_PLUMP",4,to_filter, vectorStringFirst);
    printFound(to_filter,"organics");

    // tree vector
    to_filter = filtVectors;
    sf.Find<uint32_t,vecTriplet>(31 * 4,4,to_filter,vectorLength<uint32_t>);
    sf.Find<const char * ,vecTriplet>("MANGROVE",4,to_filter, vectorStringFirst);
    printFound(to_filter,"trees");

    // plant vector
    to_filter = filtVectors;
    sf.Find<uint32_t,vecTriplet>(21 * 4,4,to_filter,vectorLength<uint32_t>);
    sf.Find<const char * ,vecTriplet>("MUSHROOM_HELMET_PLUMP",4,to_filter, vectorStringFirst);
    printFound(to_filter,"plants");

    // color descriptors
    //AMBER, 112
    to_filter = filtVectors;
    sf.Find<uint32_t,vecTriplet>(112 * 4,4,to_filter,vectorLength<uint32_t>);
    sf.Find<const char * ,vecTriplet>("AMBER",4,to_filter, vectorStringFirst);
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
    sf.Find<uint32_t,vecTriplet>(338 * 4,4,to_filter,vectorLength<uint32_t>);
    sf.Find<const char * ,vecTriplet>("AMBER",4,to_filter, vectorStringFirst);
    printFound(to_filter,"all descriptors");

    // creature type
    //ELEPHANT, ?? (demons abound)
    to_filter = filtVectors;
    //sf.Find<uint32_t,vecTriplet>(338 * 4,4,to_filter,vectorLength<uint32_t>);
    sf.Find<const char * ,vecTriplet>("ELEPHANT",4,to_filter, vectorString);
    sf.Find<const char * ,vecTriplet>("CAT",4,to_filter, vectorString);
    sf.Find<const char * ,vecTriplet>("DWARF",4,to_filter, vectorString);
    sf.Find<const char * ,vecTriplet>("WAMBLER_FLUFFY",4,to_filter, vectorString);
    sf.Find<const char * ,vecTriplet>("TOAD",4,to_filter, vectorString);
    sf.Find<const char * ,vecTriplet>("DEMON_1",4,to_filter, vectorString);

    vector <uint64_t> toad_first = to_filter;
    vector <uint64_t> elephant_first = to_filter;
    sf.Find<const char * ,vecTriplet>("TOAD",4,toad_first, vectorStringFirst);
    sf.Find<const char * ,vecTriplet>("ELEPHANT",4,elephant_first, vectorStringFirst);
    printFoundStrVec(toad_first,"toad-first creature types",sf);
    printFound(elephant_first,"elephant-first creature types");
    printFound(to_filter,"all creature types");
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

    DFHack::memory_info *minfo = DF->getMemoryInfo();
    DFHack::memory_info::OSType os = minfo->getOS();

    string prompt =
    "Select search type: 1=number(default), 2=vector by length, 3=vector>object>string,\n"
    "                    4=string, 5=automated offset search, 6=vector by address in its array,\n"
    "                    7=pointer vector by address of an object, 8=vector>first object>string\n";
    int mode;
    do
    {
        getNumber(prompt,mode, 1, false);
    } while (mode < 1 || mode > 8 );
    switch (mode)
    {
        case 1:
            DF->Detach();
            FindIntegers(DFMgr, selected_ranges);
            break;
        case 2:
            DF->Detach();
            FindVectorByLength(DFMgr, selected_ranges);
            break;
        case 3:
            DF->Detach();
            FindVectorByObjectRawname(DFMgr, selected_ranges);
            break;
        case 4:
            DF->Detach();
            FindStrings(DFMgr, selected_ranges);
            break;
        case 5:
            automatedLangtables(DF,selected_ranges);
            break;
        case 6:
            DF->Detach();
            FindVectorByBounds(DFMgr,selected_ranges);
            break;
        case 7:
            DF->Detach();
            FindPtrVectorsByObjectAddress(DFMgr,selected_ranges);
            break;
        case 8:
            DF->Detach();
            FindVectorByFirstObjectRawname(DFMgr, selected_ranges);
            break;
        default:
            cout << "not implemented :(" << endl;
    }
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    return 0;
}
