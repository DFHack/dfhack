// this is an incremental search tool. It only works on Linux.
// here be dragons... and ugly code :P
#include <iostream>
#include <climits>
#include <vector>
#include <map>
#include <set>
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

inline void printRange(DFHack::t_memrange * tpr)
{
    std::cout << std::hex << tpr->start << " - " << tpr->end << "|" << (tpr->read ? "r" : "-") << (tpr->write ? "w" : "-") << (tpr->execute ? "x" : "-") << "|" << tpr->name << std::endl;
}

/*
 * Since the start and/or end of a memory range can change each time we
 * detach and re-attach to the DF process, we save the indexes of the
 * memory ranges we want to look at, and re-read the ranges each time
 * we re-attach.  Hopefully we won't encounter any circumstances where
 * entire new ranges are added or removed...
 */
bool getRanges(DFHack::Process * p, vector <int>& selected_ranges)
{
    vector <DFHack::t_memrange> ranges;
    ranges.clear();
    selected_ranges.clear();
    p->getMemRanges(ranges);
    cout << "Which range to search? (default is 1-4)" << endl;
    for(size_t i = 0; i< ranges.size();i++)
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
            // empty input, assume default. observe the length of the memory
            // range vector these are hardcoded values, intended for my
            // convenience only
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
    cout << "selected ranges:" <<endl;

    for (int i = start; i <= end; i++)
    {
        // check if readable
        if (ranges[i].read)
        {
            selected_ranges.push_back(i);
            printRange(&(ranges[i]));
        }
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

bool readRanges(DFHack::Context * DF, vector <int>& selected_ranges,
               vector <DFHack::t_memrange>& ranges)
{
    DFHack::Process * p = DF->getProcess();

    vector <DFHack::t_memrange> new_ranges;
    new_ranges.clear();
    p->getMemRanges(new_ranges);

    for (size_t i = 0; i < selected_ranges.size(); i++)
    {
        int idx = selected_ranges[i];

        if (ranges.size() == i)
            // First time ranges vector has been filled in.
            ranges.push_back(new_ranges[idx]);
        // If something was wrong with the range on one memory read,
        // don't read it again.
        else if(ranges[i].valid)
            ranges[i] = new_ranges[idx];
    }

    return true;
}


template <class T>
bool Incremental ( vector <uint64_t> &found, const char * what, T& output,
                   const char *singular = "address", const char *plural = "addresses", bool numberz = false )
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
    if( sscanf(select.c_str(),"p %zd", &num) && num > 0)
    {
        cout << "Found "<< plural <<":" << endl;
        for(size_t i = 0; i < min(found.size(), num);i++)
        {
            cout << hex << "0x" << found[i] << endl;
        }
        goto incremental_more;
    }
    else if(select == "p")
    {
        cout << "Found "<< plural <<":" << endl;
        for(size_t i = 0; i < found.size();i++)
        {
            cout << hex << "0x" << found[i] << endl;
        }
        goto incremental_more;
    }
    else if(select == "q")
    {
        return false;
    }
    else if(select.empty())
    {
        goto incremental_more;
    }
    else
    {
        if(numberz)
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
        }
        stringstream ss (stringstream::in | stringstream::out);
        ss << select;
        ss >> output;
        cout << output;
        if(!ss.fail())
        {
            return true;
        }
        cout << "not a valid value for type: " << what << endl;
        goto incremental_more;
    }
}

void FindIntegers(DFHack::ContextManager & DFMgr,
                  vector <int>& selected_ranges)
{
    vector <DFHack::t_memrange> ranges;

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
    while(Incremental(found, "integer",test1,"address", "addresses",true))
    {
        DFMgr.Refresh();
        DFHack::Context * DF = DFMgr.getSingleContext();
        DF->Attach();
        readRanges(DF, selected_ranges, ranges);
        SegmentedFinder sf(ranges,DF);
        switch(size)
        {
            case 1:
                sf.Incremental<uint8_t,uint8_t>(test1,alignment,found, equalityP<uint8_t>);
                break;
            case 2:
                sf.Incremental<uint16_t,uint16_t>(test1,alignment,found, equalityP<uint16_t>);
                break;
            case 4:
                sf.Incremental<uint32_t,uint32_t>(test1,alignment,found, equalityP<uint32_t>);
                break;
        }
        DF->Detach();
    }
}

void FindVectorByLength(DFHack::ContextManager & DFMgr,
                        vector <int>& selected_ranges )
{
    vector <DFHack::t_memrange> ranges;

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
        readRanges(DF, selected_ranges, ranges);
        SegmentedFinder sf(ranges,DF);
        //sf.Incremental<int ,vecTriplet>(0,4,found,vectorAll);
        //sf.Filter<uint32_t,vecTriplet>(length * element_size,found,vectorLength<uint32_t>);
        sf.Incremental<uint32_t,vecTriplet>(length * element_size, 4 , found, vectorLength<uint32_t>);
        DF->Detach();
    }
}

void FindVectorByObjectRawname(DFHack::ContextManager & DFMgr,
                               vector <int>& selected_ranges)
{
    vector <DFHack::t_memrange> ranges;
    vector <uint64_t> found;
    string select;

    while (Incremental(found, "raw name",select,"vector","vectors"))
    {
        DFMgr.Refresh();
        DFHack::Context * DF = DFMgr.getSingleContext();
        DF->Attach();
        readRanges(DF, selected_ranges, ranges);
        SegmentedFinder sf(ranges,DF);
        sf.Find<int ,vecTriplet>(0,4,found, vectorAll);
        sf.Filter<const char * ,vecTriplet>(select.c_str(),found, vectorString);
        DF->Detach();
    }
}

void FindVectorByFirstObjectRawname(DFHack::ContextManager & DFMgr,
                                    vector <int>& selected_ranges)
{
    vector <DFHack::t_memrange> ranges;
    vector <uint64_t> found;
    string select;
    while (Incremental(found, "raw name",select,"vector","vectors"))
    {
        DFMgr.Refresh();
        DFHack::Context * DF = DFMgr.getSingleContext();
        DF->Attach();
        readRanges(DF, selected_ranges, ranges);
        SegmentedFinder sf(ranges,DF);
        sf.Find<int ,vecTriplet>(0,4,found, vectorAll);
        sf.Filter<const char * ,vecTriplet>(select.c_str(),found, vectorStringFirst);
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

void FindVectorByBounds(DFHack::ContextManager & DFMgr,
                        vector <int>& selected_ranges)
{
    vector <DFHack::t_memrange> ranges;
    vector <uint64_t> found;
    uint32_t select;
    while (Incremental(found, "address between vector.start and vector.end",select,"vector","vectors"))
    {
        DFMgr.Refresh();
        DFHack::Context * DF = DFMgr.getSingleContext();
        DF->Attach();
        readRanges(DF, selected_ranges, ranges);
        SegmentedFinder sf(ranges,DF);
        sf.Find<int ,vecTriplet>(0,4,found, vectorAll);
        sf.Filter<uint32_t ,vecTriplet>(select,found, vectorAddrWithin);
        // sort by size of vector
        std::sort(found.begin(), found.end(), VectorSizeFunctor(sf));
        DF->Detach();
    }
}

void FindPtrVectorsByObjectAddress(DFHack::ContextManager & DFMgr,
                                   vector <int>& selected_ranges)
{
    vector <DFHack::t_memrange> ranges;
    vector <uint64_t> found;
    uint32_t select;
    while (Incremental(found, "object address",select,"vector","vectors"))
    {
        DFMgr.Refresh();
        DFHack::Context * DF = DFMgr.getSingleContext();
        DF->Attach();
        readRanges(DF, selected_ranges, ranges);
        SegmentedFinder sf(ranges,DF);
        sf.Find<int ,vecTriplet>(0,4,found, vectorAll);
        sf.Filter<uint32_t ,vecTriplet>(select,found, vectorOfPtrWithin);
        DF->Detach();
    }
}

void FindStrBufs(DFHack::ContextManager & DFMgr,
                 vector <int>& selected_ranges)
{
    vector <DFHack::t_memrange> ranges;
    vector <uint64_t> found;
    string select;
    while (Incremental(found,"buffer",select,"buffer","buffers"))
    {
        DFMgr.Refresh();
        DFHack::Context * DF = DFMgr.getSingleContext();
        DF->Attach();
        readRanges(DF, selected_ranges, ranges);
        SegmentedFinder sf(ranges,DF);
        sf.Find< const char * ,uint32_t>(select.c_str(),1,found, findStrBuffer);
        DF->Detach();
    }
}



void FindStrings(DFHack::ContextManager & DFMgr,
                 vector <int>& selected_ranges)
{
    vector <DFHack::t_memrange> ranges;
    vector <uint64_t> found;
    string select;
    while (Incremental(found,"string",select,"string","strings"))
    {
        DFMgr.Refresh();
        DFHack::Context * DF = DFMgr.getSingleContext();
        DF->Attach();
        readRanges(DF, selected_ranges, ranges);
        SegmentedFinder sf(ranges,DF);
        sf.Incremental< const char * ,uint32_t>(select.c_str(),1,found, findString);
        DF->Detach();
    }
}

void FindData(DFHack::ContextManager & DFMgr,
              vector <int>& selected_ranges)
{
    vector <DFHack::t_memrange> ranges;
    vector <uint64_t> found;
    Bytestream select;
    while (Incremental(found,"byte stream",select,"byte stream","byte streams"))
    {
        DFMgr.Refresh();
        DFHack::Context * DF = DFMgr.getSingleContext();
        DF->Attach();
        readRanges(DF, selected_ranges, ranges);
        SegmentedFinder sf(ranges,DF);
        sf.Incremental< Bytestream ,uint32_t>(select,1,found, findBytestream);
        DF->Detach();
    }
}

bool TriggerIncremental ( vector <uint64_t> &found )
{
    string select;
    if(found.empty())
    {
        cout << "search ready - position the DF cursor and hit enter when ready" << endl;
    }
    else if( found.size() == 1 )
    {
        cout << "Found single coord!" << endl;
        cout << hex << "0x" << found[0] << endl;
    }
    else
    {
        cout << "Found " << dec << found.size() << " coords." << endl;
    }
    incremental_more:
    cout << ">>";
    std::getline(cin, select);
    size_t num = 0;
    if( sscanf(select.c_str(),"p %zd", &num) && num > 0)
    {
        cout << "Found coords:" << endl;
        for(size_t i = 0; i < min(found.size(), num);i++)
        {
            cout << hex << "0x" << found[i] << endl;
        }
        goto incremental_more;
    }
    else if(select == "p")
    {
        cout << "Found coords:" << endl;
        for(size_t i = 0; i < found.size();i++)
        {
            cout << hex << "0x" << found[i] << endl;
        }
        goto incremental_more;
    }
    else if(select == "q")
    {
        return false;
    }
    else return true;
}

void FindCoords(DFHack::ContextManager & DFMgr, vector <int>& selected_ranges)
{
    vector <uint64_t> found;
    vector <DFHack::t_memrange> ranges;

    int size = 4;
    do
    {
        getNumber("Select coord size (2,4 bytes)",size, 4);
    } while (size != 2 && size != 4);
    while (TriggerIncremental(found))
    {
        DFMgr.Refresh();
        DFHack::Context * DF = DFMgr.getSingleContext();
        DF->Attach();
        readRanges(DF, selected_ranges, ranges);
        DFHack::Gui * pos = DF->getGui();
        pos->Start();
        int32_t x, y, z;
        pos->getCursorCoords(x,y,z);
        cout << "Searching for: " << dec << x << ":" << y << ":" << z << endl;
        Bytestream select;
        if(size == 2)
        {
            select.insert<uint16_t>(x);
            select.insert<uint16_t>(y);
            select.insert<uint16_t>(z);
        }
        else
        {
            select.insert<uint32_t>(x);
            select.insert<uint32_t>(y);
            select.insert<uint32_t>(z);
        }
        cout << select << endl;
        SegmentedFinder sf(ranges,DF);
        sf.Incremental< Bytestream ,uint32_t>(select,1,found, findBytestream);
        DF->Detach();
    }
}

void PtrTrace(DFHack::ContextManager & DFMgr,
              vector <int>& selected_ranges)
{
    vector <DFHack::t_memrange> ranges;

    int element_size;
    do
    {
        getNumber("Set search granularity",element_size, 4);
    } while (element_size < 1);

    vector <uint64_t> found;
    set <uint64_t> check; // to detect circles
    uint32_t select;
    while (Incremental(found,"address",select,"addresses","addresses",true))
    {
        DFMgr.Refresh();
        found.clear();
        DFHack::Context * DF = DFMgr.getSingleContext();
        DF->Attach();
        readRanges(DF, selected_ranges, ranges);
        SegmentedFinder sf(ranges,DF);
        cout <<"Starting: 0x" << hex << select << endl;
        while(sf.getSegmentForAddress(select))
        {
            sf.Incremental<uint32_t,uint32_t>(select,element_size,found, equalityP<uint32_t>);
            if(found.empty())
            {
                cout << ".";
                cout.flush();
                select -=element_size;
                continue;
            }
            cout << endl;
            cout <<"Object start: 0x" << hex << select << endl;
            cout <<"Pointer:      0x" << hex << found[0] << endl;
            // make sure we don't go in circles'
            if(check.count(select))
            {
                break;
            }
            check.insert(select);
            // ascend
            select = found[0];
            found.clear();
        }
        DF->Detach();
    }
}
/*
{
    vector <uint64_t> found;
    Bytestream select;
    while (Incremental(found,"byte stream",select,"byte stream","byte streams"))
    {
        DFMgr.Refresh();
        DFHack::Context * DF = DFMgr.getSingleContext();
        DF->Attach();
        SegmentedFinder sf(ranges,DF);
        sf.Incremental< Bytestream ,uint32_t>(select,1,found, findBytestream);
        DF->Detach();
    }
}
*/
void DataPtrTrace(DFHack::ContextManager & DFMgr,
                  vector <int>& selected_ranges)
{
    vector <DFHack::t_memrange> ranges;
    int element_size;
    do
    {
        getNumber("Set search granularity",element_size, 4);
    } while (element_size < 1);

    vector <uint64_t> found;
    set <uint64_t> check; // to detect circles
    uint32_t select;
    Bytestream bs_select;
    DFHack::Context * DF = DFMgr.getSingleContext();
    DF->Attach();
    DFMgr.Refresh();
    readRanges(DF, selected_ranges, ranges);
    found.clear();
    SegmentedFinder sf(ranges,DF);
    while(found.empty())
    {
        Incremental(found,"byte stream",bs_select,"byte stream","byte streams");
        
        sf.Incremental< Bytestream ,uint32_t>(bs_select,1,found, findBytestream);
    }
    select = found[0];
    
    
    
    
    cout <<"Starting: 0x" << hex << select << endl;
    while(sf.getSegmentForAddress(select))
    {
        sf.Incremental<uint32_t,uint32_t>(select,element_size,found, equalityP<uint32_t>);
        if(found.empty())
        {
            cout << ".";
            cout.flush();
            select -=element_size;
            continue;
        }
        cout << endl;
        cout <<"Object start: 0x" << hex << select << endl;
        cout <<"Pointer:      0x" << hex << found[0] << endl;
        // make sure we don't go in circles'
        if(check.count(select))
        {
            break;
        }
        check.insert(select);
        // ascend
        select = found[0];
        found.clear();
    }
    DF->Detach();
}

void printFound(vector <uint64_t> &found, const char * what)
{
    cout << what << ":" << endl;
    for(size_t i = 0; i < found.size();i++)
    {
        cout << hex << "0x" << found[i] << endl;
    }
}

void printFoundStrVec(vector <uint64_t> &found, const char * what, SegmentedFinder & s)
{
    cout << what << ":" << endl;
    for(size_t i = 0; i < found.size();i++)
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

// meh
#pragma pack(1)
struct tilecolors
{
    uint16_t fore;
    uint16_t back;
    uint16_t bright;
};
#pragma pack()

void autoSearch(DFHack::Context * DF, vector <int>& selected_ranges)
{
    vector <DFHack::t_memrange> ranges;
    vector <uint64_t> allVectors;
    vector <uint64_t> filtVectors;
    vector <uint64_t> to_filter;

    readRanges(DF, selected_ranges, ranges);

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
    for(size_t i = 0; i < to_filter.size(); i++)
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

    // new organics vector
    to_filter = filtVectors;
    sf.Filter<const char * ,vecTriplet>("MUSHROOM_HELMET_PLUMP",to_filter, vectorString);
    sf.Filter<const char * ,vecTriplet>("MEADOW-GRASS",to_filter, vectorString);
    sf.Filter<const char * ,vecTriplet>("TUNNEL_TUBE",to_filter, vectorString);
    sf.Filter<const char * ,vecTriplet>("WEED_BLADE",to_filter, vectorString);
    sf.Filter<const char * ,vecTriplet>("EYEBALL",to_filter, vectorString);
    printFound(to_filter,"organics 31.19");

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
    }
    if(to_use)
    {
        
    }
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
    vector <int> selected_ranges;
    getRanges(p, selected_ranges);

    string prompt =
    "Select search type: 1=number(default), 2=vector by length, 3=vector>object>string,\n"
    "                    4=string, 5=automated offset search, 6=vector by address in its array,\n"
    "                    7=pointer vector by address of an object, 8=vector>first object>string\n"
    "                    9=string buffers, 10=known data, 11=backpointers, 12=data+backpointers\n"
    "                    13=coord lookup\n"
    "                    0= exit\n";
    int mode;
    bool finish = 0;
    do
    {
        getNumber(prompt,mode, 1, false);
        switch (mode)
        {
            case 0:
                finish = 1;
                break;
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
                autoSearch(DF,selected_ranges);
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
            case 9:
                DF->Detach();
                FindStrBufs(DFMgr, selected_ranges);
                break;
            case 10:
                DF->Detach();
                FindData(DFMgr, selected_ranges);
                break;
            case 11:
                DF->Detach();
                PtrTrace(DFMgr, selected_ranges);
                break;
            case 12:
                DF->Detach();
                DataPtrTrace(DFMgr, selected_ranges);
                break;
            case 13:
                DF->Detach();
                FindCoords(DFMgr, selected_ranges);
                break;
            default:
                cout << "Unknown function, try again." << endl;
        }
    } while ( !finish );
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    return 0;
}
