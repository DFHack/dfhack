-- Stock dialog for selecting materials

local _ENV = mkmodule('gui.materials')

local gui = require('gui')
local widgets = require('gui.widgets')
local dlg = require('gui.dialogs')
local utils = require('utils')

ARROW = string.char(26)

CREATURE_BASE = 19
PLANT_BASE = 419

MaterialDialog = defclass(MaterialDialog, gui.FramedScreen)

MaterialDialog.focus_path = 'MaterialDialog'

MaterialDialog.ATTRS{
    prompt = 'Type or select a material from this list',
    frame_style = gui.GREY_LINE_FRAME,
    frame_inset = 1,
    frame_title = 'Select Material',
    -- new attrs
    none_caption = 'none',
    hide_none = false,
    use_inorganic = true,
    use_creature = true,
    use_plant = true,
    mat_filter = DEFAULT_NIL,
    on_select = DEFAULT_NIL,
    on_cancel = DEFAULT_NIL,
    on_close = DEFAULT_NIL,
}

function MaterialDialog:init(info)
    self:addviews{
        widgets.Label{
            text = {
                self.prompt, '\n\n',
                'Category: ', { text = self:cb_getfield('context_str'), pen = COLOR_CYAN }
            },
            text_pen = COLOR_WHITE,
            frame = { l = 0, t = 0 },
        },
        widgets.Label{
            view_id = 'back',
            visible = false,
            text = { { key = 'LEAVESCREEN', text = ': Back' } },
            frame = { r = 0, b = 0 },
            auto_width = true,
        },
        widgets.FilteredList{
            view_id = 'list',
            not_found_label = 'No matching materials',
            frame = { l = 0, r = 0, t = 4, b = 2 },
            icon_width = 2,
            on_submit = self:callback('onSubmitItem'),
        },
        widgets.Label{
            text = { {
                key = 'SELECT', text = ': Select',
                disabled = function() return not self.subviews.list:canSubmit() end
            } },
            frame = { l = 0, b = 0 },
        }
    }
    self:initBuiltinMode()
end

function MaterialDialog:getWantedFrameSize(rect)
    return math.max(self.frame_width or 40, #self.prompt), math.min(28, rect.height-8)
end

function MaterialDialog:onDestroy()
    if self.on_close then
        self.on_close()
    end
end

function MaterialDialog:initBuiltinMode()
    local choices = {}
    if not self.hide_none then
        table.insert(choices, { text = self.none_caption, mat_type = -1, mat_index = -1 })
    end

    if self.use_inorganic then
        table.insert(choices, {
            icon = ARROW, text = 'inorganic', key = 'CUSTOM_SHIFT_I',
            cb = self:callback('initInorganicMode')
        })
    end
    if self.use_creature then
        table.insert(choices, {
            icon = ARROW, text = 'creature', key = 'CUSTOM_SHIFT_C',
            cb = self:callback('initCreatureMode')
        })
    end
    if self.use_plant then
        table.insert(choices, {
            icon = ARROW, text = 'plant', key = 'CUSTOM_SHIFT_P',
            cb = self:callback('initPlantMode')
        })
    end

    local table = df.global.world.raws.mat_table.builtin
    for i=0,df.builtin_mats._last_item do
        self:addMaterial(choices, table[i], i, -1, false, nil)
    end

    self:pushContext('Any material', choices)
end

function MaterialDialog:initInorganicMode()
    local choices = {}

    for i,mat in ipairs(df.global.world.raws.inorganics) do
        self:addMaterial(choices, mat.material, 0, i, false, mat)
    end

    self:pushContext('Inorganic materials', choices)
end

function MaterialDialog:initCreatureMode()
    local choices = {}

    for i,v in ipairs(df.global.world.raws.creatures.all) do
        self:addObjectChoice(choices, v, v.name[0], CREATURE_BASE, i)
    end

    self:pushContext('Creature materials', choices)
end

function MaterialDialog:initPlantMode()
    local choices = {}

    for i,v in ipairs(df.global.world.raws.plants.all) do
        self:addObjectChoice(choices, v, v.name, PLANT_BASE, i)
    end

    self:pushContext('Plant materials', choices)
end

function MaterialDialog:addObjectChoice(choices, obj, name, typ, index)
    -- Check if any eligible children
    local count = #obj.material
    local idx = 0

    if self.mat_filter then
        count = 0
        for i,v in ipairs(obj.material) do
            if self.mat_filter(v, obj, typ+i, index) then
                count = count + 1
                if count > 1 then break end
                idx = i
            end
        end
    end

    -- Create an entry
    if count < 1 then
        return
    elseif count == 1 then
        self:addMaterial(choices, obj.material[idx], typ+idx, index, true, obj)
    else
        table.insert(choices, {
            icon = ARROW, text = name, mat_type = typ, mat_index = index,
            obj = obj, cb = self:callback('onSelectObj')
        })
    end
end

function MaterialDialog:onSelectObj(item)
    local choices = {}
    for i,v in ipairs(item.obj.material) do
        self:addMaterial(choices, v, item.mat_type+i, item.mat_index, false, item.obj)
    end
    self:pushContext(item.text, choices)
end

function MaterialDialog:addMaterial(choices, mat, typ, idx, pfix, parent)
    -- Check the filter
    if self.mat_filter and not self.mat_filter(mat, parent, typ, idx) then
        return
    end

    -- Find the material name
    local state = 0
    if mat.heat.melting_point <= 10015 then
        state = 1
    end
    local name = mat.state_name[state]
    name = string.gsub(name, '^frozen ','')
    name = string.gsub(name, '^molten ','')
    name = string.gsub(name, '^condensed ','')

    -- Add prefix if requested
    local key
    if pfix and mat.prefix ~= '' then
        name = mat.prefix .. ' ' .. name
        key = mat.prefix
    end

    table.insert(choices, {
        text = name,
        search_key = key,
        material = mat,
        mat_type = typ, mat_index = idx
    })
end

function MaterialDialog:pushContext(name, choices)
    if not self.back_stack then
        self.back_stack = {}
        self.subviews.back.visible = false
    else
        table.insert(self.back_stack, {
            context_str = self.context_str,
            all_choices = self.subviews.list:getChoices(),
            edit_text = self.subviews.list:getFilter(),
            selected = self.subviews.list:getSelected(),
        })
        self.subviews.back.visible = true
    end

    self.context_str = name
    self.subviews.list:setChoices(choices, 1)
end

function MaterialDialog:onGoBack()
    local save = table.remove(self.back_stack)
    self.subviews.back.visible = (#self.back_stack > 0)

    self.context_str = save.context_str
    self.subviews.list:setChoices(save.all_choices)
    self.subviews.list:setFilter(save.edit_text, save.selected)
end

function MaterialDialog:submitMaterial(typ, index)
    self:dismiss()

    if self.on_select then
        self.on_select(typ, index)
    end
end

function MaterialDialog:onSubmitItem(idx, item)
    if item.cb then
        item:cb(idx)
    else
        self:submitMaterial(item.mat_type, item.mat_index)
    end
end

function MaterialDialog:onInput(keys)
    if keys.LEAVESCREEN or keys.LEAVESCREEN_ALL then
        if self.subviews.back.visible and not keys.LEAVESCREEN_ALL then
            self:onGoBack()
        else
            self:dismiss()
            if self.on_cancel then
                self.on_cancel()
            end
        end
    else
        self:inputToSubviews(keys)
    end
end

function showMaterialPrompt(title, prompt, on_select, on_cancel, mat_filter)
    MaterialDialog{
        frame_title = title,
        prompt = prompt,
        mat_filter = mat_filter,
        on_select = on_select,
        on_cancel = on_cancel,
    }:show()
end

function ItemTypeDialog(args)
    args.text = args.prompt or 'Type or select an item type'
    args.text_pen = COLOR_WHITE
    args.with_filter = true
    args.icon_width = 2

    local choices = {}

    if not args.hide_none then
        table.insert(choices, {
            icon = '?', text = args.none_caption or 'none',
            item_type = -1, item_subtype = -1
        })
    end

    local filter = args.item_filter

    for itype = 0,df.item_type._last_item do
        local attrs = df.item_type.attrs[itype]
        local defcnt = dfhack.items.getSubtypeCount(itype)

        if not filter or filter(itype,-1) then
            local name = attrs.caption or df.item_type[itype]
            local icon
            if defcnt >= 0 then
                name = 'any '..name
                icon = '+'
            end
            table.insert(choices, {
                icon = icon, text = string.lower(name), item_type = itype, item_subtype = -1
            })
        end

        if defcnt > 0 then
            for subtype = 0,defcnt-1 do
                local def = dfhack.items.getSubtypeDef(itype, subtype)
                if not filter or filter(itype,subtype,def) then
                    table.insert(choices, {
                        icon = '\x1e', text = ' '..def.name, item_type = itype, item_subtype = subtype
                    })
                end
            end
        end
    end

    args.choices = choices

    if args.on_select then
        local cb = args.on_select
        args.on_select = function(idx, obj)
            return cb(obj.item_type, obj.item_subtype)
        end
    end

    return dlg.ListBox(args)
end

return _ENV
