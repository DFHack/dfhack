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


#include "Internal.h"

#include <string>
#include <vector>
#include <map>
using namespace std;

#include "VersionInfo.h"
#include "MemAccess.h"
#include "Types.h"
#include "Core.h"

#include "modules/Engravings.h"
#include "df/world.h"

using namespace DFHack;
using df::global::world;

bool Engravings::isValid()
{
    return (world != NULL);
}

uint32_t Engravings::getCount()
{
    return world->engravings.size();
}

df::engraving * Engravings::getEngraving(int index)
{
    if (uint32_t(index) >= getCount())
        return NULL;
    return world->engravings[index];
}

bool Engravings::copyEngraving(const int32_t index, t_engraving &out)
{
    if (uint32_t(index) >= getCount())
        return false;

    out.origin = world->engravings[index];

    out.artist = out.origin->artist;
    out.masterpiece_event = out.origin->masterpiece_event;
    out.skill_rating = out.origin->skill_rating;
    out.pos = out.origin->pos;
    out.flags = out.origin->flags;
    out.tile = out.origin->tile;
    out.art_id = out.origin->art_id;
    out.art_subid = out.origin->art_subid;
    out.quality = out.origin->quality;
    return true;
}
