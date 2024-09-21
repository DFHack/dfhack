-- Simple widgets for screens

local _ENV = mkmodule('gui.widgets')

local gui = require('gui')
local guidm = require('gui.dwarfmode')
local textures = require('gui.textures')
local utils = require('utils')

Widget = require('gui.widgets.widget')
Divider = require('gui.widgets.divider')
Panel = require('gui.widgets.panel')
Window = require('gui.widgets.window')
ResizingPanel = require('gui.widgets.resizing_panel')
Pages = require('gui.widgets.pages')
EditField = require('gui.widgets.edit_field')
HotkeyLabel = require('gui.widgets.hotkey_label')
Label = require('gui.widgets.label')
Scrollbar = require('gui.widgets.scrollbar')
WrappedLabel = require('gui.widgets.wrapped_label')
TooltipLabel = require('gui.widgets.tooltip_label')
HelpButton = require('gui.widgets.help_button')
ConfigureButton = require('gui.widgets.configure_button')
BannerPanel = require('gui.widgets.banner_panel')

makeButtonLabelText = Label.makeButtonLabelText
parse_label_text = Label.parse_label_text

local getval = utils.getval
local to_pen = dfhack.pen.parse

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

----------------
-- TextButton --
----------------

---@class widgets.TextButton.initTable: widgets.Panel.attrs.partial, widgets.HotkeyLabel.attrs.partial

---@class widgets.TextButton: widgets.BannerPanel
---@field super widgets.BannerPanel
---@overload fun(init_table: widgets.TextButton.initTable): self
TextButton = defclass(TextButton, BannerPanel)

---@param info widgets.TextButton.initTable
function TextButton:init(info)
    self.label = HotkeyLabel{
        frame={t=0, l=1, r=1},
        key=info.key,
        key_sep=info.key_sep,
        label=info.label,
        on_activate=info.on_activate,
        text_pen=info.text_pen,
        text_dpen=info.text_dpen,
        text_hpen=info.text_hpen,
        disabled=info.disabled,
        enabled=info.enabled,
        auto_height=info.auto_height,
        auto_width=info.auto_width,
        on_click=info.on_click,
        on_rclick=info.on_rclick,
        scroll_keys=info.scroll_keys,
    }

    self:addviews{self.label}
end

function TextButton:setLabel(label)
    self.label:setLabel(label)
end

----------------------
-- CycleHotkeyLabel --
----------------------

---@class widgets.CycleHotkeyLabel.attrs: widgets.Label.attrs
---@field key? string
---@field key_back? string
---@field key_sep string
---@field label? string|fun(): string
---@field label_width? integer
---@field label_below boolean
---@field option_gap integer
---@field options? { label: string, value: string, pen: dfhack.pen|nil }[]
---@field initial_option integer|string
---@field on_change? fun(new_option_value: integer|string, old_option_value: integer|string)

---@class widgets.CycleHotkeyLabel.attrs.partial: widgets.CycleHotkeyLabel.attrs

---@class widgets.CycleHotkeyLabel: widgets.Label, widgets.CycleHotkeyLabel.attrs
---@field super widgets.Label
---@field ATTRS widgets.CycleHotkeyLabel.attrs|fun(attributes: widgets.CycleHotkeyLabel.attrs.partial)
---@overload fun(init_table: widgets.CycleHotkeyLabel.attrs.partial)
CycleHotkeyLabel = defclass(CycleHotkeyLabel, Label)

CycleHotkeyLabel.ATTRS{
    key=DEFAULT_NIL,
    key_back=DEFAULT_NIL,
    key_sep=': ',
    option_gap=1,
    label=DEFAULT_NIL,
    label_width=DEFAULT_NIL,
    label_below=false,
    options=DEFAULT_NIL,
    initial_option=1,
    on_change=DEFAULT_NIL,
}

function CycleHotkeyLabel:init()
    self:setOption(self.initial_option)

    if self.label_below then
        self.option_gap = self.option_gap + (self.key_back and 1 or 0) + (self.key and 2 or 0)
    end

    self:setText{
        self.key_back ~= nil and {key=self.key_back, key_sep='', width=0, on_activate=self:callback('cycle', true)} or {},
        {key=self.key, key_sep=self.key_sep, text=self.label, width=self.label_width,
         on_activate=self:callback('cycle')},
        self.label_below and NEWLINE or '',
        {gap=self.option_gap, text=self:callback('getOptionLabel'),
         pen=self:callback('getOptionPen')},
    }
end

-- CycleHotkeyLabels are always clickable and therefore should always change
-- color when hovered.
function CycleHotkeyLabel:shouldHover()
    return true
end

function CycleHotkeyLabel:cycle(backwards)
    local old_option_idx = self.option_idx
    if self.option_idx == #self.options and not backwards then
        self.option_idx = 1
    elseif self.option_idx == 1 and backwards then
        self.option_idx = #self.options
    else
        self.option_idx = self.option_idx + (not backwards and 1 or -1)
    end
    if self.on_change then
        self.on_change(self:getOptionValue(),
                       self:getOptionValue(old_option_idx))
    end
end

function CycleHotkeyLabel:setOption(value_or_index, call_on_change)
    local option_idx = nil
    for i in ipairs(self.options) do
        if value_or_index == self:getOptionValue(i) then
            option_idx = i
            break
        end
    end
    if not option_idx then
        if self.options[value_or_index] then
            option_idx = value_or_index
        end
    end
    if not option_idx then
        option_idx = 1
    end
    local old_option_idx = self.option_idx
    self.option_idx = option_idx
    if call_on_change and self.on_change then
        self.on_change(self:getOptionValue(),
                       self:getOptionValue(old_option_idx))
    end
end

local function cyclehotkeylabel_getOptionElem(self, option_idx, key, require_key)
    option_idx = option_idx or self.option_idx
    local option = self.options[option_idx]
    if type(option) == 'table' then
        return option[key]
    end
    return not require_key and option or nil
end

function CycleHotkeyLabel:getOptionLabel(option_idx)
    return getval(cyclehotkeylabel_getOptionElem(self, option_idx, 'label'))
end

function CycleHotkeyLabel:getOptionValue(option_idx)
    return cyclehotkeylabel_getOptionElem(self, option_idx, 'value')
end

function CycleHotkeyLabel:getOptionPen(option_idx)
    local pen = getval(cyclehotkeylabel_getOptionElem(self, option_idx, 'pen', true))
    if type(pen) == 'string' then return nil end
    return pen
end

function CycleHotkeyLabel:onInput(keys)
    if CycleHotkeyLabel.super.onInput(self, keys) then
        return true
    elseif keys._MOUSE_L and not is_disabled(self) then
        local x = self:getMousePos()
        if x then
            self:cycle(self.key_back and x == 0)
            return true
        end
    end
end

-----------------
-- ButtonGroup --
-----------------

---@class widgets.ButtonGroup.attrs: widgets.CycleHotkeyLabel.attrs

---@class widgets.ButtonGroup.initTable: widgets.ButtonGroup.attrs
---@field button_specs widgets.ButtonLabelSpec[]
---@field button_specs_selected widgets.ButtonLabelSpec[]

---@class widgets.ButtonGroup: widgets.ButtonGroup.attrs, widgets.CycleHotkeyLabel
---@field super widgets.CycleHotkeyLabel
---@overload fun(init_table: widgets.ButtonGroup.initTable): self
ButtonGroup = defclass(ButtonGroup, CycleHotkeyLabel)

ButtonGroup.ATTRS{
    auto_height=false,
}

function ButtonGroup:init(info)
    ensure_key(self, 'frame').h = #info.button_specs[1].chars + 2

    local start = 0
    for idx in ipairs(self.options) do
        local opt_val = self:getOptionValue(idx)
        local width = #info.button_specs[idx].chars[1]
        self:addviews{
            Label{
                frame={t=2, l=start, w=width},
                text=makeButtonLabelText(info.button_specs[idx]),
                on_click=function() self:setOption(opt_val, true) end,
                visible=function() return self:getOptionValue() ~= opt_val end,
            },
            Label{
                frame={t=2, l=start, w=width},
                text=makeButtonLabelText(info.button_specs_selected[idx]),
                on_click=function() self:setOption(opt_val, false) end,
                visible=function() return self:getOptionValue() == opt_val end,
            },
        }
        start = start + width
    end
end

-----------------------
-- ToggleHotkeyLabel --
-----------------------

---@class widgets.ToggleHotkeyLabel: widgets.CycleHotkeyLabel
---@field super widgets.CycleHotkeyLabel
ToggleHotkeyLabel = defclass(ToggleHotkeyLabel, CycleHotkeyLabel)
ToggleHotkeyLabel.ATTRS{
    options={{label='On', value=true, pen=COLOR_GREEN},
             {label='Off', value=false}},
}

----------
-- List --
----------

---@class widgets.ListChoice
---@field text string|widgets.LabelToken[]
---@field key string
---@field search_key? string
---@field icon? string|dfhack.pen|fun(): string|dfhack.pen
---@field icon_pen? dfhack.pen

---@class widgets.List.attrs: widgets.Widget.attrs
---@field choices widgets.ListChoice[]
---@field selected integer
---@field text_pen dfhack.color|dfhack.pen
---@field text_hpen? dfhack.color|dfhack.pen
---@field cursor_pen dfhack.color|dfhack.pen
---@field inactive_pen? dfhack.color|dfhack.pen
---@field icon_pen? dfhack.color|dfhack.pen
---@field on_select? function
---@field on_submit? function
---@field on_submit2? function
---@field on_double_click? function
---@field on_double_click2? function
---@field row_height integer
---@field scroll_keys table<string, string|integer>
---@field icon_width? integer
---@field page_top integer
---@field page_size integer
---@field scrollbar widgets.Scrollbar
---@field last_select_click_ms integer

---@class widgets.List.attrs.partial: widgets.List.attrs

---@class widgets.List: widgets.Widget, widgets.List.attrs
---@field super widgets.Widget
---@field ATTRS widgets.List.attrs|fun(attributes: widgets.List.attrs.partial)
---@overload fun(init_table: widgets.List.attrs.partial): self
List = defclass(List, Widget)

List.ATTRS{
    text_pen = COLOR_CYAN,
    text_hpen = DEFAULT_NIL, -- hover color, defaults to inverting the FG/BG pens for each text object
    cursor_pen = COLOR_LIGHTCYAN,
    inactive_pen = DEFAULT_NIL,
    icon_pen = DEFAULT_NIL,
    on_select = DEFAULT_NIL,
    on_submit = DEFAULT_NIL,
    on_submit2 = DEFAULT_NIL,
    on_double_click = DEFAULT_NIL,
    on_double_click2 = DEFAULT_NIL,
    row_height = 1,
    scroll_keys = STANDARDSCROLL,
    icon_width = DEFAULT_NIL,
}

---@param self widgets.List
---@param info widgets.List.attrs.partial
function List:init(info)
    self.page_top = 1
    self.page_size = 1
    self.scrollbar = Scrollbar{
        frame={r=0},
        on_scroll=self:callback('on_scrollbar')}

    self:addviews{self.scrollbar}

    if info.choices then
        self:setChoices(info.choices, info.selected)
    else
        self.choices = {}
        self.selected = 1
    end

    self.last_select_click_ms = 0 -- used to track double-clicking on an item
end

function List:setChoices(choices, selected)
    self.choices = {}

    for i,v in ipairs(choices or {}) do
        local l = utils.clone(v);
        if type(v) ~= 'table' then
            l = { text = v }
        else
            l.text = v.text or v.caption or v[1]
        end
        parse_label_text(l)
        self.choices[i] = l
    end

    self:setSelected(selected)

    -- Check if page_top needs to be adjusted
    if #self.choices - self.page_size < 0 then
        self.page_top = 1
    elseif self.page_top > #self.choices - self.page_size + 1 then
        self.page_top = #self.choices - self.page_size + 1
    end
end

function List:setSelected(selected)
    self.selected = selected or self.selected or 1
    self:moveCursor(0, true)
    return self.selected
end

function List:getChoices()
    return self.choices
end

function List:getSelected()
    if #self.choices > 0 then
        return self.selected, self.choices[self.selected]
    end
end

function List:getContentWidth()
    local width = 0
    for i,v in ipairs(self.choices) do
        render_text(v)
        local roww = v.text_width
        if v.key then
            roww = roww + 3 + #gui.getKeyDisplay(v.key)
        end
        width = math.max(width, roww)
    end
    return width + (self.icon_width or 0)
end

function List:getContentHeight()
    return #self.choices * self.row_height
end

local function update_list_scrollbar(list)
    list.scrollbar:update(list.page_top, list.page_size, #list.choices)
end

function List:postComputeFrame(body)
    local row_count = body.height // self.row_height
    self.page_size = math.max(1, row_count)

    local num_choices = #self.choices
    if num_choices == 0 then
        self.page_top = 1
        update_list_scrollbar(self)
        return
    end

    if self.page_top > num_choices - self.page_size + 1 then
        self.page_top = math.max(1, num_choices - self.page_size + 1)
    end

    update_list_scrollbar(self)
end

function List:postUpdateLayout()
    update_list_scrollbar(self)
end

function List:moveCursor(delta, force_cb)
    local cnt = #self.choices

    if cnt < 1 then
        self.page_top = 1
        self.selected = 1
        update_list_scrollbar(self)
        if force_cb and self.on_select then
            self.on_select(nil,nil)
        end
        return
    end

    local off = self.selected+delta-1
    local ds = math.abs(delta)

    if ds > 1 then
        if off >= cnt+ds-1 then
            off = 0
        else
            off = math.min(cnt-1, off)
        end
        if off <= -ds then
            off = cnt-1
        else
            off = math.max(0, off)
        end
    end

    local buffer = 1 + math.min(4, math.floor(self.page_size/10))

    self.selected = 1 + off % cnt
    if (self.selected - buffer) < self.page_top then
        self.page_top = math.max(1, self.selected - buffer)
    elseif (self.selected + buffer + 1) > (self.page_top + self.page_size) then
        local max_page_top = cnt - self.page_size + 1
        self.page_top = math.max(1,
            math.min(max_page_top, self.selected - self.page_size + buffer + 1))
    end
    update_list_scrollbar(self)

    if (force_cb or delta ~= 0) and self.on_select then
        self.on_select(self:getSelected())
    end
end

function List:on_scrollbar(scroll_spec)
    local v = 0
    if tonumber(scroll_spec) then
        v = scroll_spec - self.page_top
    elseif scroll_spec == 'down_large' then
        v = math.ceil(self.page_size / 2)
    elseif scroll_spec == 'up_large' then
        v = -math.ceil(self.page_size / 2)
    elseif scroll_spec == 'down_small' then
        v = 1
    elseif scroll_spec == 'up_small' then
        v = -1
    end

    local max_page_top = math.max(1, #self.choices - self.page_size + 1)
    self.page_top = math.max(1, math.min(max_page_top, self.page_top + v))
    update_list_scrollbar(self)
end

function List:onRenderBody(dc)
    local choices = self.choices
    local top = self.page_top
    local iend = math.min(#choices, top+self.page_size-1)
    local iw = self.icon_width

    local function paint_icon(icon, obj)
        if type(icon) ~= 'string' then
            dc:char(nil,icon)
        else
            if current then
                dc:string(icon, obj.icon_pen or self.icon_pen or cur_pen)
            else
                dc:string(icon, obj.icon_pen or self.icon_pen or cur_dpen)
            end
        end
    end

    local hoveridx = self:getIdxUnderMouse()
    for i = top,iend do
        local obj = choices[i]
        local current = (i == self.selected)
        local hovered = (i == hoveridx)
        -- cur_pen and cur_dpen can't be integers or background colors get
        -- messed up in render_text for subsequent renders
        local cur_pen = to_pen(self.cursor_pen)
        local cur_dpen = to_pen(self.text_pen)
        local active_pen = (current and cur_pen or cur_dpen)

        if not getval(self.active) then
            cur_pen = self.inactive_pen or self.cursor_pen
        end

        local y = (i - top)*self.row_height
        local icon = getval(obj.icon)

        if iw and icon then
            dc:seek(0, y):pen(active_pen)
            paint_icon(icon, obj)
        end

        render_text(obj, dc, iw or 0, y, cur_pen, cur_dpen, not current, self.text_hpen, hovered)

        local ip = dc.width

        if obj.key then
            local keystr = gui.getKeyDisplay(obj.key)
            ip = ip-3-#keystr
            dc:seek(ip,y):pen(self.text_pen)
            dc:string('('):string(keystr,COLOR_LIGHTGREEN):string(')')
        end

        if icon and not iw then
            dc:seek(ip-1,y):pen(active_pen)
            paint_icon(icon, obj)
        end
    end
end

function List:getIdxUnderMouse()
    if self.scrollbar:getMousePos() then return end
    local _,mouse_y = self:getMousePos()
    if mouse_y and #self.choices > 0 and
            mouse_y < (#self.choices-self.page_top+1) * self.row_height then
        return self.page_top + math.floor(mouse_y/self.row_height)
    end
end

function List:submit()
    if self.on_submit and #self.choices > 0 then
        self.on_submit(self:getSelected())
        return true
    end
end

function List:submit2()
    if self.on_submit2 and #self.choices > 0 then
        self.on_submit2(self:getSelected())
        return true
    end
end

function List:double_click()
    if #self.choices == 0 then return end
    local cb = dfhack.internal.getModifiers().shift and
            self.on_double_click2 or self.on_double_click
    if cb then
        cb(self:getSelected())
        return true
    end
end

function List:onInput(keys)
    if self:inputToSubviews(keys) then
        return true
    end
    if keys.SELECT then
        return self:submit()
    elseif keys.SELECT_ALL then
        return self:submit2()
    elseif keys._MOUSE_L then
        local idx = self:getIdxUnderMouse()
        if idx then
            local now_ms = dfhack.getTickCount()
            if idx ~= self:getSelected() then
                self.last_select_click_ms = now_ms
            else
                if now_ms - self.last_select_click_ms <= DOUBLE_CLICK_MS then
                    self.last_select_click_ms = 0
                    if self:double_click() then return true end
                else
                    self.last_select_click_ms = now_ms
                end
            end

            self:setSelected(idx)
            if dfhack.internal.getModifiers().shift then
                self:submit2()
            else
                self:submit()
            end
            return true
        end
    else
        for k,v in pairs(self.scroll_keys) do
            if keys[k] then
                if v == '+page' then
                    v = self.page_size
                elseif v == '-page' then
                    v = -self.page_size
                end

                self:moveCursor(v)
                return true
            end
        end

        for i,v in ipairs(self.choices) do
            if v.key and keys[v.key] then
                self:setSelected(i)
                self:submit()
                return true
            end
        end

        local current = self.choices[self.selected]
        if current then
            return check_text_keys(current, keys)
        end
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
            local search_key = v.search_key
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

---@class widgets.TabPens
---@field text_mode_tab_pen dfhack.pen
---@field text_mode_label_pen dfhack.pen
---@field lt dfhack.pen
---@field lt2 dfhack.pen
---@field t dfhack.pen
---@field rt2 dfhack.pen
---@field rt dfhack.pen
---@field lb dfhack.pen
---@field lb2 dfhack.pen
---@field b dfhack.pen
---@field rb2 dfhack.pen
---@field rb dfhack.pen

local TSO = df.global.init.tabs_texpos[0] -- tab spritesheet offset
local DEFAULT_ACTIVE_TAB_PENS = {
    text_mode_tab_pen=to_pen{fg=COLOR_YELLOW},
    text_mode_label_pen=to_pen{fg=COLOR_WHITE},
    lt=to_pen{tile=TSO+5, write_to_lower=true},
    lt2=to_pen{tile=TSO+6, write_to_lower=true},
    t=to_pen{tile=TSO+7, fg=COLOR_BLACK, write_to_lower=true, top_of_text=true},
    rt2=to_pen{tile=TSO+8, write_to_lower=true},
    rt=to_pen{tile=TSO+9, write_to_lower=true},
    lb=to_pen{tile=TSO+15, write_to_lower=true},
    lb2=to_pen{tile=TSO+16, write_to_lower=true},
    b=to_pen{tile=TSO+17, fg=COLOR_BLACK, write_to_lower=true, bottom_of_text=true},
    rb2=to_pen{tile=TSO+18, write_to_lower=true},
    rb=to_pen{tile=TSO+19, write_to_lower=true},
}

local DEFAULT_INACTIVE_TAB_PENS = {
    text_mode_tab_pen=to_pen{fg=COLOR_BROWN},
    text_mode_label_pen=to_pen{fg=COLOR_DARKGREY},
    lt=to_pen{tile=TSO+0, write_to_lower=true},
    lt2=to_pen{tile=TSO+1, write_to_lower=true},
    t=to_pen{tile=TSO+2, fg=COLOR_WHITE, write_to_lower=true, top_of_text=true},
    rt2=to_pen{tile=TSO+3, write_to_lower=true},
    rt=to_pen{tile=TSO+4, write_to_lower=true},
    lb=to_pen{tile=TSO+10, write_to_lower=true},
    lb2=to_pen{tile=TSO+11, write_to_lower=true},
    b=to_pen{tile=TSO+12, fg=COLOR_WHITE, write_to_lower=true, bottom_of_text=true},
    rb2=to_pen{tile=TSO+13, write_to_lower=true},
    rb=to_pen{tile=TSO+14, write_to_lower=true},
}

---------
-- Tab --
---------

---@class widgets.Tab.attrs: widgets.Widget.attrs
---@field id? string|integer
---@field label string
---@field on_select? function
---@field get_pens? fun(): widgets.TabPens

---@class widgets.Tab.attrs.partial: widgets.Tab.attrs

---@class widgets.Tab.initTable: widgets.Tab.attrs
---@field label string

---@class widgets.Tab: widgets.Widget, widgets.Tab.attrs
---@field super widgets.Widget
---@field ATTRS widgets.Tab.attrs|fun(attributes: widgets.Tab.attrs.partial)
---@overload fun(init_table: widgets.Tab.initTable): self
Tab = defclass(Tabs, Widget)
Tab.ATTRS{
    id=DEFAULT_NIL,
    label=DEFAULT_NIL,
    on_select=DEFAULT_NIL,
    get_pens=DEFAULT_NIL,
}

function Tab:preinit(init_table)
    init_table.frame = init_table.frame or {}
    init_table.frame.w = #init_table.label + 4
    init_table.frame.h = 2
end

function Tab:onRenderBody(dc)
    local pens = self.get_pens()
    dc:seek(0, 0)
    if dfhack.screen.inGraphicsMode() then
        dc:char(nil, pens.lt):char(nil, pens.lt2)
        for i=1,#self.label do
            dc:char(self.label:sub(i,i), pens.t)
        end
        dc:char(nil, pens.rt2):char(nil, pens.rt)
        dc:seek(0, 1)
        dc:char(nil, pens.lb):char(nil, pens.lb2)
        for i=1,#self.label do
            dc:char(self.label:sub(i,i), pens.b)
        end
        dc:char(nil, pens.rb2):char(nil, pens.rb)
    else
        local tp = pens.text_mode_tab_pen
        dc:char(' ', tp):char('/', tp)
        for i=1,#self.label do
            dc:char('-', tp)
        end
        dc:char('\\', tp):char(' ', tp)
        dc:seek(0, 1)
        dc:char('/', tp):char('-', tp)
        dc:string(self.label, pens.text_mode_label_pen)
        dc:char('-', tp):char('\\', tp)
    end
end

function Tab:onInput(keys)
    if Tab.super.onInput(self, keys) then return true end
    if keys._MOUSE_L and self:getMousePos() then
        self.on_select(self.id)
        return true
    end
end

-------------
-- Tab Bar --
-------------

---@class widgets.TabBar.attrs: widgets.ResizingPanel.attrs
---@field labels string[]
---@field on_select? function
---@field get_cur_page? function
---@field active_tab_pens widgets.TabPens
---@field inactive_tab_pens widgets.TabPens
---@field get_pens? fun(index: integer, tabbar: self): widgets.TabPens
---@field key string
---@field key_back string

---@class widgets.TabBar.attrs.partial: widgets.TabBar.attrs

---@class widgets.TabBar.initTable: widgets.TabBar.attrs
---@field labels string[]

---@class widgets.TabBar: widgets.ResizingPanel, widgets.TabBar.attrs
---@field super widgets.ResizingPanel
---@field ATTRS widgets.TabBar.attrs|fun(attribute: widgets.TabBar.attrs.partial)
---@overload fun(init_table: widgets.TabBar.initTable): self
TabBar = defclass(TabBar, ResizingPanel)
TabBar.ATTRS{
    labels=DEFAULT_NIL,
    on_select=DEFAULT_NIL,
    get_cur_page=DEFAULT_NIL,
    active_tab_pens=DEFAULT_ACTIVE_TAB_PENS,
    inactive_tab_pens=DEFAULT_INACTIVE_TAB_PENS,
    get_pens=DEFAULT_NIL,
    key='CUSTOM_CTRL_T',
    key_back='CUSTOM_CTRL_Y',
}

---@param self widgets.TabBar
function TabBar:init()
    for idx,label in ipairs(self.labels) do
        self:addviews{
            Tab{
                frame={t=0, l=0},
                id=idx,
                label=label,
                on_select=self.on_select,
                get_pens=self.get_pens and function()
                    return self.get_pens(idx, self)
                end or function()
                    if self.get_cur_page() == idx then
                        return self.active_tab_pens
                    end

                    return self.inactive_tab_pens
                end,
            }
        }
    end
end

function TabBar:postComputeFrame(body)
    local t, l, width = 0, 0, body.width
    for _,tab in ipairs(self.subviews) do
        if l > 0 and l + tab.frame.w > width then
            t = t + 2
            l = 0
        end
        tab.frame.t = t
        tab.frame.l = l
        l = l + tab.frame.w
    end
end

function TabBar:onInput(keys)
    if TabBar.super.onInput(self, keys) then return true end
    if self.key and keys[self.key] then
        local zero_idx = self.get_cur_page() - 1
        local next_zero_idx = (zero_idx + 1) % #self.labels
        self.on_select(next_zero_idx + 1)
        return true
    end
    if self.key_back and keys[self.key_back] then
        local zero_idx = self.get_cur_page() - 1
        local prev_zero_idx = (zero_idx - 1) % #self.labels
        self.on_select(prev_zero_idx + 1)
        return true
    end
end

--------------------------------
-- RangeSlider
--------------------------------

---@class widgets.RangeSlider.attrs: widgets.Widget.attrs
---@field num_stops integer
---@field get_left_idx_fn? function
---@field get_right_idx_fn? function
---@field on_left_change? fun(index: integer)
---@field on_right_change? fun(index: integer)

---@class widgets.RangeSlider.attrs.partial: widgets.RangeSlider.attrs

---@class widgets.RangeSlider.initTable: widgets.RangeSlider.attrs
---@field num_stops integer

---@class widgets.RangeSlider: widgets.Widget, widgets.RangeSlider.attrs
---@field super widgets.Widget
---@field ATTRS widgets.RangeSlider.attrs|fun(attributes: widgets.RangeSlider.attrs.partial)
---@overload fun(init_table: widgets.RangeSlider.initTable): self
RangeSlider = defclass(RangeSlider, Widget)
RangeSlider.ATTRS{
    num_stops=DEFAULT_NIL,
    get_left_idx_fn=DEFAULT_NIL,
    get_right_idx_fn=DEFAULT_NIL,
    on_left_change=DEFAULT_NIL,
    on_right_change=DEFAULT_NIL,
}

function RangeSlider:preinit(init_table)
    init_table.frame = init_table.frame or {}
    init_table.frame.h = init_table.frame.h or 1
end

function RangeSlider:init()
    if self.num_stops < 2 then error('too few RangeSlider stops') end
    self.is_dragging_target = nil -- 'left', 'right', or 'both'
    self.is_dragging_idx = nil -- offset from leftmost dragged tile
end

local function rangeslider_get_width_per_idx(self)
    return math.max(5, (self.frame_body.width-7) // (self.num_stops-1))
end

function RangeSlider:onInput(keys)
    if not keys._MOUSE_L then return false end
    local x = self:getMousePos()
    if not x then return false end
    local left_idx, right_idx = self.get_left_idx_fn(), self.get_right_idx_fn()
    local width_per_idx = rangeslider_get_width_per_idx(self)
    local left_pos = width_per_idx*(left_idx-1)
    local right_pos = width_per_idx*(right_idx-1) + 4
    if x < left_pos then
        self.on_left_change(self.get_left_idx_fn() - 1)
    elseif x < left_pos+3 then
        self.is_dragging_target = 'left'
        self.is_dragging_idx = x - left_pos
    elseif x < right_pos then
        self.is_dragging_target = 'both'
        self.is_dragging_idx = x - left_pos
    elseif x < right_pos+3 then
        self.is_dragging_target = 'right'
        self.is_dragging_idx = x - right_pos
    else
        self.on_right_change(self.get_right_idx_fn() + 1)
    end
    return true
end

local function rangeslider_do_drag(self, width_per_idx)
    local x = self.frame_body:localXY(dfhack.screen.getMousePos())
    local cur_pos = x - self.is_dragging_idx
    cur_pos = math.max(0, cur_pos)
    cur_pos = math.min(width_per_idx*(self.num_stops-1)+7, cur_pos)
    local offset = self.is_dragging_target == 'right' and -2 or 1
    local new_idx = math.max(0, cur_pos+offset)//width_per_idx + 1
    local new_left_idx, new_right_idx
    if self.is_dragging_target == 'right' then
        new_right_idx = new_idx
    else
        new_left_idx = new_idx
        if self.is_dragging_target == 'both' then
            new_right_idx = new_left_idx + self.get_right_idx_fn() - self.get_left_idx_fn()
            if new_right_idx > self.num_stops then
                return
            end
        end
    end
    if new_left_idx and new_left_idx ~= self.get_left_idx_fn() then
        if not new_right_idx and new_left_idx > self.get_right_idx_fn() then
            self.on_right_change(new_left_idx)
        end
        self.on_left_change(new_left_idx)
    end
    if new_right_idx and new_right_idx ~= self.get_right_idx_fn() then
        if new_right_idx < self.get_left_idx_fn() then
            self.on_left_change(new_right_idx)
        end
        self.on_right_change(new_right_idx)
    end
end

local SLIDER_LEFT_END = to_pen{ch=198, fg=COLOR_GREY, bg=COLOR_BLACK}
local SLIDER_TRACK = to_pen{ch=205, fg=COLOR_GREY, bg=COLOR_BLACK}
local SLIDER_TRACK_SELECTED = to_pen{ch=205, fg=COLOR_LIGHTGREEN, bg=COLOR_BLACK}
local SLIDER_TRACK_STOP = to_pen{ch=216, fg=COLOR_GREY, bg=COLOR_BLACK}
local SLIDER_TRACK_STOP_SELECTED = to_pen{ch=216, fg=COLOR_LIGHTGREEN, bg=COLOR_BLACK}
local SLIDER_RIGHT_END = to_pen{ch=181, fg=COLOR_GREY, bg=COLOR_BLACK}
local SLIDER_TAB_LEFT = to_pen{ch=60, fg=COLOR_BLACK, bg=COLOR_YELLOW}
local SLIDER_TAB_CENTER = to_pen{ch=9, fg=COLOR_BLACK, bg=COLOR_YELLOW}
local SLIDER_TAB_RIGHT = to_pen{ch=62, fg=COLOR_BLACK, bg=COLOR_YELLOW}

function RangeSlider:onRenderBody(dc, rect)
    local left_idx, right_idx = self.get_left_idx_fn(), self.get_right_idx_fn()
    local width_per_idx = rangeslider_get_width_per_idx(self)
    -- draw track
    dc:seek(1,0)
    dc:char(nil, SLIDER_LEFT_END)
    dc:char(nil, SLIDER_TRACK)
    for stop_idx=1,self.num_stops-1 do
        local track_stop_pen = SLIDER_TRACK_STOP_SELECTED
        local track_pen = SLIDER_TRACK_SELECTED
        if left_idx > stop_idx or right_idx < stop_idx then
            track_stop_pen = SLIDER_TRACK_STOP
            track_pen = SLIDER_TRACK
        elseif right_idx == stop_idx then
            track_pen = SLIDER_TRACK
        end
        dc:char(nil, track_stop_pen)
        for i=2,width_per_idx do
            dc:char(nil, track_pen)
        end
    end
    if right_idx >= self.num_stops then
        dc:char(nil, SLIDER_TRACK_STOP_SELECTED)
    else
        dc:char(nil, SLIDER_TRACK_STOP)
    end
    dc:char(nil, SLIDER_TRACK)
    dc:char(nil, SLIDER_RIGHT_END)
    -- draw tabs
    dc:seek(width_per_idx*(left_idx-1))
    dc:char(nil, SLIDER_TAB_LEFT)
    dc:char(nil, SLIDER_TAB_CENTER)
    dc:char(nil, SLIDER_TAB_RIGHT)
    dc:seek(width_per_idx*(right_idx-1)+4)
    dc:char(nil, SLIDER_TAB_LEFT)
    dc:char(nil, SLIDER_TAB_CENTER)
    dc:char(nil, SLIDER_TAB_RIGHT)
    -- manage dragging
    if self.is_dragging_target then
        rangeslider_do_drag(self, width_per_idx)
    end
    if df.global.enabler.mouse_lbut_down == 0 then
        self.is_dragging_target = nil
        self.is_dragging_idx = nil
    end
end

--------------------------------
-- DimensionsTooltip
--------------------------------

---@class widgets.DimensionsTooltip.attrs: widgets.ResizingPanel.attrs
---@field display_offset? df.coord2d
---@field get_anchor_pos_fn fun(): df.coord?

---@class widgets.DimensionsTooltip: widgets.ResizingPanel
---@field super widgets.ResizingPanel
---@overload fun(init_table: widgets.DimensionsTooltip.attrs): self
DimensionsTooltip = defclass(DimensionsTooltip, ResizingPanel)

DimensionsTooltip.ATTRS{
    frame_style=gui.FRAME_THIN,
    frame_background=gui.CLEAR_PEN,
    no_force_pause_badge=true,
    auto_width=true,
    display_offset={x=3, y=3},
    get_anchor_pos_fn=DEFAULT_NIL,
}

local function get_cur_area_dims(anchor_pos)
    local mouse_pos = dfhack.gui.getMousePos(true)
    if not mouse_pos or not anchor_pos then return 1, 1, 1 end

    -- clamp to map edges (you can start a selection from out of bounds)
    mouse_pos = xyz2pos(
        math.max(0, math.min(df.global.world.map.x_count-1, mouse_pos.x)),
        math.max(0, math.min(df.global.world.map.y_count-1, mouse_pos.y)),
        math.max(0, math.min(df.global.world.map.z_count-1, mouse_pos.z)))
    anchor_pos = xyz2pos(
        math.max(0, math.min(df.global.world.map.x_count-1, anchor_pos.x)),
        math.max(0, math.min(df.global.world.map.y_count-1, anchor_pos.y)),
        math.max(0, math.min(df.global.world.map.z_count-1, anchor_pos.z)))

    return math.abs(mouse_pos.x - anchor_pos.x) + 1,
        math.abs(mouse_pos.y - anchor_pos.y) + 1,
        math.abs(mouse_pos.z - anchor_pos.z) + 1
end

function DimensionsTooltip:init()
    ensure_key(self, 'frame').w = 17
    self.frame.h = 4

    self.label = Label{
        frame={t=0},
        auto_width=true,
        text={
            {
                text=function()
                    local anchor_pos = self.get_anchor_pos_fn()
                    return ('%dx%dx%d'):format(get_cur_area_dims(anchor_pos))
                end
            }
        },
    }

    self:addviews{
        Panel{
            -- set minimum size for tooltip frame so the DFHack frame badge fits
            frame={t=0, l=0, w=7, h=2},
        },
        self.label,
    }
end

function DimensionsTooltip:render(dc)
    local x, y = dfhack.screen.getMousePos()
    if not x or not self.get_anchor_pos_fn() then return end
    local sw, sh = dfhack.screen.getWindowSize()
    local frame_width = math.max(9, self.label:getTextWidth() + 2)
    self.frame.l = math.min(x + self.display_offset.x, sw - frame_width)
    self.frame.t = math.min(y + self.display_offset.y, sh - self.frame.h)
    self:updateLayout()
    DimensionsTooltip.super.render(self, dc)
end

return _ENV
