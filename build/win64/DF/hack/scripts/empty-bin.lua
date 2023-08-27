-- Empty a bin onto the floor
-- Based on "emptybin" by StoneToad
-- https://gist.github.com/stonetoad/11129025
-- http://dwarffortresswiki.org/index.php/DF2014_Talk:Bin

--[====[

empty-bin
=========

Empties the contents of the selected bin onto the floor.

]====]

local bin = dfhack.gui.getSelectedItem(true) or qerror("No item selected")
local items = dfhack.items.getContainedItems(bin)

if #items > 0 then
    print('Emptying ' .. dfhack.items.getDescription(bin, 0))
    for _, item in pairs(items) do
        print('  ' .. dfhack.items.getDescription(item, 0))
        dfhack.items.moveToGround(item, bin.pos)
    end
else
    print('No contained items')
end
