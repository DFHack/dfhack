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
#include "TileTypes.h"
#include "Export.h"

#include <cassert>
#include <map>

using namespace DFHack;

const int NUM_TILETYPES = 1+(int)ENUM_LAST_ITEM(tiletype);
const int NUM_MATERIALS = 1+(int)ENUM_LAST_ITEM(tiletype_material);
const int NUM_CVTABLES = 1+(int)tiletype_material::CONSTRUCTION;

typedef std::map<df::tiletype_variant, df::tiletype> T_VariantMap;
typedef std::map<std::string, T_VariantMap> T_DirectionMap;
typedef std::map<df::tiletype_special, T_DirectionMap> T_SpecialMap;
typedef std::map<df::tiletype_shape, T_SpecialMap> T_ShapeMap;
typedef T_ShapeMap T_MaterialMap[NUM_MATERIALS];

static bool tables_ready = false;
static T_MaterialMap tile_table;
static df::tiletype tile_to_mat[NUM_CVTABLES][NUM_TILETYPES];

static df::tiletype find_match(
    df::tiletype_material mat, df::tiletype_shape shape, df::tiletype_special special,
    std::string dir, df::tiletype_variant variant, bool warn
) {
    using namespace df::enums::tiletype_shape;
    using namespace df::enums::tiletype_special;

    if (mat < 0 || mat >= NUM_MATERIALS)
        return tiletype::Void;

    auto &sh_map = tile_table[mat];

    if (!sh_map.count(shape))
    {
        if (warn)
        {
            fprintf(
                stderr, "NOTE: No shape %s in %s.\n",
                enum_item_key(shape).c_str(), enum_item_key(mat).c_str()
            );
        }

        switch (shape)
        {
            case BROOK_BED:
                if (sh_map.count(FORTIFICATION)) { shape = FORTIFICATION; break; }

            case FORTIFICATION:
                if (sh_map.count(WALL)) { shape = WALL; break; }
                return tiletype::Void;

            case BROOK_TOP:
            case BOULDER:
            case PEBBLES:
                if (sh_map.count(FLOOR)) { shape = FLOOR; break; }
                return tiletype::Void;

            default:
                return tiletype::Void;
        };
    }

    auto &sp_map = sh_map[shape];

    if (!sp_map.count(special))
    {
        if (warn)
        {
            fprintf(
                stderr, "NOTE: No special %s in %s:%s.\n",
                enum_item_key(special).c_str(), enum_item_key(mat).c_str(),
                enum_item_key(shape).c_str()
            );
        }

        switch (special)
        {
            case TRACK:
                if (sp_map.count(SMOOTH)) {
                    special = SMOOTH; break;
                }

            case df::enums::tiletype_special::NONE:
            case NORMAL:
            case SMOOTH:
            case WET:
            case FURROWED:
            case RIVER_SOURCE:
            case WATERFALL:
            case WORN_1:
            case WORN_2:
            case WORN_3:
                if (sp_map.count(NORMAL)) {
                    special = NORMAL; break;
                }
                if (sp_map.count(df::enums::tiletype_special::NONE)) {
                    special = df::enums::tiletype_special::NONE; break;
                }
                // For targeting construction
                if (sp_map.count(SMOOTH)) {
                    special = SMOOTH; break;
                }

            default:
                return tiletype::Void;
        }
    }

    auto &dir_map = sp_map[special];

    if (!dir_map.count(dir))
    {
        if (warn)
        {
            fprintf(
                stderr, "NOTE: No direction '%s' in %s:%s:%s.\n",
                dir.c_str(), enum_item_key(mat).c_str(),
                enum_item_key(shape).c_str(), enum_item_key(special).c_str()
            );
        }

        if (dir_map.count("--------"))
            dir = "--------";
        else if (dir_map.count("NSEW"))
            dir = "NSEW";
        else if (dir_map.count("N-S-W-E-"))
            dir = "N-S-W-E-";
        else
            dir = dir_map.begin()->first;
    }

    auto &var_map = dir_map[dir];

    if (!var_map.count(variant))
    {
        if (warn)
        {
            fprintf(
                stderr, "NOTE: No variant '%s' in %s:%s:%s:%s.\n",
                enum_item_key(variant).c_str(), enum_item_key(mat).c_str(),
                enum_item_key(shape).c_str(), enum_item_key(special).c_str(), dir.c_str()
            );
        }

        variant = var_map.begin()->first;
    }

    return var_map[variant];
}

static void init_tables()
{
    tables_ready = true;
    memset(tile_to_mat, 0, sizeof(tile_to_mat));

    // Index tile types
    FOR_ENUM_ITEMS(tiletype, tt)
    {
        auto &attrs = df::enum_traits<df::tiletype>::attrs(tt);
        if (attrs.material < 0)
            continue;

        tile_table[attrs.material][attrs.shape][attrs.special][attrs.direction][attrs.variant] = tt;

        if (isCoreMaterial(attrs.material))
        {
            assert(attrs.material < NUM_CVTABLES);
            tile_to_mat[attrs.material][tt] = tt;
        }
    }

    // Build mapping of everything to STONE and back
    FOR_ENUM_ITEMS(tiletype, tt)
    {
        auto &attrs = df::enum_traits<df::tiletype>::attrs(tt);
        if (!isCoreMaterial(attrs.material))
            continue;

        if (attrs.material != tiletype_material::STONE)
        {
            df::tiletype ttm = find_match(
                tiletype_material::STONE,
                attrs.shape, attrs.special, attrs.direction, attrs.variant,
                isStoneMaterial(attrs.material)
            );

            tile_to_mat[tiletype_material::STONE][tt] = ttm;

            if (ttm == tiletype::Void)
                fprintf(stderr, "No match for tile %s in STONE.\n",
                        enum_item_key(tt).c_str());
        }
        else
        {
            FOR_ENUM_ITEMS(tiletype_material, mat)
            {
                if (!isCoreMaterial(mat) || mat == attrs.material)
                    continue;

                df::tiletype ttm = find_match(
                    mat,
                    attrs.shape, attrs.special, attrs.direction, attrs.variant,
                    isStoneMaterial(mat)
                );

                tile_to_mat[mat][tt] = ttm;

                if (ttm == tiletype::Void)
                    fprintf(stderr, "No match for tile %s in %s.\n",
                            enum_item_key(tt).c_str(), enum_item_key(mat).c_str());
            }
        }
    }

    // Transitive closure via STONE
    FOR_ENUM_ITEMS(tiletype_material, mat)
    {
        if (!isCoreMaterial(mat) || mat == tiletype_material::STONE)
            continue;

        FOR_ENUM_ITEMS(tiletype, tt)
        {
            if (!tt || tile_to_mat[mat][tt])
                continue;

            auto stone = tile_to_mat[tiletype_material::STONE][tt];
            if (stone)
                tile_to_mat[mat][tt] = tile_to_mat[mat][stone];
        }
    }
}

df::tiletype DFHack::matchTileMaterial(df::tiletype source, df::tiletype_material tmat)
{
    if (!isCoreMaterial(tmat) || !source || source >= NUM_TILETYPES)
        return tiletype::Void;

    if (!tables_ready)
        init_tables();

    return tile_to_mat[tmat][source];
}

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
                
                // If the material is a plant, consider soil an acceptable alternative
                if (cur_material == tiletype_material::PLANT and tileMaterial(tt) == tiletype_material::SOIL) {
                    value |= 2;
                }

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
