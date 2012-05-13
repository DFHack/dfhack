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
#include "TileTypes.h"
#include "Export.h"

namespace DFHack
{
    df::tiletype findSimilarTileType (const df::tiletype sourceTileType, const df::tiletype_shape tshape)
    {
        df::tiletype match = tiletype::Void;
        int value = 0, matchv = 0;

        const df::tiletype_shape cur_shape = tileShape(sourceTileType);
        const df::tiletype_material cur_material = tileMaterial(sourceTileType);
        const df::tiletype_special cur_special = tileSpecial(sourceTileType);
        const df::tiletype_variant cur_variant = tileVariant(sourceTileType);
        const TileDirection cur_direction = tileDirection(sourceTileType);

        //Shortcut.
        //If the current tile is already a shape match, leave.
        if (tshape == cur_shape)
            return sourceTileType;

        #ifdef assert
        assert(is_valid_enum_item(sourceTileType));
        #endif

        // Special case for smooth pillars.
        // When you want a smooth wall, no need to search for best match.  Just use a pillar instead.
        // Choosing the right direction would require knowing neighbors.

        if ((tshape == tiletype_shape::WALL) && ((cur_special == tiletype_special::SMOOTH) || (cur_material == tiletype_material::CONSTRUCTION)))
        {
            switch (cur_material)
            {
            case tiletype_material::CONSTRUCTION:
                return tiletype::ConstructedPillar;
            case tiletype_material::FROZEN_LIQUID:
                return tiletype::FrozenPillar;
            case tiletype_material::MINERAL:
                return tiletype::MineralPillar;
            case tiletype_material::FEATURE:
                return tiletype::FeaturePillar;
            case tiletype_material::LAVA_STONE:
                return tiletype::LavaPillar;
            case tiletype_material::STONE:
                return tiletype::StonePillar;
            default:
                break;
            }
        }

        // Run through until perfect match found or hit end.
        FOR_ENUM_ITEMS(tiletype, tt)
        {
            if (value == (8|4|1))
                break;
            if (tileShape(tt) == tshape)
            {
                // Special flag match is mandatory, but only if it might possibly make a difference
                if (tileSpecial(tt) != tiletype_special::NONE && cur_special != tiletype_special::NONE && tileSpecial(tt) != cur_special)
                    continue;

                // Special case for constructions.
                // Never turn a construction into a non-contruction.
                if ((cur_material == tiletype_material::CONSTRUCTION) && (tileMaterial(tt) != cur_material))
                    continue;

                value = 0;
                //Material is high-value match
                if (cur_material == tileMaterial(tt))
                    value |= 8;

                // Direction is medium value match
                if (cur_direction == tileDirection(tt))
                    value |= 4;

                // Variant is low-value match
                if (cur_variant == tileVariant(tt))
                    value |= 1;

                // Check value against last match.
                if (value > matchv)
                {
                    match = tt;
                    matchv = value;
                }
            }
        }

        // If the selected tile has a variant, then pick a random one
        match = findRandomVariant(match);
        if (match)
            return match;
        return sourceTileType;
    }
    df::tiletype findRandomVariant (const df::tiletype tile)
    {
        if (tileVariant(tile) == tiletype_variant::NONE)
            return tile;
        std::vector<df::tiletype> matches;
        FOR_ENUM_ITEMS(tiletype, tt)
        {
            if (tileShape(tt) == tileShape(tile) &&
                tileMaterial(tt) == tileMaterial(tile) &&
                tileSpecial(tt) == tileSpecial(tile))
                matches.push_back(tt);
        }
        return matches[rand() % matches.size()];
    }
}
