// this will be an incremental search tool in the future. now it is just a memory region dump thing

#include <iostream>
#include <climits>
#include <integers.h>
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

#include <DFTypes.h>
#include <DFHackAPI.h>
#include <DFProcessEnumerator.h>
#include <DFProcess.h>
#include <DFMemInfo.h>

//TODO: lots of optimization
void searchLoop(DFHack::API & DF, vector <DFHack::t_memrange>& ranges, int size, int alignment)
{
    int32_t test1;
    int32_t test2;
    vector <int64_t> found;
    vector <int64_t> newfound;
    found.reserve(100000);
    newfound.reserve(100000);
    
    //bool initial = 1;
    cout << "search ready - insert integers, 'p' for results" << endl;
    
    string select;
    while (1)
    {
        cout << ">>";
        DF.Detach();
        std::getline(cin, select);
        DF.Attach();
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
            newfound.clear();
            bool initial = found.empty();
            
            if(initial)
            {
                // for each range
                for (int i = 0; i < ranges.size();i++)
                {
                    // can't read? range is invalid to us
                    if(!ranges[i].read)
                        continue;
                    
                    //loop
                    for(uint64_t offset = ranges[i].start;offset <= ranges[i].end - size; offset+=alignment)
                    {
                        DF.ReadRaw(offset, size, (uint8_t *) &test2);
                        if(test1 == test2 )
                            found.push_back(offset);
                    }
                }
                cout << "found " << found.size() << " addresses" << endl;
            }
            else
            {
                for(int j = 0; j < found.size();j++)
                {
                    DF.ReadRaw(found[j], size, (uint8_t *) &test2);
                    if(test1 == test2)
                    {
                        newfound.push_back(found[j]);
                    }
                }
                
                cout << "matched " << newfound.size() << " addresses out of " << found.size() << endl;
                found = newfound;
            }
        }
        else break;
    }
}

int main (void)
{
    string select;
    DFHack::API DF("Memory.xml");
    if(!DF.Attach())
    {
        cerr << "DF not found" << endl;
        return 1;
    }
    DFHack::Process * p = DF.getProcess();
    vector <DFHack::t_memrange> ranges;
    vector <DFHack::t_memrange> selected_ranges;
    p->getMemRanges(ranges);
    cout << "Which range to search? (default is 1-4)" << endl;
    for(int i = 0; i< ranges.size();i++)
    {
        cout << dec << "(" << i << ") ";
        ranges[i].print();
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
        selected_ranges[i].print();
    }
    
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
    
    searchLoop(DF,selected_ranges, size, alignment);

    // initial value
    // cycle until you get only a few offsets (~10?)
    
    if(!DF.Detach())
    {
        cerr << "Can't detach from DF" << endl;
        return 1;
    }
    #ifndef LINUX_BUILD
    cout << "Done. Press any key to continue" << endl;
    cin.ignore();
    #endif
    return 0;
}