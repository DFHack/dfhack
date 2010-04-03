#include "DFCommonInternal.h"
#include <shms.h>
#include <mod-core.h>
#include <mod-maps.h>
#include <mod-creature40d.h>
using namespace DFHack;

#include "private/APIPrivate.h"

APIPrivate::APIPrivate()
{
}

bool APIPrivate::InitReadNames()
{
    try
    {
        name_firstname_offset = offset_descriptor->getOffset("name_firstname");
        name_nickname_offset = offset_descriptor->getOffset("name_nickname");
        name_words_offset = offset_descriptor->getOffset("name_words");
    }
    catch(Error::MissingMemoryDefinition)
    {
        return false;
    }
    return true;
}


void APIPrivate::readName(t_name & name, uint32_t address)
{
    g_pProcess->readSTLString(address + name_firstname_offset , name.first_name, 128);
    g_pProcess->readSTLString(address + name_nickname_offset , name.nickname, 128);
    g_pProcess->read(address + name_words_offset ,48, (uint8_t *) name.words);
}