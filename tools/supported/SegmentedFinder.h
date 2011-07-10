#ifndef SEGMENTED_FINDER_H
#define SEGMENTED_FINDER_H
#include <malloc.h>
#include <iosfwd>
#include <iterator>

class SegmentedFinder;
class SegmentFinder
{
    public:
    SegmentFinder(DFHack::t_memrange & mr, DFHack::Context * DF, SegmentedFinder * SF)
    {
        _DF = DF;
        mr_ = mr;
		valid=false;
        if(mr.valid)
        {
            mr_.buffer = (uint8_t *)malloc (mr_.end - mr_.start);
            _SF = SF;
            try
            {
                DF->ReadRaw(mr_.start,(mr_.end - mr_.start),mr_.buffer);
                valid = true;
            }
            catch (DFHack::Error::MemoryAccessDenied &)
            {
                free(mr_.buffer);
                valid = false;
                mr.valid = false; // mark the range passed in as bad
                cout << "Range 0x" << hex << mr_.start << " - 0x" <<  mr_.end;

                if (strlen(mr_.name) != 0)
                    cout << " (" << mr_.name << ")";

                cout << dec << " not readable." << endl;
                cout << "Skipping this range on future scans." << endl;
            }
        }
    }
    ~SegmentFinder()
    {
        if(valid)
            free(mr_.buffer);
    }
    bool isValid()
    {
        return valid;
    }
    template <class needleType, class hayType, typename comparator >
    bool Find (needleType needle, const uint8_t increment , vector <uint64_t> &newfound, comparator oper)
    {
        if(!valid) return !newfound.empty();
        //loop
        for(uint64_t offset = 0; offset < (mr_.end - mr_.start) - sizeof(hayType); offset += increment)
        {
            if( oper(_SF,(hayType *)(mr_.buffer + offset), needle) )
                newfound.push_back(mr_.start + offset);
        }
        return !newfound.empty();
    }

    template < class needleType, class hayType, typename comparator >
    uint64_t FindInRange (needleType needle, comparator oper, uint64_t start, uint64_t length)
    {
        if(!valid) return 0;
        uint64_t stopper = min((mr_.end - mr_.start) - sizeof(hayType), (start - mr_.start) - sizeof(hayType) + length);
        //loop
        for(uint64_t offset = start - mr_.start; offset < stopper; offset +=1)
        {
            if( oper(_SF,(hayType *)(mr_.buffer + offset), needle) )
                return mr_.start + offset;
        }
        return 0;
    }

    template <class needleType, class hayType, typename comparator >
    bool Filter (needleType needle, vector <uint64_t> &found, vector <uint64_t> &newfound, comparator oper)
    {
        if(!valid) return !newfound.empty();
        for( uint64_t i = 0; i < found.size(); i++)
        {
            if(mr_.isInRange(found[i]))
            {
                uint64_t corrected = found[i] - mr_.start;
                if( oper(_SF,(hayType *)(mr_.buffer + corrected), needle) )
                    newfound.push_back(found[i]);
            }
        }
        return !newfound.empty();
    }
    private:
    friend class SegmentedFinder;
    SegmentedFinder * _SF;
    DFHack::Context * _DF;
    DFHack::t_memrange mr_;
    bool valid;
};

class SegmentedFinder
{
    public:
    SegmentedFinder(vector <DFHack::t_memrange>& ranges, DFHack::Context * DF)
    {
        _DF = DF;
        for(size_t i = 0; i < ranges.size(); i++)
        {
            segments.push_back(new SegmentFinder(ranges[i], DF, this));
        }
    }
    ~SegmentedFinder()
    {
        for(size_t i = 0; i < segments.size(); i++)
        {
            delete segments[i];
        }
    }
    SegmentFinder * getSegmentForAddress (uint64_t addr)
    {
        for(size_t i = 0; i < segments.size(); i++)
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
        found.clear();
        for(size_t i = 0; i < segments.size(); i++)
        {
            segments[i]->Find<needleType,hayType,comparator>(needle, increment, found, oper);
        }
        return !(found.empty());
    }

    template < class needleType, class hayType, typename comparator >
    uint64_t FindInRange (needleType needle, comparator oper, uint64_t start, uint64_t length)
    {
        SegmentFinder * sf = getSegmentForAddress(start);
        if(sf)
        {
            return sf->FindInRange<needleType,hayType,comparator>(needle, oper, start, length);
        }
        return 0;
    }

    template <class needleType, class hayType, typename comparator >
    bool Filter (const needleType needle, vector <uint64_t> &found, comparator oper)
    {
        vector <uint64_t> newfound;
        for(size_t i = 0; i < segments.size(); i++)
        {
            segments[i]->Filter<needleType,hayType,comparator>(needle, found, newfound, oper);
        }
        found.clear();
        found = newfound;
        return !(found.empty());
    }

    template <class needleType, class hayType, typename comparator >
    bool Incremental (needleType needle,  const uint8_t increment ,vector <uint64_t> &found, comparator oper)
    {
        if(found.empty())
        {
            return Find <needleType, hayType, comparator>(needle,increment,found,oper);
        }
        else
        {
            return Filter <needleType, hayType, comparator>(needle,found,oper);
        }
    }

    template <typename T>
    T * Translate(uint64_t address)
    {
        for(size_t i = 0; i < segments.size(); i++)
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

class Bytestreamdata
{
    public:
        void * object;
        uint32_t length;
        uint32_t allocated;
        uint32_t n_used;
};

class Bytestream
{
public:
    Bytestream(void * obj, uint32_t len, bool alloc = false)
    {
        d = new Bytestreamdata();
        d->allocated = alloc;
        d->object = obj;
        d->length = len;
        d->n_used = 1;
        constant = false;
    }
    Bytestream()
    {
        d = new Bytestreamdata();
        d->allocated = false;
        d->object = 0;
        d->length = 0;
        d->n_used = 1;
        constant = false;
    }
    Bytestream( Bytestream & bs)
    {
        d =bs.d;
        d->n_used++;
        constant = false;
    }
    Bytestream( const Bytestream & bs)
    {
        d =bs.d;
        d->n_used++;
        constant = true;
    }
    ~Bytestream()
    {
        d->n_used --;
        if(d->allocated && d->object && d->n_used == 0)
        {
            free (d->object);
            free (d);
        }
    }
    bool Allocate(size_t bytes)
    {
        if(constant)
            return false;
        if(d->allocated)
        {
            d->object = realloc(d->object, bytes);
        }
        else
        {
            d->object = malloc( bytes );
        }

        if(d->object)
        {
            d->allocated = bytes;
            return true;
        }
        else
        {
            d->allocated = 0;
            return false;
        }
    }
    template < class T >
    bool insert( T what )
    {
        if(constant)
            return false;
        if(d->length+sizeof(T) >= d->allocated)
            Allocate((d->length+sizeof(T)) * 2);
        (*(T *)( (uint64_t)d->object + d->length)) = what;
        d->length += sizeof(T);
        return true;
    }
    Bytestreamdata * d;
    bool constant;
};
std::ostream& operator<< ( std::ostream& out, Bytestream& bs )
{
    if(bs.d->object)
    {
        out << "bytestream " << dec << bs.d->length << "/" << bs.d->allocated << " bytes" << endl;
        for(size_t i = 0; i < bs.d->length; i++)
        {
            out << hex << (int) ((uint8_t *) bs.d->object)[i] << " ";
        }
        out << endl;
    }
    else
    {
        out << "empty bytestresm" << endl;
    }
    return out;
}

std::istream& operator>> ( std::istream& out, Bytestream& bs )
{
    string read;
    while(!out.eof())
    {
        string tmp;
        out >> tmp;
        read.append(tmp);
    }
    cout << read << endl;
    bs.d->length = 0;
    size_t first = read.find_first_of("\"");
    size_t last = read.find_last_of("\"");
    size_t start = first + 1;
    if(first == read.npos)
    {
        std::transform(read.begin(), read.end(), read.begin(), (int(*)(int)) tolower);
        bs.Allocate(read.size()); // overkill. size / 2 should be good, but this is safe
        int state = 0;
        char big = 0;
        char small = 0;
        string::iterator it = read.begin();
        // iterate through string, construct a bytestream out of 00-FF bytes
        while(it != read.end())
        {
            char reads = *it;
            if((reads >='0' && reads <= '9'))
            {
                if(state == 0)
                {
                    big = reads - '0';
                    state = 1;
                }
                else if(state == 1)
                {
                    small = reads - '0';
                    state = 0;
                    bs.insert<char>(big*16 + small);
                }
            }
            if((reads >= 'a' && reads <= 'f'))
            {
                if(state == 0)
                {
                    big = reads - 'a' + 10;
                    state = 1;
                }
                else if(state == 1)
                {
                    small = reads - 'a' + 10;
                    state = 0;
                    bs.insert<char>(big*16 + small);
                }
            }
            it++;
        }
        // we end in state= 1. should we add or should we trim... or throw errors?
        // I decided on adding
        if (state == 1)
        {
            small = 0;
            bs.insert<char>(big*16 + small);
        }
    }
    else
    {
        if(last == first)
        {
            // only one "
            last = read.size();
        }
        size_t length = last - start;
        // construct bytestream out of stuff between ""
        bs.d->length = length;
        if(length)
        {
            // todo: Bytestream should be able to handle this without external code
            bs.Allocate(length);
            bs.d->length = length;
            const char* strstart = read.c_str();
            memcpy(bs.d->object, strstart + start, length);
        }
        else
        {
            bs.d->object = 0;
        }
    }
    cout << bs;
    return out;
}

bool findBytestream (SegmentedFinder* s, void *addr, Bytestream compare )
{
    if(memcmp(addr, compare.d->object, compare.d->length) == 0)
        return true;
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

bool findStrBuffer (SegmentedFinder* s, uint32_t *addr, const char * compare )
{
    if(memcmp((const char *)addr, compare, strlen(compare)) == 0)
        return true;
    return false;
}

#endif // SEGMENTED_FINDER_H
