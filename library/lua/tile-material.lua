-- tile-material: Functions to help retrieve the material for a tile.

--[[
Copyright 2015-2016 Milo Christiansen

This software is provided 'as-is', without any express or implied warranty. In
no event will the authors be held liable for any damages arising from the use of
this software.

Permission is granted to anyone to use this software for any purpose, including
commercial applications, and to alter it and redistribute it freely, subject to
the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim
that you wrote the original software. If you use this software in a product, an
acknowledgment in the product documentation would be appreciated but is not
required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.
]]

local _ENV = mkmodule("tile-material")

--[====[
tile-material
=============

This module contains functions for finding the material of a tile.

There is a function that will find the material of the tile based on it's type (in other words
it will return the material DF is using for that tile), and there are functions that will attempt
to return only a certain class of materials.

Most users will be most interested in the generic "GetTileMat" function, but the other functions
should be useful in certain cases. For example "GetLayerMat" will always return the material of
the stone (or soil) in the current layer, ignoring any veins or other inclusions.

Some tile types/materials have special behavior with the "GetTileMat" function.

* Open space and other "material-less" tiles (such as semi-molten rock or eerie glowing pits)
  will return nil.
* Ice will return the hard-coded water material ("WATER:NONE").

The specialized functions will return nil if a material of their type is not possible for a tile.
For example calling "GetVeinMat" for a tile that does not have (and has never had) a mineral vein
will always return nil.

There are two functions for dealing with constructions, one to get the material of the construction
and one that gets the material of the tile the construction was built over.

All the functions take coordinates as either three arguments (x, y, z) or one argument containing
a table with numeric x, y, and z keys.

I am not sure how caved in tiles are handled, but after some quick testing it appears that the
game creates mineral veins for them. I am not 100% sure if these functions will reliably work
with all caved in tiles, but I can confirm that they do in at least some cases...
]====]

-- Since there isn't any consistent style for module documentation I documented every function in
-- the style used by GoDoc (which is what I am most used to).

-- Internal
local function prepPos(x, y, z)
    if x ~= nil and y == nil and z == nil then
        if type(x) ~= "table" or type(x.x) ~= "number" or type(x.y) ~= "number" or type(x.z) ~= "number" or x.x == -30000 then
            error "Invalid coordinate argument(s)."
        end
        return x
    else
        if type(x) ~= "number" or type(y) ~= "number" or type(z) ~= "number" or x == -30000 then
            error "Invalid coordinate argument(s)."
        end
        return {x = x, y = y, z = z}
    end
end

-- Internal
local function fixedMat(id)
    local mat = dfhack.matinfo.find(id)
    return function(x, y, z)
        return mat
    end
end

-- BasicMats is a matspec table to pass to GetTileMatSpec or GetTileTypeMat. This particular
-- matspec table covers the common case of returning plant materials for plant tiles and other
-- materials for the remaining tiles.
BasicMats = {
    [df.tiletype_material.AIR] = nil, -- Empty
    [df.tiletype_material.SOIL] = GetLayerMat,
    [df.tiletype_material.STONE] = GetLayerMat,
    [df.tiletype_material.FEATURE] = GetFeatureMat,
    [df.tiletype_material.LAVA_STONE] = GetLavaStone,
    [df.tiletype_material.MINERAL] = GetVeinMat,
    [df.tiletype_material.FROZEN_LIQUID] = fixedMat("WATER:NONE"),
    [df.tiletype_material.CONSTRUCTION] = GetConstructionMat,
    [df.tiletype_material.GRASS_LIGHT] = GetGrassMat,
    [df.tiletype_material.GRASS_DARK] = GetGrassMat,
    [df.tiletype_material.GRASS_DRY] = GetGrassMat,
    [df.tiletype_material.GRASS_DEAD] = GetGrassMat,
    [df.tiletype_material.PLANT] = GetShrubMat,
    [df.tiletype_material.HFS] = nil, -- Eerie Glowing Pit
    [df.tiletype_material.CAMPFIRE] = GetLayerMat,
    [df.tiletype_material.FIRE] = GetLayerMat,
    [df.tiletype_material.ASHES] = GetLayerMat,
    [df.tiletype_material.MAGMA] = nil, -- SMR
    [df.tiletype_material.DRIFTWOOD] = GetLayerMat,
    [df.tiletype_material.POOL] = GetLayerMat,
    [df.tiletype_material.BROOK] = GetLayerMat,
    [df.tiletype_material.ROOT] = GetLayerMat,
    [df.tiletype_material.TREE] = GetTreeMat,
    [df.tiletype_material.MUSHROOM] = GetTreeMat,
    [df.tiletype_material.UNDERWORLD_GATE] = nil, -- I guess this is for the gates found in vaults?
}

-- NoPlantMats is a matspec table to pass to GetTileMatSpec or GetTileTypeMat. This particular
-- matspec table will ignore plants, returning layer materials (or nil for trees) instead.
NoPlantMats = {
    [df.tiletype_material.SOIL] = GetLayerMat,
    [df.tiletype_material.STONE] = GetLayerMat,
    [df.tiletype_material.FEATURE] = GetFeatureMat,
    [df.tiletype_material.LAVA_STONE] = GetLavaStone,
    [df.tiletype_material.MINERAL] = GetVeinMat,
    [df.tiletype_material.FROZEN_LIQUID] = fixedMat("WATER:NONE"),
    [df.tiletype_material.CONSTRUCTION] = GetConstructionMat,
    [df.tiletype_material.GRASS_LIGHT] = GetLayerMat,
    [df.tiletype_material.GRASS_DARK] = GetLayerMat,
    [df.tiletype_material.GRASS_DRY] = GetLayerMat,
    [df.tiletype_material.GRASS_DEAD] = GetLayerMat,
    [df.tiletype_material.PLANT] = GetLayerMat,
    [df.tiletype_material.CAMPFIRE] = GetLayerMat,
    [df.tiletype_material.FIRE] = GetLayerMat,
    [df.tiletype_material.ASHES] = GetLayerMat,
    [df.tiletype_material.DRIFTWOOD] = GetLayerMat,
    [df.tiletype_material.POOL] = GetLayerMat,
    [df.tiletype_material.BROOK] = GetLayerMat,
    [df.tiletype_material.ROOT] = GetLayerMat,
}

-- OnlyPlantMats is a matspec table to pass to GetTileMatSpec or GetTileTypeMat. This particular
-- matspec table will return nil for any non-plant tile. Plant tiles return the plant material.
OnlyPlantMats = {
    [df.tiletype_material.GRASS_LIGHT] = GetGrassMat,
    [df.tiletype_material.GRASS_DARK] = GetGrassMat,
    [df.tiletype_material.GRASS_DRY] = GetGrassMat,
    [df.tiletype_material.GRASS_DEAD] = GetGrassMat,
    [df.tiletype_material.PLANT] = GetShrubMat,
    [df.tiletype_material.TREE] = GetTreeMat,
    [df.tiletype_material.MUSHROOM] = GetTreeMat,
}

-- GetLayerMat returns the layer material for the given tile.
-- AFAIK this will never return nil.
function GetLayerMat(x, y, z)
    local pos = prepPos(x, y, z)

    local region_info = dfhack.maps.getRegionBiome(dfhack.maps.getTileBiomeRgn(pos))
    local map_block = dfhack.maps.ensureTileBlock(pos)

    local biome = df.world_geo_biome.find(region_info.geo_index)

    local layer_index = map_block.designation[pos.x%16][pos.y%16].geolayer_index
    local layer_mat_index = biome.layers[layer_index].mat_index

    return dfhack.matinfo.decode(0, layer_mat_index)
end

-- GetLavaStone returns the biome lava stone material (generally obsidian).
function GetLavaStone(x, y, z)
    local pos = prepPos(x, y, z)

    local regions = df.global.world.world_data.region_details

    local rx, ry = dfhack.maps.getTileBiomeRgn(pos)

    for _, region in ipairs(regions) do
        if region.pos.x == rx and region.pos.y == ry then
            return dfhack.matinfo.decode(0, region.lava_stone)
        end
    end
    return nil
end

-- GetVeinMat returns the vein material of the given tile or nil if the tile has no veins.
-- Multiple veins in one tile should be handled properly (smallest vein type, last in the list wins,
-- which seems to be the rule DF uses).
function GetVeinMat(x, y, z)
    local pos = prepPos(x, y, z)

    local map_block = dfhack.maps.ensureTileBlock(pos)

    local events = {}
    for _, event in ipairs(map_block.block_events) do
        if getmetatable(event) == "block_square_event_mineralst" then
            if dfhack.maps.getTileAssignment(event.tile_bitmask, pos.x, pos.y) then
                table.insert(events, event)
            end
        end
    end

    if #events == 0 then
        return nil
    end

    local event_priority = function(event)
        if event.flags.cluster then
            return 1
        elseif event.flags.vein then
            return 2
        elseif event.flags.cluster_small then
            return 3
        elseif event.flags.cluster_one then
            return 4
        else
            return 5
        end
    end

    local priority = events[1]
    for _, event in ipairs(events) do
        if event_priority(event) >= event_priority(priority) then
            priority = event
        end
    end

    return dfhack.matinfo.decode(0, priority.inorganic_mat)
end

-- GetConstructionMat returns the material of the construction at the given tile or nil if the tile
-- has no construction.
function GetConstructionMat(x, y, z)
    local pos = prepPos(x, y, z)

    for _, construction in ipairs(df.global.world.constructions) do
        if construction.pos.x == pos.x and construction.pos.y == pos.y and construction.pos.z == pos.z then
            return dfhack.matinfo.decode(construction)
        end
    end
    return nil
end

-- GetConstructOriginalTileMat returns the material of the tile under the construction at the given
-- tile or nil if the tile has no construction.
function GetConstructOriginalTileMat(x, y, z)
    local pos = prepPos(x, y, z)

    for _, construction in ipairs(df.global.world.constructions) do
        if construction.pos.x == pos.x and construction.pos.y == pos.y and construction.pos.z == pos.z then
            return GetTileTypeMat(construction.original_tile, BasicMats, pos)
        end
    end
    return nil
end

-- GetTreeMat returns the material of the tree at the given tile or nil if the tile does not have a
-- tree or giant mushroom.
-- Currently roots are ignored.
function GetTreeMat(x, y, z)
    local pos = prepPos(x, y, z)

    local function coordInTree(pos, tree)
        local x1 = tree.pos.x - math.floor(tree.tree_info.dim_x / 2)
        local x2 = tree.pos.x + math.floor(tree.tree_info.dim_x / 2)
        local y1 = tree.pos.y - math.floor(tree.tree_info.dim_y / 2)
        local y2 = tree.pos.y + math.floor(tree.tree_info.dim_y / 2)
        local z1 = tree.pos.z
        local z2 = tree.pos.z + tree.tree_info.body_height

        if not ((pos.x >= x1 and pos.x <= x2) and (pos.y >= y1 and pos.y <= y2) and (pos.z >= z1 and pos.z <= z2)) then
            return false
        end

        return not tree.tree_info.body[pos.z - tree.pos.z]:_displace((pos.y - y1) * tree.tree_info.dim_x + (pos.x - x1)).blocked
    end

    for _, tree in ipairs(df.global.world.plants.all) do
        if tree.tree_info ~= nil then
            if coordInTree(pos, tree) then
                return dfhack.matinfo.decode(419, tree.material)
            end
        end
    end
    return nil
end

-- GetShrubMat returns the material of the shrub at the given tile or nil if the tile does not
-- contain a shrub or sapling.
function GetShrubMat(x, y, z)
    local pos = prepPos(x, y, z)

    for _, shrub in ipairs(df.global.world.plants.all) do
        if shrub.tree_info == nil then
            if shrub.pos.x == pos.x and shrub.pos.y == pos.y and shrub.pos.z == pos.z then
                return dfhack.matinfo.decode(419, shrub.material)
            end
        end
    end
    return nil
end

-- GetGrassMat returns the material of the grass at the given tile or nil if the tile is not
-- covered in grass.
function GetGrassMat(x, y, z)
    local pos = prepPos(x, y, z)

    local map_block = dfhack.maps.ensureTileBlock(pos)

    for _, event in ipairs(map_block.block_events) do
        if getmetatable(event) == "block_square_event_grassst" then
            local amount = event.amount[pos.x%16][pos.y%16]
            if amount > 0 then
                return df.plant_raw.find(event.plant_index).material
            end
        end
    end
    return nil
end

-- GetFeatureMat returns the material of the feature (adamantine tube, underworld surface, etc) at
-- the given tile or nil if the tile is not made of a feature stone.
function GetFeatureMat(x, y, z)
    local pos = prepPos(x, y, z)

    local map_block = dfhack.maps.ensureTileBlock(pos)

    if df.tiletype.attrs[map_block.tiletype[pos.x%16][pos.y%16]].material ~= df.tiletype_material.FEATURE then
        return nil
    end

    if map_block.designation[pos.x%16][pos.y%16].feature_local then
        -- adamantine tube, etc
        for id, idx in ipairs(df.global.world.features.feature_local_idx) do
            if idx == map_block.local_feature then
                return dfhack.matinfo.decode(df.global.world.features.map_features[id])
            end
        end
    elseif map_block.designation[pos.x%16][pos.y%16].feature_global then
        -- cavern, magma sea, underworld, etc
        for id, idx in ipairs(df.global.world.features.feature_global_idx) do
            if idx == map_block.global_feature then
                return dfhack.matinfo.decode(df.global.world.features.map_features[id])
            end
        end
    end

    return nil
end

-- GetTileMat will return the material of the specified tile as determined by its tile type and the
-- world geology data, etc.
-- The returned material should exactly match the material reported by DF except in cases where is
-- is impossible to get a material.
-- This is equivalent to calling GetTileMatSpec with the BasicMats matspec table.
function GetTileMat(x, y, z)
    return GetTileMatSpec(BasicMats, x, y, z)
end

-- GetTileMatSpec is exactly like GetTileMat except you may specify an explicit matspec table.
--
-- "matspec" tables are simply tables with tiletype material classes as keys and functions
-- taking a coordinate table and returning a material as values. These tables are used to
-- determine how a specific material for a given tiletype material classification is determined.
-- Any tiletype material class that is unset (left nil) in a matspec table will result in tiles
-- of that type returning nil for their material.
function GetTileMatSpec(matspec, x, y, z)
    local pos = prepPos(x, y, z)

    local typ = dfhack.maps.getTileType(pos)
    if typ == nil then
        return nil
    end

    return GetTileTypeMat(typ, matspec, pos)
end

-- GetTileTypeMat returns the material of the given tile assuming it is the given tiletype.
--
-- Use this function when you want to check to see what material a given tile would be if it
-- was a specific tiletype. For example you can check to see if the tile used to be part of
-- a mineral vein or similar. Note that you can do the same basic thing by calling the individual
-- material finders directly, but this is sometimes simpler.
--
-- Unless the tile could be the given type this function will probably return nil.
function GetTileTypeMat(typ, matspec, x, y, z)
    local pos = prepPos(x, y, z)

    local type_mat = df.tiletype.attrs[typ].material

    local mat_getter = matspec[type_mat]
    if mat_getter == nil then
        return nil
    end
    return mat_getter(pos)
end

return _ENV
