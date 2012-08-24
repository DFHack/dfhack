-- Support for messing with the dwarfmode screen

local _ENV = mkmodule('gui.dwarfmode')

local gui = require('gui')
local dscreen = dfhack.screen
local world_map = df.global.world.map

AREA_MAP_WIDTH = 23
MENU_WIDTH = 30

function getPanelLayout()
    local sw, sh = dscreen.getWindowSize()
    local view_height = sh-2
    local view_rb = sw-1
    local area_x2 = sw-AREA_MAP_WIDTH-2
    local menu_x2 = sw-MENU_WIDTH-2
    local menu_x1 = area_x2-MENU_WIDTH-1
    local area_pos = df.global.ui_area_map_width
    local menu_pos = df.global.ui_menu_width
    local rv = {}

    if area_pos < 3 then
        rv.area_map = gui.mkdims_xy(area_x2+1,1,view_rb-1,view_height)
        view_rb = area_x2
    end
    if menu_pos < area_pos or df.global.ui.main.mode ~= 0 then
        if menu_pos >= area_pos then
            rv.menu_forced = true
            menu_pos = area_pos-1
        end
        local menu_x = menu_x2
        if menu_pos < 2 then menu_x = menu_x1 end
        rv.menu = gui.mkdims_xy(menu_x+1,1,view_rb-1,view_height)
        view_rb = menu_x
    end
    rv.area_pos = area_pos
    rv.menu_pos = menu_pos
    rv.map = gui.mkdims_xy(1,1,view_rb-1,view_height)
    return rv
end

function getCursorPos()
    if df.global.cursor.x ~= -30000 then
        return copyall(df.global.cursor)
    end
end

function setCursorPos(cursor)
    df.global.cursor = cursor
end

function clearCursorPos()
    df.global.cursor = xyz2pos(nil)
end

Viewport = defclass(Viewport)

function Viewport.make(map,x,y,z)
    local self = gui.mkdims_wh(x,y,map.width,map.height)
    self.z = z
    return mkinstance(Viewport, self)
end

function Viewport.get(layout)
    return Viewport.make(
        (layout or getPanelLayout()).map,
        df.global.window_x,
        df.global.window_y,
        df.global.window_z
    )
end

function Viewport:resize(layout)
    return Viewport.make(
        (layout or getPanelLayout()).map,
        self.x1, self.y1, self.z
    )
end

function Viewport:set()
    local vp = self:clip()
    df.global.window_x = vp.x1
    df.global.window_y = vp.y1
    df.global.window_z = vp.z
    return vp
end

function Viewport:clip(x,y,z)
    return self:make(
        math.max(0, math.min(x or self.x1, world_map.x_count-self.width)),
        math.max(0, math.min(y or self.y1, world_map.y_count-self.height)),
        math.max(0, math.min(z or self.z, world_map.z_count-1))
    )
end

function Viewport:isVisibleXY(target,gap)
    gap = gap or 0

    return math.max(target.x-gap,0) >= self.x1
       and math.min(target.x+gap,world_map.x_count-1) <= self.x2
       and math.max(target.y-gap,0) >= self.y1
       and math.min(target.y+gap,world_map.y_count-1) <= self.y2
end

function Viewport:isVisible(target,gap)
    gap = gap or 0

    return self:isVisibleXY(target,gap) and target.z == self.z
end

function Viewport:centerOn(target)
    return self:clip(
        target.x - math.floor(self.width/2),
        target.y - math.floor(self.height/2),
        target.z
    )
end

function Viewport:scrollTo(target,gap)
    gap = math.max(0, gap or 5)
    if gap*2 >= math.min(self.width, self.height) then
        gap = math.floor(math.min(self.width, self.height)/2)
    end
    local x = math.min(self.x1, target.x-gap)
    x = math.max(x, target.x+gap+1-self.width)
    local y = math.min(self.y1, target.y-gap)
    y = math.max(y, target.y+gap+1-self.height)
    return self:clip(x, y, target.z)
end

function Viewport:reveal(target,gap,max_scroll,scroll_gap,scroll_z)
    gap = math.max(0, gap or 5)
    if self:isVisible(target, gap) then
        return self
    end

    max_scroll = math.max(0, max_scroll or 5)
    if self:isVisibleXY(target, -max_scroll)
    and (scroll_z or target.z == self.z) then
        return self:scrollTo(target, scroll_gap or gap)
    else
        return self:centerOn(target)
    end
end

MOVEMENT_KEYS = {
    CURSOR_UP = { 0, -1, 0 }, CURSOR_DOWN = { 0, 1, 0 },
    CURSOR_LEFT = { -1, 0, 0 }, CURSOR_RIGHT = { 1, 0, 0 },
    CURSOR_UPLEFT = { -1, -1, 0 }, CURSOR_UPRIGHT = { 1, -1, 0 },
    CURSOR_DOWNLEFT = { -1, 1, 0 }, CURSOR_DOWNRIGHT = { 1, 1, 0 },
    CURSOR_UP_FAST = { 0, -1, 0, true }, CURSOR_DOWN_FAST = { 0, 1, 0, true },
    CURSOR_LEFT_FAST = { -1, 0, 0, true }, CURSOR_RIGHT_FAST = { 1, 0, 0, true },
    CURSOR_UPLEFT_FAST = { -1, -1, 0, true }, CURSOR_UPRIGHT_FAST = { 1, -1, 0, true },
    CURSOR_DOWNLEFT_FAST = { -1, 1, 0, true }, CURSOR_DOWNRIGHT_FAST = { 1, 1, 0, true },
    CURSOR_UP_Z = { 0, 0, 1 }, CURSOR_DOWN_Z = { 0, 0, -1 },
    CURSOR_UP_Z_AUX = { 0, 0, 1 }, CURSOR_DOWN_Z_AUX = { 0, 0, -1 },
}

function Viewport:scrollByKey(key)
    local info = MOVEMENT_KEYS[key]
    if info then
        local delta = 10
        if info[4] then delta = 20 end

        return self:clip(
            self.x1 + delta*info[1],
            self.y1 + delta*info[2],
            self.z + info[3]
        )
    else
        return self
    end
end

DwarfOverlay = defclass(DwarfOverlay, gui.Screen)

function DwarfOverlay:updateLayout()
    self.df_layout = getPanelLayout()
end

function DwarfOverlay:getViewport(old_vp)
    if old_vp then
        return old_vp:resize(self.df_layout)
    else
        return Viewport.get(self.df_layout)
    end
end

function DwarfOverlay:propagateMoveKeys(keys)
    for code,_ in pairs(MOVEMENT_KEYS) do
        if keys[code] then
            self:sendInputToParent(code)
            return code
        end
    end
end

function DwarfOverlay:simulateViewScroll(keys, anchor, no_clip_cursor)
    local layout = self.df_layout
    local cursor = getCursorPos()

    anchor = anchor or cursor

    if anchor and keys.A_MOVE_SAME_SQUARE then
        self:getViewport():centerOn(anchor):set()
        return 'A_MOVE_SAME_SQUARE'
    end

    for code,_ in pairs(MOVEMENT_KEYS) do
        if keys[code] then
            local vp = self:getViewport():scrollByKey(code)
            if (cursor and not no_clip_cursor) or no_clip_cursor == false then
                vp = vp:reveal(anchor,4,20,4,true)
            end
            vp:set()

            return code
        end
    end
end

function DwarfOverlay:onAboutToShow(below)
    local screen = dfhack.gui.getCurViewscreen()
    if below then screen = below.parent end
    if not df.viewscreen_dwarfmodest:is_instance(screen) then
        error("This screen requires the main dwarfmode view")
    end
end

MenuOverlay = defclass(MenuOverlay, DwarfOverlay)

function MenuOverlay:updateLayout()
    DwarfOverlay.updateLayout(self)
    self.frame_rect = self.df_layout.menu
end

MenuOverlay.getWindowSize = gui.FramedScreen.getWindowSize
MenuOverlay.getMousePos = gui.FramedScreen.getMousePos

function MenuOverlay:onAboutToShow(below)
    DwarfOverlay.onAboutToShow(self,below)

    self:updateLayout()
    if not self.df_layout.menu then
        error("The menu panel of dwarfmode is not visible")
    end
end

function MenuOverlay:onRender()
    self:renderParent()

    local menu = self.df_layout.menu
    if menu then
        -- Paint signature on the frame.
        dscreen.paintString(
            {fg=COLOR_BLACK,bg=COLOR_DARKGREY},
            menu.x1+1, menu.y2+1, "DFHack"
        )

        self:onRenderBody(gui.Painter.new(menu))
    end
end

return _ENV
