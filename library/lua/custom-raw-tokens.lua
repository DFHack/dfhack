--[[
custom-raw-tokens
Allows for reading custom tokens added to raws by mods
by Tachytaenius (wolfboyft)

Yes, non-vanilla raw tokens do quietly print errors into the error log but the error log gets filled with garbage anyway

NOTE: This treats plant growths similarly to creature castes but there is no way to deselect a growth, so don't put a token you want to apply to a whole plant after any growth definitions
]]

local _ENV = mkmodule("custom-raw-tokens")

local customRawTokensCache = {}
dfhack.onStateChange.customRawTokens = function(code)
    if code == SC_WORLD_UNLOADED then
        customRawTokensCache = {}
    end
end

local function doToken(cacheTable, token, iter)
    local args, lenArgs = {}, 0
    for arg in iter do
        lenArgs = lenArgs + 1
        args[lenArgs] = arg
    end
    if lenArgs == 0 then
        cacheTable[token] = true
        return true
    else
        cacheTable[token] = args
        return table.unpack(args)
    end
end

local function getSubtype(item)
    if item:getSubtype() == -1 then return nil end -- number
    return dfhack.items.getSubtypeDef(item:getType(), item:getSubtype()) -- struct
end

local rawStringsFieldNames = {
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

local function getTokenCore(typeDefinition, token)
    -- Have we got a table for this item subtype/reaction/whatever?
    -- tostring is needed here because the same raceDefinition key won't give the same value every time
    local thisTypeDefCache = ensure_key(customRawTokensCache, tostring(typeDefinition))

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
    local success, dftype = pcall(function() return typeDefinition._type end)
    local rawStrings = typeDefinition[rawStringsFieldNames[dftype]]
    if not success or not rawStrings then
        error("Expected a raw type definition or instance in argument 1")
    end
    local currentTokenIterator
    for _, rawString in ipairs(rawStrings) do -- e.g. "[CUSTOM_TOKEN:FOO:2]"
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

local function getRaceCasteTokenCore(raceDefinition, casteNumber, token)
    -- Have we got tables for this race/caste pair?
    local thisRaceDefCache = ensure_key(customRawTokensCache, tostring(raceDefinition))
    local thisRaceDefCacheCaste = ensure_key(thisRaceDefCache, casteNumber)

    -- Have we already extracted and stored this custom raw token for this race/caste pair?
    local tokenData = thisRaceDefCacheCaste[token]
    if tokenData ~= nil then
        if type(tokenData) == "table" then
            return table.unpack(tokenData)
        elseif tokenData == false and casteNumber ~= -1 then
            return getRaceCasteTokenCore(raceDefinition, -1, token)
        else
            return tokenData
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
        elseif thisCasteActive and token == rawStringToken then
            currentTokenIterator = iter
        end
    end
    if currentTokenIterator then
        return doToken(thisRaceDefCache, token, currentTokenIterator)
    end
    thisRaceDefCacheCaste[token] = false
    if casteNumber == -1 then
        return false -- Don't get into an infinite loop!
    end
    -- Not present, try with no caste
    return getRaceCasteTokenCore(raceDefinition, -1, token)
end

local function getPlantGrowthTokenCore(plantDefinition, growthNumber, token)
    -- Have we got tables for this plant/growth pair?
    local thisPlantDefCache = ensure_key(customRawTokensCache, tostring(plantDefinition))
    local thisPlantDefCacheGrowth = ensure_key(thisPlantDefCache, growthNumber)

    -- Have we already extracted and stored this custom raw token for this plant/growth pair?
    local tokenData = thisPlantDefCacheGrowth[token]
    if tokenData ~= nil then
        if type(tokenData) == "table" then
            return table.unpack(tokenData)
        elseif tokenData == false and growthNumber ~= -1 then
            return getPlantGrowthTokenCore(plantDefinition, -1, token)
        else
            return tokenData
        end
    end

    -- Get data anew. Here we have to track what growth is currently being written to
    local growthId, thisGrowthActive
    if growthNumber ~= -1 then
        growthId = plantDefinition.growths[growthNumber].id
        thisGrowthActive = false
    else
        thisGrowthActive = true
    end
    local currentTokenIterator
    for _, rawString in ipairs(plantDefinition.raws) do
        local noBrackets = rawString.value:sub(2, -2)
        local iter = noBrackets:gmatch("[^:]*")
        local rawStringToken = iter()
        if rawStringToken == "GROWTH" then
            local newGrowth = iter()
            if newGrowth then
                thisGrowthActive = newGrowth == growthId
            end
        elseif thisGrowthActive and token == rawStringToken then
            currentTokenIterator = iter
        end
    end
    if currentTokenIterator then
        return doToken(thisPlantDefCache, token, currentTokenIterator)
    end
    thisPlantDefCacheGrowth[token] = false
    if growthNumber == -1 then
        return false
    end
    return getPlantGrowthTokenCore(plantDefinition, -1, token)
end

--[[
Function signatures:
getToken(rawStruct, token)
getToken(rawStructInstance, token)
getToken(raceDefinition, casteNumber, token)
getToken(raceDefinition, casteString, token)
getToken(plantDefinition, growthNumber, token)
getToken(plantDefinition, growthString, token)
]]

local function getTokenArg1RaceDefinition(raceDefinition, b, c)
    local casteNumber, token
    if not c then
        -- 2 arguments
        casteNumber = -1
        assert(type(b) == "string", "Invalid argument 2 to getToken, must be a string")
        token = b
    elseif type(b) == "number" then
        -- 3 arguments, casteNumber
        assert(b == -1 or b < #raceDefinition.caste and math.floor(b) == b and b >= 0, "Invalid argument 2 to getToken, must be -1 or a caste name or number present in the creature raw")
        casteNumber = b
        assert(type(c) == "string", "Invalid argument 3 to getToken, must be a string")
        token = c
    else
        -- 3 arguments, casteString
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
end

local function getTokenArg1PlantDefinition(plantDefinition, b, c)
    local growthNumber, token
    if not c then
        -- 2 arguments
        growthNumber = -1
        assert(type(b) == "string", "Invalid argument 2 to getToken, must be a string")
        token = b
    elseif type(b) == "number" then
        -- 3 arguments, growthNumber
        assert(b == -1 or b < #plantDefinition.growths and math.floor(b) == b and b >= 0, "Invalid argument 2 to getToken, must be -1 or a growth name or number present in the plant raw")
        growthNumber = b
        assert(type(c) == "string", "Invalid argument 3 to getToken, must be a string")
        token = c
    else
        -- 3 arguments, growthString
        assert(type(b) == "string", "Invalid argument 2 to getToken, must be -1 or a growth name or number present in the plant raw")
        local growthString = b
        for i, v in ipairs(plantDefinition.growths) do
            if v.id == growthString then
                growthNumber = i
                break
            end
        end
        assert(growthNumber, "Invalid argument 2 to getToken, growth name \"" .. growthString .. "\" not found")
        assert(type(c) == "string", "Invalid argument 3 to getToken, must be a string")
        token = c
    end
    return getPlantGrowthTokenCore(plantDefinition, growthNumber, token)
end

local function getTokenArg1Else(userdata, token)
    assert(type(token) == "string", "Invalid argument 2 to getToken, must be a string")
    local rawStruct
    if df.is_instance(df.historical_entity, userdata) then
        rawStruct = userdata.entity_raw
    elseif df.is_instance(df.item, userdata) then
        rawStruct = getSubtype(userdata)
    elseif df.is_instance(df.job, userdata) then
        if job.job_type == df.job_type.CustomReaction then
            for i, v in ipairs(df.global.world.raws.reactions.reactions) do
                if job.reaction_name == v.code then
                    rawStruct = v
                    break
                end
            end
        end
    elseif df.is_instance(df.proj_itemst, userdata) then
        if not userdata.item then return false end
        if df.is_instance(df.item_plantst, userdata.item) or df.is_instance(df.item_plant_growthst, userdata.item) then
            -- use plant behaviour from getToken
            return getToken(userdata.item, token)
        end
        rawStruct = userdata.item and userdata.item.subtype
    elseif df.is_instance(df.proj_unitst, userdata) then
        if not usertdata.unit then return false end
        -- special return so do tag here
        local unit = userdata.unit
        return getRaceCasteTokenCore(df.global.world.raws.creatures.all[unit.race], unit.caste, token)
    elseif df.is_instance(df.building_workshopst, userdata) or df.is_instance(df.building_furnacest, userdata) then
        rawStruct = df.building_def.find(userdata.custom_type)
    elseif df.is_instance(df.interaction_instance, userdata) then
        rawStruct = df.global.world.raws.interactions[userdata.interaction_id]
    else
        -- Assume raw struct *is* argument 1
        rawStruct = userdata
    end
    if not rawStruct then return false end
    return getTokenCore(rawStruct, token)
end

function getToken(from, b, c)
    -- Argument processing
    assert(from and type(from) == "userdata", "Expected userdata for argument 1 to getToken")
    if df.is_instance(df.creature_raw, from) then
        -- Signatures from here:
        -- getToken(raceDefinition, casteNumber, token)
        -- getToken(raceDefinition, casteString, token)
        return getTokenArg1RaceDefinition(from, b, c)
    elseif df.is_instance(df.unit, from) then
        -- Signatures from here:
        -- getToken(rawStructInstance, token)
        assert(type(b) == "string", "Invalid argument 2 to getToken, must be a string")
        local unit, token = from, b
        return getRaceCasteTokenCore(df.global.world.raws.creatures.all[unit.race], unit.caste, token)
    elseif df.is_instance(df.plant_raw, from) then
        -- Signatures from here:
        -- getToken(plantDefinition, growthNumber, token)
        -- getToken(plantDefinition, growthString, token)
        return getTokenArg1PlantDefinition(from, b, c)
    elseif df.is_instance(df.plant, from) then
        -- Signatures from here:
        -- getToken(rawStructInstance, token)
        assert(type(b) == "string", "Invalid argument 2 to getToken, must be a string")
        local plantDefinition, plantGrowthNumber, token = df.global.world.raws.plants.all[from.material], -1, b
        return getPlantGrowthTokenCore(plantDefinition, plantGrowthNumber, token)
    elseif df.is_instance(df.item_plantst, from) then
        -- Signatures from here:
        -- getToken(rawStructInstance, token)
        local matInfo = dfhack.matinfo.decode(from)
        if matInfo.mode ~= "plant" then return false end
        assert(type(b) == "string", "Invalid argument 2 to getToken, must be a string")
        local plantDefinition, plantGrowthNumber, token = matInfo.plant, -1, b
        return getPlantGrowthTokenCore(plantDefinition, plantGrowthNumber, token)
    elseif df.is_instance(df.item_plant_growthst, from) then
        -- Signatures from here:
        -- getToken(rawStructInstance, token)
        local matInfo = dfhack.matinfo.decode(from)
        if matInfo.mode ~= "plant" then return false end
        assert(type(b) == "string", "Invalid argument 2 to getToken, must be a string")
        local plantDefinition, plantGrowthNumber, token = matInfo.plant, from.growth_print, b
        return getPlantGrowthTokenCore(plantDefinition, plantGrowthNumber, token)
    else
        -- Signatures from here:
        -- getToken(rawStruct, token)
        -- getToken(rawStructInstance, token)
        return getTokenArg1Else(from, b)
    end
end

return _ENV
