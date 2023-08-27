-- A GUI front-end for quickfort
--@ module = true

-- reload changed transitive dependencies
reqscript('quickfort').refresh_scripts()

local quickfort_command = reqscript('internal/quickfort/command')
local quickfort_list = reqscript('internal/quickfort/list')
local quickfort_parse = reqscript('internal/quickfort/parse')
local quickfort_preview = reqscript('internal/quickfort/preview')
local quickfort_transform = reqscript('internal/quickfort/transform')

local dialogs = require('gui.dialogs')
local gui = require('gui')
local guidm = require('gui.dwarfmode')
local widgets = require('gui.widgets')

-- wide enough to take up most of the screen, allowing long lines to be
-- displayed without rampant wrapping, but not so large that it blots out the
-- entire DF map screen.
local dialog_width = 73

-- persist these between invocations
show_library = show_library == nil and true or show_library
show_hidden = show_hidden or false
filter_text = filter_text or ''
selected_id = selected_id or 1
repeat_dir = repeat_dir or false
repetitions = repetitions or 1
transform = transform or false
transformations = transformations or {}

--
-- BlueprintDetails
--

-- displays blueprint details, such as the full modeline and comment, that
-- otherwise might be truncated for length in the blueprint selection list
BlueprintDetails = defclass(BlueprintDetails, dialogs.MessageBox)
BlueprintDetails.ATTRS{
    focus_path='quickfort/dialog/details',
    frame_title='Details',
    frame_width=28, -- minimum width required for the bottom frame text
}

-- adds hint about left arrow being a valid "exit" key for this dialog
function BlueprintDetails:onRenderFrame(dc, rect)
    BlueprintDetails.super.onRenderFrame(self, dc, rect)
    dc:seek(rect.x1+2, rect.y2):string('Left arrow', dc.cur_key_pen):
            string(': Back', COLOR_GREY)
end

function BlueprintDetails:onInput(keys)
    if keys.KEYBOARD_CURSOR_LEFT or keys.SELECT
            or keys.LEAVESCREEN or keys._MOUSE_R_DOWN then
        self:dismiss()
    end
end

--
-- BlueprintDialog
--

-- blueprint selection dialog, shown when the script starts or when a user wants
-- to load a new blueprint into the ui
BlueprintDialog = defclass(BlueprintDialog, dialogs.ListBox)
BlueprintDialog.ATTRS{
    focus_path='quickfort/dialog',
    frame_title='Load quickfort blueprint',
    with_filter=true,
    frame_width=dialog_width,
    row_height=2,
    frame_inset={t=0,l=1,r=0,b=1},
    list_frame_inset={t=1},
}

function BlueprintDialog:init()
    self:addviews{
        widgets.Label{frame={t=0, l=1}, text='Filters:', text_pen=COLOR_GREY},
        widgets.ToggleHotkeyLabel{frame={t=0, l=12}, label='Library',
                key='CUSTOM_ALT_L', initial_option=show_library,
                text_pen=COLOR_GREY,
                on_change=self:callback('update_setting', 'show_library')},
        widgets.ToggleHotkeyLabel{frame={t=0, l=34}, label='Hidden',
                key='CUSTOM_ALT_H', initial_option=show_hidden,
                text_pen=COLOR_GREY,
                on_change=self:callback('update_setting', 'show_hidden')}
    }
end

-- always keep our list big enough to display 10 items so we don't jarringly
-- resize when the filter is being edited and it suddenly matches no blueprints
function BlueprintDialog:getWantedFrameSize()
    local mw, mh = BlueprintDialog.super.getWantedFrameSize(self)
    return mw, math.max(mh, 24)
end

function BlueprintDialog:onRenderFrame(dc, rect)
    BlueprintDialog.super.onRenderFrame(self, dc, rect)
    dc:seek(rect.x1+2, rect.y2):string('Ctrl+D', dc.cur_key_pen):
            string(': Show details', COLOR_GREY)
end

function BlueprintDialog:update_setting(setting, value)
    _ENV[setting] = value
    self:refresh()
end

-- ensures each newline-delimited sequence within text is no longer than
-- width characters long. also ensures that no more than max_lines lines are
-- returned in the truncated string.
local more_marker = '...'
local function truncate(text, width, max_lines)
    local truncated_text = {}
    for line in text:gmatch('[^'..NEWLINE..']*') do
        if #line > width then
            line = line:sub(1, width-#more_marker) .. more_marker
        end
        table.insert(truncated_text, line)
        if #truncated_text >= max_lines then break end
    end
    return table.concat(truncated_text, NEWLINE)
end

-- extracts the blueprint list id from a dialog list entry
local function get_id(text)
    local _, _, id = text:find('^(%d+)')
    return tonumber(id)
end

local function save_selection(list)
    local _, obj = list:getSelected()
    if obj then
        selected_id = get_id(obj.text)
    end
end

-- reinstate the saved selection in the list, or a nearby list id if that exact
-- item is no longer in the list
local function restore_selection(list)
    local best_idx = 1
    for idx,v in ipairs(list:getVisibleChoices()) do
        local cur_id = get_id(v.text)
        if selected_id >= cur_id then best_idx = idx end
        if selected_id <= cur_id then break end
    end
    list.list:setSelected(best_idx)
    save_selection(list)
end

-- generates a new list of unfiltered choices by calling quickfort's list
-- implementation, then applies the saved (or given) filter text
-- returns false on list error
function BlueprintDialog:refresh()
    local choices = {}
    local ok, results = pcall(quickfort_list.do_list_internal, show_library,
                              show_hidden)
    if not ok then
        self._dialog = dialogs.showMessage('Cannot list blueprints',
                tostring(results):wrap(dialog_width), COLOR_RED)
        self:dismiss()
        return false
    end
    for _,v in ipairs(results) do
        local start_comment = ''
        if v.start_comment then
            start_comment = string.format(' cursor start: %s', v.start_comment)
        end
        local sheet_spec = ''
        if v.section_name then
            sheet_spec = string.format(
                    ' -n %s',
                    quickfort_parse.quote_if_has_spaces(v.section_name))
        end
        local main = ('%d) %s%s (%s)')
                     :format(v.id, quickfort_parse.quote_if_has_spaces(v.path),
                     sheet_spec, v.mode)
        local text = ('%s%s'):format(main, start_comment)
        if v.comment then
            text = text .. ('\n    %s'):format(v.comment)
        end
        local full_text = main
        if #start_comment > 0 then
            full_text = full_text .. '\n\n' .. start_comment
        end
        if v.comment then
            full_text = full_text .. '\n\n comment: ' .. v.comment
        end
        local truncated_text =
                truncate(text, self.frame_body.width - 2, self.row_height)

        -- search for the extra syntax shown in the list items in case someone
        -- is typing exactly what they see
        table.insert(choices,
                     {text=truncated_text,
                      full_text=full_text,
                      search_key=v.search_key .. main})
    end
    self.subviews.list:setChoices(choices)
    self:updateLayout() -- allows the dialog to resize width to fit the content
    self.subviews.list:setFilter(filter_text)
    restore_selection(self.subviews.list)
    return true
end

function BlueprintDialog:onInput(keys)
    local _, obj = self.subviews.list:getSelected()
    if keys.CUSTOM_CTRL_D and obj then
        local details = BlueprintDetails{
                text=obj.full_text:wrap(self.frame_body.width)}
        details:show()
        -- for testing
        self._details = details
    elseif keys.LEAVESCREEN or keys._MOUSE_R_DOWN then
        self:dismiss()
        if self.on_cancel then
            self.on_cancel()
        end
    else
        self:inputToSubviews(keys)
        local prev_filter_text = filter_text
        -- save the filter if it was updated so we always have the most recent
        -- text for the next invocation of the dialog
        filter_text = self.subviews.list:getFilter()
        if prev_filter_text ~= filter_text then
            -- if the filter text has changed, restore the last selected item
            restore_selection(self.subviews.list)
        else
            -- otherwise, save the new selected item
            save_selection(self.subviews.list)
        end
    end
end

--
-- Quickfort
--

-- the main map screen UI. the information panel appears in a window and
-- the loaded blueprint generates a flashing shadow over tiles that will be
-- modified by the blueprint when it is applied.
Quickfort = defclass(Quickfort, widgets.Window)
Quickfort.ATTRS {
    frame_title='Quickfort',
    frame={w=34, h=30, r=2, t=18},
    resizable=true,
    resize_min={h=26},
    autoarrange_subviews=true,
    autoarrange_gap=1,
    filter='',
}
function Quickfort:init()
    self.saved_cursor = dfhack.gui.getMousePos() or {x=0, y=0, z=0}

    self:addviews{
        widgets.ResizingPanel{subviews={
            widgets.Label{
                frame={t=0, l=0, w=30},
                text_pen=COLOR_GREY,
                text={
                    {text=self:callback('get_summary_label')},
                    NEWLINE,
                    'Click or hit ',
                    {key='SELECT', key_sep=' ',
                     on_activate=self:callback('commit')},
                    'to apply.',
                },
            },
        }},
        widgets.HotkeyLabel{key='CUSTOM_L', label='Load new blueprint',
            on_activate=self:callback('show_dialog')},
        widgets.ResizingPanel{autoarrange_subviews=true, subviews={
            widgets.Label{text='Current blueprint:'},
            widgets.WrappedLabel{
                text_pen=COLOR_GREY,
                text_to_wrap=self:callback('get_blueprint_name')}
            }},
        widgets.ResizingPanel{autoarrange_subviews=true, subviews={
            widgets.Label{
                text={'Blueprint tiles: ',
                    {text=self:callback('get_total_tiles')}}},
            widgets.Label{
                text={'Invalid tiles:   ',
                    {text=self:callback('get_invalid_tiles')}},
                text_dpen=COLOR_RED,
                disabled=self:callback('has_invalid_tiles')}}},
        widgets.HotkeyLabel{key='CUSTOM_SHIFT_L',
            label=self:callback('get_lock_cursor_label'),
            active=function() return self.blueprint_name end,
            enabled=function() return self.blueprint_name end,
            on_activate=self:callback('toggle_lock_cursor')},
        widgets.ResizingPanel{autoarrange_subviews=true, subviews={
            widgets.CycleHotkeyLabel{key='CUSTOM_R',
                view_id='repeat_cycle',
                label='Repeat',
                active=function() return self.blueprint_name end,
                enabled=function() return self.blueprint_name end,
                options={{label='No', value=false},
                         {label='Down z-levels', value='>'},
                         {label='Up z-levels', value='<'}},
                initial_option=repeat_dir,
                on_change=self:callback('on_repeat_change')},
            widgets.ResizingPanel{view_id='repeat_times_panel',
                    visible=function() return repeat_dir and self.blueprint_name end,
                    subviews={
                widgets.HotkeyLabel{key='STRING_A045',
                    frame={t=1, l=2}, key_sep='',
                    on_activate=self:callback('on_adjust_repetitions', -1)},
                widgets.HotkeyLabel{key='STRING_A043',
                    frame={t=1, l=3}, key_sep='',
                    on_activate=self:callback('on_adjust_repetitions', 1)},
                widgets.HotkeyLabel{key='STRING_A047',
                    frame={t=1, l=4}, key_sep='',
                    on_activate=self:callback('on_adjust_repetitions', -10)},
                widgets.HotkeyLabel{key='STRING_A042',
                    frame={t=1, l=5}, key_sep='',
                    on_activate=self:callback('on_adjust_repetitions', 10)},
                widgets.EditField{key='CUSTOM_SHIFT_R',
                    view_id='repeat_times',
                    frame={t=1, l=7, h=1},
                    label_text='x ',
                    text=tostring(repetitions),
                    on_char=function(ch) return ch:match('%d') end,
                    on_submit=self:callback('on_repeat_times_submit')}}}}},
        widgets.ResizingPanel{autoarrange_subviews=true, subviews={
            widgets.ToggleHotkeyLabel{key='CUSTOM_T',
                view_id='transform',
                label='Transform',
                active=function() return self.blueprint_name end,
                enabled=function() return self.blueprint_name end,
                initial_option=transform,
                on_change=self:callback('on_transform_change')},
            widgets.ResizingPanel{view_id='transform_panel',
                    visible=function() return transform and self.blueprint_name end,
                    subviews={
                widgets.HotkeyLabel{key='STRING_A040',
                    frame={t=1, l=2}, key_sep='',
                    on_activate=self:callback('on_transform', 'ccw')},
                widgets.HotkeyLabel{key='STRING_A041',
                    frame={t=1, l=3}, key_sep='',
                    on_activate=self:callback('on_transform', 'cw')},
                widgets.HotkeyLabel{key='STRING_A095',
                    frame={t=1, l=4}, key_sep='',
                    on_activate=self:callback('on_transform', 'flipv')},
                widgets.HotkeyLabel{key='STRING_A061',
                    frame={t=1, l=5}, key_sep=':',
                    on_activate=self:callback('on_transform', 'fliph')},
                widgets.WrappedLabel{
                    frame={t=1, l=8},
                    text_to_wrap=function()
                            return #transformations == 0 and 'No transform'
                                or table.concat(transformations, ', ') end}}}}},
        widgets.HotkeyLabel{key='CUSTOM_O', label='Generate manager orders',
            active=function() return self.blueprint_name end,
            enabled=function() return self.blueprint_name end,
            on_activate=self:callback('do_command', 'orders')},
        widgets.HotkeyLabel{key='CUSTOM_SHIFT_O',
            label='Preview manager orders',
            active=function() return self.blueprint_name end,
            enabled=function() return self.blueprint_name end,
            on_activate=self:callback('do_command', 'orders', true)},
        widgets.HotkeyLabel{key='CUSTOM_SHIFT_U', label='Undo blueprint',
            active=function() return self.blueprint_name end,
            enabled=function() return self.blueprint_name end,
            on_activate=self:callback('do_command', 'undo')},
    }
end

function Quickfort:get_summary_label()
    if self.mode == 'config' then
        return 'Blueprint configures game, not map.'
    elseif self.mode == 'notes' then
        return 'Blueprint shows help text.'
    end
    return 'Reposition with the mouse.'
end

function Quickfort:get_blueprint_name()
    if self.blueprint_name then
        local text = {self.blueprint_name}
        if self.section_name then
            table.insert(text, '  '..self.section_name)
        end
        return text
    end
    return 'No blueprint loaded'
end

function Quickfort:get_lock_cursor_label()
    if self.cursor_locked and self.saved_cursor.z ~= df.global.window_z then
        return 'Zoom to locked position'
    end
    return (self.cursor_locked and 'Unl' or 'L') .. 'ock blueprint position'
end

function Quickfort:toggle_lock_cursor()
    if self.cursor_locked then
        local was_on_different_zlevel = self.saved_cursor.z ~= df.global.window_z
        dfhack.gui.revealInDwarfmodeMap(self.saved_cursor)
        if was_on_different_zlevel then
            return
        end
    end
    self.cursor_locked = not self.cursor_locked
end

function Quickfort:get_total_tiles()
    if not self.saved_preview then return '0' end
    return tostring(self.saved_preview.total_tiles)
end

function Quickfort:has_invalid_tiles()
    return self:get_invalid_tiles() ~= '0'
end

function Quickfort:get_invalid_tiles()
    if not self.saved_preview then return '0' end
    return tostring(self.saved_preview.invalid_tiles)
end

function Quickfort:on_repeat_change(val)
    repeat_dir = val
    self:updateLayout()
    self.dirty = true
end

function Quickfort:on_adjust_repetitions(amt)
    repetitions = math.max(1, repetitions + amt)
    self.subviews.repeat_times:setText(tostring(repetitions))
    self.dirty = true
end

function Quickfort:on_repeat_times_submit(val)
    repetitions = tonumber(val)
    if not repetitions or repetitions < 1 then
        repetitions = 1
    end
    self.subviews.repeat_times:setText(tostring(repetitions))
    self.dirty = true
end

function Quickfort:on_transform_change(val)
    transform = val
    self:updateLayout()
    self.dirty = true
end

local origin, test_point = {x=0, y=0}, {x=1, y=-2}
local minimal_sequence = {
    ['x=1, y=-2'] = {},
    ['x=2, y=-1'] = {'cw', 'flipv'},
    ['x=2, y=1'] = {'cw'},
    ['x=1, y=2'] = {'flipv'},
    ['x=-1, y=2'] = {'cw', 'cw'},
    ['x=-2, y=1'] = {'ccw', 'flipv'},
    ['x=-2, y=-1'] = {'ccw'},
    ['x=-1, y=-2'] = {'fliph'}
}

-- reduces the list of transformations to a minimal sequence
local function reduce_transform(elements)
    local pos = test_point
    for _,elem in ipairs(elements) do
        pos = quickfort_transform.make_transform_fn_from_name(elem)(pos, origin)
    end
    local ret = quickfort_transform.resolve_vector(pos, minimal_sequence)
    if #ret == #elements then
        -- if we're not making the sequence any shorter, prefer the existing set
        return elements
    end
    return copyall(ret)
end

function Quickfort:on_transform(val)
    table.insert(transformations, val)
    transformations = reduce_transform(transformations)
    self:updateLayout()
    self.dirty = true
end

function Quickfort:dialog_cb(text)
    local id = get_id(text)
    local name, sec_name, mode = quickfort_list.get_blueprint_by_number(id)
    self.blueprint_name, self.section_name, self.mode = name, sec_name, mode
    self:updateLayout()
    if self.mode == 'notes' then
        self:do_command('run', false, self:callback('show_dialog'))
    end
    self.dirty = true
end

function Quickfort:dialog_cancel_cb()
    if not self.blueprint_name then
        -- ESC was pressed on the first showing of the dialog when no blueprint
        -- has ever been loaded. the user doesn't want to be here. exit script.
        self.parent_view:dismiss()
    end
end

function Quickfort:show_dialog(initial)
    -- if this is the first showing, absorb the filter from the commandline (if
    -- one was specified)
    if initial and #self.filter > 0 then
        filter_text = self.filter
    end

    local file_dialog = BlueprintDialog{
        on_select=function(idx, obj) self:dialog_cb(obj.text) end,
        on_cancel=self:callback('dialog_cancel_cb')
    }

    if file_dialog:refresh() then
        -- autoload if this is the first showing of the dialog and a filter was
        -- specified on the commandline and the filter matches exactly one
        -- choice
        if initial and #self.filter > 0 then
            local choices = file_dialog.subviews.list:getVisibleChoices()
            if #choices == 1 then
                local selection = choices[1].text
                file_dialog:dismiss()
                self:dialog_cb(selection)
                return
            end
        end
        file_dialog:show()
    end

    -- for testing
    self._dialog = file_dialog
end

function Quickfort:run_quickfort_command(command, dry_run, preview)
    local ctx = quickfort_command.init_ctx{
        command=command,
        blueprint_name=self.blueprint_name,
        cursor=self.saved_cursor,
        aliases=quickfort_list.get_aliases(self.blueprint_name),
        quiet=true,
        dry_run=dry_run,
        preview=preview,
    }

    local section_name = self.section_name
    local modifiers = quickfort_parse.get_modifiers_defaults()

    if repeat_dir and repetitions > 1 then
        local repeat_str = repeat_dir .. tostring(repetitions)
        quickfort_parse.parse_repeat_params(repeat_str, modifiers)
    end

    if transform and #transformations > 0 then
        local transform_str = table.concat(transformations, ',')
        quickfort_parse.parse_transform_params(transform_str, modifiers)
    end

    quickfort_command.do_command_section(ctx, section_name, modifiers)

    return ctx
end

function Quickfort:refresh_preview()
    local ctx = self:run_quickfort_command('run', true, true)
    self.saved_preview = ctx.preview
end

local to_pen = dfhack.pen.parse
local CURSOR_PEN = to_pen{ch='o', fg=COLOR_BLUE,
                         tile=dfhack.screen.findGraphicsTile('CURSORS', 5, 22)}
local GOOD_PEN = to_pen{ch='x', fg=COLOR_GREEN,
                        tile=dfhack.screen.findGraphicsTile('CURSORS', 1, 2)}
local BAD_PEN = to_pen{ch='X', fg=COLOR_RED,
                       tile=dfhack.screen.findGraphicsTile('CURSORS', 3, 0)}

function Quickfort:onRenderFrame(dc, rect)
    Quickfort.super.onRenderFrame(self, dc, rect)

    if not self.blueprint_name then return end
    if not dfhack.screen.inGraphicsMode() and not gui.blink_visible(500) then
        return
    end

    -- if the (non-locked) cursor has moved since last preview processing or any
    -- settings have changed, regenerate the preview
    local cursor = dfhack.gui.getMousePos() or self.saved_cursor
    if self.dirty or not same_xyz(self.saved_cursor, cursor) then
        if not self.cursor_locked then
            self.saved_cursor = cursor
        end
        self:refresh_preview()
        self.dirty = false
    end

    local tiles = self.saved_preview.tiles
    if not tiles[cursor.z] then return end

    local function get_overlay_pen(pos)
        if same_xyz(pos, self.saved_cursor) then return CURSOR_PEN end
        local preview_tile = quickfort_preview.get_preview_tile(tiles, pos)
        if preview_tile == nil then return end
        return preview_tile and GOOD_PEN or BAD_PEN
    end

    guidm.renderMapOverlay(get_overlay_pen, self.saved_preview.bounds[cursor.z])
end

function Quickfort:onInput(keys)
    if Quickfort.super.onInput(self, keys) then
        return true
    end

    if keys._MOUSE_L_DOWN and not self:getMouseFramePos() then
        local pos = dfhack.gui.getMousePos()
        if pos then
            self:commit()
            return true
        end
    end
end

function Quickfort:commit()
    -- don't dismiss the window in case the player wants to lay down more copies
    self:do_command('run', false)
end

function Quickfort:do_command(command, dry_run, post_fn)
    self.dirty = true
    print(('executing via gui/quickfort: quickfort %s --cursor=%d,%d,%d'):format(
                quickfort_parse.format_command(
                    command, self.blueprint_name, self.section_name, dry_run),
                self.saved_cursor.x, self.saved_cursor.y, self.saved_cursor.z))
    local ctx = self:run_quickfort_command(command, dry_run, false)
    quickfort_command.finish_commands(ctx)
    if command == 'run' then
        if #ctx.messages > 0 then
            self._dialog = dialogs.showMessage(
                    'Attention',
                    table.concat(ctx.messages, '\n\n'):wrap(dialog_width),
                    nil,
                    post_fn)
        elseif post_fn then
            post_fn()
        end
    elseif command == 'orders' then
        local count = 0
        for _,_ in pairs(ctx.order_specs or {}) do count = count + 1 end
        local messages = {('%d order(s) %senqueued for\n%s.'):format(count,
                dry_run and 'would be ' or '',
                quickfort_parse.format_command(nil, self.blueprint_name,
                                               self.section_name))}
        if count > 0 then
            table.insert(messages, '')
        end
        for _,stat in pairs(ctx.stats) do
            if stat.is_order then
                table.insert(messages, ('  %s: %d'):format(stat.label,
                                                           stat.value))
            end
        end
        self._dialog = dialogs.showMessage(
               ('Orders %senqueued'):format(dry_run and 'that would be ' or ''),
               table.concat(messages,'\n'):wrap(dialog_width))
    end
end

--
-- QuickfortScreen
--

QuickfortScreen = defclass(QuickfortScreen, gui.ZScreen)
QuickfortScreen.ATTRS {
    focus_path='quickfort',
    pass_movement_keys=true,
    pass_mouse_clicks=false,
    filter=DEFAULT_NIL,
}

function QuickfortScreen:init()
    self:addviews{Quickfort{filter=self.filter}}
end

function QuickfortScreen:onShow()
    QuickfortScreen.super.onShow(self)
    self.subviews[1]:show_dialog(true)
end

function QuickfortScreen:onDismiss()
    view = nil
end

if dfhack_flags.module then
    return
end

if not dfhack.isMapLoaded() then
    qerror('This script requires a fortress map to be loaded')
end

-- treat all arguments as blueprint list dialog filter text
view = view and view:raise()
        or QuickfortScreen{filter=table.concat({...}, ' ')}:show()
