-- overlay that displays map block borders

--[====[

devel/block-borders
===================

An overlay that draws borders of map blocks. See :doc:`/docs/api/Maps` for
details on map blocks.

]====]

local gui = require "gui"
local guidm = require "gui.dwarfmode"

local ui = df.global.plotinfo

local DRAW_CHARS = {
    ns = string.char(179),
    ew = string.char(196),
    ne = string.char(192),
    nw = string.char(217),
    se = string.char(218),
    sw = string.char(191),
}
-- persist across script runs
color = color or COLOR_LIGHTCYAN

BlockBordersOverlay = defclass(BlockBordersOverlay, guidm.MenuOverlay)
BlockBordersOverlay.ATTRS{
    block_size = 16,
    draw_borders = true,
}

function BlockBordersOverlay:onInput(keys)
    if keys.LEAVESCREEN then
        self:dismiss()
    elseif keys.D_PAUSE then
        self.draw_borders = not self.draw_borders
    elseif keys.CUSTOM_B then
        self.block_size = self.block_size == 16 and 48 or 16
    elseif keys.CUSTOM_C then
        color = color + 1
        if color > 15 then
            color = 1
        end
    elseif keys.CUSTOM_SHIFT_C then
        color = color - 1
        if color < 1 then
            color = 15
        end
    elseif keys.D_LOOK then
        self:sendInputToParent(ui.main.mode == df.ui_sidebar_mode.LookAround and 'LEAVESCREEN' or 'D_LOOK')
    else
        self:propagateMoveKeys(keys)
    end
end

function BlockBordersOverlay:onRenderBody(dc)
    dc = dc:viewport(1, 1, dc.width - 2, dc.height - 2)
    dc:key_string('D_PAUSE', 'Toggle borders')
      :newline()
    dc:key_string('CUSTOM_B', self.block_size == 16 and '1 block (16 tiles)' or '3 blocks (48 tiles)')
      :newline()
    dc:key('CUSTOM_C')
      :string(', ')
      :key_string('CUSTOM_SHIFT_C', 'Color: ')
      :string('Example', color)
      :newline()
    dc:key_string('D_LOOK', 'Toggle cursor')
      :newline()

    self:renderOverlay()
end

function BlockBordersOverlay:renderOverlay()
    if not self.draw_borders then return end

    local block_end = self.block_size - 1
    self:renderMapOverlay(function(pos, is_cursor)
        if is_cursor then return end
        local block_x = pos.x % self.block_size
        local block_y = pos.y % self.block_size
        local key
        if block_x == 0 and block_y == 0 then
            key = 'se'
        elseif block_x == 0 and block_y == block_end then
            key = 'ne'
        elseif block_x == block_end and block_y == 0 then
            key = 'sw'
        elseif block_x == block_end and block_y == block_end then
            key = 'nw'
        elseif block_x == 0 or block_x == block_end then
            key = 'ns'
        elseif block_y == 0 or block_y == block_end then
            key = 'ew'
        end
        return DRAW_CHARS[key], color or COLOR_LIGHTCYAN
    end)
end

if not dfhack.isMapLoaded() then
    qerror('This script requires a fortress map to be loaded')
end

-- we can work both with a cursor and without one. start in a mode that mirrors
-- the current game state
local is_cursor = not not guidm.getCursorPos()
local sidebar_mode = df.ui_sidebar_mode[is_cursor and 'LookAround' or 'Default']
BlockBordersOverlay{sidebar_mode=sidebar_mode}:show()
