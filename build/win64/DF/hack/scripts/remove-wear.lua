-- Reset items in your fort to 0 wear
-- original author: Laggy, edited by expwnent
local help = [====[

remove-wear
===========
Sets the wear on items in your fort to zero.  Usage:

:remove-wear all:
    Removes wear from all items in your fort.
:remove-wear ID1 ID2 ...:
    Removes wear from items with the given ID numbers.

]====]

local args = {...}
local count = 0

if args[1] == 'help' then
    print(help)
    return
elseif args[1] == 'all' or args[1] == '-all' then
    for _, item in ipairs(df.global.world.items.all) do
        if item.wear > 0 then --hint:df.item_actual
            item:setWear(0)
            count = count + 1
        end
    end
else
    for i, arg in ipairs(args) do
        local item_id = tonumber(arg)
        if item_id then
            local item = df.item.find(item_id)
            if item then
                item:setWear(0)
                count = count + 1
            else
                dfhack.printerr('remove-wear: could not find item: ' .. item_id)
            end
        else
            qerror('Invalid item ID: ' .. arg)
        end
    end
end

print('remove-wear: removed wear from '..count..' objects')
