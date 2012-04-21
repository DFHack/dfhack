local _ENV = mkmodule('plugins.sort.units')

local utils = require('utils')

orders = orders or {}

orders.name = {
    key = function(unit)
        return dfhack.TranslateName(unit.name)
    end,
    compare = utils.compare_name
}

orders.age = {
    key = function(unit)
        return dfhack.units.getAge(unit)
    end
}

return _ENV