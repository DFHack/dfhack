local dialogs = require('gui.dialogs')
local gui = require('gui')
local helpdb = require('helpdb')
local overlay = require('plugins.overlay')
local repeatUtil = require('repeat-util')
local utils = require('utils')
local widgets = require('gui.widgets')

-- init files
local SYSTEM_INIT_FILE = 'dfhack-config/init/dfhack.control-panel-system.init'
local PREFERENCES_INIT_FILE = 'dfhack-config/init/dfhack.control-panel-preferences.init'
local AUTOSTART_FILE = 'dfhack-config/init/onMapLoad.control-panel-new-fort.init'
local REPEATS_FILE = 'dfhack-config/init/onMapLoad.control-panel-repeats.init'

-- service and command lists
local FORT_SERVICES = {
    'autobutcher',
    'autochop',
    'autoclothing',
    'autofarm',
    'autofish',
    'autolabor',
    'autonestbox',
    'autoslab',
    'dwarfvet',
    'emigration',
    'fastdwarf',
    'fix/protect-nicks',
    'hermit',
    'misery',
    'nestboxes',
    'prioritize',
    'seedwatch',
    'starvingdead',
    'suspendmanager',
    'tailor',
}

local FORT_AUTOSTART = {
    'autobutcher target 10 10 14 2 BIRD_GOOSE',
    'autobutcher target 10 10 14 2 BIRD_TURKEY',
    'autobutcher target 10 10 14 2 BIRD_CHICKEN',
    'autofarm threshold 150 grass_tail_pig',
    'ban-cooking all',
    'buildingplan set boulders false',
    'buildingplan set logs false',
    'fix/blood-del fort',
    'light-aquifers-only fort',
}
for _,v in ipairs(FORT_SERVICES) do
    table.insert(FORT_AUTOSTART, v)
end
table.sort(FORT_AUTOSTART)

-- these are re-enabled by the default DFHack init scripts
local SYSTEM_SERVICES = {
    'buildingplan',
    'confirm',
    'logistics',
    'overlay',
}
-- these are fully controlled by the user
local SYSTEM_USER_SERVICES = {
    'faststart',
    'work-now',
}
for _,v in ipairs(SYSTEM_USER_SERVICES) do
    table.insert(SYSTEM_SERVICES, v)
end
table.sort(SYSTEM_SERVICES)

local PREFERENCES = {
    ['dfhack']={
        HIDE_CONSOLE_ON_STARTUP={label='Hide console on startup', type='bool', default=true,
         desc='Hide the external DFHack terminal window on startup. Use the "show" command to unhide it.'},
        HIDE_ARMOK_TOOLS={label='Hide "armok" tools in command lists', type='bool', default=false,
         desc='Don\'t show tools that give you god-like powers wherever DFHack tools are listed.'},
    },
    ['gui']={
        DEFAULT_INITIAL_PAUSE={label='DFHack tools autopause game', type='bool', default=true,
         desc='Whether to pause the game when a DFHack tool window is shown.'},
    },
    ['gui.widgets']={
        DOUBLE_CLICK_MS={label='Mouse double click speed (ms)', type='int', default=500, min=50,
         desc='How long to wait for the second click of a double click, in ms.'},
        SCROLL_INITIAL_DELAY_MS={label='Mouse initial scroll repeat delay (ms)', type='int', default=300, min=5,
         desc='The delay before scrolling quickly when holding the mouse button down on a scrollbar, in ms.'},
        SCROLL_DELAY_MS={label='Mouse scroll repeat delay (ms)', type='int', default=20, min=5,
         desc='The delay between events when holding the mouse button down on a scrollbar, in ms.'},
        FILTER_FULL_TEXT={label='DFHack list filters search full text', type='bool', default=false,
         desc='Whether to search for a match in the full text (true) or just at the start of words (false).'},
    },
}
local CPP_PREFERENCES = {
    {
        label='Prevent duplicate key events',
        type='bool',
        default=true,
        desc='Whether to pass key events through to DF when DFHack keybindings are triggered.',
        init_fmt=':lua dfhack.internal.setSuppressDuplicateKeyboardEvents(%s)',
        get_fn=dfhack.internal.getSuppressDuplicateKeyboardEvents,
        set_fn=dfhack.internal.setSuppressDuplicateKeyboardEvents,
    },
}

local REPEATS = {
    ['autoMilkCreature']={
        desc='Automatically milk creatures that are ready for milking.',
        command={'--time', '14', '--timeUnits', 'days', '--command', '[', 'workorder', '"{\\"job\\":\\"MilkCreature\\",\\"item_conditions\\":[{\\"condition\\":\\"AtLeast\\",\\"value\\":2,\\"flags\\":[\\"empty\\"],\\"item_type\\":\\"BUCKET\\"}]}"', ']'}},
    ['autoShearCreature']={
        desc='Automatically shear creatures that are ready for shearing.',
        command={'--time', '14', '--timeUnits', 'days', '--command', '[', 'workorder', 'ShearCreature', ']'}},
    ['cleanowned']={
        desc='Encourage dwarves to drop tattered clothing and grab new ones.',
        command={'--time', '1', '--timeUnits', 'months', '--command', '[', 'cleanowned', 'X', ']'}},
    ['combine']={
        desc='Combine partial stacks in stockpiles into full stacks.',
        command={'--time', '7', '--timeUnits', 'days', '--command', '[', 'combine', 'all', '-q', ']'}},
    ['stuck-instruments']={
        desc='Fix activity references on stuck instruments to make them usable again.',
        command={'--time', '1', '--timeUnits', 'days', '--command', '[', 'fix/stuck-instruments', ']'}},
    ['general-strike']={
        desc='Prevent dwarves from getting stuck and refusing to work.',
        command={'--time', '1', '--timeUnits', 'days', '--command', '[', 'fix/general-strike', '-q', ']'}},
    ['orders-sort']={
        desc='Sort manager orders by repeat frequency so one-time orders can be completed.',
        command={'--time', '1', '--timeUnits', 'days', '--command', '[', 'orders', 'sort', ']'}},
    ['warn-starving']={
        desc='Show a warning dialog when units are starving or dehydrated.',
        command={'--time', '10', '--timeUnits', 'days', '--command', '[', 'warn-starving', ']'}},
    ['empty-wheelbarrows']={
        desc='Empties wheelbarrows which have rocks stuck in them.',
        command={'--time', '1', '--timeUnits', 'days', '--command', '[', 'fix/empty-wheelbarrows', '-q', ']'}},
}
local REPEATS_LIST = {}
for k in pairs(REPEATS) do
    table.insert(REPEATS_LIST, k)
end
table.sort(REPEATS_LIST)

-- save_fn takes the file as a param and should call f:write() to write data
local function save_file(path, save_fn)
    local ok, f = pcall(io.open, path, 'w')
    if not ok or not f then
        dialogs.showMessage('Error',
            ('Cannot open file for writing: "%s"'):format(path))
        return
    end
    f:write('# DO NOT EDIT THIS FILE\n')
    f:write('# Please use gui/control-panel to edit this file\n\n')
    save_fn(f)
    f:close()
end

local function get_icon_pens()
    local start = dfhack.textures.getControlPanelTexposStart()
    local valid = start > 0
    start = start + 10

    local function tp(offset)
        return valid and start + offset or nil
    end

    local enabled_pen_left = dfhack.pen.parse{fg=COLOR_CYAN,
            tile=tp(0), ch=string.byte('[')}
    local enabled_pen_center = dfhack.pen.parse{fg=COLOR_LIGHTGREEN,
            tile=tp(1) or nil, ch=251} -- check
    local enabled_pen_right = dfhack.pen.parse{fg=COLOR_CYAN,
            tile=tp(2) or nil, ch=string.byte(']')}
    local disabled_pen_left = dfhack.pen.parse{fg=COLOR_CYAN,
            tile=tp(3) or nil, ch=string.byte('[')}
    local disabled_pen_center = dfhack.pen.parse{fg=COLOR_RED,
            tile=tp(4) or nil, ch=string.byte('x')}
    local disabled_pen_right = dfhack.pen.parse{fg=COLOR_CYAN,
            tile=tp(5) or nil, ch=string.byte(']')}
    local button_pen_left = dfhack.pen.parse{fg=COLOR_CYAN,
            tile=tp(6) or nil, ch=string.byte('[')}
    local button_pen_right = dfhack.pen.parse{fg=COLOR_CYAN,
            tile=tp(7) or nil, ch=string.byte(']')}
    local help_pen_center = dfhack.pen.parse{
            tile=tp(8) or nil, ch=string.byte('?')}
    local configure_pen_center = dfhack.pen.parse{
            tile=tp(9) or nil, ch=15} -- gear/masterwork symbol
    return enabled_pen_left, enabled_pen_center, enabled_pen_right,
            disabled_pen_left, disabled_pen_center, disabled_pen_right,
            button_pen_left, button_pen_right,
            help_pen_center, configure_pen_center
end
local ENABLED_PEN_LEFT, ENABLED_PEN_CENTER, ENABLED_PEN_RIGHT,
        DISABLED_PEN_LEFT, DISABLED_PEN_CENTER, DISABLED_PEN_RIGHT,
        BUTTON_PEN_LEFT, BUTTON_PEN_RIGHT,
        HELP_PEN_CENTER, CONFIGURE_PEN_CENTER = get_icon_pens()

--
-- ConfigPanel
--

ConfigPanel = defclass(ConfigPanel, widgets.Panel)
ConfigPanel.ATTRS{
    intro_text=DEFAULT_NIL,
    is_enableable=DEFAULT_NIL,
    is_configurable=DEFAULT_NIL,
    select_label='Toggle enabled',
}

function ConfigPanel:init()
    self:addviews{
        widgets.Panel{
            frame={t=0, b=7},
            autoarrange_subviews=true,
            autoarrange_gap=1,
            subviews={
                widgets.WrappedLabel{
                    frame={t=0},
                    text_to_wrap=self.intro_text,
                },
                widgets.FilteredList{
                    frame={t=5},
                    view_id='list',
                    on_select=self:callback('on_select'),
                    on_double_click=self:callback('on_submit'),
                    on_double_click2=self:callback('launch_config'),
                    row_height=2,
                },
            },
        },
        widgets.WrappedLabel{
            view_id='desc',
            frame={b=4, h=2},
            auto_height=false,
        },
        widgets.HotkeyLabel{
            frame={b=2, l=0},
            label=self.select_label,
            key='SELECT',
            enabled=self.is_enableable,
            on_activate=self:callback('on_submit')
        },
        widgets.HotkeyLabel{
            view_id='show_help_label',
            frame={b=1, l=0},
            label='Show tool help or run commands',
            key='CUSTOM_CTRL_H',
            on_activate=self:callback('show_help')
        },
        widgets.HotkeyLabel{
            view_id='launch',
            frame={b=0, l=0},
            label='Launch config UI',
            key='CUSTOM_CTRL_G',
            enabled=self.is_configurable,
            on_activate=self:callback('launch_config'),
        },
    }
end

function ConfigPanel:onInput(keys)
    local handled = ConfigPanel.super.onInput(self, keys)
    if keys._MOUSE_L_DOWN then
        local list = self.subviews.list.list
        local idx = list:getIdxUnderMouse()
        if idx then
            local x = list:getMousePos()
            if x <= 2 then
                self:on_submit()
            elseif x >= 4 and x <= 6 then
                self:show_help()
            elseif x >= 8 and x <= 10 then
                self:launch_config()
            end
        end
    end
    return handled
end

local COMMAND_REGEX = '^([%w/_-]+)'

function ConfigPanel:refresh()
    local choices = {}
    for _,choice in ipairs(self:get_choices()) do
        local command = choice.target or choice.command
        command = command:match(COMMAND_REGEX)
        local gui_config = 'gui/' .. command
        local want_gui_config = utils.getval(self.is_configurable, gui_config)
                and helpdb.is_entry(gui_config)
        local enabled = choice.enabled
        local function get_enabled_pen(enabled_pen, disabled_pen)
            if not utils.getval(self.is_enableable) then
                return gui.CLEAR_PEN
            end
            return enabled and enabled_pen or disabled_pen
        end
        local text = {
            {tile=get_enabled_pen(ENABLED_PEN_LEFT, DISABLED_PEN_LEFT)},
            {tile=get_enabled_pen(ENABLED_PEN_CENTER, DISABLED_PEN_CENTER)},
            {tile=get_enabled_pen(ENABLED_PEN_RIGHT, DISABLED_PEN_RIGHT)},
            ' ',
            {tile=BUTTON_PEN_LEFT},
            {tile=HELP_PEN_CENTER},
            {tile=BUTTON_PEN_RIGHT},
            ' ',
            {tile=want_gui_config and BUTTON_PEN_LEFT or gui.CLEAR_PEN},
            {tile=want_gui_config and CONFIGURE_PEN_CENTER or gui.CLEAR_PEN},
            {tile=want_gui_config and BUTTON_PEN_RIGHT or gui.CLEAR_PEN},
            ' ',
            choice.target,
        }
        local desc = helpdb.is_entry(command) and
                helpdb.get_entry_short_help(command) or ''
        table.insert(choices,
                {text=text, command=choice.command, target=choice.target, desc=desc,
                 search_key=choice.target, enabled=enabled,
                 gui_config=want_gui_config and gui_config})
    end
    local list = self.subviews.list
    local filter = list:getFilter()
    local selected = list:getSelected()
    list:setChoices(choices)
    list:setFilter(filter, selected)
    list.edit:setFocus(true)
end

function ConfigPanel:on_select(idx, choice)
    local desc = self.subviews.desc
    desc.text_to_wrap = choice and choice.desc or ''
    if desc.frame_body then
        desc:updateLayout()
    end
    if choice then
        self.subviews.launch.enabled = utils.getval(self.is_configurable)
                and not not choice.gui_config
    end
end

function ConfigPanel:on_submit()
    if not utils.getval(self.is_enableable) then return false end
    _,choice = self.subviews.list:getSelected()
    if not choice then return end
    local tokens = {}
    table.insert(tokens, choice.command)
    table.insert(tokens, choice.enabled and 'disable' or 'enable')
    table.insert(tokens, choice.target)
    dfhack.run_command(tokens)
    self:refresh()
end

function ConfigPanel:show_help()
    _,choice = self.subviews.list:getSelected()
    if not choice then return end
    local command = choice.target:match(COMMAND_REGEX)
    dfhack.run_command('gui/launcher', command .. ' ')
end

function ConfigPanel:launch_config()
    if not utils.getval(self.is_configurable) then return false end
    _,choice = self.subviews.list:getSelected()
    if not choice or not choice.gui_config then return end
    dfhack.run_command(choice.gui_config)
end

--
-- Services
--

Services = defclass(Services, ConfigPanel)
Services.ATTRS{
    services_list=DEFAULT_NIL,
}

function Services:get_enabled_map()
    local enabled_map = {}
    local output = dfhack.run_command_silent('enable'):split('\n+')
    for _,line in ipairs(output) do
        local _,_,command,enabled_str,extra = line:find('%s*(%S+):%s+(%S+)%s*(.*)')
        if enabled_str then
            enabled_map[command] = enabled_str == 'on'
        end
    end
    return enabled_map
end

local function get_first_word(text)
    local word = text:trim():split(' +')[1]
    if word:startswith(':') then word = word:sub(2) end
    return word
end

function Services:get_choices()
    local enabled_map = self:get_enabled_map()
    local choices = {}
    local hide_armok = dfhack.getHideArmokTools()
    for _,service in ipairs(self.services_list) do
        local entry_name = get_first_word(service)
        if not hide_armok or not helpdb.is_entry(entry_name)
                or not helpdb.get_entry_tags(entry_name).armok then
            table.insert(choices, {target=service, enabled=enabled_map[service]})
        end
    end
    return choices
end

--
-- FortServices
--

FortServices = defclass(FortServices, Services)
FortServices.ATTRS{
    is_enableable=dfhack.world.isFortressMode,
    is_configurable=function() return dfhack.world.isFortressMode() end,
    intro_text='These tools can only be enabled when you have a fort loaded,'..
                ' but once you enable them, they will stay enabled when you'..
                ' save and reload your fort. If you want them to be'..
                ' auto-enabled for new forts, please see the "Autostart" tab.',
    services_list=FORT_SERVICES,
}

--
-- FortServicesAutostart
--

FortServicesAutostart = defclass(FortServicesAutostart, Services)
FortServicesAutostart.ATTRS{
    is_enableable=true,
    is_configurable=false,
    intro_text='Tools that are enabled on this page will be auto-enabled for'..
                ' you when you start a new fort, using the default'..
                ' configuration. To see tools that are enabled right now in'..
                ' an active fort, please see the "Fort" tab.',
    services_list=FORT_AUTOSTART,
}

function FortServicesAutostart:init()
    local enabled_map = {}
    local ok, f = pcall(io.open, AUTOSTART_FILE)
    if ok and f then
        local services_set = utils.invert(FORT_AUTOSTART)
        for line in f:lines() do
            line = line:trim()
            if #line == 0 or line:startswith('#') then goto continue end
            local service = line:match('^on%-new%-fortress enable ([%S]+)$')
                    or line:match('^on%-new%-fortress (.+)')
            if service and services_set[service] then
                enabled_map[service] = true
            end
            ::continue::
        end
    end
    self.enabled_map = enabled_map
end

function FortServicesAutostart:get_enabled_map()
    return self.enabled_map
end

function FortServicesAutostart:on_submit()
    _,choice = self.subviews.list:getSelected()
    if not choice then return end
    self.enabled_map[choice.target] = not choice.enabled

    local save_fn = function(f)
        for service,enabled in pairs(self.enabled_map) do
            if enabled then
                if service:match(' ') then
                    f:write(('on-new-fortress %s\n'):format(service))
                else
                    f:write(('on-new-fortress enable %s\n'):format(service))
                end
            end
        end
    end
    save_file(AUTOSTART_FILE, save_fn)
    self:refresh()
end

--
-- SystemServices
--

local function system_service_is_configurable(gui_config)
    return gui_config ~= 'gui/automelt' or dfhack.world.isFortressMode()
end

SystemServices = defclass(SystemServices, Services)
SystemServices.ATTRS{
    title='System',
    is_enableable=true,
    is_configurable=system_service_is_configurable,
    intro_text='These are DFHack system services that are not bound to' ..
            ' a specific fort. Some of these are critical DFHack services' ..
            ' that can be manually disabled, but will re-enable themselves' ..
            ' when DF restarts.',
    services_list=SYSTEM_SERVICES,
}

function SystemServices:on_submit()
    SystemServices.super.on_submit(self)

    local enabled_map = self:get_enabled_map()
    local save_fn = function(f)
        for _,service in ipairs(SYSTEM_USER_SERVICES) do
            if enabled_map[service] then
                f:write(('enable %s\n'):format(service))
            end
        end
    end
    save_file(SYSTEM_INIT_FILE, save_fn)
end

--
-- Overlays
--

Overlays = defclass(Overlays, ConfigPanel)
Overlays.ATTRS{
    title='Overlays',
    is_enableable=true,
    is_configurable=false,
    intro_text='These are DFHack overlays that add information and'..
                ' functionality to various DF screens.',
}

function Overlays:init()
    self.subviews.launch.visible = false
    self:addviews{
        widgets.HotkeyLabel{
            frame={b=0, l=0},
            label='Launch overlay widget repositioning UI',
            key='CUSTOM_CTRL_G',
            on_activate=function() dfhack.run_script('gui/overlay') end,
        },
    }
end

function Overlays:get_choices()
    local choices = {}
    local state = overlay.get_state()
    for _,name in ipairs(state.index) do
        table.insert(choices, {command='overlay',
                               target=name,
                               enabled=state.config[name].enabled})
    end
    return choices
end

--
-- Preferences
--

IntegerInputDialog = defclass(IntegerInputDialog, widgets.Window)
IntegerInputDialog.ATTRS{
    visible=false,
    frame={w=50, h=8},
    frame_title='Edit setting',
    frame_style=gui.PANEL_FRAME,
    on_hide=DEFAULT_NIL,
}

function IntegerInputDialog:init()
    self:addviews{
        widgets.Label{
            frame={t=0, l=0},
            text={
                'Please enter a new value for ',
                {text=function() return self.id or '' end},
                NEWLINE,
                {text=self:callback('get_spec_str')},
            },
        },
        widgets.EditField{
            view_id='input_edit',
            frame={t=3, l=0},
            on_char=function(ch) return ch:match('%d') end,
        },
    }
end

function IntegerInputDialog:get_spec_str()
    if not self.spec or (not self.spec.min and not self.spec.max) then
        return ''
    end
    local strs = {}
    if self.spec.min then
        table.insert(strs, ('at least %d'):format(self.spec.min))
    end
    if self.spec.max then
        table.insert(strs, ('at most %d'):format(self.spec.max))
    end
    return ('(%s)'):format(table.concat(strs, ', '))
end

function IntegerInputDialog:show(id, spec, initial)
    self.visible = true
    self.id, self.spec = id, spec
    local edit = self.subviews.input_edit
    edit:setText(tostring(initial))
    edit:setFocus(true)
    self:updateLayout()
end

function IntegerInputDialog:hide(val)
    self.visible = false
    self.on_hide(tonumber(val))
end

function IntegerInputDialog:onInput(keys)
    if IntegerInputDialog.super.onInput(self, keys) then
        return true
    end
    if keys.SELECT then
        self:hide(self.subviews.input_edit.text)
        return true
    elseif keys.LEAVESCREEN or keys._MOUSE_R_DOWN then
        self:hide()
        return true
    end
end

Preferences = defclass(Preferences, ConfigPanel)
Preferences.ATTRS{
    title='Preferences',
    is_enableable=true,
    is_configurable=true,
    intro_text='These are the customizable DFHack system settings.',
    select_label='Edit setting',
}

function Preferences:init()
    self.subviews.show_help_label.visible = false
    self.subviews.launch.visible = false
    self:addviews{
        widgets.HotkeyLabel{
            frame={b=0, l=0},
            label='Restore defaults',
            key='CUSTOM_CTRL_G',
            on_activate=self:callback('restore_defaults')
        },
        IntegerInputDialog{
            view_id='input_dlg',
            on_hide=self:callback('set_val'),
        },
    }
end

function Preferences:onInput(keys)
    -- call grandparent's onInput since we don't want ConfigPanel's processing
    local handled = Preferences.super.super.onInput(self, keys)
    if keys._MOUSE_L_DOWN then
        local list = self.subviews.list.list
        local idx = list:getIdxUnderMouse()
        if idx then
            local x = list:getMousePos()
            if x <= 2 then
                self:on_submit()
            end
        end
    end
    return handled
end

local function make_preference_text(label, value)
    return {
        {tile=BUTTON_PEN_LEFT},
        {tile=CONFIGURE_PEN_CENTER},
        {tile=BUTTON_PEN_RIGHT},
        ' ',
        ('%s (%s)'):format(label, value),
    }
end

function Preferences:refresh()
    if self.subviews.input_dlg.visible then return end
    local choices = {}
    for ctx_name,settings in pairs(PREFERENCES) do
        local ctx_env = require(ctx_name)
        for id,spec in pairs(settings) do
            local text = make_preference_text(spec.label, ctx_env[id])
            table.insert(choices,
                {text=text, desc=spec.desc, search_key=text[#text],
                 ctx_env=ctx_env, id=id, spec=spec})
        end
    end
    for _,spec in ipairs(CPP_PREFERENCES) do
        local text = make_preference_text(spec.label, spec.get_fn())
        table.insert(choices,
            {text=text, desc=spec.desc, search_key=text[#text], spec=spec})
    end
    table.sort(choices, function(a, b) return a.spec.label < b.spec.label end)
    local list = self.subviews.list
    local filter = list:getFilter()
    local selected = list:getSelected()
    list:setChoices(choices)
    list:setFilter(filter, selected)
    list.edit:setFocus(true)
end

local function preferences_set_and_save(self, choice, val)
    if choice.spec.set_fn then
        choice.spec.set_fn(val)
    else
        choice.ctx_env[choice.id] = val
    end
    self:do_save()
    self:refresh()
end

function Preferences:on_submit()
    _,choice = self.subviews.list:getSelected()
    if not choice then return end
    local cur_val
    if choice.spec.get_fn then
        cur_val = choice.spec.get_fn()
    else
        cur_val = choice.ctx_env[choice.id]
    end
    if choice.spec.type == 'bool' then
        preferences_set_and_save(self, choice, not cur_val)
    elseif choice.spec.type == 'int' then
        self.subviews.input_dlg:show(choice.id or choice.spec.label, choice.spec, cur_val)
    end
end

function Preferences:set_val(val)
    _,choice = self.subviews.list:getSelected()
    if not choice or not val then return end
    preferences_set_and_save(self, choice, val)
end

function Preferences:do_save()
    local save_fn = function(f)
        for ctx_name,settings in pairs(PREFERENCES) do
            local ctx_env = require(ctx_name)
            for id in pairs(settings) do
                f:write((':lua require("%s").%s=%s\n'):format(
                        ctx_name, id, tostring(ctx_env[id])))
            end
        end
        for _,spec in ipairs(CPP_PREFERENCES) do
            local line = spec.init_fmt:format(spec.get_fn())
            f:write(('%s\n'):format(line))
        end
    end
    save_file(PREFERENCES_INIT_FILE, save_fn)
end

function Preferences:restore_defaults()
    for ctx_name,settings in pairs(PREFERENCES) do
        local ctx_env = require(ctx_name)
        for id,spec in pairs(settings) do
            ctx_env[id] = spec.default
        end
    end
    for _,spec in ipairs(CPP_PREFERENCES) do
        spec.set_fn(spec.default)
    end
    os.remove(PREFERENCES_INIT_FILE)
    self:refresh()
    dialogs.showMessage('Success', 'Default preferences restored.')
end

--
-- RepeatAutostart
--

RepeatAutostart = defclass(RepeatAutostart, ConfigPanel)
RepeatAutostart.ATTRS{
    title='Periodic',
    is_enableable=true,
    is_configurable=false,
    intro_text='Tools that can run periodically to fix bugs or warn you of'..
                ' dangers that are otherwise difficult to detect (like'..
                ' starving caged animals).',
}

function RepeatAutostart:init()
    self.subviews.show_help_label.visible = false
    self.subviews.launch.visible = false
    local enabled_map = {}
    local ok, f = pcall(io.open, REPEATS_FILE)
    if ok and f then
        for line in f:lines() do
            line = line:trim()
            if #line == 0 or line:startswith('#') then goto continue end
            local service = line:match('^repeat %-%-name ([%S]+)')
            if service then
                enabled_map[service] = true
            end
            ::continue::
        end
    end
    self.enabled_map = enabled_map
end

function RepeatAutostart:onInput(keys)
    -- call grandparent's onInput since we don't want ConfigPanel's processing
    local handled = RepeatAutostart.super.super.onInput(self, keys)
    if keys._MOUSE_L_DOWN then
        local list = self.subviews.list.list
        local idx = list:getIdxUnderMouse()
        if idx then
            local x = list:getMousePos()
            if x <= 2 then
                self:on_submit()
            end
        end
    end
    return handled
end

function RepeatAutostart:refresh()
    local choices = {}
    for _,name in ipairs(REPEATS_LIST) do
        local enabled = self.enabled_map[name]
        local text = {
            {tile=enabled and ENABLED_PEN_LEFT or DISABLED_PEN_LEFT},
            {tile=enabled and ENABLED_PEN_CENTER or DISABLED_PEN_CENTER},
            {tile=enabled and ENABLED_PEN_RIGHT or DISABLED_PEN_RIGHT},
            ' ',
            name,
        }
        table.insert(choices,
            {text=text, desc=REPEATS[name].desc, search_key=name,
             name=name, enabled=enabled})
    end
    local list = self.subviews.list
    local filter = list:getFilter()
    local selected = list:getSelected()
    list:setChoices(choices)
    list:setFilter(filter, selected)
    list.edit:setFocus(true)
end

function RepeatAutostart:on_submit()
    _,choice = self.subviews.list:getSelected()
    if not choice then return end
    self.enabled_map[choice.name] = not choice.enabled
    local run_commands = dfhack.isMapLoaded()

    local save_fn = function(f)
        for name,enabled in pairs(self.enabled_map) do
            if enabled then
                local command_str = ('repeat --name %s %s\n'):
                        format(name, table.concat(REPEATS[name].command, ' '))
                f:write(command_str)
                if run_commands then
                    dfhack.run_command(command_str) -- actually start it up too
                end
            elseif run_commands then
                repeatUtil.cancel(name)
            end
        end
    end
    save_file(REPEATS_FILE, save_fn)
    self:refresh()
end

--
-- ControlPanel
--

ControlPanel = defclass(ControlPanel, widgets.Window)
ControlPanel.ATTRS {
    frame_title='DFHack Control Panel',
    frame={w=61, h=36},
    resizable=true,
    resize_min={h=28},
    autoarrange_subviews=true,
    autoarrange_gap=1,
}

function ControlPanel:init()
    self:addviews{
        widgets.TabBar{
            frame={t=0},
            labels={
                'Fort',
                'Maintenance',
                'System',
                'Overlays',
                'Preferences',
                'Autostart',
            },
            on_select=self:callback('set_page'),
            get_cur_page=function() return self.subviews.pages:getSelected() end,
        },
        widgets.Pages{
            view_id='pages',
            frame={t=5, l=0, b=0, r=0},
            subviews={
                FortServices{},
                RepeatAutostart{},
                SystemServices{},
                Overlays{},
                Preferences{},
                FortServicesAutostart{},
            },
        },
    }

    self:refresh_page()
end

function ControlPanel:refresh_page()
    self.subviews.pages:getSelectedPage():refresh()
end

function ControlPanel:set_page(val)
    self.subviews.pages:setSelected(val)
    self:refresh_page()
    self:updateLayout()
end

--
-- ControlPanelScreen
--

ControlPanelScreen = defclass(ControlPanelScreen, gui.ZScreen)
ControlPanelScreen.ATTRS {
    focus_path='control-panel',
}

function ControlPanelScreen:init()
    self:addviews{ControlPanel{}}
end

function ControlPanelScreen:onDismiss()
    view = nil
end

view = view and view:raise() or ControlPanelScreen{}:show()
