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

#pragma once
#include "Export.h"
#include "DataDefs.h"
#include "df/building.h"
#include "df/civzone_type.h"
#include "df/furnace_type.h"
#include "df/workshop_type.h"
#include "df/construction_type.h"
#include "df/shop_type.h"
#include "df/siegeengine_type.h"
#include "df/trap_type.h"

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

}
}
