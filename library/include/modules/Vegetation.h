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
#ifndef CL_MOD_VEGETATION
#define CL_MOD_VEGETATION
/**
 * \defgroup grp_vegetation Vegetation : stuff that grows and gets cut down or trampled by dwarves
 * @ingroup grp_modules
 */

#include "Export.h"
#include "DataDefs.h"
#include "df/plant.h"

namespace DFHack
{
namespace Vegetation
{
const uint32_t sapling_to_tree_threshold = 120 * 28 * 12 * 3; // 3 years

// "Simplified" copy of plant
struct t_plant {
    df::language_name name;
    df::plant_flags flags;
    int16_t material;
    df::coord pos;
    int32_t grow_counter;
    uint16_t temperature_1;
    uint16_t temperature_2;
    int32_t is_burning;
    int32_t hitpoints;
    int16_t update_order;
    //std::vector<void *> unk1;
    //int32_t unk2;
    //uint16_t temperature_3;
    //uint16_t temperature_4;
    //uint16_t temperature_5;
    // Pointer to original object, in case you want to modify it
    df::plant *origin;
};

DFHACK_EXPORT bool isValid();
DFHACK_EXPORT uint32_t getCount();
DFHACK_EXPORT df::plant * getPlant(const int32_t index);
DFHACK_EXPORT bool copyPlant (const int32_t index, t_plant &out);
}
}
#endif
