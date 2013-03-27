-- List input items for the building currently being built.
--
-- This is where the filters in lua/dfhack/buildings.lua come from.

local dumper = require 'dumper'
local utils = require 'utils'
local buildings = require 'dfhack.buildings'

local function name_enum(tgt,name,ename,enum)
    if tgt[name] ~= nil then
        tgt[name] = ename..'.'..enum[tgt[name]]
    end
end

local lookup = {}
local items = df.global.world.items

for i=df.job_item_vector_id._first_item,df.job_item_vector_id._last_item do
    local id = df.job_item_vector_id.attrs[i].other
    local ptr
    if id == df.items_other_id.ANY then
        ptr = items.all
    elseif id == df.items_other_id.BAD then
        ptr = items.bad
    else
        ptr = items.other[id]
    end
    if ptr then
        local _,addr = df.sizeof(ptr)
        lookup[addr] = 'df.job_item_vector_id.'..df.job_item_vector_id[i]
    end
end

local function clone_filter(src,quantity)
    local tgt = utils.clone_with_default(src, buildings.input_filter_defaults, true)
    if quantity ~= 1 then
        tgt.quantity = quantity
    end
    name_enum(tgt, 'item_type', 'df.item_type', df.item_type)
    name_enum(tgt, 'has_tool_use', 'df.tool_uses', df.tool_uses)
    local ptr = src.item_vector
    if ptr and ptr ~= df.global.world.items.other[0] then
        local _,addr = df.sizeof(ptr)
        tgt.vector_id = lookup[addr]
    end
    return tgt
end

local function dump(name)
    local out = {}
    for i,v in ipairs(df.global.ui_build_selector.requirements) do
        out[#out+1] = clone_filter(v.filter, v.count_required)
    end

    local fmt = dumper.DataDumper(out,name,false,1,4)
    fmt = string.gsub(fmt, '"(df%.[^"]+)"','%1')
    fmt = string.gsub(fmt, '%s+$', '')
    print(fmt)
end

local itype = df.global.ui_build_selector.building_type
local stype = df.global.ui_build_selector.building_subtype

if itype == df.building_type.Workshop then
    dump('    [df.workshop_type.'..df.workshop_type[stype]..'] = ')
elseif itype == df.building_type.Furnace then
    dump('    [df.furnace_type.'..df.furnace_type[stype]..'] = ')
elseif itype == df.building_type.Trap then
    dump('    [df.trap_type.'..df.trap_type[stype]..'] = ')
else
    dump('    [df.building_type.'..df.building_type[itype]..'] = ')
end
