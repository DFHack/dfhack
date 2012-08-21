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

function getCursorPos()
    return copyall(df.global.cursor)
end

function setCursorPos(cursor)
    df.global.cursor = cursor
end

function getViewportPos()
    return {
        x = df.global.window_x,
        y = df.global.window_y,
        z = df.global.window_z
    }
end

function clipViewport(view, layout)
    local map = df.global.world.map
    layout = layout or getPanelLayout()
    return {
        x = math.max(0, math.min(view.x, map.x_count-layout.map.width)),
        y = math.max(0, math.min(view.y, map.y_count-layout.map.height)),
        z = math.max(0, math.min(view.z, map.z_count-1))
    }
end

function setViewportPos(view, layout)
    local map = df.global.world.map
    layout = layout or getPanelLayout()
    local vp = clipViewport(view, layout)
    df.global.window_x = vp.x
    df.global.window_y = vp.y
    df.global.window_z = vp.z
    return vp
end

function centerViewportOn(target, layout)
    layout = layout or getPanelLayout()
    local view = xyz2pos(
        target.x-math.floor(layout.map.width/2),
        target.y-math.floor(layout.map.height/2),
        target.z
    )
    return setViewportPos(view, layout)
end

function isInViewport(layout,view,target,gap)
    gap = gap or 0

    local map = df.global.world.map
    return math.max(target.x-gap,0) >= view.x
       and math.min(target.x+gap,map.x_count-1) < view.x+layout.map.width
       and math.max(target.y-gap,0) >= view.y
       and math.min(target.y+gap,map.y_count-1) < view.y+layout.map.height
       and target.z == view.z
end

function revealInViewport(target,gap,view,layout)
    layout = layout or getPanelLayout()

    if not isInViewport(layout, getViewportPos(), target, gap) then
        if view and isInViewport(layout, view, target, gap) then
            return setViewportPos(view, layout)
        else
            return centerViewportOn(target, layout)
        end
    end
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
    -- Dwarfmode constantly needs repainting
    dscreen.invalidate()
end

function DwarfOverlay:onAboutToShow(below)
    local screen = dfhack.gui.getCurViewscreen()
    if below then screen = below.parent end
    if not df.viewscreen_dwarfmodest:is_instance(screen) then
        error("This screen requires the main dwarfmode view")
    end
end

MenuOverlay = defclass(MenuOverlay, DwarfOverlay)

function MenuOverlay:onAboutToShow(below)
    DwarfOverlay.onAboutToShow(self,below)

    self:updateLayout()
    if not self.df_layout.menu then
        error("The menu panel of dwarfmode is not visible")
    end
end

function MenuOverlay:onRender()
    self:renderParent()
    self:updateLayout()

    local menu = self.df_layout.menu
    if menu then
        self:onRenderBody(gui.Painter.new(menu))
    end
end

return _ENV
