local _ENV = mkmodule('plugins.hotkeys')

local gui = require('gui')
local helpdb = require('helpdb')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

local logo_textures = dfhack.textures.loadTileset('hack/data/art/logo.png', 8, 12, true)
local logo_hovered_textures = dfhack.textures.loadTileset('hack/data/art/logo_hovered.png', 8, 12, true)

local function get_command(cmdline)
    local first_word = cmdline:trim():split(' +')[1]
    if first_word:startswith(':') then first_word = first_word:sub(2) end
    return first_word
end

function should_hide_armok(cmdline)
    local command = get_command(cmdline)
    return dfhack.getHideArmokTools() and
            helpdb.is_entry(command) and
            helpdb.get_entry_tags(command).armok
end

-- ----------------- --
-- HotspotMenuWidget --
-- ----------------- --

HotspotMenuWidget = defclass(HotspotMenuWidget, overlay.OverlayWidget)
HotspotMenuWidget.ATTRS{
    desc='Shows the DFHack logo context menu button.',
    default_pos={x=5,y=1},
    default_enabled=true,
    version=2,
    viewscreens={
        'adopt_region',
        -- 'adventure_log', -- need to verify compatibility
        -- 'barter', -- need to verify compatibility
        'choose_game_type',
        -- 'choose_start_site', -- conflicts with vanilla panel layouts
        -- 'debug', -- need to verify compatibility
        -- 'dungeon_monsterstatus', -- need to verify compatibility
        'dungeonmode',
        'dwarfmode',
        'export_region',
        'game_cleaner',
        'initial_prep',
        -- 'layer_unit_action', -- need to verify compatibility
        -- 'layer_unit_health', -- need to verify compatibility
        -- 'legends', -- conflicts with vanilla export button and info text
        -- 'loadgame', -- disable temporarily while we get texture reloading sorted
        -- 'new_arena', -- conflicts with vanilla panel layouts
        -- 'new_region', -- conflicts with vanilla panel layouts
        'savegame',
        -- 'setupadventure', -- need to verify compatibility
        'setupdwarfgame',
        'title/Default',
        'update_region',
        'world'
    },
    frame={w=4, h=3}
}

function HotspotMenuWidget:init()
    local to_pen = dfhack.pen.parse
    local function tp(idx, ch)
        return to_pen{
            tile=function() return dfhack.textures.getTexposByHandle(logo_textures[idx]) end,
            ch=ch,
            fg=COLOR_GREY,
        }
    end
    local function tph(idx, ch)
        return to_pen{
            tile=function() return dfhack.textures.getTexposByHandle(logo_hovered_textures[idx]) end,
            ch=ch,
            fg=COLOR_WHITE,
            bold=true,
        }
    end
    local function get_tile_token(idx, ch)
        return {
            tile=tp(idx, ch),
            htile=tph(idx, ch),
            width=1,
        }
    end

    self:addviews{
        widgets.Label{
            text={
                get_tile_token(1, 179), get_tile_token(2, 'D'), get_tile_token(3, 'F'), get_tile_token(4, 179), NEWLINE,
                get_tile_token(5, 179), get_tile_token(6, 'H'), get_tile_token(7, 'a'), get_tile_token(8, 179), NEWLINE,
                get_tile_token(9, 179), get_tile_token(10, 'c'), get_tile_token(11, 'k'), get_tile_token(12, 179),
            },
            on_click=function() dfhack.run_command('hotkeys') end,
        },
    }
end

function HotspotMenuWidget:overlay_trigger()
    return MenuScreen{hotspot=self}:show()
end

-- register the menu hotspot with the overlay
OVERLAY_WIDGETS = {menu=HotspotMenuWidget}

-- ---- --
-- Menu --
-- ---- --

local ARROW = string.char(26)
local MAX_LIST_WIDTH = 45
local MAX_LIST_HEIGHT = 15

Menu = defclass(Menu, widgets.Panel)
Menu.ATTRS{
    hotspot=DEFAULT_NIL,
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

-- number of non-text tiles: icon, space between cmd and hk, scrollbar+margin
local LIST_BUFFER = 2 + 1 + 3

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
        command_token.width = max_width - choice.hk_width - (LIST_BUFFER - 1)
    end

    return choices, max_width
end

function Menu:init()
    local hotkeys, bindings = getHotkeys()
    if #hotkeys == 0 then
        hotkeys = {''}
        bindings = {['']='gui/launcher'}
    end

    local is_inverted = not not self.hotspot.frame.b
    local choices,list_width = get_choices(hotkeys, bindings, is_inverted)

    list_width = math.max(35, list_width)

    local list_frame = copyall(self.hotspot.frame)
    local list_widget_frame = {h=math.min(#choices, MAX_LIST_HEIGHT)}
    local quickstart_frame = {}
    list_frame.w = list_width + 2
    list_frame.h = list_widget_frame.h + 4
    if list_frame.t then
        list_frame.t = math.max(0, list_frame.t - 1)
        list_widget_frame.t = 0
        quickstart_frame.b = 0
    else
        list_frame.b = math.max(0, list_frame.b - 1)
        list_widget_frame.b = 0
        quickstart_frame.t = 0
    end
    if list_frame.l then
        list_frame.l = math.max(0, list_frame.l + 5)
    else
        list_frame.r = math.max(0, list_frame.r + 5)
    end

    local help_frame = {w=list_frame.w, l=list_frame.l, r=list_frame.r}
    if list_frame.t then
        help_frame.t = list_frame.t + list_frame.h
    else
        help_frame.b = list_frame.b + list_frame.h
    end

    self:addviews{
        widgets.Panel{
            view_id='list_panel',
            frame=list_frame,
            frame_style=gui.PANEL_FRAME,
            frame_background=gui.CLEAR_PEN,
            subviews={
                widgets.List{
                    view_id='list',
                    frame=list_widget_frame,
                    choices=choices,
                    icon_width=2,
                    on_select=self:callback('onSelect'),
                    on_submit=self:callback('onSubmit'),
                    on_submit2=self:callback('onSubmit2'),
                },
                widgets.Panel{frame={h=1}},
                widgets.HotkeyLabel{
                    frame=quickstart_frame,
                    label='Quickstart guide',
                    key='STRING_A063',
                    on_activate=function()
                        self:onSubmit(nil, {command='quickstart-guide'})
                    end,
                },
            },
        },
        widgets.ResizingPanel{
            view_id='help_panel',
            autoarrange_subviews=true,
            frame=help_frame,
            frame_style=gui.PANEL_FRAME,
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

    self.initialize = function()
        self.subviews.list:setSelected(is_inverted and #choices or 1)
    end
end

function Menu:onSelect(_, choice)
    if not choice or #self.subviews == 0 then return end
    local command = get_command(choice.command)
    self.subviews.help.text_to_wrap = helpdb.is_entry(command) and
            helpdb.get_entry_short_help(command) or 'Command not found'
    self.subviews.help_panel:updateLayout()
end

function Menu:onSubmit(_, choice)
    if not choice then return end
    dfhack.screen.hideGuard(self.parent_view, dfhack.run_command, choice.command)
    self.parent_view:dismiss()
end

function Menu:onSubmit2(_, choice)
    if not choice then return end
    self.parent_view:dismiss()
    dfhack.run_script('gui/launcher', choice.command)
end

function Menu:onInput(keys)
    if keys.LEAVESCREEN or keys._MOUSE_R then
        return false
    elseif keys.KEYBOARD_CURSOR_RIGHT then
        self:onSubmit2(self.subviews.list:getSelected())
        return true
    elseif keys._MOUSE_L then
        local list = self.subviews.list
        local x = list:getMousePos()
        if x == 0 then -- clicked on icon
            self:onSubmit2(list:getSelected())
            return true
        end
        if not self:getMouseFramePos() then
            self.parent_view:dismiss()
            return true
        end
    end
    self:inputToSubviews(keys)
    return true -- we're modal
end

function Menu:onRenderFrame(dc, rect)
    if self.initialize then
        self.initialize()
        self.initialize = nil
    end
    Menu.super.onRenderFrame(self, dc, rect)
end

function Menu:getMouseFramePos()
    return self.subviews.list_panel:getMouseFramePos() or
            self.subviews.help_panel:getMouseFramePos()
end

function Menu:onRenderBody(dc)
    Menu.super.onRenderBody(self, dc)
    local list = self.subviews.list
    local idx = list:getIdxUnderMouse()
    if idx and idx ~= self.last_mouse_idx then
        -- focus follows mouse, but if cursor keys were used to change the
        -- selection, don't override the selection until the mouse moves to
        -- another item
        list:setSelected(idx)
        self.last_mouse_idx = idx
    end
end

-- ---------- --
-- MenuScreen --
-- ---------- --

MenuScreen = defclass(MenuScreen, gui.ZScreen)
MenuScreen.ATTRS {
    focus_path='hotkeys/menu',
    initial_pause=false,
    hotspot=DEFAULT_NIL,
}

function MenuScreen:init()
    self:addviews{
        Menu{hotspot=self.hotspot},
    }
end

function MenuScreen:onDismiss()
    cleanupHotkeys()
end

return _ENV
