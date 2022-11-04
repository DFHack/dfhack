local _ENV = mkmodule('plugins.overlay')

local gui = require('gui')
local json = require('json')
local utils = require('utils')
local widgets = require('gui.widgets')

local OVERLAY_CONFIG_FILE = 'dfhack-config/overlay.json'
local OVERLAY_WIDGETS_VAR = 'OVERLAY_WIDGETS'

local DEFAULT_X_POS, DEFAULT_Y_POS = -2, -2

-- ---------------- --
-- state and config --
-- ---------------- --

local active_triggered_screen = nil -- if non-nil, hotspots will not get updates
local widget_db = {} -- map of widget name to ephermeral state
local widget_index = {} -- ordered list of widget names
local overlay_config = {} -- map of widget name to persisted state
local active_hotspot_widgets = {} -- map of widget names to the db entry
local active_viewscreen_widgets = {} -- map of vs_name to map of w.names -> db

local function reset()
    if active_triggered_screen then
        active_triggered_screen:dismiss()
    end
    active_triggered_screen = nil

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

local function triggered_screen_has_lock()
    if not active_triggered_screen then return false end
    if active_triggered_screen:isActive() then return true end
    active_triggered_screen = nil
    return false
end

-- ----------- --
-- utility fns --
-- ----------- --

local function normalize_list(element_or_list)
    if type(element_or_list) == 'table' then return element_or_list end
    return {element_or_list}
end

-- allow "short form" to be specified, but use "long form"
local function normalize_viewscreen_name(vs_name)
    if vs_name:match('viewscreen_.*st') then return vs_name end
    return 'viewscreen_' .. vs_name .. 'st'
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
    -- if someone accidentally uses 1-based instead of 0-based indexing, fix it
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

local function get_screen_rect()
    local w, h = dfhack.screen.getWindowSize()
    return gui.ViewRect{rect=gui.mkdims_wh(0, 0, w, h)}
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
    for _,name_or_number in ipairs(normalize_list(args)) do
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
    do_by_names_or_numbers(args, function(name, db_entry)
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
    end)
    if not skip_save then
        save_config()
    end
end

local function do_disable(args)
    do_by_names_or_numbers(args, function(name, db_entry)
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
        print(('disabled widget %s'):format(name))
    end)
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
        dfhack.color(enabled and COLOR_YELLOW or COLOR_LIGHTGREEN)
        dfhack.print(enabled and '[enabled] ' or '[disabled]')
        dfhack.color()
        print((' %d) %s%s'):format(i, name,
            db_entry.widget.overlay_trigger and ' (can trigger)' or ''))
        ::continue::
    end
    if num_filtered > 0 then
        print(('(%d widgets filtered out)'):format(num_filtered))
    end
end

local function load_widget(name, widget_class)
    local widget = widget_class{name=name}
    widget_db[name] = {
        widget=widget,
        next_update_ms=widget.overlay_onupdate and 0 or math.huge,
    }
    if not overlay_config[name] then overlay_config[name] = {} end
    local config = overlay_config[name]
    if not config.pos then
        config.pos = sanitize_pos(widget.default_pos)
    end
    widget.frame = make_frame(config.pos, widget.frame)
    if config.enabled then
        do_enable(name, true, true)
    else
        config.enabled = false
    end
end

local function load_widgets(env_prefix, provider, env_fn)
    local env_name = env_prefix .. provider
    local ok, provider_env = pcall(env_fn, env_name)
    if not ok or not provider_env[OVERLAY_WIDGETS_VAR] then return end
    local overlay_widgets = provider_env[OVERLAY_WIDGETS_VAR]
    if type(overlay_widgets) ~= 'table' then
        dfhack.printerr(
                ('error loading overlay widgets from "%s": %s map is malformed')
                :format(env_name, OVERLAY_WIDGETS_VAR))
        return
    end
    for widget_name,widget_class in pairs(overlay_widgets) do
        local name = provider .. '.' .. widget_name
        if not safecall(load_widget, name, widget_class) then
            dfhack.printerr(('error loading overlay widget "%s"'):format(name))
        end
    end
end

-- also called directly from cpp on init
function reload()
    reset()

    for _,plugin in ipairs(dfhack.internal.listPlugins()) do
        load_widgets('plugins.', plugin, require)
    end
    for _,script_path in ipairs(dfhack.internal.getScriptPaths()) do
        local files = dfhack.filesystem.listdir_recursive(
                                            script_path, nil, false)
        if not files then goto skip_path end
        for _,f in ipairs(files) do
            if not f.isdir and
                    f.path:endswith('.lua') and
                    not f.path:startswith('test/') and
                    not f.path:startswith('internal/') then
                local script_name = f.path:sub(1, #f.path - 4) -- remove '.lua'
                load_widgets('', script_name, reqscript)
            end
        end
        ::skip_path::
    end

    for name in pairs(widget_db) do
        table.insert(widget_index, name)
    end
    table.sort(widget_index)

    reposition_widgets()
end

local function do_reload()
    reload()
    print('reloaded overlay configuration')
end

local function do_reposition(args)
    local name_or_number, x, y = table.unpack(args)
    local name = get_name(name_or_number)
    -- TODO: check existence of widget, validate numbers, warn if offscreen
    local pos = sanitize_pos{x=tonumber(x), y=tonumber(y)}
    overlay_config[name].pos = pos
    local widget = widget_db[name].widget
    widget.frame = make_frame(pos, widget.frame)
    widget:updateLayout(get_screen_rect())
    save_config()
    print(('repositioned widget %s to x=%d, y=%d'):format(name, pos.x, pos.y))
end

-- note that the widget does not have to be enabled to be triggered
local function do_trigger(args)
    if triggered_screen_has_lock() then
        dfhack.printerr(
                'cannot trigger widget; another widget is already active')
        return
    end
    local target = args[1]
    do_by_names_or_numbers(target, function(name, db_entry)
        local widget = db_entry.widget
        if widget.overlay_trigger then
            widget:overlay_trigger()
            print(('triggered widget %s'):format(name))
        end
    end)
end

local command_fns = {
    enable=do_enable,
    disable=do_disable,
    list=do_list,
    reload=do_reload,
    reposition=do_reposition,
    trigger=do_trigger,
}

local HELP_ARGS = utils.invert{'help', '--help', '-h'}

function overlay_command(args)
    local command = table.remove(args, 1) or 'help'
    if HELP_ARGS[command] or not command_fns[command] then return false end
    command_fns[command](args)
    return true
end

-- ---------------- --
-- event management --
-- ---------------- --

local function detect_frame_change(widget, fn)
    local frame = widget.frame
    local w, h = frame.w, frame.h
    local ret = fn()
    if w ~= frame.w or h ~= frame.h then
        widget:updateLayout()
    end
    return ret
end

-- reduces the next call by a small random amount to introduce jitter into the
-- widget processing timings
local function do_update(db_entry, now_ms, vs)
    if db_entry.next_update_ms > now_ms then return end
    local w = db_entry.widget
    local freq_ms = w.overlay_onupdate_max_freq_seconds * 1000
    local jitter = math.random(0, freq_ms // 8) -- up to ~12% jitter
    db_entry.next_update_ms = now_ms + freq_ms - jitter
    if detect_frame_change(w,
            function() return w:overlay_onupdate(vs) end) then
        active_triggered_screen = w:overlay_trigger()
        if active_triggered_screen then return true end
    end
end

function update_hotspot_widgets()
    if triggered_screen_has_lock() then return end
    local now_ms = dfhack.getTickCount()
    for _,db_entry in pairs(active_hotspot_widgets) do
        if do_update(db_entry, now_ms) then return end
    end
end

function update_viewscreen_widgets(vs_name, vs)
    local vs_widgets = active_viewscreen_widgets[vs_name]
    if not vs_widgets then return end
    local now_ms = dfhack.getTickCount()
    for _,db_entry in pairs(vs_widgets) do
        if do_update(db_entry, now_ms, vs) then return end
    end
end

function feed_viewscreen_widgets(vs_name, keys)
    local vs_widgets = active_viewscreen_widgets[vs_name]
    if not vs_widgets then return false end
    for _,db_entry in pairs(vs_widgets) do
        local widget = db_entry.widget
        if detect_frame_change(widget,
                function() return widget:onInput(keys) end) then
            return true
        end
    end
    return false
end

function render_viewscreen_widgets(vs_name)
    local vs_widgets = active_viewscreen_widgets[vs_name]
    if not vs_widgets then return false end
    local dc = gui.Painter.new()
    for _,db_entry in pairs(vs_widgets) do
        local widget = db_entry.widget
        detect_frame_change(widget, function() widget:render(dc) end)
    end
end

-- called when the DF window is resized
function reposition_widgets()
    local sr = get_screen_rect()
    for _,db_entry in pairs(widget_db) do
        db_entry.widget:updateLayout(sr)
    end
end

-- ------------------------------------------------- --
-- OverlayWidget (base class of all overlay widgets) --
-- ------------------------------------------------- --

OverlayWidget = defclass(OverlayWidget, widgets.Widget)
OverlayWidget.ATTRS{
    name=DEFAULT_NIL, -- this is set by the framework to the widget name
    default_pos={x=DEFAULT_X_POS, y=DEFAULT_Y_POS}, -- initial widget screen pos, 1-based
    hotspot=false, -- whether to call overlay_onupdate for all screens
    viewscreens={}, -- override with list of viewscrens to interpose
    overlay_onupdate_max_freq_seconds=5, -- throttle calls to overlay_onupdate
}

-- set defaults for frame. the widget is expected to keep these up to date as
-- display contents change.
function OverlayWidget:init()
    self.frame = self.frame or {}
    self.frame.w = self.frame.w or 5
    self.frame.h = self.frame.h or 1
end

return _ENV
