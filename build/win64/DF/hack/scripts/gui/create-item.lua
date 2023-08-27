-- A gui-based item creation script.
-- author Putnam
-- edited by expwnent

local argparse = require('argparse')
local createitem = reqscript('modtools/create-item')
local eventful = require('plugins.eventful')
local script = require('gui.script')

local function getGenderString(gender)
    local sym = df.pronoun_type.attrs[gender].symbol
    if not sym then return '' end
    return '(' .. sym .. ')'
end

local function usesCreature(itemtype)
    local typesThatUseCreatures = {
        REMAINS = true,
        FISH = true,
        FISH_RAW = true,
        VERMIN = true,
        PET = true,
        EGG = true,
        CORPSE = true,
        CORPSEPIECE = true,
    }
    return typesThatUseCreatures[df.item_type[itemtype]]
end

local function getCreatureList()
    local crList = {}
    for k, cr in ipairs(df.global.world.raws.creatures.alphabetic) do
        for kk, ca in ipairs(cr.caste) do
            local str = ca.caste_name[0]
            str = str .. ' ' .. getGenderString(ca.sex)
            table.insert(crList, { str, nil, ca })
        end
    end
    return crList
end

local function getCreaturePartList(creatureID, casteID)
    local crpList = { { 'generic' } }
    for k, crp in ipairs(df.global.world.raws.creatures.all[creatureID].caste[casteID].body_info.body_parts) do
        local str = crp.name_singular[0][0]
        table.insert(crpList, { str })
    end
    return crpList
end

local function getCreaturePartLayerList(creatureID, casteID, partID)
    local crplList = { { 'whole' } }
    for k, crpl in ipairs(df.global.world.raws.creatures.all[creatureID].caste[casteID].body_info.body_parts[partID].layers) do
        local str = crpl.layer_name
        table.insert(crplList, { str })
    end
    return crplList
end

local function getCreatureMaterialList(creatureID, casteID)
    local crmList = {}
    for k, crm in ipairs(df.global.world.raws.creatures.all[creatureID].material) do
        local str = crm.id
        table.insert(crmList, { str })
    end
    return crmList
end

local function getCreatureRaceAndCaste(caste)
    return df.global.world.raws.creatures.list_creature[caste.index],
        df.global.world.raws.creatures.list_caste[caste.index]
end

local function getRestrictiveMatFilter(itemType, opts)
    if opts.unrestricted then return nil end
    local itemTypes = {
        WEAPON = function(mat, parent, typ, idx)
            return (mat.flags.ITEMS_WEAPON or mat.flags.ITEMS_WEAPON_RANGED)
        end,
        AMMO = function(mat, parent, typ, idx)
            return (mat.flags.ITEMS_AMMO)
        end,
        ARMOR = function(mat, parent, typ, idx)
            return (mat.flags.ITEMS_ARMOR or mat.flags.LEATHER)
        end,
        INSTRUMENT = function(mat, parent, typ, idx)
            return (mat.flags.ITEMS_HARD)
        end,
        AMULET = function(mat, parent, typ, idx)
            return (mat.flags.ITEMS_SOFT or mat.flags.ITEMS_HARD)
        end,
        ROCK = function(mat, parent, typ, idx)
            return (mat.flags.IS_STONE)
        end,
        BAR = function(mat, parent, typ, idx)
            return (mat.flags.IS_METAL or mat.flags.SOAP or mat.id == 'COAL' or
                mat.id == 'POTASH' or mat.id == 'ASH' or mat.id == 'PEARLASH')
        end,
        BLOCKS = function(mat, parent, typ, idx)
            return mat.flags.IS_STONE or mat.flags.IS_METAL or mat.flags.IS_GLASS or mat.flags.WOOD
        end,
    }
    for k, v in ipairs { 'GOBLET', 'FLASK', 'TOY', 'RING', 'CROWN', 'SCEPTER', 'FIGURINE', 'TOOL' } do
        itemTypes[v] = itemTypes.INSTRUMENT
    end
    for k, v in ipairs { 'SHOES', 'SHIELD', 'HELM', 'GLOVES' } do
        itemTypes[v] = itemTypes.ARMOR
    end
    for k, v in ipairs { 'EARRING', 'BRACELET' } do
        itemTypes[v] = itemTypes.AMULET
    end
    itemTypes.BOULDER = itemTypes.ROCK
    return itemTypes[df.item_type[itemType]]
end

local function getMatFilter(itemtype, opts)
    local itemTypes = {
        SEEDS = function(mat, parent, typ, idx)
            return mat.flags.SEED_MAT
        end,
        PLANT = function(mat, parent, typ, idx)
            return mat.flags.STRUCTURAL_PLANT_MAT
        end,
        LEAVES = function(mat, parent, typ, idx)
            return mat.flags.LEAF_MAT
        end,
        MEAT = function(mat, parent, typ, idx)
            return mat.flags.MEAT
        end,
        CHEESE = function(mat, parent, typ, idx)
            return (mat.flags.CHEESE_PLANT or mat.flags.CHEESE_CREATURE)
        end,
        LIQUID_MISC = function(mat, parent, typ, idx)
            return mat.id == 'WATER' or mat.id == 'LYE' or mat.flags.LIQUID_MISC_PLANT or
                mat.flags.LIQUID_MISC_CREATURE or mat.flags.LIQUID_MISC_OTHER
        end,
        POWDER_MISC = function(mat, parent, typ, idx)
            return (mat.flags.POWDER_MISC_PLANT or mat.flags.POWDER_MISC_CREATURE)
        end,
        DRINK = function(mat, parent, typ, idx)
            return (mat.flags.ALCOHOL_PLANT or mat.flags.ALCOHOL_CREATURE)
        end,
        GLOB = function(mat, parent, typ, idx)
            return (mat.flags.STOCKPILE_GLOB)
        end,
        WOOD = function(mat, parent, typ, idx)
            return (mat.flags.WOOD)
        end,
        THREAD = function(mat, parent, typ, idx)
            return (mat.flags.THREAD_PLANT)
        end,
        LEATHER = function(mat, parent, typ, idx)
            return (mat.flags.LEATHER)
        end,
    }
    return itemTypes[df.item_type[itemtype]] or getRestrictiveMatFilter(itemtype, opts)
end

local function qualityTable()
    return { { 'None' },
        { '-Well-crafted-' },
        { '+Finely-crafted+' },
        { '*Superior*' },
        { string.char(240) .. 'Exceptional' .. string.char(240) },
        { string.char(15) .. 'Masterwork' .. string.char(15) },
    }
end

local function showItemPrompt(text, item_filter, hide_none)
    require('gui.materials').ItemTypeDialog {
        prompt = text,
        item_filter = item_filter,
        hide_none = hide_none,
        on_select = script.mkresume(true),
        on_cancel = script.mkresume(false),
        on_close = script.qresume(nil),
    }:show()
    return script.wait()
end

local function showMaterialPrompt(title, prompt, filter, inorganic, creature, plant)
    require('gui.materials').MaterialDialog {
        frame_title = title,
        prompt = prompt,
        mat_filter = filter,
        use_inorganic = inorganic,
        use_creature = creature,
        use_plant = plant,
        on_select = script.mkresume(true),
        on_cancel = script.mkresume(false),
        on_close = script.qresume(nil),
    }:show()
    return script.wait()
end

local default_accessors = {
    get_unit = function(opts)
        return tonumber(opts.unit) and df.unit.find(tonumber(opts.unit)) or
            dfhack.gui.getSelectedUnit(true)
    end,
    get_item_type = function()
        return showItemPrompt('What item do you want?',
            function(itype) return df.item_type[itype] ~= 'FOOD' end, true)
    end,
    get_mat = function(itype, opts)
        if not usesCreature(itype) then
            return showMaterialPrompt('Wish', 'And what material should it be made of?',
                not opts.unrestricted and getMatFilter(itype, opts) or nil)
        end
        local creatureok, _, creatureTable = script.showListPrompt('Wish', 'What creature should it be?',
            COLOR_LIGHTGREEN, getCreatureList(), 1, true)
        if not creatureok then return false end
        local raceId, casteId = getCreatureRaceAndCaste(creatureTable[3])
        if df.item_type[itype] ~= 'CORPSEPIECE' then
            return true, -1, raceId, casteId, -1
        end
        local bodpartok, bodypart = script.showListPrompt('Wish', 'What body part should it be?',
            COLOR_LIGHTGREEN, getCreaturePartList(raceId, casteId), 1, true)
        if not bodpartok then return false end
        local corpsepieceGeneric = false
        local partlayerok, partlayerID
        if bodypart == 1 then
            corpsepieceGeneric = true
            partlayerok, partlayerID = script.showListPrompt('Wish', 'What creature material should it be?',
                COLOR_LIGHTGREEN, getCreatureMaterialList(raceId, casteId), 1, true)
        else
            --the offsets here are because indexes in lua are wonky (some start at 0, some start at 1), so we adjust for that, as well as the index offset created by inserting the "generic" option at the start of the body part selection prompt
            bodypart = bodypart - 2
            partlayerok, partlayerID = script.showListPrompt('Wish', 'What tissue layer should it be?',
                COLOR_LIGHTGREEN, getCreaturePartLayerList(raceId, casteId, bodypart), 1, true)
            partlayerID = partlayerID - 1
        end
        if not partlayerok then return end
        return true, -1, raceId, casteId, bodypart, partlayerID - 1, corpsepieceGeneric
    end,
    get_quality = function()
        return script.showListPrompt('Wish', 'What quality should it be?',
            COLOR_LIGHTGREEN, qualityTable())
    end,
    get_description = function()
        return script.showInputPrompt('Slab', 'What should the slab say?', COLOR_WHITE)
    end,
    get_count = function()
        return script.showInputPrompt('Wish', 'How many do you want?',
            COLOR_LIGHTGREEN)
    end,
}

if not dfhack.isMapLoaded() then
    qerror('create-item needs a loaded map to work')
end

local opts = {}
local positionals = argparse.processArgsGetopt({ ... }, {
    { nil, 'startup',      handler = function() opts.startup = true end },
    { 'f', 'unrestricted', handler = function() opts.unrestricted = true end },
    { 'h', 'help',         handler = function() opts.help = true end },
    {
        'u',
        'unit',
        hasArg = true,
        handler = function(arg) opts.unit = argparse.nonnegativeInt(arg, 'unit') end,
    },
    {
        'c',
        'count',
        hasArg = true,
        handler = function(arg) opts.count = argparse.nonnegativeInt(arg, 'count') end,
    },
})

if positionals[1] == 'help' then opts.help = true end
if opts.help then
    print(dfhack.script_help())
    return
end

if opts.startup then
    eventful.onReactionComplete.hackWishP = function(reaction, unit)
        if not reaction.code:find('DFHACK_WISH') then return end
        local accessors = copyall(default_accessors)
        accessors.get_unit = function() return unit end
        script.start(createitem.hackWish, accessors, { count = 1 })
    end
    return
end

script.start(createitem.hackWish, default_accessors, opts)
