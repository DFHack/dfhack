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
#include "Debug.h"

#include "modules/Gui.h"
#include "modules/Translation.h"
#include "modules/Units.h"
#include "modules/World.h"
#include "modules/Translation.h"

#include "df/block_square_event_world_constructionst.h"
#include "df/gamest.h"
#include "df/map_block.h"
#include "df/plotinfost.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/world.h"
#include "df/world_data.h"
#include "df/world_site.h"

using std::string;

using namespace DFHack;
using namespace df::enums;

using df::global::plotinfo;
using df::global::world;

namespace DFHack
{
    DBG_DECLARE(core, world, DebugCategory::LINFO);
}

bool World::ReadPauseState()
{
    using df::global::game;
    using df::global::pause_state;

    if (!pause_state || !plotinfo || !game || !world)
        return false;

    // count single stepping as paused
    if (World::isFortressMode()) {
        if (auto scr = Gui::getViewscreenByType<df::viewscreen_dwarfmodest>(0); scr && scr->keyRepeat)
            return true;
    }

    return *pause_state ||
        (plotinfo->main.mode != df::ui_sidebar_mode::Default &&
            (plotinfo->main.mode != df::ui_sidebar_mode::Squads ||
             plotinfo->squads.in_kill_order ||
             plotinfo->squads.in_move_order
            )
        ) ||
        (game->main_interface.squads.open &&
            (game->main_interface.squads.giving_kill_order ||
             game->main_interface.squads.giving_move_order ||
             game->main_interface.squads.giving_patrol_order ||
             game->main_interface.squads.giving_burrow_order
            )
        ) ||
        game->main_interface.bottom_mode_selected == df::main_bottom_mode_type::BUILDING_PICK_MATERIALS ||
        (game->main_interface.view_sheets.open &&
            game->main_interface.view_sheets.unit_overview_expelling
        ) ||
        game->main_interface.create_work_order.open ||
        game->main_interface.info.open ||
        game->main_interface.assign_trade.open ||
        game->main_interface.trade.open ||
        game->main_interface.announcement_alert.open ||
        world->status.popups.size() ||
        game->main_interface.stocks.open ||
        game->main_interface.petitions.open ||
        game->main_interface.diplomacy.open ||
        game->main_interface.name_creator.open ||
        game->main_interface.image_creator.open ||
        game->main_interface.assign_vehicle.open ||
        game->main_interface.job_details.open ||
        game->main_interface.options.open ||
        game->main_interface.assign_display_item.open ||
        game->main_interface.custom_stockpile.open;
}

void World::SetPauseState(bool paused)
{
    bool dummy;
    DF_GLOBAL_VALUE(pause_state, dummy) = paused;
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

string World::getWorldName(bool in_english)
{
    if (!world || !world->world_data)
        return "";
    return Translation::translateName(&world->world_data->name, in_english);
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

df::unit * World::getAdventurer() {
    if (!isAdventureMode() || !world)
        return NULL;

    return world->units.adv_unit;
}

int32_t World::GetCurrentSiteId() {
    if (!plotinfo)
        return -1;
    if (isFortressMode())
        return plotinfo->site_id;
    if (auto adv = getAdventurer(); adv && world->world_data) {
        DEBUG(world).print("searching for adventure site\n");
        auto & world_map = world->map;
        auto adv_pos = Units::getPosition(adv);
        DEBUG(world).print("adv_pos: (%d, %d, %d)\n", adv_pos.x, adv_pos.y, adv_pos.z);
        df::coord2d rgn_pos(world_map.region_x + adv_pos.x/48, world_map.region_y + adv_pos.y/48);
        for (auto site : world->world_data->sites) {
            DEBUG(world).print("scanning site %d: %s\n", site->id, Translation::translateName(&site->name, true).c_str());
            DEBUG(world).print("  rgn_pos: (%d, %d); site bounds: (%d, %d), (%d, %d) \n",
                rgn_pos.x, rgn_pos.y, site->global_min_x, site->global_min_y, site->global_max_x, site->global_max_y);
            if (rgn_pos.x >= site->global_min_x && rgn_pos.x <= site->global_max_x
                && rgn_pos.y >= site->global_min_y && rgn_pos.y <= site->global_max_y)
            {
                DEBUG(world).print("found site: %d\n", site->id);
                return site->id;
            }
        }
    }
    return -1;
}

bool World::IsSiteLoaded() {
    return GetCurrentSiteId() != -1;
}

PersistentDataItem World::AddPersistentEntityData(int entity_id, const std::string &key) {
    return Persistence::addItem(entity_id, key);
}

PersistentDataItem World::AddPersistentWorldData(const std::string &key) {
    return AddPersistentEntityData(Persistence::WORLD_ENTITY_ID, key);
}

PersistentDataItem World::AddPersistentSiteData(const std::string &key) {
    return AddPersistentEntityData(GetCurrentSiteId(), key);
}

PersistentDataItem World::GetPersistentEntityData(int entity_id, const std::string &key, bool create) {
    bool added = false;
    return create ? Persistence::getByKey(entity_id, key, &added) : Persistence::getByKey(entity_id, key);
}

PersistentDataItem World::GetPersistentWorldData(const std::string &key, bool create) {
    return GetPersistentEntityData(Persistence::WORLD_ENTITY_ID, key, create);
}

PersistentDataItem World::GetPersistentSiteData(const std::string &key, bool create) {
    return GetPersistentEntityData(GetCurrentSiteId(), key, create);
}

void World::GetPersistentEntityData(std::vector<PersistentDataItem> *vec, int entity_id, const std::string &key, bool prefix) {
    if (prefix && key.empty())
        Persistence::getAll(*vec, entity_id);
    else if (prefix) {
        std::string min = key;
        if (min.back() != '/')
            min.push_back('/');
        std::string max = min;
        ++max.back();
        Persistence::getAllByKeyRange(*vec, entity_id, min, max);
    }
    else
        Persistence::getAllByKey(*vec, entity_id, key);
}

void World::GetPersistentWorldData(std::vector<PersistentDataItem> *vec, const std::string &key, bool prefix) {
    GetPersistentEntityData(vec, Persistence::WORLD_ENTITY_ID, key, prefix);
}

void World::GetPersistentSiteData(std::vector<PersistentDataItem> *vec, const std::string &key, bool prefix) {
    GetPersistentEntityData(vec, GetCurrentSiteId(), key, prefix);
}

bool World::DeletePersistentData(const PersistentDataItem &item) {
    return Persistence::deleteItem(item);
}

df::tile_bitmask *World::getPersistentTilemask(PersistentDataItem &item, df::map_block *block, bool create) {
    if (!block)
        return NULL;

    int id = item.fake_df_id();
    if (id > -100)
        return NULL;

    for (auto ev : block->block_events) {
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

bool World::deletePersistentTilemask(PersistentDataItem &item, df::map_block *block) {
    if (!block)
        return false;

    int id = item.fake_df_id();
    if (id > -100)
        return false;

    bool found = false;
    for (int i = block->block_events.size()-1; i >= 0; i--) {
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
