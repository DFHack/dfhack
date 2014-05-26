-- Clone the current uniform template in the military screen.

local utils = require 'utils'
local gui = require 'gui'

local entity = df.global.ui.main.fortress_entity

local args = {...}
local vs = dfhack.gui.getCurViewscreen()
local vstype = df.viewscreen_layer_militaryst
if not vstype:is_instance(vs) then
    qerror('Call this from the military screen')
end

local slist = vs.layer_objects[0]

if vs.page == vstype.T_page.Uniforms
and slist.active and slist.num_entries > 0
and not vs.equip.in_name_uniform
then
    local idx = slist.num_entries

    if #vs.equip.uniforms ~= idx or #entity.uniforms ~= idx then
        error('Uniform vector length mismatch')
    end

    local uniform = vs.equip.uniforms[slist:getListCursor()]

    local ucopy = uniform:new()
    ucopy.id = entity.next_uniform_id
    ucopy.name = ucopy.name..'(Copy)'

    for k,v in ipairs(ucopy.uniform_item_info) do
        for k2,v2 in ipairs(v) do
            v[k2] = v2:new()
        end
    end

    entity.next_uniform_id = entity.next_uniform_id + 1
    entity.uniforms:insert('#',ucopy)
    vs.equip.uniforms:insert('#',ucopy)

    slist.num_entries = idx+1
    slist.cursor = idx-1
    gui.simulateInput(vs, 'STANDARDSCROLL_DOWN')
else
    qerror('Call this with a uniform selected on the Uniforms page of military screen')
end
