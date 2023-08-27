-- creates an item of a given type and material
--author expwnent
--@module=true

local argparse = require('argparse')
local utils = require('utils')

local no_quality_item_types = utils.invert{
    'BAR',
    'SMALLGEM',
    'BLOCKS',
    'ROUGH',
    'BOULDER',
    'WOOD',
    'CORPSE',
    'CORPSEPIECE',
    'REMAINS',
    'MEAT',
    'FISH',
    'FISH_RAW',
    'VERMIN',
    'PET',
    'SEEDS',
    'PLANT',
    'SKIN_TANNED',
    'PLANT_GROWTH',
    'THREAD',
    'DRINK',
    'POWDER_MISC',
    'CHEESE',
    'FOOD',
    'LIQUID_MISC',
    'COIN',
    'GLOB',
    'ROCK',
    'EGG',
    'BRANCH',
}

local CORPSE_PIECES = utils.invert{'BONE', 'SKIN', 'CARTILAGE', 'TOOTH', 'NERVE', 'NAIL', 'HORN', 'HOOF', 'CHITIN',
    'SHELL', 'IVORY', 'SCALE'}
local HAIR_PIECES = utils.invert{'HAIR', 'EYEBROW', 'EYELASH', 'MOUSTACHE', 'CHIN_WHISKERS', 'SIDEBURNS'}
local LIQUID_PIECES = utils.invert{'BLOOD', 'PUS', 'VENOM', 'SWEAT', 'TEARS', 'SPIT', 'MILK'}

local function moveToContainer(item, creator, container_type)
    local containerMat = dfhack.matinfo.find('PLANT_MAT:NETHER_CAP:WOOD')
    if not containerMat then
        for i,n in ipairs(df.global.world.raws.plants.all) do
            if n.flags.TREE then
                containerMat = dfhack.matinfo.find('PLANT_MAT:' .. n.id .. ':WOOD')
            end
            if containerMat then break end
        end
    end
    local bucketType = dfhack.items.findType(container_type .. ':NONE')
    local bucket = df.item.find(dfhack.items.createItem(bucketType, -1, containerMat.type, containerMat.index, creator))
    dfhack.items.moveToContainer(item, bucket)
    return bucket
end

-- this part was written by four rabbits in a trenchcoat (ppaawwll)
local function createCorpsePiece(creator, bodypart, partlayer, creatureID, casteID, generic)
    -- (partlayer is also used to determine the material if we're spawning a "generic" body part (i'm just lazy lol))
    creatureID = tonumber(creatureID)
    -- get the actual raws of the target creature
    local creatorRaceRaw = df.creature_raw.find(creatureID)
    local wholePart = false
    casteID = tonumber(casteID)
    bodypart = tonumber(bodypart)
    partlayer = tonumber(partlayer)
    -- somewhat similar to the bodypart variable below, a value of -1 here means that the user wants to spawn a whole body part. we set the partlayer to 0 (outermost) because the specific layer isn't important, and we're spawning them all anyway. if it's a generic corpsepiece we ignore it, as it gets added to anyway below (we can't do it below because between here and there there's lines that reference the part layer
    if partlayer == -1 and not generic then
        partlayer = 0
        wholePart = true
    end
    -- get body info for easy reference
    local creatorBody = creatorRaceRaw.caste[casteID].body_info
    local layerName
    local layerMat = 'BONE'
    local tissueID
    local liquid = false
    -- in the hackWish function, the bodypart variable is initialized to -1, which isn't changed if the spawned item is a corpse
    local isCorpse = bodypart == -1 and not generic
    if not generic and not isCorpse then -- if we have a specified body part and layer, figure all the stuff out about that
        -- store the tissue id of the specific layer we selected
        tissueID = tonumber(creatorBody.body_parts[bodypart].layers[partlayer].tissue_id)
        local mats = {}
        -- get the material name from the material itself
        for i in string.gmatch(dfhack.matinfo.getToken(creatorRaceRaw.tissue[tissueID].mat_type, creatureID), '([^:]+)') do
            table.insert(mats, i)
        end
        layerMat = mats[3]
        layerName = creatorBody.body_parts[bodypart].layers[partlayer].layer_name
    elseif not isCorpse then -- otherwise, figure out the mat name from the dual-use partlayer argument
        layerMat = creatorRaceRaw.material[partlayer].id
        layerName = layerMat
    end
    -- default is MEAT, so if anything else fails to change it to something else, we know that the body layer is a meat item
    local item_type = 'MEAT'
    -- get race name and layer name, both for finding the item material, and the latter for determining the corpsepiece flags to set
    local raceName = string.upper(creatorRaceRaw.creature_id)
    -- every key is a valid non-hair corpsepiece, so if we try to index a key that's not on the table, we don't have a non-hair corpsepiece
    -- we do the same as above but with hair
    -- if the layer is fat, spawn a glob of fat and DON'T check for other layer types
    if layerName == 'FAT' then
        item_type = 'GLOB'
    elseif CORPSE_PIECES[layerName] or HAIR_PIECES[layerName] then -- check if hair
        item_type = 'CORPSEPIECE'
    elseif LIQUID_PIECES[layerName] then
        item_type = 'LIQUID_MISC'
        liquid = true
    end
    if isCorpse then
        item_type = 'CORPSE'
        generic = true
    end
    local itemType = dfhack.items.findType(item_type .. ':NONE')
    local itemSubtype = dfhack.items.findSubtype(item_type .. ':NONE')
    local material = 'CREATURE_MAT:' .. raceName .. ':' .. layerMat
    local materialInfo = dfhack.matinfo.find(material)
    local item_id = dfhack.items.createItem(itemType, itemSubtype, materialInfo['type'], materialInfo.index, creator)
    local item = df.item.find(item_id)
    -- if the item type is a corpsepiece, we know we have one, and then go on to set the appropriate flags
    if item_type == 'CORPSEPIECE' then
        if layerName == 'BONE' then -- check if bones
            item.corpse_flags.bone = true
            item.material_amount.Bone = 1
        elseif layerName == 'SKIN' then -- check if skin/leather
            item.corpse_flags.leather = true
            item.material_amount.Leather = 1
            -- elseif layerName == "CARTILAGE" then -- check if cartilage (NO SPECIAL FLAGS)
        elseif layerName == 'HAIR' then -- check if hair (simplified from before)
            item.corpse_flags.hair_wool = true
            item.material_amount.HairWool = 1
            if materialInfo.material.flags.YARN then
                item.corpse_flags.yarn = true
                item.material_amount.Yarn = 1
            end
        elseif layerName == 'TOOTH' or layerName == 'IVORY' then -- check if tooth
            item.corpse_flags.tooth = true
            item.material_amount.Tooth = 1
        elseif layerName == 'NERVE' then    -- check if nervous tissue
            item.corpse_flags.skull1 = true -- apparently "skull1" is supposed to be named "rots/can_rot"
            item.corpse_flags.separated_part = true
            -- elseif layerName == "NAIL" then -- check if nail (NO SPECIAL FLAGS)
        elseif layerName == 'HORN' or layerName == 'HOOF' then -- check if nail
            item.corpse_flags.horn = true
            item.material_amount.Horn = 1
        elseif layerName == 'SHELL' then
            item.corpse_flags.shell = true
            item.material_amount.Shell = 1
        end
        -- checking for skull
        if not generic and not isCorpse and creatorBody.body_parts[bodypart].token == 'SKULL' then
            item.corpse_flags.skull2 = true
        end
    end
    local matType
    -- figure out which material type the material is (probably a better way of doing this but whatever)
    for i in pairs(creatorRaceRaw.tissue) do
        if creatorRaceRaw.tissue[i].tissue_material_str[1] == layerMat then
            matType = creatorRaceRaw.tissue[i].mat_type
        end
    end
    if item_type == 'CORPSEPIECE' or item_type == 'CORPSE' then
        --referencing the source unit for, material, relation purposes???
        item.race = creatureID
        item.normal_race = creatureID
        item.normal_caste = casteID
        -- usually the first two castes are for the creature's sex, so we set the item's sex to the caste if both the creature has one and it's a valid sex id (0 or 1)
        if casteID < 2 and #(creatorRaceRaw.caste) > 1 then
            item.sex = casteID
        else
            item.sex = -1 -- it
        end
        -- on a dwarf tissue index 3 (bone) is 22, but this is not always the case for all creatures, so we get the mat_type of index 3 instead
        -- here we also set the actual referenced creature material of the corpsepiece
        item.bone1.mat_type = matType
        item.bone1.mat_index = creatureID
        item.bone2.mat_type = matType
        item.bone2.mat_index = creatureID
        -- skin (and presumably other parts) use body part modifiers for size or amount
        for i = 0,200 do                          -- fuck it this works
            -- inserts
            item.body.bp_modifiers:insert('#', 1) --jus,t, set a lot of it to one who cares
        end
        -- copy target creature's relsizes to the item's's body relsizes thing
        for i,n in pairs(creatorBody.body_parts) do
            -- inserts
            item.body.body_part_relsize:insert('#', n.relsize)
            item.body.components.body_part_status:insert(i, creator.body.components.body_part_status[0]) --copy the status of the creator's first part to every body_part_status of the desired creature
            item.body.components.body_part_status[i].missing = true
        end
        for i in pairs(creatorBody.layer_part) do
            -- inserts
            item.body.components.layer_status:insert(i, creator.body.components.layer_status[0]) --copy the layer status of the creator's first layer to every layer_status of the desired creature
            item.body.components.layer_status[i].gone = true
        end
        if item_type == 'CORPSE' then
            item.corpse_flags.unbutchered = true
        end
        if not generic then
            -- keeps the body part that the user selected to spawn the item from
            item.body.components.body_part_status[bodypart].missing = false
            -- restores the selected layer of the selected body part
            item.body.components.layer_status[creatorBody.body_parts[bodypart].layers[partlayer].layer_id].gone = false
        elseif generic then
            for i in pairs(creatorBody.body_parts) do
                for n in pairs(creatorBody.body_parts[i].layers) do
                    if item_type == 'CORPSE' then
                        item.body.components.body_part_status[i].missing = false
                        item.body.components.layer_status[creatorBody.body_parts[i].layers[n].layer_id].gone = false
                    else
                        -- search through the target creature's body parts and bring back every one which has the desired material
                        if creatorRaceRaw.tissue[creatorBody.body_parts[i].layers[n].tissue_id].tissue_material_str[1] == layerMat and creatorBody.body_parts[i].token ~= 'SKULL' and not creatorBody.body_parts[i].flags.SMALL then
                            item.body.components.body_part_status[i].missing = false
                            item.body.components.layer_status[creatorBody.body_parts[i].layers[n].layer_id].gone = false
                            -- save the index of the bone layer to a variable
                        end
                    end
                end
            end
        end
        -- brings back every tissue layer associated with a body part if we're spawning the entire thing
        if wholePart then
            for i in pairs(creatorBody.body_parts[bodypart].layers) do
                item.body.components.layer_status[creatorBody.body_parts[bodypart].layers[i].layer_id].gone = false
                item.corpse_flags.separated_part = true
                item.corpse_flags.unbutchered = true
            end
        end
        -- DO THIS LAST or else the game crashes for some reason
        item.caste = casteID
    end
    if liquid then
        return moveToContainer(item, creator, 'BUCKET')
    end
    return item
end

local function createItem(mat, itemType, quality, creator, description, amount)
    local item = df.item.find(dfhack.items.createItem(itemType[1], itemType[2], mat[1], mat[2], creator))
    local item2 = nil
    assert(item, 'failed to create item')
    local mat_token = dfhack.matinfo.decode(item):getToken()
    quality = math.max(0, math.min(5, quality - 1))
    item:setQuality(quality)
    local item_type = df.item_type[itemType[1]]
    if item_type == 'SLAB' then
        item.description = description
    elseif item_type == 'GLOVES' then
        --create matching gloves
        item:setGloveHandedness(1)
        item2 = df.item.find(dfhack.items.createItem(itemType[1], itemType[2], mat[1], mat[2], creator))
        assert(item2, 'failed to create item')
        item2:setQuality(quality)
        item2:setGloveHandedness(2)
    elseif item_type == 'SHOES' then
        --create matching shoes
        item2 = df.item.find(dfhack.items.createItem(itemType[1], itemType[2], mat[1], mat[2], creator))
        assert(item2, 'failed to create item')
        item2:setQuality(quality)
    end
    if tonumber(amount) > 1 then
        item:setStackSize(amount)
        if item2 then item2:setStackSize(amount) end
    end
    if item_type == 'DRINK' then
        return moveToContainer(item, creator, 'BARREL')
    elseif mat_token == 'WATER' or mat_token == 'LYE' then
        return moveToContainer(item, creator, 'BUCKET')
    end
    return {item, item2}
end

local function get_first_citizen()
    local citizens = dfhack.units.getCitizens()
    if not citizens or not citizens[1] then
        qerror('Could not choose a creator unit. Please select one in the UI')
    end
    return citizens[1]
end

-- returns the list of created items, or nil on error
function hackWish(accessors, opts)
    local unit = accessors.get_unit(opts) or get_first_citizen()
    local qualityok, quality = false, df.item_quality.Ordinary
    local itemok, itemtype, itemsubtype = accessors.get_item_type()
    if not itemok then return end
    local matok, mattype, matindex, casteId, bodypart, partlayerID, corpsepieceGeneric = accessors.get_mat(itemtype, opts)
    if not matok then return end
    if not no_quality_item_types[df.item_type[itemtype]] then
        qualityok, quality = accessors.get_quality()
        if not qualityok then return end
    end
    local description
    if df.item_type[itemtype] == 'SLAB' then
        local descriptionok
        descriptionok, description = accessors.get_description()
        if not descriptionok then return end
    end
    local count = opts.count
    if not count then
        repeat
            local amountok, amount = accessors.get_count()
            if not amountok then return end
            count = tonumber(amount)
        until count
    end
    if not mattype or not itemtype then return end
    if df.item_type.attrs[itemtype].is_stackable then
        return createItem({mattype, matindex}, {itemtype, itemsubtype}, quality, unit, description, count)
    end
    local items = {}
    for _ = 1,count do
        if itemtype == df.item_type.CORPSEPIECE or itemtype == df.item_type.CORPSE then
            table.insert(items, createCorpsePiece(unit, bodypart, partlayerID, matindex, casteId, corpsepieceGeneric))
        else
            for _,item in ipairs(createItem({mattype, matindex}, {itemtype, itemsubtype}, quality, unit, description, 1)) do
                table.insert(items, item)
            end
        end
    end
    return items
end

if dfhack_flags.module then
    return
end

if not dfhack.isMapLoaded() then
    qerror('modtools/create-item needs a loaded map to work')
end

local opts = {}
local positionals = argparse.processArgsGetopt({...}, {
    {'h', 'help', handler = function() opts.help = true end},
    {'u', 'unit', hasArg = true, handler = function(arg) opts.unit = arg end},
    {'i', 'item', hasArg = true, handler = function(arg) opts.item = arg end},
    {'m', 'material', hasArg = true, handler = function(arg) opts.mat = arg end},
    {'t', 'caste', hasArg = true, handler = function(arg) opts.caste = arg end},
    {'q', 'quality', hasArg = true, handler = function(arg) opts.quality = arg end},
    {'d', 'description', hasArg = true, handler = function(arg) opts.description = arg end},
    {
        'c',
        'count',
        hasArg = true,
        handler = function(arg) opts.count = argparse.nonnegativeInt(arg, 'count') end,
    },
    {'p', 'pos', hasArg = true, handler = function(arg) opts.pos = argparse.coords(arg) end},
})

if positionals[1] == 'help' then opts.help = true end
if opts.help then
    print(dfhack.script_help())
    return
end

if opts.unit == '\\LAST' then
    opts.unit = tostring(df.global.unit_next_id - 1)
end

local function get_caste(race_id, caste)
    if not caste then return 0 end
    if tonumber(caste) then return tonumber(caste) end
    caste = caste:lower()
    for i, c in ipairs(df.creature_raw.find(race_id).caste) do
        if caste == tostring(c.caste_id):lower() then return i end
    end
    return 0
end

local accessors = {
    get_unit = function()
        return tonumber(opts.unit) and df.unit.find(tonumber(opts.unit)) or nil
    end,
    get_item_type = function()
        local item_type = dfhack.items.findType(opts.item)
        if item_type == -1 then
            error('invalid item: ' .. tostring(opts.item))
        end
        return true, item_type, dfhack.items.findSubtype(opts.item)
    end,
    get_mat = function(itype)
        local mat_info = dfhack.matinfo.find(opts.mat)
        if not mat_info then
            error('invalid material: ' .. tostring(opts.mat))
        end
        local caste = get_caste(mat_info.index, opts.caste)
        if df.item_type[itype] ~= 'CORPSEPIECE' then
            return true, mat_info['type'], mat_info.index, caste, -1
        end
        -- support only generic corpse pieces for now. this can be extended to support
        -- everything that gui/create-item can produce, but we need more commandline
        -- arguments to provide the info
        return true, -1, mat_info.index, caste, 1, 0, true
    end,
    get_quality = function()
        return true, (tonumber(opts.quality) or df.item_quality.Ordinary) + 1
    end,
    get_description = function()
        return true, opts.description or error('description not specified')
    end,
    get_count = function()
        return true, opts.count or 1
    end,
}

local items = hackWish(accessors, {})
if items and opts.pos then
    for _,item in ipairs(items) do
        dfhack.items.moveToGround(item, opts.pos)
    end
end
