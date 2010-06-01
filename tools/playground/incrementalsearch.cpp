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
class SegmentedFinder;
class SegmentFinder
{
    public:
    SegmentFinder(DFHack::t_memrange & mr, DFHack::Context * DF, SegmentedFinder * SF)
    {
        _DF = DF;
        mr_ = mr;
        mr_.buffer = (uint8_t *)malloc (mr_.end - mr_.start);
        DF->ReadRaw(mr_.start,(mr_.end - mr_.start),mr_.buffer);
        _SF = SF;
    }
    ~SegmentFinder()
    {
        delete mr_.buffer;
    }
    template <class needleType, class hayType, typename comparator >
    bool Find (needleType needle,  const uint8_t increment ,vector <uint64_t> &found, vector <uint64_t> &newfound, comparator oper)
    {
        if(found.empty())
        {
            //loop
            for(uint64_t offset = 0; offset < (mr_.end - mr_.start) - sizeof(hayType); offset += increment)
            {
                if( oper(_SF,(hayType *)(mr_.buffer + offset), needle) )
                    newfound.push_back(mr_.start + offset);
            }
        }
        else
        {
            for( uint64_t i = 0; i < found.size(); i++)
            {
                if(mr_.isInRange(found[i]))
                {
                    uint64_t corrected = found[i] - mr_.start;
                    if( oper(_SF,(hayType *)(mr_.buffer + corrected), needle) )
                        newfound.push_back(found[i]);
                }
            }
        }
        return true;
    }
    private:
    friend class SegmentedFinder;
    SegmentedFinder * _SF;
    DFHack::Context * _DF;
    DFHack::t_memrange mr_;
};

class SegmentedFinder
{
    public:
    SegmentedFinder(vector <DFHack::t_memrange>& ranges, DFHack::Context * DF)
    {
        _DF = DF;
        for(int i = 0; i < ranges.size(); i++)
        {
            segments.push_back(new SegmentFinder(ranges[i], DF, this));
        }
    }
    ~SegmentedFinder()
    {
        for(int i = 0; i < segments.size(); i++)
        {
            delete segments[i];
        }
    }
    SegmentFinder * getSegmentForAddress (uint64_t addr)
    {
        for(int i = 0; i < segments.size(); i++)
        {
            if(segments[i]->mr_.isInRange(addr))
            {
                return segments[i];
            }
        }
        return 0;
    }
    template <class needleType, class hayType, typename comparator >
    bool Find (const needleType needle, const uint8_t increment, vector <uint64_t> &found, comparator oper)
    {
        vector <uint64_t> newfound;
        for(int i = 0; i < segments.size(); i++)
        {
            segments[i]->Find<needleType,hayType,comparator>(needle, increment, found, newfound, oper);
        }
        found.clear();
        found = newfound;
        return !(found.empty());
    }
    template <typename T>
    T * Translate(uint64_t address)
    {
        for(int i = 0; i < segments.size(); i++)
        {
            if(segments[i]->mr_.isInRange(address))
            {
                return (T *) (segments[i]->mr_.buffer + address - segments[i]->mr_.start);
            }
        }
        return 0;
    }
    template <typename T>
    T Read(uint64_t address)
    {
        return *Translate<T>(address);
    }
    template <typename T>
    bool Read(uint64_t address, T& target)
    {
        T * test = Translate<T>(address);
        if(test)
        {
            target = *test;
            return true;
        }
        return false;
    }
    private:
    DFHack::Context * _DF;
    vector <SegmentFinder *> segments;
};

template <typename T>
bool equalityP (SegmentedFinder* s, T *x, T y)
{
    return (*x) == y;
}

struct vecTriplet
{
    uint32_t start;
    uint32_t finish;
    uint32_t alloc_finish;
};

template <typename Needle>
bool vectorLength (SegmentedFinder* s, vecTriplet *x, Needle &y)
{
    if(x->start <= x->finish && x->finish <= x->alloc_finish)
        if((x->finish - x->start) == y)
            return true;
    return false;
}

bool vectorString (SegmentedFinder* s, vecTriplet *x, const char *y)
{
    if(x->start <= x->finish && x->finish <= x->alloc_finish)
    {
        // deref ptr start, get ptr to firt object
        uint32_t object_ptr;
        if(!s->Read(x->start,object_ptr))
            return false;
        // deref ptr to first object, get ptr to string
        uint32_t string_ptr;
        if(!s->Read(object_ptr,string_ptr))
            return false;
        // get string location in our local cache
        char * str = s->Translate<char>(string_ptr);
        if(!str)
            return false;
        if(strcmp(y, str) == 0)
            return true;
    }
    return false;
}

bool vectorAll (SegmentedFinder* s, vecTriplet *x, int )
{
    if(x->start <= x->finish && x->finish <= x->alloc_finish)
    {
        if(s->getSegmentForAddress(x->start) == s->getSegmentForAddress(x->finish)
            && s->getSegmentForAddress(x->finish) == s->getSegmentForAddress(x->alloc_finish))
            return true;
    }
    return false;
}

bool findString (SegmentedFinder* s, uint32_t *addr, const char * compare )
{
    // read string pointer, translate to local scheme
    char *str = s->Translate<char>(*addr);
    // verify
    if(!str)
        return false;
    if(strcmp(str, compare) == 0)
        return true;
    return false;
}

//TODO: lots of optimization
void searchLoop(DFHack::ContextManager & DFMgr, vector <DFHack::t_memrange>& ranges, int size, int alignment)
{
    uint32_t test1;
    vector <uint64_t> found;
    found.reserve(100);
    //bool initial = 1;
    cout << "search ready - insert integers, 'p' for results" << endl;
    string select;
    while (1)
    {
        cout << ">>";
        std::getline(cin, select);
        if(select == "p")
        {
            cout << "Found addresses:" << endl;
            for(int i = 0; i < found.size();i++)
            {
                cout << hex << "0x" << found[i] << endl;
            }
        }
        else if(sscanf(select.c_str(),"%d", &test1) == 1)
        {
            // refresh the list of processes, get first suitable, attach
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
            if( found.size() == 1)
            {
                cout << "Found an address!" << endl;
                cout << hex << "0x" << found[0] << endl;
            }
            else
                cout << "Found " << dec << found.size() << " addresses." << endl;
            DF->Detach();
        }
        else break;
    }
}

void searchLoopVector(DFHack::ContextManager & DFMgr, vector <DFHack::t_memrange>& ranges, uint32_t element_size)
{
    vecTriplet load;
    uint32_t length;
    vector <uint64_t> found;
    found.reserve(100);
    //bool initial = 1;
    cout << "search ready - insert vector length" << endl;
    string select;
    while (1)
    {
        cout << ">>";
        std::getline(cin, select);
        if(select == "p")
        {
            cout << "Found vectors:" << endl;
            for(int i = 0; i < found.size();i++)
            {
                cout << hex << "0x" << found[i] << endl;
            }
        }
        else if(sscanf(select.c_str(),"%d", &length) == 1)
        {
            // refresh the list of processes, get first suitable, attach
            DFMgr.Refresh();
            DFHack::Context * DF = DFMgr.getSingleContext();
            DF->Attach();

            // clear the list of found addresses
            found.clear();
            SegmentedFinder sf(ranges,DF);
            sf.Find<int ,vecTriplet>(0,4,found,vectorAll);
            sf.Find<uint32_t,vecTriplet>(length*element_size,4,found,vectorLength<uint32_t>);
            if( found.size() == 1)
            {
                cout << "Found an address!" << endl;
                cout << hex << "0x" << found[0] << endl;
            }
            else
                cout << "Found " << dec << found.size() << " addresses." << endl;
            // detach again
            DF->Detach();
        }
        else
        {
            break;
        }
    }
}

void searchLoopStrObjVector(DFHack::ContextManager & DFMgr, vector <DFHack::t_memrange>& ranges)
{
    vector <uint64_t> found;
    cout << "search ready - insert string" << endl;
    string select;
    while (1)
    {
        cout << ">>";
        std::getline(cin, select);
        if(select == "p")
        {
            cout << "Found vectors:" << endl;
            for(int i = 0; i < found.size();i++)
            {
                cout << hex << "0x" << found[i] << endl;
            }
        }
        else if(!select.empty())
        {
            // refresh the list of processes, get first suitable, attach
            DFMgr.Refresh();
            DFHack::Context * DF = DFMgr.getSingleContext();
            DF->Attach();

            // clear the list of found addresses
            found.clear();
            SegmentedFinder sf(ranges,DF);
            sf.Find<int ,vecTriplet>(0,4,found, vectorAll);
            sf.Find<const char * ,vecTriplet>(select.c_str(),4,found, vectorString);
            if( found.size() == 1)
            {
                cout << "Found an address!" << endl;
                cout << hex << "0x" << found[0] << endl;
            }
            else
                cout << "Found " << dec << found.size() << " addresses." << endl;
            // detach again
            DF->Detach();
        }
        else
        {
            break;
        }
    }
}

void searchLoopStr(DFHack::ContextManager & DFMgr, vector <DFHack::t_memrange>& ranges)
{
    vector <uint64_t> found;
    cout << "search ready - insert string" << endl;
    string select;
    while (1)
    {
        cout << ">>";
        std::getline(cin, select);
        if(select == "p")
        {
            cout << "Found strings:" << endl;
            for(int i = 0; i < found.size();i++)
            {
                cout << hex << "0x" << found[i] << endl;
            }
        }
        else if(!select.empty())
        {
            // refresh the list of processes, get first suitable, attach
            DFMgr.Refresh();
            DFHack::Context * DF = DFMgr.getSingleContext();
            DF->Attach();

            // clear the list of found addresses
            found.clear();
            SegmentedFinder sf(ranges,DF);
            sf.Find< const char * ,uint32_t>(select.c_str(),1,found, findString);
            if( found.size() == 1)
            {
                cout << "Found a string!" << endl;
                cout << hex << "0x" << found[0] << endl;
            }
            else
                cout << "Found " << dec << found.size() << " strings." << endl;
            // detach again
            DF->Detach();
        }
        else
        {
            break;
        }
    }
}


inline void printRange(DFHack::t_memrange * tpr)
{
    std::cout << std::hex << tpr->start << " - " << tpr->end << "|" << (tpr->read ? "r" : "-") << (tpr->write ? "w" : "-") << (tpr->execute ? "x" : "-") << "|" << tpr->name << std::endl;
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
    vector <DFHack::t_memrange> ranges;
    vector <DFHack::t_memrange> selected_ranges;
    p->getMemRanges(ranges);
    cout << "Which range to search? (default is 1-4)" << endl;
    for(int i = 0; i< ranges.size();i++)
    {
        cout << dec << "(" << i << ") ";
        printRange(&(ranges[i]));
    }

    try_again_ranges:
    cout << ">>";
    std::getline(cin, select);
    int start, end;
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
    }
    // I like the C variants here. much less object clutter
    else if(sscanf(select.c_str(), "%d-%d", &start, &end) == 2)
    {
        start = min(start, (int)ranges.size());
        end = min(end, (int)ranges.size());
    }
    else
    {
        goto try_again_ranges; // yes, this is a goto. bite me.
    }
    end++;
    cout << "selected ranges:" <<endl;
    selected_ranges.insert(selected_ranges.begin(),ranges.begin() + start, ranges.begin() + end);
    for(int i = 0; i< selected_ranges.size();i++)
    {
        printRange(&(selected_ranges[i]));
    }
    try_again_type:
    cout << "Select search type: 1=number(default), 2=vector, 3=vector>object>string, 4=string, 5=automated lang tables" << endl;
    cout << ">>";
    std::getline(cin, select);
    int mode;
    if(select.empty())
    {
        mode = 1;
    }
    else if( sscanf(select.c_str(), "%d", &mode) == 1 )
    {
        if(mode != 1 && mode != 2 && mode != 3 && mode != 4 && mode != 5)
        {
            goto try_again_type;
        }
    }
    else
    {
        goto try_again_type;
    }

    if(mode == 1)
    {
        // input / validation of variable size
        try_again_size:
        cout << "Select searched variable size (1,2,4 bytes, default is 4)" << endl;
        cout << ">>";
        std::getline(cin, select);
        int size;
        if(select.empty())
        {
            size = 4;
        }
        else if( sscanf(select.c_str(), "%d", &size) == 1 )
        {
            if(/*size != 8 &&*/ size != 4 && size != 2 && size != 1)
            {
                goto try_again_size;
            }
        }
        else
        {
            goto try_again_size;
        }

        // input / validation of variable alignment (default is to use the same alignment as size)
        try_again_align:
        cout << "Variable alignment (1,2,4 bytes, default is " << size << ")" << endl;
        cout << ">>";
        std::getline(cin, select);
        int alignment = size;
        if(select.empty())
        {
            alignment = size;
        }
        else if( sscanf(select.c_str(), "%d", &alignment) == 1 )
        {
            if(/*alignment != 8 &&*/ alignment != 4 && alignment != 2 && alignment != 1)
            {
                goto try_again_align;
            }
        }
        else
        {
            goto try_again_align;
        }
        // we detach, searchLoop looks for the process again.
        DF->Detach();
        searchLoop(DFMgr, selected_ranges, size, alignment);
    }
    else if(mode == 2)// vector
    {
        // input / validation of variable size
        try_again_vsize:
        cout << "Select searched vector item size (in bytes, default is 4)" << endl;
        cout << ">>";
        std::getline(cin, select);
        uint32_t size;
        if(select.empty())
        {
            size = 4;
        }
        else if( sscanf(select.c_str(), "%d", &size) == 1 )
        {
            if(size == 0)
            {
                goto try_again_vsize;
            }
        }
        else
        {
            goto try_again_vsize;
        }
        // we detach, searchLoop looks for the process again.
        DF->Detach();
        searchLoopVector(DFMgr, selected_ranges,size);
    }
    else if(mode == 3)// vector>object>string
    {
        searchLoopStrObjVector(DFMgr, selected_ranges);
    }
    else if(mode == 4)// string
    {
        searchLoopStr(DFMgr, selected_ranges);
    }
    else if(mode == 5) // find lang tables and stuff
    {
        vector <uint64_t> allVectors;
        vector <uint64_t> to_filter;
        uint64_t kulet_vector;
        uint64_t word_table_offset;
        uint64_t DWARF_vector;
        uint64_t DWARF_object;
        SegmentedFinder sf(selected_ranges, DF);

        // enumerate all vectors
        sf.Find<int ,vecTriplet>(0,4,allVectors, vectorAll);

        // find lang vector (neutral word table)
        to_filter = allVectors;
        sf.Find<const char * ,vecTriplet>("ABBEY",4,to_filter, vectorString);
        uint64_t lang_addr = to_filter[0];

        // find dwarven language word table
        to_filter = allVectors;
        sf.Find<const char * ,vecTriplet>("kulet",4,to_filter, vectorString);
        kulet_vector = to_filter[0];

        // find vector of languages
        to_filter = allVectors;
        sf.Find<const char * ,vecTriplet>("DWARF",4,to_filter, vectorString);

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
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    return 0;
}
