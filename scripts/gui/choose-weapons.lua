-- Rewrite individual choice weapons into specific types.

local utils = require 'utils'
local dlg = require 'gui.dialogs'

local defs = df.global.world.raws.itemdefs
local entity = df.global.ui.main.fortress_entity
local tasks = df.global.ui.tasks
local equipment = df.global.ui.equipment

function find_best_weapon(unit,mode)
    local best = nil
    local skill = nil
    local skill_level = nil
    local count = 0
    local function try(id,iskill)
        local slevel = dfhack.units.getNominalSkill(unit,iskill)
        -- Choose most skill
        if (skill ~= nil and slevel > skill_level)
        or (skill == nil and slevel > 0) then
            best,skill,skill_level,count = id,iskill,slevel,0
        end
        -- Then most produced within same skill
        if skill == iskill then
            local cnt = tasks.created_weapons[id]
            if cnt > count then
                best,count = id,cnt
            end
        end
    end
    for _,id in ipairs(entity.resources.weapon_type) do
        local def = defs.weapons[id]
        if def.skill_ranged >= 0 then
            if mode == nil or mode == 'ranged' then
                try(id, def.skill_ranged)
            end
        else
            if mode == nil or mode == 'melee' then
                try(id, def.skill_melee)
            end
        end
    end
    return best
end

function unassign_wrong_items(unit,position,spec,subtype)
    for i=#spec.assigned-1,0,-1 do
        local id = spec.assigned[i]
        local item = df.item.find(id)

        if item.subtype.subtype ~= subtype then
            spec.assigned:erase(i)

            -- TODO: somewhat unexplored area; maybe missing some steps
            utils.erase_sorted(position.assigned_items,id)
            if utils.erase_sorted(equipment.items_assigned.WEAPON,item,'id') then
                utils.insert_sorted(equipment.items_unassigned.WEAPON,item,'id')
            end
            equipment.update.weapon = true
            unit.military.pickup_flags.update = true
        end
    end
end

local count = 0

function adjust_uniform_spec(unit,position,spec,force)
    if not unit then return end
    local best
    if spec.indiv_choice.melee then
        best = find_best_weapon(unit, 'melee')
    elseif spec.indiv_choice.ranged then
        best = find_best_weapon(unit, 'ranged')
    elseif spec.indiv_choice.any or force then
        best = find_best_weapon(unit, nil)
    end
    if best then
        count = count + 1
        spec.item_filter.item_subtype = best
        spec.indiv_choice.any = false
        spec.indiv_choice.melee = false
        spec.indiv_choice.ranged = false
        unassign_wrong_items(unit, position, spec, best)
    end
end

function adjust_position(unit,position,force)
    if not unit then
        local fig = df.historical_figure.find(position.occupant)
        if not fig then return end
        unit = df.unit.find(fig.unit_id)
    end

    for _,v in ipairs(position.uniform.weapon) do
        adjust_uniform_spec(unit, position, v, force)
    end
end

function adjust_squad(squad, force)
    for _,pos in ipairs(squad.positions) do
        adjust_position(nil, pos, force)
    end
end

local args = {...}
local vs = dfhack.gui.getCurViewscreen()
local vstype = df.viewscreen_layer_militaryst
if not vstype:is_instance(vs) then
    qerror('Call this from the military screen')
end

if vs.page == vstype.T_page.Equip
and vs.equip.mode == vstype.T_equip.T_mode.Customize then
    local slist = vs.layer_objects[0]
    local squad = vs.equip.squads[slist:getListCursor()]

    if slist.active then
        print('Adjusting squad.')
        adjust_squad(squad)
    else
        local plist = vs.layer_objects[1]
        local pidx = plist:getListCursor()
        local pos = squad.positions[pidx]
        local unit = vs.equip.units[pidx]

        if plist.active then
            print('Adjusting position.')
            adjust_position(unit, pos)
        elseif unit and vs.equip.edit_mode < 0 then
            local wlist = vs.layer_objects[2]
            local idx = wlist:getListCursor()
            local cat = vs.equip.assigned.category[idx]

            if wlist.active and cat == df.uniform_category.weapon then
                print('Adjusting spec.')
                adjust_uniform_spec(unit, pos, vs.equip.assigned.spec[idx], true)
            end
        end
    end
else
    qerror('Call this from the Equip page of military screen')
end

if count > 1 then
    dlg.showMessage(
        'Choose Weapons',
        'Updated '..count..' uniform entries.', COLOR_GREEN
    )
elseif count == 0 then
    dlg.showMessage(
        'Choose Weapons',
        'Did not find any entries to update.', COLOR_YELLOW
    )
end
