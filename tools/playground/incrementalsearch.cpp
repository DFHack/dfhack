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
    selected_ranges.insert(selected_ranges.begin(),ranges.begin() + start, ranges.begin() + end);
    for(int i = 0; i< selected_ranges.size();i++)
    {
        printRange(&(selected_ranges[i]));
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
        stringstream ss (stringstream::in | stringstream::out);
        ss << select;
        ss >> hex >> output;
        if(ss.fail())
        {
            ss >> dec >> output;
            if(ss.fail())
            {
                cout << "not a valid value for type: " << what << endl;
                goto incremental_more;
            }
        }
        return true;
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

void automatedLangtables(DFHack::Context * DF, vector <DFHack::t_memrange>& ranges)
{
    vector <uint64_t> allVectors;
    vector <uint64_t> to_filter;
    uint64_t kulet_vector;
    uint64_t word_table_offset;
    uint64_t DWARF_vector;
    uint64_t DWARF_object;
    SegmentedFinder sf(ranges, DF);

    // enumerate all vectors
    sf.Find<int ,vecTriplet>(0,4,allVectors, vectorAll);

    // find lang vector (neutral word table)
    to_filter = allVectors;
    sf.Find<const char * ,vecTriplet>("ABBEY",4,to_filter, vectorStringFirst);
    uint64_t lang_addr = to_filter[0];

    // find dwarven language word table
    to_filter = allVectors;
    sf.Find<const char * ,vecTriplet>("kulet",4,to_filter, vectorStringFirst);
    kulet_vector = to_filter[0];

    // find vector of languages
    to_filter = allVectors;
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

    string prompt =
    "Select search type: 1=number(default), 2=vector by length, 3=vector>object>string,\n"
    "                    4=string, 5=automated lang tables, 6=vector by address in its array,\n"
    "                    7=pointer vector by address of an object, 8=vector>first object>string\n";
    int mode;
    do
    {
        getNumber(prompt,mode, 1, false);
    } while (mode < 1 || mode > 8 );
    switch (mode)
    {
        case 1:
            FindIntegers(DFMgr, selected_ranges);
            break;
        case 2:
            FindVectorByLength(DFMgr, selected_ranges);
            break;
        case 3:
            FindVectorByObjectRawname(DFMgr, selected_ranges);
            break;
        case 4:
            FindStrings(DFMgr, selected_ranges);
            break;
        case 5:
            automatedLangtables(DF,selected_ranges);
            break;
        case 6:
            FindVectorByBounds(DFMgr,selected_ranges);
            break;
        case 7:
            FindPtrVectorsByObjectAddress(DFMgr,selected_ranges);
            break;
        case 8:
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
