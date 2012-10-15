-- Simple widgets for screens

local _ENV = mkmodule('gui.widgets')

local gui = require('gui')
local utils = require('utils')

local dscreen = dfhack.screen

------------
-- Widget --
------------

Widget = defclass(Widget, gui.View)

Widget.ATTRS {
    frame = DEFAULT_NIL,
    frame_inset = DEFAULT_NIL,
    frame_background = DEFAULT_NIL,
}

function Widget:computeFrame(parent_rect)
    local sw, sh = parent_rect.width, parent_rect.height
    return gui.compute_frame_body(sw, sh, self.frame, self.frame_inset)
end

function Widget:render(dc)
    if self.frame_background then
        dc:fill(self.frame_rect, self.frame_background)
    end

    Widget.super.render(self, dc)
end

----------------
-- Edit field --
----------------

EditField = defclass(EditField, Widget)

EditField.ATTRS{
    text = '',
    text_pen = DEFAULT_NIL,
    on_char = DEFAULT_NIL,
    on_change = DEFAULT_NIL,
}

function EditField:onRenderBody(dc)
    dc:pen(self.text_pen or COLOR_LIGHTCYAN):fill(0,0,dc.width-1,0)

    local cursor = '_'
    if not self.active or gui.blink_visible(300) then
        cursor = ' '
    end
    local txt = self.text .. cursor
    if #txt > dc.width then
        txt = string.char(27)..string.sub(txt, #txt-dc.width+2)
    end
    dc:string(txt)
end

function EditField:onInput(keys)
    if keys._STRING then
        local old = self.text
        if keys._STRING == 0 then
            self.text = string.sub(old, 1, #old-1)
        else
            local cv = string.char(keys._STRING)
            if not self.on_char or self.on_char(cv, old) then
                self.text = old .. cv
            end
        end
        if self.on_change and self.text ~= old then
            self.on_change(self.text, old)
        end
        return true
    end
end

return _ENV
