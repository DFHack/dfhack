#include "Internal.h"
#include <shms.h>
#include <mod-core.h>
#include <mod-maps.h>
#include <mod-creature40d.h>
#include "private/ContextShared.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/DFProcess.h"

#include "dfhack/modules/Materials.h"
#include "dfhack/modules/Creatures.h"
#include "dfhack/modules/Maps.h"
#include "dfhack/modules/Position.h"
#include "dfhack/modules/Translation.h"
#include "dfhack/modules/Vegetation.h"
#include "dfhack/modules/Gui.h"
#include "dfhack/modules/World.h"
#include "dfhack/modules/Buildings.h"
#include "dfhack/modules/Constructions.h"
#include "dfhack/modules/WindowIO.h"

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
    for(int i = 0 ; i < allModules.size(); i++)
    {
        delete allModules[i];
    }
    allModules.clear();
}

bool DFContextShared::InitReadNames()
{
    name_firstname_offset = offset_descriptor->getOffset("name_firstname");
    name_nickname_offset = offset_descriptor->getOffset("name_nickname");
    name_words_offset = offset_descriptor->getOffset("name_words");
    return true;
}

void DFContextShared::readName(t_name & name, uint32_t address)
{
    p->readSTLString(address + name_firstname_offset , name.first_name, 128);
    p->readSTLString(address + name_nickname_offset , name.nickname, 128);
    p->read(address + name_words_offset ,48, (uint8_t *) name.words);
}
