--[[
custom-raw-tags
Allows for reading custom tags added to raws by mods
by Tachytaenius (wolfboyft)

Yes, custom raw tags do quietly print errors into the error log but the error log gets filled with garbage anyway

TODO: the rest of the raw constructs
NOTE: Creature variations are already expanded in creature_raw.raws, so they don't need to be handled
]]

local _ENV = mkmodule("custom-raw-tags")

local eventful = require("plugins.eventful")
local utils = require("utils")

local customRawTagsCache = {}
eventful.onUnload["custom-raw-tags"] = function()
    customRawTagsCache = {}
end

-- Not state, defined later:
local ensureTable, doTable, getSubtype, rawStringsFieldNames
local getTagCore, getRaceCasteTagCore

--[[
Function signatures:
getTag(rawStruct, tag)
getTag(rawStructInstance, tag)
getTag(raceDefinition, casteNumber, tag)
getTag(raceDefinition, casteString, tag)
]]
function getTag(a, b, c)
    -- Argument processing
    assert(type(a) == "userdata", "Expected userdata for argument 1 to getTag")
    if df.is_instance(df.creature_raw, a) then
        local raceDefinition, casteNumber, tag = a
        if not c then -- 2 arguments
            casteNumber = -1
            assert(type(b) == "string", "Invalid argument 2 to getTag, must be a string")
            tag = b
        elseif type(b) == "number" then -- 3 arguments, casteNumber
            assert(b == -1 or b < #raceDefinition.caste and math.floor(b) == b and b >= 0, "Invalid argument 2 to getTag, must be -1 or a caste name or number present in the creature raw")
            casteNumber = b
            assert(type(c) == "string", "Invalid argument 3 to getTag, must be a string")
            tag = c
        else -- 3 arguments, casteString
            assert(type(b) == "string", "Invalid argument 2 to getTag, must be -1 or a caste name or number present in the creature raw")
            local casteString = b
            for i, v in ipairs(raceDefinition.caste) do
                if v.caste_id == casteString then
                    casteNumber = i
                    break
                end
            end
            assert(casteNumber, "Invalid argument 2 to getTag, caste name \"" .. casteString .. "\" not found")
            assert(type(c) == "string", "Invalid argument 3 to getTag, must be a string")
            tag = c
        end
        return getRaceCasteTagCore(raceDefinition, casteNumber, tag)
    elseif df.is_instance(df.unit, a) then
        assert(type(b) == "string", "Invalid argument 2 to getTag, must be a string")
        local unit, tag = a, b
        return getRaceCasteTagCore(df.global.world.raws.creatures.all[unit.race], unit.caste, tag)
    else
        local rawStruct, tag
        if df.is_instance(df.historical_entity, a) then
            rawStruct = a.entity_raw
        elseif df.is_instance(df.item, a) then
            rawStruct = getSubtype(a)
        elseif df.is_instance(df.job, a) then
            if job.job_type == df.job_type.CustomReaction then
                for i, v in ipairs(df.global.world.raws.reactions.reactions) do
                    if job.reaction_name == v.code then
                        rawStruct = v
                        break
                    end
                end
            end
        elseif df.is_instance(df.proj_itemst, a) then
            rawStruct = a.item and a.item.subtype
        elseif df.is_instance(df.proj_unitst, a) then
            if not a.unit then return false end
            assert(type(b) == "string", "Invalid argument 2 to getTag, must be a string")
            local unit, tag = a.unit, b
            return getRaceCasteTagCore(df.global.world.raws.creatures.all[unit.race], unit.caste, tag)
        elseif df.is_instance(df.building_workshopst, a) or df.is_instance(df.building_furnacest, a) then
            rawStruct = df.building_def.find(a.custom_type)
        elseif df.is_instance(df.plant, a) then
            rawStruct = df.global.world.raws.plants.all[a.material]
        elseif df.is_instance(df.interaction_instance, a) then
            rawStruct = df.global.world.raws.interactions[a.interaction_id]
        else
           -- Assume raw struct *is* argument a
           rawStruct = a
        end
        if not rawStruct then return false end
        assert(type(b) == "string", "Invalid argument 2 to getTag, must be a string")
        tag = b
        return getTagCore(rawStruct, tag)
    end
end

function getTagCore(typeDefinition, tag)
    -- Have we got a table for this item subtype/reaction/whatever?
    -- tostring is needed here because the same raceDefinition key won't give the same value every time for some reason
    local thisTypeDefCache = ensureTable(customRawTagsCache, tostring(typeDefinition))
    
    -- Have we already extracted and stored this custom raw tag for this type definition?
    local tagData = thisTypeDefCache[tag]
    if tagData ~= nil then
        if type(tagData) == "table" then
            return table.unpack(tagData)
        else
            return tagData
        end
    end
    
    -- Get data anew
    local rawStrings = typeDefinition[rawStringsFieldNames[typeDefinition._type]]
    if not rawStrings then
        error("Expected a raw type definition or instance in argument 1")
    end
    local currentTagIterator
    for _, rawString in ipairs(rawStrings) do
        local noBrackets = rawString.value:sub(2, -2)
        local iter = noBrackets:gmatch("[^:]*") -- iterate over all the text between colons between the brackets
        if tag == iter() then
            currentTagIterator = iter -- we return for last instance of tag if multiple instances are present
        end
    end
    if currentTagIterator then
        return doTag(thisTypeDefCache, tag, currentTagIterator)
    end
    -- Not present
    thisTypeDefCache[tag] = false
    return false
end

function getRaceCasteTagCore(raceDefinition, casteNumber, tag)
    -- Have we got tables for this race/caste pair?
    local thisRaceDefCache = ensureTable(customRawTagsCache, tostring(raceDefinition))
    local thisRaceDefCacheCaste = ensureTable(thisRaceDefCache, casteNumber)
    
    -- Have we already extracted and stored this custom raw tag for this race/caste pair?
    local tagData = thisRaceDefCacheCaste[tag]
    if tagData ~= nil then
        if type(tagData) == "table" then
            return table.unpack(tagData)
        elseif tagData == false and casteNumber ~= -1 then
            return getRaceCasteTagCore(raceDefinition, -1, tag)
        else
            return tagData
        end
    end
    
    -- Get data anew. Here we have to track what caste is currently being written to
    local casteId, thisCasteActive
    if casteNumber ~= -1 then
        casteId = raceDefinition.caste[casteNumber].caste_id
        thisCasteActive = false
    else
        thisCasteActive = true
    end
    local currentTagIterator
    for _, rawString in ipairs(raceDefinition.raws) do
        local noBrackets = rawString.value:sub(2, -2)
        local iter = noBrackets:gmatch("[^:]*")
        local rawStringTag = iter()
        if rawStringTag == "CASTE" or rawStringTag == "SELECT_CASTE" or rawStringTag == "SELECT_ADDITIONAL_CASTE" or rawStringTag == "USE_CASTE" then
            local newCaste = iter()
            if newCaste then
                thisCasteActive = newCaste == casteId or rawStringTag == "SELECT_CASTE" and newCaste == "ALL"
            end
        elseif thisCasteActive and tag == rawStringTag then
            currentTagIterator = iter
        end
    end
    if currentTagIterator then
        return doTag(thisRaceDefCache, tag, currentTagIterator)
    end
    thisRaceDefCacheCaste[tag] = false
    if casteNumber ~= -1 then
        -- Not present, try with no caste
        return getRaceCasteTagCore(raceDefinition, -1, tag)
    else
        return false -- don't get into an infinite loop!
    end
end

function ensureTable(tableToHoldIn, key)
    local ensuredTable = tableToHoldIn[key]
    if not ensuredTable then
        ensuredTable = {}
        tableToHoldIn[key] = ensuredTable
    end
    return ensuredTable
end

function doTag(cacheTable, tag, iter)
    local args, lenArgs = {}, 0
    for arg in iter do
        lenArgs = lenArgs + 1
        args[lenArgs] = arg
    end
    if lenArgs == 0 then
        cacheTable[tag] = true
        return true
    else
        cacheTable[tag] = args
        return table.unpack(args)
    end
end

function getSubtype(item)
    if item:getSubtype() == -1 then return nil end
    return dfhack.items.getSubtypeDef(item:getType(), item:getSubtype())
end

rawStringsFieldNames = {
    [df.inorganic_raw] = "str",
    [df.plant_raw] = "raws",
    [df.creature_raw] = "raws",
    [df.itemdef_weaponst] = "raw_strings",
    [df.itemdef_trapcompst] = "raw_strings",
    [df.itemdef_toyst] = "raw_strings",
    [df.itemdef_toolst] = "raw_strings",
    [df.itemdef_instrumentst] = "raw_strings",
    [df.itemdef_armorst] = "raw_strings",
    [df.itemdef_ammost] = "raw_strings",
    [df.itemdef_siegeammost] = "raw_strings",
    [df.itemdef_glovesst] = "raw_strings",
    [df.itemdef_shoesst] = "raw_strings",
    [df.itemdef_shieldst] = "raw_strings",
    [df.itemdef_helmst] = "raw_strings",
    [df.itemdef_pantsst] = "raw_strings",
    [df.itemdef_foodst] = "raw_strings",
    [df.entity_raw] = "raws",
    [df.language_word] = "str",
    [df.language_symbol] = "str",
    [df.language_translation] = "str",
    [df.reaction] = "raw_strings",
    [df.interaction] = "str"
}

return _ENV
