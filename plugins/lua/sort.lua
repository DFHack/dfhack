local _ENV = mkmodule('plugins.sort')

local utils = require('utils')
local units = require('plugins.sort.units')
local items = require('plugins.sort.items')

orders = orders or {}
orders.units = units.orders
orders.items = items.orders

function parse_ordering_spec(type,...)
    local group = orders[type]
    if group == nil then
        dfhack.printerr('Invalid ordering class: '..tostring(type))
        return nil
    end

    local specs = table.pack(...)
    local rv = { }

    for _,spec in ipairs(specs) do
        local nil_first = false
        if string.sub(spec,1,1) == '<' then
            nil_first = true
            spec = string.sub(spec,2)
        end

        local reverse = false
        if string.sub(spec,1,1) == '>' then
            reverse = true
            spec = string.sub(spec,2)
        end

        local cm = group[spec]

        if cm == nil then
            dfhack.printerr('Unknown order for '..type..': '..tostring(spec))
            return nil
        end

        if nil_first or reverse then
            cm = copyall(cm)
            cm.nil_first = nil_first
            cm.reverse = reverse
        end

        rv[#rv+1] = cm
    end

    return rv
end

make_sort_order = utils.make_sort_order

return _ENV