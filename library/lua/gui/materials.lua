-- Stock dialog for selecting materials

local _ENV = mkmodule('gui.materials')

local gui = require('gui')
local widgets = require('gui.widgets')
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
        widgets.List{
            view_id = 'list',
            frame = { l = 0, r = 0, t = 6, b = 2 },
            icon_width = 2,
            on_submit = self:callback('onSubmitItem'),
        },
        widgets.EditField{
            view_id = 'edit',
            frame = { l = 2, t = 4 },
            on_change = self:callback('onFilterChoices'),
            on_char = self:callback('onFilterChar'),
        },
        widgets.Label{
            view_id = 'not_found',
            text = 'No matching materials.',
            text_pen = COLOR_LIGHTRED,
            frame = { l = 2, r = 0, t = 6 },
        },
        widgets.Label{
            text = { {
                key = 'SELECT', text = ': Select',
                disabled = function() return self.subviews.not_found.visible end
            } },
            frame = { l = 0, b = 0 },
        }
    }
    self:initBuiltinMode()
end

function MaterialDialog:getWantedFrameSize(rect)
    return math.max(40, #self.prompt), math.min(28, rect.height-8)
end

function MaterialDialog:onDestroy()
    if self.on_close then
        self.on_close()
    end
end

function MaterialDialog:initBuiltinMode()
    local choices = {
        { text = 'none', mat_type = -1, mat_index = -1 },
        { icon = ARROW, text = 'inorganic', key = 'CUSTOM_SHIFT_I',
          cb = self:callback('initInorganicMode') },
        { icon = ARROW, text = 'creature', key = 'CUSTOM_SHIFT_C',
          cb = self:callback('initCreatureMode')},
        { icon = ARROW, text = 'plant', key = 'CUSTOM_SHIFT_P',
          cb = self:callback('initPlantMode') },
    }

    self:addMaterials(
        choices,
        df.global.world.raws.mat_table.builtin, 0, -1,
        df.builtin_mats._last_item
    )

    self:pushContext('Any material', choices)
end

function MaterialDialog:initInorganicMode()
    local choices = {}

    self:addMaterials(
        choices,
        df.global.world.raws.inorganics, 0, nil,
        nil, 'material'
    )

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
    if #obj.material == 1 then
        self:addMaterial(choices, obj.material[0], typ, index, true)
    else
        table.insert(choices, {
            icon = ARROW, text = name, mat_type = typ, mat_index = index,
            obj = obj, cb = self:callback('onSelectObj')
        })
    end
end

function MaterialDialog:onSelectObj(item)
    local choices = {}
    self:addMaterials(choices, item.obj.material, item.mat_type, item.mat_index)
    self:pushContext(item.text, choices)
end

function MaterialDialog:addMaterials(choices, vector, tid, index, maxid, field)
    for i=0,(maxid or #vector-1) do
        local mat = vector[i]
        if mat and field then
            mat = mat[field]
        end
        if mat then
            local typ, idx
            if index then
                typ, idx = tid+i, index
            else
                typ, idx = tid, i
            end
            self:addMaterial(choices, mat, typ, idx)
        end
    end
end

function MaterialDialog:addMaterial(choices, mat, typ, idx, pfix)
    local state = 0
    if mat.heat.melting_point <= 10015 then
        state = 1
    end
    local name = mat.state_name[state]
    name = string.gsub(name, '^frozen ','')
    name = string.gsub(name, '^molten ','')
    name = string.gsub(name, '^condensed ','')
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
            all_choices = self.all_choices,
            edit_text = self.subviews.edit.text,
            choices = self.choices,
            selected = self.subviews.list.selected,
        })
        self.subviews.back.visible = true
    end

    self.context_str = name
    self.all_choices = choices
    self.subviews.edit.text = ''
    self:setChoices(choices, 1)
end

function MaterialDialog:onGoBack()
    local save = table.remove(self.back_stack)
    self.subviews.back.visible = (#self.back_stack > 0)

    self.context_str = save.context_str
    self.all_choices = save.all_choices
    self.subviews.edit.text = save.edit_text
    self:setChoices(save.choices, save.selected)
end

function MaterialDialog:setChoices(choices, pos)
    self.choices = choices
    self.subviews.list:setChoices(self.choices, pos)
    self.subviews.not_found.visible = (#self.choices == 0)
end

function MaterialDialog:onFilterChoices(text)
    local tokens = utils.split_string(text, ' ')
    local choices = {}

    for i,v in ipairs(self.all_choices) do
        local ok = true
        local search_key = v.search_key or v.text
        for _,key in ipairs(tokens) do
            if key ~= '' and not string.match(search_key, '%f[^%s\x00]'..key) then
                ok = false
                break
            end
        end
        if ok then
            table.insert(choices, v)
        end
    end

    self:setChoices(choices)
end

local bad_chars = {
    ['%'] = true, ['.'] = true, ['+'] = true, ['*'] = true,
    ['['] = true, [']'] = true, ['('] = true, [')'] = true,
}

function MaterialDialog:onFilterChar(char, text)
    if bad_chars[char] then
        return false
    end
    if char == ' ' then
        if #self.choices == 1 then
            self.subviews.list:submit()
            return false
        end
        return string.match(text, '%S$')
    end
    return true
end

function MaterialDialog:submitMaterial(typ, index)
    self:dismiss()

    if self.on_select then
        local info = dfhack.matinfo.decode(typ, index)
        self.on_select(info, typ, index)
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

function showMaterialPrompt(title, prompt, on_select, on_cancel)
    MaterialDialog{
        frame_title = title,
        prompt = prompt,
        on_select = on_select,
        on_cancel = on_cancel,
    }:show()
end

return _ENV
