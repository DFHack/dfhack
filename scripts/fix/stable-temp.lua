-- Reset item temperature to the value of their tile.

local count = 0
local types = {}

local function update_temp(item,btemp)
    if item.temperature ~= btemp then
        count = count + 1
        local tid = item:getType()
        types[tid] = (types[tid] or 0) + 1
    end
    item.temperature = btemp
    item.temperature_fraction = 0

    if item.contaminants then
        for _,c in ipairs(item.contaminants) do
            c.temperature = btemp
            c.temperature_fraction = 0
        end
    end

    for _,sub in ipairs(dfhack.items.getContainedItems(item)) do
        update_temp(sub,btemp)
    end

    item:checkTemperatureDamage()
end

local last_frame = df.global.world.frame_counter-1

for _,item in ipairs(df.global.world.items.all) do
    if item.flags.on_ground and df.item_actual:is_instance(item) and
       item.temp_updated_frame == last_frame then
        local pos = item.pos
        local block = dfhack.maps.getTileBlock(pos)
        if block then
            update_temp(item, block.temperature_1[pos.x%16][pos.y%16])
        end
    end
end

print('Items updated: '..count)

local tlist = {}
for k,_ in pairs(types) do tlist[#tlist+1] = k end
table.sort(tlist, function(a,b) return types[a] > types[b] end)
for _,k in ipairs(tlist) do
    print('    '..df.item_type[k]..':', types[k])
end
