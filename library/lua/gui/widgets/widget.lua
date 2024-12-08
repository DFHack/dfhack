local gui = require('gui')

------------
-- Widget --
------------

---@class widgets.Widget.frame
---@field l? integer Gap between the left edge of the frame and the parent.
---@field t? integer Gap between the top edge of the frame and the parent.
---@field r? integer Gap between the right edge of the frame and the parent.
---@field b? integer Gap between the bottom edge of the frame and the parent.
---@field w? integer Desired width
---@field h? integer Desired height

---@class widgets.Widget.inset
---@field l? integer Left margin
---@field t? integer Top margin
---@field r? integer Right margin
---@field b? integer Bottom margin
---@field x? integer Left/right margin (if `l` and/or `r` are ommited)
---@field y? integer Top/bottom margin (if `t` and/or `b` are ommited)

---@class widgets.Widget.attrs: gui.View.attrs
---@field frame? widgets.Widget.frame
---@field frame_inset? widgets.Widget.inset|integer
---@field frame_background? dfhack.pen

---@class widgets.Widget.attrs.partial: widgets.Widget.attrs

---@class widgets.Widget: gui.View
---@field super gui.View
---@field ATTRS widgets.Widget.attrs|fun(attributes: widgets.Widget.attrs.partial)
---@overload fun(init_table: widgets.Widget.attrs.partial): self
Widget = defclass(Widget, gui.View)

Widget.ATTRS {
    frame = DEFAULT_NIL,
    frame_inset = DEFAULT_NIL,
    frame_background = DEFAULT_NIL,
}

---@param parent_rect { width: integer, height: integer }
---@return any
function Widget:computeFrame(parent_rect)
    local sw, sh = parent_rect.width, parent_rect.height
    return gui.compute_frame_body(sw, sh, self.frame, self.frame_inset)
end

---@param dc gui.Painter
---@param rect { x1: integer, y1: integer, x2: integer, y2: integer }
function Widget:onRenderFrame(dc, rect)
    if self.frame_background then
        dc:fill(rect, self.frame_background)
    end
end

return Widget
