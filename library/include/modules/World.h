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

    class DFHACK_EXPORT PersistentDataItem {
        int id;
        std::string key_value;

        std::string *str_value;
        int *int_values;
    public:
        static const int NumInts = 7;

        bool isValid() const { return id != 0; }
        int entry_id() const { return -id; }

        int raw_id() const { return id; }

        const std::string &key() const { return key_value; }

        std::string &val() { return *str_value; }
        const std::string &val() const { return *str_value; }
        int &ival(int i) { return int_values[i]; }
        int ival(int i) const { return int_values[i]; }

        // Pack binary data into string field.
        // Since DF serialization chokes on NUL bytes,
        // use bit magic to ensure none of the bytes is 0.
        // Choose the lowest bit for padding so that
        // sign-extend can be used normally.

        size_t data_size() const { return str_value->size(); }

        bool check_data(size_t off, size_t sz = 1) {
            return (str_value->size() >= off+sz);
        }
        void ensure_data(size_t off, size_t sz = 0) {
            if (str_value->size() < off+sz) str_value->resize(off+sz, '\x01');
        }
        uint8_t *pdata(size_t off) { return (uint8_t*)&(*str_value)[off]; }

        static const size_t int7_size = 1;
        uint8_t get_uint7(size_t off) {
            uint8_t *p = pdata(off);
            return p[0]>>1;
        }
        int8_t get_int7(size_t off) {
            uint8_t *p = pdata(off);
            return int8_t(p[0])>>1;
        }
        void set_uint7(size_t off, uint8_t val) {
            uint8_t *p = pdata(off);
            p[0] = uint8_t((val<<1) | 1);
        }
        void set_int7(size_t off, int8_t val) { set_uint7(off, val); }

        static const size_t int28_size = 4;
        uint32_t get_uint28(size_t off) {
            uint8_t *p = pdata(off);
            return (p[0]>>1) | ((p[1]&~1U)<<6) | ((p[2]&~1U)<<13) | ((p[3]&~1U)<<20);
        }
        int32_t get_int28(size_t off) {
            uint8_t *p = pdata(off);
            return (p[0]>>1) | ((p[1]&~1U)<<6) | ((p[2]&~1U)<<13) | ((int8_t(p[3])&~1)<<20);
        }
        void set_uint28(size_t off, uint32_t val) {
            uint8_t *p = pdata(off);
            p[0] = uint8_t((val<<1) | 1);
            p[1] = uint8_t((val>>6) | 1);
            p[2] = uint8_t((val>>13) | 1);
            p[3] = uint8_t((val>>20) | 1);
        }
        void set_int28(size_t off, int32_t val) { set_uint28(off, val); }

        PersistentDataItem() : id(0), str_value(0), int_values(0) {}
        PersistentDataItem(int id, const std::string &key, std::string *sv, int *iv)
            : id(id), key_value(key), str_value(sv), int_values(iv) {}
    };

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

        DFHACK_EXPORT void ClearPersistentCache();

        DFHACK_EXPORT df::tile_bitmask *getPersistentTilemask(const PersistentDataItem &item, df::map_block *block, bool create = false);
        DFHACK_EXPORT bool deletePersistentTilemask(const PersistentDataItem &item, df::map_block *block);
    }
}
#endif

