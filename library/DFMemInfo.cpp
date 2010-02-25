/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf

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

#include "DFCommonInternal.h"
/*
#if !defined(NDEBUG)
#define BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING
#define BOOST_MULTI_INDEX_ENABLE_SAFE_MODE
#endif
// really, we don't need it
#define BOOST_MULTI_INDEX_DISABLE_SERIALIZATION

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>

using boost::multi_index_container;
using namespace boost::multi_index;
*/
using namespace DFHack;

/*
 * Private data
 */
class memory_info::Private
{
    public:
    map <string, uint32_t> addresses;
    map <string, uint32_t> offsets;
    map <string, uint32_t> hexvals;
    map <string, string> strings;
    
    vector<string> professions;
    vector<string> jobs;
    vector<string> skills;
    vector< vector<string> > traits;
    map <uint32_t, string> labors;
    
    // storage for class and multiclass
    vector<t_class *> classes;
    
    // cache for faster name lookup, indexed by classID
    vector<string> classnames;
    // index for the next added class
    uint32_t classindex;
    
    int32_t base;
    
    string version;
    OSType OS;
};

// normal constructor
memory_info::memory_info()
:d(new Private)
{
    d->base = 0;
    d->classindex = 0;
}

// copy constructor
memory_info::memory_info(const memory_info &old)
:d(new Private)
{
    d->version = old.d->version;
    d->OS = old.d->OS;
    d->addresses = old.d->addresses;
    d->offsets = old.d->offsets;
    d->hexvals = old.d->hexvals;
    d->strings = old.d->strings;
    d->base = old.d->base;
    //d->classes = old.d->classes;
    for(int i = 0; i < old.d->classes.size(); i++)
    {
        t_class * copy = new t_class(*old.d->classes[i]);
        d->classes.push_back(copy);
    }
    d->classindex = old.d->classindex;
    d->professions = old.d->professions;
    d->jobs = old.d->jobs;
    d->skills = old.d->skills;
    d->traits = old.d->traits;
    d->labors = old.d->labors;
}

// destructor
memory_info::~memory_info()
{
    // delete the vtables
    for(uint32_t i = 0; i < d->classes.size();i++)
    {
        delete d->classes[i];
    }
    // delete our data
    delete d;
}

void memory_info::setVersion(const char * v)
{
    d->version = v;
}


void memory_info::setVersion(const string &v)
{
    d->version = v;
}


string memory_info::getVersion()
{
    return d->version;
}


void memory_info::setOS(const char *os)
{
    string oss = os;
    if(oss == "windows")
        d->OS = OS_WINDOWS;
    else if(oss == "linux")
        d->OS = OS_LINUX;
    else
        d->OS = OS_BAD;
}


void memory_info::setOS(const string &os)
{
    if(os == "windows")
        d->OS = OS_WINDOWS;
    else if(os == "linux")
        d->OS = OS_LINUX;
    else
        d->OS = OS_BAD;
}


void memory_info::setOS(OSType os)
{
    if(os >= OS_WINDOWS && os < OS_BAD)
    {
        d->OS = os;
        return;
    }
    d->OS = OS_BAD;
}


memory_info::OSType memory_info::getOS() const
{
    return d->OS;
}

uint32_t memory_info::getBase () const
{
    return d->base;
}


void memory_info::setBase (const string &s)
{
    d->base = strtol(s.c_str(), NULL, 16);
}


void memory_info::setBase (const uint32_t b)
{
    d->base = b;
}


void memory_info::setOffset (const string & key, const string & value)
{
    uint32_t offset = strtol(value.c_str(), NULL, 16);
    d->offsets[key] = offset;
}


void memory_info::setAddress (const string & key, const string & value)
{
    uint32_t address = strtol(value.c_str(), NULL, 16);
    d->addresses[key] = address;
}


void memory_info::setHexValue (const string & key, const string & value)
{
    uint32_t hexval = strtol(value.c_str(), NULL, 16);
    d->hexvals[key] = hexval;
}


void memory_info::setString (const string & key, const string & value)
{
    d->strings[key] = value;
}

void memory_info::setLabor(const string & key, const string & value)
{
    uint32_t keyInt = strtol(key.c_str(), NULL, 10);
    d->labors[keyInt] = value;
}

void memory_info::setProfession (const string & key, const string & value)
{
    uint32_t keyInt = strtol(key.c_str(), NULL, 10);
    if(d->professions.size() <= keyInt)
    {
        d->professions.resize(keyInt+1);
    }
    d->professions[keyInt] = value;
}

void memory_info::setJob (const string & key, const string & value)
{
    uint32_t keyInt = strtol(key.c_str(), NULL, 10);
    if(d->jobs.size() <= keyInt)
    {
        d->jobs.resize(keyInt+1);
    }
    d->jobs[keyInt] = value;
}

void memory_info::setSkill (const string & key, const string & value)
{
    uint32_t keyInt = strtol(key.c_str(), NULL, 10);
    if(d->skills.size() <= keyInt){
        d->skills.resize(keyInt+1);
    }
    d->skills[keyInt] = value;
}

void memory_info::setTrait(const string & key,
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
t_class * memory_info::setClass (const char * name, uint32_t vtable, uint32_t typeoffset)
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


void memory_info::setClassChild (t_class * parent, const char * name, const char * type)
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


bool memory_info::resolveObjectToClassID(const uint32_t address, int32_t & classid)
{
    uint32_t vtable = g_pProcess->readDWord(address);
    // FIXME: stupid search. we need a better container
    for(uint32_t i = 0;i< d->classes.size();i++)
    {
        if(d->classes[i]->vtable == vtable) // got class
        {
            // if it is a multiclass, try resolving it
            if(d->classes[i]->type_offset)
            {
                vector <t_type*>& vec = d->classes[i]->subs;
                uint32_t type = g_pProcess->readWord(address + d->classes[i]->type_offset);
                //printf ("class %d:%s offset 0x%x\n", i , classes[i].classname.c_str(), classes[i].type_offset);
                // return typed building if successful
                for (uint32_t k = 0; k < vec.size();k++)
                {
                    if(vec[k]->type == type)
                    {
                        //cout << " multi " <<  address + classes[i].type_offset << " " << vec[k].classname << endl;
                        classid = vec[k]->assign;
                        return true;
                    }
                }
            }
            // otherwise return the class we found
            classid = d->classes[i]->assign;
            return true;
        }
    }
    string classname = g_pProcess->readClassName(vtable);
    setClass(classname.c_str(),vtable);
    return true;
}

//ALERT: doesn't care about multiclasses
bool memory_info::resolveClassnameToVPtr(const string classname, uint32_t & vptr)
{
    // FIXME: another stupid search.
    for(uint32_t i = 0;i< d->classes.size();i++)
    {
        //if(classes[i].)
        if(d->classes[i]->classname == classname) // got class
        {
            vptr = d->classes[i]->vtable;
            return true;
        }
    }
    // we failed to find anything that would match
    return false;
}

bool memory_info::resolveClassIDToClassname (const int32_t classID, string & classname)
{
    if (classID >=0 && classID < d->classnames.size())
    {
        classname = d->classnames[classID];
        return true;
    }
    return false;
}


// return pointer to our internal classID -> className mapping
const vector<string> * memory_info::getClassIDMapping()
{
    return &d->classnames;
}


// change base of all addresses
void memory_info::RebaseAddresses(const int32_t new_base)
{
    map<string, uint32_t>::iterator iter;
    int32_t rebase = - (int32_t)d->base + new_base;
    for(iter = d->addresses.begin(); iter != d->addresses.end(); iter++)
    {
        d->addresses[iter->first] = iter->second + rebase;
    }
}


// change base of all addresses *and* vtable entries
void memory_info::RebaseAll(int32_t new_base)
{
    map<string, uint32_t>::iterator iter;
    int32_t rebase = - (int32_t)d->base + new_base;
    for(iter = d->addresses.begin(); iter != d->addresses.end(); iter++)
    {
        d->addresses[iter->first] = iter->second + rebase;
    }
    RebaseVTable(rebase);
}


// change all vtable entries by offset
void memory_info::RebaseVTable(int32_t offset)
{
    vector<t_class *>::iterator iter;
    for(iter = d->classes.begin(); iter != d->classes.end(); iter++)
    {
        (*iter)->vtable += offset;
    }
}

// Get named address
uint32_t memory_info::getAddress (const char *key)
{
    map <string, uint32_t>::iterator iter = d->addresses.find(key);
    
    if(iter != d->addresses.end())
    {
        return (*iter).second;
    }
    return 0;
}


// Get named offset
uint32_t memory_info::getOffset (const char *key)
{
    map <string, uint32_t>::iterator iter = d->offsets.find(key);
    if(iter != d->offsets.end())
    {
        return (*iter).second;
    }
    return 0;
}

// Get named numerical value
uint32_t memory_info::getHexValue (const char *key)
{
    map <string, uint32_t>::iterator iter = d->hexvals.find(key);
    if(iter != d->hexvals.end())
    {
        return (*iter).second;
    }
    return 0;
}


// Get named address
uint32_t memory_info::getAddress (const string &key)
{
    map <string, uint32_t>::iterator iter = d->addresses.find(key);
    
    if(iter != d->addresses.end())
    {
        return (*iter).second;
    }
    return 0;
}


// Get named offset
uint32_t memory_info::getOffset (const string &key)
{
    map <string, uint32_t>::iterator iter = d->offsets.find(key);
    if(iter != d->offsets.end())
    {
        return (*iter).second;
    }
    return 0;
}

// Get named numerical value
uint32_t memory_info::getHexValue (const string &key)
{
    map <string, uint32_t>::iterator iter = d->hexvals.find(key);
    if(iter != d->hexvals.end())
    {
        return (*iter).second;
    }
    return 0;
}

// Get named string
std::string memory_info::getString (const string &key)
{
    map <string, string>::iterator iter = d->strings.find(key);
    if(iter != d->strings.end())
    {
        return (*iter).second;
    }
    return string("");
}

// Get Profession
string memory_info::getProfession (const uint32_t key) const
{
    if(d->professions.size() > key)
    {
        return d->professions[key];
    }
    else
    {
        return string("");
    }
}

// Get Job
string memory_info::getJob (const uint32_t key) const
{
    if(d->jobs.size() > key)
    {
        return d->jobs[key];
    }
    return string("Job Does Not Exist");
}

string memory_info::getSkill (const uint32_t key) const
{
    if(d->skills.size() > key)
    {
        return d->skills[key];
    }
    return string("Skill is not Defined");
}

// FIXME: ugly hack that needs to die
int absolute (int number)
{
    if (number < 0)
        return -number;
    return number;
}

string memory_info::getTrait (const uint32_t traitIdx, const uint32_t traitValue) const
{
    if(d->traits.size() > traitIdx)
    {
        int diff = absolute(traitValue-50);
        if(diff < 10)
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
    return string("Trait is not Defined");
}

string memory_info::getTraitName(const uint32_t traitIdx) const
{
    if(d->traits.size() > traitIdx)
    {
        return d->traits[traitIdx][d->traits[traitIdx].size()-1];
    }
    return string("Trait is not Defined");
}

string memory_info::getLabor (const uint32_t laborIdx)
{
    if(d->labors.count(laborIdx))
    {
        return d->labors[laborIdx];
    }
    return string("");
}

// Reset everything
/*
0xDEADC0DE
void memory_info::flush()
{
    d->base = 0;
    d->addresses.clear();
    d->offsets.clear();
    d->strings.clear();
    d->hexvals.clear();
    d->classes.clear();
    d->classindex = 0;
    d->version = "";
    d->OS = OS_BAD;
}
*/