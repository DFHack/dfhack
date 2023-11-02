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
#include "Export.h"
#include "DataDefs.h"
#include "modules/Maps.h"

#include <vector>

/**
 * \defgroup grp_burrows Burrows module and its types
 * @ingroup grp_modules
 */

namespace df
{
    struct unit;
    struct burrow;
    struct block_burrow;
}

namespace DFHack
{
namespace Burrows
{
    DFHACK_EXPORT df::burrow *findByName(std::string name, bool ignore_final_plus = false);

    // Units
    DFHACK_EXPORT void clearUnits(df::burrow *burrow);

    DFHACK_EXPORT bool isAssignedUnit(df::burrow *burrow, df::unit *unit);
    DFHACK_EXPORT void setAssignedUnit(df::burrow *burrow, df::unit *unit, bool enable);

    // Tiles
    DFHACK_EXPORT void clearTiles(df::burrow *burrow);

    DFHACK_EXPORT void listBlocks(std::vector<df::map_block*> *pvec, df::burrow *burrow);

    DFHACK_EXPORT bool isAssignedBlockTile(df::burrow *burrow, df::map_block *block, df::coord2d tile);
    DFHACK_EXPORT bool setAssignedBlockTile(df::burrow *burrow, df::map_block *block, df::coord2d tile, bool enable);

    inline bool isAssignedTile(df::burrow *burrow, df::coord tile) {
        return isAssignedBlockTile(burrow, Maps::getTileBlock(tile), tile);
    }
    inline bool setAssignedTile(df::burrow *burrow, df::coord tile, bool enable) {
        return setAssignedBlockTile(burrow, Maps::getTileBlock(tile), tile, enable);
    }

    DFHACK_EXPORT df::block_burrow *getBlockMask(df::burrow *burrow, df::map_block *block, bool create = false);
    DFHACK_EXPORT bool deleteBlockMask(df::burrow *burrow, df::map_block *block, df::block_burrow *mask);

    inline bool deleteBlockMask(df::burrow *burrow, df::map_block *block) {
        return deleteBlockMask(burrow, block, getBlockMask(burrow, block));
    }
}
}
