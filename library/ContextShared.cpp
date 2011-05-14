#include "Internal.h"

#include <string>
#include <vector>
#include <map>
#include <cstring>
using namespace std;

#include "private/ContextShared.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/DFProcess.h"
#include "dfhack/DFModule.h"
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
    catch(exception & e)
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
