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
#include "modules/Constructions.h"
#include "df/world.h"
using namespace DFHack;
using namespace DFHack::Simple;
using df::global::world;

bool Constructions::isValid()
{
    return (world != NULL);
}

uint32_t Constructions::getCount()
{
    return world->constructions.size();
}

df::construction * Constructions::getConstruction(const int32_t index)
{
    if (index < 0 || index >= getCount())
        return NULL;
    return world->constructions[index];
}

bool Constructions::copyConstruction(const int32_t index, t_construction &out)
{
    if (index < 0 || index >= getCount())
        return false;

    out.origin = world->constructions[index];

    out.pos = out.origin->pos;
    out.item_type = out.origin->item_type;
    out.item_subtype = out.origin->item_subtype;
    out.mat_type = out.origin->mat_type;
    out.mat_index = out.origin->mat_index;
    out.flags = out.origin->flags;
    out.original_tile = out.origin->original_tile;
    return true;
}
