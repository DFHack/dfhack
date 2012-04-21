local _ENV = mkmodule('plugins.sort')

local utils = require('utils')
local units = require('plugins.sort.units')

orders = orders or {}
orders.units = units.orders

function parse_ordering_spec(type,...)
    local group = orders[type]
    if group == nil then
        dfhack.printerr('Invalid ordering class: '..tostring(type))
        return nil
    end

    local specs = table.pack(...)
    local rv = { }
    for _,spec in ipairs(specs) do
        local cm = group[spec]
        if cm == nil then
            dfhack.printerr('Unknown order for '..type..': '..tostring(spec))
            return nil
        end
        rv[#rv+1] = cm
    end

    return rv
end

make_sort_order = utils.make_sort_order

return _ENV