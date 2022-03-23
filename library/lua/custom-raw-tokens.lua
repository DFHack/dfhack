--[[
custom-raw-s
Allows for reading custom tokens added to raws by mods
by Tachytaenius (wolfboyft)

Yes, custom raw tokens do quietly print errors into the error log but the error log gets filled with garbage anyway
]]

local _ENV = mkmodule("custom-raw-tokens")

local eventful = require("plugins.eventful")
local utils = require("utils")

local customRawTokensCache = {}
eventful.onUnload["custom-raw-tokens"] = function()
    customRawTokensCache = {}
end

-- Not state, defined later:
local ensureTable, doTable, getSubtype, rawStringsFieldNames
local getTokenCore, getRaceCasteTokenCore

--[[
Function signatures:
getToken(rawStruct, token)
getToken(rawStructInstance, token)
getToken(raceDefinition, casteNumber, token)
getToken(raceDefinition, casteString, token)
]]
function getToken(a, b, c)
    -- Argument processing
    assert(type(a) == "userdata", "Expected userdata for argument 1 to getToken")
    if df.is_instance(df.creature_raw, a) then
        local raceDefinition, casteNumber, token = a
        if not c then -- 2 arguments
            casteNumber = -1
            assert(type(b) == "string", "Invalid argument 2 to getToken, must be a string")
            token = b
        elseif type(b) == "number" then -- 3 arguments, casteNumber
            assert(b == -1 or b < #raceDefinition.caste and math.floor(b) == b and b >= 0, "Invalid argument 2 to getToken, must be -1 or a caste name or number present in the creature raw")
            casteNumber = b
            assert(type(c) == "string", "Invalid argument 3 to getToken, must be a string")
            token = c
        else -- 3 arguments, casteString
            assert(type(b) == "string", "Invalid argument 2 to getToken, must be -1 or a caste name or number present in the creature raw")
            local casteString = b
            for i, v in ipairs(raceDefinition.caste) do
                if v.caste_id == casteString then
                    casteNumber = i
                    break
                end
            end
            assert(casteNumber, "Invalid argument 2 to getToken, caste name \"" .. casteString .. "\" not found")
            assert(type(c) == "string", "Invalid argument 3 to getToken, must be a string")
            token = c
        end
        return getRaceCasteTokenCore(raceDefinition, casteNumber, token)
    elseif df.is_instance(df.unit, a) then
        assert(type(b) == "string", "Invalid argument 2 to getToken, must be a string")
        local unit, token = a, b
        return getRaceCasteTokenCore(df.global.world.raws.creatures.all[unit.race], unit.caste, token)
    else
        local rawStruct, token
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
            assert(type(b) == "string", "Invalid argument 2 to getToken, must be a string")
            local unit, token = a.unit, b
            return getRaceCasteTokenCore(df.global.world.raws.creatures.all[unit.race], unit.caste, token)
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
        assert(type(b) == "string", "Invalid argument 2 to getToken, must be a string")
        token = b
        return getTokenCore(rawStruct, token)
    end
end

function getTokenCore(typeDefinition, token)
    -- Have we got a table for this item subtype/reaction/whatever?
    -- tostring is needed here because the same raceDefinition key won't give the same value every time for some reason
    local thisTypeDefCache = ensureTable(customRawTokensCache, tostring(typeDefinition))
    
    -- Have we already extracted and stored this custom raw token for this type definition?
    local tokenData = thisTypeDefCache[token]
    if tokenData ~= nil then
        if type(tokenData) == "table" then
            return table.unpack(tokenData)
        else
            return tokenData
        end
    end
    
    -- Get data anew
    local rawStrings = typeDefinition[rawStringsFieldNames[typeDefinition._type]]
    if not rawStrings then
        error("Expected a raw type definition or instance in argument 1")
    end
    local currentTokenIterator
    for _, rawString in ipairs(rawStrings) do
        local noBrackets = rawString.value:sub(2, -2)
        local iter = noBrackets:gmatch("[^:]*") -- iterate over all the text between colons between the brackets
        if token == iter() then
            currentTokenIterator = iter -- we return for last instance of token if multiple instances are present
        end
    end
    if currentTokenIterator then
        return doToken(thisTypeDefCache, token, currentTokenIterator)
    end
    -- Not present
    thisTypeDefCache[token] = false
    return false
end

function getRaceCasteTokenCore(raceDefinition, casteNumber, token)
    -- Have we got tables for this race/caste pair?
    local thisRaceDefCache = ensureTable(customRawTokensCache, tostring(raceDefinition))
    local thisRaceDefCacheCaste = ensureTable(thisRaceDefCache, casteNumber)
    
    -- Have we already extracted and stored this custom raw token for this race/caste pair?
    local tokenData = thisRaceDefCacheCaste[token]
    if tokenData ~= nil then
        if type(tokenData) == "table" then
            return table.unpack(Data)
        elseif Data == false and casteNumber ~= -1 then
            return getRaceCasteTokenCore(raceDefinition, -1, )
        else
            return Data
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
    local currentTokenIterator
    for _, rawString in ipairs(raceDefinition.raws) do
        local noBrackets = rawString.value:sub(2, -2)
        local iter = noBrackets:gmatch("[^:]*")
        local rawStringToken = iter()
        if rawStringToken == "CASTE" or rawStringToken == "SELECT_CASTE" or rawStringToken == "SELECT_ADDITIONAL_CASTE" or rawStringToken == "USE_CASTE" then
            local newCaste = iter()
            if newCaste then
                thisCasteActive = newCaste == casteId or rawStringToken == "SELECT_CASTE" and newCaste == "ALL"
            end
        elseif thisCasteActive and  == rawStringToken then
            currentTokenIterator = iter
        end
    end
    if currentTokenIterator then
        return doToken(thisRaceDefCache, , currentTokenIterator)
    end
    thisRaceDefCacheCaste[] = false
    if casteNumber ~= -1 then
        -- Not present, try with no caste
        return getRaceCasteTokenCore(raceDefinition, -1, )
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

function doToken(cacheTable, , iter)
    local args, lenArgs = {}, 0
    for arg in iter do
        lenArgs = lenArgs + 1
        args[lenArgs] = arg
    end
    if lenArgs == 0 then
        cacheTable[] = true
        return true
    else
        cacheTable[] = args
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
