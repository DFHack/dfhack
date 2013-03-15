-- Support for messing with the dwarfmode screen

local _ENV = mkmodule('gui.dwarfmode')

local gui = require('gui')
local utils = require('utils')

local dscreen = dfhack.screen

local g_cursor = df.global.cursor
local g_sel_rect = df.global.selection_rect
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
    if g_cursor.x ~= -30000 then
        return copyall(g_cursor)
    end
end

function setCursorPos(cursor)
    df.global.cursor = cursor
end

function clearCursorPos()
    df.global.cursor = xyz2pos(nil)
end

function getSelection()
    local p1, p2
    if g_sel_rect.start_x ~= -30000 then
        p1 = xyz2pos(g_sel_rect.start_x, g_sel_rect.start_y, g_sel_rect.start_z)
    end
    if g_sel_rect.end_x ~= -30000 then
        p2 = xyz2pos(g_sel_rect.end_x, g_sel_rect.end_y, g_sel_rect.end_z)
    end
    return p1, p2
end

function setSelectionStart(pos)
    g_sel_rect.start_x = pos.x
    g_sel_rect.start_y = pos.y
    g_sel_rect.start_z = pos.z
end

function setSelectionEnd(pos)
    g_sel_rect.end_x = pos.x
    g_sel_rect.end_y = pos.y
    g_sel_rect.end_z = pos.z
end

function clearSelection()
    g_sel_rect.start_x = -30000
    g_sel_rect.start_y = -30000
    g_sel_rect.start_z = -30000
    g_sel_rect.end_x = -30000
    g_sel_rect.end_y = -30000
    g_sel_rect.end_z = -30000
end

function getSelectionRange(p1, p2)
    local r1 = xyz2pos(
        math.min(p1.x, p2.x), math.min(p1.y, p2.y), math.min(p1.z, p2.z)
    )
    local r2 = xyz2pos(
        math.max(p1.x, p2.x), math.max(p1.y, p2.y), math.max(p1.z, p2.z)
    )
    local sz = xyz2pos(
        r2.x - r1.x + 1, r2.y - r1.y + 1, r2.z - r1.z + 1
    )
    return r1, sz, r2
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

function Viewport:getPos()
    return xyz2pos(self.x1, self.y1, self.z)
end

function Viewport:getSize()
    return xy2pos(self.width, self.height)
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

function Viewport:tileToScreen(coord)
    return xyz2pos(coord.x - self.x1, coord.y - self.y1, coord.z - self.z)
end

function Viewport:getCenter()
    return xyz2pos(
        math.floor((self.x2+self.x1)/2),
        math.floor((self.y2+self.y1)/2),
        self.z
    )
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

local function get_movement_delta(key, delta, big_step)
    local info = MOVEMENT_KEYS[key]
    if info then
        if info[4] then
            delta = big_step
        end

        return delta*info[1], delta*info[2], info[3]
    end
end

HOTKEY_KEYS = {}

for i,v in ipairs(df.global.ui.main.hotkeys) do
    HOTKEY_KEYS['D_HOTKEY'..(i+1)] = v
end

local function get_hotkey_target(key)
    local hk = HOTKEY_KEYS[key]
    if hk and hk.cmd == df.ui_hotkey.T_cmd.Zoom then
        return xyz2pos(hk.x, hk.y, hk.z)
    end
end

function Viewport:scrollByKey(key)
    local dx, dy, dz = get_movement_delta(key, 10, 20)
    if dx then
        return self:clip(
            self.x1 + dx,
            self.y1 + dy,
            self.z + dz
        )
    else
        local hk_pos = get_hotkey_target(key)
        if hk_pos then
            return self:centerOn(hk_pos)
        else
            return self
        end
    end
end

DwarfOverlay = defclass(DwarfOverlay, gui.Screen)

function DwarfOverlay:updateLayout(parent_rect)
    self.df_layout = getPanelLayout()
    DwarfOverlay.super.updateLayout(self, parent_rect)
end

function DwarfOverlay:getViewport(old_vp)
    if old_vp then
        return old_vp:resize(self.df_layout)
    else
        return Viewport.get(self.df_layout)
    end
end

function DwarfOverlay:moveCursorTo(cursor,viewport,gap)
    setCursorPos(cursor)
    self:zoomViewportTo(cursor,viewport,gap)
end

function DwarfOverlay:zoomViewportTo(target, viewport, gap)
    if gap and self:getViewport():isVisible(target, gap) then
        return
    end
    self:getViewport(viewport):reveal(target, 5, 0, 10):set()
end

function DwarfOverlay:selectBuilding(building,cursor,viewport,gap)
    cursor = cursor or utils.getBuildingCenter(building)

    df.global.world.selected_building = building
    self:moveCursorTo(cursor, viewport, gap)
end

function DwarfOverlay:propagateMoveKeys(keys)
    for code,_ in pairs(keys) do
        if MOVEMENT_KEYS[code] or HOTKEY_KEYS[code] then
            if not HOTKEY_KEYS[code] or get_hotkey_target(code) then
                self:sendInputToParent(code)
            end
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

    for code,_ in pairs(keys) do
        if MOVEMENT_KEYS[code] or HOTKEY_KEYS[code] then
            local vp = self:getViewport():scrollByKey(code)
            if (cursor and not no_clip_cursor) or no_clip_cursor == false then
                vp = vp:reveal(anchor,4,20,4,true)
            end
            vp:set()

            return code
        end
    end
end

function DwarfOverlay:simulateCursorMovement(keys, anchor)
    local layout = self.df_layout
    local cursor = getCursorPos()
    local cx, cy, cz = pos2xyz(cursor)

    if anchor and keys.A_MOVE_SAME_SQUARE then
        setCursorPos(anchor)
        self:getViewport():centerOn(anchor):set()
        return 'A_MOVE_SAME_SQUARE'
    end

    for code,_ in pairs(keys) do
        if MOVEMENT_KEYS[code] then
            local dx, dy, dz = get_movement_delta(code, 1, 10)
            local ncur = xyz2pos(cx+dx, cy+dy, cz+dz)

            if dfhack.maps.isValidTilePos(ncur) then
                setCursorPos(ncur)
                self:getViewport():reveal(ncur,4,10,6,true):set()
            end

            return code
        elseif HOTKEY_KEYS[code] then
            local pos = get_hotkey_target(code)

            if pos and dfhack.maps.isValidTilePos(pos) then
                setCursorPos(pos)
                self:getViewport():centerOn(pos):set()
            end

            return code
        end
    end
end

function DwarfOverlay:onAboutToShow(parent)
    if not df.viewscreen_dwarfmodest:is_instance(parent) then
        error("This screen requires the main dwarfmode view")
    end
end

MenuOverlay = defclass(MenuOverlay, DwarfOverlay)

MenuOverlay.ATTRS {
    frame_inset = 0,
    frame_background = CLEAR_PEN,
}

function MenuOverlay:computeFrame(parent_rect)
    return self.df_layout.menu, gui.inset_frame(self.df_layout.menu, self.frame_inset)
end

function MenuOverlay:onAboutToShow(below)
    MenuOverlay.super.onAboutToShow(self,below)

    self:updateLayout()
    if not self.df_layout.menu then
        error("The menu panel of dwarfmode is not visible")
    end
end

function MenuOverlay:render(dc)
    self:renderParent()

    local menu = self.df_layout.menu
    if menu then
        -- Paint signature on the frame.
        dscreen.paintString(
            {fg=COLOR_BLACK,bg=COLOR_DARKGREY},
            menu.x1+1, menu.y2+1, "DFHack"
        )

        if self.frame_background then
            dc:fill(menu, self.frame_background)
        end

        MenuOverlay.super.render(self, dc)
    end
end

return _ENV
