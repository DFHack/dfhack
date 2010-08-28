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

#include "Internal.h"

#include "dfhack/VersionInfoFactory.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/DFError.h"
#include <algorithm>

using namespace DFHack;

VersionInfoFactory::~VersionInfoFactory()
{
    // for each stored version, delete
    for(uint32_t i = 0; i < versions.size();i++)
    {
        delete versions[i];
    }
    versions.clear();
}

void VersionInfoFactory::ParseVTable(TiXmlElement* vtable, VersionInfo* mem)
{
    TiXmlElement* pClassEntry;
    TiXmlElement* pClassSubEntry;
    /*
    // check for rebase, do rebase if check positive
    const char * rebase = vtable->Attribute("rebase");
    if(rebase)
    {
        int32_t rebase_offset = strtol(rebase, NULL, 16);
        mem->RebaseVTable(rebase_offset);
    }
    */
    // parse vtable entries
    pClassEntry = vtable->FirstChildElement();
    for(;pClassEntry;pClassEntry=pClassEntry->NextSiblingElement())
    {
        string type = pClassEntry->Value();
        const char *cstr_name = pClassEntry->Attribute("name");
        const char *cstr_vtable = pClassEntry->Attribute("vtable");
        uint32_t vtable = 0;
        if(cstr_vtable)
            vtable = strtol(cstr_vtable, NULL, 16);
        // it's a simple class
        if(type== "class")
        {
            mem->setClass(cstr_name, vtable);
        }
        // it's a multi-type class
        else if (type == "multiclass")
        {
            // get offset of the type variable
            const char *cstr_typeoffset = pClassEntry->Attribute("typeoffset");
            uint32_t typeoffset = 0;
            if(cstr_typeoffset)
                typeoffset = strtol(cstr_typeoffset, NULL, 16);
            t_class * mclass = mem->setClass(cstr_name, vtable, typeoffset);
            // parse class sub-entries
            pClassSubEntry = pClassEntry->FirstChildElement();
            for(;pClassSubEntry;pClassSubEntry=pClassSubEntry->NextSiblingElement())
            {
                type = pClassSubEntry->Value();
                if(type== "class")
                {
                    // type is a value loaded from type offset
                    cstr_name = pClassSubEntry->Attribute("name");
                    const char *cstr_value = pClassSubEntry->Attribute("type");
                    mem->setClassChild(mclass,cstr_name,cstr_value);
                }
            }
        }
    }
}

// FIXME: this is ripe for replacement with a more generic approach
void VersionInfoFactory::ParseOffsets(TiXmlElement * parent, VersionInfo* target, bool initial)
{
    // we parse the groups iteratively instead of recursively
    // breadcrubs acts like a makeshift stack
    // first pair entry stores the current element of that level
    // second pair entry the group object from OffsetGroup
    typedef pair < TiXmlElement *, OffsetGroup * > groupPair;
    vector< groupPair > breadcrumbs;
    {
        TiXmlElement* pEntry;
        // we get the <Offsets>, look at the children
        pEntry = parent->FirstChildElement();
        if(!pEntry)
            return;

        OffsetGroup * currentGroup = reinterpret_cast<OffsetGroup *> (target);
        breadcrumbs.push_back(groupPair(pEntry,currentGroup));
    }

    // work variables
    OffsetGroup * currentGroup = 0;
    TiXmlElement * currentElem = 0;
    cerr << "<Offsets>"<< endl;
    while(1)
    {
        // get current work variables
        currentElem = breadcrumbs.back().first;
        currentGroup = breadcrumbs.back().second;

        // we reached the end of the current group?
        if(!currentElem)
        {
            // go one level up
            breadcrumbs.pop_back();
            // exit if no more work
            if(breadcrumbs.empty())
            {
                break;
            }
            else
            {
                cerr << "</group>" << endl;
                continue;
            }
        }

        if(!currentGroup)
        {
            groupPair & gp = breadcrumbs.back();
            gp.first = gp.first->NextSiblingElement();
            continue;
        }

        // skip non-elements
        if (currentElem->Type() != TiXmlNode::ELEMENT)
        {
            groupPair & gp = breadcrumbs.back();
            gp.first = gp.first->NextSiblingElement();
            continue;
        }

        // we have a valid current element and current group
        // get properties
        string type = currentElem->Value();
        std::transform(type.begin(), type.end(), type.begin(), ::tolower);
        const char *cstr_name = currentElem->Attribute("name");
        if(!cstr_name)
        {
            // ERROR, missing attribute
        }

        // evaluate elements
        const char *cstr_value = currentElem->Attribute("value");
        if(type == "group")
        {
            // FIXME: possibly use setGroup always, with the initial flag as parameter?
            // create or get group
            OffsetGroup * og;
            if(initial)
                og = currentGroup->createGroup(cstr_name);
            else
                og = currentGroup->getGroup(cstr_name);
            cerr << "<group name=\"" << cstr_name << "\">" << endl;
            // advance this level to the next element
            groupPair & gp = breadcrumbs.back();
            gp.first = gp.first->NextSiblingElement();

            // add a new level that will be processed next
            breadcrumbs.push_back(groupPair(currentElem->FirstChildElement(), og));
            continue;
        }
        else if(type == "address")
        {
            if(initial)
            {
                currentGroup->createAddress(cstr_name);
            }
            else if(cstr_value)
            {
                currentGroup->setAddress(cstr_name, cstr_value);
            }
            else
            {
                // ERROR, missing attribute
            }
        }
        else if(type == "offset")
        {
            if(initial)
            {
                currentGroup->createOffset(cstr_name);
            }
            else if(cstr_value)
            {
                currentGroup->setOffset(cstr_name, cstr_value);
            }
            else
            {
                // ERROR, missing attribute
            }
        }
        else if(type == "string")
        {
            if(initial)
            {
                currentGroup->createString(cstr_name);
            }
            else if(cstr_value)
            {
                currentGroup->setString(cstr_name, cstr_value);
            }
            else
            {
                // ERROR, missing attribute
            }
        }
        else if(type == "hexvalue")
        {
            if(initial)
            {
                currentGroup->createHexValue(cstr_name);
            }
            else if(cstr_value)
            {
                currentGroup->setHexValue(cstr_name, cstr_value);
            }
            else
            {
                // ERROR, missing attribute
            }
        }

        // advance to next element
        groupPair & gp = breadcrumbs.back();
        gp.first = gp.first->NextSiblingElement();
        continue;
    }
    cerr << "</Offsets>"<< endl;
}

void VersionInfoFactory::ParseBase (TiXmlElement* entry, VersionInfo* mem)
{
    TiXmlElement* pElement;
    TiXmlElement* pElement2nd;
    const char *cstr_version = entry->Attribute("name");

    if (!cstr_version)
        throw Error::MemoryXmlBadAttribute("name");

    mem->setVersion(cstr_version);
    mem->setOS(VersionInfo::OS_BAD);

    // process additional entries
    pElement = entry->FirstChildElement()->ToElement();
    for(;pElement;pElement=pElement->NextSiblingElement())
    {
        // only elements get processed
        const char *cstr_type = pElement->Value();
        std::string type = cstr_type;
        if(type == "VTable")
        {
            ParseVTable(pElement, mem);
            continue;
        }
        else if(type == "Offsets")
        {
            // we don't care about the descriptions here, do nothing
            ParseOffsets(pElement, mem, true);
            continue;
        }
        else if (type == "Professions")
        {
            pElement2nd = entry->FirstChildElement("Profession");
            for(;pElement2nd;pElement2nd=pElement2nd->NextSiblingElement("Profession"))
            {
                const char * id = pElement2nd->Attribute("id");
                const char * name = pElement2nd->Attribute("name");
                // FIXME: missing some attributes here
                if(id && name)
                {
                    mem->setProfession(id,name);
                }
                else
                {
                    // FIXME: this is crap, doesn't tell anything about the error
                    throw Error::MemoryXmlUnderspecifiedEntry(name);
                }
            }
        }
        else if (type == "Jobs")
        {
            pElement2nd = entry->FirstChildElement("Job");
            for(;pElement2nd;pElement2nd=pElement2nd->NextSiblingElement("Job"))
            {
                const char * id = pElement2nd->Attribute("id");
                const char * name = pElement2nd->Attribute("name");
                if(id && name)
                {
                    mem->setJob(id,name);
                }
                else
                {
                    // FIXME: this is crap, doesn't tell anything about the error
                    throw Error::MemoryXmlUnderspecifiedEntry(name);
                }
            }
        }
        else if (type == "Skills")
        {
            pElement2nd = entry->FirstChildElement("Skill");
            for(;pElement2nd;pElement2nd=pElement2nd->NextSiblingElement("Skill"))
            {
                const char * id = pElement2nd->Attribute("id");
                const char * name = pElement2nd->Attribute("name");
                if(id && name)
                {
                    mem->setSkill(id,name);
                }
                else
                {
                    // FIXME: this is crap, doesn't tell anything about the error
                    throw Error::MemoryXmlUnderspecifiedEntry(name);
                }
            }
        }
        else if (type == "Traits")
        {
            pElement2nd = entry->FirstChildElement("Trait");
            for(;pElement2nd;pElement2nd=pElement2nd->NextSiblingElement("Trait"))
            {
                const char * id = pElement2nd->Attribute("id");
                const char * name = pElement2nd->Attribute("name");
                const char * lvl0 = pElement->Attribute("level_0");
                const char * lvl1 = pElement->Attribute("level_1");
                const char * lvl2 = pElement->Attribute("level_2");
                const char * lvl3 = pElement->Attribute("level_3");
                const char * lvl4 = pElement->Attribute("level_4");
                const char * lvl5 = pElement->Attribute("level_5");
                if(id && name && lvl0 && lvl1 && lvl2 && lvl3 && lvl4 && lvl5)
                {
                    mem->setTrait(id, name, lvl0, lvl1, lvl2, lvl3, lvl4, lvl5);
                }
                else
                {
                    // FIXME: this is crap, doesn't tell anything about the error
                    throw Error::MemoryXmlUnderspecifiedEntry(name);
                }
            }
        }
        else if (type == "Labors")
        {
            pElement2nd = entry->FirstChildElement("Labor");
            for(;pElement2nd;pElement2nd=pElement2nd->NextSiblingElement("Labor"))
            {
                const char * id = pElement2nd->Attribute("id");
                const char * name = pElement2nd->Attribute("name");
                if(id && name)
                {
                    mem->setLabor(id,name);
                }
                else
                {
                    // FIXME: this is crap, doesn't tell anything about the error
                    throw Error::MemoryXmlUnderspecifiedEntry(name);
                }
            }
        }
        else if (type == "Levels")
        {
            pElement2nd = entry->FirstChildElement("Level");
            for(;pElement2nd;pElement2nd=pElement2nd->NextSiblingElement("Level"))
            {
                const char * id = pElement2nd->Attribute("id");
                const char * name = pElement2nd->Attribute("name");
                const char * nextlvl = pElement2nd->Attribute("xpNxtLvl");
                if(id && name && nextlvl)
                {
                    mem->setLevel(id, name, nextlvl);
                }
                else
                {
                    // FIXME: this is crap, doesn't tell anything about the error
                    throw Error::MemoryXmlUnderspecifiedEntry(name);
                }
            }
        }
        else if (type == "Moods")
        {
            pElement2nd = entry->FirstChildElement("Mood");
            for(;pElement2nd;pElement2nd=pElement2nd->NextSiblingElement("Mood"))
            {
                const char * id = pElement2nd->Attribute("id");
                const char * name = pElement2nd->Attribute("name");
                if(id && name)
                {
                    mem->setMood(id, name);
                }
                else
                {
                    // FIXME: this is crap, doesn't tell anything about the error
                    throw Error::MemoryXmlUnderspecifiedEntry(name);
                }
            }
        }
        else
        {
            //FIXME: only log, not hard error
            //throw Error::MemoryXmlUnknownType(type.c_str());
        }
    } // for
} // method

void VersionInfoFactory::EvalVersion(string base, VersionInfo * mem)
{
    if(knownVersions.find(base) != knownVersions.end())
    {
        v_descr & desc = knownVersions[base];
        if (!desc.second)
        {
            VersionInfo * newmem = new VersionInfo();
            ParseVersion(desc.first, newmem);
            desc.second = newmem;
        }
        mem->copy(desc.second);
    }
}

void VersionInfoFactory::ParseVersion (TiXmlElement* entry, VersionInfo* mem)
{
    TiXmlElement* pMemEntry;
    const char *cstr_name = entry->Attribute("name");
    const char *cstr_os = entry->Attribute("os");
    const char *cstr_base = entry->Attribute("base");
    const char *cstr_rebase = entry->Attribute("rebase");
    if(cstr_base)
    {
        string base = cstr_base;
        EvalVersion(base, mem);
    }

    if (!cstr_name)
        throw Error::MemoryXmlBadAttribute("name");
    if (!cstr_os)
        throw Error::MemoryXmlBadAttribute("os");

    string os = cstr_os;
    mem->setVersion(cstr_name);
    mem->setOS(cstr_os);

    // offset inherited addresses by 'rebase'.
    int32_t rebase = 0;
    if(cstr_rebase)
    {
        rebase = mem->getBase() + strtol(cstr_rebase, NULL, 16);
        mem->RebaseAddresses(rebase);
    }

    //set base to default, we're overwriting this because the previous rebase could cause havoc on Vista/7
    if(os == "windows")
    {
        // set default image base. this is fixed for base relocation later
        mem->setBase(0x400000);
    }
    else if(os == "linux")
    {
        // this is wrong... I'm not going to do base image relocation on linux though.
        // users are free to use a sane kernel that doesn't do this kind of **** by default
        mem->setBase(0x0);
    }
    else
    {
        throw Error::MemoryXmlBadAttribute("os");
    }

    // process additional entries
    //cout << "Entry " << cstr_version << " " <<  cstr_os << endl;
    pMemEntry = entry->FirstChildElement()->ToElement();
    for(;pMemEntry;pMemEntry=pMemEntry->NextSiblingElement())
    {
        string type, name, value;
        const char *cstr_type = pMemEntry->Value();
        type = cstr_type;
        // check for missing parts
        if(type == "VTable")
        {
            ParseVTable(pMemEntry, mem);
            continue;
        }
        else if(type == "Offsets")
        {
            ParseOffsets(pMemEntry, mem);
            continue;
        }
        else if (type == "MD5")
        {
            const char *cstr_value = pMemEntry->Attribute("value");
            if(!cstr_value)
                throw Error::MemoryXmlUnderspecifiedEntry(cstr_name);
            mem->setMD5(cstr_value);
        }
        else if (type == "PETimeStamp")
        {
            const char *cstr_value = pMemEntry->Attribute("value");
            if(!cstr_value)
                throw Error::MemoryXmlUnderspecifiedEntry(cstr_name);
            mem->setPE(strtol(cstr_value, 0, 16));
        }
    } // for
} // method

VersionInfoFactory::VersionInfoFactory(string path_to_xml)
{
    error = false;
    loadFile(path_to_xml);
}

// load the XML file with offsets
bool VersionInfoFactory::loadFile(string path_to_xml)
{
    TiXmlDocument doc( path_to_xml.c_str() );
    //bool loadOkay = doc.LoadFile();
    if (!doc.LoadFile())
    {
        error = true;
        throw Error::MemoryXmlParse(doc.ErrorDesc(), doc.ErrorId(), doc.ErrorRow(), doc.ErrorCol());
    }
    TiXmlHandle hDoc(&doc);
    TiXmlElement* pElem;
    TiXmlHandle hRoot(0);
    VersionInfo *mem;

    // block: name
    {
        pElem=hDoc.FirstChildElement().Element();
        // should always have a valid root but handle gracefully if it does
        if (!pElem)
        {
            error = true;
            throw Error::MemoryXmlNoRoot();
        }
        string m_name=pElem->Value();
        if(m_name != "DFHack")
        {
            error = true;
            throw Error::MemoryXmlNoRoot();
        }
        // save this for later
        hRoot=TiXmlHandle(pElem);
    }
    // transform elements
    {
        // trash existing list
        for(uint32_t i = 0; i < versions.size(); i++)
        {
            delete versions[i];
        }
        versions.clear();

        // For each base version
        TiXmlElement* pMemInfo=hRoot.FirstChild( "Base" ).Element();
        map <string ,TiXmlElement *> map_pNamedEntries;
        vector <string> v_sEntries;
        for( ; pMemInfo; pMemInfo=pMemInfo->NextSiblingElement("Base"))
        {
            const char *name = pMemInfo->Attribute("name");
            if(name)
            {
                string str_name = name;
                VersionInfo *base = new VersionInfo();
                mem = new VersionInfo();
                ParseBase( pMemInfo , mem );
                knownVersions[str_name] = v_descr (pMemInfo, mem);
            }
        }

        // For each derivative version
        pMemInfo=hRoot.FirstChild( "Version" ).Element();
        for( ; pMemInfo; pMemInfo=pMemInfo->NextSiblingElement("Version"))
        {
            const char *name = pMemInfo->Attribute("name");
            if(name)
            {
                string str_name = name;
                knownVersions[str_name] = v_descr (pMemInfo, NULL);
                v_sEntries.push_back(str_name);
            }
        }
        // Parse the versions
        for(uint32_t i = 0; i< v_sEntries.size();i++)
        {
            //FIXME: add a set of entries processed in a step of this cycle, use it to check for infinite loops
            string & name = v_sEntries[i];
            v_descr & desc = knownVersions[name];
            if(!desc.second)
            {
                VersionInfo *version = new VersionInfo();
                ParseVersion( desc.first , version );
                versions.push_back(version);
            }
        }

        // process found things here
    }
    error = false;
    return true;
}
