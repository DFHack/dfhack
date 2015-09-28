/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

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
#include <iostream>
using namespace std;

#include "VersionInfoFactory.h"
#include "VersionInfo.h"
#include "Error.h"
using namespace DFHack;

#include <tinyxml.h>

VersionInfoFactory::VersionInfoFactory()
{
    error = false;
}

VersionInfoFactory::~VersionInfoFactory()
{
    clear();
}

void VersionInfoFactory::clear()
{
    // for each stored version, delete
    for(size_t i = 0; i < versions.size();i++)
    {
        delete versions[i];
    }
    versions.clear();
    error = false;
}

VersionInfo * VersionInfoFactory::getVersionInfoByMD5(string hash)
{
    for(size_t i = 0; i < versions.size();i++)
    {
        if(versions[i]->hasMD5(hash))
            return versions[i];
    }
    return 0;
}

VersionInfo * VersionInfoFactory::getVersionInfoByPETimestamp(uint32_t timestamp)
{
    for(size_t i = 0; i < versions.size();i++)
    {
        if(versions[i]->hasPE(timestamp))
            return versions[i];
    }
    return 0;
}

void VersionInfoFactory::ParseVersion (TiXmlElement* entry, VersionInfo* mem)
{
    bool no_vtables = getenv("DFHACK_NO_VTABLES");
    bool no_globals = getenv("DFHACK_NO_GLOBALS");
    TiXmlElement* pMemEntry;
    const char *cstr_name = entry->Attribute("name");
    if (!cstr_name)
        throw Error::SymbolsXmlBadAttribute("name");

    const char *cstr_os = entry->Attribute("os-type");
    if (!cstr_os)
        throw Error::SymbolsXmlBadAttribute("os-type");

    string os = cstr_os;
    mem->setVersion(cstr_name);

    if(os == "windows")
    {
        mem->setOS(OS_WINDOWS);
        // set default image base. this is fixed for base relocation later
        mem->setBase(0x400000);
    }
    else if(os == "linux")
    {
        mem->setOS(OS_LINUX);
        // this is wrong... I'm not going to do base image relocation on linux though.
        mem->setBase(0x8048000);
    }
    else if(os == "darwin")
    {
        mem->setOS(OS_APPLE);
        // this is wrong... I'm not going to do base image relocation on linux though.
        mem->setBase(0x1000);
    }
    else
    {
        return; // ignore it if it's invalid
    }

    // process additional entries
    //cout << "Entry " << cstr_version << " " <<  cstr_os << endl;
    pMemEntry = entry->FirstChildElement()->ToElement();
    for(;pMemEntry;pMemEntry=pMemEntry->NextSiblingElement())
    {
        string type, name, value;
        const char *cstr_type = pMemEntry->Value();
        type = cstr_type;
        bool is_vtable = (type == "vtable-address");
        if(is_vtable || type == "global-address")
        {
            const char *cstr_key = pMemEntry->Attribute("name");
            if(!cstr_key)
                throw Error::SymbolsXmlUnderspecifiedEntry(cstr_name);
            const char *cstr_value = pMemEntry->Attribute("value");
            if(!cstr_value)
            {
                cerr << "Dummy symbol table entry: " << cstr_key << endl;
                continue;
            }
            if ((is_vtable && no_vtables) || (!is_vtable && no_globals))
                continue;
            uint32_t addr = strtol(cstr_value, 0, 0);
            if (is_vtable)
                mem->setVTable(cstr_key, addr);
            else
                mem->setAddress(cstr_key, addr);
        }
        else if (type == "md5-hash")
        {
            const char *cstr_value = pMemEntry->Attribute("value");
            fprintf(stderr, "%s: MD5: %s\n", cstr_name, cstr_value);
            if(!cstr_value)
                throw Error::SymbolsXmlUnderspecifiedEntry(cstr_name);
            mem->addMD5(cstr_value);
        }
        else if (type == "binary-timestamp")
        {
            const char *cstr_value = pMemEntry->Attribute("value");
            fprintf(stderr, "%s: PE: %s\n", cstr_name, cstr_value);
            if(!cstr_value)
                throw Error::SymbolsXmlUnderspecifiedEntry(cstr_name);
            mem->addPE(strtol(cstr_value, 0, 16));
        }
    } // for
} // method

// load the XML file with offsets
bool VersionInfoFactory::loadFile(string path_to_xml)
{
    TiXmlDocument doc( path_to_xml.c_str() );
    std::cerr << "Loading " << path_to_xml << " ... ";
    //bool loadOkay = doc.LoadFile();
    if (!doc.LoadFile())
    {
        error = true;
        cerr << "failed!\n";
        throw Error::SymbolsXmlParse(doc.ErrorDesc(), doc.ErrorId(), doc.ErrorRow(), doc.ErrorCol());
    }
    else
    {
        cerr << "OK\n";
    }
    TiXmlHandle hDoc(&doc);
    TiXmlElement* pElem;
    TiXmlHandle hRoot(0);

    // block: name
    {
        pElem=hDoc.FirstChildElement().Element();
        // should always have a valid root but handle gracefully if it does
        if (!pElem)
        {
            error = true;
            throw Error::SymbolsXmlNoRoot();
        }
        string m_name=pElem->Value();
        if(m_name != "data-definition")
        {
            error = true;
            throw Error::SymbolsXmlNoRoot();
        }
        // save this for later
        hRoot=TiXmlHandle(pElem);
    }
    // transform elements
    {
        clear();
        // For each version
        TiXmlElement * pMemInfo=hRoot.FirstChild( "symbol-table" ).Element();
        for( ; pMemInfo; pMemInfo=pMemInfo->NextSiblingElement("symbol-table"))
        {
            const char *name = pMemInfo->Attribute("name");
            if(name)
            {
                VersionInfo *version = new VersionInfo();
                ParseVersion( pMemInfo , version );
                versions.push_back(version);
            }
        }
    }
    error = false;
    std::cerr << "Loaded " << versions.size() << " DF symbol tables." << std::endl;
    return true;
}
