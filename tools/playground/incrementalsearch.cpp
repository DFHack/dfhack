// this will be an incremental search tool in the future. now it is just a memory region dump thing

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
    SegmentFinder(DFHack::t_memrange & mr, DFHack::Context * DF)
    {
        _DF = DF;
        mr_ = mr;
        mr_.buffer = (uint8_t *)malloc (mr_.end - mr_.start);
        DF->ReadRaw(mr_.start,(mr_.end - mr_.start),mr_.buffer);
    }
    ~SegmentFinder()
    {
        delete mr_.buffer;
    }
    template <class T, typename P >
    bool Find (T needle,  const uint8_t increment ,vector <uint64_t> &found, vector <uint64_t> &newfound, P oper)
    {
        if(found.empty())
        {
            //loop
            for(uint64_t offset = 0; offset < (mr_.end - mr_.start) - sizeof(T); offset += increment)
            {
                if( (*(T *)(mr_.buffer + offset)) == needle)
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
                    if( oper((T *)(mr_.buffer + corrected), needle) )
                        newfound.push_back(found[i]);
                }
            }
        }
        return true;
    }
    private:
    friend class SegmentedFinder;
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
            segments.push_back(new SegmentFinder(ranges[i], DF));
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
    }
    template<class T, typename P>
    bool Find (const T dword, const uint8_t increment, vector <uint64_t> &found, vector <uint64_t> &newfound, P oper)
    {
        newfound.clear();
        for(int i = 0; i < segments.size(); i++)
        {
            segments[i]->Find<T,P>(dword, increment, found, newfound, oper);
        }
        found.clear();
        found = newfound;
        return !(found.empty());
    }
    private:
    DFHack::Context * _DF;
    vector <SegmentFinder *> segments;
};

template <typename T>
bool equalityP (T *x, T y)
{
    return (*x) == y;
}

typedef struct
{
    uint32_t start;
    uint32_t finish;
    uint32_t alloc_finish;
} vecTriplet;

template <typename T>
bool vectorLength (T *x, T y)
{
    return (*x) == y;
}

//TODO: lots of optimization
void searchLoop(DFHack::ContextManager & DFMgr, vector <DFHack::t_memrange>& ranges, int size, int alignment)
{
    uint32_t test1;
    vector <uint64_t> found;
    vector <uint64_t> newfound;
    found.reserve(100);
    newfound.reserve(100);
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
                    sf.Find<uint8_t>(test1,alignment,found,newfound, equalityP<uint8_t>);
                    break;
                case 2:
                    sf.Find<uint16_t>(test1,alignment,found,newfound, equalityP<uint16_t>);
                    break;
                case 4:
                    sf.Find<uint32_t>(test1,alignment,found,newfound, equalityP<uint32_t>);
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

/*
class VecVerifyPredicate
{
    public:
    VecVerifyPredicate(){}
    bool operator()(vecTriplet * vt, uint64_t length)
    {
        if(vt.start <= vt.finish && vt.finish <= vt.alloc_finish)
            return true;
        return false;
    }
};
*/
void searchLoopVector(DFHack::ContextManager & DFMgr, vector <DFHack::t_memrange>& ranges, uint32_t element_size)
{
    vecTriplet load;
    uint32_t length;
    vector <int64_t> found;
    vector <int64_t> newfound;
    found.reserve(100000);
    newfound.reserve(100000);
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

            // for each range
            for (int i = 0; i < ranges.size();i++)
            {
                // can't read? range is invalid to us
                if(!ranges[i].read)
                    continue;
                //loop
                for(uint64_t offset = ranges[i].start;offset <= ranges[i].end - sizeof(vecTriplet); offset+=4)
                {
                    DF->ReadRaw(offset, sizeof(vecTriplet), (uint8_t *) &load);
                    if(load.start <= load.finish && load.finish <= load.alloc_finish)
                        if((load.finish - load.start) / element_size == length)
                            found.push_back(offset);
                }
            }
            cout << "found " << found.size() << " addresses" << endl;

            // detach again
            DF->Detach();
        }
        else
        {
            break;
        }
    }
}
void mkcopy(DFHack::ContextManager & DFMgr, vector <DFHack::t_memrange>& ranges, uint32_t element_size)
{
    DFMgr.Refresh();
    DFHack::Context * DF = DFMgr.getSingleContext();
    DF->Attach();
    for (int i = 0; i < ranges.size();i++)
    {
        // can't read? range is invalid to us
        if(!ranges[i].read)
            continue;
        char * buffah = (char *) malloc(ranges[i].end - ranges[i].start);
        if(buffah)
        {
            DF->ReadRaw(ranges[i].start,ranges[i].end - ranges[i].start, (uint8_t *) buffah);
            cerr << "buffer  for range " << i << " allocated and filled" << endl;
            free(buffah);
            cerr << "and freed" << endl;
        }
        else
            cerr << "buffer for range " << i << " failed to allocate" << endl;
        //loop
        /*
        for(uint64_t offset = ranges[i].start;offset <= ranges[i].end - sizeof(vecTriplet); offset+=4)
        {
            DF->ReadRaw(offset, sizeof(vecTriplet), (uint8_t *) &load);
            if(load.start <= load.finish && load.finish <= load.alloc_finish)
                if((load.finish - load.start) / element_size == length)
                    found.push_back(offset);
        }
        */
    }
    DF->Detach();
    DFMgr.purge();
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
    cout << "Select search type: 1=number(default), 2=vector, 3=string" << endl;
    cout << ">>";
    std::getline(cin, select);
    int mode;
    if(select.empty())
    {
        mode = 1;
    }
    else if( sscanf(select.c_str(), "%d", &mode) == 1 )
    {
        if(mode != 1 && mode != 2 && mode != 3)
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
    else if(mode == 3)// string
    {
        mkcopy(DFMgr, selected_ranges,0);
        //searchLoopString(DF, selected_ranges);
    }
    #ifndef LINUX_BUILD
        cout << "Done. Press any key to continue" << endl;
        cin.ignore();
    #endif
    return 0;
}
