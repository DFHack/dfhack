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
* Common data types
*/
struct t_class
{
    string classname;
    uint32_t vtable;
    bool is_multiclass;
    uint32_t multi_index;
    uint32_t assign;// index to typeclass array if multiclass. return value if not.
    uint32_t type_offset; // offset of type data for multiclass
};

struct t_type
{
    string classname;
    uint32_t assign;
    uint32_t type;
};


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
    
    vector<t_class> classes;
    vector<vector<t_type> > classsubtypes;
    int32_t base;
    uint32_t classindex;
    string version;
    OSType OS;
};

memory_info::memory_info()
:d(new Private)
{
    d->base = 0;
    d->classindex = 0;
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
    d->classes = old.d->classes;
    d->classsubtypes = old.d->classsubtypes;
    d->classindex = old.d->classindex;
    d->professions = old.d->professions;
    d->jobs = old.d->jobs;
    d->skills = old.d->skills;
    d->traits = old.d->traits;
    d->labors = old.d->labors;
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
void memory_info::setClass (const char * name, const char * vtable)
{
    for (uint32_t i=0; i<d->classes.size(); i++)
    {
        if(d->classes[i].classname == name)
        {
            d->classes[i].vtable = strtol(vtable, NULL, 16);
            return;
        }
    }
    
    t_class cls;
        cls.assign = d->classindex;
        cls.classname = name;
        cls.is_multiclass = false;
        cls.type_offset = 0;
        d->classindex++;
        cls.vtable = strtol(vtable, NULL, 16);
    d->classes.push_back(cls);
    //cout << "class " << name << ", assign " << cls.assign << ", vtable  " << cls.vtable << endl;
}


// find old entry by name, rewrite, return its multi index. otherwise make a new one, append an empty vector of t_type to classtypes,  return its index.
uint32_t memory_info::setMultiClass (const char * name, const char * vtable, const char * typeoffset)
{
    for (uint32_t i=0; i<d->classes.size(); i++)
    {
        if(d->classes[i].classname == name)
        {
            // vtable and typeoffset can be left out from the xml definition when there's already a named multiclass
            if(vtable != NULL)
                d->classes[i].vtable = strtol(vtable, NULL, 16);
            if(typeoffset != NULL)
                d->classes[i].type_offset = strtol(typeoffset, NULL, 16);
            return d->classes[i].multi_index;
        }
    }
    
    //FIXME: add checking for vtable and typeoffset here. they HAVE to be valid. maybe change the return value into a bool and pass in multi index by reference?
    t_class cls;
        cls.assign = d->classindex;
        cls.classname = name;
        cls.is_multiclass = true;
        cls.type_offset = strtol(typeoffset, NULL, 16);
        cls.vtable = strtol(vtable, NULL, 16);
        cls.multi_index = d->classsubtypes.size();
    d->classes.push_back(cls);
    d->classindex++;

    vector<t_type> thistypes;
    d->classsubtypes.push_back(thistypes);
    //cout << "multiclass " << name << ", assign " << cls.assign << ", vtable  " << cls.vtable << endl;
    return d->classsubtypes.size() - 1;
}


void memory_info::setMultiClassChild (uint32_t multi_index, const char * name, const char * type)
{
    vector <t_type>& vec = d->classsubtypes[multi_index];
    for (uint32_t i=0; i<vec.size(); i++)
    {
        if(vec[i].classname == name)
        {
            vec[i].type = strtol(type, NULL, 16);
            return;
        }
    }
    // new multiclass child
    t_type mcc;
        mcc.assign = d->classindex;
        mcc.classname = name;
        mcc.type = strtol(type, NULL, 16);
    vec.push_back(mcc);
    d->classindex++;
    //cout << "    classtype " << name << ", assign " << mcc.assign << ", vtable  " << mcc.type << endl;
}


bool memory_info::resolveClassId(uint32_t address, int32_t & classid)
{
    uint32_t vtable = g_pProcess->readDWord(address);
    // FIXME: stupid search. we need a better container
    for(uint32_t i = 0;i< d->classes.size();i++)
    {
        if(d->classes[i].vtable == vtable) // got class
        {
            // if it is a multiclass, try resolving it
            if(d->classes[i].is_multiclass)
            {
                vector <t_type>& vec = d->classsubtypes[d->classes[i].multi_index];
                uint32_t type = g_pProcess->readWord(address + d->classes[i].type_offset);
                //printf ("class %d:%s offset 0x%x\n", i , classes[i].classname.c_str(), classes[i].type_offset);
                // return typed building if successful
                for (uint32_t k = 0; k < vec.size();k++)
                {
                    if(vec[k].type == type)
                    {
                        //cout << " multi " <<  address + classes[i].type_offset << " " << vec[k].classname << endl;
                        classid = vec[k].assign;
                        return true;
                    }
                }
            }
            // otherwise return the class we found
            classid = d->classes[i].assign;
            return true;
        }
    }
    // we failed to find anything that would match
    return false;
}

//ALERT: doesn't care about multiclasses
uint32_t memory_info::getClassVPtr(string classname)
{
    // FIXME: another stupid search.
    for(uint32_t i = 0;i< d->classes.size();i++)
    {
        //if(classes[i].)
        if(d->classes[i].classname == classname) // got class
        {
            return d->classes[i].vtable;
        }
    }
    // we failed to find anything that would match
    return 0;
}

// Flatten vtables into a index<->name mapping
void memory_info::getClassIDMapping(vector<string> & v_ClassID2ObjName)
{
    for(uint32_t i = 0;i< d->classes.size();i++)
    {
        v_ClassID2ObjName.push_back(d->classes[i].classname);
        if(!d->classes[i].is_multiclass)
        {
            continue;
        }
        vector <t_type>& vec = d->classsubtypes[d->classes[i].multi_index];
        for (uint32_t k = 0; k < vec.size();k++)
        {
            v_ClassID2ObjName.push_back(vec[k].classname);
        }
    }
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
    vector<t_class>::iterator iter;
    for(iter = d->classes.begin(); iter != d->classes.end(); iter++)
    {
        iter->vtable += offset;
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
void memory_info::flush()
{
    d->base = 0;
    d->addresses.clear();
    d->offsets.clear();
    d->strings.clear();
    d->hexvals.clear();
    d->classes.clear();
    d->classsubtypes.clear();
    d->classindex = 0;
    d->version = "";
    d->OS = OS_BAD;
}
