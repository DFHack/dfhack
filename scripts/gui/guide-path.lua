-- Show and manipulate the path used by Guide Cart orders.

local utils = require 'utils'
local gui = require 'gui'
local guidm = require 'gui.dwarfmode'
local dlg = require 'gui.dialogs'

local tile_attrs = df.tiletype.attrs

GuidePathUI = defclass(GuidePathUI, guidm.MenuOverlay)

GuidePathUI.focus_path = 'guide-path'

GuidePathUI.ATTRS {
    route = DEFAULT_NIL,
    stop = DEFAULT_NIL,
    order = DEFAULT_NIL,
}

function GuidePathUI:init()
    self.saved_mode = df.global.ui.main.mode

    for i=0,#self.route.stops-1 do
        if self.route.stops[i] == self.stop then
            self.stop_idx = i
            break
        end
    end

    if not self.stop_idx then
        error('Could not find stop index')
    end

    self.next_stop = self.route.stops[(self.stop_idx+1)%#self.route.stops]
end

function GuidePathUI:onShow()
    -- with cursor, but without those ugly lines from native hauling mode
    df.global.ui.main.mode = df.ui_sidebar_mode.Stockpiles
end

function GuidePathUI:onDestroy()
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

local function isTrackTile(tile)
    return tile_attrs[tile].special == df.tiletype_special.TRACK
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

function GuidePathUI:renderPath(cursor)
    local gpath = self.order.guide_path
    local startp = self.stop.pos
    local endp = self.next_stop.pos
    local vp = self:getViewport()
    local dc = gui.Painter.new(self.df_layout.map)
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
            local tile = getTileType(pt)
            if not isTrackTile(tile) then
                ok = false
            end
            if visible then
                local char = '+'
                if i < pcnt-1 then
                    local npt = get_path_point(gpath, i+1)
                    if npt.x == pt.x+1 then
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
                if not ok then color = COLOR_LIGHTRED end
                paintMapTile(dc, vp, cursor, pt, char, color)
            end
        end
    end

    if gui.blink_visible(120) then
        paintMapTile(dc, vp, cursor, startp, 240, COLOR_LIGHTGREEN, COLOR_GREEN)
    end

    return ok
end

function GuidePathUI:onRenderBody(dc)
    dc:clear():seek(1,1):pen(COLOR_WHITE):string("Guide Path")

    dc:seek(2,3)

    local cursor = guidm.getCursorPos()
    local path_ok = self:renderPath(cursor)

    if path_ok == nil then
        dc:string('No saved path', COLOR_DARKGREY)
    elseif path_ok then
        dc:string('Valid path', COLOR_GREEN)
    else
        dc:string('Invalid path', COLOR_RED)
    end

    dc:newline():newline(1)
    dc:key('CUSTOM_Z'):string(": Reset path",COLOR_GREY,nil,path_ok~=nil)
    --dc:key('CUSTOM_P'):string(": Find path",COLOR_GREY,nil,false)
    dc:newline(1)
    dc:key('CUSTOM_C'):string(": Zoom cur, ")
    dc:key('CUSTOM_N'):string(": Zoom next")

    dc:newline():newline(1):string('At cursor:')
    dc:newline():newline(2)

    local tile = getTileType(cursor)
    if isTrackTile(tile) then
        dc:string('Track '..tile_attrs[tile].direction, COLOR_GREEN)
    else
        dc:string('No track', COLOR_DARKGREY)
    end

    dc:newline():newline(1)
    dc:key('LEAVESCREEN'):string(": Back")
end

function GuidePathUI:onInput(keys)
    if keys.CUSTOM_C then
        self:moveCursorTo(copyall(self.stop.pos))
    elseif keys.CUSTOM_N then
        self:moveCursorTo(copyall(self.next_stop.pos))
    elseif keys.CUSTOM_Z then
        local gpath = self.order.guide_path
        gpath.x:resize(0)
        gpath.y:resize(0)
        gpath.z:resize(0)
    elseif keys.LEAVESCREEN then
        self:dismiss()
    elseif self:propagateMoveKeys(keys) then
        return
    end
end

if not string.match(dfhack.gui.getCurFocus(), '^dwarfmode/Hauling/DefineStop/Cond/Guide') then
    qerror("This script requires the main dwarfmode view in 'h' mode over a Guide order")
end

local hauling = df.global.ui.hauling
local route = hauling.view_routes[hauling.cursor_top]
local stop = hauling.view_stops[hauling.cursor_top]
local order = hauling.stop_conditions[hauling.cursor_stop]

local list = GuidePathUI{ route = route, stop = stop, order = order }
list:show()
