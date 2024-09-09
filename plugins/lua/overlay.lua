local _ENV = mkmodule('plugins.overlay')

local gui = require('gui')
local json = require('json')
local scriptmanager = require('script-manager')
local utils = require('utils')
local widgets = require('gui.widgets')

local OVERLAY_CONFIG_FILE = 'dfhack-config/overlay.json'
local OVERLAY_WIDGETS_VAR = 'OVERLAY_WIDGETS'
local GLOBAL_KEY = 'OVERLAY'

local DEFAULT_X_POS, DEFAULT_Y_POS = -2, -2

-- ---------------- --
-- state and config --
-- ---------------- --

local trigger_lock_holder_description = nil
local trigger_lock_holder_screen = nil -- if non-nil, no triggering allowed
local widget_db = {} -- map of widget name to ephermeral state
local widget_index = {} -- ordered list of widget names
local overlay_config = {} -- map of widget name to persisted state
local active_hotspot_widgets = {} -- map of widget names to the db entry
local active_viewscreen_widgets = {} -- map of vs_name to map of w.names -> db

function get_state()
    return {index=widget_index, config=overlay_config, db=widget_db}
end

function register_trigger_lock_screen(scr, desc)
    if trigger_lock_holder_screen then
        if not trigger_lock_holder_screen:isActive() then
            trigger_lock_holder_screen:dismiss()
        end
        trigger_lock_holder_description = nil
    end
    trigger_lock_holder_screen = scr
    if trigger_lock_holder_screen then
        trigger_lock_holder_description = desc
        return true
    end
end

local function triggered_screen_has_lock()
    if not trigger_lock_holder_screen then return false end
    if trigger_lock_holder_screen:isActive() then
        if trigger_lock_holder_screen.raise then
            trigger_lock_holder_screen:raise()
        end
        return true
    end
    return register_trigger_lock_screen(nil, nil)
end

local function reset()
    register_trigger_lock_screen(nil, nil)

    widget_db = {}
    widget_index = {}

    local ok, config = pcall(json.decode_file, OVERLAY_CONFIG_FILE)
    overlay_config = ok and config or {}

    active_hotspot_widgets = {}
    active_viewscreen_widgets = {}
end

local function save_config()
    if not safecall(json.encode_file, overlay_config, OVERLAY_CONFIG_FILE) then
        dfhack.printerr(('failed to save overlay config file: "%s"')
                :format(path))
    end
end

-- ----------- --
-- utility fns --
-- ----------- --

function normalize_list(element_or_list)
    if type(element_or_list) == 'table' then return element_or_list end
    return {element_or_list}
end

-- normalize "short form" viewscreen names to "long form" and remove any focus
local function normalize_viewscreen_name(vs_name)
    if vs_name == 'all' or vs_name:match('^viewscreen_.*st') then
        return vs_name:match('^[^/]+')
    end
    return 'viewscreen_' .. vs_name:match('^[^/]+') .. 'st'
end

-- reduce "long form" viewscreen names to "short form"; keep focus
function simplify_viewscreen_name(vs_name)
    local short_name = vs_name:match('^viewscreen_([^/]+)st')
    return short_name or vs_name
end

local function is_empty(tbl)
    for _ in pairs(tbl) do
        return false
    end
    return true
end

local function sanitize_pos(pos)
    local x = math.floor(tonumber(pos.x) or DEFAULT_X_POS)
    local y = math.floor(tonumber(pos.y) or DEFAULT_Y_POS)
    -- if someone accidentally uses 0-based instead of 1-based indexing, fix it
    if x == 0 then x = 1 end
    if y == 0 then y = 1 end
    return {x=x, y=y}
end

local function make_frame(pos, old_frame)
    old_frame = old_frame or {}
    local frame = {w=old_frame.w, h=old_frame.h}
    if pos.x < 0 then frame.r = math.abs(pos.x) - 1 else frame.l = pos.x - 1 end
    if pos.y < 0 then frame.b = math.abs(pos.y) - 1 else frame.t = pos.y - 1 end
    return frame
end

local function get_interface_rects()
    local full = gui.ViewRect{rect=gui.mkdims_wh(0, 0, dfhack.screen.getWindowSize())}
    local scaled = gui.ViewRect{rect=gui.get_interface_rect()}
    return full, scaled
end

-- ------------- --
-- CLI functions --
-- ------------- --

local function get_name(name_or_number)
    local num = tonumber(name_or_number)
    if num and widget_index[num] then
        return widget_index[num]
    end
    return tostring(name_or_number)
end

local function do_by_names_or_numbers(args, fn)
    local arglist = normalize_list(args)
    if #arglist == 0 then
        dfhack.printerr('please specify a widget name or list number')
        return
    end
    for _,name_or_number in ipairs(arglist) do
        local name = get_name(name_or_number)
        local db_entry = widget_db[name]
        if not db_entry then
            dfhack.printerr(('widget not found: "%s"'):format(name))
        else
            fn(name, db_entry)
        end
    end
end

local function do_enable(args, quiet, skip_save)
    local enable_fn = function(name, db_entry)
        overlay_config[name].enabled = true
        if db_entry.widget.hotspot then
            active_hotspot_widgets[name] = db_entry
        end
        for _,vs_name in ipairs(normalize_list(db_entry.widget.viewscreens)) do
            vs_name = normalize_viewscreen_name(vs_name)
            ensure_key(active_viewscreen_widgets, vs_name)[name] = db_entry
        end
        if not quiet then
            print(('enabled widget %s'):format(name))
        end
    end
    if args[1] == 'all' then
        for name,db_entry in pairs(widget_db) do
            if not overlay_config[name].enabled then
                enable_fn(name, db_entry)
            end
        end
    else
        do_by_names_or_numbers(args, enable_fn)
    end
    if not skip_save then
        save_config()
    end
end

local function do_disable(args, quiet)
    local disable_fn = function(name, db_entry)
        overlay_config[name].enabled = false
        if db_entry.widget.hotspot then
            active_hotspot_widgets[name] = nil
        end
        for _,vs_name in ipairs(normalize_list(db_entry.widget.viewscreens)) do
            vs_name = normalize_viewscreen_name(vs_name)
            ensure_key(active_viewscreen_widgets, vs_name)[name] = nil
            if is_empty(active_viewscreen_widgets[vs_name]) then
                active_viewscreen_widgets[vs_name] = nil
            end
        end
        if not quiet then
            print(('disabled widget %s'):format(name))
        end
    end
    if args[1] == 'all' then
        for name,db_entry in pairs(widget_db) do
            if overlay_config[name].enabled then
                disable_fn(name, db_entry)
            end
        end
    else
        do_by_names_or_numbers(args, disable_fn)
    end
    save_config()
end

local function do_list(args)
    local filter = args and #args > 0
    local num_filtered = 0
    for i,name in ipairs(widget_index) do
        if filter then
            local passes = false
            for _,str in ipairs(args) do
                if name:find(str) then
                    passes = true
                    break
                end
            end
            if not passes then
                num_filtered = num_filtered + 1
                goto continue
            end
        end
        local db_entry = widget_db[name]
        local enabled = overlay_config[name].enabled
        dfhack.color(enabled and COLOR_LIGHTGREEN or COLOR_YELLOW)
        dfhack.print(enabled and '[enabled] ' or '[disabled]')
        dfhack.color()
        print((' %d) %s'):format(i, name))
        ::continue::
    end
    if num_filtered > 0 then
        print(('(%d widgets filtered out)'):format(num_filtered))
    end
end

local function get_focus_strings(viewscreens)
    local focus_strings = nil
    for _,vs in ipairs(viewscreens) do
        if vs:match('/') then
            focus_strings = focus_strings or {}
            vs = simplify_viewscreen_name(vs)
            table.insert(focus_strings, vs)
        end
    end
    return focus_strings
end

local function load_widget(name, widget_class)
    local widget = widget_class{name=name}
    widget_db[name] = {
        widget=widget,
        focus_strings=get_focus_strings(normalize_list(widget.viewscreens)),
        next_update_ms=widget.overlay_onupdate and 0 or math.huge,
    }
    if not overlay_config[name] then overlay_config[name] = {} end
    if widget.version ~= overlay_config[name].version then
        overlay_config[name] = {}
    end
    local config = overlay_config[name]
    config.version = widget.version
    if config.enabled == nil then
        config.enabled = widget.default_enabled
    end
    config.pos = sanitize_pos(config.pos or widget.default_pos)
    widget.frame = make_frame(config.pos, widget.frame)
    if config.enabled then
        do_enable(name, true, true)
    else
        config.enabled = false
    end
end

local function load_widgets(env_name, env)
    local overlay_widgets = env[OVERLAY_WIDGETS_VAR]
    if not overlay_widgets then return end
    if type(overlay_widgets) ~= 'table' then
        dfhack.printerr(
                ('error loading overlay widgets from "%s": %s map is malformed')
                :format(env_name, OVERLAY_WIDGETS_VAR))
        return
    end
    for widget_name,widget_class in pairs(overlay_widgets) do
        local name = env_name .. '.' .. widget_name
        if not safecall(load_widget, name, widget_class) then
            dfhack.printerr(('error loading overlay widget "%s"'):format(name))
        end
    end
end

-- called directly from cpp on plugin enable
function rescan()
    reset()

    for _,plugin in ipairs(dfhack.internal.listPlugins()) do
        local env_name = 'plugins.' .. plugin
        local ok, plugin_env = pcall(require, env_name)
        if ok then
            load_widgets(plugin, plugin_env)
        end
    end
    scriptmanager.foreach_module_script(load_widgets)

    for name in pairs(widget_db) do
        table.insert(widget_index, name)
    end
    table.sort(widget_index)

    reposition_widgets()
end

dfhack.onStateChange[GLOBAL_KEY] = function(sc)
    if sc ~= SC_WORLD_LOADED then
        return
    end
    -- pick up widgets from active mods
    rescan()
end

local function dump_widget_config(name, widget)
    local pos = overlay_config[name].pos
    print(('widget %s is positioned at x=%d, y=%d'):format(name, pos.x, pos.y))
    local viewscreens = normalize_list(widget.viewscreens)
    if #viewscreens > 0 then
        print('  it will be attached to the following viewscreens:')
        for _,vs in ipairs(viewscreens) do
            print(('    %s'):format(simplify_viewscreen_name(vs)))
        end
    end
    if widget.hotspot then
        print('  it will act as a hotspot on all screens')
    end
end

local function do_position(args, quiet)
    local name_or_number, x, y = table.unpack(args)
    local name = get_name(name_or_number)
    if not widget_db[name] then
        if not name_or_number then
            dfhack.printerr('please specify a widget name or list number')
        else
            dfhack.printerr(('widget not found: "%s"'):format(name))
        end
        return
    end
    local widget = widget_db[name].widget
    local pos
    if x == 'default' then
        pos = sanitize_pos(widget.default_pos)
    else
        x, y = tonumber(x), tonumber(y)
        if not x or not y then
            dump_widget_config(name, widget)
            return
        end
        pos = sanitize_pos{x=x, y=y}
    end
    overlay_config[name].pos = pos
    widget.frame = make_frame(pos, widget.frame)
    local full, scaled = get_interface_rects()
    widget:updateLayout(widget.fullscreen and full or scaled)
    save_config()
    if not quiet then
        print(('repositioned widget %s to x=%d, y=%d'):format(name, pos.x, pos.y))
    end
end

-- note that the widget does not have to be enabled to be triggered
local function do_trigger(args, quiet)
    if triggered_screen_has_lock() then
        dfhack.printerr(('cannot trigger widget; widget "%s" is already active')
                        :format(trigger_lock_holder_description))
        return
    end
    do_by_names_or_numbers(args[1], function(name, db_entry)
        local widget = db_entry.widget
        if widget.overlay_trigger then
            register_trigger_lock_screen(
                widget:overlay_trigger(table.unpack(args, 2)),
                name
            )

            if not quiet then
                print(('triggered widget %s'):format(name))
            end
        end
    end)
end

local command_fns = {
    enable=do_enable,
    disable=do_disable,
    list=do_list,
    position=do_position,
    trigger=do_trigger,
}

local HELP_ARGS = utils.invert{'help', '--help', '-h'}

function overlay_command(args, quiet)
    local command = table.remove(args, 1) or 'help'
    if HELP_ARGS[command] or not command_fns[command] then return false end
    command_fns[command](args, quiet)
    return true
end

-- ---------------- --
-- event management --
-- ---------------- --

local function detect_frame_change(widget, fn)
    local frame = widget.frame
    local w, h = frame.w, frame.h
    local now_ms = dfhack.getTickCount()
    local ret = fn()
    record_widget_runtime(widget.name, now_ms)
    if w ~= frame.w or h ~= frame.h then
        widget:updateLayout()
    end
    return ret
end

local function get_next_onupdate_timestamp(now_ms, widget)
    local freq_s = widget.overlay_onupdate_max_freq_seconds
    if freq_s == 0 then
        return now_ms
    end
    local freq_ms = math.floor(freq_s * 1000)
    local jitter = math.random(0, freq_ms // 8) -- up to ~12% jitter
    return now_ms + freq_ms - jitter
end

-- reduces the next call by a small random amount to introduce jitter into the
-- widget processing timings
local function do_update(name, db_entry, now_ms, vs)
    local w = db_entry.widget
    if w.overlay_onupdate_max_freq_seconds ~= 0 and
        db_entry.next_update_ms > now_ms
    then
        return
    end
    if not utils.getval(w.active) then return end
    db_entry.next_update_ms = get_next_onupdate_timestamp(now_ms, w)
    if detect_frame_change(w, function() return w:overlay_onupdate(vs) end) then
        if register_trigger_lock_screen(w:overlay_trigger(), name) then
            return true
        end
    end
end

function update_hotspot_widgets()
    if triggered_screen_has_lock() then return end
    local now_ms = dfhack.getTickCount()
    for name,db_entry in pairs(active_hotspot_widgets) do
        if do_update(name, db_entry, now_ms) then return end
    end
end

local function matches_focus_strings(db_entry, vs_name, vs)
    if not db_entry.focus_strings then return true end
    local matched = true
    local simple_vs_name = simplify_viewscreen_name(vs_name)
    for _,fs in ipairs(db_entry.focus_strings) do
        if fs:startswith(simple_vs_name) then
            matched = false
            if dfhack.gui.matchFocusString(fs, vs) then
                return true
            end
        end
    end
    return matched
end

local function _update_viewscreen_widgets(vs_name, vs, now_ms)
    local vs_widgets = active_viewscreen_widgets[vs_name]
    if not vs_widgets then return end
    local is_all = vs_name == 'all'
    now_ms = now_ms or dfhack.getTickCount()
    for name,db_entry in pairs(vs_widgets) do
        if (is_all or matches_focus_strings(db_entry, vs_name, vs)) and
                do_update(name, db_entry, now_ms, vs) then
            return
        end
    end
    return now_ms
end

function update_viewscreen_widgets(vs_name, vs)
    if triggered_screen_has_lock() then return end
    local now_ms = _update_viewscreen_widgets(vs_name, vs, nil)
    if now_ms then
        _update_viewscreen_widgets('all', vs, now_ms)
    end
end

local function _feed_viewscreen_widgets(vs_name, vs, keys)
    local vs_widgets = active_viewscreen_widgets[vs_name]
    if not vs_widgets then return false end
    for _,db_entry in pairs(vs_widgets) do
        local w = db_entry.widget
        if (not vs or matches_focus_strings(db_entry, vs_name, vs)) and
            utils.getval(w.active) and
            utils.getval(w.visible) and
            detect_frame_change(w, function() return w:onInput(keys) end)
        then
            --print('widget handled input:', w.name)
            return true
        end
    end
    return false
end

function feed_viewscreen_widgets(vs_name, vs, keys)
    if not _feed_viewscreen_widgets(vs_name, vs, keys) and
            not _feed_viewscreen_widgets('all', nil, keys) then
        return false
    end
    return true
end

local function _render_viewscreen_widgets(vs_name, vs, full_dc, scaled_dc)
    local vs_widgets = active_viewscreen_widgets[vs_name]
    if not vs_widgets then return end
    local full, scaled = get_interface_rects()
    full_dc = full_dc or gui.Painter.new(full)
    scaled_dc = scaled_dc or gui.Painter.new(scaled)
    for _,db_entry in pairs(vs_widgets) do
        local w = db_entry.widget
        if (not vs or matches_focus_strings(db_entry, vs_name, vs)) and utils.getval(w.visible) then
            detect_frame_change(w, function() w:render(w.fullscreen and full_dc or scaled_dc) end)
        end
    end
    return full_dc, scaled_dc
end

local force_refresh

function render_viewscreen_widgets(vs_name, vs)
    local full_dc, scaled_dc = _render_viewscreen_widgets(vs_name, vs, nil, nil)
    _render_viewscreen_widgets('all', nil, full_dc, scaled_dc)
    if force_refresh then
        force_refresh = nil
        df.global.gps.force_full_display_count = 1
    end
end

-- called when the DF window is resized
function reposition_widgets()
    local full, scaled = get_interface_rects()
    for _,db_entry in pairs(widget_db) do
        local widget = db_entry.widget
        widget:updateLayout(widget.fullscreen and full or scaled)
    end
    force_refresh = true
end

-- ------------------------------------------------- --
-- OverlayWidget (base class of all overlay widgets) --
-- ------------------------------------------------- --

OverlayWidget = defclass(OverlayWidget, widgets.Panel)
OverlayWidget.ATTRS{
    name=DEFAULT_NIL, -- this is set by the framework to the widget name
    desc=DEFAULT_NIL, -- add a short description (<100 chars); displays in control panel
    default_pos={x=DEFAULT_X_POS, y=DEFAULT_Y_POS}, -- 1-based widget screen pos
    default_enabled=false, -- initial enabled state if not in config
    fullscreen=false, -- true if widget covers entire screen
    full_interface=false, -- true if widget covers entire interface area
    hotspot=false, -- whether to call overlay_onupdate on all screens
    viewscreens={}, -- override with associated viewscreen or list of viewscrens
    overlay_onupdate_max_freq_seconds=5, -- throttle calls to overlay_onupdate
}

function OverlayWidget:init()
    if self.overlay_onupdate_max_freq_seconds < 0 then
        error(('overlay_onupdate_max_freq_seconds must be >= 0: %s')
              :format(tostring(self.overlay_onupdate_max_freq_seconds)))
    end

    -- set defaults for frame. the widget is expected to keep these up to date
    -- when display contents change so the widget position can shift if the
    -- frame is relative to the right or bottom edges.
    self.frame = self.frame or {}
    self.frame.w = self.frame.w or 5
    self.frame.h = self.frame.h or 1
end

-- ------------------- --
-- TitleVersionOverlay --
-- ------------------- --

TitleVersionOverlay = defclass(TitleVersionOverlay, OverlayWidget)
TitleVersionOverlay.ATTRS{
    desc='Show DFHack version number and quick links on the DF title page.',
    default_pos={x=11, y=1},
    version=2,
    default_enabled=true,
    viewscreens='title/Default',
    frame={w=35, h=5},
    autoarrange_subviews=1,
}

function TitleVersionOverlay:init()
    local text = {}
    table.insert(text, 'DFHack ' .. dfhack.getDFHackVersion() ..
            (dfhack.isRelease() and '' or (' (git: %s)'):format(dfhack.getGitCommit(true))))
    if #dfhack.getDFHackBuildID() > 0 then
        table.insert(text, NEWLINE)
        table.insert(text, 'Build ID: ' .. dfhack.getDFHackBuildID())
    end
    if dfhack.isPrerelease() then
        table.insert(text, NEWLINE)
        table.insert(text, {text='Pre-release build', pen=COLOR_LIGHTRED})
    end

    for _,t in ipairs(text) do
        self.frame.w = math.max(self.frame.w, #t)
    end

    self:addviews{
        widgets.Label{
            frame={t=0, l=0},
            text=text,
            text_pen=COLOR_WHITE,
        },
        widgets.HotkeyLabel{
            frame={l=0},
            label='Quickstart guide',
            auto_width=true,
            key='STRING_A063',
            on_activate=function() dfhack.run_script('quickstart-guide') end,
        },
        widgets.HotkeyLabel{
            frame={l=0},
            label='Control panel',
            auto_width=true,
            key='STRING_A047',
            on_activate=function() dfhack.run_script('gui/control-panel') end,
        },
    }
end

OVERLAY_WIDGETS = {
    title_version = TitleVersionOverlay,
}

return _ENV
