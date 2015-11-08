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

#include "modules/Constructions.h"
#include "modules/Buildings.h"
#include "modules/Maps.h"

#include "TileTypes.h"

#include "df/world.h"
#include "df/job_item.h"
#include "df/building_type.h"
#include "df/building_constructionst.h"

using namespace DFHack;
using namespace df::enums;
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
    if (uint32_t(index) >= getCount())
        return NULL;
    return world->constructions[index];
}

df::construction * Constructions::findAtTile(df::coord pos)
{
    for (auto it = world->constructions.begin(); it != world->constructions.end(); ++it) {
        if ((*it)->pos == pos)
            return *it;
    }
    return NULL;
}

bool Constructions::copyConstruction(const int32_t index, t_construction &out)
{
    if (uint32_t(index) >= getCount())
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

bool Constructions::designateNew(df::coord pos, df::construction_type type,
                                 df::item_type item, int mat_index)
{
    auto ttype = Maps::getTileType(pos);
    if (!ttype || tileMaterial(*ttype) == tiletype_material::CONSTRUCTION)
        return false;

    auto current = Buildings::findAtTile(pos);
    if (current)
    {
        auto cons = strict_virtual_cast<df::building_constructionst>(current);
        if (!cons)
            return false;

        cons->type = type;
        return true;
    }

    auto newinst = Buildings::allocInstance(pos, building_type::Construction);
    if (!newinst)
        return false;

    auto newcons = strict_virtual_cast<df::building_constructionst>(newinst);
    newcons->type = type;

    df::job_item *filter = new df::job_item();
    filter->item_type = item;
    filter->mat_index = mat_index;
    filter->flags2.bits.building_material = true;
    if (mat_index < 0)
        filter->flags2.bits.non_economic = true;

    std::vector<df::job_item*> filters;
    filters.push_back(filter);

    if (!Buildings::constructWithFilters(newinst, filters))
    {
        delete newinst;
        return false;
    }

    return true;
}

bool Constructions::designateRemove(df::coord pos, bool *immediate)
{
    using df::global::process_dig;

    if (immediate)
        *immediate = false;

    if (auto current = Buildings::findAtTile(pos))
    {
        auto cons = strict_virtual_cast<df::building_constructionst>(current);
        if (!cons)
            return false;

        if (Buildings::deconstruct(cons))
        {
            if (immediate)
                *immediate = true;
        }

        return true;
    }

    auto block = Maps::getTileBlock(pos);
    if (!block)
        return false;

    auto ttype = block->tiletype[pos.x&15][pos.y&15];

    if (tileMaterial(ttype) == tiletype_material::CONSTRUCTION)
    {
        auto &dsgn = block->designation[pos.x&15][pos.y&15];
        dsgn.bits.dig = tile_dig_designation::Default;
        block->flags.bits.designated = true;
        if (process_dig)
            *process_dig = true;
        return true;
    }

    return false;
}
