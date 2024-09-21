local gui = require('gui')
local utils = require('utils')
local Panel = require('gui.widgets.panel')

local getval = utils.getval

-------------------
-- ResizingPanel --
-------------------

---@class widgets.ResizingPanel.attrs: widgets.Panel.attrs
---@field auto_height boolean
---@field auto_width boolean

---@class widgets.ResizingPanel.attrs.partial: widgets.ResizingPanel.attrs

---@class widgets.ResizingPanel: widgets.Panel, widgets.ResizingPanel.attrs
---@field super widgets.Panel
---@field ATTRS widgets.ResizingPanel.attrs|fun(attributes: widgets.ResizingPanel.attrs.partial)
---@overload fun(init_table: widgets.ResizingPanel.attrs.partial): self
ResizingPanel = defclass(ResizingPanel, Panel)

ResizingPanel.ATTRS{
    auto_height = true,
    auto_width = false,
}

-- adjust our frame dimensions according to positions and sizes of our subviews
function ResizingPanel:postUpdateLayout(frame_body)
    local w, h = 0, 0
    for _,s in ipairs(self.subviews) do
        if getval(s.visible) then
            w = math.max(w, (s.frame and s.frame.l or 0) +
                            (s.frame and s.frame.w or frame_body.width))
            h = math.max(h, (s.frame and s.frame.t or 0) +
                            (s.frame and s.frame.h or frame_body.height))
        end
    end
    local l,t,r,b = gui.parse_inset(self.frame_inset)
    w = w + l + r
    h = h + t + b
    if self.frame_style then
        w = w + 2
        h = h + 2
    end
    if not self.frame then self.frame = {} end
    local oldw, oldh = self.frame.w, self.frame.h
    if not self.auto_height then h = oldh end
    if not self.auto_width then w = oldw end
    self.frame.w, self.frame.h = w, h
    if not self._updateLayoutGuard and (oldw ~= w or oldh ~= h) then
        self._updateLayoutGuard = true -- protect against infinite loops
        self:updateLayout() -- our frame has changed, we need to fully refresh
    end
    self._updateLayoutGuard = nil
end

return ResizingPanel
