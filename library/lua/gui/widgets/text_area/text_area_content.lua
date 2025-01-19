local gui = require('gui')
local common = require('gui.widgets.common')
local Widget = require('gui.widgets.widget')
local WrappedText = require('gui.widgets.text_area.wrapped_text')
local HistoryStore = require('gui.widgets.text_area.history_store')

local CLIPBOARD_MODE = {LOCAL = 1, LINE = 2}
local HISTORY_ENTRY = HistoryStore.HISTORY_ENTRY

OneLineWrappedText = defclass(OneLineWrappedText, WrappedText)

function OneLineWrappedText:update(text)
    self.lines = {text}
end

TextAreaContent = defclass(TextAreaContent, Widget)

TextAreaContent.ATTRS{
    text = '',
    text_pen = COLOR_LIGHTCYAN,
    ignore_keys = {},
    pen_selection = COLOR_CYAN,
    on_text_change = DEFAULT_NIL,
    on_cursor_change = DEFAULT_NIL,
    enable_cursor_blink = true,
    debug = false,
    one_line_mode = false,
    history_size = 25,
}

function TextAreaContent:init()
    self.sel_end = nil
    self.clipboard = nil
    self.clipboard_mode = CLIPBOARD_MODE.LOCAL
    self.render_start_line_y = 1

    self.cursor = nil

    self.main_pen = dfhack.pen.parse({
        fg=self.text_pen,
        bg=COLOR_RESET,
        bold=true
    })
    self.sel_pen = dfhack.pen.parse({
        fg=self.text_pen,
        bg=self.pen_selection,
        bold=true
    })

    local TextWrapper = self.one_line_mode and OneLineWrappedText or WrappedText
    self.wrapped_text = TextWrapper{
        text=self.text,
        wrap_width=256
    }

    self.history = HistoryStore{history_size=self.history_size}
end

function TextAreaContent:setRenderStartLineY(render_start_line_y)
    self.render_start_line_y = render_start_line_y
end

function TextAreaContent:postComputeFrame()
    self:recomputeLines()
end

function TextAreaContent:recomputeLines()
    if not self.frame_body then
        -- called before first layout compute
        return
    end

    self.wrapped_text:update(
        self.text,
        -- something cursor '_' need to be add at the end of a line
        self.frame_body.width - 1
    )
end

function TextAreaContent:setCursor(cursor_offset)
    local old_cursor = self.cursor

    self.cursor = math.max(
        1,
        math.min(#self.text + 1, cursor_offset)
    )

    if self.debug then
        print('cursor', self.cursor)
    end

    self.sel_end = nil
    self.last_cursor_x = nil

    if self.on_cursor_change and self.cursor ~= old_cursor then
        self.on_cursor_change(self.cursor, old_cursor)
    end
end

function TextAreaContent:setSelection(from_offset, to_offset)
    -- text selection is always start on self.cursor and end on self.sel_end
    self:setCursor(from_offset)
    self.sel_end = to_offset

    if self.debug and to_offset then
        print('sel_end', to_offset)
    end
end

function TextAreaContent:hasSelection()
    return not not self.sel_end
end

function TextAreaContent:eraseSelection()
    if (self:hasSelection()) then
        local from, to = self.cursor, self.sel_end
        if (from > to) then
            from, to = to, from
        end

        local new_text = self.text:sub(1, from - 1) .. self.text:sub(to + 1)
        self:setText(new_text)

        self:setCursor(from)
        self.sel_end = nil
    end
end

function TextAreaContent:setClipboard(text)
    dfhack.internal.setClipboardTextCp437Multiline(text)
end

function TextAreaContent:copy()
    if self.sel_end then
        self.clipboard_mode =  CLIPBOARD_MODE.LOCAL

        local from = self.cursor
        local to = self.sel_end

        if from > to then
            from, to = to, from
        end

        self:setClipboard(self.text:sub(from, to))

        return from, to
    else
        self.clipboard_mode = CLIPBOARD_MODE.LINE

        local curr_line = self.text:sub(
            self:lineStartOffset(),
            self:lineEndOffset()
        )
        if curr_line:sub(-1,-1) ~= NEWLINE then
            curr_line = curr_line .. NEWLINE
        end

        self:setClipboard(curr_line)

        return self:lineStartOffset(), self:lineEndOffset()
    end
end

function TextAreaContent:cut()
    local from, to = self:copy()
    if not self:hasSelection() then
        self:setSelection(from, to)
    end
    self:eraseSelection()
end

function TextAreaContent:paste()
    local clipboard_lines = dfhack.internal.getClipboardTextCp437Multiline()
    local clipboard = table.concat(clipboard_lines, '\n')
    if clipboard then
        if self.clipboard_mode == CLIPBOARD_MODE.LINE and not self:hasSelection() then
            local origin_offset = self.cursor
            self:setCursor(self:lineStartOffset())
            self:insert(clipboard)
            self:setCursor(#clipboard + origin_offset)
        else
            self:eraseSelection()
            self:insert(clipboard)
        end

    end
end

function TextAreaContent:setText(text)
    local old_text = self.text

    self.text = text

    self:recomputeLines()

    if self.on_text_change and text ~= old_text then
        self.on_text_change(text, old_text)
    end
end

function TextAreaContent:insert(text)
    self:eraseSelection()
    local new_text =
        self.text:sub(1, self.cursor - 1) ..
        text ..
        self.text:sub(self.cursor)

    self:setText(new_text)
    self:setCursor(self.cursor + #text)
end

function TextAreaContent:normalizeLine(text_line)
    if self.one_line_mode or self.debug then
        return text_line
    else
        -- do not render new lines symbol in multiline mode
        return text_line:gsub(NEWLINE, '')
    end
end

function TextAreaContent:onRenderBody(dc)
    dc:pen(self.main_pen)

    local max_width = dc.width

    local lines_to_render = math.min(
        dc.height,
        #self.wrapped_text.lines - self.render_start_line_y + 1
    )

    dc:seek(0, self.render_start_line_y - 1)
    for i = self.render_start_line_y, self.render_start_line_y + lines_to_render - 1 do
        local line = self:normalizeLine(self.wrapped_text.lines[i])
        dc:string(line)
        dc:newline()
    end

    local show_focus = not self.enable_cursor_blink
        or (
            not self:hasSelection()
            and self.parent_view.focus
            and gui.blink_visible(530)
        )

    if show_focus then
        local x, y = self.wrapped_text:indexToCoords(self.cursor)
        dc:seek(x - 1, y - 1)
            :char('_')
    end

    if self:hasSelection() then
        local from, to = self.cursor, self.sel_end
        if (from > to) then
            from, to = to, from
        end

        local from_x, from_y = self.wrapped_text:indexToCoords(from)
        local to_x, to_y = self.wrapped_text:indexToCoords(to)

        local line = self:normalizeLine(
            self.wrapped_text.lines[from_y]:sub(
                from_x, to_y == from_y and to_x or nil
            )
        )

        dc:pen(self.sel_pen)
            :seek(from_x - 1, from_y - 1)
            :string(line)

        for y = from_y + 1, to_y - 1 do
            line = self:normalizeLine(self.wrapped_text.lines[y])
            dc:seek(0, y - 1)
                :string(line)
        end

        if (to_y > from_y) then
            local line = self:normalizeLine(
                self.wrapped_text.lines[to_y]:sub(1, to_x)
            )
            dc:seek(0, to_y - 1)
                :string(line)
        end

        dc:pen({fg=self.text_pen, bg=COLOR_RESET})
    end

    if self.debug then
        local cursor_char = self:charAtCursor()
        local x, y = self.wrapped_text:indexToCoords(self.cursor)
        local debug_msg = string.format(
            'x: %s y: %s ind: %s #line: %s char: %s hist-: %s hist+: %s',
            x,
            y,
            self.cursor,
            self:lineEndOffset() - self:lineStartOffset(),
            (cursor_char == NEWLINE and 'NEWLINE') or
            (cursor_char == ' ' and 'SPACE') or
            (cursor_char == '' and 'nil') or
            cursor_char,
            #self.history.past,
            #self.history.future
        )
        local sel_debug_msg = self.sel_end and string.format(
            'sel_end: %s',
            self.sel_end
        ) or ''

        dc:pen({fg=COLOR_LIGHTRED, bg=COLOR_RESET})
            :seek(0, self.parent_view.frame_body.height + self.render_start_line_y - 2)
            :string(debug_msg)
            :seek(0, self.parent_view.frame_body.height + self.render_start_line_y - 3)
            :string(sel_debug_msg)
    end
end

function TextAreaContent:charAtCursor()
    return self.text:sub(self.cursor, self.cursor)
end

function TextAreaContent:getMultiLeftClick(x, y)
    if self.last_click then
        local from_last_click_ms = dfhack.getTickCount() - self.last_click.tick

        if (
            self.last_click.x ~= x or
            self.last_click.y ~= y or
            from_last_click_ms > common.DOUBLE_CLICK_MS
        ) then
            self.clicks_count = 0;
        end
    end

    return self.clicks_count or 0
end

function TextAreaContent:triggerMultiLeftClick(x, y)
    local clicks_count = self:getMultiLeftClick(x, y)

    self.clicks_count = clicks_count + 1
    if (self.clicks_count >= 4) then
        self.clicks_count = 1
    end

    self.last_click = {
        tick=dfhack.getTickCount(),
        x=x,
        y=y,
    }
    return self.clicks_count
end

function TextAreaContent:currentSpacesRange()
    -- select "word" only from spaces
    local prev_word_end, _  = self.text
        :sub(1, self.cursor)
        :find('[^%s]%s+$')
    local _, next_word_start = self.text:find('%s[^%s]', self.cursor)

    return prev_word_end + 1 or 1, next_word_start - 1 or #self.text
end

function TextAreaContent:currentWordRange()
    -- select current word
    local _, prev_word_end = self.text
        :sub(1, self.cursor - 1)
        :find('.*[%s,."\']')
    local next_word_start, _  = self.text:find('[%s,."\']', self.cursor)

    return (prev_word_end or 0) + 1, (next_word_start or #self.text + 1) - 1
end

function TextAreaContent:lineStartOffset(offset)
    local loc_offset = offset or self.cursor
    return self.text:sub(1, loc_offset - 1):match(".*\n()") or 1
end

function TextAreaContent:lineEndOffset(offset)
    local loc_offset = offset or self.cursor
    return self.text:find("\n", loc_offset) or #self.text + 1
end

function TextAreaContent:wordStartOffset(offset)
    return self.text
        :sub(1, offset or self.cursor - 1)
        :match('.*%s()[^%s]') or 1
end

function TextAreaContent:wordEndOffset(offset)
    return self.text
        :match(
            '%s*[^%s]*()',
            offset or self.cursor
        ) or #self.text + 1
end

function TextAreaContent:onInput(keys)
    for _,ignore_key in ipairs(self.ignore_keys) do
        if keys[ignore_key] then
            return false
        end
    end

    if self:onMouseInput(keys) then
        return true
    elseif self:onHistoryInput(keys) then
        return true
    elseif self:onTextManipulationInput(keys) then
        return true
    elseif self:onCursorInput(keys) then
        return true
    elseif keys.CUSTOM_CTRL_C then
        self:copy()
        return true
    elseif keys.CUSTOM_CTRL_X then
        self.history:store(HISTORY_ENTRY.OTHER, self.text, self.cursor)
        self:cut()
        return true
    elseif keys.CUSTOM_CTRL_V then
        self.history:store(HISTORY_ENTRY.OTHER, self.text, self.cursor)
        self:paste()
        return true
    else
        return TextAreaContent.super.onInput(self, keys)
    end
end

function TextAreaContent:onHistoryInput(keys)
    if keys.CUSTOM_CTRL_Z then
        local history_entry = self.history:undo(self.text, self.cursor)

        if history_entry then
            self:setText(history_entry.text)
            self:setCursor(history_entry.cursor)
        end

        return true
    elseif keys.CUSTOM_CTRL_Y then
        local history_entry = self.history:redo(self.text, self.cursor)

        if history_entry then
            self:setText(history_entry.text)
            self:setCursor(history_entry.cursor)
        end

        return true
    end
end

function TextAreaContent:onMouseInput(keys)
    if keys._MOUSE_L then
        local mouse_x, mouse_y = self:getMousePos()
        if mouse_x and mouse_y then

            local clicks_count = self:triggerMultiLeftClick(
                mouse_x + 1,
                mouse_y + 1
            )
            if clicks_count == 3 then
                self:setSelection(
                    self:lineStartOffset(),
                    self:lineEndOffset()
                )
            elseif clicks_count == 2 then
                local cursor_char = self:charAtCursor()

                local is_white_space = (
                    cursor_char == ' ' or cursor_char == NEWLINE
                )

                local from, to
                if is_white_space then
                    from, to = self:currentSpacesRange()
                else
                    from, to = self:currentWordRange()
                end

                self:setSelection(from, to)
            else
                self:setCursor(self.wrapped_text:coordsToIndex(
                    mouse_x + 1,
                    mouse_y + 1
                ))
            end

            return true
        end

    elseif keys._MOUSE_L_DOWN then

        local mouse_x, mouse_y = self:getMousePos()

        if mouse_x and mouse_y then
            if (self:getMultiLeftClick(mouse_x + 1, mouse_y + 1) > 1) then
                return true
            end

            local offset = self.wrapped_text:coordsToIndex(
                mouse_x + 1,
                mouse_y + 1
            )

            if self.cursor ~= offset then
                self:setSelection(self.cursor, offset)
            else
                self.sel_end = nil
            end

            return true
        end
    end
end

function TextAreaContent:onCursorInput(keys)
    if keys.KEYBOARD_CURSOR_LEFT then
        if self:hasSelection() then
            self:setCursor(math.min(self.cursor, self.sel_end))
        else
            self:setCursor(self.cursor - 1)
        end

        return true
    elseif keys.KEYBOARD_CURSOR_RIGHT then
        if self:hasSelection() then
            self:setCursor(math.max(self.cursor, self.sel_end))
        else
            self:setCursor(self.cursor + 1)
        end

        return true
    elseif keys.KEYBOARD_CURSOR_UP and not self.one_line_mode then
        local x, y = self.wrapped_text:indexToCoords(self.cursor)
        local last_cursor_x = self.last_cursor_x or x
        local offset = y > 1 and
            self.wrapped_text:coordsToIndex(last_cursor_x, y - 1) or
            1
        self:setCursor(offset)
        self.last_cursor_x = last_cursor_x
        return true
    elseif keys.KEYBOARD_CURSOR_DOWN and not self.one_line_mode then
        local x, y = self.wrapped_text:indexToCoords(self.cursor)
        local last_cursor_x = self.last_cursor_x or x
        local offset = y < #self.wrapped_text.lines and
            self.wrapped_text:coordsToIndex(last_cursor_x, y + 1) or
            #self.text + 1
        self:setCursor(offset)
        self.last_cursor_x = last_cursor_x
        return true
    elseif keys.CUSTOM_CTRL_HOME then
        self:setCursor(1)
        return true
    elseif keys.CUSTOM_CTRL_END then
        -- go to text end
        self:setCursor(#self.text + 1)
        return true
    elseif keys.CUSTOM_CTRL_LEFT then
        -- back one word
        local word_start = self:wordStartOffset()
        self:setCursor(word_start)
        return true
    elseif keys.CUSTOM_CTRL_RIGHT then
        -- forward one word
        local word_end = self:wordEndOffset()
        self:setCursor(word_end)
        return true
    elseif keys.CUSTOM_HOME then
        -- line start
        self:setCursor(
            self:lineStartOffset()
        )
        return true
    elseif keys.CUSTOM_END then
        -- line end
        self:setCursor(
            self:lineEndOffset()
        )
        return true
    end
end

function TextAreaContent:onTextManipulationInput(keys)
    if keys.SELECT then
        -- handle enter
        self.history:store(
            HISTORY_ENTRY.WHITESPACE_BLOCK,
            self.text,
            self.cursor
        )

        if not self.one_line_mode then
            self:insert(NEWLINE)
        end

        return true

    elseif keys._STRING then
        if keys._STRING == 0 then
            -- handle backspace
            self.history:store(HISTORY_ENTRY.BACKSPACE, self.text, self.cursor)

            if (self:hasSelection()) then
                self:eraseSelection()
            else
                if (self.cursor == 1) then
                    return true
                end

                self:setSelection(
                    self.cursor - 1,
                    self.cursor - 1
                )
                self:eraseSelection()
            end

        else
            local cv = string.char(keys._STRING)

            if (self:hasSelection()) then
                self.history:store(HISTORY_ENTRY.OTHER, self.text, self.cursor)
                self:eraseSelection()
            else
                local entry_type = cv == ' ' and HISTORY_ENTRY.WHITESPACE_BLOCK
                    or HISTORY_ENTRY.TEXT_BLOCK
                self.history:store(entry_type, self.text, self.cursor)
            end

            self:insert(cv)
        end

        return true
    elseif keys.CUSTOM_CTRL_A then
        -- select all
        self:setSelection(#self.text + 1, 1)
        return true
    elseif keys.CUSTOM_CTRL_U then
        -- delete current line
        self.history:store(HISTORY_ENTRY.OTHER, self.text, self.cursor)

        if (self:hasSelection()) then
            -- delete all lines that has selection
            self:setSelection(
                self:lineStartOffset(self.cursor),
                self:lineEndOffset(self.sel_end)
            )
            self:eraseSelection()
        else
            self:setSelection(
                self:lineStartOffset(),
                self:lineEndOffset()
            )
            self:eraseSelection()
        end

        return true
    elseif keys.CUSTOM_CTRL_K then
        -- delete from cursor to end of current line
        self.history:store(HISTORY_ENTRY.OTHER, self.text, self.cursor)

        local line_end = self:lineEndOffset(self.sel_end or self.cursor) - 1
        self:setSelection(
            self.cursor,
            math.max(line_end, self.cursor)
        )
        self:eraseSelection()

        return true
    elseif keys.CUSTOM_DELETE then
        self.history:store(HISTORY_ENTRY.DELETE, self.text, self.cursor)

        if (self:hasSelection()) then
            self:eraseSelection()
        else
            self:setText(
                self.text:sub(1, self.cursor - 1) ..
                self.text:sub(self.cursor + 1)
            )
        end

        return true
    elseif keys.CUSTOM_CTRL_W then
        -- delete one word backward
        self.history:store(HISTORY_ENTRY.OTHER, self.text, self.cursor)

        if not self:hasSelection() and self.cursor ~= 1 then
            self:setSelection(
                self:wordStartOffset(),
                math.max(self.cursor - 1, 1)
            )
        end
        self:eraseSelection()

        return true
    end
end

return TextAreaContent
