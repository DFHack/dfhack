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
#ifndef CL_MOD_ENGRAVINGS
#define CL_MOD_ENGRAVINGS
/*
* DF engravings
*/
#include "Export.h"
#include "DataDefs.h"
#include "df/engraving.h"
/**
 * \defgroup grp_engraving Engraving module parts
 * @ingroup grp_modules
 */
namespace DFHack
{
namespace Engravings
{
// "Simplified" copy of engraving
struct t_engraving {
    int32_t artist;
    int32_t masterpiece_event;
    int32_t skill_rating;
    df::coord pos;
    df::engraving_flags flags;
    int8_t tile;
    int32_t art_id;
    int16_t art_subid;
    df::item_quality quality;
    // Pointer to original object, in case you want to modify it
    df::engraving *origin;
};

DFHACK_EXPORT bool isValid();
DFHACK_EXPORT uint32_t getCount();
DFHACK_EXPORT bool copyEngraving (const int32_t index, t_engraving &out);
DFHACK_EXPORT df::engraving * getEngraving (const int32_t index);
}
}
#endif
