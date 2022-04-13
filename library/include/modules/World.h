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

#pragma once
#ifndef CL_MOD_WORLD
#define CL_MOD_WORLD

/**
 * \defgroup grp_world World: all kind of stuff related to the current world state
 * @ingroup grp_modules
 */

#include "Export.h"
#include "Module.h"
#include "modules/Persistence.h"
#include <ostream>

#include "DataDefs.h"

namespace df
{
    struct tile_bitmask;
    struct map_block;
}

namespace DFHack
{
    typedef df::game_mode GameMode;
    typedef df::game_type GameType;

#define GAMEMODE_ADVENTURE df::enums::game_mode::ADVENTURE

    /**
     * \ingroup grp_world
     */
    struct t_gamemodes
    {
        GameMode g_mode;
        GameType g_type;
    };
    class DFContextShared;

    /**
     * The World module
     * \ingroup grp_modules
     * \ingroup grp_world
     */
    namespace World
    {
        ///true if paused, false if not
        DFHACK_EXPORT bool ReadPauseState();
        ///true if paused, false if not
        DFHACK_EXPORT void SetPauseState(bool paused);

        DFHACK_EXPORT uint32_t ReadCurrentTick();
        DFHACK_EXPORT uint32_t ReadCurrentYear();
        DFHACK_EXPORT uint32_t ReadCurrentMonth();
        DFHACK_EXPORT uint32_t ReadCurrentDay();
        DFHACK_EXPORT uint8_t ReadCurrentWeather();
        DFHACK_EXPORT void SetCurrentWeather(uint8_t weather);
        DFHACK_EXPORT bool ReadGameMode(t_gamemodes& rd);
        DFHACK_EXPORT bool WriteGameMode(const t_gamemodes & wr); // this is very dangerous
        DFHACK_EXPORT std::string ReadWorldFolder();

        DFHACK_EXPORT bool isFortressMode(df::game_type t = (df::game_type)-1);
        DFHACK_EXPORT bool isAdventureMode(df::game_type t = (df::game_type)-1);
        DFHACK_EXPORT bool isArena(df::game_type t = (df::game_type)-1);
        DFHACK_EXPORT bool isLegends(df::game_type t = (df::game_type)-1);

        // Store data in fake historical figure names.
        // This ensures that the values are stored in save games.
        DFHACK_EXPORT PersistentDataItem AddPersistentData(const std::string &key);
        DFHACK_EXPORT PersistentDataItem GetPersistentData(const std::string &key);
        DFHACK_EXPORT PersistentDataItem GetPersistentData(int entry_id);
        // Calls GetPersistentData(key); if not found, adds and sets added to true.
        // The result can still be not isValid() e.g. if the world is not loaded.
        DFHACK_EXPORT PersistentDataItem GetPersistentData(const std::string &key, bool *added);
        // Lists all items with the given key.
        // If prefix is true, search for keys starting with key+"/".
        // GetPersistentData(&vec,"",true) returns all items.
        // Items have alphabetic order by key; same key ordering is undefined.
        DFHACK_EXPORT void GetPersistentData(std::vector<PersistentDataItem> *vec,
                                             const std::string &key, bool prefix = false);
        // Deletes the item; returns true if success.
        DFHACK_EXPORT bool DeletePersistentData(const PersistentDataItem &item);

        DFHACK_EXPORT df::tile_bitmask *getPersistentTilemask(const PersistentDataItem &item, df::map_block *block, bool create = false);
        DFHACK_EXPORT bool deletePersistentTilemask(const PersistentDataItem &item, df::map_block *block);
    }
}
#endif
