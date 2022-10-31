local _ENV = mkmodule('plugins.overlay')

local json = require('json')
local utils = require('utils')
local widgets = require('gui.widgets')

local WIDGETS_ENABLED_FILE = 'dfhack-config/overlay/widgets.json'
local WIDGETS_STATE_DIR = 'dfhack-config/overlay/widgets/'

local widget_db = {} -- map of widget name to state
local active_hotspot_widgets = {} -- map of widget names to the db entry
local active_viewscreen_widgets = {} -- map of vs_name to map of w.names -> db
local active_triggered_screen = nil

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

    local frame = {}
    local pos = utils.assign({x=-1, y=20}, config.pos or {})
    if pos.x < 0 then frame.r = math.abs(pos.x) - 1 else frame.l = pos.x - 1 end
    if pos.y < 0 then frame.b = math.abs(pos.y) - 1 else frame.t = pos.y - 1 end

    return provider_env[classname]{frame=frame}
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

local function load_widget(name, config, enabled)
    local widget = instantiate_widget(name, config)
    if not widget then return end
    local db_entry = {
        widget=widget,
        next_update_ms=widget.overlay_onupdate and 0 or math.huge,
    }
    widget_db[name] = db_entry
    if not enabled then return end
    if config.hotspot then
        active_hotspot_widgets[name] = db_entry
    end
    for vs_name in ipairs(normalize_list(config.viewscreens)) do
        vs_name = normalize_viewscreen_name(vs_name)
        ensure_key(active_viewscreen_widgets, vs_name)[name] = db_entry
    end
end

local function load_config(fname)
    local ok, config = pcall(json.decode_file, fname)
    return ok and config or {}
end

function reload()
    widget_db = {}
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
end

-- reduces the next call by a small random amount to introduce jitter into the
-- widget processing timings
local function do_update(db_entry, now_ms, vs)
    if db_entry.next_update_ms > now_ms then return end
    local w = db_entry.widget
    local freq_ms = w.overlay_onupdate_max_freq_seconds * 1000
    local jitter = math.rand(0, freq_ms // 8) -- up to ~12% jitter
    db_entry.next_update_ms = now_ms + freq_ms - jitter
    if w:overlay_onupdate(vs) then
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
        if db_entry.widget:onInput(keys) then return true end
    end
    return false
end

function render_viewscreen_widgets(vs_name)
    local vs_widgets = active_viewscreen_widgets[vs_name]
    if not vs_widgets then return false end
    local dc = Painter.new()
    for _,db_entry in pairs(vs_widgets) do
        db_entry.widget:render(dc)
    end
end

-- called when the DF window is resized
function reposition_widgets()
    local w, h = dscreen.getWindowSize()
    local vr = ViewRect{rect=mkdims_wh(0, 0, w, h)}
    for _,db_entry in pairs(widget_db) do
        db_entry.widget:updateLayout(vr)
    end
end

OverlayWidget = defclass(OverlayWidget, widgets.Widget)
OverlayWidget.ATTRS{
    overlay_onupdate_max_freq_seconds=5,
}

return _ENV
