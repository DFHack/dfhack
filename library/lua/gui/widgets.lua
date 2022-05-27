-- Simple widgets for screens

local _ENV = mkmodule('gui.widgets')

local gui = require('gui')
local utils = require('utils')

local dscreen = dfhack.screen

local function show_view(view,vis)
    if view then
        view.visible = vis
    end
end

local function getval(obj)
    if type(obj) == 'function' then
        return obj()
    else
        return obj
    end
end

local function map_opttab(tab,idx)
    if tab then
        return tab[idx]
    else
        return idx
    end
end

STANDARDSCROLL = {
    STANDARDSCROLL_UP = -1,
    STANDARDSCROLL_DOWN = 1,
    STANDARDSCROLL_PAGEUP = '-page',
    STANDARDSCROLL_PAGEDOWN = '+page',
}

SECONDSCROLL = {
    SECONDSCROLL_UP = -1,
    SECONDSCROLL_DOWN = 1,
    SECONDSCROLL_PAGEUP = '-page',
    SECONDSCROLL_PAGEDOWN = '+page',
}

------------
-- Widget --
------------

Widget = defclass(Widget, gui.View)

Widget.ATTRS {
    frame = DEFAULT_NIL,
    frame_inset = DEFAULT_NIL,
    frame_background = DEFAULT_NIL,
}

function Widget:computeFrame(parent_rect)
    local sw, sh = parent_rect.width, parent_rect.height
    return gui.compute_frame_body(sw, sh, self.frame, self.frame_inset)
end

function Widget:onRenderFrame(dc, rect)
    if self.frame_background then
        dc:fill(rect, self.frame_background)
    end
end

-----------
-- Panel --
-----------

Panel = defclass(Panel, Widget)

Panel.ATTRS {
    on_render = DEFAULT_NIL,
    on_layout = DEFAULT_NIL,
    autoarrange_subviews = false, -- whether to automatically lay out subviews
    autoarrange_gap = 0, -- how many blank lines to insert between widgets
}

function Panel:init(args)
    self:addviews(args.subviews)
end

function Panel:onRenderBody(dc)
    if self.on_render then self.on_render(dc) end
end

function Panel:postComputeFrame(body)
    if self.on_layout then self.on_layout(body) end
end

-- if self.autoarrange_subviews is true, lay out visible subviews vertically,
-- adding gaps between widgets according to self.autoarrange_gap.
function Panel:postUpdateLayout()
    if not self.autoarrange_subviews then return end

    local gap = self.autoarrange_gap
    local y = 0
    for _,subview in ipairs(self.subviews) do
        if not subview.frame then goto continue end
        subview.frame.t = y
        if subview.visible then
            y = y + (subview.frame.h or 0) + gap
        end
        ::continue::
    end
    self.frame_rect.height = y

    -- let widgets adjust to their new positions
    self:updateSubviewLayout()
end

-------------------
-- ResizingPanel --
-------------------

ResizingPanel = defclass(ResizingPanel, Panel)

-- adjust our frame dimensions according to positions and sizes of our subviews
function ResizingPanel:postUpdateLayout(frame_body)
    local w, h = 0, 0
    for _,subview in ipairs(self.subviews) do
        if subview.visible then
            w = math.max(w, (subview.frame.l or 0) +
                            (subview.frame.w or frame_body.width))
            h = math.max(h, (subview.frame.t or 0) +
                            (subview.frame.h or frame_body.height))
        end
    end
    if not self.frame then self.frame = {} end
    self.frame.w, self.frame.h = w, h
end

-----------
-- Pages --
-----------

Pages = defclass(Pages, Panel)

function Pages:init(args)
    for _,v in ipairs(self.subviews) do
        v.visible = false
    end
    self:setSelected(args.selected or 1)
end

function Pages:setSelected(idx)
    if type(idx) ~= 'number' then
        local key = idx
        if type(idx) == 'string' then
            key = self.subviews[key]
        end
        idx = utils.linear_index(self.subviews, key)
        if not idx then
            error('Unknown page: '..key)
        end
    end

    show_view(self.subviews[self.selected], false)
    self.selected = math.min(math.max(1, idx), #self.subviews)
    show_view(self.subviews[self.selected], true)
end

function Pages:getSelected()
    return self.selected, self.subviews[self.selected]
end

function Pages:getSelectedPage()
    return self.subviews[self.selected]
end

----------------
-- Edit field --
----------------

EditField = defclass(EditField, Widget)

EditField.ATTRS{
    label_text = DEFAULT_NIL,
    text = '',
    text_pen = DEFAULT_NIL,
    on_char = DEFAULT_NIL,
    on_change = DEFAULT_NIL,
    on_submit = DEFAULT_NIL,
    key = DEFAULT_NIL,
    key_sep = DEFAULT_NIL,
    frame = {h=1},
    modal = false,
}

function EditField:init()
    local function on_activate()
        self.saved_text = self.text
        self:setFocus(true)
    end

    self:addviews{HotkeyLabel{frame={t=0,l=0},
                              key=self.key,
                              key_sep=self.key_sep,
                              label=self.label_text,
                              on_activate=self.key and on_activate or nil}}
end

function EditField:getPreferredFocusState()
    return not self.key
end

function EditField:postUpdateLayout()
    self.text_offset = self.subviews[1]:getTextWidth()
end

function EditField:onRenderBody(dc)
    dc:pen(self.text_pen or COLOR_LIGHTCYAN):fill(0,0,dc.width-1,0)

    local cursor = '_'
    if not self.active or not self.focus or gui.blink_visible(300) then
        cursor = ' '
    end
    local txt = self.text .. cursor
    local max_width = dc.width - self.text_offset
    if #txt > max_width then
        txt = string.char(27)..string.sub(txt, #txt-max_width+2)
    end
    dc:advance(self.text_offset):string(txt)
end

function EditField:onInput(keys)
    if not self.focus then
        -- only react to our hotkey
        return self:inputToSubviews(keys)
    end

    if self.key and keys.LEAVESCREEN then
        local old = self.text
        self.text = self.saved_text
        if self.on_change and old ~= self.saved_text then
            self.on_change(self.text, old)
        end
        self:setFocus(false)
        return true
    end

    if keys.SELECT then
        if self.key then
            self:setFocus(false)
        end
        if self.on_submit then
            self.on_submit(self.text)
            return true
        end
        return not not self.key
    end

    if keys._STRING then
        local old = self.text
        if keys._STRING == 0 then
            -- handle backspace
            self.text = string.sub(old, 1, #old-1)
        else
            local cv = string.char(keys._STRING)
            if not self.on_char or self.on_char(cv, old) then
                self.text = old .. cv
            end
        end
        if self.on_change and self.text ~= old then
            self.on_change(self.text, old)
        end
        return true
    end

    -- if we're modal, then unconditionally eat all the input
    return self.modal
end

-----------
-- Label --
-----------

function parse_label_text(obj)
    local text = obj.text or {}
    if type(text) ~= 'table' then
        text = { text }
    end
    local curline = nil
    local out = { }
    local active = nil
    local idtab = nil
    for _,v in ipairs(text) do
        local vv
        if type(v) == 'string' then
            vv = v:split(NEWLINE)
        else
            vv = { v }
        end

        for i = 1,#vv do
            local cv = vv[i]
            if i > 1 then
                if not curline then
                    table.insert(out, {})
                end
                curline = nil
            end
            if cv ~= '' then
                if not curline then
                    curline = {}
                    table.insert(out, curline)
                end

                if type(cv) == 'string' then
                    table.insert(curline, { text = cv })
                else
                    table.insert(curline, cv)

                    if cv.on_activate then
                        active = active or {}
                        table.insert(active, cv)
                    end

                    if cv.id then
                        idtab = idtab or {}
                        idtab[cv.id] = cv
                    end
                end
            end
        end
    end
    obj.text_lines = out
    obj.text_active = active
    obj.text_ids = idtab
end

local function is_disabled(token)
    return (token.disabled ~= nil and getval(token.disabled)) or
           (token.enabled ~= nil and not getval(token.enabled))
end

function render_text(obj,dc,x0,y0,pen,dpen,disabled)
    local width = 0
    for iline = dc and obj.start_line_num or 1, #obj.text_lines do
        local x, line = 0, obj.text_lines[iline]
        if dc then
            local offset = (obj.start_line_num or 1) - 1
            local y = y0 + iline - offset - 1
            -- skip text outside of the containing frame
            if y > dc.height - 1 then break end
            dc:seek(x+x0, y)
        end
        for _,token in ipairs(line) do
            token.line = iline
            token.x1 = x

            if token.gap then
                x = x + token.gap
                if dc then
                    dc:advance(token.gap)
                end
            end

            if token.tile then
                x = x + 1
                if dc then
                    dc:char(nil, token.tile)
                end
            end

            if token.text or token.key then
                local text = ''..(getval(token.text) or '')
                local keypen = dfhack.pen.parse(token.key_pen or COLOR_LIGHTGREEN)

                if dc then
                    local tpen = getval(token.pen)
                    if disabled or is_disabled(token) then
                        dc:pen(getval(token.dpen) or tpen or dpen)
                        if keypen.fg ~= COLOR_BLACK then
                            keypen.bold = false
                        end
                    else
                        dc:pen(tpen or pen)
                    end
                end

                local width = getval(token.width)
                local padstr
                if width then
                    x = x + width
                    if #text > width then
                        text = string.sub(text,1,width)
                    else
                        if token.pad_char then
                            padstr = string.rep(token.pad_char,width-#text)
                        end
                        if dc and token.rjustify then
                            if padstr then dc:string(padstr) else dc:advance(width-#text) end
                        end
                    end
                else
                    x = x + #text
                end

                if token.key then
                    local keystr = gui.getKeyDisplay(token.key)
                    local sep = token.key_sep or ''

                    x = x + #keystr

                    if sep:startswith('()') then
                        if dc then
                            dc:string(text)
                            dc:string(' ('):string(keystr,keypen)
                            dc:string(sep:sub(2))
                        end
                        x = x + 1 + #sep
                    else
                        if dc then
                            dc:string(keystr,keypen):string(sep):string(text)
                        end
                        x = x + #sep
                    end
                else
                    if dc then
                        dc:string(text)
                    end
                end

                if width and dc and not token.rjustify then
                    if padstr then dc:string(padstr) else dc:advance(width-#text) end
                end
            end

            token.x2 = x
        end
        width = math.max(width, x)
    end
    obj.text_width = width
end

function check_text_keys(self, keys)
    if self.text_active then
        for _,item in ipairs(self.text_active) do
            if item.key and keys[item.key] and not is_disabled(item) then
                item.on_activate()
                return true
            end
        end
    end
end

Label = defclass(Label, Widget)

Label.ATTRS{
    text_pen = COLOR_WHITE,
    text_dpen = COLOR_DARKGREY, -- disabled
    text_hpen = DEFAULT_NIL, -- highlight - default is text_pen with reversed brightness
    disabled = DEFAULT_NIL,
    enabled = DEFAULT_NIL,
    auto_height = true,
    auto_width = false,
    on_click = DEFAULT_NIL,
    on_rclick = DEFAULT_NIL,
    --
    scroll_keys = STANDARDSCROLL,
    show_scroll_icons = DEFAULT_NIL, -- DEFAULT_NIL, 'right', 'left', false
    up_arrow_icon = string.char(24),
    down_arrow_icon = string.char(25),
    scroll_icon_pen = COLOR_LIGHTCYAN,
}

function Label:init(args)
    self.start_line_num = 1
    -- use existing saved text if no explicit text was specified. this avoids
    -- overwriting pre-formatted text that subclasses may have already set
    self:setText(args.text or self.text)
    if not self.text_hpen then
        self.text_hpen = ((tonumber(self.text_pen) or tonumber(self.text_pen.fg) or 0) + 8) % 16
    end
end

function Label:setText(text)
    self.text = text
    parse_label_text(self)

    if self.auto_height then
        self.frame = self.frame or {}
        self.frame.h = self:getTextHeight()
    end
end

function Label:update_scroll_inset()
    if self.show_scroll_icons == nil then
        self._show_scroll_icons = self:getTextHeight() > self.frame_body.height and 'right' or false
    else
        self._show_scroll_icons = self.show_scroll_icons
    end
    if self._show_scroll_icons then
        -- here self._show_scroll_icons can only be either
        -- 'left' or any true value which we interpret as right
        local l,t,r,b = gui.parse_inset(self.frame_inset)
        if self._show_scroll_icons == 'left' and l <= 0 then
            l = 1
        elseif r <= 0 then
            r = 1
        end
        self.frame_inset = {l=l,t=t,r=r,b=b}
    end
end

function Label:render_scroll_icons(dc, x, y1, y2)
    if self.start_line_num ~= 1 then
        dc:seek(x, y1):char(self.up_arrow_icon, self.scroll_icon_pen)
    end
    local last_visible_line = self.start_line_num + self.frame_body.height - 1
    if last_visible_line < self:getTextHeight() then
        dc:seek(x, y2):char(self.down_arrow_icon, self.scroll_icon_pen)
    end
end

function Label:computeFrame(parent_rect)
    local frame_rect,body_rect = Label.super.computeFrame(self, parent_rect)

    self.frame_rect = frame_rect
    self.frame_body = parent_rect:viewport(body_rect or frame_rect)

    self:update_scroll_inset() -- frame_body is now set

    -- recalc with updated frame_inset
    return Label.super.computeFrame(self, parent_rect)
end

function Label:preUpdateLayout()
    if self.auto_width then
        self.frame = self.frame or {}
        self.frame.w = self:getTextWidth()
    end
end

function Label:itemById(id)
    if self.text_ids then
        return self.text_ids[id]
    end
end

function Label:getTextHeight()
    return #self.text_lines
end

function Label:getTextWidth()
    render_text(self)
    return self.text_width
end

function Label:onRenderBody(dc)
    local text_pen = self.text_pen
    if self:getMousePos() and (self.on_click or self.on_rclick) then
        text_pen = self.text_hpen
    end
    render_text(self,dc,0,0,text_pen,self.text_dpen,is_disabled(self))
end

function Label:onRenderFrame(dc, rect)
    if self._show_scroll_icons
    and self:getTextHeight() > self.frame_body.height
    then
        local x = self._show_scroll_icons == 'left'
                and self.frame_body.x1-dc.x1-1
                or  self.frame_body.x2-dc.x1+1
        self:render_scroll_icons(dc,
            x,
            self.frame_body.y1-dc.y1,
            self.frame_body.y2-dc.y1
        )
    end
end

function Label:scroll(nlines)
    local n = self.start_line_num + nlines
    n = math.min(n, self:getTextHeight() - self.frame_body.height + 1)
    n = math.max(n, 1)
    self.start_line_num = n
end

function Label:onInput(keys)
    if is_disabled(self) then return false end
    if keys._MOUSE_L_DOWN and self:getMousePos() and self.on_click then
        self:on_click()
    end
    if keys._MOUSE_R_DOWN and self:getMousePos() and self.on_rclick then
        self:on_rclick()
    end
    for k,v in pairs(self.scroll_keys) do
        if keys[k] then
            if v == '+page' then
                v = self.frame_body.height
            elseif v == '-page' then
                v = -self.frame_body.height
            end
            self:scroll(v)
        end
    end
    return check_text_keys(self, keys)
end

------------------
-- WrappedLabel --
------------------

WrappedLabel = defclass(WrappedLabel, Label)

WrappedLabel.ATTRS{
    text_to_wrap=DEFAULT_NIL,
    indent=0,
}

function WrappedLabel:getWrappedText(width)
    -- 0 width can happen if the parent has 0 width
    if not self.text_to_wrap or width <= 0 then return nil end
    local text_to_wrap = getval(self.text_to_wrap)
    if type(text_to_wrap) == 'table' then
        text_to_wrap = table.concat(text_to_wrap, NEWLINE)
    end
    return text_to_wrap:wrap(width - self.indent)
end

-- we can't set the text in init() since we may not yet have a frame that we
-- can get wrapping bounds from.
function WrappedLabel:postComputeFrame()
    local wrapped_text = self:getWrappedText(self.frame_body.width)
    if not wrapped_text then return end
    local text = {}
    for _,line in ipairs(wrapped_text:split(NEWLINE)) do
        table.insert(text, {gap=self.indent, text=line})
        -- a trailing newline will get ignored so we don't have to manually trim
        table.insert(text, NEWLINE)
    end
    self:setText(text)
end

------------------
-- TooltipLabel --
------------------

TooltipLabel = defclass(TooltipLabel, WrappedLabel)

TooltipLabel.ATTRS{
    show_tooltip=DEFAULT_NIL,
    indent=2,
    text_pen=COLOR_GREY,
}

function TooltipLabel:preUpdateLayout()
    self.visible = getval(self.show_tooltip)
end

-----------------
-- HotkeyLabel --
-----------------

HotkeyLabel = defclass(HotkeyLabel, Label)

HotkeyLabel.ATTRS{
    key=DEFAULT_NIL,
    key_sep=': ',
    label=DEFAULT_NIL,
    on_activate=DEFAULT_NIL,
}

function HotkeyLabel:init()
    self:setText{{key=self.key, key_sep=self.key_sep, text=self.label,
                  on_activate=self.on_activate}}
end

----------------------
-- CycleHotkeyLabel --
----------------------

CycleHotkeyLabel = defclass(CycleHotkeyLabel, Label)

CycleHotkeyLabel.ATTRS{
    key=DEFAULT_NIL,
    label=DEFAULT_NIL,
    label_width=DEFAULT_NIL,
    options=DEFAULT_NIL,
    initial_option=1,
    on_change=DEFAULT_NIL,
}

function CycleHotkeyLabel:init()
    -- initialize option_idx
    for i in ipairs(self.options) do
        if self.initial_option == self:getOptionValue(i) then
            self.option_idx = i
            break
        end
    end
    if not self.option_idx then
        if self.options[self.initial_option] then
            self.option_idx = self.initial_option
        end
    end
    if not self.option_idx then
        error(('cannot find option with value or index: "%s"')
              :format(self.initial_option))
    end

    self:setText{
        {key=self.key, key_sep=': ', text=self.label, width=self.label_width,
         on_activate=self:callback('cycle')},
        '  ',
        {text=self:callback('getOptionLabel')},
    }
end

function CycleHotkeyLabel:cycle()
    local old_option_idx = self.option_idx
    if self.option_idx == #self.options then
        self.option_idx = 1
    else
        self.option_idx = self.option_idx + 1
    end
    if self.on_change then
        self.on_change(self:getOptionValue(),
                       self:getOptionValue(old_option_idx))
    end
end

function CycleHotkeyLabel:getOptionLabel(option_idx)
    option_idx = option_idx or self.option_idx
    local option = self.options[option_idx]
    if type(option) == 'table' then
        return option.label
    end
    return option
end

function CycleHotkeyLabel:getOptionValue(option_idx)
    option_idx = option_idx or self.option_idx
    local option = self.options[option_idx]
    if type(option) == 'table' then
        return option.value
    end
    return option
end

-----------------------
-- ToggleHotkeyLabel --
-----------------------

ToggleHotkeyLabel = defclass(ToggleHotkeyLabel, CycleHotkeyLabel)
ToggleHotkeyLabel.ATTRS{
    options={{label='On', value=true},
             {label='Off', value=false}},
}

----------
-- List --
----------

List = defclass(List, Widget)

List.ATTRS{
    text_pen = COLOR_CYAN,
    cursor_pen = COLOR_LIGHTCYAN,
    inactive_pen = DEFAULT_NIL,
    on_select = DEFAULT_NIL,
    on_submit = DEFAULT_NIL,
    on_submit2 = DEFAULT_NIL,
    row_height = 1,
    scroll_keys = STANDARDSCROLL,
    icon_width = DEFAULT_NIL,
}

function List:init(info)
    self.page_top = 1
    self.page_size = 1

    if info.choices then
        self:setChoices(info.choices, info.selected)
    else
        self.choices = {}
        self.selected = 1
    end
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

function List:postComputeFrame(body)
    self.page_size = math.max(1, math.floor(body.height / self.row_height))
    self:moveCursor(0)
end

function List:moveCursor(delta, force_cb)
    local page = math.max(1, self.page_size)
    local cnt = #self.choices

    if cnt < 1 then
        self.page_top = 1
        self.selected = 1
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

    self.selected = 1 + off % cnt
    self.page_top = 1 + page * math.floor((self.selected-1) / page)

    if (force_cb or delta ~= 0) and self.on_select then
        self.on_select(self:getSelected())
    end
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

    for i = top,iend do
        local obj = choices[i]
        local current = (i == self.selected)
        local cur_pen = self.cursor_pen
        local cur_dpen = self.text_pen
        local active_pen = current and cur_pen or cur_dpen

        if not self.active then
            cur_pen = self.inactive_pen or self.cursor_pen
        end

        local y = (i - top)*self.row_height
        local icon = getval(obj.icon)

        if iw and icon then
            dc:seek(0, y):pen(active_pen)
            paint_icon(icon, obj)
        end

        render_text(obj, dc, iw or 0, y, cur_pen, cur_dpen, not current)

        local ip = dc.width

        if obj.key then
            local keystr = gui.getKeyDisplay(obj.key)
            ip = ip-2-#keystr
            dc:seek(ip,y):pen(self.text_pen)
            dc:string('('):string(keystr,COLOR_LIGHTGREEN):string(')')
        end

        if icon and not iw then
            dc:seek(ip-1,y):pen(active_pen)
            paint_icon(icon, obj)
        end
    end
end

function List:submit()
    if self.on_submit and #self.choices > 0 then
        self.on_submit(self:getSelected())
    end
end

function List:submit2()
    if self.on_submit2 and #self.choices > 0 then
        self.on_submit2(self:getSelected())
    end
end

function List:onInput(keys)
    if self.on_submit and keys.SELECT then
        self:submit()
        return true
    elseif self.on_submit2 and keys.SEC_SELECT then
        self:submit2()
        return true
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

FilteredList = defclass(FilteredList, Widget)

FilteredList.ATTRS {
    edit_below = false,
    edit_key = DEFAULT_NIL,
}

function FilteredList:init(info)
    self.edit = EditField{
        text_pen = info.edit_pen or info.cursor_pen,
        frame = { l = info.icon_width, t = 0, h = 1 },
        on_change = self:callback('onFilterChange'),
        on_char = self:callback('onFilterChar'),
        key = self.edit_key,
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
    self.edit.text = ''
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
    self.edit.text = filter

    if filter ~= '' then
        local tokens = filter:split()
        local ipos = pos

        choices = {}
        cidx = {}
        pos = nil

        for i,v in ipairs(self.choices) do
            local ok = true
            local search_key = v.search_key or v.text
            for _,key in ipairs(tokens) do
                key = key:escape_pattern()
                -- start matches at non-space or non-punctuation. this allows
                -- punctuation itself to be matched if that is useful (e.g.
                -- filenames or parameter names)
                if key ~= '' and
                        not search_key:match('%f[^%p\x00]'..key) and
                        not search_key:match('%f[^%s\x00]'..key) then
                    ok = false
                    break
                end
            end
            if ok then
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

function FilteredList:onFilterChar(char, text)
    if char == ' ' then
        return string.match(text, '%S$')
    end
    return true
end

return _ENV
