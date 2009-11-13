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

memory_info::memory_info()
{
    base = 0;
    classindex = 0;
}


void memory_info::setVersion(const char * v)
{
    version = v;
}


void memory_info::setVersion(string v)
{
    version = v;
}


string memory_info::getVersion()
{
    return version;
}


void memory_info::setOS(const char *os)
{
    string oss = os;
    if(oss == "windows")
        OS = OS_WINDOWS;
    else if(oss == "linux")
        OS = OS_LINUX;
    else
        OS = OS_BAD;
}


void memory_info::setOS(string os)
{
    if(os == "windows")
        OS = OS_WINDOWS;
    else if(os == "linux")
        OS = OS_LINUX;
    else
        OS = OS_BAD;
}


void memory_info::setOS(OSType os)
{
    if(os >= OS_WINDOWS && os < OS_BAD)
    {
        OS = os;
        return;
    }
    OS = OS_BAD;
}


memory_info::OSType memory_info::getOS()
{
    return OS;
}


// copy constructor
memory_info::memory_info(const memory_info &old)
{
    version = old.version;
    OS = old.OS;
    addresses = old.addresses;
    offsets = old.offsets;
    hexvals = old.hexvals;
    strings = old.strings;
    base = old.base;
    classes = old.classes;
    classsubtypes = old.classsubtypes;
    classindex = old.classindex;
    professions = old.professions;
    jobs = old.jobs;
    skills = old.skills;
	traits = old.traits;
    labors = old.labors;
}


uint32_t memory_info::getBase ()
{
    return base;
}


void memory_info::setBase (string s)
{
    base = strtol(s.c_str(), NULL, 16);
}


void memory_info::setBase (uint32_t b)
{
    base = b;
}


void memory_info::setOffset (string key, string value)
{
    uint32_t offset = strtol(value.c_str(), NULL, 16);
    offsets[key] = offset;
}


void memory_info::setAddress (string key, string value)
{
    uint32_t address = strtol(value.c_str(), NULL, 16);
    addresses[key] = address;
}


void memory_info::setHexValue (string key, string value)
{
    uint32_t hexval = strtol(value.c_str(), NULL, 16);
    hexvals[key] = hexval;
}


void memory_info::setString (string key, string value)
{
    strings[key] = value;
}

void memory_info::setLabor(string key, string value)
{
    uint32_t keyInt = strtol(key.c_str(), NULL, 10);
    labors[keyInt] = value;
}
void memory_info::setProfession (string key, string value)
{
    uint32_t keyInt = strtol(key.c_str(), NULL, 10);
    if(professions.size() <= keyInt){
        professions.resize(keyInt+1);
    }
    professions[keyInt] = value;
}

void memory_info::setJob (string key, string value)
{
    uint32_t keyInt = strtol(key.c_str(), NULL, 10);
    if(jobs.size() <= keyInt){
        jobs.resize(keyInt+1);
    }
    jobs[keyInt] = value;
}

void memory_info::setSkill (string key, string value)
{
    uint32_t keyInt = strtol(key.c_str(), NULL, 10);
    if(skills.size() <= keyInt){
        skills.resize(keyInt+1);
    }
    skills[keyInt] = value;
}

void memory_info::setTrait(string key,string value,string zero,string one,string two,string three,string four,string five)
{
	uint32_t keyInt = strtol(key.c_str(), NULL, 10);
	if(traits.size() <= keyInt){
		traits.resize(keyInt+1);
	}
	traits[keyInt].push_back(zero);
	traits[keyInt].push_back(one);
	traits[keyInt].push_back(two);
	traits[keyInt].push_back(three);
	traits[keyInt].push_back(four);
	traits[keyInt].push_back(five);
	traits[keyInt].push_back(value);
}

// FIXME: next three methods should use some kind of custom container so it doesn't have to search so much.
void memory_info::setClass (const char * name, const char * vtable)
{
    for (uint32_t i=0; i<classes.size(); i++)
    {
        if(classes[i].classname == name)
        {
            classes[i].vtable = strtol(vtable, NULL, 16);
            return;
        }
    }
    
    t_class cls;
        cls.assign = classindex;
        cls.classname = name;
        cls.is_multiclass = false;
        cls.type_offset = 0;
        classindex++;
        cls.vtable = strtol(vtable, NULL, 16);
    classes.push_back(cls);
    //cout << "class " << name << ", assign " << cls.assign << ", vtable  " << cls.vtable << endl;
}


// find old entry by name, rewrite, return its multi index. otherwise make a new one, append an empty vector of t_type to classtypes,  return its index.
uint32_t memory_info::setMultiClass (const char * name, const char * vtable, const char * typeoffset)
{
    for (uint32_t i=0; i<classes.size(); i++)
    {
        if(classes[i].classname == name)
        {
            // vtable and typeoffset can be left out from the xml definition when there's already a named multiclass
            if(vtable != NULL)
                classes[i].vtable = strtol(vtable, NULL, 16);
            if(typeoffset != NULL)
                classes[i].type_offset = strtol(typeoffset, NULL, 16);
            return classes[i].multi_index;
        }
    }
    
    //FIXME: add checking for vtable and typeoffset here. they HAVE to be valid. maybe change the return value into a bool and pass in multi index by reference?
    t_class cls;
        cls.assign = classindex;
        cls.classname = name;
        cls.is_multiclass = true;
        cls.type_offset = strtol(typeoffset, NULL, 16);
        cls.vtable = strtol(vtable, NULL, 16);
        cls.multi_index = classsubtypes.size();
    classes.push_back(cls);
    classindex++;

    vector<t_type> thistypes;
    classsubtypes.push_back(thistypes);
    //cout << "multiclass " << name << ", assign " << cls.assign << ", vtable  " << cls.vtable << endl;
    return classsubtypes.size() - 1;
}


void memory_info::setMultiClassChild (uint32_t multi_index, const char * name, const char * type)
{
    vector <t_type>& vec = classsubtypes[multi_index];
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
        mcc.assign = classindex;
        mcc.classname = name;
        mcc.type = strtol(type, NULL, 16);
    vec.push_back(mcc);
    classindex++;
    //cout << "    classtype " << name << ", assign " << mcc.assign << ", vtable  " << mcc.type << endl;
}


bool memory_info::resolveClassId(uint32_t address, int32_t & classid)
{
    uint32_t vtable = MreadDWord(address);
    // FIXME: stupid search. we need a better container
    for(uint32_t i = 0;i< classes.size();i++)
    {
        if(classes[i].vtable == vtable) // got class
        {
            // if it is a multiclass, try resolving it
            if(classes[i].is_multiclass)
            {
                vector <t_type>& vec = classsubtypes[classes[i].multi_index];
                uint32_t type = MreadWord(address + classes[i].type_offset);
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
            classid = classes[i].assign;
            return true;
        }
    }
    // we failed to find anything that would match
    return false;
}

// Flatten vtables into a index<->name mapping
void memory_info::copyBuildings(vector<string> & v_buildingtypes)
{
    for(uint32_t i = 0;i< classes.size();i++)
    {
        v_buildingtypes.push_back(classes[i].classname);
        if(!classes[i].is_multiclass)
        {
            continue;
        }
        vector <t_type>& vec = classsubtypes[classes[i].multi_index];
        for (uint32_t k = 0; k < vec.size();k++)
        {
            v_buildingtypes.push_back(vec[k].classname);
        }
    }
}


// change base of all addresses/vtable entries
void memory_info::RebaseAddresses(int32_t new_base)
{
    map<string, uint32_t>::iterator iter;
    int32_t rebase = - (int32_t)base + new_base;
    for(iter = addresses.begin(); iter != addresses.end(); iter++)
    {
        addresses[iter->first] = iter->second + rebase;
    }
}


// change base of all addresses/vtable entries
void memory_info::RebaseAll(int32_t new_base)
{
    map<string, uint32_t>::iterator iter;
    int32_t rebase = - (int32_t)base + new_base;
    for(iter = addresses.begin(); iter != addresses.end(); iter++)
    {
        addresses[iter->first] = iter->second + rebase;
    }
    RebaseVTable(rebase);
}


// change all vtable entries by offset
void memory_info::RebaseVTable(int32_t offset)
{
    vector<t_class>::iterator iter;
    for(iter = classes.begin(); iter != classes.end(); iter++)
    {
        iter->vtable += offset;
    }
}


// Get named address
uint32_t memory_info::getAddress (string key)
{
    if(addresses.count(key))
    {
        return addresses[key];
    }
    return 0;
}


// Get named offset
uint32_t memory_info::getOffset (string key)
{
    if(offsets.count(key))
    {
        return offsets[key];
    }
    return 0;
}


// Get named string
std::string memory_info::getString (string key)
{
    if(strings.count(key))
    {
        return strings[key];
    }
    else return string("");
}


// Get named numerical value
uint32_t memory_info::getHexValue (string key)
{
    if(hexvals.count(key))
    {
        return hexvals[key];
    }
    return 0;
}

// Get Profession
string memory_info::getProfession (uint32_t key)
{
    if(professions.size() > key)
    {
        return professions[key];
    }
    else{
        return string("");
    }
}

// Get Job
string memory_info::getJob (uint32_t key)
{
    if(jobs.size() > key){
        return jobs[key];
    }
    return string("Job Does Not Exist");
}

string memory_info::getSkill (uint32_t key)
{
    if(skills.size() > key){
        return skills[key];
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

string memory_info::getTrait (uint32_t traitIdx, uint32_t traitValue)
{
    if(traits.size() > traitIdx)
    {
        int diff = absolute(traitValue-50);
        if(diff < 10)
        {
            return string("");
        }
        if (traitValue >= 91)
            return traits[traitIdx][5];
        else if (traitValue >= 76)
            return traits[traitIdx][4];
        else if (traitValue >= 61)
            return traits[traitIdx][3];
        else if (traitValue >= 25)
            return traits[traitIdx][2];
        else if (traitValue >= 10)
            return traits[traitIdx][1];
        else
            return traits[traitIdx][0];
    }
    return string("Trait is not Defined");
}

string memory_info::getTraitName(uint32_t key)
{
    if(traits.size() > key)
    {
        return traits[key][traits[key].size()-1];
    }
    return string("Trait is not Defined");
}

string memory_info::getLabor (uint32_t key)
{
    if(labors.count(key))
    {
        return labors[key];
    }
    return string("");
}

// Reset everything
void memory_info::flush()
{
    base = 0;
    addresses.clear();
    offsets.clear();
    strings.clear();
    hexvals.clear();
    classes.clear();
    classsubtypes.clear();
    classindex = 0;
    version = "";
    OS = OS_BAD;
}
