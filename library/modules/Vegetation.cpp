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


#include "Internal.h"

#include <string>
#include <vector>
#include <map>
using namespace std;

#include "VersionInfo.h"
#include "MemAccess.h"
#include "Types.h"
#include "Core.h"
using namespace DFHack;

#include "modules/Vegetation.h"
#include "df/world.h"

using namespace DFHack;
using df::global::world;

bool Vegetation::isValid()
{
    return (world != NULL);
}

uint32_t Vegetation::getCount()
{
    return world->plants.all.size();
}

df::plant * Vegetation::getPlant(const int32_t index)
{
    if (uint32_t(index) >= getCount())
        return NULL;
    return world->plants.all[index];
}

bool Vegetation::copyPlant(const int32_t index, t_plant &out)
{
    if (uint32_t(index) >= getCount())
        return false;

    out.origin = world->plants.all[index];

    out.name = out.origin->name;
    out.flags = out.origin->flags;
    out.material = out.origin->material;
    out.pos = out.origin->pos;
    out.grow_counter = out.origin->grow_counter;
    out.temperature_1 = out.origin->temperature_1;
    out.temperature_2 = out.origin->temperature_2;
    out.is_burning = out.origin->is_burning;
    out.hitpoints = out.origin->hitpoints;
    out.update_order = out.origin->update_order;
    //out.unk1 = out.origin->anon_1;
    //out.unk2 = out.origin->anon_2;
    //out.temperature_3 = out.origin->temperature_3;
    //out.temperature_4 = out.origin->temperature_4;
    //out.temperature_5 = out.origin->temperature_5;
    return true;
}
