--[[
Allows for reading custom tags added to raws by mods
by Tachytaenius (wolfboyft)

The first argument to customRawTags.getTag is one of DF's internal structs for the things defined by raws, like item_ammost or reaction
For example, dfhack.gui.getSelectedItem().subtype
The second is the name of the tag

It will return false if the tag is absent
It will return true for tags with no arguments
For tags with one or more arguments, it will return each argument as a string
(Since everything except false and nil implies true you can still use "if customRawTags(...) ..." for things with arguments to determine whether the tag is present)

The results are cached
All state resets on unload to prepare for potentially different raws

Examples usage for
    [FOO:BAR:20]
    [BAZ]

customRawTags.getTag(struct, "FOO") returns "BAR", "20"
customRawTags.getTag(struct, "BAZ") returns true
customRawTags.getTag(struct, "QUX") returns false

customRawTags.getRaceCasteTag(raceDefinition, casteNumber, tag) gets a tag respecting the caste tag priority of a race/caste pair
casteNumber -1 selects no caste
calling getTag with a creature_raw will do getRaceCasteTag with a caste number of -1
customRawTags.getUnitTag(unit, tag) is a wrapper for getRaceCasteTag

Yes, custom raw tags do quietly print errors into the error log but the error log gets filled with garbage anyway

TODO: the rest of the raw constructs
NOTE: Creature variations are already expanded in creature_raw.raws, so they don't need to be handled
]]

local _ENV = mkmodule("custom-raw-tags")

local eventful = require("plugins.eventful")

local customRawTagsCache = {}
clearExtractedcustomRawTagsKey = {} -- table so that it can be unique
eventful.onUnload[clearExtractedcustomRawTagsKey] = function()
    customRawTagsCache = {}
end

local ensureTable, doTable, rawStringsFieldNames -- (Is not state) Defined after the main functions for file clarity

function getTag(typeDefinition, tag)
    if type(typeDefinition) ~= "userdata" then
        error("Expected raw struct (eg plant_raw or item_ammost) for argument 1 of getTag")
    end
    local rawStrings = typeDefinition[rawStringsFieldNames[typeDefinition._type]] -- used when getting data anew below, just used as a boolean here
    if not rawStrings then
        error("Expected raw struct (e.g. plant_raw or item_ammost) for argument 1 of getTag")
    end
    assert(type(tag == "string"), "Expected string for argument 2 of getTag")
    
    -- Is this a race definition?
    if typeDefinition._type == df.creature_raw then
        return getRaceCasteTag(typeDefinition, -1, tag)
    end
    
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
    for _, rawString in ipairs(rawStrings) do
        local noBrackets = rawString.value:sub(2, -2)
        local iter = noBrackets:gmatch("[^:]*") -- iterate over all the text between colons between the brackets
        if tag == iter() then
            return doTag(thisTypeDefCache, tag, iter)
        end
    end
    -- Not present
    thisTypeDefCache[tag] = false
    return false
end

function getUnitTag(unit, tag) -- respects both race and caste of a creature
    assert(raceDefinition._type == df.unit, "Expected unit for argument 1 of getRaceCasteTag")
    assert(type(tag == "string"), "Expected string for argument 2 of getUnitTag")
    
    local raceDefinition = df.global.world.raws.creatures.all[unit.race]
    local casteNumber = unit.caste
    return getRaceCasteTag(raceDefinition, casteNumber, tag)
end

function getRaceCasteTag(raceDefinition, casteNumber, tag)
    assert(raceDefinition._type == df.creature_raw, "Expected creature_raw for argument 1 of getRaceCasteTag")
    assert(type(casteNumber == "number"), "Expected number for argument 2 of getRaceCasteTag")
    assert(type(tag == "string"), "Expected string for argument 3 of getRaceCasteTag")
    
    -- Have we got tables for this race/caste pair?
    local thisRaceDefCache = ensureTable(customRawTagsCache, tostring(raceDefinition))
    local thisRaceDefCacheCaste = ensureTable(thisRaceDefCache, casteNumber)
    
    -- Have we already extracted and stored this custom raw tag for this race/caste pair?
    local tagData = thisRaceDefCacheCaste[tag]
    if tagData ~= nil then
        if type(tagData) == "table" then
            return table.unpack(tagData)
        elseif tagData == false and casteNumber ~= -1 then
            return getRaceCasteTag(raceDefinition, -1, tag)
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
            return doTag(thisRaceDefCacheCaste, tag, iter)
        end
    end
    thisRaceDefCacheCaste[tag] = false
    if casteNumber ~= -1 then
        -- Not present, try with no caste
        return getRaceCasteTag(raceDefinition, -1, tag)
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
    local args, i = {}, 0
    for arg in iter do
        i = i + 1
        args[i] = arg
    end
    if i == 0 then
        cacheTable[tag] = true
        return true
    else
        cacheTable[tag] = args
        return table.unpack(args)
    end
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
