local Widget = require('gui.widgets.widget')

local to_pen = dfhack.pen.parse

---@enum STANDARDSCROLL
STANDARDSCROLL = {
    STANDARDSCROLL_UP = -1,
    KEYBOARD_CURSOR_UP = -1,
    STANDARDSCROLL_DOWN = 1,
    KEYBOARD_CURSOR_DOWN = 1,
    STANDARDSCROLL_PAGEUP = '-page',
    KEYBOARD_CURSOR_UP_FAST = '-page',
    STANDARDSCROLL_PAGEDOWN = '+page',
    KEYBOARD_CURSOR_DOWN_FAST = '+page',
}

---------------
-- Scrollbar --
---------------

SCROLL_INITIAL_DELAY_MS = 300
SCROLL_DELAY_MS = 20

---@class widgets.Scrollbar.attrs: widgets.Widget.attrs
---@field on_scroll? fun(new_top_elem?: integer)

---@class widgets.Scrollbar.attrs.partial: widgets.Scrollbar.attrs

---@class widgets.Scrollbar: widgets.Widget, widgets.Scrollbar.attrs
---@field super widgets.Widget
---@field ATTRS widgets.Scrollbar.attrs|fun(attributes: widgets.Scrollbar.attrs.partial)
---@overload fun(init_table: widgets.Scrollbar.attrs.partial): self
Scrollbar = defclass(Scrollbar, Widget)

Scrollbar.ATTRS{
    on_scroll = DEFAULT_NIL,
}

---@param init_table widgets.Scrollbar.attrs.partial
function Scrollbar:preinit(init_table)
    init_table.frame = init_table.frame or {}
    init_table.frame.w = init_table.frame.w or 2
end

function Scrollbar:init()
    self.last_scroll_ms = 0
    self.is_first_click = false
    self.scroll_spec = nil
    self.is_dragging = false -- index of the scrollbar tile that we're dragging
    self:update(1, 1, 1)
end

local function scrollbar_get_max_pos_and_height(scrollbar)
    local frame_body = scrollbar.frame_body
    local scrollbar_body_height = (frame_body and frame_body.height or 3) - 2

    local height = math.max(2, math.floor(
        (math.min(scrollbar.elems_per_page, scrollbar.num_elems) * scrollbar_body_height) /
        scrollbar.num_elems))

    return scrollbar_body_height - height, height
end

-- calculate and cache the number of tiles of empty space above the top of the
-- scrollbar and the number of tiles the scrollbar should occupy to represent
-- the percentage of text that is on the screen.
-- if elems_per_page or num_elems are not specified, the last values passed to
-- Scrollbar:update() are used.
function Scrollbar:update(top_elem, elems_per_page, num_elems)
    if not top_elem then error('must specify index of new top element') end
    elems_per_page = elems_per_page or self.elems_per_page
    num_elems = num_elems or self.num_elems
    self.top_elem = top_elem
    self.elems_per_page, self.num_elems = elems_per_page, num_elems

    local max_pos, height = scrollbar_get_max_pos_and_height(self)
    local pos = (num_elems == elems_per_page) and 0 or
            math.ceil(((top_elem-1) * max_pos) /
                      (num_elems - elems_per_page))

    self.bar_offset, self.bar_height = pos, height
end

local function scrollbar_do_drag(scrollbar)
    local x,y = dfhack.screen.getMousePos()
    if not y then return end
    x,y = scrollbar.frame_body:localXY(x, y)
    local cur_pos = y - scrollbar.is_dragging
    local max_top = scrollbar.num_elems - scrollbar.elems_per_page + 1
    local max_pos = scrollbar_get_max_pos_and_height(scrollbar)
    local new_top_elem = math.floor(cur_pos * max_top / max_pos) + 1
    new_top_elem = math.max(1, math.min(new_top_elem, max_top))
    if new_top_elem ~= scrollbar.top_elem then
        scrollbar.on_scroll(new_top_elem)
    end
end

local function scrollbar_is_visible(scrollbar)
    return scrollbar.elems_per_page < scrollbar.num_elems
end

local SBSO = df.global.init.scrollbar_texpos[0] --Scroll Bar Spritesheet Offset / change this to point to a different spritesheet (ui themes, anyone? :p)
local SCROLLBAR_UP_LEFT_PEN = to_pen{tile=SBSO+0, ch=47, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_UP_RIGHT_PEN = to_pen{tile=SBSO+1, ch=92, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_DOWN_LEFT_PEN = to_pen{tile=SBSO+24, ch=92, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_DOWN_RIGHT_PEN = to_pen{tile=SBSO+25, ch=47, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_UP_LEFT_PEN = to_pen{tile=SBSO+6, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_UP_RIGHT_PEN = to_pen{tile=SBSO+7, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_LEFT_PEN = to_pen{tile=SBSO+30, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_RIGHT_PEN = to_pen{tile=SBSO+31, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_UP_LEFT_PEN = to_pen{tile=SBSO+10, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_UP_RIGHT_PEN = to_pen{tile=SBSO+11, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_DOWN_LEFT_PEN = to_pen{tile=SBSO+22, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_DOWN_RIGHT_PEN = to_pen{tile=SBSO+23, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_LEFT_PEN = to_pen{tile=SBSO+18, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_RIGHT_PEN = to_pen{tile=SBSO+19, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_DOWN_LEFT_PEN = to_pen{tile=SBSO+42, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_DOWN_RIGHT_PEN = to_pen{tile=SBSO+43, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_2TALL_UP_LEFT_PEN = to_pen{tile=SBSO+26, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_2TALL_UP_RIGHT_PEN = to_pen{tile=SBSO+27, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_2TALL_DOWN_LEFT_PEN = to_pen{tile=SBSO+38, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_2TALL_DOWN_RIGHT_PEN = to_pen{tile=SBSO+39, ch=219, fg=COLOR_CYAN, bg=COLOR_BLACK}
local SCROLLBAR_UP_LEFT_HOVER_PEN = to_pen{tile=SBSO+2, ch=47, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_UP_RIGHT_HOVER_PEN = to_pen{tile=SBSO+3, ch=92, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_DOWN_LEFT_HOVER_PEN = to_pen{tile=SBSO+14, ch=92, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_DOWN_RIGHT_HOVER_PEN = to_pen{tile=SBSO+15, ch=47, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_UP_LEFT_HOVER_PEN = to_pen{tile=SBSO+8, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_UP_RIGHT_HOVER_PEN = to_pen{tile=SBSO+9, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_LEFT_HOVER_PEN = to_pen{tile=SBSO+32, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_RIGHT_HOVER_PEN = to_pen{tile=SBSO+33, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_UP_LEFT_HOVER_PEN = to_pen{tile=SBSO+34, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_UP_RIGHT_HOVER_PEN = to_pen{tile=SBSO+35, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_DOWN_LEFT_HOVER_PEN = to_pen{tile=SBSO+46, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_DOWN_RIGHT_HOVER_PEN = to_pen{tile=SBSO+47, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_LEFT_HOVER_PEN = to_pen{tile=SBSO+20, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_CENTER_RIGHT_HOVER_PEN = to_pen{tile=SBSO+21, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_DOWN_LEFT_HOVER_PEN = to_pen{tile=SBSO+44, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_DOWN_RIGHT_HOVER_PEN = to_pen{tile=SBSO+45, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_2TALL_UP_LEFT_HOVER_PEN = to_pen{tile=SBSO+28, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_2TALL_UP_RIGHT_HOVER_PEN = to_pen{tile=SBSO+29, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_2TALL_DOWN_LEFT_HOVER_PEN = to_pen{tile=SBSO+40, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_2TALL_DOWN_RIGHT_HOVER_PEN = to_pen{tile=SBSO+41, ch=219, fg=COLOR_LIGHTCYAN, bg=COLOR_BLACK}
local SCROLLBAR_BAR_BG_LEFT_PEN = to_pen{tile=SBSO+12, ch=176, fg=COLOR_DARKGREY, bg=COLOR_BLACK}
local SCROLLBAR_BAR_BG_RIGHT_PEN = to_pen{tile=SBSO+13, ch=176, fg=COLOR_DARKGREY, bg=COLOR_BLACK}

function Scrollbar:onRenderBody(dc)
    -- don't draw if all elements are visible
    if not scrollbar_is_visible(self) then
        return
    end
    -- determine which elements should be highlighted
    local _,hover_y = self:getMousePos()
    local hover_up, hover_down, hover_bar = false, false, false
    if hover_y == 0 then
        hover_up = true
    elseif hover_y == dc.height-1 then
        hover_down = true
    elseif hover_y then
        hover_bar = true
    end
    -- render up arrow
    dc:seek(0, 0)
    dc:char(nil, hover_up and SCROLLBAR_UP_LEFT_HOVER_PEN or SCROLLBAR_UP_LEFT_PEN)
    dc:char(nil, hover_up and SCROLLBAR_UP_RIGHT_HOVER_PEN or SCROLLBAR_UP_RIGHT_PEN)
    -- render scrollbar body
    local starty = self.bar_offset + 1
    local endy = self.bar_offset + self.bar_height
    local midy = (starty + endy)/2
    for y=1,dc.height-2 do
        dc:seek(0, y)
        if y >= starty and y <= endy then
            if y == starty and y <= midy - 1 then
                dc:char(nil, hover_bar and SCROLLBAR_BAR_UP_LEFT_HOVER_PEN or SCROLLBAR_BAR_UP_LEFT_PEN)
                dc:char(nil, hover_bar and SCROLLBAR_BAR_UP_RIGHT_HOVER_PEN or SCROLLBAR_BAR_UP_RIGHT_PEN)
            elseif y == midy - 0.5 and self.bar_height == 2 then
                dc:char(nil, hover_bar and SCROLLBAR_BAR_2TALL_UP_LEFT_HOVER_PEN or SCROLLBAR_BAR_2TALL_UP_LEFT_PEN)
                dc:char(nil, hover_bar and SCROLLBAR_BAR_2TALL_UP_RIGHT_HOVER_PEN or SCROLLBAR_BAR_2TALL_UP_RIGHT_PEN)
            elseif y == midy + 0.5 and self.bar_height == 2 then
                dc:char(nil, hover_bar and SCROLLBAR_BAR_2TALL_DOWN_LEFT_HOVER_PEN or SCROLLBAR_BAR_2TALL_DOWN_LEFT_PEN)
                dc:char(nil, hover_bar and SCROLLBAR_BAR_2TALL_DOWN_RIGHT_HOVER_PEN or SCROLLBAR_BAR_2TALL_DOWN_RIGHT_PEN)
            elseif y == midy - 0.5 then
                dc:char(nil, hover_bar and SCROLLBAR_BAR_CENTER_UP_LEFT_HOVER_PEN or SCROLLBAR_BAR_CENTER_UP_LEFT_PEN)
                dc:char(nil, hover_bar and SCROLLBAR_BAR_CENTER_UP_RIGHT_HOVER_PEN or SCROLLBAR_BAR_CENTER_UP_RIGHT_PEN)
            elseif y == midy + 0.5 then
                dc:char(nil, hover_bar and SCROLLBAR_BAR_CENTER_DOWN_LEFT_HOVER_PEN or SCROLLBAR_BAR_CENTER_DOWN_LEFT_PEN)
                dc:char(nil, hover_bar and SCROLLBAR_BAR_CENTER_DOWN_RIGHT_HOVER_PEN or SCROLLBAR_BAR_CENTER_DOWN_RIGHT_PEN)
            elseif y == midy then
                dc:char(nil, hover_bar and SCROLLBAR_BAR_CENTER_LEFT_HOVER_PEN or SCROLLBAR_BAR_CENTER_LEFT_PEN)
                dc:char(nil, hover_bar and SCROLLBAR_BAR_CENTER_RIGHT_HOVER_PEN or SCROLLBAR_BAR_CENTER_RIGHT_PEN)
            elseif y == endy and y >= midy + 1 then
                dc:char(nil, hover_bar and SCROLLBAR_BAR_DOWN_LEFT_HOVER_PEN or SCROLLBAR_BAR_DOWN_LEFT_PEN)
                dc:char(nil, hover_bar and SCROLLBAR_BAR_DOWN_RIGHT_HOVER_PEN or SCROLLBAR_BAR_DOWN_RIGHT_PEN)
            else
                dc:char(nil, hover_bar and SCROLLBAR_BAR_LEFT_HOVER_PEN or SCROLLBAR_BAR_LEFT_PEN)
                dc:char(nil, hover_bar and SCROLLBAR_BAR_RIGHT_HOVER_PEN or SCROLLBAR_BAR_RIGHT_PEN)
            end
        else
            dc:char(nil, SCROLLBAR_BAR_BG_LEFT_PEN)
            dc:char(nil, SCROLLBAR_BAR_BG_RIGHT_PEN)
        end
    end
    -- render down arrow
    dc:seek(0, dc.height-1)
    dc:char(nil, hover_down and SCROLLBAR_DOWN_LEFT_HOVER_PEN or SCROLLBAR_DOWN_LEFT_PEN)
    dc:char(nil, hover_down and SCROLLBAR_DOWN_RIGHT_HOVER_PEN or SCROLLBAR_DOWN_RIGHT_PEN)
    if not self.on_scroll then return end
    -- manage state for dragging and continuous scrolling
    if self.is_dragging then
        scrollbar_do_drag(self)
    end
    if df.global.enabler.mouse_lbut_down == 0 then
        self.last_scroll_ms = 0
        self.is_dragging = false
        self.scroll_spec = nil
        return
    end
    if self.last_scroll_ms == 0 then return end
    local now = dfhack.getTickCount()
    local delay = self.is_first_click and
            SCROLL_INITIAL_DELAY_MS or SCROLL_DELAY_MS
    if now - self.last_scroll_ms >= delay then
        self.is_first_click = false
        self.on_scroll(self.scroll_spec)
        self.last_scroll_ms = now
    end
end

function Scrollbar:onInput(keys)
    if not self.on_scroll or not scrollbar_is_visible(self) then
        return false
    end

    if self.parent_view and self.parent_view:getMousePos() then
        if keys.CONTEXT_SCROLL_UP then
            self.on_scroll('up_small')
            return true
        elseif keys.CONTEXT_SCROLL_DOWN then
            self.on_scroll('down_small')
            return true
        elseif keys.CONTEXT_SCROLL_PAGEUP then
            self.on_scroll('up_large')
            return true
        elseif keys.CONTEXT_SCROLL_PAGEDOWN then
            self.on_scroll('down_large')
            return true
        end
    end
    if not keys._MOUSE_L then return false end
    local _,y = self:getMousePos()
    if not y then return false end
    local scroll_spec = nil
    if y == 0 then scroll_spec = 'up_small'
    elseif y == self.frame_body.height - 1 then scroll_spec = 'down_small'
    elseif y <= self.bar_offset then scroll_spec = 'up_large'
    elseif y > self.bar_offset + self.bar_height then scroll_spec = 'down_large'
    else
        self.is_dragging = y - self.bar_offset
        return true
    end
    self.scroll_spec = scroll_spec
    self.on_scroll(scroll_spec)
    -- reset continuous scroll state
    self.is_first_click = true
    self.last_scroll_ms = dfhack.getTickCount()
    return true
end

Scrollbar.STANDARDSCROLL = STANDARDSCROLL
Scrollbar.SCROLL_INITIAL_DELAY_MS = SCROLL_INITIAL_DELAY_MS
Scrollbar.SCROLL_DELAY_MS = SCROLL_DELAY_MS

return Scrollbar
