/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

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
#include <array>
#include <string>
#include <vector>
#include <map>
#include <unordered_set>
#include <cstring>
using namespace std;

#include "modules/World.h"
#include "MemAccess.h"
#include "VersionInfo.h"
#include "Types.h"
#include "Error.h"
#include "ModuleFactory.h"
#include "Core.h"

#include "modules/Maps.h"

#include "MiscUtils.h"

#include "VTableInterpose.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/historical_figure.h"
#include "df/map_block.h"
#include "df/block_square_event_world_constructionst.h"
#include "df/viewscreen_legendsst.h"
#include "df/d_init.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/ui.h"
#include "VTableInterpose.h"
#include "PluginManager.h"

using namespace DFHack;
using namespace Pausing;
using namespace df::enums;

using df::global::world;

bool World::ReadPauseState()
{
    return DF_GLOBAL_VALUE(pause_state, false);
}

void World::SetPauseState(bool paused)
{
    bool dummy;
    DF_GLOBAL_VALUE(pause_state, dummy) = paused;
}

std::unordered_set<Lock*> PlayerLock::locks;
std::unordered_set<Lock*> AnnouncementLock::locks;

template<typename Locks>
inline bool any_lock(Locks locks) {
    return std::any_of(locks.begin(), locks.end(), [](Lock* lock) { return lock->isLocked(); });
}

template<typename Locks, typename LockT>
inline bool only_lock(Locks locks, LockT* this_lock) {
    return std::all_of(locks.begin(), locks.end(), [&](Lock* lock) {
        if (lock == this_lock) {
            return lock->isLocked();
        }
        return !lock->isLocked();
    });
}

bool AnnouncementLock::isAnyLocked() const {
    return any_lock(locks);
}

bool AnnouncementLock::isOnlyLocked() const {
    return only_lock(locks, this);
}

bool PlayerLock::isAnyLocked() const {
    return any_lock(locks);
}

bool PlayerLock::isOnlyLocked() const {
    return only_lock(locks, this);
}

namespace pausing {
    AnnouncementLock announcementLock("monitor");
    PlayerLock playerLock("monitor");

    const size_t array_size = sizeof(decltype(df::announcements::flags)) / sizeof(df::announcement_flags);
    bool state_saved = false; // indicates whether a restore state is ok
    bool announcements_disabled = false; // indicates whether disable or restore was last enacted, could use a better name
    bool saved_states[array_size]; // state to restore
    bool locked_states[array_size]; // locked state (re-applied each frame)
    bool allow_player_pause = true; // toggles player pause ability

    using df::global::ui;
    using namespace df::enums;
    struct player_pause_hook : df::viewscreen_dwarfmodest {
        typedef df::viewscreen_dwarfmodest interpose_base;
        DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key>* input)) {
            if ((ui->main.mode == ui_sidebar_mode::Default) && !allow_player_pause) {
                input->erase(interface_key::D_PAUSE);
            }
            INTERPOSE_NEXT(feed)(input);
        }
    };

    IMPLEMENT_VMETHOD_INTERPOSE(player_pause_hook, feed);
}
using namespace pausing;

AnnouncementLock* World::AcquireAnnouncementPauseLock(const char* name) {
    return new AnnouncementLock(name);
}

PlayerLock* World::AcquirePlayerPauseLock(const char* name) {
    return new PlayerLock(name);
}

void World::ReleasePauseLock(Lock* lock){
    delete lock;
}

bool World::DisableAnnouncementPausing() {
    if (!announcementLock.isAnyLocked()) {
        for (auto& flag : df::global::d_init->announcements.flags) {
            flag.bits.PAUSE = false;
        }
        announcements_disabled = true;
    }
    return announcements_disabled;
}

bool World::SaveAnnouncementSettings() {
    if (!announcementLock.isAnyLocked()) {
        for (size_t i = 0; i < array_size; ++i) {
            saved_states[i] = df::global::d_init->announcements.flags[i].bits.PAUSE;
        }
        return true;
    }
    return false;
}

bool World::RestoreAnnouncementSettings() {
    if (!announcementLock.isAnyLocked() && state_saved) {
        for (size_t i = 0; i < array_size; ++i) {
            df::global::d_init->announcements.flags[i].bits.PAUSE = saved_states[i];
        }
        announcements_disabled = false;
        return true;
    }
    return false;
}

bool World::EnablePlayerPausing() {
    if (!playerLock.isAnyLocked()) {
        allow_player_pause = true;
    }
    return allow_player_pause;
}

bool World::DisablePlayerPausing() {
    if (!playerLock.isAnyLocked()) {
        allow_player_pause = false;
    }
    return !allow_player_pause;
}

void World::Update() {
    static bool did_once = false;
    if (!did_once) {
        did_once = true;
        INTERPOSE_HOOK(player_pause_hook, feed).apply();
    }
    if (announcementLock.isAnyLocked()) {
        for (size_t i = 0; i < array_size; ++i) {
            df::global::d_init->announcements.flags[i].bits.PAUSE = locked_states[i];
        }
    }
}


uint32_t World::ReadCurrentYear()
{
    return DF_GLOBAL_VALUE(cur_year, 0);
}

uint32_t World::ReadCurrentTick()
{
    // prevent this from returning anything less than 0,
    // to avoid day/month calculations with 0xffffffff
    return std::max(0, DF_GLOBAL_VALUE(cur_year_tick, 0));
}

bool World::ReadGameMode(t_gamemodes& rd)
{
    if(df::global::gamemode && df::global::gametype)
    {
        rd.g_mode = (DFHack::GameMode)*df::global::gamemode;
        rd.g_type = (DFHack::GameType)*df::global::gametype;
        return true;
    }
    return false;
}
bool World::WriteGameMode(const t_gamemodes & wr)
{
    if(df::global::gamemode && df::global::gametype)
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
    return ReadCurrentTick() / 1200 / 28;
}

uint32_t World::ReadCurrentDay()
{
    return ((ReadCurrentTick() / 1200) % 28) + 1;
}

uint8_t World::ReadCurrentWeather()
{
    if (df::global::current_weather)
        return (*df::global::current_weather)[2][2];
    return 0;
}

void World::SetCurrentWeather(uint8_t weather)
{
    if (df::global::current_weather)
        memset(df::global::current_weather, weather, 25);
}

string World::ReadWorldFolder()
{
    return world->cur_savegame.save_dir;
}

bool World::isFortressMode(df::game_type t)
{
    if (t == -1 && df::global::gametype)
        t = *df::global::gametype;
    return (t == game_type::DWARF_MAIN || t == game_type::DWARF_RECLAIM ||
        t == game_type::DWARF_UNRETIRE);
}

bool World::isAdventureMode(df::game_type t)
{
    if (t == -1 && df::global::gametype)
        t = *df::global::gametype;
    return (t == game_type::ADVENTURE_MAIN);
}

bool World::isArena(df::game_type t)
{
    if (t == -1 && df::global::gametype)
        t = *df::global::gametype;
    return (t == game_type::DWARF_ARENA || t == game_type::ADVENTURE_ARENA);
}

bool World::isLegends(df::game_type t)
{
    if (t == -1 && df::global::gametype)
        t = *df::global::gametype;
    return (t == game_type::VIEW_LEGENDS);
}

PersistentDataItem World::AddPersistentData(const std::string &key)
{
    return Persistence::addItem(key);
}

PersistentDataItem World::GetPersistentData(const std::string &key)
{
    return Persistence::getByKey(key);
}

PersistentDataItem World::GetPersistentData(int entry_id)
{
    if (entry_id < 100)
        return PersistentDataItem();

    return Persistence::getByIndex(size_t(entry_id - 100));
}

PersistentDataItem World::GetPersistentData(const std::string &key, bool *added)
{
    bool temp = false;
    if (!added)
        added = &temp;

    return Persistence::getByKey(key, added);
}

void World::GetPersistentData(std::vector<PersistentDataItem> *vec, const std::string &key, bool prefix)
{
    if (prefix && key.empty())
    {
        Persistence::getAll(*vec);
    }
    else if (prefix)
    {
        std::string min = key;
        if (min.back() != '/')
        {
            min.push_back('/');
        }
        std::string max = min;
        ++max.back();

        Persistence::getAllByKeyRange(*vec, min, max);
    }
    else
    {
        Persistence::getAllByKey(*vec, key);
    }
}

bool World::DeletePersistentData(const PersistentDataItem &item)
{
    return Persistence::deleteItem(item);
}

df::tile_bitmask *World::getPersistentTilemask(const PersistentDataItem &item, df::map_block *block, bool create)
{
    if (!block)
        return NULL;

    int id = item.raw_id();
    if (id > -100)
        return NULL;

    for (size_t i = 0; i < block->block_events.size(); i++)
    {
        auto ev = block->block_events[i];
        if (ev->getType() != block_square_event_type::world_construction)
            continue;
        auto wcsev = strict_virtual_cast<df::block_square_event_world_constructionst>(ev);
        if (!wcsev || wcsev->construction_id != id)
            continue;
        return &wcsev->tile_bitmask;
    }

    if (!create)
        return NULL;

    auto ev = df::allocate<df::block_square_event_world_constructionst>();
    if (!ev)
        return NULL;

    ev->construction_id = id;
    ev->tile_bitmask.clear();
    vector_insert_at(block->block_events, 0, (df::block_square_event*)ev);

    return &ev->tile_bitmask;
}

bool World::deletePersistentTilemask(const PersistentDataItem &item, df::map_block *block)
{
    if (!block)
        return false;
    int id = item.raw_id();
    if (id > -100)
        return false;

    bool found = false;
    for (int i = block->block_events.size()-1; i >= 0; i--)
    {
        auto ev = block->block_events[i];
        if (ev->getType() != block_square_event_type::world_construction)
            continue;
        auto wcsev = strict_virtual_cast<df::block_square_event_world_constructionst>(ev);
        if (!wcsev || wcsev->construction_id != id)
            continue;

        delete wcsev;
        vector_erase_at(block->block_events, i);
        found = true;
    }

    return found;
}
