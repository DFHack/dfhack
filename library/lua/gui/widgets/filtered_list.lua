local utils = require('utils')
local Widget = require('gui.widgets.widget')
local EditField = require('gui.widgets.edit_field')
local List = require('gui.widgets.list')

local getval = utils.getval

---@param tab? table
---@param idx integer
---@return any|integer
local function map_opttab(tab,idx)
    if tab then
        return tab[idx]
    else
        return idx
    end
end

-------------------
-- Filtered List --
-------------------


---@class widgets.FilteredList.attrs: widgets.Widget.attrs
---@field choices widgets.ListChoice[]
---@field selected? integer
---@field edit_pen dfhack.color|dfhack.pen
---@field edit_below boolean
---@field edit_key? string
---@field edit_ignore_keys? string[]
---@field edit_on_char? function
---@field edit_on_change? function
---@field list widgets.List
---@field edit widgets.EditField
---@field not_found widgets.Label

---@class widgets.FilteredList.attrs.partial: widgets.FilteredList.attrs

---@class widgets.FilteredList.initTable: widgets.FilteredList.attrs.partial, widgets.List.attrs.partial, widgets.EditField.attrs.partial
---@field not_found_label? string

---@class widgets.FilteredList: widgets.Widget, widgets.FilteredList.attrs
---@field super widgets.Widget
---@field ATTRS widgets.FilteredList.attrs|fun(attributes: widgets.FilteredList.attrs.partial)
---@overload fun(init_table: widgets.FilteredList.initTable): self
FilteredList = defclass(FilteredList, Widget)

FilteredList.ATTRS {
    edit_below = false,
    edit_key = DEFAULT_NIL,
    edit_ignore_keys = DEFAULT_NIL,
    edit_on_char = DEFAULT_NIL,
    edit_on_change = DEFAULT_NIL,
}

---@param self widgets.FilteredList
---@param info widgets.FilteredList.initTable
function FilteredList:init(info)
    local on_change = self:callback('onFilterChange')
    if self.edit_on_change then
        on_change = function(text)
            self.edit_on_change(text)
            self:onFilterChange(text)
        end
    end

    self.edit = EditField{
        text_pen = info.edit_pen or info.cursor_pen,
        frame = { l = info.icon_width, t = 0, h = 1 },
        on_change = on_change,
        on_char = self.edit_on_char,
        key = self.edit_key,
        ignore_keys = self.edit_ignore_keys,
    }
    self.list = List{
        frame = { t = 2 },
        text_pen = info.text_pen,
        cursor_pen = info.cursor_pen,
        inactive_pen = info.inactive_pen,
        icon_pen = info.icon_pen,
        row_height = info.row_height,
        scroll_keys = info.scroll_keys,
        icon_width = info.icon_width,
    }
    if self.edit_below then
        self.edit.frame = { l = info.icon_width, b = 0, h = 1 }
        self.list.frame = { t = 0, b = 2 }
    end
    if info.on_select then
        self.list.on_select = function()
            return info.on_select(self:getSelected())
        end
    end
    if info.on_submit then
        self.list.on_submit = function()
            return info.on_submit(self:getSelected())
        end
    end
    if info.on_submit2 then
        self.list.on_submit2 = function()
            return info.on_submit2(self:getSelected())
        end
    end
    if info.on_double_click then
        self.list.on_double_click = function()
            return info.on_double_click(self:getSelected())
        end
    end
    if info.on_double_click2 then
        self.list.on_double_click2 = function()
            return info.on_double_click2(self:getSelected())
        end
    end
    self.not_found = Label{
        visible = true,
        text = info.not_found_label or 'No matches',
        text_pen = COLOR_LIGHTRED,
        frame = { l = info.icon_width, t = self.list.frame.t },
    }
    self:addviews{ self.edit, self.list, self.not_found }
    if info.choices then
        self:setChoices(info.choices, info.selected)
    else
        self.choices = {}
    end
end

function FilteredList:getChoices()
    return self.choices
end

function FilteredList:getVisibleChoices()
    return self.list.choices
end

function FilteredList:setChoices(choices, pos)
    choices = choices or {}
    self.edit:setText('')
    self.list:setChoices(choices, pos)
    self.choices = self.list.choices
    self.not_found.visible = (#choices == 0)
end

function FilteredList:submit()
    return self.list:submit()
end

function FilteredList:submit2()
    return self.list:submit2()
end

function FilteredList:canSubmit()
    return not self.not_found.visible
end

function FilteredList:getSelected()
    local i,v = self.list:getSelected()
    if i then
        return map_opttab(self.choice_index, i), v
    end
end

function FilteredList:getContentWidth()
    return self.list:getContentWidth()
end

function FilteredList:getContentHeight()
    return self.list:getContentHeight() + 2
end

function FilteredList:getFilter()
    return self.edit.text, self.list.choices
end

function FilteredList:setFilter(filter, pos)
    local choices = self.choices
    local cidx = nil

    filter = filter or ''
    if filter ~= self.edit.text then
        self.edit:setText(filter)
    end

    if filter ~= '' then
        local tokens = filter:split()
        local ipos = pos

        choices = {}
        cidx = {}
        pos = nil

        for i,v in ipairs(self.choices) do
            local search_key = getval(v.search_key)
            if not search_key then
                if type(v.text) ~= 'table' then
                    search_key = v.text
                else
                    local texts = {}
                    for _,token in ipairs(v.text) do
                        table.insert(texts,
                            type(token) == 'string' and token
                                or getval(token.text) or '')
                    end
                    search_key = table.concat(texts, ' ')
                end
            end
            if utils.search_text(search_key, tokens) then
                table.insert(choices, v)
                cidx[#choices] = i
                if ipos == i then
                    pos = #choices
                end
            end
        end
    end

    self.choice_index = cidx
    self.list:setChoices(choices, pos)
    self.not_found.visible = (#choices == 0)
end

function FilteredList:onFilterChange(text)
    self:setFilter(text)
end

return FilteredList
