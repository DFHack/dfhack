-- Removes water from buckets (for lye-making).
--[[=begin

fix/dry-buckets
===============
Removes water from all buckets in your fortress, allowing them
to be used for making lye.  Skips buckets in buildings (eg a well),
being carried, or currently used by a job.

=end]]

local emptied = 0
local water_type = dfhack.matinfo.find('WATER').type

for _,item in ipairs(df.global.world.items.all) do
    container = dfhack.items.getContainer(item)
    if container ~= nil
    and container:getType() == df.item_type.BUCKET
    and not (container.flags.in_job or container.flags.in_building)
    and item:getMaterial() == water_type
    and item:getType() == df.item_type.LIQUID_MISC
    and not (item.flags.in_job or item.flags.in_building) then
        dfhack.items.remove(item)
        emptied = emptied + 1
    end
end

print('Emptied '..emptied..' buckets.')
