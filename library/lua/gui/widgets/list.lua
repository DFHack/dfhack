local gui = require('gui')
local utils = require('utils')
local common = require('gui.widgets.common')
local Widget = require('gui.widgets.widget')
local Scrollbar = require('gui.widgets.scrollbar')
local Label = require('gui.widgets.labels.label')
local Panel = require('gui.widgets.containers.panel')

local getval = utils.getval
local to_pen = dfhack.pen.parse

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
    scroll_keys = common.STANDARDSCROLL,
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
        Label.parse_label_text(l)
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
        Label.render_text(v)
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

        Label.render_text(obj, dc, iw or 0, y, cur_pen, cur_dpen, not current, self.text_hpen, hovered)

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
                if now_ms - self.last_select_click_ms <= Panel.DOUBLE_CLICK_MS then
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
            return Label.check_text_keys(current, keys)
        end
    end
end

return List
