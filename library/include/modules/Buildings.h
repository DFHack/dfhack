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

namespace DFHack
{
namespace Simple
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
    uint32_t type;
    int32_t custom_type;
    void * origin;
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

}
}
}
