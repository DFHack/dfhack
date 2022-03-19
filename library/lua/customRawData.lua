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

customRawData(struct, "FOO", true, false) returns "BAR", 20
customRawData(struct, "BAZ") returns true
customRawData(struct, "QUX"[, ...]) returns false
customRawData(struct, "QUUZ", true) returns "40"
if customRawData(struct, "QUUZ") is called afterwards it will return "40" and not 40 because of how the caching works
customRawData(struct, "CORGE") will error, because "GRAULT" cannot be converted to a number

Yes, custom raw tags do quietly print errors into the error log but the error log gets filled with garbage anyway
]]

local eventful = require("plugins.eventful")

local rawStringsFieldNames -- Defined after the main function for file clarity (not state)

local customRaws = {}
eventful.onUnload.clearExtractedCustomRawData = function()
    customRaws = {}
end

local function customRawData(typeDefinition, tag, ...)
    -- TODO: more advanced raw constructs
    
    -- Have we got a table for this item subtype/reaction/whatever?
    local customRawTable = customRaws[typeDefinition]
    if not customRawTable then
        customRawTable = {}
        customRaws[typeDefinition] = customRawTable
    end
    
    -- Have we already extracted and stored this custom raw tag for this type definition?
    local tagData = customRawTable[tag]
    if tagData ~= nil then
        if type(tagData) == "table" then
            return table.unpack(tagData)
        else
            return tagData
        end
    end
    
    -- Get data anew
    local rawStrings = typeDefinition[rawStringsFieldNames[typeDefinition._type]]
    for _, v in ipairs(rawStrings) do
        local noBrackets = v.value:sub(2, -2)
        local iter = noBrackets:gmatch("[^:]*")
        if tag == iter() then
            local args = {}
            for arg in iter do
                local isString = select(#args+1, ...)
                if not isString then
                    arg = tonumber(arg)
                end
                args[#args+1] = arg
            end
            if #args == 0 then
                customRawTable[tag] = true
                return true
            else
                customRawTable[tag] = args
                return table.unpack(args)
            end
        end
    end
    return false
end

rawStringsFieldNames = {
    inorganic_raw = "str",
    plant_raw = "raws",
    creature_raw = "raws",
    itemdef_weaponst = "raw_strings",
    itemdef_trapcompst = "raw_strings",
    itemdef_toyst = "raw_strings",
    itemdef_toolst = "raw_strings",
    itemdef_instrumentst = "raw_strings",
    itemdef_armorst = "raw_strings",
    itemdef_ammost = "raw_strings",
    itemdef_siegeammost = "raw_strings",
    itemdef_glovesst = "raw_strings",
    itemdef_shoesst = "raw_strings",
    itemdef_shieldsst = "raw_strings",
    itemdef_helmsst = "raw_strings",
    itemdef_pantsst = "raw_strings",
    itemdef_foodst = "raw_strings",
    entity_raw = "raws",
    language_word = "str",
    language_symbol = "str",
    language_translation = "str",
    reaction = "raw_strings",
    interaction = "str"
}

return customRawData
