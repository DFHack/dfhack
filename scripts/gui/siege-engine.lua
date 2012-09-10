-- Front-end for the siege engine plugin.

local utils = require 'utils'
local gui = require 'gui'
local guidm = require 'gui.dwarfmode'
local dlg = require 'gui.dialogs'

local plugin = require 'plugins.siege-engine'
local wmap = df.global.world.map

-- Globals kept between script calls
last_target_min = last_target_min or nil
last_target_max = last_target_max or nil

SiegeEngine = defclass(SiegeEngine, guidm.MenuOverlay)

SiegeEngine.focus_path = 'siege-engine'

function SiegeEngine:init(building)
    self:init_fields{
        building = building,
        center = utils.getBuildingCenter(building),
        links = {}, selected = 1,
    }
    guidm.MenuOverlay.init(self)
    self.mode_main = {
        render = self:callback 'onRenderBody_main',
        input = self:callback 'onInput_main',
    }
    self.mode_aim = {
        render = self:callback 'onRenderBody_aim',
        input = self:callback 'onInput_aim',
    }
    return self
end

function SiegeEngine:onShow()
    guidm.MenuOverlay.onShow(self)

    self.old_cursor = guidm.getCursorPos()
    self.old_viewport = self:getViewport()

    self.mode = self.mode_main
    self:showCursor(false)
end

function SiegeEngine:onDestroy()
    self:selectBuilding(self.building, self.old_cursor, self.old_viewport, 10)
end

function SiegeEngine:showCursor(enable)
    local cursor = guidm.getCursorPos()
    if cursor and not enable then
        self.cursor = cursor
        self.target_select_first = nil
        guidm.clearCursorPos()
    elseif not cursor and enable then
        local view = self:getViewport()
        cursor = self.cursor
        if not cursor or not view:isVisible(cursor) then
            cursor = view:getCenter()
        end
        self.cursor = nil
        guidm.setCursorPos(cursor)
    end
end

function SiegeEngine:centerViewOn(pos)
    local cursor = guidm.getCursorPos()
    if cursor then
        guidm.setCursorPos(pos)
    else
        self.cursor = pos
    end
    self:getViewport():centerOn(pos):set()
end

function SiegeEngine:zoomToTarget()
    local target_min, target_max = plugin.getTargetArea(self.building)
    if target_min then
        local cx = math.floor((target_min.x + target_max.x)/2)
        local cy = math.floor((target_min.y + target_max.y)/2)
        local cz = math.floor((target_min.z + target_max.z)/2)
        for z = cz,target_max.z do
            if plugin.getTileStatus(self.building, xyz2pos(cx,cy,z)) ~= 'blocked' then
                cz = z
                break
            end
        end
        self:centerViewOn(xyz2pos(cx,cy,cz))
    end
end

function paint_target_grid(dc, view, origin, p1, p2)
    local r1, sz, r2 = guidm.getSelectionRange(p1, p2)

    if view.z < r1.z or view.z > r2.z then
        return
    end

    local p1 = view:tileToScreen(r1)
    local p2 = view:tileToScreen(r2)
    local org = view:tileToScreen(origin)
    dc:pen{ fg = COLOR_CYAN, bg = COLOR_CYAN, ch = '+', bold = true }

    -- Frame
    dc:fill(p1.x,p1.y,p1.x,p2.y)
    dc:fill(p1.x,p1.y,p2.x,p1.y)
    dc:fill(p2.x,p1.y,p2.x,p2.y)
    dc:fill(p1.x,p2.y,p2.x,p2.y)

    -- Grid
    local gxmin = org.x+10*math.ceil((p1.x-org.x)/10)
    local gxmax = org.x+10*math.floor((p2.x-org.x)/10)
    local gymin = org.y+10*math.ceil((p1.y-org.y)/10)
    local gymax = org.y+10*math.floor((p2.y-org.y)/10)
    for x = gxmin,gxmax,10 do
        for y = gymin,gymax,10 do
            dc:fill(p1.x,y,p2.x,y)
            dc:fill(x,p1.y,x,p2.y)
        end
    end
end

function SiegeEngine:renderTargetView(target_min, target_max)
    local view = self:getViewport()
    local map = self.df_layout.map
    local map_dc = gui.Painter.new(map)

    plugin.paintAimScreen(
        self.building, view:getPos(),
        xy2pos(map.x1, map.y1), view:getSize()
    )

    if target_min and math.floor(dfhack.getTickCount()/500) % 2 == 0 then
        paint_target_grid(map_dc, view, self.center, target_min, target_max)
    end

    local cursor = guidm.getCursorPos()
    if cursor then
        local cx, cy, cz = pos2xyz(view:tileToScreen(cursor))
        if cz == 0 then
            map_dc:seek(cx,cy):char('X', COLOR_YELLOW)
        end
    end
end

function SiegeEngine:onRenderBody_main(dc)
    dc:newline(1):pen(COLOR_WHITE):string("Target: ")

    local target_min, target_max = plugin.getTargetArea(self.building)
    if target_min then
        dc:string(
            (target_max.x-target_min.x+1).."x"..
            (target_max.y-target_min.y+1).."x"..
            (target_max.z-target_min.z+1).." Rect"
        )
    else
        dc:string("None (default)")
    end

    dc:newline(3):string("r",COLOR_LIGHTGREEN):string(": Rectangle")
    if last_target_min then
        dc:string(", "):string("p",COLOR_LIGHTGREEN):string(": Paste")
    end
    dc:newline(3)
    if target_min then
        dc:string("x",COLOR_LIGHTGREEN):string(": Clear, ")
        dc:string("z",COLOR_LIGHTGREEN):string(": Zoom")
    end

    if self.target_select_first then
        self:renderTargetView(self.target_select_first, guidm.getCursorPos())
    else
        self:renderTargetView(target_min, target_max)
    end
end

function SiegeEngine:setTargetArea(p1, p2)
    self.target_select_first = nil

    if not plugin.setTargetArea(self.building, p1, p2) then
        dlg.showMessage(
            'Set Target Area',
            'Could not set the target area', COLOR_LIGHTRED
        )
    else
        last_target_min = p1
        last_target_max = p2
    end
end

function SiegeEngine:onInput_main(keys)
    if keys.CUSTOM_R then
        self:showCursor(true)
        self.target_select_first = nil
        self.mode = self.mode_aim
    elseif keys.CUSTOM_P and last_target_min then
        self:setTargetArea(last_target_min, last_target_max)
    elseif keys.CUSTOM_Z then
        self:zoomToTarget()
    elseif keys.CUSTOM_X then
        plugin.clearTargetArea(self.building)
    elseif self:simulateViewScroll(keys) then
        self.cursor = nil
    else
        return false
    end
    return true
end

local status_table = {
    ok = { pen = COLOR_GREEN, msg = "Target accessible" },
    out_of_range = { pen = COLOR_CYAN, msg = "Target out of range" },
    blocked = { pen = COLOR_RED, msg = "Target obstructed" },
}

function SiegeEngine:onRenderBody_aim(dc)
    local cursor = guidm.getCursorPos()
    local first = self.target_select_first

    dc:newline(1):string('Select target rectangle'):newline()

    local info = status_table[plugin.getTileStatus(self.building, cursor)]
    if info then
        dc:newline(2):string(info.msg, info.pen)
    else
        dc:newline(2):string('ERROR', COLOR_RED)
    end

    dc:newline():newline(1):string("Enter",COLOR_LIGHTGREEN)
    if first then
        dc:string(": Finish rectangle")
    else
        dc:string(": Start rectangle")
    end
    dc:newline()

    local target_min, target_max = plugin.getTargetArea(self.building)
    if target_min then
        dc:newline(1):string("z",COLOR_LIGHTGREEN):string(": Zoom to current target")
    end

    if first then
        self:renderTargetView(first, cursor)
    else
        local target_min, target_max = plugin.getTargetArea(self.building)
        self:renderTargetView(target_min, target_max)
    end
end

function SiegeEngine:onInput_aim(keys)
    if keys.SELECT then
        local cursor = guidm.getCursorPos()
        if self.target_select_first then
            self:setTargetArea(self.target_select_first, cursor)

            self.mode = self.mode_main
            self:showCursor(false)
        else
            self.target_select_first = cursor
        end
    elseif keys.CUSTOM_Z then
        self:zoomToTarget()
    elseif keys.LEAVESCREEN then
        self.mode = self.mode_main
        self:showCursor(false)
    elseif self:simulateCursorMovement(keys) then
        self.cursor = nil
    else
        return false
    end
    return true
end

function SiegeEngine:onRenderBody(dc)
    dc:clear()
    dc:seek(1,1):pen(COLOR_WHITE):string(utils.getBuildingName(self.building)):newline()

    self.mode.render(dc)

    dc:seek(1, math.max(dc:localY(), 21)):pen(COLOR_WHITE)
    dc:string("ESC", COLOR_LIGHTGREEN):string(": Back, ")
    dc:string("c", COLOR_LIGHTGREEN):string(": Recenter")
end

function SiegeEngine:onInput(keys)
    if self.mode.input(keys) then
        --
    elseif keys.CUSTOM_C then
        self:centerViewOn(self.center)
    elseif keys.LEAVESCREEN then
        self:dismiss()
    end
end

if not string.match(dfhack.gui.getCurFocus(), '^dwarfmode/QueryBuilding/Some/SiegeEngine') then
    qerror("This script requires a siege engine selected in 'q' mode")
end

local building = df.global.world.selected_building

if not df.building_siegeenginest:is_instance(building) then
    qerror("A siege engine must be selected")
end

local list = mkinstance(SiegeEngine):init(df.global.world.selected_building)
list:show()
