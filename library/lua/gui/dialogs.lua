-- Some simple dialog screens

local _ENV = mkmodule('gui.dialogs')

local gui = require('gui')
local widgets = require('gui.widgets')
local utils = require('utils')

local dscreen = dfhack.screen

MessageBox = defclass(MessageBox, gui.FramedScreen)

MessageBox.focus_path = 'MessageBox'

MessageBox.ATTRS{
    frame_style = gui.GREY_LINE_FRAME,
    -- new attrs
    text = {},
    on_accept = DEFAULT_NIL,
    on_cancel = DEFAULT_NIL,
    on_close = DEFAULT_NIL,
    text_pen = DEFAULT_NIL,
}

function MessageBox:preinit(info)
    if type(info.text) == 'string' then
        info.text = utils.split_string(info.text, "\n")
    end
end

function MessageBox:getWantedFrameSize()
    local text = self.text
    local w = #(self.frame_title or '') + 4
    w = math.max(w, 20)
    w = math.max(self.frame_width or w, w)
    for _, l in ipairs(text) do
        w = math.max(w, #l)
    end
    local h = #text+1
    if h > 1 then
        h = h+1
    end
    return w+2, #text+2
end

function MessageBox:onRenderBody(dc)
    if #self.text > 0 then
        dc:newline(1):pen(self.text_pen or COLOR_GREY)
        for _, l in ipairs(self.text or {}) do
            dc:string(l):newline(1)
        end
    end

    if self.on_accept then
        local fr = self.frame_rect
        local dc2 = gui.Painter.new_xy(fr.x1+1,fr.y2+1,fr.x2-8,fr.y2+1)
        dc2:key('LEAVESCREEN'):string('/'):key('MENU_CONFIRM')
    end
end

function MessageBox:onDestroy()
    if self.on_close then
        self.on_close()
    end
end

function MessageBox:onInput(keys)
    if keys.MENU_CONFIRM then
        self:dismiss()
        if self.on_accept then
            self.on_accept()
        end
    elseif keys.LEAVESCREEN or (keys.SELECT and not self.on_accept) then
        self:dismiss()
        if self.on_cancel then
            self.on_cancel()
        end
    end
end

function showMessage(title, text, tcolor, on_close)
    MessageBox{
        frame_title = title,
        text = text,
        text_pen = tcolor,
        on_close = on_close
    }:show()
end

function showYesNoPrompt(title, text, tcolor, on_accept, on_cancel)
    MessageBox{
        frame_title = title,
        text = text,
        text_pen = tcolor,
        on_accept = on_accept,
        on_cancel = on_cancel,
    }:show()
end

InputBox = defclass(InputBox, MessageBox)

InputBox.focus_path = 'InputBox'

InputBox.ATTRS{
    on_input = DEFAULT_NIL,
}

function InputBox:preinit(info)
    info.on_accept = nil
end

function InputBox:init(info)
    self:addviews{
        widgets.EditField{
            view_id = 'edit',
            text = info.input,
            text_pen = info.input_pen,
            frame = { l = 1, r = 1, h = 1 },
        }
    }
end

function InputBox:getWantedFrameSize()
    local mw, mh = InputBox.super.getWantedFrameSize(self)
    self.subviews.edit.frame.t = mh
    return mw, mh+2
end

function InputBox:onInput(keys)
    if keys.SELECT then
        self:dismiss()
        if self.on_input then
            self.on_input(self.subviews.edit.text)
        end
    elseif keys.LEAVESCREEN then
        self:dismiss()
        if self.on_cancel then
            self.on_cancel()
        end
    else
        self:inputToSubviews(keys)
    end
end

function showInputPrompt(title, text, tcolor, input, on_input, on_cancel, min_width)
    InputBox{
        frame_title = title,
        text = text,
        text_pen = tcolor,
        input = input,
        on_input = on_input,
        on_cancel = on_cancel,
        frame_width = min_width,
    }:show()
end

ListBox = defclass(ListBox, MessageBox)

ListBox.focus_path = 'ListBox'

ListBox.ATTRS{
    selection = 1,
    choices = {},
    select_pen = DEFAULT_NIL,
    on_select = DEFAULT_NIL
}

function InputBox:preinit(info)
    info.on_accept = nil
end

function ListBox:init(info)
    self.page_top = 1
end

local function choice_text(entry)
    if type(entry)=="table" then
        return entry.caption or entry[1]
    else
        return entry
    end
end

function ListBox:getWantedFrameSize()
    local mw, mh = ListBox.super.getWantedFrameSize(self)
    for _,v in ipairs(self.choices) do
        local text = choice_text(v)
        mw = math.max(mw, #text+2)
    end
    return mw, mh+#self.choices+1
end

function ListBox:postUpdateLayout()
    self.page_size = self.frame_rect.height - #self.text - 3
    self:moveCursor(0)
end

function ListBox:moveCursor(delta)
    local page = math.max(1, self.page_size)
    local cnt = #self.choices
    local off = self.selection+delta-1
    local ds = math.abs(delta)

    if ds > 1 then
        if off >= cnt+ds-1 then
            off = 0
        else
            off = math.min(cnt-1, off)
        end
        if off <= -ds then
            off = cnt-1
        else
            off = math.max(0, off)
        end
    end

    self.selection = 1 + off % cnt
    self.page_top = 1 + page * math.floor((self.selection-1) / page)
end

function ListBox:onRenderBody(dc)
    ListBox.super.onRenderBody(self, dc)

    dc:newline(1):pen(self.select_pen or COLOR_CYAN)

    local choices = self.choices
    local iend = math.min(#choices, self.page_top+self.page_size-1)

    for i = self.page_top,iend do
        local text = choice_text(choices[i])
        if text then
            dc.cur_pen.bold = (i == self.selection);
            dc:string(text)
        else
            dc:string('?ERROR?', COLOR_LIGHTRED)
        end
        dc:newline(1)
    end
end

function ListBox:onInput(keys)
    if keys.SELECT then
        self:dismiss()

        local choice=self.choices[self.selection]
        if self.on_select then
            self.on_select(self.selection, choice)
        end

        if choice then
            local callback = choice.on_select or choice[2]
            if callback then
                callback(choice, self.selection)
            end
        end
    elseif keys.LEAVESCREEN then
        self:dismiss()
        if self.on_cancel then
            self.on_cancel()
        end
    elseif keys.STANDARDSCROLL_UP then
        self:moveCursor(-1)
    elseif keys.STANDARDSCROLL_DOWN then
        self:moveCursor(1)
    elseif keys.STANDARDSCROLL_PAGEUP then
        self:moveCursor(-self.page_size)
    elseif keys.STANDARDSCROLL_PAGEDOWN then
        self:moveCursor(self.page_size)
    end
end

function showListPrompt(title, text, tcolor, choices, on_select, on_cancel, min_width)
    ListBox{
        frame_title = title,
        text = text,
        text_pen = tcolor,
        choices = choices,
        on_select = on_select,
        on_cancel = on_cancel,
        frame_width = min_width,
    }:show()
end

return _ENV
