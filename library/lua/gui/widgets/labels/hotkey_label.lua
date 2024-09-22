local utils = require('utils')
local Label = require('gui.widgets.labels.label')

local getval = utils.getval

local function is_disabled(token)
    return (token.disabled ~= nil and getval(token.disabled)) or
           (token.enabled ~= nil and not getval(token.enabled))
end

-----------------
-- HotkeyLabel --
-----------------

---@class widgets.HotkeyLabel.attrs: widgets.Label.attrs
---@field key? string
---@field key_sep string
---@field label? string|fun(): string
---@field on_activate? function

---@class widgets.HotkeyLabel.attrs.partial: widgets.HotkeyLabel.attrs

---@class widgets.HotkeyLabel: widgets.Label, widgets.HotkeyLabel.attrs
---@field super widgets.Label
---@field ATTRS widgets.HotkeyLabel.attrs|fun(attributes: widgets.HotkeyLabel.attrs.partial)
---@overload fun(init_table: widgets.HotkeyLabel.attrs.partial): self
HotkeyLabel = defclass(HotkeyLabel, Label)

HotkeyLabel.ATTRS{
    key=DEFAULT_NIL,
    key_sep=': ',
    label=DEFAULT_NIL,
    on_activate=DEFAULT_NIL,
}

function HotkeyLabel:init()
    self:initializeLabel()
end

function HotkeyLabel:setOnActivate(on_activate)
    self.on_activate = on_activate
    self:initializeLabel()
end

function HotkeyLabel:setLabel(label)
    self.label = label
    self:initializeLabel()
end

function HotkeyLabel:shouldHover()
    -- When on_activate is set, text should also hover on mouseover
    return self.on_activate or HotkeyLabel.super.shouldHover(self)
end

function HotkeyLabel:initializeLabel()
    self:setText{{key=self.key, key_sep=self.key_sep, text=self.label,
                  on_activate=self.on_activate}}
end

function HotkeyLabel:onInput(keys)
    if HotkeyLabel.super.onInput(self, keys) then
        return true
    elseif keys._MOUSE_L and self:getMousePos() and self.on_activate
            and not is_disabled(self) then
        self.on_activate()
        return true
    end
end

return HotkeyLabel
