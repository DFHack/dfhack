-- Gelds or ungelds animals
-- Written by Josh Cooper(cppcooper) on 2019-12-10, last modified: 2020-02-23

utils = require('utils')

local validArgs = utils.invert({
    'unit',
    'toggle',
    'ungeld',
    'help',
    'find',
})
local args = utils.processArgs({...}, validArgs)
local help = [====[

geld
====
Geld allows the user to geld and ungeld animals.

Valid options:

``-unit <id>``: Gelds the unit with the specified ID.
                This is optional; if not specified, the selected unit is used instead.

``-ungeld``:    Ungelds the specified unit instead (see also `ungeld`).

``-toggle``:    Toggles the gelded status of the specified unit.

``-help``:      Shows this help information

]====]

unit=nil

if args.help then
    print(help)
    return
end

if args.unit then
    id=tonumber(args.unit)
    if id then
        unit = df.unit.find(id)
    else
        qerror("Invalid ID provided.")
    end
else
    unit = dfhack.gui.getSelectedUnit()
end

if not unit then
    qerror("Invalid unit selection.")
end

if unit.sex == df.pronoun_type.she then
    qerror("Cannot geld female animals")
    return
end

function FindBodyPart(unit,newstate)
    bfound = false
    for i,wound in ipairs(unit.body.wounds) do
        for j,part in ipairs(wound.parts) do
            if unit.body.wounds[i].parts[j].flags2.gelded ~= newstate then
                bfound = true
                if newstate ~= nil then
                    unit.body.wounds[i].parts[j].flags2.gelded = newstate
                end
            end
        end
    end
    return bfound
end

function AddParts(unit)
    for i,wound in ipairs(unit.body.wounds) do
        if wound.id == 1 and #wound.parts == 0 then
            utils.insert_or_update(unit.body.wounds[i].parts,{ new = true, body_part_id = 1 }, 'body_part_id')
        end
    end
end

function Geld(unit)
    unit.flags3.gelded = true
    if not FindBodyPart(unit,true) then
        utils.insert_or_update(unit.body.wounds,{ new = true, id = unit.body.wound_next_id }, 'id')
        unit.body.wound_next_id = unit.body.wound_next_id + 1
        AddParts(unit)
        if not FindBodyPart(unit,true) then
            error("could not find body part")
        end
    end
    print(string.format("unit %s gelded.",unit.id))
end

function Ungeld(unit)
    unit.flags3.gelded = false
    FindBodyPart(unit,false)
    print(string.format("unit %s ungelded.",unit.id))
end

if args.find then
    print(FindBodyPart(unit) and "found" or "not found")
    return
end

local oldstate = dfhack.units.isGelded(unit)
local newstate

if args.ungeld then
    newstate = false
elseif args.toggle then
    newstate = not oldstate
else
    newstate = true
end

if newstate ~= oldstate then
    if newstate then
        Geld(unit)
    else
        Ungeld(unit)
    end
else
    qerror(string.format("unit %s is already %s", unit.id, oldstate and "gelded" or "ungelded"))
end
