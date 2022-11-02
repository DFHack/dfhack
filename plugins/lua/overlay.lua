local _ENV = mkmodule('plugins.overlay')

local gui = require('gui')
local json = require('json')
local utils = require('utils')
local widgets = require('gui.widgets')

local WIDGETS_ENABLED_FILE = 'dfhack-config/overlay/widgets.json'
local WIDGETS_STATE_DIR = 'dfhack-config/overlay/widgets/'

local widget_db = {} -- map of widget name to state
local widget_index = {} -- list of widget names
local active_hotspot_widgets = {} -- map of widget names to the db entry
local active_viewscreen_widgets = {} -- map of vs_name to map of w.names -> db
local active_triggered_screen = nil

local function load_config(path)
    local ok, config = safecall(json.decode_file, path)
    return ok and config or {}
end

local function save_config(data, path)
    if not safecall(json.encode_file, data, path) then
        dfhack.printerr(('failed to save overlay config file: "%s"')
                :format(path))
    end
end

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

local function save_enabled()
    local enabled_map = {}
    for name,db_entry in pairs(widget_db) do
        enabled_map[name] = db_entry.enabled
    end
    save_config(enabled_map, WIDGETS_ENABLED_FILE)
end

local function do_enable(args)
    do_by_names_or_numbers(args, function(name, db_entry)
        db_entry.enabled = true
        if db_entry.config.hotspot then
            active_hotspot_widgets[name] = db_entry
        end
        for _,vs_name in ipairs(normalize_list(db_entry.config.viewscreens)) do
            vs_name = normalize_viewscreen_name(vs_name)
            ensure_key(active_viewscreen_widgets, vs_name)[name] = db_entry
        end
    end)
    save_enabled()
end

local function do_disable(args)
    do_by_names_or_numbers(args, function(name, db_entry)
        db_entry.enabled = false
        if db_entry.config.hotspot then
            active_hotspot_widgets[name] = nil
        end
        for _,vs_name in ipairs(normalize_list(db_entry.config.viewscreens)) do
            vs_name = normalize_viewscreen_name(vs_name)
            ensure_key(active_viewscreen_widgets, vs_name)[name] = nil
            if is_empty(active_viewscreen_widgets[vs_name]) then
                active_viewscreen_widgets[vs_name] = nil
            end
        end
    end)
    save_enabled()
end

local function do_list(args)
    local filter = args and #args > 0
    for i,name in ipairs(widget_index) do
        if filter then
            local passes = false
            for _,str in ipairs(args) do
                if name:find(str) then
                    passes = true
                    break
                end
            end
            if not passes then goto continue end
        end
        local enabled = widget_db[name].enabled
        dfhack.color(enabled and COLOR_YELLOW or COLOR_LIGHTGREEN)
        dfhack.print(enabled and '[enabled] ' or '[disabled]')
        dfhack.color()
        print((' %d) %s%s'):format(i, name,
            widget_db[name].widget.overlay_trigger and ' (can trigger)' or ''))
        ::continue::
    end
end

local function make_frame(config, old_frame)
    local old_frame, frame = old_frame or {}, {}
    frame.w, frame.h = old_frame.w, old_frame.h
    local pos = utils.assign({x=-1, y=20}, config.pos or {})
    -- if someone accidentally uses 1-based instead of 0-based indexing, fix it
    if pos.x == 0 then pos.x = 1 end
    if pos.y == 0 then pos.y = 1 end
    if pos.x < 0 then frame.r = math.abs(pos.x) - 1 else frame.l = pos.x - 1 end
    if pos.y < 0 then frame.b = math.abs(pos.y) - 1 else frame.t = pos.y - 1 end
    return frame
end

local function get_screen_rect()
    local w, h = dfhack.screen.getWindowSize()
    return gui.ViewRect{rect=gui.mkdims_wh(0, 0, w, h)}
end

local function do_reposition(args)
    local name_or_number, x, y = table.unpack(args)
    local name = get_name(name_or_number)
    local db_entry = widget_db[name]
    local config = db_entry.config
    config.pos.x, config.pos.y = tonumber(x), tonumber(y)
    db_entry.widget.frame = make_frame(config, db_entry.widget.frame)
    db_entry.widget:updateLayout(get_screen_rect())
    save_config(config, WIDGETS_STATE_DIR .. name .. '.json')
end

local function do_trigger(args)
    local target = args[1]
    do_by_names_or_numbers(target, function(name, db_entry)
        if db_entry.widget.overlay_trigger then
            db_entry.widget:overlay_trigger()
        end
    end)
end

local command_fns = {
    enable=do_enable,
    disable=do_disable,
    list=do_list,
    reload=function() reload() end,
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

local function instantiate_widget(name, config)
    local provider = config.provider
    local ok, provider_env = pcall(require, provider)
    if not ok then
        ok, provider_env = pcall(require, 'plugins.'..provider)
    end
    if not ok then
        ok, provider_env = pcall(reqscript, provider)
    end
    if not ok then
        dfhack.printerr(
                ('error loading overlay widget "%s": could not find provider' ..
                    ' environment "%s"')
                :format(name, provider))
        return nil
    end

    local classname = config.class
    if not provider_env[classname] then
        dfhack.printerr(
                ('error loading overlay widget "%s": could not find class "%s"')
                :format(name, classname))
        return nil
    end

    return provider_env[classname]{frame=make_frame(config)}
end

local function load_widget(name, config, enabled)
    local widget = instantiate_widget(name, config)
    if not widget then return end
    local db_entry = {
        enabled=enabled,
        config=config,
        widget=widget,
        next_update_ms=widget.overlay_onupdate and 0 or math.huge,
    }
    widget_db[name] = db_entry
    if enabled then do_enable(name) end
end

function reload()
    widget_db = {}
    widget_index = {}
    active_hotspot_widgets = {}
    active_viewscreen_widgets = {}
    active_triggered_screen = nil

    local enabled_map = load_config(WIDGETS_ENABLED_FILE)
    for _,fname in ipairs(dfhack.filesystem.listdir(WIDGETS_STATE_DIR)) do
        local _,_,name = fname:find('^(.*)%.json$')
        if not name then goto continue end
        local widget_config = load_config(WIDGETS_STATE_DIR..fname)
        if not widget_config.provider or not widget_config.class then
            dfhack.printerr(
                    ('error loading overlay widget "%s": "provider" and' ..
                     ' "class" must be specified in %s%s')
                    :format(name, WIDGETS_STATE_DIR, fname))
            goto continue
        end
        load_widget(name, widget_config, not not enabled_map[name])
        ::continue::
    end

    for name in pairs(widget_db) do
        table.insert(widget_index, name)
    end
    table.sort(widget_index)

    reposition_widgets()
end

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
    if active_triggered_screen then
        if active_triggered_screen:isActive() then return end
        active_triggered_screen = nil
    end
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

OverlayWidget = defclass(OverlayWidget, widgets.Widget)
OverlayWidget.ATTRS{
    overlay_onupdate_max_freq_seconds=5,
}

function OverlayWidget:preinit(info)
    info.frame = info.frame or {}
    info.frame.w = info.frame.w or 5
    info.frame.h = info.frame.h or 1
end

return _ENV
