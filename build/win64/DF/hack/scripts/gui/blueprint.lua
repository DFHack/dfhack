-- A GUI front-end for the blueprint plugin
--@ module = true

local plugin = require('plugins.blueprint')
local dialogs = require('gui.dialogs')
local gui = require('gui')
local guidm = require('gui.dwarfmode')
local utils = require('utils')
local widgets = require('gui.widgets')

local function get_dims(pos1, pos2)
    local width, height, depth = math.abs(pos1.x - pos2.x) + 1,
            math.abs(pos1.y - pos2.y) + 1,
            math.abs(pos1.z - pos2.z) + 1
    return width, height, depth
end

ActionPanel = defclass(ActionPanel, widgets.ResizingPanel)
ActionPanel.ATTRS{
    get_mark_fn=DEFAULT_NIL,
    is_setting_start_pos_fn=DEFAULT_NIL,
    autoarrange_subviews=true,
}
function ActionPanel:init()
    self:addviews{
        widgets.WrappedLabel{
            view_id='action_label',
            text_to_wrap=self:callback('get_action_text')},
        widgets.TooltipLabel{
            view_id='selected_area',
            indent=1,
            text={{text=self:callback('get_area_text')}},
            show_tooltip=self.get_mark_fn}}
end
function ActionPanel:get_action_text()
    local text = 'Select the '
    if self.is_setting_start_pos_fn() then
        text = text .. 'playback start'
    elseif self.get_mark_fn() then
        text = text .. 'second corner'
    else
        text = text .. 'first corner'
    end
    return text .. ' with the mouse.'
end
function ActionPanel:get_area_text()
    local mark = self.get_mark_fn()
    if not mark then return '' end
    local other = dfhack.gui.getMousePos()
            or {x=mark.x, y=mark.y, z=df.global.window_z}
    local width, height, depth = get_dims(mark, other)
    local tiles = width * height * depth
    local plural = tiles > 1 and 's' or ''
    return ('%dx%dx%d (%d tile%s)'):format(width, height, depth, tiles, plural)
end

NamePanel = defclass(NamePanel, widgets.ResizingPanel)
NamePanel.ATTRS{
    name=DEFAULT_NIL,
    autoarrange_subviews=true,
    on_layout_change=DEFAULT_NIL,
}
function NamePanel:init()
    self:addviews{
        widgets.EditField{
            view_id='name',
            key='CUSTOM_N',
            text=self.name,
            on_change=self:callback('update_tooltip'),
            on_focus=self:callback('on_edit_focus'),
            on_unfocus=self:callback('on_edit_unfocus'),
        },
        widgets.TooltipLabel{
            view_id='name_help',
            text_to_wrap=self:callback('get_name_help'),
            text_dpen=COLOR_RED,
            disabled=function() return self.has_name_collision end,
            show_tooltip=true,
        },
    }

    self:detect_name_collision()
end
function NamePanel:on_edit_focus()
    local name_view = self.subviews.name
    if name_view.text == 'blueprint' then
        name_view:setText('')
        self:update_tooltip()
    end
end
function NamePanel:on_edit_unfocus()
    local name_view = self.subviews.name
    if name_view.text == '' then
        name_view:setText('blueprint')
        self:update_tooltip()
    end
end
function NamePanel:detect_name_collision()
    -- don't let base names start with a slash - it would get ignored by
    -- the blueprint plugin later anyway
    local name_view = self.subviews.name
    local name = utils.normalizePath(name_view.text):gsub('^/','')
    if name ~= name_view.text then
        name_view:setText(name, name_view.cursor-1)
    end

    if name == '' then
        self.has_name_collision = false
        return
    end

    local suffix_pos = #name + 1

    local paths = dfhack.filesystem.listdir_recursive('dfhack-config/blueprints', nil, false)
    if not paths then return false end
    for _,v in ipairs(paths) do
        if (v.isdir and v.path..'/' == name) or
                (v.path:startswith(name) and
                 v.path:sub(suffix_pos,suffix_pos):find('[.-]')) then
            self.has_name_collision = true
            return
        end
    end
    self.has_name_collision = false
end
function NamePanel:update_tooltip()
    local prev_val = self.has_name_collision
    self:detect_name_collision()
    if self.has_name_collision ~= prev_val then
        self.on_layout_change()
    end
end
function NamePanel:get_name_help()
    if self.has_name_collision then
        return 'Warning: may overwrite existing files.'
    end
    return 'Set base name for the generated blueprint files.'
end

PhasesPanel = defclass(PhasesPanel, widgets.ResizingPanel)
PhasesPanel.ATTRS{
    phases=DEFAULT_NIL,
    on_layout_change=DEFAULT_NIL,
    autoarrange_subviews=true,
}
function PhasesPanel:init()
    self:addviews{
        widgets.CycleHotkeyLabel{
            view_id='phases',
            key='CUSTOM_SHIFT_P',
            label='phases',
            options={{label='Autodetect', value='Autodetect', pen=COLOR_GREEN},
                     'Custom'},
            initial_option=self.phases.auto_phase and 'Autodetect' or 'Custom',
            on_change=function() self.on_layout_change() end,
        },
        widgets.HotkeyLabel{
            view_id='toggle_all',
            key='CUSTOM_SHIFT_A',
            label='toggle all',
            on_activate=self:callback('toggle_all'),
        },
        -- we need an explicit spacer since the subviews are autoarranged
        widgets.Panel{
            frame={h=1}},
        widgets.Panel{
            frame={h=1},
            subviews={widgets.ToggleHotkeyLabel{view_id='dig_phase',
                        frame={t=0, l=0, w=19}, key='CUSTOM_D', label='dig',
                        initial_option=self:get_default('dig'), label_width=9},
                      widgets.ToggleHotkeyLabel{view_id='carve_phase',
                        frame={t=0, l=19, w=19}, key='CUSTOM_SHIFT_D', label='carve',
                        initial_option=self:get_default('carve')},
                    }},
        widgets.Panel{
            frame={h=1},
            subviews={widgets.ToggleHotkeyLabel{view_id='construct_phase',
                        frame={t=0, l=0, w=19}, key='CUSTOM_SHIFT_B',
                        label='construct',
                        initial_option=self:get_default('construct')},
                      widgets.ToggleHotkeyLabel{view_id='build_phase',
                        frame={t=0, l=19, w=19}, key='CUSTOM_B', label='build',
                        initial_option=self:get_default('build')}}},
        widgets.Panel{frame={h=1},
            subviews={widgets.ToggleHotkeyLabel{view_id='place_phase',
                        frame={t=0, l=0, w=19},
                        key='CUSTOM_P', label='place',
                        initial_option=self:get_default('place')},
--                     widgets.ToggleHotkeyLabel{view_id='zone_phase',
--                         frame={t=0, l=15, w=19},
--                         key='CUSTOM_Z', label='zone',
--                         initial_option=self:get_default('zone'),
--                         label_width=5}
                    }},
--         widgets.Panel{frame={h=1},
--             subviews={widgets.ToggleHotkeyLabel{view_id='query_phase',
--                         frame={t=0, l=0, w=19},
--                         key='CUSTOM_Q', label='query',
--                         initial_option=self:get_default('query')},
--                     widgets.ToggleHotkeyLabel{view_id='rooms_phase',
--                         frame={t=0, l=15, w=19},
--                         key='CUSTOM_SHIFT_Q', label='rooms',
--                         initial_option=self:get_default('rooms')}
--                     }},
        widgets.TooltipLabel{
            text_to_wrap='Select blueprint phases to export.',
            show_tooltip=true},
    }
end
function PhasesPanel:get_default(label)
    return self.phases.auto_phase or not not self.phases[label]
end
function PhasesPanel:toggle_all()
    local dig_phase = self.subviews.dig_phase
    dig_phase:cycle()
    local target_state = dig_phase.option_idx
    for _,subview in pairs(self.subviews) do
        if subview.options and subview.view_id:endswith('_phase') then
            subview.option_idx = target_state
        end
    end
end
function PhasesPanel:preUpdateLayout()
    local is_custom = self.subviews.phases.option_idx > 1
    for _,subview in ipairs(self.subviews) do
        if subview.view_id ~= 'phases' then
            subview.visible = is_custom
        end
    end
end

StartPosPanel = defclass(StartPosPanel, widgets.ResizingPanel)
StartPosPanel.ATTRS{
    start_pos=DEFAULT_NIL,
    start_comment=DEFAULT_NIL,
    on_setting_fn=DEFAULT_NIL,
    on_layout_change=DEFAULT_NIL,
    autoarrange_subviews = true,
}
function StartPosPanel:init()
    self:addviews{
        widgets.CycleHotkeyLabel{
            view_id='startpos',
            key='CUSTOM_P',
            label='playback start',
            options={'Unset', 'Setting', 'Set'},
            initial_option=self.start_pos and 'Set' or 'Unset',
            on_change=self:callback('on_change'),
        },
        widgets.TooltipLabel{
            text_to_wrap=self:callback('get_comment'),
            show_tooltip=function() return not not self.start_pos end,
            indent=1,
            text_pen=COLOR_WHITE,
        },
        widgets.TooltipLabel{
            text_to_wrap='Choose where the cursor should be positioned when ' ..
                    'replaying the blueprints.',
            show_tooltip=true,
        },
    }
end
function StartPosPanel:get_comment()
    return ('Comment: %s'):format(self.start_comment or '')
end
function StartPosPanel:on_change()
    local option = self.subviews.startpos:getOptionLabel()
    if option == 'Unset' then
        self.start_pos = nil
    elseif option == 'Setting' then
        self.on_setting_fn()
    elseif option == 'Set' then
        -- keep reference to _input_box so it is available to tests
        self._input_box = dialogs.InputBox{
            text={'Please enter a comment for the start position\n',
                  '\n',
                  'Example: "on central stairs"\n'},
            on_input=function(comment) self.start_comment = comment end,
            on_close=function() self.on_layout_change() end,
        }
        if self.start_comment then
            self._input_box.subviews.edit.text = self.start_comment
        end
        self._input_box:show()
    end
    self.on_layout_change()
end

--
-- Blueprint
--

Blueprint = defclass(Blueprint, widgets.Window)
Blueprint.ATTRS {
    frame_title='Blueprint',
    frame={w=47, h=40, r=2, t=18},
    resizable=true,
    resize_min={h=10},
    autoarrange_subviews=true,
    autoarrange_gap=1,
    presets=DEFAULT_NIL,
}

function Blueprint:preinit(info)
    if not info.presets then
        local presets = {}
        plugin.parse_gui_commandline(presets, {})
        info.presets = presets
    end
end

function Blueprint:init()
    self:addviews{
        ActionPanel{
            get_mark_fn=function() return self.mark end,
            is_setting_start_pos_fn=self:callback('is_setting_start_pos')},
        NamePanel{
            name=self.presets.name,
            on_layout_change=self:callback('updateLayout')},
        PhasesPanel{
            view_id='phases_panel',
            phases=self.presets,
            on_layout_change=self:callback('updateLayout')},
        widgets.ResizingPanel{autoarrange_subviews=true, subviews={
            widgets.ToggleHotkeyLabel{
                view_id='engrave',
                key='CUSTOM_SHIFT_E',
                label='engrave',
                options={{label='On', value=true}, {label='Off', value=false}},
                initial_option=not not self.presets.engrave},
            widgets.TooltipLabel{
                    text_to_wrap='Capture engravings.',
                    show_tooltip=true}}},
        widgets.ResizingPanel{autoarrange_subviews=true, subviews={
            widgets.ToggleHotkeyLabel{
                view_id='smooth',
                key='CUSTOM_SHIFT_S',
                label='smooth',
                options={{label='On', value=true}, {label='Off', value=false}},
                initial_option=not not self.presets.smooth},
            widgets.TooltipLabel{
                text_to_wrap='Capture smoothed tiles.',
                show_tooltip=true}}},
        widgets.ResizingPanel{autoarrange_subviews=true, subviews={
            widgets.CycleHotkeyLabel{
                view_id='format',
                key='CUSTOM_F',
                label='format',
                options={{label='Minimal text .csv', value='minimal', pen=COLOR_GREEN},
                        {label='Pretty text .csv', value='pretty'}},
                initial_option=self.presets.format},
            widgets.TooltipLabel{
                text_to_wrap='File output format.',
                show_tooltip=true}}},
        StartPosPanel{
            view_id='startpos_panel',
            start_pos=self.presets.start_pos,
            start_comment=self.presets.start_comment,
            on_setting_fn=self:callback('save_cursor_pos'),
            on_layout_change=self:callback('updateLayout')},
        widgets.ResizingPanel{autoarrange_subviews=true, subviews={
            widgets.ToggleHotkeyLabel{
                view_id='meta',
                key='CUSTOM_M',
                label='meta',
                initial_option=not self.presets.nometa},
            widgets.TooltipLabel{
                text_to_wrap='Combine blueprints that can be replayed together.',
                show_tooltip=true}}},
        widgets.ResizingPanel{autoarrange_subviews=true, subviews={
            widgets.CycleHotkeyLabel{
                view_id='splitby',
                key='CUSTOM_T',
                label='split',
                options={{label='No', value='none', pen=COLOR_GREEN},
                            {label='By group', value='group'},
                            {label='By phase', value='phase'}},
                initial_option=self.presets.split_strategy},
            widgets.TooltipLabel{
                text_to_wrap='Split blueprints into multiple files.',
                show_tooltip=true}}},
    }
end

function Blueprint:onShow()
    Blueprint.super.onShow(self)
    local start = self.presets.start
    if not start or not dfhack.maps.isValidTilePos(start) then
        return
    end
    guidm.setCursorPos(start)
    dfhack.gui.revealInDwarfmodeMap(start, true)
    self:on_mark(start)
end

function Blueprint:save_cursor_pos()
    self.saved_cursor = copyall(df.global.cursor)
end

function Blueprint:is_setting_start_pos()
    return self.subviews.startpos:getOptionLabel() == 'Setting'
end

function Blueprint:on_mark(pos)
    self.mark = pos
    self:updateLayout()
end

function Blueprint:get_bounds(start_pos)
    local cur = self.saved_cursor or dfhack.gui.getMousePos()
    if not cur then return end
    local mark = self.mark or cur
    start_pos = start_pos or self.subviews.startpos_panel.start_pos or mark

    return {
        x1=math.min(cur.x, mark.x, start_pos.x),
        x2=math.max(cur.x, mark.x, start_pos.x),
        y1=math.min(cur.y, mark.y, start_pos.y),
        y2=math.max(cur.y, mark.y, start_pos.y),
        z1=math.min(cur.z, mark.z),
        z2=math.max(cur.z, mark.z)
    }
end

local to_pen = dfhack.pen.parse
local START_PEN = to_pen{ch='X', fg=COLOR_BLUE,
                         tile=dfhack.screen.findGraphicsTile('CURSORS', 5, 22)}
local BOX_PEN = to_pen{ch='X', fg=COLOR_GREEN,
                       tile=dfhack.screen.findGraphicsTile('CURSORS', 0, 0)}

function Blueprint:onRenderFrame(dc, rect)
    Blueprint.super.onRenderFrame(self, dc, rect)

    if not dfhack.screen.inGraphicsMode() and not gui.blink_visible(500) then
        return
    end

    local start_pos = self.subviews.startpos_panel.start_pos
    if self:is_setting_start_pos() then
        start_pos = dfhack.gui.getMousePos()
    end
    if not self.mark and not start_pos then return end

    local bounds = self:get_bounds(start_pos)

    local function get_overlay_pen(pos)
        -- always render start_pos tile, even if it would overwrite the cursor
        if same_xy(start_pos, pos) then return START_PEN end
        if self.mark then
            return BOX_PEN
        end
    end

    if bounds then
        guidm.renderMapOverlay(get_overlay_pen, bounds)
    end
end

function Blueprint:onInput(keys)
    if Blueprint.super.onInput(self, keys) then return true end

    if keys.LEAVESCREEN or keys._MOUSE_R_DOWN then
        if self:is_setting_start_pos() then
            self.subviews.startpos.option_idx = 1
            self.saved_cursor = nil
            self:updateLayout()
        elseif self.mark then
            self.mark = nil
            self:updateLayout()
        else
            self.parent_view:dismiss()
        end
        return true
    end

    local pos = nil
    if keys._MOUSE_L_DOWN and not self:getMouseFramePos() then
        pos = dfhack.gui.getMousePos()
        if pos then
            guidm.setCursorPos(pos)
        end
    elseif keys.SELECT then
        pos = guidm.getCursorPos()
    end

    if pos then
        if self:is_setting_start_pos() then
            self.subviews.startpos_panel.start_pos = pos
            self.subviews.startpos:cycle()
            guidm.setCursorPos(self.saved_cursor)
            self.saved_cursor = nil
        elseif self.mark then
            self.saved_cursor = pos
            self:commit(pos)
        else
            self:on_mark(pos)
        end
        return true
    end
end

-- assemble and execute the blueprint commandline
function Blueprint:commit(pos)
    local mark = self.mark
    local width, height, depth = get_dims(mark, pos)
    if depth > 1 then
        -- when there are multiple levels, process them top to bottom
        depth = -depth
    end

    local name = self.subviews.name.text
    local params = {tostring(width), tostring(height), tostring(depth), name}

    local phases_view = self.subviews.phases
    if phases_view:getOptionValue() == 'Custom' then
        local some_phase_is_set = false
        for _,sv in pairs(self.subviews.phases_panel.subviews) do
            if sv.options and sv:getOptionLabel() == 'On' then
                table.insert(params, sv.label)
                some_phase_is_set = true
            end
        end
        if not some_phase_is_set then
            dialogs.MessageBox{
                frame_title='Error',
                text='Ensure at least one phase is enabled or enable autodetect'
            }:show()
            return
        end
    end

    -- set cursor to top left corner of the *uppermost* z-level
    local bounds = self:get_bounds()
    if not bounds then
        dialogs.MessageBox{
            frame_title='Error',
            text='Ensure blueprint bounds are set'
        }:show()
        return
    end

    table.insert(params, ('--cursor=%d,%d,%d')
                         :format(bounds.x1, bounds.y1, bounds.z2))

    if self.subviews.engrave:getOptionValue() then
        table.insert(params, '--engrave')
    end

    if self.subviews.smooth:getOptionValue() then
        table.insert(params, '--smooth')
    end

    local format = self.subviews.format:getOptionValue()
    if format ~= 'minimal' then
        table.insert(params, ('--format=%s'):format(format))
    end

    local start_pos = self.subviews.startpos_panel.start_pos
    if start_pos then
        local playback_start_x = start_pos.x - bounds.x1 + 1
        local playback_start_y = start_pos.y - bounds.y1 + 1
        local start_pos_param = ('--playback-start=%d,%d')
                                :format(playback_start_x, playback_start_y)
        local start_comment = self.subviews.startpos_panel.start_comment
        if start_comment and #start_comment > 0 then
            start_pos_param = start_pos_param .. (',%s'):format(start_comment)
        end
        table.insert(params, start_pos_param)
    end

    local meta = self.subviews.meta:getOptionValue()
    if not meta then
        table.insert(params, ('--nometa'))
    end

    local splitby = self.subviews.splitby:getOptionValue()
    if splitby ~= 'none' then
        table.insert(params, ('--splitby=%s'):format(splitby))
    end

    print('running: blueprint ' .. table.concat(params, ' '))
    local files = plugin.run(table.unpack(params))

    local text = 'No files generated (see console for any error output)'
    if files and #files > 0 then
        text = 'Generated blueprint file(s):\n'
        for _,fname in ipairs(files) do
            text = text .. ('  %s\n'):format(fname)
        end
    end

    dialogs.MessageBox{
        frame_title='Blueprint completed',
        text=text,
        on_close=function() self.parent_view:dismiss() end,
    }:show()
end

--
-- BlueprintScreen
--

BlueprintScreen = defclass(BlueprintScreen, gui.ZScreen)
BlueprintScreen.ATTRS {
    focus_path='blueprint',
    pass_movement_keys=true,
    pass_mouse_clicks=false,
    presets=DEFAULT_NIL,
}

function BlueprintScreen:init()
    self:addviews{Blueprint{presets=self.presets}}
end

function BlueprintScreen:onDismiss()
    view = nil
end

if dfhack_flags.module then
    return
end

if not dfhack.isMapLoaded() then
    qerror('This script requires a fortress map to be loaded')
end

local options, args = {}, {...}
local ok, err = dfhack.pcall(plugin.parse_gui_commandline, options, args)
if not ok then
    dfhack.printerr(tostring(err))
    options.help = true
end

if options.help then
    print(dfhack.script_help())
    return
end

view = view and view:raise() or BlueprintScreen{presets=options}:show()
