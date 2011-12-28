/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/


#include "Internal.h"

#include <string>
#include <vector>
#include <algorithm>
#include <map>
using namespace std;

#include "dfhack/VersionInfo.h"
#include "dfhack/Error.h"
#include "dfhack/Process.h"
using namespace DFHack;

//Inital amount of space in levels vector (since we usually know the number, efficient!)
#define NUM_RESERVE_LVLS 20
#define NUM_RESERVE_MOODS 6

//FIXME: put those in some shared file
template<template <typename> class P = std::less >
struct compare_pair_first
{
    template<class T1, class T2>
    bool operator()(const std::pair<T1, T2>& left, const std::pair<T1, T2>& right)
    {
        return P<T1>()(left.first, right.first);
    }
};

template<template <typename> class P = std::less >
struct compare_pair_second
{
    template<class T1, class T2>
    bool operator()(const std::pair<T1, T2>& left, const std::pair<T1, T2>& right)
    {
        return P<T2>()(left.second, right.second);
    }
};

/*
* Common data types
*/
namespace DFHack
{
    struct t_type
    {
        t_type(uint32_t assign, uint32_t type, std::string classname)
        :classname(classname),assign(assign),type(type){};
        std::string classname;
        uint32_t assign;
        uint32_t type;
    };

    struct t_class
    {
        t_class(const t_class &old)
        {
            classname = old.classname;
            vtable = old.vtable;
            assign = old.assign;
            type_offset = old.type_offset;
            for(uint32_t i = 0; i < old.subs.size();i++)
            {
                t_type * t = new t_type (*old.subs[i]);
                subs.push_back(t);
            }
        }
        t_class ()
        {
            vtable = 0;
            assign = 0;
            type_offset = 0;
        }
        ~t_class()
        {
            for(uint32_t i = 0; i < subs.size();i++)
            {
                delete subs[i];
            }
            subs.clear();
        }
        std::string classname;
        uint32_t vtable;
        uint32_t assign;// index to typeclass array if multiclass. return value if not.
        uint32_t type_offset; // offset of type data for multiclass
        std::vector<t_type *> subs;
    };
    class indentr
    {
    public:
        friend std::basic_ostream<char>& operator <<(std::basic_ostream<char>&, const indentr &);
        indentr(int ns = 0, int size = 4)
        {
            numspaces = ns;
            step = size;
        }
        void indent ()
        {
            numspaces += step;
            if (numspaces < 0) numspaces = 0;
        }
        void unindent ()
        {
            numspaces -= step;
            if (numspaces < 0) numspaces = 0;
        }
        int get ()
        {
            return numspaces;
        }
        void set (int ns)
        {
            numspaces = ns;
            if (numspaces < 0) numspaces = 0;
        }
        private:
        int step;
        int numspaces;
    };
    std::basic_ostream<char>& operator<< (std::basic_ostream<char>& os, const indentr & idtr)
    {
        for(int i = 0; i < idtr.numspaces ;i++)
        {
            os << ' ';
        }
        return os;
    }
};

/*
 * Private data
 */
namespace DFHack
{
    typedef pair <INVAL_TYPE, uint32_t> nullableUint32;
    typedef map <string, nullableUint32 >::iterator uint32_Iter;
    typedef pair <INVAL_TYPE, int32_t> nullableInt32;
    typedef map <string, nullableInt32 >::iterator int32_Iter;
    typedef pair <INVAL_TYPE, string> nullableString;
    typedef map <string, nullableString >::iterator strings_Iter;
    typedef map <string, OffsetGroup *>::iterator groups_Iter;
    class OffsetGroupPrivate
    {
        public:
        map <string, nullableUint32 > addresses;
        map <string, nullableUint32 > hexvals;
        map <string, nullableInt32 > offsets;
        map <string, nullableString > strings;
        map <string, OffsetGroup *> groups;
        std::string name;
        OffsetGroup * parent;
    };
}

void OffsetGroup::createOffset(const string & key)
{
    OGd->offsets[key] = nullableInt32(NOT_SET, 0);
}

void OffsetGroup::createAddress(const string & key)
{
    OGd->addresses[key] = nullableUint32(NOT_SET, 0);
}

void OffsetGroup::createHexValue(const string & key)
{
    OGd->hexvals[key] = nullableUint32(NOT_SET, 0);
}

void OffsetGroup::createString(const string & key)
{
    OGd->strings[key] = nullableString(NOT_SET, std::string());
}

void OffsetGroup::setOffset (const string & key, const string & value, const INVAL_TYPE inval)
{
    int32_Iter it = OGd->offsets.find(key);
    if(it != OGd->offsets.end())
    {
        int32_t offset = strtol(value.c_str(), NULL, 16);
        // REPORT pointless rewrites
        if((*it).second.second == offset && (*it).second.first != NOT_SET )
            std::cout << "Pointless offset setting: " << this->getFullName() + key << endl;
        (*it).second.second = offset;
        if(inval != NOT_SET)
            (*it).second.first = inval;
    }
    else throw Error::MissingMemoryDefinition("offset", getFullName() + key);
}

void OffsetGroup::setOffsetValidity (const string & key, const INVAL_TYPE inval)
{
    if(inval != NOT_SET)
    {
        int32_Iter it = OGd->offsets.find(key);
        if(it != OGd->offsets.end())
        {
            (*it).second.first = inval;
        }
        else throw Error::MissingMemoryDefinition("offset", getFullName() + key);
    }
}

void OffsetGroup::setAddress (const string & key, const string & value, const INVAL_TYPE inval)
{
    uint32_Iter it = OGd->addresses.find(key);
    if(it != OGd->addresses.end())
    {
        uint32_t address = strtol(value.c_str(), NULL, 16);
        if((*it).second.second == address)
            std::cout << "Pointless address setting: " << this->getFullName() + key << endl;
        (*it).second.second = address;
        if(inval != NOT_SET)
            (*it).second.first = inval;
    }
    else throw Error::MissingMemoryDefinition("address", getFullName() + key);
}

void OffsetGroup::setAddressValidity (const string & key, const INVAL_TYPE inval)
{
    if(inval != NOT_SET)
    {
        uint32_Iter it = OGd->addresses.find(key);
        if(it != OGd->addresses.end())
        {
            (*it).second.first = inval;
        }
        else throw Error::MissingMemoryDefinition("address", getFullName() + key);
    }
}


void OffsetGroup::setHexValue (const string & key, const string & value, const INVAL_TYPE inval)
{
    uint32_Iter it = OGd->hexvals.find(key);
    if(it != OGd->hexvals.end())
    {
        (*it).second.second = strtol(value.c_str(), NULL, 16);
        if(inval != NOT_SET)
            (*it).second.first = inval;
    }
    else throw Error::MissingMemoryDefinition("hexvalue", getFullName() + key);
}

void OffsetGroup::setHexValueValidity (const string & key, const INVAL_TYPE inval)
{
    if(inval != NOT_SET)
    {
        uint32_Iter it = OGd->hexvals.find(key);
        if(it != OGd->hexvals.end())
        {
            (*it).second.first = inval;
        }
        else throw Error::MissingMemoryDefinition("hexvalue", getFullName() + key);
    }
}

void OffsetGroup::setString (const string & key, const string & value, const INVAL_TYPE inval)
{
    strings_Iter it = OGd->strings.find(key);
    if(it != OGd->strings.end())
    {
        (*it).second.second = value;
        if(inval != NOT_SET)
            (*it).second.first = inval;
    }
    else throw Error::MissingMemoryDefinition("string", getFullName() + key);
}

void OffsetGroup::setStringValidity (const string & key, const INVAL_TYPE inval)
{
    if(inval != NOT_SET)
    {
        strings_Iter it = OGd->strings.find(key);
        if(it != OGd->strings.end())
        {
            (*it).second.first = inval;
        }
        else throw Error::MissingMemoryDefinition("string", getFullName() + key);
    }
}

// Get named address
uint32_t OffsetGroup::getAddress (const string & key)
{
    uint32_Iter iter = OGd->addresses.find(key);

    if(iter != OGd->addresses.end())
    {
        if((*iter).second.first == IS_VALID)
            return (*iter).second.second;
        if((*iter).second.first == IS_INVALID)
            throw Error::InvalidMemoryDefinition("address", getFullName() + key);
        throw Error::UnsetMemoryDefinition("address", getFullName() + key);
    }
    throw Error::MissingMemoryDefinition("address", getFullName() + key);
}

// Get named offset, return bool instead of throwing exceptions
bool OffsetGroup::getSafeAddress (const string & key, uint32_t & out)
{
    uint32_Iter iter = OGd->addresses.find(key);
    if(iter != OGd->addresses.end() && (*iter).second.first == IS_VALID)
    {
        out = (*iter).second.second;
        return true;
    }
    return false;
}

// Get named offset
int32_t OffsetGroup::getOffset (const string & key)
{
    int32_Iter iter = OGd->offsets.find(key);
    if(iter != OGd->offsets.end())
    {
        if((*iter).second.first == IS_VALID)
            return (*iter).second.second;
        if((*iter).second.first == IS_INVALID)
            throw Error::InvalidMemoryDefinition("offset", getFullName() + key);
        throw Error::UnsetMemoryDefinition("offset", getFullName() + key);
    }
    throw Error::MissingMemoryDefinition("offset",  getFullName() + key);
}

// Get named offset, return bool instead of throwing exceptions
bool OffsetGroup::getSafeOffset (const string & key, int32_t & out)
{
    int32_Iter iter = OGd->offsets.find(key);
    if(iter != OGd->offsets.end() && (*iter).second.first == IS_VALID)
    {
        out = (*iter).second.second;
        return true;
    }
    return false;
}


// Get named numerical value
uint32_t OffsetGroup::getHexValue (const string & key)
{
    uint32_Iter iter = OGd->hexvals.find(key);
    if(iter != OGd->hexvals.end())
    {
        if((*iter).second.first == IS_VALID)
            return (*iter).second.second;
        if((*iter).second.first == IS_INVALID)
            throw Error::InvalidMemoryDefinition("hexvalue", getFullName() + key);
        throw Error::UnsetMemoryDefinition("hexvalue", getFullName() + key);
    }
    throw Error::MissingMemoryDefinition("hexvalue", getFullName() + key);
}

// Get named string
std::string OffsetGroup::getString (const string &key)
{
    strings_Iter iter = OGd->strings.find(key);
    if(iter != OGd->strings.end())
    {
        if((*iter).second.first == IS_VALID)
            return (*iter).second.second;
        if((*iter).second.first == IS_INVALID)
            throw Error::InvalidMemoryDefinition("string", getFullName() + key);
        throw Error::UnsetMemoryDefinition("string", getFullName() + key);
    }
    throw Error::MissingMemoryDefinition("string", getFullName() + key);
}

OffsetGroup * OffsetGroup::getGroup(const std::string &name)
{
    groups_Iter iter = OGd->groups.find(name);
    if(iter != OGd->groups.end())
        return ((*iter).second);
    return 0;
}

OffsetGroup * OffsetGroup::createGroup(const std::string &name)
{
    OffsetGroup * ret = getGroup(name);
    if(ret)
        return ret;
    ret = new OffsetGroup(name, this);
    OGd->groups[name] = ret;
    return ret;
}

void OffsetGroup::RebaseAddresses(int32_t offset)
{
    for(uint32_Iter iter = OGd->addresses.begin(); iter != OGd->addresses.end(); iter++)
    {
        if(iter->second.first)
            OGd->addresses[iter->first].second = iter->second.second + offset;
    }
    for(groups_Iter iter = OGd->groups.begin(); iter != OGd->groups.end(); iter++)
    {
        (*iter).second->RebaseAddresses(offset);
    }
}

OffsetGroup::OffsetGroup()
{
    OGd = new OffsetGroupPrivate();
    OGd->name = "";
    OGd->parent = 0;
}

OffsetGroup::OffsetGroup(const std::string & name, OffsetGroup * parent)
{
    OGd = new OffsetGroupPrivate();
    OGd->name = name;
    OGd->parent = parent;
}

OffsetGroup::~OffsetGroup()
{
    for(groups_Iter it = OGd->groups.begin();it != OGd->groups.end();it++)
    {
        delete (*it).second;
    }
    OGd->groups.clear();
    delete OGd;
}

std::string OffsetGroup::getName()
{
    return OGd->name;
}

OffsetGroup * OffsetGroup::getParent()
{
    return OGd->parent;
}

std::string OffsetGroup::getFullName()
{
    string temp, accum;
    OffsetGroup * curr = this;
    while(curr)
    {
        temp = curr->getName() + string("/") + accum;
        accum = temp;
        curr = curr->getParent();
    }
    return accum;
}

std::string OffsetGroup::PrintOffsets(int indentation)
{
    uint32_Iter iter;
    ostringstream ss;
    indentr i(indentation);
    typedef pair <uint32_t, pair< string, nullableUint32 > > horrible;
    vector < horrible > addrsorter;
    for(iter = OGd->addresses.begin(); iter != OGd->addresses.end(); iter++)
    {
        if(!(*iter).second.first)
            addrsorter.push_back( make_pair( 0, *iter ) );
        else
        {
            addrsorter.push_back( make_pair( (*iter).second.second, *iter ) );
        }
    }
    std::sort(addrsorter.begin(), addrsorter.end(), compare_pair_first<>());
    for(size_t idx = 0; idx < addrsorter.size();idx++)
    {
        horrible & h = addrsorter[idx];
        ss << i << "<Address name=\"" << h.second.first << "\"";
        if(h.second.second.first)
            ss << " value=\"" << hex << "0x" << h.first << "\"";
        if(h.second.second.first == IS_INVALID)
            ss << " valid=\"false\"";
        ss << " />";
        if(h.second.second.first == NOT_SET)
            ss << " NOT SET!";
        ss << endl;
    }

    typedef pair <int32_t, pair< string, nullableInt32 > > terrible;
    vector < terrible > offsorter;
    int32_Iter iter2;
    for(iter2 = OGd->offsets.begin(); iter2 != OGd->offsets.end(); iter2++)
    {
        if(!(*iter2).second.first)
            offsorter.push_back( make_pair( 0, *iter2 ) );
        else
        {
            offsorter.push_back( make_pair( (*iter2).second.second, *iter2 ) );
        }
    }
    std::sort(offsorter.begin(), offsorter.end(), compare_pair_first<>());
    for(size_t idx = 0; idx < offsorter.size();idx++)
    {
        terrible & h = offsorter[idx];
        ss << i << "<Offset name=\"" << h.second.first << "\"";
        if(h.second.second.first)
            ss << " value=\"" << hex << "0x" << h.first << "\"";
        if(h.second.second.first == IS_INVALID)
            ss << " valid=\"false\"";
        ss << " />";
        if(h.second.second.first == NOT_SET)
            ss << " NOT SET!";
        ss << endl;
    }

    for(iter = OGd->hexvals.begin(); iter != OGd->hexvals.end(); iter++)
    {
        ss << i << "<HexValue name=\"" << (*iter).first << "\"";
        if((*iter).second.first)
            ss << " value=\"" << hex << "0x" << (*iter).second.second << "\"";
        ss << " />";
        if((*iter).second.first == IS_INVALID)
            ss << " INVALID!";
        if(!(*iter).second.first)
            ss << " MISSING!";
        ss << endl;
    }
    strings_Iter iter3;
    for(iter3 = OGd->strings.begin(); iter3 != OGd->strings.end(); iter3++)
    {
        ss << i << "<HexValue name=\"" << (*iter3).first << "\"";
        if((*iter3).second.first)
            ss << " value=\"" << (*iter3).second.second << "\"";
        ss << " />";
        if((*iter3).second.first == IS_INVALID)
            ss << " INVALID!";
        if(!(*iter3).second.first)
            ss << " MISSING!";
        ss << endl;
    }
    groups_Iter iter4;
    for(iter4 = OGd->groups.begin(); iter4 != OGd->groups.end(); iter4++)
    {
        ss << i << "<Group name=\"" << (*iter4).first << "\">" << endl;
        i.indent();
        ss <<(*iter4).second->PrintOffsets(i.get());
        i.unindent();
        ss << i << "</Group>" << endl;
    }
    return ss.str();
}

// the big ugly method behind the curtain...
void OffsetGroup::setInvalid(INVAL_TYPE invalidity)
{
    if(invalidity == NOT_SET)
        return;

    uint32_Iter iter;
    for(iter = OGd->addresses.begin(); iter != OGd->addresses.end(); iter++)
    {
        if((*iter).second.first)
            (*iter).second.first = invalidity;
    }
    int32_Iter iter2;
    for(iter2 = OGd->offsets.begin(); iter2 != OGd->offsets.end(); iter2++)
    {
        if((*iter2).second.first)
            (*iter2).second.first = invalidity;
    }
    for(iter = OGd->hexvals.begin(); iter != OGd->hexvals.end(); iter++)
    {
        if((*iter).second.first)
            (*iter).second.first = invalidity;
    }
    strings_Iter iter3;
    for(iter3 = OGd->strings.begin(); iter3 != OGd->strings.end(); iter3++)
    {
        if((*iter3).second.first)
            (*iter3).second.first = invalidity;
    }
    groups_Iter iter4;
    for(iter4 = OGd->groups.begin(); iter4 != OGd->groups.end(); iter4++)
    {
        (*iter4).second->setInvalid(invalidity);
    }
}
std::vector<OffsetKey> OffsetGroup::getKeys() const
{
	std::vector<OffsetKey> ret;
	OffsetKey K;
	K.keytype=IS_ADDRESS;
    for(uint32_Iter iter = OGd->addresses.begin(); iter != OGd->addresses.end(); iter++)
    {
		K.key=iter->first;
		K.inval=iter->second.first;
		ret.push_back(K);
    }
	K.keytype=IS_OFFSET;
    for(int32_Iter iter = OGd->offsets.begin(); iter != OGd->offsets.end(); iter++)
    {
		K.key=iter->first;
		K.inval=iter->second.first;
		ret.push_back(K);
    }
	K.keytype=IS_HEX_VAL;
    for(uint32_Iter iter = OGd->hexvals.begin(); iter != OGd->hexvals.end(); iter++)
    {
        K.key=iter->first;
		K.inval=iter->second.first;
		ret.push_back(K);
    }
	K.keytype=IS_STRING;
    for(strings_Iter iter = OGd->strings.begin(); iter != OGd->strings.end(); iter++)
    {
        K.key=iter->first;
		K.inval=iter->second.first;
		ret.push_back(K);
    }
	K.keytype=IS_GROUP;
	K.inval=IS_VALID;
    for(groups_Iter iter = OGd->groups.begin(); iter != OGd->groups.end(); iter++)
    {
        K.key=iter->first;
		
		ret.push_back(K);
    }
	return ret;
}
/*
 * Private data
 */
namespace DFHack
{
    class VersionInfoPrivate
    {
        public:
        vector<string> professions;
        vector<string> jobs;
        vector<string> skills;
        vector<DFHack::t_level> levels;
        vector< vector<string> > traits;
        vector<string> moods;
        map <uint32_t, string> labors;

        // storage for class and multiclass
        vector<t_class *> classes;

        // cache for faster name lookup, indexed by classID
        vector<string> classnames;
        // map between vptr and class id, needs further type id lookup for multi-classes, not inherited
        map<uint32_t, t_class *> classIDs;

        // index for the next added class
        uint32_t classindex;

        int32_t base;
        Process * p; // the process this belongs to

        string version;
        OSType OS;
        std::string md5;
        bool has_md5;
        uint32_t PE_timestamp;
        bool has_timestamp;
    };
}


// normal constructor
VersionInfo::VersionInfo()
:d(new VersionInfoPrivate)
{
    d->base = 0;
    d->p = 0;
    d->classindex = 0;
    d->levels.reserve(NUM_RESERVE_LVLS);
    d->moods.reserve(NUM_RESERVE_MOODS);
    d->has_md5 = false;
    d->has_timestamp = false;
    OffsetGroup();
}


// copy constructor
VersionInfo::VersionInfo(const VersionInfo &old)
:d(new VersionInfoPrivate)
{
    copy(&old);
}

void OffsetGroup::copy(const OffsetGroup * old)
{
    OGd->addresses = old->OGd->addresses;
    OGd->offsets = old->OGd->offsets;
    OGd->hexvals = old->OGd->hexvals;
    OGd->strings = old->OGd->strings;
    for(groups_Iter it = old->OGd->groups.begin(); it != old->OGd->groups.end(); it++)
    {
        OffsetGroup * ogn = new OffsetGroup((*it).first, this);
        ogn->copy((*it).second);
        OGd->groups[(*it).first] = ogn;
    }
}

void VersionInfo::copy(const VersionInfo * old)
{
    d->version = old->d->version;
    d->OS = old->d->OS;
    d->md5 = old->d->md5;
    d->has_md5 = old->d->has_md5;
    d->PE_timestamp = old->d->PE_timestamp;
    d->has_timestamp = old->d->has_timestamp;
    d->base = old->d->base;
    //d->classes = old.d->classes;
    for(uint32_t i = 0; i < old->d->classes.size(); i++)
    {
        t_class * copy = new t_class(*old->d->classes[i]);
        d->classes.push_back(copy);
    }
    d->classnames = old->d->classnames;
    d->classindex = old->d->classindex;
    d->professions = old->d->professions;
    d->jobs = old->d->jobs;
    d->skills = old->d->skills;
    d->traits = old->d->traits;
    d->labors = old->d->labors;
    d->levels = old->d->levels;
    d->moods = old->d->moods;
    OffsetGroup::copy(reinterpret_cast<const OffsetGroup *>(old));
}

void VersionInfo::setParentProcess(Process * _p)
{
    d->p = _p;
}


// destructor
VersionInfo::~VersionInfo()
{
    // delete the vtables
    for(uint32_t i = 0; i < d->classes.size();i++)
    {
        delete d->classes[i];
    }
    // delete our data
    delete d;
}


void VersionInfo::setVersion(const char * v)
{
    d->version = v;
    OGd->name = v;
}


void VersionInfo::setVersion(const string &v)
{
    d->version = v;
    OGd->name = v;
}


string VersionInfo::getVersion()
{
    return d->version;
}

void VersionInfo::setMD5(const string &v)
{
    d->md5 = v;
    d->has_md5 = true;
}

bool VersionInfo::getMD5( string & output )
{
    if(!d->has_md5) return false;
    output = d->md5;
    return true;
}

void VersionInfo::setPE(uint32_t v)
{
    d->PE_timestamp = v;
    d->has_timestamp = true;
}

bool VersionInfo::getPE(uint32_t & output)
{
    if(!d->has_timestamp) return false;
    output = d->PE_timestamp;
    return true;
}

void VersionInfo::setOS(const char *os)
{
    string oss = os;
    if(oss == "windows")
        d->OS = OS_WINDOWS;
    else if(oss == "linux")
        d->OS = OS_LINUX;
    else if(oss == "apple")
        d->OS = OS_APPLE;
    else
        d->OS = OS_BAD;
}


void VersionInfo::setOS(const string &os)
{
    if(os == "windows")
        d->OS = OS_WINDOWS;
    else if(os == "linux")
        d->OS = OS_LINUX;
    else if(os == "apple")
        d->OS = OS_APPLE;
    else
        d->OS = OS_BAD;
}


void VersionInfo::setOS(OSType os)
{
    if(os >= OS_WINDOWS && os < OS_BAD)
    {
        d->OS = os;
        return;
    }
    d->OS = OS_BAD;
}


OSType VersionInfo::getOS() const
{
    return d->OS;
}

uint32_t VersionInfo::getBase () const
{
    return d->base;
}


void VersionInfo::setBase (const string &s)
{
    d->base = strtol(s.c_str(), NULL, 16);
}


void VersionInfo::setBase (const uint32_t b)
{
    d->base = b;
}


void VersionInfo::setLabor(const string & key, const string & value)
{
    uint32_t keyInt = strtol(key.c_str(), NULL, 10);
    d->labors[keyInt] = value;
}


void VersionInfo::setProfession (const string & key, const string & value)
{
    uint32_t keyInt = strtol(key.c_str(), NULL, 10);
    if(d->professions.size() <= keyInt)
    {
        d->professions.resize(keyInt+1,"");
    }
    d->professions[keyInt] = value;
}


void VersionInfo::setJob (const string & key, const string & value)
{
    uint32_t keyInt = strtol(key.c_str(), NULL, 10);
    if(d->jobs.size() <= keyInt)
    {
        d->jobs.resize(keyInt+1);
    }
    d->jobs[keyInt] = value;
}


void VersionInfo::setSkill (const string & key, const string & value)
{
    uint32_t keyInt = strtol(key.c_str(), NULL, 10);
    if(d->skills.size() <= keyInt){
        d->skills.resize(keyInt+1);
    }
    d->skills[keyInt] = value;
}


void VersionInfo::setLevel(const std::string &nLevel,
                           const std::string &nName,
                           const std::string &nXp)
{
    uint32_t keyInt = strtol(nLevel.c_str(), NULL, 10);

    if(d->levels.size() <= keyInt)
        d->levels.resize(keyInt+1);

    d->levels[keyInt].level = keyInt;
    d->levels[keyInt].name = nName;
    d->levels[keyInt].xpNxtLvl = strtol(nXp.c_str(), NULL, 10);
}


void VersionInfo::setMood(const std::string &id, const std::string &mood)
{
    uint32_t keyInt = strtol(id.c_str(), NULL, 10);

    if(d->moods.size() <= keyInt)
        d->moods.resize(keyInt+1);

    d->moods[keyInt] = mood;
}


void VersionInfo::setTrait(const string & key,
                           const string & value,
                           const string & zero,
                           const string & one,
                           const string & two,
                           const string & three,
                           const string & four,
                           const string & five)
{
    uint32_t keyInt = strtol(key.c_str(), NULL, 10);
    if(d->traits.size() <= keyInt)
    {
        d->traits.resize(keyInt+1);
    }
    d->traits[keyInt].push_back(zero);
    d->traits[keyInt].push_back(one);
    d->traits[keyInt].push_back(two);
    d->traits[keyInt].push_back(three);
    d->traits[keyInt].push_back(four);
    d->traits[keyInt].push_back(five);
    d->traits[keyInt].push_back(value);
}


// FIXME: next three methods should use some kind of custom container so it doesn't have to search so much.
t_class * VersionInfo::setClass (const char * name, uint32_t vtable, uint32_t typeoffset)
{
    if(name == 0)
        return 0;
    for (uint32_t i=0; i<d->classes.size(); i++)
    {
        if(d->classes[i]->classname == name)
        {
            if(vtable != 0)
                d->classes[i]->vtable = vtable;
            if(typeoffset != 0)
                d->classes[i]->type_offset = typeoffset;
            return d->classes[i];
        }
    }

    t_class *cls = new t_class();
    // get an unique ID and add ourselves to the index
    cls->assign = d->classindex;
    cls->classname = name;
    d->classnames.push_back(name);

    // vtables no longer a requirement
    cls->vtable = vtable;

    // multi class yes/no
    cls->type_offset = typeoffset;

    d->classes.push_back(cls);
    d->classindex++;
    return cls;

}


void VersionInfo::setClassChild (t_class * parent, const char * name, const char * type)
{
    vector <t_type *>& vec = parent->subs;
    for (uint32_t i=0; i<vec.size(); i++)
    {
        if(vec[i]->classname == name)
        {
            vec[i]->type = strtol(type, NULL, 16);
            return;
        }
    }
    // new multiclass child
    t_type *mcc = new t_type(d->classindex,strtol(type, NULL, 16),name);
    d->classnames.push_back(name);
    vec.push_back(mcc);
    d->classindex++;

    //cout << "    classtype " << name << ", assign " << mcc->assign << ", vtable  " << mcc->type << endl;
}


// FIXME: This in now DEPRECATED!
bool VersionInfo::resolveObjectToClassID(const uint32_t address, int32_t & classid)
{
    uint32_t vtable = d->p->readDWord(address);
    // try to find the vtable in our cache
    map<uint32_t, t_class *>::iterator it;
    it = d->classIDs.find(vtable);
    t_class * cl;

    // class found in cache?
    if(it != d->classIDs.end())
    {
        cl = (*it).second;
    }
    else// couldn't find?
    {
        // we set up the class for the first time
        //FIXME: use actual pointers everywhere.
        string classname = d->p->readClassName((void*)vtable);
        d->classIDs[vtable] = cl = setClass(classname.c_str(),vtable);
    }
    // and isn't a multi-class
    if(!cl->type_offset)
    {
        // return
        classid = cl->assign;
        return true;
    }
    // and is a multiclass
    else
    {
        // find the type
        vector <t_type*>& vec = cl->subs;
        uint32_t type = d->p->readWord(address + cl->type_offset);
        // return typed building if successful
        //FIXME: the vector should map directly to the type IDs here, so we don't have to mess with O(n) search
        for (uint32_t k = 0; k < vec.size();k++)
        {
            if(vec[k]->type == type)
            {
                //cout << " multi " <<  address + classes[i].type_offset << " " << vec[k].classname << endl;
                classid = vec[k]->assign;
                return true;
            }
        }
        // couldn't find the type... now what do we do here? throw?
        // this is a case where it might be a good idea, because it uncovers some deeper problems
        // we return the parent class instead, it doesn't change the API semantics and sort-of makes sense
        classid = cl->assign;
        return true;
    }
}


//ALERT: doesn't care about multiclasses
bool VersionInfo::resolveClassnameToVPtr(const string classname, void * & vptr)
{
    // FIXME: another stupid search.
    for(uint32_t i = 0;i< d->classes.size();i++)
    {
        //if(classes[i].)
        if(d->classes[i]->classname == classname) // got class
        {
            vptr = (void *) d->classes[i]->vtable;
            return true;
        }
    }
    // we failed to find anything that would match
    return false;
}


bool VersionInfo::resolveClassnameToClassID (const string classname, int32_t & classID)
{
    // FIXME: another stupid search.
    classID = -1;
    for(uint32_t i = 0;i< d->classnames.size();i++)
    {
        if(d->classnames[i] == classname)
        {
            classID = i;
            return true;
        }
    }
    // we failed to find anything that would match
    return false;
}


bool VersionInfo::resolveClassIDToClassname (const int32_t classID, string & classname)
{
    if (classID >=0 && (uint32_t)classID < d->classnames.size())
    {
        classname = d->classnames[classID];
        return true;
    }
    return false;
}


// return pointer to our internal classID -> className mapping
const vector<string> * VersionInfo::getClassIDMapping()
{
    return &d->classnames;
}


// change base of all addresses
void VersionInfo::RebaseAddresses(const int32_t new_base)
{
    OffsetGroup::RebaseAddresses(- (int32_t)d->base + new_base);
}


// change base of all addresses *and* vtable entries
void VersionInfo::RebaseAll(int32_t new_base)
{
    RebaseAddresses(new_base);
    int32_t rebase = - (int32_t)d->base + new_base;
    RebaseVTable(rebase);
}


// change all vtable entries by offset
void VersionInfo::RebaseVTable(int32_t offset)
{
    vector<t_class *>::iterator iter;
    for(iter = d->classes.begin(); iter != d->classes.end(); iter++)
    {
        (*iter)->vtable += offset;
    }
}


// Get Profession
string VersionInfo::getProfession (const uint32_t key) const
{
    if(d->professions.size() > key)
    {
        return d->professions[key];
    }
    throw Error::MissingMemoryDefinition("profession", key);
}


// Get Job
string VersionInfo::getJob (const uint32_t key) const
{
    if(d->jobs.size() > key)

    {
        return d->jobs[key];
    }
    throw Error::MissingMemoryDefinition("job", key);
}


string VersionInfo::getSkill (const uint32_t key) const
{
    if(d->skills.size() > key)
    {
        return d->skills[key];
    }
    throw Error::MissingMemoryDefinition("skill", key);
}


DFHack::t_level VersionInfo::getLevelInfo(const uint32_t level) const
{
    if(d->levels.size() > level)
    {
        return d->levels[level];
    }
    throw Error::MissingMemoryDefinition("Level", level);
}


// FIXME: ugly hack that needs to die
int absolute (int number)
{
    if (number < 0)
        return -number;
    return number;
}


string VersionInfo::getTrait (const uint32_t traitIdx, const uint32_t traitValue) const
{
    if(d->traits.size() > traitIdx)
    {
        int diff = absolute(traitValue-50);
        if(diff <= 10)
        {
            return string("");
        }
        if (traitValue >= 91)
            return d->traits[traitIdx][5];
        else if (traitValue >= 76)
            return d->traits[traitIdx][4];
        else if (traitValue >= 61)
            return d->traits[traitIdx][3];
        else if (traitValue >= 25)
            return d->traits[traitIdx][2];
        else if (traitValue >= 10)
            return d->traits[traitIdx][1];
        else
            return d->traits[traitIdx][0];
    }
    throw Error::MissingMemoryDefinition("trait", traitIdx);
}


string VersionInfo::getTraitName(const uint32_t traitIdx) const
{
    if(d->traits.size() > traitIdx)
    {
        return d->traits[traitIdx][d->traits[traitIdx].size()-1];
    }
    throw Error::MissingMemoryDefinition("traitname", traitIdx);
}


std::vector< std::vector<std::string> > const& VersionInfo::getAllTraits()
{
    return d->traits;
}

std:: map<uint32_t, std::string> const& VersionInfo::getAllLabours()
{
    return d->labors;
}

string VersionInfo::getLabor (const uint32_t laborIdx)
{
    if(d->labors.count(laborIdx))
    {
        return d->labors[laborIdx];
    }
    throw Error::MissingMemoryDefinition("labor", laborIdx);
}


std::string VersionInfo::getMood(const uint32_t moodID)
{
    if(d->moods.size() > moodID)
    {
        return d->moods[moodID];
    }
    throw Error::MissingMemoryDefinition("Mood", moodID);
}

std::string VersionInfo::PrintOffsets()
{
    ostringstream ss;
    string md5;
    uint32_t PE;
    indentr i;
    ss << i << "<Version name=\"" << getVersion() << "\">" << endl;
    i.indent();
    switch (getOS())
    {
        case OS_WINDOWS:
            if(getPE(PE))
                ss << i << "<PETimeStamp value=\"" << hex << "0x" << PE << "\" />" << endl;
        case OS_LINUX:
            if(getMD5(md5))
                ss << i << "<MD5 value=\"" << md5 << "\" />" << endl;
            break;
        default:
            ss << i << " UNKNOWN" << endl;
    }
    ss << i << "<Offsets>" << endl;
    i.indent();
    ss << OffsetGroup::PrintOffsets(i.get());
    i.unindent();
    ss << i << "</Offsets>" << endl;
    i.unindent();
    ss << i << "</Version>" << endl;
    ss << endl;
    return ss.str();
}
