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
        next_persistent_id = 0;
    }
    bool Inited;

    bool PauseInited;
    bool StartedWeather;
    bool StartedMode;

    int next_persistent_id;
    std::multimap<std::string, int> persistent_index;

    Process * owner;
};

typedef std::pair<std::string, int> T_persistent_item;

World::World()
{
    Core & c = Core::getInstance();
    d = new Private;
    d->owner = c.p;

    if(df::global::pause_state)
        d->PauseInited = true;

    if(df::global::current_weather)
        d->StartedWeather = true;
    if (df::global::gamemode && df::global::gametype)
        d->StartedMode = true;

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
    return *df::global::pause_state;
}

void World::SetPauseState(bool paused)
{
    if (d->PauseInited)
        *df::global::pause_state = paused;
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
        rd.g_mode = (DFHack::GameMode)*df::global::gamemode;
        rd.g_type = (DFHack::GameType)*df::global::gametype;
        return true;
    }
    return false;
}
bool World::WriteGameMode(const t_gamemodes & wr)
{
    if(d->Inited && d->StartedMode)
    {
        *df::global::gamemode = wr.g_mode;
        *df::global::gametype = wr.g_type;
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
        return (*df::global::current_weather)[2][2];
    return 0;
}

void World::SetCurrentWeather(uint8_t weather)
{
    if (d->Inited && d->StartedWeather)
        memset(df::global::current_weather, weather, 25);
}

string World::ReadWorldFolder()
{
    return world->cur_savegame.save_dir;
}

static PersistentDataItem dataFromHFig(df::historical_figure *hfig)
{
    return PersistentDataItem(hfig->id, hfig->name.first_name, &hfig->name.nickname, hfig->name.words);
}

void World::ClearPersistentCache()
{
    d->next_persistent_id = 0;
    d->persistent_index.clear();
}

bool World::BuildPersistentCache()
{
    if (d->next_persistent_id)
        return true;
    if (!Core::getInstance().isWorldLoaded())
        return false;

    std::vector<df::historical_figure*> &hfvec = df::historical_figure::get_vector();

    // Determine the next entry id as min(-100, lowest_id-1)
    d->next_persistent_id = -100;

    if (hfvec.size() > 0 && hfvec[0]->id <= -100)
        d->next_persistent_id = hfvec[0]->id-1;

    // Add the entries to the lookup table
    d->persistent_index.clear();

    for (size_t i = 0; i < hfvec.size() && hfvec[i]->id <= -100; i++)
    {
        if (!hfvec[i]->name.has_name || hfvec[i]->name.first_name.empty())
            continue;

        d->persistent_index.insert(T_persistent_item(hfvec[i]->name.first_name, -hfvec[i]->id));
    }

    return true;
}

PersistentDataItem World::AddPersistentData(const std::string &key)
{
    if (!BuildPersistentCache() || key.empty())
        return PersistentDataItem();

    std::vector<df::historical_figure*> &hfvec = df::historical_figure::get_vector();

    df::historical_figure *hfig = new df::historical_figure();
    hfig->id = d->next_persistent_id--;
    hfig->name.has_name = true;
    hfig->name.first_name = key;
    memset(hfig->name.words, 0xFF, sizeof(hfig->name.words));

    hfvec.insert(hfvec.begin(), hfig);

    d->persistent_index.insert(T_persistent_item(key, -hfig->id));

    return dataFromHFig(hfig);
}

PersistentDataItem World::GetPersistentData(const std::string &key)
{
    if (!BuildPersistentCache())
        return PersistentDataItem();

    auto it = d->persistent_index.find(key);
    if (it != d->persistent_index.end())
        return GetPersistentData(it->second);

    return PersistentDataItem();
}

PersistentDataItem World::GetPersistentData(int entry_id)
{
    if (entry_id < 100)
        return PersistentDataItem();

    auto hfig = df::historical_figure::find(-entry_id);
    if (hfig && hfig->name.has_name)
        return dataFromHFig(hfig);

    return PersistentDataItem();
}

PersistentDataItem World::GetPersistentData(const std::string &key, bool *added)
{
    *added = false;

    PersistentDataItem rv = GetPersistentData(key);

    if (!rv.isValid())
    {
        *added = true;
        rv = AddPersistentData(key);
    }

    return rv;
}

void World::GetPersistentData(std::vector<PersistentDataItem> *vec, const std::string &key, bool prefix)
{
    if (!BuildPersistentCache())
        return;

    auto eqrange = d->persistent_index.equal_range(key);

    if (prefix)
    {
        if (key.empty())
        {
            eqrange.first = d->persistent_index.begin();
            eqrange.second = d->persistent_index.end();
        }
        else
        {
            std::string bound = key;
            if (bound[bound.size()-1] != '/')
                bound += "/";
            eqrange.first = d->persistent_index.lower_bound(bound);

            bound[bound.size()-1]++;
            eqrange.second = d->persistent_index.lower_bound(bound);
        }
    }

    for (auto it = eqrange.first; it != eqrange.second; ++it)
    {
        auto hfig = df::historical_figure::find(-it->second);
        if (hfig && hfig->name.has_name)
            vec->push_back(dataFromHFig(hfig));
    }
}

bool World::DeletePersistentData(const PersistentDataItem &item)
{
    if (item.id > -100)
        return false;
    if (!BuildPersistentCache())
        return false;

    std::vector<df::historical_figure*> &hfvec = df::historical_figure::get_vector();

    auto eqrange = d->persistent_index.equal_range(item.key_value);

    for (auto it = eqrange.first; it != eqrange.second; ++it)
    {
        if (it->second != -item.id)
            continue;

        d->persistent_index.erase(it);

        int idx = binsearch_index(hfvec, item.id);

        if (idx >= 0) {
            delete hfvec[idx];
            hfvec.erase(hfvec.begin()+idx);
        }

        return true;
    }

    return false;
}
