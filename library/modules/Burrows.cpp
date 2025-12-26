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

#include "Core.h"
#include "DataDefs.h"
#include "Error.h"
#include "MiscUtils.h"

#include "modules/Burrows.h"
#include "modules/Maps.h"
#include "modules/Units.h"

#include "df/block_burrow.h"
#include "df/block_burrow_link.h"
#include "df/burrow.h"
#include "df/language_name.h"
#include "df/map_block.h"
#include "df/plotinfost.h"
#include "df/unit.h"
#include "df/world.h"

#include <vector>
#include <cstdlib>

using namespace DFHack;
using namespace df::enums;

using df::global::world;
using df::global::plotinfo;

df::burrow *Burrows::findByName(std::string name, bool ignore_final_plus)
{
    auto &vec = df::burrow::get_vector();

    if (ignore_final_plus && name.ends_with('+'))
        name = name.substr(0, name.length() - 1);

    for (size_t i = 0; i < vec.size(); i++) {
        std::string bname = vec[i]->name;
        if (ignore_final_plus && bname.ends_with('+'))
            bname = bname.substr(0, bname.length() - 1);

        if (bname == name)
            return vec[i];
    }

    return NULL;
}

void Burrows::clearUnits(df::burrow *burrow)
{
    CHECK_NULL_POINTER(burrow);

    for (size_t i = 0; i < burrow->units.size(); i++)
    {
        auto unit = df::unit::find(burrow->units[i]);

        if (unit) {
            erase_from_vector(unit->burrows, burrow->id);
            erase_from_vector(unit->inactive_burrows, burrow->id);
        }
    }

    burrow->units.clear();
}

bool Burrows::isAssignedUnit(df::burrow *burrow, df::unit *unit)
{
    CHECK_NULL_POINTER(unit);
    CHECK_NULL_POINTER(burrow);

    return binsearch_index(unit->burrows, burrow->id) >= 0 ||
        binsearch_index(unit->inactive_burrows, burrow->id) >= 0;
}

void Burrows::setAssignedUnit(df::burrow *burrow, df::unit *unit, bool enable)
{
    CHECK_NULL_POINTER(unit);
    CHECK_NULL_POINTER(burrow);

    if (enable) {
        if (burrow->flags.bits.suspended)
            insert_into_vector(unit->inactive_burrows, burrow->id);
        else
            insert_into_vector(unit->burrows, burrow->id);
        insert_into_vector(burrow->units, unit->id);
    } else {
        erase_from_vector(unit->burrows, burrow->id);
        erase_from_vector(unit->inactive_burrows, burrow->id);
        erase_from_vector(burrow->units, unit->id);
    }
}

void Burrows::listBlocks(std::vector<df::map_block*> *pvec, df::burrow *burrow)
{
    CHECK_NULL_POINTER(burrow);

    pvec->clear();
    pvec->reserve(burrow->block_x.size());

    df::coord base(world->map.region_x*3,world->map.region_y*3,world->map.region_z);

    for (size_t i = 0; i < burrow->block_x.size(); i++)
    {
        df::coord pos(burrow->block_x[i], burrow->block_y[i], burrow->block_z[i]);

        auto block = Maps::getBlock(pos - base);
        if (block)
            pvec->push_back(block);
    }
}

static void destroyBurrowMask(df::block_burrow *mask)
{
    if (!mask) return;

    auto link = mask->link;

    link->prev->next = link->next;
    if (link->next)
        link->next->prev = link->prev;
    delete link;

    delete mask;
}

void Burrows::clearTiles(df::burrow *burrow)
{
    CHECK_NULL_POINTER(burrow);

    df::coord base(world->map.region_x*3,world->map.region_y*3,world->map.region_z);

    for (size_t i = 0; i < burrow->block_x.size(); i++)
    {
        df::coord pos(burrow->block_x[i], burrow->block_y[i], burrow->block_z[i]);

        auto block = Maps::getBlock(pos - base);
        if (!block)
            continue;

        destroyBurrowMask(getBlockMask(burrow, block));
    }

    burrow->block_x.clear();
    burrow->block_y.clear();
    burrow->block_z.clear();
}

df::block_burrow *Burrows::getBlockMask(df::burrow *burrow, df::map_block *block, bool create)
{
    CHECK_NULL_POINTER(burrow);
    CHECK_NULL_POINTER(block);

    int32_t id = burrow->id;
    df::block_burrow_link *prev = &block->block_burrows;
    df::block_burrow_link *link = prev->next;

    for (; link; prev = link, link = link->next)
        if (link->item->id == id)
            return link->item;

    if (create)
    {
        link = new df::block_burrow_link;
        link->item = new df::block_burrow;

        link->item->id = burrow->id;
        link->item->tile_bitmask.clear();
        link->item->link = link;

        link->next = NULL;
        link->prev = prev;
        prev->next = link;

        df::coord base(world->map.region_x*3,world->map.region_y*3,world->map.region_z);
        df::coord pos = base + block->map_pos/16;

        burrow->block_x.push_back(pos.x);
        burrow->block_y.push_back(pos.y);
        burrow->block_z.push_back(pos.z);

        return link->item;
    }

    return NULL;
}

bool Burrows::deleteBlockMask(df::burrow *burrow, df::map_block *block, df::block_burrow *mask)
{
    CHECK_NULL_POINTER(burrow);
    CHECK_NULL_POINTER(block);

    if (!mask)
        return false;

    df::coord base(world->map.region_x*3,world->map.region_y*3,world->map.region_z);
    df::coord pos = base + block->map_pos/16;

    destroyBurrowMask(mask);

    for (size_t i = 0; i < burrow->block_x.size(); i++)
    {
        df::coord cur(burrow->block_x[i], burrow->block_y[i], burrow->block_z[i]);

        if (cur == pos)
        {
            vector_erase_at(burrow->block_x, i);
            vector_erase_at(burrow->block_y, i);
            vector_erase_at(burrow->block_z, i);

            break;
        }
    }

    return true;
}

bool Burrows::isAssignedBlockTile(df::burrow *burrow, df::map_block *block, df::coord2d tile)
{
    CHECK_NULL_POINTER(burrow);

    if (!block) return false;

    auto mask = getBlockMask(burrow, block);

    return mask ? mask->getassignment(tile & 15) : false;
}

bool Burrows::setAssignedBlockTile(df::burrow *burrow, df::map_block *block, df::coord2d tile, bool enable)
{
    CHECK_NULL_POINTER(burrow);

    if (!block) return false;

    auto mask = getBlockMask(burrow, block, enable);

    if (mask)
    {
        mask->setassignment(tile & 15, enable);

        if (!enable && !mask->has_assignments())
            deleteBlockMask(burrow, block, mask);
    }

    return true;
}
