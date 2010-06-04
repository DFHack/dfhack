#ifndef SEGMENTED_FINDER_H
#define SEGMENTED_FINDER_H

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

// find a vector of 32bit pointers, where an object pointed to has a string 'y' as the first member
bool vectorString (SegmentedFinder* s, vecTriplet *x, const char *y)
{
    uint32_t object_ptr;
    uint32_t idx = x->start;
    // iterate over vector of pointers
    for(uint32_t idx = x->start; idx < x->finish; idx += sizeof(uint32_t))
    {
        // deref ptr idx, get ptr to object
        if(!s->Read(idx,object_ptr))
        {
            return false;
        }
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

// find a vector of 32bit pointers, where the first object pointed to has a string 'y' as the first member
bool vectorStringFirst (SegmentedFinder* s, vecTriplet *x, const char *y)
{
    uint32_t object_ptr;
    uint32_t idx = x->start;
    // deref ptr idx, get ptr to object
    if(!s->Read(idx,object_ptr))
    {
        return false;
    }
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
    return false;
}

// test if the address is between vector.start and vector.finish
// not very useful alone, but could be a good step to filter some things
bool vectorAddrWithin (SegmentedFinder* s, vecTriplet *x, uint32_t address)
{
    if(address < x->finish && address >= x->start)
        return true;
    return false;
}

// test if an object address is within the vector of pointers
//
bool vectorOfPtrWithin (SegmentedFinder* s, vecTriplet *x, uint32_t address)
{
    uint32_t object_ptr;
    uint32_t idx = x->start;
    for(uint32_t idx = x->start; idx < x->finish; idx += sizeof(uint32_t))
    {
        if(!s->Read(idx,object_ptr))
        {
            return false;
        }
        if(object_ptr == address)
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

#endif // SEGMENTED_FINDER_H