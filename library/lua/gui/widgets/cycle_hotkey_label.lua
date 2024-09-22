local utils = require('utils')
local Label = require('gui.widgets.label')

local getval = utils.getval

local function is_disabled(token)
    return (token.disabled ~= nil and getval(token.disabled)) or
           (token.enabled ~= nil and not getval(token.enabled))
end

----------------------
-- CycleHotkeyLabel --
----------------------

---@class widgets.CycleHotkeyLabel.attrs: widgets.Label.attrs
---@field key? string
---@field key_back? string
---@field key_sep string
---@field label? string|fun(): string
---@field label_width? integer
---@field label_below boolean
---@field option_gap integer
---@field options? { label: string, value: string, pen: dfhack.pen|nil }[]
---@field initial_option integer|string
---@field on_change? fun(new_option_value: integer|string, old_option_value: integer|string)

---@class widgets.CycleHotkeyLabel.attrs.partial: widgets.CycleHotkeyLabel.attrs

---@class widgets.CycleHotkeyLabel: widgets.Label, widgets.CycleHotkeyLabel.attrs
---@field super widgets.Label
---@field ATTRS widgets.CycleHotkeyLabel.attrs|fun(attributes: widgets.CycleHotkeyLabel.attrs.partial)
---@overload fun(init_table: widgets.CycleHotkeyLabel.attrs.partial)
CycleHotkeyLabel = defclass(CycleHotkeyLabel, Label)

CycleHotkeyLabel.ATTRS{
    key=DEFAULT_NIL,
    key_back=DEFAULT_NIL,
    key_sep=': ',
    option_gap=1,
    label=DEFAULT_NIL,
    label_width=DEFAULT_NIL,
    label_below=false,
    options=DEFAULT_NIL,
    initial_option=1,
    on_change=DEFAULT_NIL,
}

function CycleHotkeyLabel:init()
    self:setOption(self.initial_option)

    if self.label_below then
        self.option_gap = self.option_gap + (self.key_back and 1 or 0) + (self.key and 2 or 0)
    end

    self:setText{
        self.key_back ~= nil and {key=self.key_back, key_sep='', width=0, on_activate=self:callback('cycle', true)} or {},
        {key=self.key, key_sep=self.key_sep, text=self.label, width=self.label_width,
         on_activate=self:callback('cycle')},
        self.label_below and NEWLINE or '',
        {gap=self.option_gap, text=self:callback('getOptionLabel'),
         pen=self:callback('getOptionPen')},
    }
end

-- CycleHotkeyLabels are always clickable and therefore should always change
-- color when hovered.
function CycleHotkeyLabel:shouldHover()
    return true
end

function CycleHotkeyLabel:cycle(backwards)
    local old_option_idx = self.option_idx
    if self.option_idx == #self.options and not backwards then
        self.option_idx = 1
    elseif self.option_idx == 1 and backwards then
        self.option_idx = #self.options
    else
        self.option_idx = self.option_idx + (not backwards and 1 or -1)
    end
    if self.on_change then
        self.on_change(self:getOptionValue(),
                       self:getOptionValue(old_option_idx))
    end
end

function CycleHotkeyLabel:setOption(value_or_index, call_on_change)
    local option_idx = nil
    for i in ipairs(self.options) do
        if value_or_index == self:getOptionValue(i) then
            option_idx = i
            break
        end
    end
    if not option_idx then
        if self.options[value_or_index] then
            option_idx = value_or_index
        end
    end
    if not option_idx then
        option_idx = 1
    end
    local old_option_idx = self.option_idx
    self.option_idx = option_idx
    if call_on_change and self.on_change then
        self.on_change(self:getOptionValue(),
                       self:getOptionValue(old_option_idx))
    end
end

local function cyclehotkeylabel_getOptionElem(self, option_idx, key, require_key)
    option_idx = option_idx or self.option_idx
    local option = self.options[option_idx]
    if type(option) == 'table' then
        return option[key]
    end
    return not require_key and option or nil
end

function CycleHotkeyLabel:getOptionLabel(option_idx)
    return getval(cyclehotkeylabel_getOptionElem(self, option_idx, 'label'))
end

function CycleHotkeyLabel:getOptionValue(option_idx)
    return cyclehotkeylabel_getOptionElem(self, option_idx, 'value')
end

function CycleHotkeyLabel:getOptionPen(option_idx)
    local pen = getval(cyclehotkeylabel_getOptionElem(self, option_idx, 'pen', true))
    if type(pen) == 'string' then return nil end
    return pen
end

function CycleHotkeyLabel:onInput(keys)
    if CycleHotkeyLabel.super.onInput(self, keys) then
        return true
    elseif keys._MOUSE_L and not is_disabled(self) then
        local x = self:getMousePos()
        if x then
            self:cycle(self.key_back and x == 0)
            return true
        end
    end
end

return CycleHotkeyLabel
