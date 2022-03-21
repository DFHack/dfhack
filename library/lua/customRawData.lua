--[[
Custom raw data manager
by Tachytaenius (wolfboyft)

The first argument to customRawData is one of DF's internal structs for the things defined by raws
For example, dfhack.gui.getSelectedItem().subtype
The second is the name of the tag
All the arguments after that are booleans denoting whether each argument is a string.
(Absence, of course, impies false (causing a tonumber call))

It will return true for tags with no arguments
For tags with one or more arguments, it will return each argument (replacing the first true), number or string as requested
(Since everything except false and nil implies true you can still use "if customRawData(...) ..." for things with arguments to determine whether the tag is present)

The first call records the results of the tag argument
Any subsequent calls asking for that tag from that struct will return the first results regardless of the conversion arguments

All state resets on unload to prepare for potentially different raws

Examples usage for
    [FOO:BAR:20]
    [BAZ]
    [QUUZ:40]
    [CORGE:GRAULT]

customRawData.getTag(struct, "FOO", true, false) returns "BAR", 20
customRawData.getTag(struct, "BAZ") returns true
customRawData.getTag(struct, "QUX"[, ...]) returns false
customRawData.getTag(struct, "QUUZ", true) returns "40"
if customRawData.getTag(struct, "QUUZ") is called afterwards it will return "40" and not 40 because of how the caching works
customRawData.getTag(struct, "CORGE") will error, because "GRAULT" cannot be converted to a number

customRawData.getRaceCasteTag(raceDefinition, casteNumber, tag[, ...]) gets a tag respecting the caste tag priority of a race/caste pair
customRawData.getUnitTag(unit, tag[, ...]) is a wrapper for getRaceCasteTag

customRawData.getTag(raceDefinition, tag[, ...]) is equivalent to customRawData.getRaceCasteTag(raceDefinition, -1, tag[, ...])

Yes, custom raw tags do quietly print errors into the error log but the error log gets filled with garbage anyway

TODO: the rest of the raw constructs
]]

local _ENV = mkmodule("customRawData")

local eventful = require("plugins.eventful")

local customRawDataCache = {}
eventful.onUnload.clearExtractedCustomRawData = function()
    customRawDataCache = {}
end

local rawStringsFieldNames, ensureTable, iterArgs -- (Is not state) Defined after the main functions for file clarity

function getTag(typeDefinition, tag, ...)
    if type(typeDefinition) ~= "userdata" then
        error("Expected raw struct (eg plant_raw or item_ammost) for argument 1 of getTag")
    end
    local rawStrings = typeDefinition[rawStringsFieldNames[typeDefinition._type]] -- used when getting data anew below, just used as a boolean here
    if not rawStrings then
        error("Expected raw struct (eg plant_raw or item_ammost) for argument 1 of getTag")
    end
    assert(type(tag == "string"), "Expected string for argument 2 of getTag")
    
    -- Is this a race definition?
    if typeDefinition._type == df.creature_raw then
        return getRaceCasteTag(typeDefinition, -1, tag, ...)
    end
    
    -- Have we got a table for this item subtype/reaction/whatever?
    -- tostring is needed here because the same raceDefinition key won't give the same value every time for some reason
    local thisTypeDefCache = ensureTable(customRawDataCache, tostring(typeDefinition))
    
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
    for _, rawString in ipairs(rawStrings) do
        local noBrackets = rawString.value:sub(2, -2)
        local iter = noBrackets:gmatch("[^:]*") -- iterate over all the text between colons between the brackets
        if tag == iter() then
            local args, i = iterArgs(thisTypeDefCache, tag, iter, ...)
            if not args then -- was error
                error("Tag argument " .. i .. " of " .. rawString.value .. " could not be converted to number.")
            end
            if i == 0 then
                thisTypeDefCache[tag] = true
                return true
            else
                thisTypeDefCache[tag] = args
                return table.unpack(args)
            end
        end
    end
    -- Not present
    thisTypeDefCache[tag] = false
    return false
end

function getUnitTag(unit, tag, ...) -- respects both race and caste of a creature
    assert(raceDefinition._type == df.unit, "Expected unit for argument 1 of getRaceCasteTag")
    assert(type(tag == "string"), "Expected string for argument 2 of getUnitTag")
    
    local raceDefinition = df.global.world.raws.creatures.all[unit.race]
    local casteNumber = unit.caste
    return getRaceCasteTag(raceDefinition, casteNumber, tag, ...)
end

function getRaceCasteTag(raceDefinition, casteNumber, tag, ...)
    assert(raceDefinition._type == df.creature_raw, "Expected creature_raw for argument 1 of getRaceCasteTag")
    assert(type(casteNumber == "number"), "Expected number for argument 2 of getRaceCasteTag")
    assert(type(tag == "string"), "Expected string for argument 3 of getRaceCasteTag")
    
    -- Have we got tables for this race/caste pair?
    local thisRaceDefCache = ensureTable(customRawDataCache, tostring(raceDefinition))
    local thisRaceDefCacheCaste = ensureTable(thisRaceDefCache, casteNumber)
    
    -- Have we already extracted and stored this custom raw tag for this race/caste pair?
    local tagData = thisRaceDefCacheCaste[tag]
    if tagData ~= nil then
        if type(tagData) == "table" then
            return table.unpack(tagData)
        elseif tagData == false and casteNumber ~= -1 then
            return getRaceCasteTag(raceDefinition, -1, tag, ...)
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
    for _, rawString in ipairs(raceDefinition.raws) do
        local noBrackets = rawString.value:sub(2, -2)
        local iter = noBrackets:gmatch("[^:]*")
        local rawStringTag = iter()
        if rawStringTag == "CASTE" or rawStringTag == "SELECT_CASTE" or rawStringTag == "SELECT_ADDITIONAL_CASTE" or rawStringTag == "USE_CASTE" then
            local newCaste = iter()
            thisCasteActive = newCaste == casteId or rawStringTag == "SELECT_CASTE" and newCaste == "ALL"
        elseif thisCasteActive and tag == rawStringTag then
            local args, i = iterArgs(thisRaceDefCacheCaste, tag, iter, ...)
            if not args then -- was error
                error("Tag argument " .. i .. " of " .. rawString.value .. " could not be converted to number.")
            end
            if i == 0 then
                thisRaceDefCacheCaste[tag] = true
                return true
            else
                thisRaceDefCacheCaste[tag] = args
                return table.unpack(args)
            end
        end
    end
    thisRaceDefCacheCaste[tag] = false
    if casteNumber ~= -1 then
        -- Not present, try with no caste
        return getRaceCasteTag(raceDefinition, -1, tag, ...)
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

function iterArgs(cacheTable, tag, iter, ...)
    local args, i = {}, 0
    for arg in iter do
        i = i + 1
        local isString = select(i, ...)
        if not isString then
            arg = tonumber(arg)
            if not arg then
                return false, i -- error
            end
        end
        args[i] = arg
    end
    return args, i
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
