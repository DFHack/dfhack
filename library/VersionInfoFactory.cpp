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

#include "VersionInfoFactory.h"
#include "VersionInfo.h"
#include "Error.h"
#include "Memory.h"
#include "MemAccess.h"
#include "PluginManager.h"

#include <tinyxml.h>

using namespace DFHack;
using std::cerr;
using std::endl;
using std::string;
using std::vector;

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
    versions.clear();
    error = false;
}

std::shared_ptr<const VersionInfo> VersionInfoFactory::getVersionInfoByMD5(string hash) const
{
    for (const auto& version : versions)
    {
        if(version->hasMD5(hash))
            return version;
    }
    return nullptr;
}

std::shared_ptr<const VersionInfo> VersionInfoFactory::getVersionInfoByPETimestamp(uintptr_t timestamp) const
{
    for (const auto& version : versions)
    {
        if(version->hasPE(timestamp))
            return version;
    }
    return nullptr;
}

std::vector<std::shared_ptr<const VersionInfo>> VersionInfoFactory::getVersionInfosForCurOs() const {
    static const OSType expected = VersionInfo::getCurOS();
    static const string zeroMd5 = "00000000000000000000000000000000";

    std::vector<std::shared_ptr<const VersionInfo>> ret;

    for (const auto& version : versions) {
        if (version->getOS() == expected
            && version->getVersion().find("LOCAL") == string::npos
            && !version->hasPE(0)
            && !version->hasMD5(zeroMd5))
        {
            ret.emplace_back(version);
        }
    }

    return ret;
}

static uintptr_t to_addr(const char * cstr) {
    if (sizeof(uintptr_t) == sizeof(unsigned long))
        return strtoul(cstr, 0, 0);
    return strtoull(cstr, 0, 0);
}

static uintptr_t get_addr(const char * cstr_value, const char * cstr_base,
    const std::vector<DFHack::t_memrange> & ranges)
{
    uintptr_t addr = to_addr(cstr_value);
    if (cstr_base) {
        string base_name = "/";
        base_name += cstr_base;
        for (auto & range : ranges) {
            string range_name = range.name;
            if (range_name.ends_with(base_name)) {
                addr += (uintptr_t)range.base;
                break;
            }
        }
    }
    return addr;
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
    }
    else if(os == "linux")
    {
        mem->setOS(OS_LINUX);
    }
    else if(os == "darwin")
    {
        mem->setOS(OS_APPLE);
    }
    else
    {
        return; // ignore it if it's invalid
    }
    mem->setBase(DEFAULT_BASE_ADDR);  // Memory.h

    // process additional entries
    //cout << "Entry " << cstr_version << " " <<  cstr_os << endl;
    if (!entry->FirstChildElement()) {
        cerr << "Empty symbol table: " << entry->Attribute("name") << endl;
        return;
    }
    std::vector<DFHack::t_memrange> ranges;
    Core::getInstance().p->getMemRanges(ranges);
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
            const char *cstr_base = pMemEntry->Attribute("base");
            const char *cstr_mangled = pMemEntry->Attribute("mangled");
            if(!cstr_value && !cstr_mangled)
            {
                cerr << "Dummy symbol table entry: " << cstr_key << endl;
                continue;
            }
            if ((is_vtable && no_vtables) || (!is_vtable && no_globals))
                continue;
            uintptr_t addr;
            if (cstr_value) {
                addr = get_addr(cstr_value, cstr_base, ranges);
            } else {
                addr = (uintptr_t)DFHack::LookupPlugin(DFHack::GLOBAL_NAMES, cstr_mangled);
                if (!addr)
                    continue;
                const char *cstr_offset = pMemEntry->Attribute("offset");
                if (cstr_offset) {
                    unsigned long offset = strtoul(cstr_offset, 0, 0);
                    addr += offset;
                }
            }
            if (is_vtable)
                mem->setVTable(cstr_key, addr);
            else
                mem->setAddress(cstr_key, addr);
        }
        else if (type == "md5-hash")
        {
            const char *cstr_value = pMemEntry->Attribute("value");
            fprintf(stderr, "%s (%s): MD5: %s\n", cstr_name, cstr_os, cstr_value ? cstr_value : "NULL");
            if(!cstr_value)
                throw Error::SymbolsXmlUnderspecifiedEntry(cstr_name);
            mem->addMD5(cstr_value);
        }
        else if (type == "binary-timestamp")
        {
            const char *cstr_value = pMemEntry->Attribute("value");
            fprintf(stderr, "%s (%s): PE: %s\n", cstr_name, cstr_os, cstr_value ? cstr_value : "NULL");
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
                auto version = std::make_shared<VersionInfo>();
                ParseVersion( pMemInfo , version.get() );
                versions.push_back(version);
            }
        }
    }
    error = false;
    std::cerr << "Loaded " << versions.size() << " DF symbol tables." << std::endl;
    return true;
}
