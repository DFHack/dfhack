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

#include "modules/World.h"
#include "MemAccess.h"
#include "VersionInfo.h"
#include "Types.h"
#include "Error.h"
#include "ModuleFactory.h"
#include "Core.h"

#include "MiscUtils.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/historical_figure.h"

using namespace DFHack;

using df::global::world;

Module* DFHack::createWorld()
{
    return new World();
}

struct World::Private
{
    Private()
    {
        Inited = PauseInited = StartedWeather = StartedMode = false;
    }
    bool Inited;

    bool PauseInited;
    void * pause_state_offset;

    bool StartedWeather;
    char * weather_offset;

    bool StartedMode;
    void * gamemode_offset;
    void * controlmode_offset;
    void * controlmodecopy_offset;

    Process * owner;
};

World::World()
{
    Core & c = Core::getInstance();
    d = new Private;
    d->owner = c.p;
    wmap = 0;

    OffsetGroup * OG_Gui = c.vinfo->getGroup("GUI");
    try
    {
        d->pause_state_offset = OG_Gui->getAddress ("pause_state");
        d->PauseInited = true;
    }
    catch(exception &){};

    OffsetGroup * OG_World = c.vinfo->getGroup("World");
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
    uint8_t pauseState = d->owner->readByte (d->pause_state_offset);
    return pauseState & 1;
}

void World::SetPauseState(bool paused)
{
    if(!d->PauseInited) return;
    d->owner->writeByte (d->pause_state_offset, paused);
}

uint32_t World::ReadCurrentYear()
{
    return *df::global::cur_year;
}

uint32_t World::ReadCurrentTick()
{
    return *df::global::cur_year_tick;
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
        d->owner->writeDWord(d->gamemode_offset,wr.g_type);
        d->owner->writeDWord(d->controlmode_offset,wr.g_mode);
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
    return world->unk_192bd8.save_dir;
}

static PersistentDataItem dataFromHFig(df::historical_figure *hfig)
{
    return PersistentDataItem(hfig->id, hfig->name.first_name, &hfig->name.nickname, hfig->name.words);
}

PersistentDataItem World::AddPersistentData(const std::string &key)
{
    std::vector<df::historical_figure*> &hfvec = df::historical_figure::get_vector();

    int new_id = -100;
    if (hfvec.size() > 0 && hfvec[0]->id <= new_id)
        new_id = hfvec[0]->id-1;

    df::historical_figure *hfig = new df::historical_figure();
    hfig->id = new_id;
    hfig->name.has_name = true;
    hfig->name.first_name = key;
    memset(hfig->name.words, 0xFF, sizeof(hfig->name.words));

    hfvec.insert(hfvec.begin(), hfig);
    return dataFromHFig(hfig);
}

PersistentDataItem World::GetPersistentData(const std::string &key)
{
    std::vector<df::historical_figure*> &hfvec = df::historical_figure::get_vector();
    for (size_t i = 0; i < hfvec.size(); i++)
    {
        df::historical_figure *hfig = hfvec[i];

        if (hfig->id >= 0)
            break;

        if (hfig->name.has_name && hfig->name.first_name == key)
            return dataFromHFig(hfig);
    }

    return PersistentDataItem();
}

void World::GetPersistentData(std::vector<PersistentDataItem> *vec, const std::string &key)
{
    std::vector<df::historical_figure*> &hfvec = df::historical_figure::get_vector();
    for (size_t i = 0; i < hfvec.size(); i++)
    {
        df::historical_figure *hfig = hfvec[i];

        if (hfig->id >= 0)
            break;

        if (hfig->name.has_name && hfig->name.first_name == key)
            vec->push_back(dataFromHFig(hfig));
    }
}

void World::DeletePersistentData(const PersistentDataItem &item)
{
    if (item.id > -100)
        return;

    std::vector<df::historical_figure*> &hfvec = df::historical_figure::get_vector();
    int idx = binsearch_index(hfvec, item.id);
    if (idx >= 0) {
        delete hfvec[idx];
        hfvec.erase(hfvec.begin()+idx);
    }
}
