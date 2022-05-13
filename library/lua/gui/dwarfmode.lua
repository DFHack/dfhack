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

refreshSidebar = dfhack.gui.refreshSidebar

-- maps a ui_sidebar_mode to the keycode that would activate that mode when the
-- current screen is 'dwarfmode/Default'
SIDEBAR_MODE_KEYS = {
    [df.ui_sidebar_mode.Default]='',
    [df.ui_sidebar_mode.DesignateMine]='D_DESIGNATE',
    [df.ui_sidebar_mode.QueryBuilding]='D_BUILDJOB',
    [df.ui_sidebar_mode.LookAround]='D_LOOK',
    [df.ui_sidebar_mode.BuildingItems]='D_BUILDITEM',
    [df.ui_sidebar_mode.Stockpiles]='D_STOCKPILES',
    [df.ui_sidebar_mode.Zones]='D_CIVZONE',
    [df.ui_sidebar_mode.ViewUnits]='D_VIEWUNIT',
}

-- Sends ESC keycodes until we get to dwarfmode/Default and then enters the
-- specified sidebar mode with the corresponding keycode. If we don't get to
-- Default after max_esc presses of ESC (default value is 10), we throw an
-- error. The target sidebar mode must be a member of SIDEBAR_MODE_KEYS
function enterSidebarMode(sidebar_mode, max_esc)
    local navkey = SIDEBAR_MODE_KEYS[sidebar_mode]
    if not navkey then
        error(('Invalid or unsupported sidebar mode: %s (%s)')
              :format(sidebar_mode, df.ui_sidebar_mode[sidebar_mode]))
    end
    local max_esc_num = tonumber(max_esc)
    if max_esc and (not max_esc_num or max_esc_num <= 0) then
        error(('max_esc must be a positive number: got %s')
              :format(tostring(max_esc)))
    end
    local remaining_esc = max_esc_num or 10
    local focus_string = ''
    while remaining_esc > 0 do
        local screen = dfhack.gui.getCurViewscreen(true)
        focus_string = dfhack.gui.getFocusString(screen)
        if df.global.ui.main.mode == df.ui_sidebar_mode.Default and
                focus_string == 'dwarfmode/Default' then
            if #navkey > 0 then gui.simulateInput(screen, navkey) end
            if navkey == 'D_DESIGNATE' then
                -- if the z-level happens to be on the surface, the mode will be
                -- set to DesignateChopTrees. we need an extra step to get to
                -- DesignateMine
                gui.simulateInput(dfhack.gui.getCurViewscreen(true),
                                 'DESIGNATE_DIG')
            end
            return
        end
        gui.simulateInput(screen, 'LEAVESCREEN')
        remaining_esc = remaining_esc - 1
    end
    error(('Unable to get into target sidebar mode (%s) from' ..
           ' current UI viewscreen (%s).'):format(
                    df.ui_sidebar_mode[sidebar_mode], focus_string))
end

function getPanelLayout()
    local dims = dfhack.gui.getDwarfmodeViewDims()
    local area_pos = df.global.ui_menu_width[1]
    local menu_pos = df.global.ui_menu_width[0]

    if dims.menu_forced then
        menu_pos = area_pos - 1
    end

    local rv = {
        menu_pos = menu_pos,
        area_pos = area_pos,
        map = gui.mkdims_xy(dims.map_x1, dims.map_y1, dims.map_x2, dims.map_y2),
    }

    if dims.menu_forced then
        rv.menu_forced = true
    end
    if dims.menu_on then
        rv.menu = gui.mkdims_xy(dims.menu_x1, dims.y1, dims.menu_x2, dims.y2)
    end
    if dims.area_on then
        rv.area_map = gui.mkdims_xy(dims.area_x1, dims.y1, dims.area_x2, dims.y2)
    end

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

function get_movement_delta(key, delta, big_step)
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

function get_hotkey_target(key)
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
    frame_background = gui.CLEAR_PEN,

    -- if sidebar_mode is set, we will enter the specified sidebar mode on show
    -- and restore the previous sidebar mode on dismiss. otherwise it is up to
    -- the caller to ensure we are in a sidebar mode where the menu is visible.
    sidebar_mode = DEFAULT_NIL,
}

function MenuOverlay:init()
    if not dfhack.isMapLoaded() then
        -- sidebar menus are only valid when a fort map is loaded
        error('A fortress map must be loaded.')
    end

    if self.sidebar_mode then
        self.saved_sidebar_mode = df.global.ui.main.mode
        -- what mode should we restore when this window is dismissed? ideally, we'd
        -- restore the mode that the user has set, but we should fall back to
        -- restoring the default mode if either of the following conditions are
        -- true:
        -- 1) enterSidebarMode doesn't support getting back into the current mode
        -- 2) a dfhack viewscreen is currently visible. in this case, we can't trust
        --    that the current sidebar mode was set by the user. it could just be a
        --    MenuOverlay subclass that is currently being shown that has set the
        --    sidebar mode for its own purposes.
        if not SIDEBAR_MODE_KEYS[self.saved_sidebar_mode]
                or dfhack.gui.getCurFocus(true):find('^dfhack/') then
            self.saved_sidebar_mode = df.ui_sidebar_mode.Default
        end

        enterSidebarMode(self.sidebar_mode)
    end
end

function MenuOverlay:computeFrame(parent_rect)
    return self.df_layout.menu, gui.inset_frame(self.df_layout.menu, self.frame_inset)
end

function MenuOverlay:onAboutToShow(parent)
    self:updateLayout()
    if not self.df_layout.menu then
        error("The menu panel of dwarfmode is not visible")
    end
end

function MenuOverlay:onDismiss()
    if self.saved_sidebar_mode then
        enterSidebarMode(self.saved_sidebar_mode)
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

-- Framework for managing rendering over the map area. This function is intended
-- to be called from a subclass's onRenderBody() function.
--
-- get_overlay_char_fn takes a coordinate position and an is_cursor boolean and
-- returns the char to render at that position and, optionally, the foreground
-- and background colors to use to draw the char. If nothing should be rendered
-- at that position, the function should return nil. If no foreground color is
-- specified, it defaults to COLOR_GREEN. If no background color is specified,
-- it defaults to COLOR_BLACK.
--
-- bounds_rect has elements {x1, x2, y1, y2} in global map coordinates (not
-- screen coordinates). The rect is intersected with the visible map viewport to
-- get the range over which get_overlay_char_fn is called. If bounds_rect is not
-- specified, the entire viewport is scanned.
--
-- example call from a subclass:
-- function MyMenuOverlaySubclass:onRenderBody()
--     local function get_overlay_char(pos)
--         return safe_index(self.overlay_chars, pos.z, pos.y, pos.x), COLOR_RED
--     end
--     self:renderMapOverlay(get_overlay_char, self.overlay_bounds)
-- end
function MenuOverlay:renderMapOverlay(get_overlay_char_fn, bounds_rect)
    local vp = self:getViewport()
    local rect = gui.ViewRect{rect=vp,
                              clip_view=bounds_rect and gui.ViewRect{rect=bounds_rect} or nil}

    -- nothing to do if the viewport is completely separate from the bounds_rect
    if rect:isDefunct() then return end

    local dc = gui.Painter.new(self.df_layout.map)
    local z = df.global.window_z
    local cursor = getCursorPos()
    for y=rect.clip_y1,rect.clip_y2 do
        for x=rect.clip_x1,rect.clip_x2 do
            local pos = xyz2pos(x, y, z)
            local overlay_char, fg_color, bg_color = get_overlay_char_fn(
                    pos, same_xy(cursor, pos))
            if not overlay_char then goto continue end
            local stile = vp:tileToScreen(pos)
            dc:map(true):seek(stile.x, stile.y):
                    pen(fg_color or COLOR_GREEN, bg_color or COLOR_BLACK):
                    char(overlay_char):map(false)
            ::continue::
        end
    end
end

--fakes a "real" workshop sidebar menu, but on exactly selected workshop
WorkshopOverlay = defclass(WorkshopOverlay, MenuOverlay)
WorkshopOverlay.focus_path="WorkshopOverlay"
WorkshopOverlay.ATTRS={
    workshop=DEFAULT_NIL,
}
function WorkshopOverlay:onAboutToShow(below)
    WorkshopOverlay.super.onAboutToShow(self,below)

    if df.global.world.selected_building ~= self.workshop then
        error("The workshop overlay tried to show up for incorrect workshop")
    end
end
function WorkshopOverlay:onInput(keys)
    local allowedKeys={ --TODO add options: job management, profile, etc...
        "CURSOR_RIGHT","CURSOR_RIGHT_FAST","CURSOR_LEFT","CURSOR_LEFT_FAST","CURSOR_UP","CURSOR_UP_FAST","CURSOR_DOWN","CURSOR_DOWN_FAST",
        "CURSOR_UPRIGHT","CURSOR_UPRIGHT_FAST","CURSOR_UPLEFT","CURSOR_UPLEFT_FAST","CURSOR_DOWNRIGHT","CURSOR_DOWNRIGHT_FAST","CURSOR_DOWNLEFT","CURSOR_DOWNLEFT_FAST",
        "CURSOR_UP_Z","CURSOR_DOWN_Z","DESTROYBUILDING","CHANGETAB","SUSPENDBUILDING"}

    if keys.LEAVESCREEN then
        self:dismiss()
        self:sendInputToParent('LEAVESCREEN')
    elseif keys.CHANGETAB then
        self:sendInputToParent("CHANGETAB")
        self:inputToSubviews(keys)
        self:updateLayout()
    else
        for _,name in ipairs(allowedKeys) do
            if keys[name] then
                self:sendInputToParent(name)
                break
            end
        end
        self:inputToSubviews(keys)
    end
    if df.global.world.selected_building ~= self.workshop then
        self:dismiss()
        return
    end
end
function WorkshopOverlay:onGetSelectedBuilding()
    return self.workshop
end
local function is_slated_for_remove( bld )
    for i,v in ipairs(bld.jobs) do
        if v.job_type==df.job_type.DestroyBuilding then
            return true
        end
    end
    return false
end
function WorkshopOverlay:render(dc)
    self:renderParent()
    if df.global.world.selected_building ~= self.workshop then
        return
    end
    if is_slated_for_remove(self.workshop) then
        return
    end

    WorkshopOverlay.super.render(self, dc)
end
return _ENV
