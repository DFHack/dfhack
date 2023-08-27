-- An in-game interface for `color-schemes`
--[====[

gui/color-schemes
=================
An in-game interface for `color-schemes`.
This script must be called from either the title screen or the dwarf fortress default screen.

]====]

--[[
+----------------------------------Color Schemes+------------------------------+  -
|                                               |                              |  |
| Colors                                        | Name                         |  |
|                                               |                              |  |
| Black-----------------+Dark Grey------------+ | colors                 LD    |  |
| |                     |                     | | ASCII default                |  |
| Blue------------------+Light Blue-----------+ | ...                          |  |
| |                     |                     | | invalid                  I   |  |
| Green-----------------+Light Green----------+ | ...                          |  |
| |                     |                     | |                              |  |
| Cyan------------------+Light Cyan-----------+ |                              |  a
| |                     |                     | |                              |  u
| Red-------------------+Light Red------------+ |                              |  t
| |                     |                     | |                              |  o
| Magenta---------------+Light Magenta--------+ |                              |  |
| |                     |                     | | ...                          |  |
| Brown-----------------+Yellow---------------+ |                              |  |
| |                     |                     | | L: loaded     I: invalid     |  |
| Grey------------------+White----------------+ | D: default                   |  |
| |                     |                     | |                              |  |
| +---------------------+---------------------+ | p: preview    R: register    |  |
|                                               | D: set default               |  |
|                                               | Enter: load   ESC: exit      |  |
|                                               |                              |  |
+--vX.X.X---------------------------------------+-----------------------DFHack-+  -

|-------------------- auto ---------------------|------------ 32 --------------|
]]

--[[
Copyright (c) 2020 Nicolas Ayala `https://github.com/nicolasayala`

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
]]

--+================+
--|    IMPORTS     |
--+================+

local gui = require "gui"
local widgets = require "gui.widgets"
local dialogs = require "gui.dialogs"

local ColorSchemes = dfhack.reqscript('color-schemes')

--+================+
--|    GLOBALS     |
--+================+

VERSION = "0.1.0"

--+================+
--|    DEBUG       |
--+================+

-- local DEBUG = true
function debugBackground(color)
    return DEBUG and {bg = color} or nil
end

--+========================+
--|   FRAMEDSCREEN CLASS   |
--+========================+

--[[
    FramedScreen allowing the same frame definition as `widgets.Widget`
]]
FramedScreen = defclass(FramedScreen, gui.FramedScreen)
FramedScreen.ATTRS {
    --[[ widgets.View ]]--
    -- active = true,
    -- visible = true,
    -- view_id = DEFAULT_NIL,

    --[[ widgets.FramedScreen ]]--
    -- frame_style = BOUNDARY_FRAME,
    -- frame_title = DEFAULT_NIL,
    -- frame_width = DEFAULT_NIL,
    -- frame_height = DEFAULT_NIL,
    -- frame_inset = 0,
    -- frame_background = CLEAR_PEN,

    --[[ New ]]--
    frame = DEFAULT_NIL, -- same as widgets.Panel.frame
    focus_path = DEFAULT_NIL,
    version = "0.0.0",
}

-- Override `FramedScreen.computeFrame`
function FramedScreen:computeFrame(parent_rect)
    local sw, sh = parent_rect.width, parent_rect.height
    return gui.compute_frame_body(sw, sh, self.frame, self.frame_inset, 1)
end

-- Override `FramedScreen.onInput`.
function FramedScreen:onInput(keys)
    -- printall(keys)
    if self:inputToSubviews(keys) then return true end
    if keys.LEAVESCREEN then
        self:dismiss()
        return true
    end
end

function FramedScreen:onRenderFrame(painter, rect)
    FramedScreen.super.onRenderFrame(self, painter, rect)
    local style = self.frame_style
    local pen = style.signature_pen or style.title_pen or style.frame_pen
    painter:seek(rect.x1 + 2,rect.y2):string(string.format("v%s", self.version), pen)
end

--+====================+
--|   HLAYOUT WIDGET   |
--+====================+

--[[
    Widget horizontal layout container, computing childs' width and automatic positioning
]]
HLayout = defclass(HLyaout, widgets.Widget)
HLayout.ATTRS {
    --[[ widgets.Widget ]]--
    -- frame = DEFAULT_NIL,
    -- frame_inset = DEFAULT_NIL,
    -- frame_background = DEFAULT_NIL,

    --[[ New ]]--
    subviews = DEFAULT_NIL,
    space = 0,
    sep_pen = DEFAULT_NIL,
}

function HLayout:init(args)
    self:addviews(args.subviews)
end

--[[
    Views with `frame.w` field take their own width. Leftover parent width
    is spent between childs. Finally all childs are positioned with spacing
    Invisible childs are properly supported
]]
function HLayout:postComputeFrame(frame_body)
    local function is_expand(view)
        return not (view.frame and view.frame.w) or view.frame.expand
    end
    local function spacing(i, views, space)
        if not views[i].visible or i == #views or not views[i+1].visible then
            return 0
        else
            return space
        end
    end
    local avail_w = frame_body.width -- Available width
    local expand_count = 0 -- Number of expanding views
    for i,view in ipairs(self.subviews) do
        local space = spacing(i, self.subviews, self.space)
        if view.visible then
            if is_expand(view) then
                expand_count = expand_count + 1
                view.frame = view.frame or {}
                view.frame.expand = true
            else
                avail_w = avail_w - view.frame.w
            end
            avail_w = avail_w - space
        end
    end
    local auto_w = expand_count > 0 and math.floor(avail_w / expand_count) or 0
    local l_offset = 0
    self.separators = {}
    for i,view in ipairs(self.subviews) do
        local space = spacing(i, self.subviews, self.space)
        if view.visible then
            view.frame.l = l_offset
            if view.frame.expand then
                l_offset = l_offset + auto_w
                view.frame.w = auto_w
            else
                l_offset = l_offset + view.frame.w
            end
        end
        if space ~= 0 then
            table.insert(self.separators, l_offset + math.floor(space / 2))
        end
        l_offset = l_offset + space
    end
end

function HLayout:onRenderBody(painter)
    for _,sep in ipairs(self.separators) do
        painter:fill(sep, 0, sep, painter.height, self.sep_pen)
    end
end

--+==================+
--|   PANEL WIDGET   |
--+==================+

--[[
    Panel widget with frame size relative to parent
]]
Panel = defclass(Panel, widgets.Panel)
Panel.ATTRS {
    --[[ widgets.Widget ]]--
    -- frame = DEFAULT_NIL,
    -- frame_inset = DEFAULT_NIL,
    -- frame_background = DEFAULT_NIL,

    --[[ widgets.Panel ]]--
    -- on_render = DEFAULT_NIL,
    -- on_layout = DEFAULT_NIL,
}

function Panel:preUpdateLayout(parent_rect)
    local frame = self.frame
    if not frame then return end

    local pw, ph = parent_rect.width, parent_rect.height
    local w, h = frame.w, frame.h
    local wp, hp = frame.wp, frame.hp
    local wmin, hmin = frame.wmin or 0, frame.hmin or 0
    local max = math.max
    local min = math.min
    local floor = math.floor

    if not w and wp then frame.w = max(floor(pw * wp), wmin) end -- wmin <= pw * wp
    if not h and hp then frame.h = max(floor(ph * hp), hmin) end -- hmin <= ph * hp
end

--+==================+
--|   COLOR WIDGET   |
--+==================+

--[[
    Simple color panel with color name
]]
Color = defclass(Color, widgets.Widget)
Color.ATTRS {
    --[[ widgets.Widget ]]--
    -- frame = DEFAULT_NIL,
    -- frame_inset = DEFAULT_NIL,
    -- frame_background = DEFAULT_NIL,

    --[[ New ]]--
    name = "White",
    color = COLOR_WHITE,
}

function Color:onRenderBody(painter)
    painter
        :fill(0, 0, painter.width, painter.height, {bg = self.color})
        :seek(0, 0):string(self.name, { fg = self.color, bg = COLOR_BLACK })
        :seek(0, 1):string(self.name, { fg = COLOR_WHITE, bg = self.color })
end

--+=================+
--|   LIST WIDGET   |
--+=================+

--[[
    List widget similiar to `widgets.List` with arrow icons and flag support
]]
List = defclass(List, widgets.Widget)
List.ATTRS {
    --[[ widgets.Widget ]]--
    -- frame = DEFAULT_NIL,
    -- frame_inset = DEFAULT_NIL,
    -- frame_background = DEFAULT_NIL,

    --[[ New ]]--
    items = DEFAULT_NIL,     -- <table> (see `List:setItems`)
    flags = DEFAULT_NIL,     -- <table> (see `List:setFlags`)
    text_pen = COLOR_CYAN,
    text_hpen = COLOR_LIGHTCYAN,
    on_submit = DEFAULT_NIL, -- <func(item)>
    on_select = DEFAULT_NIL, -- <func(item)>
}

local KEYS_STANDARD_SCROLL = {
    STANDARDSCROLL_UP = -1,
    STANDARDSCROLL_DOWN = 1,
    STANDARDSCROLL_PAGEUP = "-page_size",
    STANDARDSCROLL_PAGEDOWN = "+page_size",
    CURSOR_UP_FAST = -5,
    CURSOR_DOWN_FAST = 5
}

function List:init(args)
    self:setItems(self.items)
    self:setFlags(self.flags)
end

--[[ flags template
{
    <flag_key> : {
        pos     : <number>
        text    : <string>
        pen     : <pen>
    }
    ...
}
]]
function List:setFlags(flags)
    self.flags = flags or {}
    -- Sanitize flags
    for _,flag in pairs(self.flags) do
        flag.text = type(flag.text) == "string" and flag.text or ""
        flag.pos = type(flag.pos) == "number" and flag.pos or 0
    end
end

--[[ items template
{
    {
        text  : <string>
        flags : {
            <flag_key> : <boolean>
            ...
        }
    }
    ...
}
]]
function List:setItems(items, selected)
    self.items = items or {}
    -- Sanitize item
    for _,item in ipairs(self.items) do
        item.text = type(item.text) == "string" and item.text or ""
        item.flags = type(item.flags) == "table" and item.flags or {}
    end
    self.size = #self.items
    self:select(selected)
end

function List:getSelected()
    if self.selected then
        return self.items[self.selected]
    else
        return nil
    end
end

function List:isEmpty()
    return self.size == 0
end

function List:select(index, relative)
    if self:isEmpty() then return nil end

    local new_selected
    if not relative then
        -- Clamp index to a valid item index
        new_selected = math.max(1, math.min(self.size, index or 1))
    else
        -- i is relative to current selected index
        local delta = index
        -- Stop high warp (selected + delta <= self.size)
        delta = math.min(self.size - self.selected, delta)
        -- Stop low warp  (selected + delta >= 1)
        delta = math.max(        1 - self.selected, delta)
        new_selected = self.selected + delta
    end

    self.selected = new_selected
    if self.on_select then self.on_select(self:getSelected()) end
    return item
end

function List:postComputeFrame(frame_body)
    self.page_size = frame_body.height
end

function List:onRenderBody(painter)
    if self:isEmpty() then return end
    -- Utility variables
    local page_size = self.page_size
    local ifirst = 1
    local ilast = self.size
    local ibegin = math.floor((self.selected - 1) / page_size) * page_size + 1
    local iend = math.min(ibegin + page_size, ilast)

    for i = ibegin,iend do
        -- Compute up or down arrow indicating available up or down page
        local arrow
        if page_size == 1 and i ~= ilast and i ~= ifirst then
            -- One element per page non first non last (for completeness sake)
            arrow = string.char(18)
        elseif i % page_size == 1 and i ~= ifirst then
            -- Top element with available up page
            arrow = string.char(24) -- 24 : up arrow
        elseif i % page_size == 0 and i ~= ilast then
            -- Bottom element with available down page
            arrow = string.char(25) -- 25 : down arrow
        end

        --[[ Print item ]]--
        local item = self.items[i]
        local y = (i - ibegin)
        local selected = (i == self.selected)
        local width = painter.width
        local text_pen = selected and self.text_hpen or self.text_pen
        painter
            :seek(0,y):string(item.text, text_pen)    -- text
            :seek(width - 1, y):char(arrow, text_pen) -- arrow

        -- Print any flags
        for flag_key,enabled in pairs(item.flags) do
            local flag = self.flags[flag_key]
            if flag and enabled then
                local flag_pen = selected and flag.hpen or flag.pen
                local x = flag.pos % width
                painter:seek(x, y):string(flag.text, flag_pen)
            end
        end
    end
end

function List:onInput(keys)
    if self.on_submit and keys.SELECT and self.selected then
        self.on_submit(self:getSelected())
        return true
    else
        -- Selection keys (up/down)
        for key,value in pairs(KEYS_STANDARD_SCROLL) do
            if keys[key] then
                local delta
                if type(value) == "string" then
                    -- Quotes over `gsub` call to retrieve only the first value
                    delta = tonumber((string.gsub(value, "page_size", self.page_size)))
                else
                    delta = value
                end
                self:select(delta, true)
                return true
            end
        end
    end
    return self:inputToSubviews(keys)
end

--+=================+
--|   GRID WIDGET   |
--+=================+

--[[
    Grid widget container, computes childs' frames
]]
Grid = defclass(Grid, widgets.Panel)
Grid.ATTRS {
    --[[ widgets.Widget ]]--
    -- frame = DEFAULT_NIL,
    -- frame_inset = DEFAULT_NIL,
    -- frame_background = DEFAULT_NIL,

    --[[ New ]]--
    columns = 3,
    rows = 3,
    spacing = 1,
}

function Grid:postComputeFrame(frame_body)
    local w = math.floor((frame_body.width - self.columns * self.spacing) / self.columns)
    local h = math.floor((frame_body.height - self.rows * self.spacing) / self.rows)
    for i, view in ipairs(self.subviews) do
        local c = (i - 1) % self.columns
        local r = math.floor((i - 1) / self.columns)
        local cspace = c * self.spacing
        local rspace = r * self.spacing
        view.frame = {
            l = c * w + cspace,
            t = r * h + rspace,
            w = w,
            h = h,
        }
    end
end

--+=========================+
--|   TAB CONTAINER WIDGET  |
--+=========================+

--[[
    Tabular menu widget
    TODO : Make standalone menu widget class (horizontal list)
]]
TabContainer = defclass(TabContainer, widgets.Widget)
TabContainer.ATTRS {
    --[[ New ]]--
    menu_spacing = 3,
    menu_pen = COLOR_WHITE,
    menu_hpen = COLOR_GREY,
    tabs = DEFAULT_NIL      -- <table> (see `TabContainer:init`)
}

--[[ tabs template
{
    {
        name : <string>
        view : <widget>
    }
    ...
}
]]
function TabContainer:init(args)
    self.tabs = self.tabs or {}
    local views = {}
    for _,tab in ipairs(self.tabs) do
        table.insert(views, tab.view)
    end

    self:addviews {
        widgets.Pages {
            view_id = "pages",
            frame = { t = 2 },
            subviews = views,
        }
    }
    self.pages = self.subviews.pages
end

function TabContainer:select(index, relative)
    if #self.pages.subviews == 0 then return nil end

    if not relative then
        self.pages:setSelected(index)
        return self.pages:getSelectedPage()
    else
        local delta = index
        local selected = self.pages:getSelected()
        return self:select(selected + delta)
    end
end

function TabContainer:onRenderBody(painter)
    local selected = self.pages:getSelected()
    for i, tab in ipairs(self.tabs) do
        local pen = (i == selected) and self.menu_pen or self.menu_hpen
        painter:string(tab.name, pen):advance(self.menu_spacing, 0)
    end
end

function TabContainer:onInput(keys)
    if self:inputToSubviews(keys) then return true end
    if keys.STANDARDSCROLL_RIGHT then
        self:select(1, true)
        return true
    elseif keys.STANDARDSCROLL_LEFT then
        self:select(-1, true)
        return true
    else
        return false
    end
end

--+============+
--|   SCREEN   |
--+============+

-- If that screen is already active, just close it
if screen and screen:isActive() then screen:dismiss() end

screen = FramedScreen {
    frame_title = "Color Schemes",
    focus_path = "color-schemes",
    frame = {w = 32, xalign = 1},
    frame_inset = 0,
    version = VERSION,
}

--[[=== METHODS ===]]--

function screen:onSubmit(item)
    -- Load scheme and update loaded flag
    if item ~= self.loaded_item then
        local scheme = item.scheme
        if scheme.valid then
            if self.loaded_item then self.loaded_item.flags.loaded = false end
            self.loaded_item = item
            self.loaded_item.flags.loaded = true
            ColorSchemes.load(scheme.name)
        end
    end
end

function screen:onRegister()
    dialogs.showInputPrompt(
        "Register",
        "Register color schemes by file or directory",
        COLOR_WHITE,
        "/raw/colors/",
        function(path)
            ColorSchemes.register(path)
            self:populateList()
        end
    )
end

function screen:onSetDefault()
    local selected = self.list:getSelected()
    if selected then
        ColorSchemes.set_default(selected.scheme.name)
        if self.default_item then
            self.default_item.flags.default = false
        end
        self.default_item = selected
        self.default_item.flags.default = true
    end
end

function screen:togglePreview(enabled)
    self.frame.w = not enabled and 32 or nil
    self.preview_label.pen = enabled and COLOR_WHITE or COLOR_DARKGREY
    self.preview.visible = enabled
    self:updateLayout()
end

function screen:onPreview()
    self:togglePreview(not self.preview.visible)
end

function screen:populateList()
    local items = {}
    self.item_loaded = nil
    local schemes = ColorSchemes.list()
    table.sort(schemes, function(a, b) return a.name < b.name end)
    -- Build items
    for _,scheme in ipairs(schemes) do
        local item = {
            text = scheme.name,
            flags = {
                loaded = scheme.loaded,
                default = scheme.default,
                invalid = not scheme.valid,
            },
            scheme = scheme,
        }
        if scheme.loaded then self.loaded_item = item end
        if scheme.default then self.default_item = item end
        table.insert(items, item)
    end
    self.list:setItems(items)
end

--[[=== SCHEMES PANEL ===]]--

screen.header = widgets.Label {
    frame = {t = 0, l = 0},
    text = "Name",
}

screen.preview_label =  {
    text = "preview",
    key = "CUSTOM_P",
    key_sep = ": ",
    on_activate = screen:callback("onPreview"),
}

screen.Lcommands = Panel {
    frame = { h = 6, l = 0, b = 0, wp = 0.5 },
    frame_background = debugBackground(COLOR_LIGHTCYAN),
    subviews = {
        widgets.Label {
            text = {
                { text = "loaded", key = "CUSTOM_SHIFT_L", key_sep = ": ", key_pen = COLOR_GREEN}, "\n",
                { text = "default", key = "CUSTOM_SHIFT_D", key_sep = ": ", key_pen = COLOR_GREY}, "\n",
                "\n",
                screen.preview_label,
                "\n",
                {
                    text = "set default",
                    key = "CUSTOM_SHIFT_D",
                    key_sep = ": ",
                    on_activate = screen:callback("onSetDefault")
                }, "\n",
                { text = "load", key = "SELECT", key_sep = ": "}, "\n",
            }
        }
    }
}

screen.Rcommands = Panel {
    frame = { h = 6, r = 0, b = 0, wp = 0.5 },
    frame_background = debugBackground(COLOR_CYAN),
    subviews = {
        widgets.Label {
            text = {
                { text = "invalid", key = "CUSTOM_SHIFT_I", key_sep = ": ", key_pen = COLOR_LIGHTRED}, "\n",
                "\n",
                "\n",
                {
                    text = "register",
                    key = "CUSTOM_SHIFT_R",
                    key_sep = ": ",
                    on_activate = screen:callback("onRegister")
                }, "\n",
                "\n",
                { text = "exit", key = "LEAVESCREEN", key_sep = ": "},
            }
        }
    }
}

screen.list = List{
    frame = { t = 2, b = 7 },
    frame_background = debugBackground(COLOR_GREEN),
    flags = {
        loaded = {pos = -4, text = "L", pen = COLOR_GREEN, hpen = COLOR_LIGHTGREEN },
        default = {pos = -3, text = "D", pen = COLOR_GREY, hpen = COLOR_WHITE },
        invalid = {pos = -2, text = "I", pen = COLOR_RED, hpen = COLOR_LIGHTRED },
    },
    on_submit = screen:callback("onSubmit")
}

screen.schemes = Panel {
    frame = { w = 30 },
    frame_background = debugBackground(COLOR_LIGHTBLUE),
    frame_inset = 1,
    subviews = {
        screen.header,
        screen.list,
        screen.Lcommands,
        screen.Rcommands
    }
}

--[[=== PREVIEW PANEL ===]]--

screen.colors = Grid {
    view_id = "Colors",
    columns = 2,
    rows = 8,
    spacing = 0,
    subviews = {
        Color { name = "Black",           color = COLOR_BLACK},
        Color { name = "Dark Grey",       color = COLOR_DARKGREY},
        Color { name = "Blue",            color = COLOR_BLUE},
        Color { name = "Light Blue",      color = COLOR_LIGHTBLUE},
        Color { name = "Green",           color = COLOR_GREEN},
        Color { name = "Light Green",     color = COLOR_LIGHTGREEN},
        Color { name = "Cyan",            color = COLOR_CYAN},
        Color { name = "Light Cyan",      color = COLOR_LIGHTCYAN},
        Color { name = "Red",             color = COLOR_RED},
        Color { name = "Light Red",       color = COLOR_LIGHTRED},
        Color { name = "Magenta",         color = COLOR_MAGENTA},
        Color { name = "Light Magenta",   color = COLOR_LIGHTMAGENTA},
        Color { name = "Brown",           color = COLOR_BROWN},
        Color { name = "Yellow",          color = COLOR_YELLOW},
        Color { name = "Grey",            color = COLOR_GREY},
        Color { name = "White",           color = COLOR_WHITE},
    }
}

screen.preview = Panel {
    frame_background = debugBackground(COLOR_RED),
    frame_inset = 1,
    subviews = {
        TabContainer {
            tabs = {
                { name = "Colors", view = screen.colors}
            }
        }
    }
}

--[[=== INITIALIZE ===]]--

screen:addviews {
    HLayout {
        frame_background = debugBackground(COLOR_MAGENTA),
        space = 1,
        sep_pen = screen.frame_style.frame_pen,
        subviews = {
            screen.preview,
            screen.schemes
        }
    }
}
screen:populateList()
screen:togglePreview(false)
screen:show()
