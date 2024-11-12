local Widget = require('gui.widgets.widget')
local ResizingPanel = require('gui.widgets.containers.resizing_panel')
local Label = require('gui.widgets.labels.label')
local Panel = require('gui.widgets.containers.panel')
local utils = require('utils')

local to_pen = dfhack.pen.parse

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
---@field wrap boolean
---@field scroll_step integer
---@field scroll_key string
---@field scroll_key_back string
---@field fast_scroll_modifier integer
---@field scroll_into_view_offset integer
---@field scroll_label_text_pen dfhack.pen
---@field scroll_label_text_hpen dfhack.pen

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
    wrap = true,
    scroll_step = 10,
    scroll_key = 'CUSTOM_ALT_T',
    scroll_key_back = 'CUSTOM_ALT_Y',
    fast_scroll_modifier = 3,
    scroll_into_view_offset = 5,
    scroll_label_text_pen = DEFAULT_NIL,
    scroll_label_text_hpen = DEFAULT_NIL,
}

local TO_THE_RIGHT = string.char(16)
local TO_THE_LEFT = string.char(17)

---@param self widgets.TabBar
function TabBar:init()
    self.scrollable = false
    self.scroll_offset = 0
    self.first_render = true

    local panel = Panel{
      view_id='TabBar__tabs',
      frame={t=0, l=0, h=2},
      frame_inset={l=0},
    }

    for idx,label in ipairs(self.labels) do
        panel:addviews{
            Tab{
                frame={t=0, l=0},
                id=idx,
                label=label,
                on_select=function()
                    self.scrollTabIntoView(self, idx)
                    self.on_select(idx)
                end,
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

    self:addviews{panel}

    if not self.wrap then
      self:addviews{
        Label{
          view_id='TabBar__scroll_left',
          frame={t=0, l=0, w=1},
          text_pen=self.scroll_label_text_pen,
          text_hpen=self.scroll_label_text_hpen,
          text=TO_THE_LEFT,
          visible = false,
          on_click=function()
              self:scrollLeft()
          end,
        },
        Label{
          view_id='TabBar__scroll_right',
          frame={t=0, l=0, w=1},
          text_pen=self.scroll_label_text_pen,
          text_hpen=self.scroll_label_text_hpen,
          text=TO_THE_RIGHT,
          visible = false,
          on_click=function()
              self:scrollRight()
          end,
        },
      }
    end
end

function TabBar:updateScrollElements()
    self:showScrollLeft()
    self:showScrollRight()
    self:updateTabPanelPosition()
end

function TabBar:leftScrollVisible()
    return self.scroll_offset < 0
end

function TabBar:showScrollLeft()
    if self.wrap then return end
    self:scrollLeftElement().visible = self:leftScrollVisible()
end

function TabBar:rightScrollVisible()
    return self.scroll_offset > self.offset_to_show_last_tab
end

function TabBar:showScrollRight()
    if self.wrap then return end
    self:scrollRightElement().visible = self:rightScrollVisible()
end

function TabBar:updateTabPanelPosition()
    self:tabsElement().frame_inset.l = self.scroll_offset
    self:tabsElement():updateLayout(self.frame_body)
end

function TabBar:tabsElement()
    return self.subviews.TabBar__tabs
end

function TabBar:scrollLeftElement()
    return self.subviews.TabBar__scroll_left
end

function TabBar:scrollRightElement()
    return self.subviews.TabBar__scroll_right
end

function TabBar:scrollTabIntoView(idx)
    if self.wrap or not self.scrollable then return end

    local tab = self:tabsElement().subviews[idx]
    local tab_l = tab.frame.l
    local tab_r = tab.frame.l + tab.frame.w
    local tabs_l = self:tabsElement().frame.l
    local tabs_r = tabs_l + self.frame_body.width
    local scroll_offset = self.scroll_offset

    if tab_l < tabs_l - scroll_offset then
        self.scroll_offset = tabs_l - tab_l + self.scroll_into_view_offset
    elseif tab_r > tabs_r - scroll_offset then
        self.scroll_offset = self.scroll_offset - (tab_r - tabs_r) - self.scroll_into_view_offset
    end

    self:capScrollOffset()
    self:updateScrollElements()
end

function TabBar:capScrollOffset()
    if self.scroll_offset > 0 then
        self.scroll_offset = 0
    elseif self.scroll_offset < self.offset_to_show_last_tab then
        self.scroll_offset = self.offset_to_show_last_tab
    end
end

function TabBar:scrollRight(alternate_step)
    if not self:scrollRightElement().visible then return end

    self.scroll_offset = self.scroll_offset - (alternate_step and alternate_step or self.scroll_step)

    self:capScrollOffset()
    self:updateScrollElements()
end

function TabBar:scrollLeft(alternate_step)
    if not self:scrollLeftElement().visible then return end

    self.scroll_offset = self.scroll_offset + (alternate_step and alternate_step or self.scroll_step)

    self:capScrollOffset()
    self:updateScrollElements()
end

function TabBar:isMouseOver()
    for _, sv in ipairs(self:tabsElement().subviews) do
        if utils.getval(sv.visible) and sv:getMouseFramePos() then return true end
    end
end

function TabBar:postComputeFrame(body)
    self.all_tabs_width = 0

    local t, l, width = 0, 0, body.width
    self.scrollable = false

    self.last_post_compute_width = self.post_compute_width or 0
    self.post_compute_width = width

    local tab_rows = 1
    for _,tab in ipairs(self:tabsElement().subviews) do
        tab.visible = true
        if l > 0 and l + tab.frame.w > width then
            if self.wrap then
                t = t + 2
                l = 0
                tab_rows = tab_rows + 1
            else
                self.scrollable = true
            end
        end
        tab.frame.t = t
        tab.frame.l = l
        l = l + tab.frame.w
        self.all_tabs_width = self.all_tabs_width + tab.frame.w
    end

    self.offset_to_show_last_tab = -(self.all_tabs_width - self.post_compute_width)

    if self.scrollable and not self.wrap then
        self:scrollRightElement().frame.l = width - 1

        if self.last_post_compute_width ~= self.post_compute_width then
            self.scroll_offset = 0
        end
    end

    if self.first_render then
        self.first_render = false
        if not self.wrap then
          self:scrollTabIntoView(self.get_cur_page())
        end
    end

    -- we have to calculate the height of this based on the number of tab rows we will have
    -- so that autoarrange_subviews will work correctly
    self:tabsElement().frame.h = tab_rows * 2

    self:updateScrollElements()
end

function TabBar:fastStep()
    return self.scroll_step * self.fast_scroll_modifier
end

function TabBar:onInput(keys)
    if TabBar.super.onInput(self, keys) then return true end
    if not self.wrap then
        if self:isMouseOver() then
          if keys.CONTEXT_SCROLL_UP then
              self:scrollLeft()
              return true
          end
          if keys.CONTEXT_SCROLL_DOWN then
              self:scrollRight()
              return true
          end
          if keys.CONTEXT_SCROLL_PAGEUP then
              self:scrollLeft(self:fastStep())
              return true
          end
          if keys.CONTEXT_SCROLL_PAGEDOWN then
              self:scrollRight(self:fastStep())
              return true
          end
        end
        if self.scroll_key and keys[self.scroll_key] then
            self:scrollRight()
            return true
        end
        if self.scroll_key_back and keys[self.scroll_key_back] then
            self:scrollLeft()
            return true
        end
    end
    if self.key and keys[self.key] then
        local zero_idx = self.get_cur_page() - 1
        local next_zero_idx = (zero_idx + 1) % #self.labels
        self.scrollTabIntoView(self, next_zero_idx + 1)
        self.on_select(next_zero_idx + 1)
        return true
    end
    if self.key_back and keys[self.key_back] then
        local zero_idx = self.get_cur_page() - 1
        local prev_zero_idx = (zero_idx - 1) % #self.labels
        self.scrollTabIntoView(self, prev_zero_idx + 1)
        self.on_select(prev_zero_idx + 1)
        return true
    end
end

TabBar.Tab = Tab

return TabBar
