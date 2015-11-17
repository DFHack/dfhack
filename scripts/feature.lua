-- List or manage map features & enable magma furnaces
local help = [[=begin

feature
=======
Enables management of map features.

* Discovering a magma feature (magma pool, volcano, magma sea, or curious
  underground structure) permits magma workshops and furnaces to be built.
* Discovering a cavern layer causes plants (trees, shrubs, and grass) from
  that cavern to grow within your fortress.

Options:

:list:          Lists all map features in your current embark by index.
:magma:         Enable magma furnaces (discovers a random magma feature).
:show X:        Marks the selected map feature as discovered.
:hide X:        Marks the selected map feature as undiscovered.

=end]]

local map_features = df.global.world.features.map_features

function toggle_feature(idx, discovered)
    map_features[tonumber(idx)].flags.Discovered = discovered
end

function list_features()
    for idx, feat in ipairs(map_features) do
        local kind = df.feature_type[feat:getType()]:gsub('_', ' ')
        local discovered = feat.flags.Discovered and 'shown. ' or 'hidden.'
        print('Feature #'..idx..' is '..discovered..' It is a "'..kind..'".')
    end
end

function enable_magma_funaces()
    for idx, feat in ipairs(map_features) do
        if tostring(feat):find('magma') ~= nil then
            toggle_feature(idx, true)
            print('Enabled magma furnaces.')
            return
        end
    end
    dfhack.printerr('Could not find a magma-bearing feature.')
end

local args = {...}
if args[1] == 'list' then
    list_features()
elseif args[1] == 'magma' then
    enable_magma_funaces()
elseif args[1] == 'show' then
    toggle_feature(args[2], true)
elseif args[1] == 'hide' then
    toggle_feature(args[2], false)
else
    print(help)
end
