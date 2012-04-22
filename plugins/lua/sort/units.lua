local _ENV = mkmodule('plugins.sort.units')

local utils = require('utils')

orders = orders or {}

-- Relies on NULL being auto-translated to NULL, and then sorted
orders.exists = {
    key = function(unit)
        return 1
    end
}

orders.name = {
    key = function(unit)
        if unit.name.has_name then
            return dfhack.TranslateName(unit.name)
        end
    end,
    compare = utils.compare_name
}

orders.age = {
    key = function(unit)
        return dfhack.units.getAge(unit)
    end
}

-- This assumes that units are added to active in arrival order
orders.arrival = {
    key_table = function(units)
        local tmp={}
        for i,v in ipairs(units) do
            tmp[v.id] = i
        end
        local idx={}
        for i,v in ipairs(df.global.world.units.active) do
            local ix = tmp[v.id]
            if ix ~= nil then
                idx[ix] = i
            end
        end
        return idx
    end
}

orders.profession = {
    key = function(unit)
        local cp = unit.custom_profession
        if cp == '' then
            cp = df.profession.attrs[unit.profession].caption
        end
        return cp
    end
}

orders.squad = {
    key = function(unit)
        local sidx = unit.military.squad_index
        if sidx >= 0 then
            return sidx
        end
    end
}

orders.squad_position = {
    key = function(unit)
        local sidx = unit.military.squad_index
        if sidx >= 0 then
            return sidx * 1000 + unit.military.squad_position
        end
    end
}

return _ENV