local gui = require('gui')
local ResizingPanel = require('gui.widgets.containers.resizing_panel')
local Panel = require('gui.widgets.containers.panel')

--------------------------------
-- DimensionsTooltip
--------------------------------

---@class widgets.DimensionsTooltip.attrs: widgets.ResizingPanel.attrs
---@field display_offset? df.coord2d
---@field get_anchor_pos_fn fun(): df.coord?

---@class widgets.DimensionsTooltip: widgets.ResizingPanel
---@field super widgets.ResizingPanel
---@overload fun(init_table: widgets.DimensionsTooltip.attrs): self
DimensionsTooltip = defclass(DimensionsTooltip, ResizingPanel)

DimensionsTooltip.ATTRS{
    frame_style=gui.FRAME_THIN,
    frame_background=gui.CLEAR_PEN,
    no_force_pause_badge=true,
    auto_width=true,
    display_offset={x=3, y=3},
    get_anchor_pos_fn=DEFAULT_NIL,
}

local function get_cur_area_dims(anchor_pos)
    local mouse_pos = dfhack.gui.getMousePos(true)
    if not mouse_pos or not anchor_pos then return 1, 1, 1 end

    -- clamp to map edges (you can start a selection from out of bounds)
    mouse_pos = xyz2pos(
        math.max(0, math.min(df.global.world.map.x_count-1, mouse_pos.x)),
        math.max(0, math.min(df.global.world.map.y_count-1, mouse_pos.y)),
        math.max(0, math.min(df.global.world.map.z_count-1, mouse_pos.z)))
    anchor_pos = xyz2pos(
        math.max(0, math.min(df.global.world.map.x_count-1, anchor_pos.x)),
        math.max(0, math.min(df.global.world.map.y_count-1, anchor_pos.y)),
        math.max(0, math.min(df.global.world.map.z_count-1, anchor_pos.z)))

    return math.abs(mouse_pos.x - anchor_pos.x) + 1,
        math.abs(mouse_pos.y - anchor_pos.y) + 1,
        math.abs(mouse_pos.z - anchor_pos.z) + 1
end

function DimensionsTooltip:init()
    ensure_key(self, 'frame').w = 17
    self.frame.h = 4

    self.label = Label{
        frame={t=0},
        auto_width=true,
        text={
            {
                text=function()
                    local anchor_pos = self.get_anchor_pos_fn()
                    return ('%dx%dx%d'):format(get_cur_area_dims(anchor_pos))
                end
            }
        },
    }

    self:addviews{
        Panel{
            -- set minimum size for tooltip frame so the DFHack frame badge fits
            frame={t=0, l=0, w=7, h=2},
        },
        self.label,
    }
end

function DimensionsTooltip:render(dc)
    local x, y = dfhack.screen.getMousePos()
    if not x or not self.get_anchor_pos_fn() then return end
    local sw, sh = dfhack.screen.getWindowSize()
    local frame_width = math.max(9, self.label:getTextWidth() + 2)
    self.frame.l = math.min(x + self.display_offset.x, sw - frame_width)
    self.frame.t = math.min(y + self.display_offset.y, sh - self.frame.h)
    self:updateLayout()
    DimensionsTooltip.super.render(self, dc)
end

return DimensionsTooltip
