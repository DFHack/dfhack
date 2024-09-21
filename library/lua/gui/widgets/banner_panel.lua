local Panel = require('gui.widgets.panel')

-----------------
-- BannerPanel --
-----------------

---@class widgets.BannerPanel: widgets.Panel
---@field super widgets.Panel
BannerPanel = defclass(BannerPanel, Panel)

---@param dc gui.Painter
function BannerPanel:onRenderBody(dc)
    dc:pen(COLOR_RED)
    for y=0,self.frame_rect.height-1 do
        dc:seek(0, y):char('[')
        dc:seek(self.frame_rect.width-1):char(']')
    end
end

return BannerPanel
