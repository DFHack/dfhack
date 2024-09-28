local gui = require('gui')
local Widget = require('gui.widgets.widget')

-------------
-- Divider --
-------------

---@class widgets.Divider.attrs: widgets.Widget.attrs
---@field frame_style gui.Frame|fun(): gui.Frame
---@field interior boolean
---@field frame_style_t? false|gui.Frame|fun(): gui.Frame
---@field frame_style_b? false|gui.Frame|fun(): gui.Frame
---@field frame_style_l? false|gui.Frame|fun(): gui.Frame
---@field frame_style_r? false|gui.Frame|fun(): gui.Frame
---@field interior_t? boolean
---@field interior_b? boolean
---@field interior_l? boolean
---@field interior_r? boolean

---@class widgets.Divider.attrs.partial: widgets.Divider.attrs

---@class widgets.Divider:  widgets.Widget, widgets.Divider.attrs
---@field super widgets.Widget
---@field ATTRS widgets.Divider.attrs|fun(attributes: widgets.Divider.attrs.partial)
---@overload fun(init_table: widgets.Divider.attrs.partial): self
Divider = defclass(Divider, Widget)

Divider.ATTRS{
    frame_style=gui.FRAME_THIN,
    interior=false,
    frame_style_t=DEFAULT_NIL,
    interior_t=DEFAULT_NIL,
    frame_style_b=DEFAULT_NIL,
    interior_b=DEFAULT_NIL,
    frame_style_l=DEFAULT_NIL,
    interior_l=DEFAULT_NIL,
    frame_style_r=DEFAULT_NIL,
    interior_r=DEFAULT_NIL,
}

local function divider_get_val(self, base_name, edge_name)
    local val = self[base_name..'_'..edge_name]
    if val ~= nil then return val end
    return self[base_name]
end

local function divider_get_junction_pen(self, edge_name)
    local interior = divider_get_val(self, 'interior', edge_name)
    local pen_name = ('%sT%s_frame_pen'):format(edge_name, interior and 'i' or 'e')
    local frame_style = divider_get_val(self, 'frame_style', edge_name)
    if type(frame_style) == 'function' then
        frame_style = frame_style()
    end
    return frame_style[pen_name]
end

---@param dc gui.Painter
function Divider:onRenderBody(dc)
    local rect, style = self.frame_rect, self.frame_style
    if type(style) == 'function' then
        style = style()
    end

    if rect.height == 1 and rect.width == 1 then
        dc:seek(0, 0):char(nil, style.x_frame_pen)
    elseif rect.width == 1 then
        local fill_start, fill_end = 0, rect.height-1
        if self.frame_style_t ~= false then
            fill_start = 1
            dc:seek(0, 0):char(nil, divider_get_junction_pen(self, 't'))
        end
        if self.frame_style_b ~= false then
            fill_end = rect.height-2
            dc:seek(0, rect.height-1):char(nil, divider_get_junction_pen(self, 'b'))
        end
        dc:fill(0, fill_start, 0, fill_end, style.v_frame_pen)
    else
        local fill_start, fill_end = 0, rect.width-1
        if self.frame_style_l ~= false then
            fill_start = 1
            dc:seek(0, 0):char(nil, divider_get_junction_pen(self, 'l'))
        end
        if self.frame_style_r ~= false then
            fill_end = rect.width-2
            dc:seek(rect.width-1, 0):char(nil, divider_get_junction_pen(self, 'r'))
        end
        dc:fill(fill_start, 0, fill_end, 0, style.h_frame_pen)
    end
end

return Divider
