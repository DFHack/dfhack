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

#include "dfhack/modules/World.h"
#include "dfhack/Process.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/Types.h"
#include "dfhack/Error.h"
#include "ModuleFactory.h"
#include <dfhack/Core.h>

using namespace DFHack;

Module* DFHack::createWorld()
{
    return new World();
}

struct World::Private
{
    Private()
    {
        Inited = StartedTime = StartedWeather = StartedMode = PauseInited = false;
    }
    bool Inited;

    bool PauseInited;
    uint32_t pause_state_offset;

    bool StartedTime;
    uint32_t year_offset;
    uint32_t tick_offset;

    bool StartedWeather;
    uint32_t weather_offset;

    bool StartedMode;
    uint32_t gamemode_offset;
    uint32_t controlmode_offset;
    uint32_t controlmodecopy_offset;

    bool StartedFolder;
    uint32_t folder_name_offset;

    Process * owner;
};

World::World()
{
    Core & c = Core::getInstance();
    d = new Private;
    d->owner = c.p;
    wmap = 0;

    OffsetGroup * OG_World = c.vinfo->getGroup("World");
    try
    {
        d->year_offset = OG_World->getAddress( "current_year" );
        d->tick_offset = OG_World->getAddress( "current_tick" );
        d->StartedTime = true;
    }
    catch(Error::All &){};
    OffsetGroup * OG_Gui = c.vinfo->getGroup("GUI");
    try
    {
        d->pause_state_offset = OG_Gui->getAddress ("pause_state");
        d->PauseInited = true;
    }
    catch(exception &){};
    try
    {
        d->weather_offset = OG_World->getAddress( "current_weather" );
        wmap = (weather_map *) d->weather_offset;
        d->StartedWeather = true;
    }
    catch(Error::All &){};
    try
    {
        d->gamemode_offset = OG_World->getAddress( "game_mode" );
        d->controlmode_offset = OG_World->getAddress( "control_mode" );
        d->StartedMode = true;
    }
    catch(Error::All &){};
    try
    {
        d->folder_name_offset = OG_World->getAddress( "save_folder" );
        d->StartedFolder = true;
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

bool World::ReadPauseState()
{
    if(!d->PauseInited) return false;
    uint32_t pauseState = d->owner->readDWord (d->pause_state_offset);
    return pauseState & 1;
}

void World::SetPauseState(bool paused)
{
    if(!d->PauseInited) return;
    d->owner->writeDWord (d->pause_state_offset, paused);
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
        rd.g_mode = (GameMode) d->owner->readDWord( d->controlmode_offset);
        rd.g_type = (GameType) d->owner->readDWord(d->gamemode_offset);
        return true;
    }
    return false;
}
bool World::WriteGameMode(const t_gamemodes & wr)
{
    if(d->Inited && d->StartedMode)
    {
        d->owner->writeDWord(d->gamemode_offset,wr.g_mode);
        d->owner->writeDWord(d->controlmode_offset,wr.g_type);
        return true;
    }
    return false;
}

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

// FIX'D according to this:
/*
World::ReadCurrentMonth and World::ReadCurrentDay
« Sent to: peterix on: June 04, 2010, 04:44:30 »
« You have forwarded or responded to this message. »

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

void World::SetCurrentWeather(uint8_t weather)
{
    if (d->Inited && d->StartedWeather)
    {
        uint8_t buf[25];
        memset(&buf,weather, sizeof(buf));
        d->owner->write(d->weather_offset,sizeof(buf),buf);
    }
}

string World::ReadWorldFolder()
{
    if (d->Inited && d->StartedFolder)
    {
        return string( * ( (string*) d->folder_name_offset ) );
    }
    return string("");
}
