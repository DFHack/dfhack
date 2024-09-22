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
TextButton = require('gui.widgets.text_button')
CycleHotkeyLabel = require('gui.widgets.cycle_hotkey_label')
ButtonGroup = require('gui.widgets.button_group')
ToggleHotkeyLabel = require('gui.widgets.toggle_hotkey_label')
List = require('gui.widgets.list')

makeButtonLabelText = Label.makeButtonLabelText
parse_label_text = Label.parse_label_text
render_text = Label.render_text
check_text_keys = Label.check_text_keys

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
