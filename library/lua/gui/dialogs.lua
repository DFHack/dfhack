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
    frame_inset = 1,
    -- new attrs
    on_accept = DEFAULT_NIL,
    on_cancel = DEFAULT_NIL,
    on_close = DEFAULT_NIL,
}

function MessageBox:init(info)
    self:addviews{
        widgets.Label{
            view_id = 'label',
            text = info.text,
            text_pen = info.text_pen,
            frame = { l = 0, t = 0 },
            auto_height = true
        }
    }
end

function MessageBox:getWantedFrameSize()
    local label = self.subviews.label
    local width = math.max(self.frame_width or 0, 20, #(self.frame_title or '') + 4)
    return math.max(width, label:getTextWidth()), label:getTextHeight()
end

function MessageBox:onRenderFrame(dc,rect)
    MessageBox.super.onRenderFrame(self,dc,rect)
    if self.on_accept then
        dc:seek(rect.x1+2,rect.y2):key('LEAVESCREEN'):string('/'):key('MENU_CONFIRM')
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
    else
        self:inputToSubviews(keys)
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
            frame = { l = 0, r = 0, h = 1 },
        }
    }
end

function InputBox:getWantedFrameSize()
    local mw, mh = InputBox.super.getWantedFrameSize(self)
    self.subviews.edit.frame.t = mh+1
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
    with_filter = false,
    cursor_pen = DEFAULT_NIL,
    select_pen = DEFAULT_NIL,
    on_select = DEFAULT_NIL,
    on_select2 = DEFAULT_NIL,
    select2_hint = DEFAULT_NIL,
}

function ListBox:preinit(info)
    info.on_accept = nil
end

function ListBox:init(info)
    local spen = dfhack.pen.parse(COLOR_CYAN, self.select_pen, nil, false)
    local cpen = dfhack.pen.parse(COLOR_LIGHTCYAN, self.cursor_pen or self.select_pen, nil, true)

    local list_widget = widgets.List
    if self.with_filter then
        list_widget = widgets.FilteredList
    end

    local on_submit2
    if self.select2_hint or self.on_select2 then
        on_submit2 = function(sel, obj)
            self:dismiss()
            if self.on_select2 then self.on_select2(sel, obj) end
            local cb = obj.on_select2
            if cb then cb(obj, sel) end
        end
    end

    self:addviews{
        list_widget{
            view_id = 'list',
            selected = info.selected,
            choices = info.choices,
            icon_width = info.icon_width,
            text_pen = spen,
            cursor_pen = cpen,
            on_submit = function(sel,obj)
                self:dismiss()
                if self.on_select then self.on_select(sel, obj) end
                local cb = obj.on_select or obj[2]
                if cb then cb(obj, sel) end
            end,
            on_submit2 = on_submit2,
            frame = { l = 0, r = 0 },
        }
    }
end

function ListBox:onRenderFrame(dc,rect)
    ListBox.super.onRenderFrame(self,dc,rect)
    if self.select2_hint then
        dc:seek(rect.x1+2,rect.y2):key('SEC_SELECT'):string(': '..self.select2_hint,COLOR_DARKGREY)
    end
end

function ListBox:getWantedFrameSize()
    local mw, mh = InputBox.super.getWantedFrameSize(self)
    local list = self.subviews.list
    list.frame.t = mh+1
    return math.max(mw, list:getContentWidth()), mh+1+math.min(20,list:getContentHeight())
end

function ListBox:onInput(keys)
    if keys.LEAVESCREEN then
        self:dismiss()
        if self.on_cancel then
            self.on_cancel()
        end
    else
        self:inputToSubviews(keys)
    end
end

function showListPrompt(title, text, tcolor, choices, on_select, on_cancel, min_width, filter)
    ListBox{
        frame_title = title,
        text = text,
        text_pen = tcolor,
        choices = choices,
        on_select = on_select,
        on_cancel = on_cancel,
        frame_width = min_width,
        with_filter = filter,
    }:show()
end

return _ENV
