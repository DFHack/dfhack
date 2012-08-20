-- Support for messing with the dwarfmode screen

local _ENV = mkmodule('gui.dwarfmode')

local gui = require('gui')
local dscreen = dfhack.screen

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

function getViewportPos()
    return {
        x = df.global.window_x,
        y = df.global.window_y,
        z = df.global.window_z
    }
end

function getCursorPos()
    return copyall(df.global.cursor)
end

function setCursorPos(cursor)
    df.global.cursor = cursor
end

function setViewportPos(layout,view)
    local map = df.global.world.map
    df.global.window_x = math.max(0, math.min(view.x, map.x_count-layout.map.width))
    df.global.window_y = math.max(0, math.min(view.y, map.y_count-layout.map.height))
    df.global.window_z = math.max(0, math.min(view.z, map.z_count-1))
    return getViewportPos()
end

function centerViewportOn(layout,target)
    local mapsz = layout.map
    local view = xyz2pos(
        target.x-math.floor(layout.map.width/2),
        target.y-math.floor(layout.map.height/2),
        target.z
    )
    return setViewportPos(layout, view)
end

function isInViewport(layout,view,target,gap)
    gap = gap or 0
    return target.x-gap >= view.x and target.x+gap < view.x+layout.map.width
       and target.y-gap >= view.y and target.y+gap < view.y+layout.map.height
       and target.z == view.z
end

DwarfOverlay = defclass(DwarfOverlay, gui.Screen)

function DwarfOverlay:updateLayout()
    self.df_layout = getPanelLayout()
    self.df_viewport = getViewportPos()
    self.df_cursor = getCursorPos()
end

local move_keys = {
    'CURSOR_UP', 'CURSOR_DOWN', 'CURSOR_LEFT', 'CURSOR_RIGHT',
    'CURSOR_UPLEFT', 'CURSOR_UPRIGHT', 'CURSOR_DOWNLEFT', 'CURSOR_DOWNRIGHT',
    'CURSOR_UP_FAST', 'CURSOR_DOWN_FAST', 'CURSOR_LEFT_FAST', 'CURSOR_RIGHT_FAST',
    'CURSOR_UPLEFT_FAST', 'CURSOR_UPRIGHT_FAST', 'CURSOR_DOWNLEFT_FAST', 'CURSOR_DOWNRIGHT_FAST',
    'CURSOR_UP_Z', 'CURSOR_DOWN_Z', 'CURSOR_UP_Z_AUX', 'CURSOR_DOWN_Z_AUX'
}

function DwarfOverlay:propagateMoveKeys(keys)
    for _,v in ipairs(move_keys) do
        if keys[v] then
            self:inputToParent(v)
            return
        end
    end
end

function DwarfOverlay:onIdle()
    dscreen.invalidate()
end

function DwarfOverlay:clearMenu()
    local menu = self.df_layout.menu
    if not menu then return nil end
    dscreen.fillRect(gui.CLEAR_PEN,menu.x1,menu.y1,menu.x2,menu.y2)
    return menu.x1,menu.y1,menu.width,menu.height
end

return _ENV
