local gui = require('gui')
local utils = require('utils')
local Panel = require('gui.widgets.panel')

---@param view gui.View
---@param vis boolean
local function show_view(view,vis)
    if view then
        view.visible = vis
    end
end

-----------
-- Pages --
-----------

---@class widgets.Pages.initTable: widgets.Panel.attrs
---@field selected? integer|string

---@class widgets.Pages: widgets.Panel
---@field super widgets.Panel
---@overload fun(attributes: widgets.Pages.initTable): self
Pages = defclass(Pages, Panel)

---@param self widgets.Pages
---@param args widgets.Pages.initTable
function Pages:init(args)
    for _,v in ipairs(self.subviews) do
        v.visible = false
    end
    self:setSelected(args.selected or 1)
end

---@param idx integer|string
function Pages:setSelected(idx)
    if type(idx) ~= 'number' then
        local key = idx
        if type(idx) == 'string' then
            key = self.subviews[key]
        end
        idx = utils.linear_index(self.subviews, key)
        if not idx then
            error('Unknown page: '..tostring(key))
        end
    end

    show_view(self.subviews[self.selected], false)
    self.selected = math.min(math.max(1, idx), #self.subviews)
    show_view(self.subviews[self.selected], true)
end

---@return integer index
---@return gui.View child
function Pages:getSelected()
    return self.selected, self.subviews[self.selected]
end

---@return gui.View child
function Pages:getSelectedPage()
    return self.subviews[self.selected]
end

return Pages
