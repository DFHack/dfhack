-- This script will elevate all the physical attributes of a unit
-- by vjek
--[[=begin

elevate-physical
================
As for elevate-mental, but for physical traits.  High is good for soldiers,
while having an ineffective hammerer can be useful too...

=end]]

function ElevatePhysicalAttributes(value)
    unit=dfhack.gui.getSelectedUnit()
    if unit==nil then
        print ("No unit under cursor!  Aborting with extreme prejudice.")
        return
    end
    --print name of dwarf
    print("Adjusting "..dfhack.TranslateName(dfhack.units.getVisibleName(unit)))
    --walk through available attributes, adjust current to max
    local ok,f,t,k = pcall(pairs,unit.body.physical_attrs)
    if ok then
        for k,v in f,t,k do
            if value ~= nil then
                print("Adjusting current value for "..tostring(k).." of "..v.value.." to the value of "..value)
                v.value=value
            else
                print("Adjusting current value for "..tostring(k).." of "..v.value.." to max value of "..v.max_value)
                v.value=v.max_value
                --below will reset values back to "normal"
                --v.value=v.max_value/2
            end
        end
    end
end
--script execution starts here
local opt = ...
opt = tonumber(opt)

if opt ~= nil then
    if opt >=0 and opt <=5000 then
        ElevatePhysicalAttributes(opt)
    end
    if opt <0 or opt >5000 then
        print("Invalid Range or argument.  This script accepts either no argument, in which case it will increase the attribute to the max_value for the unit, or an argument between 0 and 5000, which will set all attributes to that value.")
    end
end

if opt == nil then
    ElevatePhysicalAttributes()
end
