-- Read the tiles from the screen and display info about them.

local utils = require 'utils'
local gui = require 'gui'

InspectScreen = defclass(InspectScreen, gui.Screen)

function InspectScreen:init(args)
    local w,h = dfhack.screen.getWindowSize()
    self.cursor_x = math.floor(w/2)
    self.cursor_y = math.floor(h/2)
end

function InspectScreen:computeFrame(parent_rect)
    local sw, sh = parent_rect.width, parent_rect.height
    self.cursor_x = math.max(0, math.min(self.cursor_x, sw-1))
    self.cursor_y = math.max(0, math.min(self.cursor_y, sh-1))

    local frame = { w = 14, r = 1, h = 10, t = 1 }
    if self.cursor_x > sw/2 then
        frame = { w = 14, l = 1, h = 10, t = 1 }
    end

    return gui.compute_frame_body(sw, sh, frame, 1, 0, false)
end

function InspectScreen:onRenderFrame(dc, rect)
    self:renderParent()
    self.cursor_pen = dfhack.screen.readTile(self.cursor_x, self.cursor_y)
    if gui.blink_visible(100) then
        dfhack.screen.paintTile({ch='X',fg=COLOR_LIGHTGREEN}, self.cursor_x, self.cursor_y)
    end
    dc:fill(rect, {ch=' ',fg=COLOR_WHITE,bg=COLOR_CYAN})
end

local FG_PEN = {fg=COLOR_WHITE,bg=COLOR_BLACK,tile_color=true}
local BG_PEN = {fg=COLOR_BLACK,bg=COLOR_WHITE,tile_color=true}
local TXT_PEN = {fg=COLOR_WHITE}

function InspectScreen:onRenderBody(dc)
    dc:pen(COLOR_WHITE, COLOR_CYAN)
    if self.cursor_pen then
        local info = self.cursor_pen
        dc:string('CH: '):char(info.ch, FG_PEN):char(info.ch, BG_PEN):string(' '):string(''..info.ch,TXT_PEN):newline()
        local fgcolor = info.fg
        local fgstr = info.fg
        if info.bold then
            fgcolor = (fgcolor+8)%16
            fgstr = fgstr..'+8'
        end
        dc:string('FG: '):string('NN',{fg=fgcolor}):string(' '):string(''..fgstr,TXT_PEN)
        dc:seek(dc.width-1):char(info.ch,{fg=info.fg,bold=info.bold}):newline()
        dc:string('BG: '):string('NN',{fg=info.bg}):string(' '):string(''..info.bg,TXT_PEN)
        dc:seek(dc.width-1):char(info.ch,{fg=COLOR_BLACK,bg=info.bg}):newline()
        local bstring = 'false'
        if info.bold then bstring = 'true' end
        dc:string('Bold: '..bstring):newline():newline()

        if info.tile and gui.USE_GRAPHICS then
            dc:string('TL: '):tile(' ', info.tile, FG_PEN):tile(' ', info.tile, BG_PEN):string(' '..info.tile):newline()
            if info.tile_color then
                dc:string('Color: true')
            elseif info.tile_fg then
                dc:string('FG: '):string('NN',{fg=info.tile_fg}):string(' '):string(''..info.tile_fg,TXT_PEN):newline()
                dc:string('BG: '):string('NN',{fg=info.tile_bg}):string(' '):string(''..info.tile_bg,TXT_PEN):newline()
            end
        end
    else
        dc:string('Invalid', COLOR_LIGHTRED)
    end
end

local MOVEMENT_KEYS = {
    CURSOR_UP = { 0, -1, 0 }, CURSOR_DOWN = { 0, 1, 0 },
    CURSOR_LEFT = { -1, 0, 0 }, CURSOR_RIGHT = { 1, 0, 0 },
    CURSOR_UPLEFT = { -1, -1, 0 }, CURSOR_UPRIGHT = { 1, -1, 0 },
    CURSOR_DOWNLEFT = { -1, 1, 0 }, CURSOR_DOWNRIGHT = { 1, 1, 0 },
    CURSOR_UP_FAST = { 0, -1, 0, true }, CURSOR_DOWN_FAST = { 0, 1, 0, true },
    CURSOR_LEFT_FAST = { -1, 0, 0, true }, CURSOR_RIGHT_FAST = { 1, 0, 0, true },
    CURSOR_UPLEFT_FAST = { -1, -1, 0, true }, CURSOR_UPRIGHT_FAST = { 1, -1, 0, true },
    CURSOR_DOWNLEFT_FAST = { -1, 1, 0, true }, CURSOR_DOWNRIGHT_FAST = { 1, 1, 0, true },
}

function InspectScreen:onInput(keys)
    if keys.LEAVESCREEN then
        self:dismiss()
    else
        for k,v in pairs(MOVEMENT_KEYS) do
            if keys[k] then
                local delta = 1
                if v[4] then
                    delta = 10
                end
                self.cursor_x = self.cursor_x + delta*v[1]
                self.cursor_y = self.cursor_y + delta*v[2]
                self:updateLayout()
                return
            end
        end
    end
end

InspectScreen{}:show()
