local gui = require('gui')
local utils = require('utils')
local Widget = require('gui.widgets.widget')
local HotkeyLabel = require('gui.widgets.labels.hotkey_label')
local TextArea = require('gui.widgets.text_area')
local WrappedText = require('gui.widgets.text_area.wrapped_text')

local getval = utils.getval

OneLineWrappedText = defclass(OneLineWrappedText, WrappedText)

function OneLineWrappedText:update(text)
    self.lines = {text}
end

TextFieldArea = defclass(TextFieldArea, TextArea)
TextFieldArea.ATTRS{
    on_char = DEFAULT_NIL,
    key = DEFAULT_NIL,
    on_submit = DEFAULT_NIL,
    on_submit2 = DEFAULT_NIL,
    modal = false,
}

function TextFieldArea:onInput(keys)
    if self.on_char and keys._STRING and keys._STRING ~= 0 then
        if not self.on_char(string.char(keys._STRING), self:getText()) then
            return self.modal
        end
    end

    if keys.SELECT or keys.SELECT_ALL then
        if self.key then
            self:setFocus(false)
        end

        if keys.SELECT_ALL then
            if self.on_submit2 then
                self.on_submit2(self:getText())
                return true
            end
        else
            if self.on_submit then
                self.on_submit(self:getText())
                return true
            end
        end

        return not not self.key
    end

    return TextFieldArea.super.onInput(self, keys)
end

function TextFieldArea:getPreferredFocusState()
    -- allow EditField to manage focus
    return false
end

----------------
-- Edit field --
----------------

---@class widgets.EditField.attrs: widgets.Widget.attrs
---@field label_text? string
---@field text string
---@field text_pen? dfhack.color|dfhack.pen
---@field on_char? function
---@field on_change? function
---@field on_submit? function
---@field on_submit2? function
---@field key? string
---@field key_sep? string
---@field modal boolean
---@field ignore_keys? string[]

---@class widgets.EditField.attrs.partial: widgets.EditField.attrs

---@class widgets.EditField: widgets.Widget, widgets.EditField.attrs
---@field super widgets.Widget
---@field ATTRS widgets.EditField.attrs|fun(attributes: widgets.EditField.attrs.partial)
---@overload fun(init_table: widgets.EditField.attrs.partial): self
EditField = defclass(EditField, Widget)

EditField.ATTRS{
    label_text = DEFAULT_NIL,
    text = '',
    text_pen = DEFAULT_NIL,
    on_char = DEFAULT_NIL,
    on_change = DEFAULT_NIL,
    on_submit = DEFAULT_NIL,
    on_submit2 = DEFAULT_NIL,
    key = DEFAULT_NIL,
    key_sep = DEFAULT_NIL,
    modal = false,
    ignore_keys = DEFAULT_NIL,
}

---@param init_table widgets.EditField.attrs
function EditField:preinit(init_table)
    init_table.frame = init_table.frame or {}
    init_table.frame.h = init_table.frame.h or 1
end

function EditField:init()
    local function on_activate()
        self.saved_text = self.text
        self:setFocus(true)
    end

    self.start_pos = 1
    self.cursor = #self.text + 1
    self.ignore_keys = self.ignore_keys or {}

    self.label = HotkeyLabel{
        frame={t=0,l=0},
        key=self.key,
        key_sep=self.key_sep,
        label=self.label_text,
        on_activate=self.key and on_activate or nil
    }
    self.text_area = TextFieldArea{
        one_line_mode=true,
        frame={t=0,r=0},
        init_text=self.text or '',
        text_pen=self.text_pen or COLOR_LIGHTCYAN,
        modal=self.modal,
        on_char=self.on_char,
        key = self.key,
        on_submit = self.on_submit,
        on_submit2 = self.on_submit2,
        on_focus = self.on_focus,
        on_unfocus = self.on_unfocus,
        ignore_keys={
            table.unpack(self.ignore_keys)
        },
        on_text_change=self:callback('onTextAreaTextChange'),
        on_cursor_change=function(cursor) self.cursor = cursor end
    }

    self:addviews{self.label, self.text_area}

    self.text_area.frame.l = self.label:getTextWidth()
end

function EditField:getPreferredFocusState()
    return not self.key
end

function EditField:setCursor(cursor)
    if not cursor then
        cursor = #self.text + 1
    end
    self.cursor = cursor

    self.text_area:setCursor(cursor)
    self.cursor = self.text_area:getCursor()
end

function EditField:setText(text, cursor)
    text = text or ''

    local old = self.text
    self.text = text
    self.text_area:setText(text)

    self:setCursor(cursor)
    if self.on_change and text ~= old then
        self.on_change(self.text, old)
    end
end

function EditField:onTextAreaTextChange(text)
    if self.text ~= text then
        local old = self.text
        self.text = text
        if self.on_change then
            self.on_change(self.text, old)
        end
    end
end

function EditField:setFocus(focus)
    self.text_area:setFocus(focus)
end

function EditField:insert(text)
    local old = self.text
    self:setText(
        old:sub(1,self.cursor-1)..text..old:sub(self.cursor),
        self.cursor + #text
    )
end

function EditField:onInput(keys)
    if self.text_area.focus and self.key and
     (keys.LEAVESCREEN or keys._MOUSE_R) then
        self:setText(self.saved_text)
        self:setFocus(false)
        return true
    end

    if EditField.super.onInput(self, keys) then
        return true
    end

    -- if we're modal, then unconditionally eat all the input
    return self.modal
end

return EditField
