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
#include "Export.h"
#include "DataDefs.h"
#include "Types.h"
#include "df/building.h"
#include "df/building_stockpilest.h"
#include "df/building_type.h"
#include "df/civzone_type.h"
#include "df/furnace_type.h"
#include "df/item.h"
#include "df/workshop_type.h"
#include "df/construction_type.h"
#include "df/shop_type.h"
#include "df/siegeengine_type.h"
#include "df/trap_type.h"
#include "modules/Items.h"
#include "modules/Maps.h"

namespace df
{
    struct job_item;
    struct item;
    struct building_extents;
    struct building_civzonest;
}

namespace DFHack
{
namespace Buildings
{
/**
 * Structure for holding a read DF building object
 * \ingroup grp_buildings
 */
struct t_building
{
    uint32_t x1;
    uint32_t y1;
    uint32_t x2;
    uint32_t y2;
    uint32_t z;
    t_matglossPair material;
    df::building_type type;
    union
    {
        int16_t subtype;
        df::civzone_type civzone_type;
        df::furnace_type furnace_type;
        df::workshop_type workshop_type;
        df::construction_type construction_type;
        df::shop_type shop_type;
        df::siegeengine_type siegeengine_type;
        df::trap_type trap_type;
    };
    int32_t custom_type;
    df::building * origin;
};

/**
 * The Buildings module - allows reading DF buildings
 * \ingroup grp_modules
 * \ingroup grp_buildings
 */
DFHACK_EXPORT uint32_t getNumBuildings ();

/**
 * read building by index
 */
DFHACK_EXPORT bool Read (const uint32_t index, t_building & building);

/**
 * read mapping from custom_type value to building RAW name
 * custom_type of -1 implies ordinary building
 */
DFHACK_EXPORT bool ReadCustomWorkshopTypes(std::map <uint32_t, std::string> & btypes);

DFHACK_EXPORT df::general_ref *getGeneralRef(df::building *building, df::general_ref_type type);
DFHACK_EXPORT df::specific_ref *getSpecificRef(df::building *building, df::specific_ref_type type);

/**
 * Sets the owner unit for the building.
 */
DFHACK_EXPORT bool setOwner(df::building *building, df::unit *owner);

/**
 * Find the building located at the specified tile.
 * Does not work on civzones.
 */
DFHACK_EXPORT df::building *findAtTile(df::coord pos);

/**
 * Find civzones located at the specified tile.
 */
DFHACK_EXPORT bool findCivzonesAt(std::vector<df::building_civzonest*> *pvec, df::coord pos);

/**
 * Allocates a building object using this type and position.
 */
DFHACK_EXPORT df::building *allocInstance(df::coord pos, df::building_type type, int subtype = -1, int custom = -1);

/**
 * Sets size and center to the correct dimensions of the building.
 * If part of the original size value was used, returns true.
 */
DFHACK_EXPORT bool getCorrectSize(df::coord2d &size, df::coord2d &center,
                                  df::building_type type, int subtype = -1, int custom = -1,
                                  int direction = 0);

/**
 * Checks if the tiles are free to be built upon.
 */
DFHACK_EXPORT bool checkFreeTiles(df::coord pos, df::coord2d size,
                                  df::building_extents *ext = NULL,
                                  bool create_ext = false, bool allow_occupied = false);

/**
 * Returns the number of tiles included by the extent, or defval.
 */
DFHACK_EXPORT int countExtentTiles(df::building_extents *ext, int defval = -1);

/**
 * Checks if the building contains the specified tile.
 */
DFHACK_EXPORT bool containsTile(df::building *bld, df::coord2d tile, bool room = false);

/**
 * Checks if the area has support from the terrain.
 */
DFHACK_EXPORT bool hasSupport(df::coord pos, df::coord2d size);

/**
 * Sets the size of the building, using size and direction as guidance.
 * Returns true if the building can be created at its position, using that size.
 */
DFHACK_EXPORT bool setSize(df::building *bld, df::coord2d size, int direction = 0);

/**
 * Decodes the size of the building into pos and size.
 */
DFHACK_EXPORT std::pair<df::coord,df::coord2d> getSize(df::building *bld);

/**
 * Constructs an abstract building, i.e. stockpile or civzone.
 */
DFHACK_EXPORT bool constructAbstract(df::building *bld);

/**
 * Initiates construction of the building, using specified items as inputs.
 * Returns true if success.
 */
DFHACK_EXPORT bool constructWithItems(df::building *bld, std::vector<df::item*> items);

/**
 * Initiates construction of the building, using specified item filters.
 * Assumes immediate ownership of the item objects, and deletes them in case of error.
 * Returns true if success.
 */
DFHACK_EXPORT bool constructWithFilters(df::building *bld, std::vector<df::job_item*> items);

/**
 * Deconstructs or queues deconstruction of a building.
 * Returns true if the building has been destroyed instantly.
 */
DFHACK_EXPORT bool deconstruct(df::building *bld);

void updateBuildings(color_ostream& out, void* ptr);
void clearBuildings(color_ostream& out);

/**
 * Iterates over the items stored on a stockpile.
 * (For stockpiles with containers, yields the containers, not their contents.)
 *
 * Usage:
 *
 *  Buildings::StockpileIterator stored;
 *  for (stored.begin(stockpile); !stored.done(); ++stored) {
 *      df::item *item = *stored;
 *  }
 *
 * Implementation detail: Uses tile blocks for speed.
 * For each tile block that contains at least part of the stockpile,
 * starting at the top left and moving right, row by row,
 * the block's items are checked for anything on the ground within that stockpile.
 */
class DFHACK_EXPORT StockpileIterator : public std::iterator<std::input_iterator_tag, df::item>
{
    df::building_stockpilest* stockpile;
    df::map_block* block;
    size_t current;
    df::item *item;

public:
    StockpileIterator() {
        stockpile = NULL;
        block = NULL;
        item = NULL;
    }

    StockpileIterator& operator++() {
        while (stockpile) {
            if (block) {
                // Check the next item in the current block.
                ++current;
            } else {
                // Start with the top-left block covering the stockpile.
                block = Maps::getTileBlock(stockpile->x1, stockpile->y1, stockpile->z);
                current = 0;
            }

            while (current >= block->items.size()) {
                // Out of items in this block; find the next block to search.
                if (block->map_pos.x + 16 < stockpile->x2) {
                    block = Maps::getTileBlock(block->map_pos.x + 16, block->map_pos.y, stockpile->z);
                    current = 0;
                } else if (block->map_pos.y + 16 < stockpile->y2) {
                    block = Maps::getTileBlock(stockpile->x1, block->map_pos.y + 16, stockpile->z);
                    current = 0;
                } else {
                    // All items in all blocks have been checked.
                    block = NULL;
                    item = NULL;
                    return *this;
                }
            }

            // If the current item isn't properly stored, move on to the next.
            item = df::item::find(block->items[current]);
            if (!item->flags.bits.on_ground) {
                continue;
            }

            if (!Buildings::containsTile(stockpile, item->pos, false)) {
                continue;
            }

            // Ignore empty bins, barrels, and wheelbarrows assigned here.
            if (item->isAssignedToThisStockpile(stockpile->id)) {
                auto ref = Items::getGeneralRef(item, df::general_ref_type::CONTAINS_ITEM);
                if (!ref) continue;
            }

            // Found a valid item; yield it.
            break;
        }

        return *this;
    }

    void begin(df::building_stockpilest* sp) {
        stockpile = sp;
        operator++();
    }

    df::item* operator*() {
        return item;
    }

    bool done() {
        return block == NULL;
    }
};

/**
 * Collects items stored on a stockpile into a vector.
 */
DFHACK_EXPORT void getStockpileContents(df::building_stockpilest *stockpile, std::vector<df::item*> *items);
DFHACK_EXPORT bool isActivityZone(df::building * building);
DFHACK_EXPORT bool isPenPasture(df::building * building);
DFHACK_EXPORT bool isPitPond(df::building * building);
DFHACK_EXPORT bool isActive(df::building * building);

DFHACK_EXPORT df::building* findPenPitAt(df::coord coord);
}
}
