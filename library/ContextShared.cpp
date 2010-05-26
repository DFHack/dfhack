#include "Internal.h"
#include <shms.h>
#include <mod-core.h>
#include <mod-maps.h>
#include <mod-creature40d.h>
#include "private/ContextShared.h"
#include "dfhack/DFMemInfo.h"
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

using namespace DFHack;

DFContextShared::DFContextShared()
{
    // init modules
    creatures = 0;
    maps = 0;
    position = 0;
    gui = 0;
    world = 0;
    materials = 0;
    translation = 0;
    vegetation = 0;
    buildings = 0;
    constructions = 0;
    items = 0;
    windowio = 0;
}

DFContextShared::~DFContextShared()
{
    if(creatures) delete creatures;
    if(maps) delete maps;
    if(position) delete position;
    if(gui) delete gui;
    if(materials) delete materials;
    if(translation) delete translation;
    if(vegetation) delete vegetation;
    if(buildings) delete buildings;
    if(constructions) delete constructions;
    if(world) delete world;
    if(windowio) delete windowio;
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
