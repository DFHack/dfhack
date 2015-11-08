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
#ifndef CL_MOD_CONSTRUCTIONS
#define CL_MOD_CONSTRUCTIONS
/*
 * DF constructions
 */
#include "Export.h"
#include "DataDefs.h"
#include "df/construction.h"
#include "df/construction_type.h"
#include "df/item_type.h"

/**
 * \defgroup grp_constructions Construction module parts
 * @ingroup grp_modules
 */
namespace DFHack
{
namespace Constructions
{
// "Simplified" copy of construction
struct t_construction {
    df::coord pos;
    df::item_type item_type;
    int16_t item_subtype;
    int16_t mat_type;
    int32_t mat_index;
    df::construction_flags flags;
    int16_t original_tile;
    // Pointer to original object, in case you want to modify it
    df::construction *origin;
};

DFHACK_EXPORT bool isValid();
DFHACK_EXPORT uint32_t getCount();
DFHACK_EXPORT bool copyConstruction (const int32_t index, t_construction &out);
DFHACK_EXPORT df::construction * getConstruction (const int32_t index);
DFHACK_EXPORT df::construction * findAtTile(df::coord pos);

DFHACK_EXPORT bool designateNew(df::coord pos, df::construction_type type,
                                df::item_type item = df::item_type::NONE, int mat_index = -1);

DFHACK_EXPORT bool designateRemove(df::coord pos, bool *immediate = NULL);

}
}
#endif
