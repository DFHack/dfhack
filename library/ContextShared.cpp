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
#include <map>
#include <cstring>
using namespace std;

#include "private/ContextShared.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/Process.h"
#include "dfhack/Module.h"
using namespace DFHack;

DFContextShared::DFContextShared()
{
    // init modules
    allModules.clear();
    memset(&(s_mods), 0, sizeof(s_mods));
    namesInited = false;
    namesFailed = false;
}

DFContextShared::~DFContextShared()
{
    // invalidate all modules
    for(unsigned int i = 0 ; i < allModules.size(); i++)
    {
        delete allModules[i];
    }
    allModules.clear();
}

bool DFContextShared::InitReadNames()
{
    try
    {
        OffsetGroup * OG = offset_descriptor->getGroup("name");
        name_firstname_offset = OG->getOffset("first");
        name_nickname_offset = OG->getOffset("nick");
        name_words_offset = OG->getOffset("second_words");
        name_parts_offset = OG->getOffset("parts_of_speech");
        name_language_offset = OG->getOffset("language");
        name_set_offset = OG->getOffset("has_name");
    }
    catch(exception &)
    {
        namesFailed = true;
        return false;
    }
    namesInited = true;
    return true;
}

void DFContextShared::readName(t_name & name, uint32_t address)
{
    if(namesFailed)
    {
        return;
    }
    if(!namesInited)
    {
        if(!InitReadNames()) return;
    }
    p->readSTLString(address + name_firstname_offset , name.first_name, 128);
    p->readSTLString(address + name_nickname_offset , name.nickname, 128);
    p->read(address + name_words_offset, 7*4, (uint8_t *)name.words);
    p->read(address + name_parts_offset, 7*2, (uint8_t *)name.parts_of_speech);
    name.language = p->readDWord(address + name_language_offset);
    name.has_name = p->readByte(address + name_set_offset);
}

void DFContextShared::copyName(uint32_t address, uint32_t target)
{
    uint8_t buf[28];

    if (address == target)
        return;

    p->copySTLString(address + name_firstname_offset, target + name_firstname_offset);
    p->copySTLString(address + name_nickname_offset, target + name_nickname_offset);
    p->read(address + name_words_offset, 7*4, buf);
    p->write(target + name_words_offset, 7*4, buf);
    p->read(address + name_parts_offset, 7*2, buf);
    p->write(target + name_parts_offset, 7*2, buf);
    p->writeDWord(target + name_language_offset, p->readDWord(address + name_language_offset));
    p->writeByte(target + name_set_offset, p->readByte(address + name_set_offset));
}
