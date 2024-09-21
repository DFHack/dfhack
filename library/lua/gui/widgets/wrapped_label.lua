local utils = require('utils')
local Label = require('gui.widgets.label')

local getval = utils.getval

------------------
-- WrappedLabel --
------------------

---@class widgets.WrappedLabel.attrs: widgets.Label.attrs
---@field text_to_wrap? string|string[]|fun(): string|string[]
---@field indent integer

---@class widgets.WrappedLabel.attrs.partial: widgets.WrappedLabel.attrs

---@class widgets.WrappedLabel: widgets.Label, widgets.WrappedLabel.attrs
---@field super widgets.Label
---@field ATTRS widgets.WrappedLabel.attrs|fun(attributes: widgets.WrappedLabel.attrs.partial)
---@overload fun(init_table: widgets.WrappedLabel.attrs.partial): self
WrappedLabel = defclass(WrappedLabel, Label)

WrappedLabel.ATTRS{
    text_to_wrap=DEFAULT_NIL,
    indent=0,
}

function WrappedLabel:getWrappedText(width)
    -- 0 width can happen if the parent has 0 width
    if not self.text_to_wrap or width <= 0 then return nil end
    local text_to_wrap = getval(self.text_to_wrap)
    if type(text_to_wrap) == 'table' then
        text_to_wrap = table.concat(text_to_wrap, NEWLINE)
    end
    return text_to_wrap:wrap(width - self.indent, {return_as_table=true})
end

function WrappedLabel:preUpdateLayout()
    self.saved_start_line_num = self.start_line_num
end

-- we can't set the text in init() since we may not yet have a frame that we
-- can get wrapping bounds from.
function WrappedLabel:postComputeFrame()
    local wrapped_text = self:getWrappedText(self.frame_body.width-3)
    if not wrapped_text then return end
    local text = {}
    for _,line in ipairs(wrapped_text) do
        table.insert(text, {gap=self.indent, text=line})
        -- a trailing newline will get ignored so we don't have to manually trim
        table.insert(text, NEWLINE)
    end
    self:setText(text)
    self:scroll(self.saved_start_line_num - 1)
end

return WrappedLabel
