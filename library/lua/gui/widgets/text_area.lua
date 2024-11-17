-- Multiline text area control

local Panel = require('gui.widgets.containers.panel')
local Scrollbar = require('gui.widgets.scrollbar')
local TextAreaContent = require('gui.widgets.text_area.text_area_content')
local HistoryStore = require('gui.widgets.text_area.history_store')

local HISTORY_ENTRY = HistoryStore.HISTORY_ENTRY

TextArea = defclass(TextArea, Panel)

TextArea.ATTRS{
    init_text = '',
    init_cursor = DEFAULT_NIL,
    text_pen = COLOR_LIGHTCYAN,
    ignore_keys = {'STRING_A096'},
    select_pen = COLOR_CYAN,
    on_text_change = DEFAULT_NIL,
    on_cursor_change = DEFAULT_NIL,
    one_line_mode = false,
    debug = false
}

function TextArea:init()
    self.render_start_line_y = 1

    self.text_area = TextAreaContent{
        frame={l=0,r=3,t=0},
        text=self.init_text,

        text_pen=self.text_pen,
        ignore_keys=self.ignore_keys,
        select_pen=self.select_pen,
        debug=self.debug,
        one_line_mode=self.one_line_mode,

        on_text_change=function (val)
            self:updateLayout()
            if self.on_text_change then
                self.on_text_change(val)
            end
        end,
        on_cursor_change=self:callback('onCursorChange')
    }
    self.scrollbar = Scrollbar{
        frame={r=0,t=1},
        on_scroll=self:callback('onScrollbar'),
        visible=not self.one_line_mode
    }

    self:addviews{
        self.text_area,
        self.scrollbar,
    }
    self:setFocus(true)
end

function TextArea:getText()
    return self.text_area.text
end

function TextArea:setText(text)
    self.text_area.history:store(
        HISTORY_ENTRY.OTHER,
        self:getText(),
        self:getCursor()
    )

    return self.text_area:setText(text)
end

function TextArea:getCursor()
    return self.text_area.cursor
end

function TextArea:setCursor(cursor_offset)
    return self.text_area:setCursor(cursor_offset)
end

function TextArea:clearHistory()
    return self.text_area.history:clear()
end

function TextArea:onCursorChange(cursor, old_cursor)
    local x, y = self.text_area.wrapped_text:indexToCoords(
        self.text_area.cursor
    )

    if y >= self.render_start_line_y + self.text_area.frame_body.height then
        self:updateScrollbar(
            y - self.text_area.frame_body.height + 1
        )
    elseif  (y < self.render_start_line_y) then
        self:updateScrollbar(y)
    end

    if self.on_cursor_change then
        self.on_cursor_change(cursor, old_cursor)
    end
end

function TextArea:scrollToCursor(cursor_offset)
    if self.scrollbar.visible then
        local _, cursor_liny_y = self.text_area.wrapped_text:indexToCoords(
            cursor_offset
        )
        self:updateScrollbar(cursor_liny_y)
    end
end

function TextArea:getPreferredFocusState()
    return self.parent_view.focus
end

function TextArea:postUpdateLayout()
    self:updateScrollbar(self.render_start_line_y)

    if self.text_area.cursor == nil then
        local cursor = self.init_cursor or #self.init_text + 1
        self.text_area:setCursor(cursor)
        self:scrollToCursor(cursor)
    end
end

function TextArea:onScrollbar(scroll_spec)
    local height = self.text_area.frame_body.height

    local render_start_line = self.render_start_line_y
    if scroll_spec == 'down_large' then
        render_start_line = render_start_line + math.ceil(height / 2)
    elseif scroll_spec == 'up_large' then
        render_start_line = render_start_line - math.ceil(height / 2)
    elseif scroll_spec == 'down_small' then
        render_start_line = render_start_line + 1
    elseif scroll_spec == 'up_small' then
        render_start_line = render_start_line - 1
    else
        render_start_line = tonumber(scroll_spec)
    end

    self:updateScrollbar(render_start_line)
end

function TextArea:updateScrollbar(scrollbar_current_y)
    local lines_count = #self.text_area.wrapped_text.lines

    local render_start_line_y = (math.min(
        #self.text_area.wrapped_text.lines - self.text_area.frame_body.height + 1,
        math.max(1, scrollbar_current_y)
    ))

    self.scrollbar:update(
        render_start_line_y,
        self.frame_body.height,
        lines_count
    )

    if (self.frame_body.height >= lines_count) then
        render_start_line_y = 1
    end

    self.render_start_line_y = render_start_line_y
    self.text_area:setRenderStartLineY(self.render_start_line_y)
end

function TextArea:renderSubviews(dc)
    self.text_area.frame_body.y1 = self.frame_body.y1-(self.render_start_line_y - 1)

    TextArea.super.renderSubviews(self, dc)
end

function TextArea:onInput(keys)
    if (self.scrollbar.is_dragging) then
        return self.scrollbar:onInput(keys)
    end

    if keys._MOUSE_L and self:getMousePos() then
        self:setFocus(true)
    end

    return TextArea.super.onInput(self, keys)
end

return TextArea
