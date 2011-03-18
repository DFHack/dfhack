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

/*
FIXME: Japa said that he had to do this with the time stuff he got from here
       Investigate.

    currentYear = Wold->ReadCurrentYear();
    currentTick = Wold->ReadCurrentTick();
    currentMonth = (currentTick+9)/33600;
    currentDay = ((currentTick+9)%33600)/1200;
    currentHour = ((currentTick+9)-(((currentMonth*28)+currentDay)*1200))/50;
    currentTickRel = (currentTick+9)-(((((currentMonth*28)+currentDay)*24)+currentHour)*50);
    */

#include "Internal.h"
#include "ContextShared.h"
#include "dfhack/modules/World.h"
#include "dfhack/DFProcess.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/DFTypes.h"
#include "dfhack/DFError.h"
#include "ModuleFactory.h"

using namespace DFHack;

Module* DFHack::createWorld(DFContextShared * d)
{
    return new World(d);
}

struct World::Private
{
    bool Inited;
    bool StartedTime;
    bool StartedWeather;
    bool StartedMode;
    uint32_t year_offset;
    uint32_t tick_offset;
    uint32_t weather_offset;
    uint32_t gamemode_offset;
    uint32_t controlmode_offset;
    uint32_t controlmodecopy_offset;
    DFContextShared *d;
    Process * owner;
};

World::World(DFContextShared * _d)
{

    d = new Private;
    d->d = _d;
    d->owner = _d->p;
    d->Inited = d->StartedTime = d->StartedWeather = d->StartedMode = false;

    OffsetGroup * OG_World = d->d->offset_descriptor->getGroup("World");
    try
    {
        d->year_offset = OG_World->getAddress( "current_year" );
        d->tick_offset = OG_World->getAddress( "current_tick" );
        d->StartedTime = true;
    }
    catch(Error::All &){};
    try
    {
        d->weather_offset = OG_World->getAddress( "current_weather" );
        d->StartedWeather = true;
    }
    catch(Error::All &){};
    try
    {
        d->gamemode_offset = OG_World->getAddress( "game_mode" );
        d->controlmode_offset = OG_World->getAddress( "control_mode" );
        d->controlmodecopy_offset = OG_World->getAddress( "control_mode" );
        d->StartedMode = true;
    }
    catch(Error::All &){};
    d->Inited = true;
}

World::~World()
{
    delete d;
}

bool World::Start()
{
    return true;
}

bool World::Finish()
{
    return true;
}

uint32_t World::ReadCurrentYear()
{
    if(d->Inited && d->StartedTime)
        return(d->owner->readDWord(d->year_offset));
    return 0;
}

uint32_t World::ReadCurrentTick()
{
    if(d->Inited && d->StartedTime)
        return(d->owner->readDWord(d->tick_offset));
    return 0;
}

bool World::ReadGameMode(t_gamemodes& rd)
{
    if(d->Inited && d->StartedMode)
    {
        rd.control_mode = (ControlMode) d->owner->readDWord( d->controlmode_offset);
        rd.game_mode = (GameMode) d->owner->readDWord(d->gamemode_offset);
        return true;
    }
    return false;
}
bool World::WriteGameMode(const t_gamemodes & wr)
{
    if(d->Inited && d->StartedMode)
    {
        d->owner->writeDWord(d->gamemode_offset,wr.game_mode);
        d->owner->writeDWord(d->controlmode_offset,wr.control_mode);
        d->owner->writeDWord(d->controlmodecopy_offset,wr.control_mode);
        return true;
    }
    return false;
}

// FIX'D according to this:
/*
World::ReadCurrentMonth and World::ReadCurrentDay
« Sent to: peterix on: June 04, 2010, 04:44:30 »
« You have forwarded or responded to this message. »
ReplyQuoteRemove
Shouldn't these be /28 and %28 instead of 24?  There're 28 days in a DF month.
Using 28 and doing the calculation on the value stored at the memory location
specified by memory.xml gets me the current month/date.
*/
uint32_t World::ReadCurrentMonth()
{
    return this->ReadCurrentTick() / 1200 / 28;
}

uint32_t World::ReadCurrentDay()
{
    return ((this->ReadCurrentTick() / 1200) % 28) + 1;
}

uint8_t World::ReadCurrentWeather()
{
    if (d->Inited && d->StartedWeather)
        return(d->owner->readByte(d->weather_offset + 12));
    return 0;
}
/*
void World::SetCurrentWeather(uint8_t weather)
{
    if (d->Inited && d->StartedWeather)
        d->owner->writeByte(d->weather_offset,weather);
}
*/
void World::SetCurrentWeather(uint8_t weather)
{
    if (d->Inited && d->StartedWeather)
    {
        uint8_t buf[25];
        memset(&buf,weather, sizeof(buf));
        d->owner->write(d->weather_offset,sizeof(buf),buf);
    }
}
