local _ENV = mkmodule('plugins.hotkeys')

local gui = require('gui')
local helpdb = require('helpdb')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

-- ----------------- --
-- HotspotMenuWidget --
-- ----------------- --

HotspotMenuWidget = defclass(HotspotMenuWidget, overlay.OverlayWidget)
HotspotMenuWidget.ATTRS{
    default_pos={x=1,y=2},
    hotspot=true,
    viewscreens={'dwarfmode'},
    overlay_onupdate_max_freq_seconds=0,
    frame={w=2, h=1}
}

function HotspotMenuWidget:init()
    self:addviews{widgets.Label{text='!!'}}
    self.mouseover = false
end

function HotspotMenuWidget:overlay_onupdate()
    local hasMouse = self:getMousePos()
    if hasMouse and not self.mouseover then
        self.mouseover = true
        return true
    end
    self.mouseover = hasMouse
end

function HotspotMenuWidget:overlay_trigger()
    local hotkeys, bindings = getHotkeys()
    return MenuScreen{
        hotspot_frame=self.frame,
        hotkeys=hotkeys,
        bindings=bindings}:show()
end

-- register the menu hotspot with the overlay
OVERLAY_WIDGETS = {menu=HotspotMenuWidget}

-- ---------- --
-- MenuScreen --
-- ---------- --

local ARROW = string.char(26)
local MENU_WIDTH = 40
local MENU_HEIGHT = 10

MenuScreen = defclass(MenuScreen, gui.Screen)
MenuScreen.ATTRS{
    focus_path='hotkeys/menu',
    hotspot_frame=DEFAULT_NIL,
    hotkeys=DEFAULT_NIL,
    bindings=DEFAULT_NIL,
}

function MenuScreen:init()
    self.mouseover = false

    local list_frame = copyall(self.hotspot_frame)
    list_frame.w = MENU_WIDTH
    list_frame.h = MENU_HEIGHT

    local help_frame = {w=MENU_WIDTH, l=list_frame.l, r=list_frame.r}
    if list_frame.t then
        help_frame.t = list_frame.t + MENU_HEIGHT + 1
    else
        help_frame.b = list_frame.b + MENU_HEIGHT + 1
    end

    local bindings = self.bindings
    local choices = {}
    for _,hotkey in ipairs(self.hotkeys) do
        local command = bindings[hotkey]
        local choice_text = command .. (' (%s)'):format(hotkey)
        local choice = {
            icon=ARROW, text=choice_text, command=command}
        table.insert(choices, list_frame.b and 1 or #choices + 1, choice)
    end

    self:addviews{
        widgets.List{
            view_id='list',
            frame=list_frame,
            choices=choices,
            icon_width=2,
            on_select=self:callback('onSelect'),
            on_submit=self:callback('onSubmit'),
            on_submit2=self:callback('onSubmit2'),
        },
        widgets.WrappedLabel{
            view_id='help',
            frame=help_frame,
            text_to_wrap='',
            scroll_keys={},
        },
    }
end

function MenuScreen:onSelect(_, choice)
    if not choice then return end
    local help = self.subviews.help
    local first_word = choice.command:trim():split(' +')[1]
    if not help or #first_word == 0 then return end
    help.text_to_wrap = helpdb.get_entry_short_help(first_word)
    help:updateLayout()
end

function MenuScreen:onSubmit(_, choice)
    self:dismiss()
    dfhack.run_command(choice.command)
end

function MenuScreen:onSubmit2(_, choice)
    self:dismiss()
    dfhack.run_script('gui/launcher', choice.command)
end

function MenuScreen:onInput(keys)
    if keys.LEAVESCREEN then
        self:dismiss()
        return true
    end
    return self:inputToSubviews(keys)
end

function MenuScreen:onRenderBody(dc)
    self:renderParent()
    local list = self.subviews.list
    local idx = list:getIdxUnderMouse()
    if idx and idx ~= self.last_mouse_idx then
        -- focus follows mouse, but if cursor keys were used to change the
        -- selection, don't override the selection until the mouse moves to
        -- another item
        list:setSelected(idx)
        self.mouseover = true
        self.last_mouse_idx = idx
    elseif not idx and self.mouseover then
        -- once the mouse has entered the list area, leaving it again should
        -- close the menu screen
        self:dismiss()
    end
end

return _ENV
