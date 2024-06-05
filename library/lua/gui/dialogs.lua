-- Some simple dialog screens

local _ENV = mkmodule('gui.dialogs')

local gui = require('gui')
local widgets = require('gui.widgets')

---------------------------------
-- DialogWindow and DialogScreen
--

DialogWindow = defclass(DialogWindow, widgets.Window)
DialogWindow.ATTRS{
    frame={w=20, h=10},
    min_width=30,
    extra_height=0,
    message_label_attrs={},
    accept_hotkey_label_attrs={},
    on_accept=DEFAULT_NIL,
    on_cancel=DEFAULT_NIL,
}

function DialogWindow:init()
    local message_label_attrs = {
        view_id='label',
        auto_height=false,
        frame={t=0, l=0, b=2},
    }
    for k,v in pairs(self.message_label_attrs) do message_label_attrs[k] = v end

    local accept_hotkey_label_attrs = {
        frame={b=0, l=0, r=0},
        key='SELECT',
        label='Ok',
        auto_width=true,
        on_activate=self:callback('accept'),
    }
    for k,v in pairs(self.accept_hotkey_label_attrs) do accept_hotkey_label_attrs[k] = v end

    self:addviews{
        widgets.Label(message_label_attrs),
        widgets.HotkeyLabel(accept_hotkey_label_attrs),
    }
end

function DialogWindow:accept()
    if self.on_accept then
        self.on_accept()
    end
    self.parent_view:dismiss()
end

function DialogWindow:cancel()
    if self.on_cancel then
        self.on_cancel()
    end
    self.parent_view:dismiss()
end

function DialogWindow:computeFrame()
    local min_width = math.max(self.frame.w or 0, 20, self.min_width, #(self.frame_title or '') + 4)

    local label = self.subviews.label
    local text_area_width = label:getTextWidth() + 1
    local text_height = label:getTextHeight()
    local sw, sh = dfhack.screen.getWindowSize()
    if text_height >= sh - (6 + self.extra_height) then
        -- account for scrollbar
        text_area_width = text_area_width + 2
    end

    local fw, fh = math.max(min_width, text_area_width), text_height + 3 + self.extra_height
    return gui.compute_frame_body(sw, sh, {w=fw, h=fh}, 1, 1, true)
end

function DialogWindow:onInput(keys)
    if (keys.LEAVESCREEN or keys._MOUSE_R) and self.on_cancel then
        self:cancel()
    end
    return DialogWindow.super.onInput(self, keys)
end

DialogScreen = defclass(DialogScreen, gui.ZScreenModal)
DialogScreen.ATTRS{
    focus_path='MessageBox',
    title=DEFAULT_NIL,
    min_width=DEFAULT_NIL,
    extra_height=DEFAULT_NIL,
    message_label_attrs=DEFAULT_NIL,
    accept_hotkey_label_attrs=DEFAULT_NIL,
    on_accept=DEFAULT_NIL,
    on_cancel=DEFAULT_NIL,
    on_close=DEFAULT_NIL,
}

function DialogScreen:init(args)
    local window = DialogWindow{
        frame_title=self.title,
        min_width=self.min_width,
        extra_height=self.extra_height,
        message_label_attrs=self.message_label_attrs,
        accept_hotkey_label_attrs=self.accept_hotkey_label_attrs,
        on_accept=self.on_accept,
        on_cancel=self.on_cancel,
        subviews=self.subviews,
    }
    window:addviews(args.subviews)
    self:addviews{window}
end

function DialogScreen:onDismiss()
    if self.on_close then
        self.on_close()
    end
end

------------------------
-- Convenience methods
--

function showMessage(title, text, tcolor, on_close)
    return DialogScreen{
        title=title,
        message_label_attrs={
            text=text,
            text_pen=tcolor,
        },
        on_close=on_close,
    }:show()
end

function showYesNoPrompt(title, text, tcolor, on_accept, on_cancel, on_pause, on_settings)
    local dialog
    local subviews = {
        widgets.HotkeyLabel{
            frame={b=2, r=0},
            label='Settings',
            key='CUSTOM_SHIFT_S',
            auto_width=true,
            on_activate=function()
                dialog:dismiss()
                on_settings()
            end,
            visible=not not on_settings,
        },
        widgets.HotkeyLabel{
            frame={b=2, l=0},
            label='Pause confirmations',
            key='CUSTOM_SHIFT_P',
            auto_width=true,
            on_activate=function()
                dialog:dismiss()
                on_pause()
            end,
            visible=not not on_pause,
        },
    }
    dialog = DialogScreen{
        title=title,
        message_label_attrs={
            frame=(on_pause or on_settings) and {t=0, l=0, b=4} or nil,
            text=text,
            text_pen=tcolor,
        },
        accept_hotkey_label_attrs={
            label='Yes, proceed',
        },
        on_accept=on_accept,
        on_cancel=on_cancel,
        min_width=on_settings and 35 or nil,
        extra_height=(on_pause or on_settings) and 2 or 0,
        subviews=subviews,
    }
    return dialog:show()
end

------------------------
-- Legacy classes
--

MessageBox = defclass(MessageBox, gui.FramedScreen)

MessageBox.focus_path = 'MessageBox'

MessageBox.ATTRS{
    frame_style = gui.WINDOW_FRAME,
    frame_inset = {l=1, t=1, b=1, r=0},
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
    local text_area_width = label:getTextWidth() + 1
    local text_height = label:getTextHeight()
    local sw, sh = dfhack.screen.getWindowSize()
    if text_height > sh - 4 then
        -- account for scrollbar
        text_area_width = text_area_width + 2
    end
    return math.max(width, text_area_width), text_height
end

function MessageBox:onRenderFrame(dc,rect)
    MessageBox.super.onRenderFrame(self,dc,rect)
    if self.on_accept then
        dc:seek(rect.x1+2,rect.y2):key('LEAVESCREEN'):string('/'):key('SELECT')
    end
end

function MessageBox:onDestroy()
    if self.on_close then
        self.on_close()
    end
end

function MessageBox:onInput(keys)
    if keys.SELECT or keys.LEAVESCREEN or keys._MOUSE_R then
        self:dismiss()
        if keys.SELECT and self.on_accept then
            self.on_accept()
        elseif (keys.LEAVESCREEN or keys._MOUSE_R) and self.on_cancel then
            self.on_cancel()
        end
        return true
    end
    self:inputToSubviews(keys)
    return true
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
        return true
    elseif keys.LEAVESCREEN or keys._MOUSE_R then
        self:dismiss()
        if self.on_cancel then
            self.on_cancel()
        end
        return true
    end
    self:inputToSubviews(keys)
    return true
end

function showInputPrompt(title, text, tcolor, input, on_input, on_cancel, min_width)
    local ib = InputBox{
        frame_title = title,
        text = text,
        text_pen = tcolor,
        input = input,
        on_input = on_input,
        on_cancel = on_cancel,
        frame_width = min_width,
    }
    ib:show()
    return ib
end

ListBox = defclass(ListBox, MessageBox)

ListBox.focus_path = 'ListBox'

ListBox.ATTRS{
    with_filter = false,
    dismiss_on_select = true,
    dismiss_on_select2 = true,
    cursor_pen = DEFAULT_NIL,
    select_pen = DEFAULT_NIL,
    on_select = DEFAULT_NIL,
    on_select2 = DEFAULT_NIL,
    select2_hint = DEFAULT_NIL,
    row_height = DEFAULT_NIL,
    list_frame_inset = DEFAULT_NIL,
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
            if self.dismiss_on_select2 then self:dismiss() end
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
                if self.dismiss_on_select then self:dismiss() end
                if self.on_select then self.on_select(sel, obj) end
                local cb = obj.on_select or obj[2]
                if cb then cb(obj, sel) end
            end,
            on_submit2 = on_submit2,
            frame = { l = 0, r = 0},
            frame_inset = self.list_frame_inset,
            row_height = self.row_height,
        }
    }
end

function ListBox:onRenderFrame(dc,rect)
    ListBox.super.onRenderFrame(self,dc,rect)
    if self.select2_hint then
       dc:seek(rect.x1+2,rect.y2):string('Shift-Click', COLOR_LIGHTGREEN):string(': '..self.select2_hint, COLOR_GREY)
    end
end

function ListBox:getWantedFrameSize()
    local mw, mh = ListBox.super.getWantedFrameSize(self)
    local list = self.subviews.list
    list.frame.t = mh+1
    return math.max(mw, list:getContentWidth()), mh+3+math.min(18,list:getContentHeight())
end

function ListBox:onInput(keys)
    if keys.LEAVESCREEN or keys._MOUSE_R then
        self:dismiss()
        if self.on_cancel then
            self.on_cancel()
        end
        return true
    end
    self:inputToSubviews(keys)
    return true
end

function showListPrompt(title, text, tcolor, choices, on_select, on_cancel, min_width, filter)
    local lb = ListBox{
        frame_title = title,
        text = text,
        text_pen = tcolor,
        choices = choices,
        on_select = on_select,
        on_cancel = on_cancel,
        frame_width = min_width,
        with_filter = filter,
    }
    lb:show()
    return lb
end

return _ENV
