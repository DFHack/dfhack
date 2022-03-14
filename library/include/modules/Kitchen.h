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
/*
* kitchen settings
*/
#include "Export.h"
#include "Module.h"
#include "Types.h"
#include "VersionInfo.h"
#include "Core.h"
#include "modules/Items.h"
#include "df/kitchen_exc_type.h"

/**
 * \defgroup grp_kitchen Kitchen settings
 * @ingroup grp_modules
 */

namespace DFHack
{
namespace Kitchen
{
/**
 * Kitchen exclusions manipulator. Currently geared towards plants and seeds.
 * \ingroup grp_modules
 * \ingroup grp_kitchen
 */

// print the exclusion list, with the material index also translated into its token (for organics) - for debug really
DFHACK_EXPORT void debug_print(color_ostream &out);

// remove this plant from the exclusion list if it is in it
DFHACK_EXPORT void allowPlantSeedCookery(int32_t plant_id);

// add this plant to the exclusion list, if it is not already in it
DFHACK_EXPORT void denyPlantSeedCookery(int32_t plant_id);

// fills a map with info from the limit info storage entries in the exclusion list
DFHACK_EXPORT void fillWatchMap(std::map<int32_t, int16_t>& watchMap);

// Finds the index of a limit info storage entry. Returns -1 if not found.
DFHACK_EXPORT int findLimit(int32_t plant_id);

// removes a limit info storage entry from the exclusion list if it's present
DFHACK_EXPORT bool removeLimit(int32_t plant_id);

// add a limit info storage item to the exclusion list, or alters an existing one
DFHACK_EXPORT bool setLimit(int32_t plant_id, int16_t limit);

// clears all limit info storage items from the exclusion list
DFHACK_EXPORT void clearLimits();

DFHACK_EXPORT std::size_t size();

// Finds the index of a kitchen exclusion in ui.kitchen.exc_types. Returns -1 if not found.
DFHACK_EXPORT int findExclusion(df::kitchen_exc_type type,
    df::item_type item_type, int16_t item_subtype,
    int16_t mat_type, int32_t mat_index);

// Adds an exclusion. Returns false if already excluded.
DFHACK_EXPORT bool addExclusion(df::kitchen_exc_type type,
    df::item_type item_type, int16_t item_subtype,
    int16_t mat_type, int32_t mat_index);

// Removes an exclusion. Returns false if not excluded.
DFHACK_EXPORT bool removeExclusion(df::kitchen_exc_type type,
    df::item_type item_type, int16_t item_subtype,
    int16_t mat_type, int32_t mat_index);

}
}
