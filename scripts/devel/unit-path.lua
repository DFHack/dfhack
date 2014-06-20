-- Show the internal path a unit is currently following.

local utils = require 'utils'
local gui = require 'gui'
local guidm = require 'gui.dwarfmode'
local dlg = require 'gui.dialogs'

local tile_attrs = df.tiletype.attrs

UnitPathUI = defclass(UnitPathUI, guidm.MenuOverlay)

UnitPathUI.focus_path = 'unit-path'

UnitPathUI.ATTRS {
    unit = DEFAULT_NIL,
}

function UnitPathUI:init()
    self.saved_mode = df.global.ui.main.mode

end

function UnitPathUI:onShow()
    -- with cursor, but without those ugly lines from native hauling mode
    df.global.ui.main.mode = df.ui_sidebar_mode.Stockpiles
end

function UnitPathUI:onDestroy()
    self:moveCursorTo(copyall(self.unit.pos))
    df.global.ui.main.mode = self.saved_mode
end

local function getTileType(cursor)
    local block = dfhack.maps.getTileBlock(cursor)
    if block then
        return block.tiletype[cursor.x%16][cursor.y%16]
    else
        return 0
    end
end

local function getTileWalkable(cursor)
    local block = dfhack.maps.getTileBlock(cursor)
    if block then
        return block.walkable[cursor.x%16][cursor.y%16]
    else
        return 0
    end
end

local function paintMapTile(dc, vp, cursor, pos, ...)
    if not same_xyz(cursor, pos) then
        local stile = vp:tileToScreen(pos)
        if stile.z == 0 then
            dc:seek(stile.x,stile.y):char(...)
        end
    end
end

local function get_path_point(gpath,i)
    return xyz2pos(gpath.x[i], gpath.y[i], gpath.z[i])
end

function UnitPathUI:renderPath(dc,vp,cursor)
    local gpath = self.unit.path.path
    local startp = self.unit.pos
    local endp = self.unit.path.dest
    local visible = gui.blink_visible(500)

    if visible then
        paintMapTile(dc, vp, cursor, endp, '+', COLOR_LIGHTGREEN)
    end

    local ok = nil
    local pcnt = #gpath.x
    if pcnt > 0 then
        ok = true

        for i = 0,pcnt-1 do
            local pt = get_path_point(gpath, i)
            if i == 0 and not same_xyz(startp,pt) then
                ok = false
            end
            if i == pcnt-1 and not same_xyz(endp,pt) then
                ok = false
            end
            --[[local tile = getTileType(pt)
            if not isTrackTile(tile) then
                ok = false
            end]]
            if visible then
                local char = '+'
                if i < pcnt-1 then
                    local npt = get_path_point(gpath, i+1)
                    if npt.z == pt.z+1 then
                        char = 30
                    elseif npt.z == pt.z-1 then
                        char = 31
                    elseif npt.x == pt.x+1 then
                        char = 26
                    elseif npt.x == pt.x-1 then
                        char = 27
                    elseif npt.y == pt.y+1 then
                        char = 25
                    elseif npt.y == pt.y-1 then
                        char = 24
                    end
                end
                local color = COLOR_LIGHTGREEN
                if getTileWalkable(pt) == 0 then color = COLOR_LIGHTRED end
                paintMapTile(dc, vp, cursor, pt, char, color)
            end
        end
    end

    if gui.blink_visible(120) then
        paintMapTile(dc, vp, cursor, startp, 240, COLOR_LIGHTGREEN, COLOR_GREEN)
    end

    return ok
end

function UnitPathUI:onRenderBody(dc)
    dc:clear():seek(1,1):pen(COLOR_WHITE):string("Unit Path")

    local prof = dfhack.units.getProfessionName(self.unit)
    local name = dfhack.units.getVisibleName(self.unit)

    dc:seek(2,3):pen(COLOR_BLUE):string(prof)
    if name and name.has_name then
        dc:seek(2,4):pen(COLOR_BLUE):string(dfhack.TranslateName(name))
    end

    local cursor = guidm.getCursorPos()
    local has_path = #self.unit.path.path.x>0

    local vp = self:getViewport()
    local mdc = gui.Painter.new(self.df_layout.map)

    if not has_path then
        if gui.blink_visible(120) then
            paintMapTile(mdc, vp, cursor, self.unit.pos, 15, COLOR_LIGHTRED, COLOR_RED)
        end

        dc:seek(1,6):pen(COLOR_RED):string('Not following path')
    else
        self:renderPath(mdc,vp,cursor)

        dc:seek(1,6):pen(COLOR_GREEN):string(df.unit_path_goal[self.unit.path.goal])
    end

    dc:newline():pen(COLOR_GREY)
    dc:newline(2):string('Speed: '..dfhack.units.computeMovementSpeed(self.unit))
    dc:newline(2):string('Slowdown: '..dfhack.units.computeSlowdownFactor(self.unit))

    dc:newline():newline(1)

    local has_station = self.unit.idle_area_type >= 0

    if has_station then
        if gui.blink_visible(250) then
            paintMapTile(mdc, vp, cursor, self.unit.idle_area, 21, COLOR_LIGHTCYAN)
        end

        dc:pen(COLOR_GREEN):string(df.unit_station_type[self.unit.idle_area_type])
        dc:newline():newline(2):pen(COLOR_GREY):string('Threshold: '..self.unit.idle_area_threshold)
    else
        dc:pen(COLOR_RED):string('No station'):newline():newline(2)
    end

    dc:newline():newline(1):string('At cursor:')
    dc:newline():newline(2)

    local tile = getTileType(cursor)
    dc:string(df.tiletype[tile],COLOR_CYAN)

    dc:newline():newline(1):pen(COLOR_WHITE)
    dc:key('CUSTOM_Z'):string(": Zoom unit, ")
    dc:key('CUSTOM_G'):string(": Zoom goal",COLOR_GREY,nil,has_path)
    dc:newline(1)
    dc:key('CUSTOM_N'):string(": Zoom station",COLOR_GREY,nil,has_station)
    dc:newline():newline(1)
    dc:key('LEAVESCREEN'):string(": Back")
end

function UnitPathUI:onInput(keys)
    if keys.CUSTOM_Z then
        self:moveCursorTo(copyall(self.unit.pos))
    elseif keys.CUSTOM_G then
        if #self.unit.path.path.x > 0 then
            self:moveCursorTo(copyall(self.unit.path.dest))
        end
    elseif keys.CUSTOM_N then
        if self.unit.idle_area_type > 0 then
            self:moveCursorTo(copyall(self.unit.idle_area))
        end
    elseif keys.LEAVESCREEN then
        self:dismiss()
    elseif self:propagateMoveKeys(keys) then
        return
    end
end

local unit = dfhack.gui.getSelectedUnit(true)

if not unit or not string.match(dfhack.gui.getCurFocus(), '^dwarfmode/ViewUnits/Some/') then
    qerror("This script requires the main dwarfmode view in 'v' mode with a unit selected")
end

UnitPathUI{ unit = unit }:show()
