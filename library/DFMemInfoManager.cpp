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
#include "DFMemInfoManager.h"

#include "dfhack/DFMemInfo.h"
#include "dfhack/DFError.h"

using namespace DFHack;

MemInfoManager::~MemInfoManager()
{
    // for each in std::vector<memory_info*> meminfo;, delete
    for(uint32_t i = 0; i < meminfo.size();i++)
    {
        delete meminfo[i];
    }
    meminfo.clear();
}

void MemInfoManager::ParseVTable(TiXmlElement* vtable, memory_info* mem)
{
    TiXmlElement* pClassEntry;
    TiXmlElement* pClassSubEntry;
    // check for rebase, do rebase if check positive
    const char * rebase = vtable->Attribute("rebase");
    if(rebase)
    {
        int32_t rebase_offset = strtol(rebase, NULL, 16);
        mem->RebaseVTable(rebase_offset);
    }
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



void MemInfoManager::ParseEntry (TiXmlElement* entry, memory_info* mem, map <string ,TiXmlElement *>& knownEntries)
{
    TiXmlElement* pMemEntry;
    const char *cstr_version = entry->Attribute("version");
    const char *cstr_os = entry->Attribute("os");
    const char *cstr_base = entry->Attribute("base");
    const char *cstr_rebase = entry->Attribute("rebase");
    if(cstr_base)
    {
        string base = cstr_base;
        ParseEntry(knownEntries[base], mem, knownEntries);
    }

    if (!cstr_version)
        throw Error::MemoryXmlBadAttribute("version");
    if (!cstr_os)
        throw Error::MemoryXmlBadAttribute("os");
    
    string os = cstr_os;
    mem->setVersion(cstr_version);
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
    else if ( os == "all")
    {
        // yay
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
        // only elements get processed
        const char *cstr_type = pMemEntry->Value();
        const char *cstr_name = pMemEntry->Attribute("name");		
        const char *cstr_value = pMemEntry->GetText();
		
		if(!cstr_value)
			cstr_value = pMemEntry->Attribute("id");

        // check for missing parts
        string type, name, value;
        type = cstr_type;
        if(type == "VTable")
        {
            ParseVTable(pMemEntry, mem);
            continue;
        }
        if(!(cstr_name && cstr_value))
        {
            throw Error::MemoryXmlUnderspecifiedEntry(cstr_version);
        }
        name = cstr_name;
        value = cstr_value;
        if (type == "HexValue")
        {
            mem->setHexValue(name, value);
        }
        else if (type == "Address")
        {
            mem->setAddress(name, value);
        }
        else if (type == "Offset")
        {
            mem->setOffset(name, value);
        }
        else if (type == "String")
        {
            mem->setString(name, value);
        }
        else if (type == "Profession")
        {
            mem->setProfession(value,name);
        }
        else if (type == "Job")
        {
            mem->setJob(value,name);
        }
        else if (type == "Skill")
        {
            mem->setSkill(value,name);
        }
        else if (type == "Trait")
        {
            mem->setTrait(value, name,pMemEntry->Attribute("level_0"),pMemEntry->Attribute("level_1"),pMemEntry->Attribute("level_2"),pMemEntry->Attribute("level_3"),pMemEntry->Attribute("level_4"),pMemEntry->Attribute("level_5"));
        }
        else if (type == "Labor")
        {
            mem->setLabor(value,name);
        }
		else if (type == "Level")
        {
			mem->setLevel(value, name, pMemEntry->Attribute("min_xp"), pMemEntry->Attribute("max_xp"));
        }
        else
        {
            throw Error::MemoryXmlUnknownType(type.c_str());
        }
    } // for
} // method

MemInfoManager::MemInfoManager(string path_to_xml)
{
    error = false;
    loadFile(path_to_xml);
}

// load the XML file with offsets
bool MemInfoManager::loadFile(string path_to_xml)
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
    memory_info mem;

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
        if(m_name != "DFExtractor")
        {
            error = true;
            throw Error::MemoryXmlNoDFExtractor(m_name.c_str());
        }
        // save this for later
        hRoot=TiXmlHandle(pElem);
    }
    // transform elements
    {
        // trash existing list
        for(uint32_t i = 0; i < meminfo.size(); i++)
        {
            delete meminfo[i];
        }
        meminfo.clear();
        TiXmlElement* pMemInfo=hRoot.FirstChild( "MemoryDescriptors" ).FirstChild( "Entry" ).Element();
        map <string ,TiXmlElement *> map_pNamedEntries;
        vector <TiXmlElement *> v_pEntries;
        for( ; pMemInfo; pMemInfo=pMemInfo->NextSiblingElement("Entry"))
        {
            v_pEntries.push_back(pMemInfo);
            const char *id = pMemInfo->Attribute("id");
            if(id)
            {
                string str_id = id;
                map_pNamedEntries[str_id] = pMemInfo;
            }
        }
        for(uint32_t i = 0; i< v_pEntries.size();i++)
        {
            memory_info *mem = new memory_info();
            //FIXME: add a set of entries processed in a step of this cycle, use it to check for infinite loops
            /* recursive */ParseEntry( v_pEntries[i] , mem , map_pNamedEntries);
            meminfo.push_back(mem);
        }
        // process found things here
    }
    error = false;
    return true;
}
