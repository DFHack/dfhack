local gui = require('gui')
local utils = require('utils')
local Widget = require('gui.widgets.widget')
local HotkeyLabel = require('gui.widgets.labels.hotkey_label')

local getval = utils.getval

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

    self:addviews{HotkeyLabel{frame={t=0,l=0},
                              key=self.key,
                              key_sep=self.key_sep,
                              label=self.label_text,
                              on_activate=self.key and on_activate or nil}}
end

function EditField:getPreferredFocusState()
    return not self.key
end

function EditField:setCursor(cursor)
    if not cursor or cursor > #self.text then
        self.cursor = #self.text + 1
        return
    end
    self.cursor = math.max(1, cursor)
end

function EditField:setText(text, cursor)
    local old = self.text
    self.text = text
    self:setCursor(cursor)
    if self.on_change and text ~= old then
        self.on_change(self.text, old)
    end
end

function EditField:postUpdateLayout()
    self.text_offset = self.subviews[1]:getTextWidth()
end

---@param dc gui.Painter
function EditField:onRenderBody(dc)
    dc:pen(self.text_pen or COLOR_LIGHTCYAN)

    local cursor_char = '_'
    if not getval(self.active) or not self.focus or gui.blink_visible(300) then
        cursor_char = (self.cursor > #self.text) and ' ' or
                                        self.text:sub(self.cursor, self.cursor)
    end
    local txt = self.text:sub(1, self.cursor - 1) .. cursor_char ..
                                                self.text:sub(self.cursor + 1)
    local max_width = dc.width - self.text_offset
    self.start_pos = 1
    if #txt > max_width then
        -- get the substring in the vicinity of the cursor
        max_width = max_width - 2
        local half_width = math.floor(max_width/2)
        local start_pos = math.max(1, self.cursor-half_width)
        local end_pos = math.min(#txt, self.cursor+half_width-1)
        if self.cursor + half_width > #txt then
            start_pos = #txt - (max_width - 1)
        end
        if self.cursor - half_width <= 1 then
            end_pos = max_width + 1
        end
        self.start_pos = start_pos > 1 and start_pos - 1 or start_pos
        txt = ('%s%s%s'):format(start_pos == 1 and '' or string.char(27),
                                txt:sub(start_pos, end_pos),
                                end_pos == #txt and '' or string.char(26))
    end
    dc:advance(self.text_offset):string(txt)
end

function EditField:insert(text)
    local old = self.text
    self:setText(old:sub(1,self.cursor-1)..text..old:sub(self.cursor),
                 self.cursor + #text)
end

function EditField:onInput(keys)
    if not self.focus then
        -- only react to our hotkey
        return self:inputToSubviews(keys)
    end

    if self.ignore_keys then
        for _,ignore_key in ipairs(self.ignore_keys) do
            if keys[ignore_key] then return false end
        end
    end

    if self.key and (keys.LEAVESCREEN or keys._MOUSE_R) then
        self:setText(self.saved_text)
        self:setFocus(false)
        return true
    end

    if keys.SELECT or keys.SELECT_ALL then
        if self.key then
            self:setFocus(false)
        end
        if keys.SELECT_ALL then
            if self.on_submit2 then
                self.on_submit2(self.text)
                return true
            end
        else
            if self.on_submit then
                self.on_submit(self.text)
                return true
            end
        end
        return not not self.key
    elseif keys.CUSTOM_DELETE then
        local old = self.text
        local del_pos = self.cursor
        if del_pos <= #old then
            self:setText(old:sub(1, del_pos-1) .. old:sub(del_pos+1), del_pos)
        end
        return true
    elseif keys._STRING then
        local old = self.text
        if keys._STRING == 0 then
            -- handle backspace
            local del_pos = self.cursor - 1
            if del_pos > 0 then
                self:setText(old:sub(1, del_pos-1) .. old:sub(del_pos+1), del_pos)
            end
        else
            local cv = string.char(keys._STRING)
            if not self.on_char or self.on_char(cv, old) then
                self:insert(cv)
            elseif self.on_char then
                return self.modal
            end
        end
        return true
    elseif keys.KEYBOARD_CURSOR_LEFT then
        self:setCursor(self.cursor - 1)
        return true
    elseif keys.CUSTOM_CTRL_LEFT then -- back one word
        local _, prev_word_end = self.text:sub(1, self.cursor-1):
                                               find('.*[%w_%-][^%w_%-]')
        self:setCursor(prev_word_end or 1)
        return true
    elseif keys.CUSTOM_HOME then
        self:setCursor(1)
        return true
    elseif keys.KEYBOARD_CURSOR_RIGHT then
        self:setCursor(self.cursor + 1)
        return true
    elseif keys.CUSTOM_CTRL_RIGHT then -- forward one word
        local _,next_word_start = self.text:find('[^%w_%-][%w_%-]', self.cursor)
        self:setCursor(next_word_start)
        return true
    elseif keys.CUSTOM_END then
        self:setCursor()
        return true
    elseif keys.CUSTOM_CTRL_C then
        dfhack.internal.setClipboardTextCp437(self.text)
        return true
    elseif keys.CUSTOM_CTRL_X then
        dfhack.internal.setClipboardTextCp437(self.text)
        self:setText('')
        return true
    elseif keys.CUSTOM_CTRL_V then
        self:insert(dfhack.internal.getClipboardTextCp437())
        return true
    elseif keys._MOUSE_L_DOWN then
        local mouse_x = self:getMousePos()
        if mouse_x then
            self:setCursor(self.start_pos + mouse_x - (self.text_offset or 0))
            return true
        end
    end

    -- if we're modal, then unconditionally eat all the input
    return self.modal
end

return EditField
