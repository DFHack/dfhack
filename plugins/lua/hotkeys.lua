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
    default_pos={x=1,y=3},
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
local MAX_LIST_WIDTH = 45
local MAX_LIST_HEIGHT = 15

MenuScreen = defclass(MenuScreen, gui.Screen)
MenuScreen.ATTRS{
    focus_path='hotkeys/menu',
    hotspot_frame=DEFAULT_NIL,
    hotkeys=DEFAULT_NIL,
    bindings=DEFAULT_NIL,
}

-- get a map from the binding string to a list of hotkey strings that all
-- point to that binding
local function get_bindings_to_hotkeys(hotkeys, bindings)
    local bindings_to_hotkeys = {}
    for _,hotkey in ipairs(hotkeys) do
        local binding = bindings[hotkey]
        table.insert(ensure_key(bindings_to_hotkeys, binding), hotkey)
    end
    return bindings_to_hotkeys
end

-- number of non-text tiles: icon, space, space between cmd and hk, scrollbar
local LIST_BUFFER = 2 + 1 + 1

local function get_choices(hotkeys, bindings, is_inverted)
    local choices, max_width, seen = {}, 0, {}
    local bindings_to_hotkeys = get_bindings_to_hotkeys(hotkeys, bindings)

    -- build list choices
    for _,hotkey in ipairs(hotkeys) do
        local command = bindings[hotkey]
        if seen[command] then goto continue end
        seen[command] = true
        local hk_width, tokens = 0, {}
        for _,hk in ipairs(bindings_to_hotkeys[command]) do
            if hk_width ~= 0 then
                table.insert(tokens, ', ')
                hk_width = hk_width + 2
            end
            table.insert(tokens, {text=hk, pen=COLOR_LIGHTGREEN})
            hk_width = hk_width + #hk
        end
        local command_str = command
        if hk_width + #command + LIST_BUFFER > MAX_LIST_WIDTH then
            local max_command_len = MAX_LIST_WIDTH - hk_width - LIST_BUFFER
            command_str = command:sub(1, max_command_len - 3) .. '...'
        end
        table.insert(tokens, 1, {text=command_str})
        local choice = {icon=ARROW, command=command, text=tokens,
                        hk_width=hk_width}
        max_width = math.max(max_width, hk_width + #command_str + LIST_BUFFER)
        table.insert(choices, is_inverted and 1 or #choices + 1, choice)
        ::continue::
    end

    -- adjust width of command fields so the hotkey tokens are right justified
    for _,choice in ipairs(choices) do
        local command_token = choice.text[1]
        command_token.width = max_width - choice.hk_width - 3
    end

    return choices, max_width
end

function MenuScreen:init()
    self.mouseover = false

    local choices,list_width = get_choices(self.hotkeys, self.bindings,
                                           self.hotspot_frame.b)

    local list_frame = copyall(self.hotspot_frame)
    list_frame.w = list_width + 2
    list_frame.h = math.min(#choices, MAX_LIST_HEIGHT) + 2
    if list_frame.t then
        list_frame.t = math.max(0, list_frame.t - 1)
    else
        list_frame.b = math.max(0, list_frame.b - 1)
    end
    if list_frame.l then
        list_frame.l = math.max(0, list_frame.l - 1)
    else
        list_frame.r = math.max(0, list_frame.r - 1)
    end

    local help_frame = {w=list_frame.w, l=list_frame.l, r=list_frame.r}
    if list_frame.t then
        help_frame.t = list_frame.t + list_frame.h + 1
    else
        help_frame.b = list_frame.b + list_frame.h + 1
    end

    self:addviews{
        widgets.ResizingPanel{
            view_id='list_panel',
            autoarrange_subviews=true,
            frame=list_frame,
            frame_style=gui.GREY_LINE_FRAME,
            frame_background=gui.CLEAR_PEN,
            subviews={
                widgets.List{
                    view_id='list',
                    choices=choices,
                    icon_width=2,
                    on_select=self:callback('onSelect'),
                    on_submit=self:callback('onSubmit'),
                    on_submit2=self:callback('onSubmit2'),
                },
            },
        },
        widgets.ResizingPanel{
            view_id='help_panel',
            autoarrange_subviews=true,
            frame=help_frame,
            frame_style=gui.GREY_LINE_FRAME,
            frame_background=gui.CLEAR_PEN,
            subviews={
                widgets.WrappedLabel{
                    view_id='help',
                    text_to_wrap='',
                    scroll_keys={},
                },
            },
        },
    }
end

function MenuScreen:onDismiss()
    cleanupHotkeys()
end

function MenuScreen:onSelect(_, choice)
    if not choice or #self.subviews == 0 then return end
    local first_word = choice.command:trim():split(' +')[1]
    if first_word:startswith(':') then first_word = first_word:sub(2) end
    self.subviews.help.text_to_wrap = helpdb.is_entry(first_word) and
            helpdb.get_entry_short_help(first_word) or 'Command not found'
    self.subviews.help_panel:updateLayout()
end

function MenuScreen:onSubmit(_, choice)
    if not choice then return end
    dfhack.screen.hideGuard(self, dfhack.run_command, choice.command)
    self:dismiss()
end

function MenuScreen:onSubmit2(_, choice)
    if not choice then return end
    self:dismiss()
    dfhack.run_script('gui/launcher', choice.command)
end

function MenuScreen:onInput(keys)
    if keys.LEAVESCREEN then
        self:dismiss()
        return true
    elseif keys.STANDARDSCROLL_RIGHT then
        self:onSubmit2(self.subviews.list:getSelected())
        return true
    elseif keys._MOUSE_L then
        local list = self.subviews.list
        local x = list:getMousePos()
        if x == 0 then -- clicked on icon
            self:onSubmit2(list:getSelected())
            return true
        end
    end
    return self:inputToSubviews(keys)
end

function MenuScreen:onRenderFrame(dc, rect)
    self:renderParent()
end

function MenuScreen:onRenderBody(dc)
    local panel = self.subviews.list_panel
    local list = self.subviews.list
    local idx = list:getIdxUnderMouse()
    if idx and idx ~= self.last_mouse_idx then
        -- focus follows mouse, but if cursor keys were used to change the
        -- selection, don't override the selection until the mouse moves to
        -- another item
        list:setSelected(idx)
        self.mouseover = true
        self.last_mouse_idx = idx
    elseif not panel:getMousePos(gui.ViewRect{rect=panel.frame_rect})
            and self.mouseover then
        -- once the mouse has entered the list area, leaving the frame should
        -- close the menu screen
        self:dismiss()
    end
end

return _ENV
