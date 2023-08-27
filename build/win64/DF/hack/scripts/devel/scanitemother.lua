-- Display the item lists that the selected item is part of.
local selected_item = dfhack.gui.getAnyItem()

if not selected_item then
    qerror("This script requires an item selected!")
end

local indices = {}
for index, value in pairs(df.global.world.items.other) do
    for _, item in pairs(value) do
        if item == selected_item then
            table.insert(indices, index)
        end
    end
end

print("Found in the following indices:")
for _, index in pairs(indices) do
    print(" " .. index)
end
