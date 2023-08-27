-- Reset item temperature to the value of their tile.
--[====[

fix/stable-temp
===============
Instantly sets the temperature of all free-lying items to be in equilibrium with
the environment, which stops temperature updates until something changes.
To maintain this efficient state, use `tweak fast-heat <tweak>`.

]====]
local args = {...}

local apply = (args[1] == 'apply')

local count = 0
local types = {} --as:number[]

local function update_temp(item,btemp)
    local item = item --as:df.item_actual
    if item.temperature.whole ~= btemp then
        count = count + 1
        local tid = item:getType()
        types[tid] = (types[tid] or 0) + 1
    end

    if apply then
        item.temperature.whole = btemp
        item.temperature.fraction = 0

        if item.contaminants then
            for _,c in ipairs(item.contaminants) do
                c.temperature.whole = btemp
                c.temperature.fraction = 0
            end
        end
    end

    for _,sub in ipairs(dfhack.items.getContainedItems(item)) do --as:df.item_actual
        update_temp(sub,btemp)
    end

    if apply then
        item:checkTemperatureDamage()
    end
end

local last_frame = df.global.world.frame_counter-1

for _,item in ipairs(df.global.world.items.all) do
    local item = item --as:df.item_actual
    if item.flags.on_ground and df.item_actual:is_instance(item) and
       item.temp_updated_frame == last_frame then
        local pos = item.pos
        local block = dfhack.maps.getTileBlock(pos)
        if block then
            update_temp(item, block.temperature_1[pos.x%16][pos.y%16])
        end
    end
end

if apply then
    print('Items updated: '..count)
else
    print("Use 'fix/stable-temp apply' to force-change temperature.")
    print('Items not in equilibrium: '..count)
end

local tlist = {}
for k,_ in pairs(types) do tlist[#tlist+1] = k end
table.sort(tlist, function(a,b) return types[a] > types[b] end)
for _,k in ipairs(tlist) do
    print('    '..df.item_type[k]..':', types[k])
end
