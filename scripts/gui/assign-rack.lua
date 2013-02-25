-- Assign weapon racks to squads. Requires the weaponrack-unassign patch.

-- See bug 1445 for more info about the patches.

local utils = require 'utils'
local gui = require 'gui'
local guidm = require 'gui.dwarfmode'
local widgets = require 'gui.widgets'
local dlg = require 'gui.dialogs'
local bp = require 'binpatch'

AssignRack = defclass(AssignRack, guidm.MenuOverlay)

AssignRack.focus_path = 'assign-rack'

AssignRack.ATTRS {
    building = DEFAULT_NIL,
    frame_inset = 1,
    frame_background = COLOR_BLACK,
}

function list_squads(building,squad_table,squad_list)
    local sqlist = building:getSquads()
    if not sqlist then
        return
    end

    for i,v in ipairs(sqlist) do
        local obj = df.squad.find(v.squad_id)
        if obj then
            if not squad_table[v.squad_id] then
                squad_table[v.squad_id] = { id = v.squad_id, obj = obj }
                table.insert(squad_list, squad_table[v.squad_id])
            end

            -- Set specific use flags
            for n,ok in pairs(v.mode) do
                if ok then
                    squad_table[v.squad_id][n] = true
                end
            end

            -- Check if any use is possible
            local btype = building:getType()
            if btype == df.building_type.Bed then
                if v.mode.sleep then
                    squad_table[v.squad_id].any = true
                end
            elseif btype == df.building.Weaponrack then
                if v.mode.train or v.mode.indiv_eq then
                    squad_table[v.squad_id].any = true
                end
            else
                if v.mode.indiv_eq then
                    squad_table[v.squad_id].any = true
                end
            end
        end
    end

    for i,v in ipairs(building.parents) do
        list_squads(v, squad_table, squad_list)
    end
end

function filter_invalid(list, id)
    for i=#list-1,0,-1 do
        local bld = df.building.find(list[i])
        if not bld or bld:getSpecificSquad() ~= id then
            list:erase(i)
        end
    end
end

function AssignRack:init(args)
    self.squad_table = {}
    self.squad_list = {}
    list_squads(self.building, self.squad_table, self.squad_list)
    table.sort(self.squad_list, function(a,b) return a.id < b.id end)

    self.choices = {}
    for i,v in ipairs(self.squad_list) do
        if v.any and (v.train or v.indiv_eq) then
            local name = v.obj.alias
            if name == '' then
                name = dfhack.TranslateName(v.obj.name, true)
            end

            filter_invalid(v.obj.rack_combat, v.id)
            filter_invalid(v.obj.rack_training, v.id)

            table.insert(self.choices, {
                icon = self:callback('isSelected', v),
                icon_pen = COLOR_LIGHTGREEN,
                obj = v,
                text = {
                    name, NEWLINE, '  ',
                    { text = function()
                        return string.format('%d combat, %d training', #v.obj.rack_combat, #v.obj.rack_training)
                      end }
                }
            })
        end
    end

    self:addviews{
        widgets.Label{
            frame = { l = 0, t = 0 },
            text = {
                'Assign Weapon Rack'
            }
        },
        widgets.List{
            view_id = 'list',
            frame = { t = 2, b = 2 },
            icon_width = 2, row_height = 2,
            scroll_keys = widgets.SECONDSCROLL,
            choices = self.choices,
            on_submit = self:callback('onSubmit'),
        },
        widgets.Label{
            frame = { l = 0, t = 2 },
            text_pen = COLOR_LIGHTRED,
            text = 'No appropriate barracks\n\nNote: weapon racks use the\nIndividual equipment flag',
            visible = (#self.choices == 0),
        },
        widgets.Label{
            frame = { l = 0, b = 0 },
            text = {
                { key = 'LEAVESCREEN', text = ': Back',
                  on_activate = self:callback('dismiss') }
            }
        },
    }
end

function AssignRack:isSelected(info)
    if self.building.specific_squad == info.id then
        return '\xfb'
    else
        return nil
    end
end

function AssignRack:onSubmit(idx, choice)
    local rid = self.building.id
    local curid = self.building.specific_squad

    local cur = df.squad.find(curid)
    if cur then
        utils.erase_sorted(cur.rack_combat, rid)
        utils.erase_sorted(cur.rack_training, rid)
    end

    self.building.specific_squad = -1
    df.global.ui.equipment.update.buildings = true

    local new = df.squad.find(choice.obj.id)
    if new and choice.obj.id ~= curid then
        self.building.specific_squad = choice.obj.id

        if choice.obj.indiv_eq then
            utils.insert_sorted(new.rack_combat, rid)
        end
        if choice.obj.train then
            utils.insert_sorted(new.rack_training, rid)
        end
    end
end

function AssignRack:onInput(keys)
    if self:propagateMoveKeys(keys) then
        if df.global.world.selected_building ~= self.building then
            self:dismiss()
        end
    else
        AssignRack.super.onInput(self, keys)
    end
end

if dfhack.gui.getCurFocus() ~= 'dwarfmode/QueryBuilding/Some/Weaponrack' then
    qerror("This script requires a weapon rack selected in the 'q' mode")
end

AssignRack{ building = dfhack.gui.getSelectedBuilding() }:show()

if not already_patched then
    local patch = bp.load_dif_file('weaponrack-unassign')
    if patch and patch:isApplied() then
        already_patched = true
    end
end

if not already_patched then
    dlg.showMessage(
        'BUG ALERT',
        { 'This script requires applying the binary patch', NEWLINE,
          'named weaponrack-unassign. Otherwise the game', NEWLINE,
          'will lose your settings due to a bug.' },
        COLOR_YELLOW
    )
end
