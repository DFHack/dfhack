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
    OffsetGroup * OG = offset_descriptor->getGroup("name");
    name_firstname_offset = OG->getOffset("first");
    name_nickname_offset = OG->getOffset("nick");
    name_words_offset = OG->getOffset("second_words");
    return true;
}

void DFContextShared::readName(t_name & name, uint32_t address)
{
    p->readSTLString(address + name_firstname_offset , name.first_name, 128);
    p->readSTLString(address + name_nickname_offset , name.nickname, 128);
    p->read(address + name_words_offset ,28, (uint8_t *) name.words);
}
