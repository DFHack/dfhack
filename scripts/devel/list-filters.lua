-- List input items for the building currently being built.

local dumper = require 'dumper'

local function copy_flags(tgt,src,name)
    local val = src[name]
    if val.whole == 0 then
        return
    end
    local out = {}
    tgt[name] = out
    for k,v in pairs(val) do
        if v then
            out[k] = v
        end
    end
end

local function copy_value(tgt,src,name,defval)
    if src[name] ~= defval then
        tgt[name] = src[name]
    end
end
local function copy_enum(tgt,src,name,defval,ename,enum)
    if src[name] ~= defval then
        tgt[name] = ename..'.'..enum[src[name]]
    end
end

local lookup = {}

for i=df.job_item_vector_id._first_item,df.job_item_vector_id._last_item do
    local id = df.job_item_vector_id.attrs[i].other
    local ptr
    if id == df.items_other_id.ANY then
        ptr = df.global.world.items.all
    elseif id == df.items_other_id.BAD then
        ptr = df.global.world.items.bad
    else
        ptr = df.global.world.items.other[id]
    end
    if ptr then
        local _,addr = df.sizeof(ptr)
        lookup[addr] = 'df.job_item_vector_id.'..df.job_item_vector_id[i]
    end
end

local function clone_filter(src,quantity)
    local tgt = {}
    src.flags2.allow_artifact = false
    if quantity ~= 1 then
        tgt.quantity = quantity
    end
    copy_enum(tgt, src, 'item_type', -1, 'df.item_type', df.item_type)
    copy_value(tgt, src, 'item_subtype', -1)
    copy_value(tgt, src, 'mat_type', -1)
    copy_value(tgt, src, 'mat_index', -1)
    copy_flags(tgt, src, 'flags1')
    copy_flags(tgt, src, 'flags2')
    copy_flags(tgt, src, 'flags3')
    copy_value(tgt, src, 'reaction_class', '')
    copy_value(tgt, src, 'has_material_reaction_product', '')
    copy_value(tgt, src, 'metal_ore', -1)
    copy_value(tgt, src, 'min_dimension', -1)
    copy_enum(tgt, src, 'has_tool_use', -1, 'df.tool_uses', df.tool_uses)
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
    local out = string.gsub(fmt, '"', '')
    print(out)
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
