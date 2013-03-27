-- Interface front-end for liquids plugin.

local utils = require 'utils'
local gui = require 'gui'
local guidm = require 'gui.dwarfmode'
local dlg = require 'gui.dialogs'

local liquids = require('plugins.liquids')

local sel_rect = df.global.selection_rect

local brushes = {
    { tag = 'range', caption = 'Rectangle', range = true },
    { tag = 'block', caption = '16x16 block' },
    { tag = 'column', caption = 'Column' },
    { tag = 'flood', caption = 'Flood' },
}

local paints = {
    { tag = 'water', caption = 'Water', liquid = true, flow = true, key = 'D_LOOK_ARENA_WATER' },
    { tag = 'magma', caption = 'Magma', liquid = true, flow = true, key = 'D_LOOK_ARENA_MAGMA' },
    { tag = 'obsidian', caption = 'Obsidian Wall' },
    { tag = 'obsidian_floor', caption = 'Obsidian Floor' },
    { tag = 'riversource', caption = 'River Source' },
    { tag = 'flowbits', caption = 'Flow Updates', flow = true },
    { tag = 'wclean', caption = 'Clean Salt/Stagnant' },
}

local flowbits = {
    { tag = '+', caption = 'Enable Updates' },
    { tag = '-', caption = 'Disable Updates' },
    { tag = '.', caption = 'Keep Updates' },
}

local setmode = {
    { tag = '.', caption = 'Set Exactly' },
    { tag = '+', caption = 'Only Increase' },
    { tag = '-', caption = 'Only Decrease' },
}

local permaflows = {
    { tag = '.', caption = "Keep Permaflow" },
    { tag = '-', caption = 'Remove Permaflow' },
    { tag = 'N', caption = 'Set Permaflow N' },
    { tag = 'S', caption = 'Set Permaflow S' },
    { tag = 'E', caption = 'Set Permaflow E' },
    { tag = 'W', caption = 'Set Permaflow W' },
    { tag = 'NE', caption = 'Set Permaflow NE' },
    { tag = 'NW', caption = 'Set Permaflow NW' },
    { tag = 'SE', caption = 'Set Permaflow SE' },
    { tag = 'SW', caption = 'Set Permaflow SW' },
}

Toggle = defclass(Toggle)

Toggle.ATTRS{ items = {}, selected = 1 }

function Toggle:get()
    return self.items[self.selected]
end

function Toggle:render(dc)
    local item = self:get()
    if item then
        dc:string(item.caption)
        if item.key then
            dc:string(" ("):key(item.key):string(")")
        end
    else
        dc:string('NONE', COLOR_RED)
    end
end

function Toggle:step(delta)
    if #self.items > 1 then
        delta = delta or 1
        self.selected = 1 + (self.selected + delta - 1) % #self.items
    end
end

LiquidsUI = defclass(LiquidsUI, guidm.MenuOverlay)

LiquidsUI.focus_path = 'liquids'

function LiquidsUI:init()
    self:assign{
        brush = Toggle{ items = brushes },
        paint = Toggle{ items = paints },
        flow = Toggle{ items = flowbits },
        set = Toggle{ items = setmode },
        permaflow = Toggle{ items = permaflows },
        amount = 7,
    }
end

function LiquidsUI:onDestroy()
    guidm.clearSelection()
end

function render_liquid(dc, block, x, y)
    local dsgn = block.designation[x%16][y%16]

    if dsgn.flow_size > 0 then
        if dsgn.liquid_type == df.tile_liquid.Magma then
            dc:pen(COLOR_RED):string("Magma")
        else
            dc:pen(COLOR_BLUE)
            if dsgn.water_stagnant then dc:string("Stagnant ") end
            if dsgn.water_salt then dc:string("Salty ") end
            dc:string("Water")
        end
        dc:string(" ["..dsgn.flow_size.."/7]")
    else
        dc:string('No Liquid')
    end
end

local permaflow_abbr = {
    north = 'N', south = 'S', east = 'E', west = 'W',
    northeast = 'NE', northwest = 'NW', southeast = 'SE', southwest = 'SW'
}

function render_flow_state(dc, block, x, y)
    local flow = block.liquid_flow[x%16][y%16]

    if block.flags.update_liquid then
        dc:string("Updating", COLOR_GREEN)
    else
        dc:string("Static")
    end
    dc:string(", ")
    if flow.perm_flow_dir ~= 0 then
        local tag = df.tile_liquid_flow_dir[flow.perm_flow_dir]
        dc:string("Permaflow "..(permaflow_abbr[tag] or tag), COLOR_CYAN)
    elseif flow.temp_flow_timer > 0 then
        dc:string("Flowing "..flow.temp_flow_timer, COLOR_GREEN)
    else
        dc:string("No Flow")
    end
end

function LiquidsUI:onRenderBody(dc)
    dc:clear():seek(1,1):string("Paint Liquids Cheat", COLOR_WHITE)

    local cursor = guidm.getCursorPos()
    local block = dfhack.maps.getTileBlock(cursor)

    if block then
        local x, y = pos2xyz(cursor)
        local tile = block.tiletype[x%16][y%16]

        dc:seek(2,3):string(df.tiletype.attrs[tile].caption, COLOR_CYAN)
        dc:newline(2):pen(COLOR_DARKGREY)
        render_liquid(dc, block, x, y)
        dc:newline(2):pen(COLOR_DARKGREY)
        render_flow_state(dc, block, x, y)
    else
        dc:seek(2,3):string("No map data", COLOR_RED):advance(0,2)
    end

    dc:newline():pen(COLOR_GREY)

    dc:newline(1):key('CUSTOM_B'):string(": ")
    self.brush:render(dc)
    dc:newline(1):key('CUSTOM_P'):string(": ")
    self.paint:render(dc)

    local paint = self.paint:get()

    dc:newline()
    if paint.liquid then
        dc:newline(1):string("Amount: "..self.amount)
        dc:advance(1):string("("):key('SECONDSCROLL_UP'):key('SECONDSCROLL_DOWN'):string(")")
        dc:newline(3):key('CUSTOM_S'):string(": ")
        self.set:render(dc)
    else
        dc:advance(0,2)
    end

    dc:newline()
    if paint.flow then
        dc:newline(1):key('CUSTOM_F'):string(": ")
        self.flow:render(dc)
        dc:newline(1):key('CUSTOM_R'):string(": ")
        self.permaflow:render(dc)
    else
        dc:advance(0,2)
    end

    dc:newline():newline(1):pen(COLOR_WHITE)
    dc:key('LEAVESCREEN'):string(": Back, ")
    dc:key('SELECT'):string(": Paint")
end

function ensure_blocks(cursor, size, cb)
    size = size or xyz2pos(1,1,1)
    local cx,cy,cz = pos2xyz(cursor)
    local all = true
    for x=1,size.x or 1,16 do
        for y=1,size.y or 1,16 do
            for z=1,size.z do
                if not dfhack.maps.getTileBlock(cx+x-1, cy+y-1, cz+z-1) then
                    all = false
                end
            end
        end
    end
    if all then
        cb()
        return
    end
    dlg.showYesNoPrompt(
        'Instantiate Blocks',
        'Not all map blocks are allocated - instantiate?\n\nWarning: new untested feature.',
        COLOR_YELLOW,
        function()
            for x=1,size.x or 1,16 do
                for y=1,size.y or 1,16 do
                    for z=1,size.z do
                        dfhack.maps.ensureTileBlock(cx+x-1, cy+y-1, cz+z-1)
                    end
                end
            end
            cb()
        end,
        function()
            cb()
        end
    )
end

function LiquidsUI:onInput(keys)
    local paint = self.paint:get()
    local liquid = paint.liquid
    if keys.CUSTOM_B then
        self.brush:step()
    elseif keys.CUSTOM_P then
        self.paint:step()
    elseif liquid and keys.SECONDSCROLL_UP then
        self.amount = math.max(0, self.amount-1)
    elseif liquid and keys.SECONDSCROLL_DOWN then
        self.amount = math.min(7, self.amount+1)
    elseif liquid and keys.CUSTOM_S then
        self.set:step()
    elseif paint.flow and keys.CUSTOM_F then
        self.flow:step()
    elseif paint.flow and keys.CUSTOM_R then
        self.permaflow:step()
    elseif keys.LEAVESCREEN then
        if guidm.getSelection() then
            guidm.clearSelection()
            return
        end
        self:dismiss()
        self:sendInputToParent('CURSOR_DOWN_Z')
        self:sendInputToParent('CURSOR_UP_Z')
    elseif keys.SELECT then
        local cursor = guidm.getCursorPos()
        local sp = guidm.getSelection()
        local size = nil
        if self.brush:get().range then
            if not sp then
                guidm.setSelectionStart(cursor)
                return
            else
                guidm.clearSelection()
                cursor, size = guidm.getSelectionRange(cursor, sp)
            end
        else
            guidm.clearSelection()
        end
        local cb = curry(
            liquids.paint,
            cursor,
            self.brush:get().tag, self.paint:get().tag,
            self.amount, size,
            self.set:get().tag, self.flow:get().tag,
            self.permaflow:get().tag
        )
        ensure_blocks(cursor, size, cb)
    elseif self:propagateMoveKeys(keys) then
        return
    elseif keys.D_LOOK_ARENA_WATER then
        self.paint.selected = 1
    elseif keys.D_LOOK_ARENA_MAGMA then
        self.paint.selected = 2
    end
end

if not string.match(dfhack.gui.getCurFocus(), '^dwarfmode/LookAround') then
    qerror("This script requires the main dwarfmode view in 'k' mode")
end

local list = LiquidsUI()
list:show()
