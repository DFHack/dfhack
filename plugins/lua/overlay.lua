local _ENV = mkmodule('plugins.overlay')

local widgets = require('gui.widgets')

local widget_db = {} -- map of widget name to state
local active_hotspot_widgets = {} -- map of widget names to the db entry
local active_viewscreen_widgets = {} -- map of vs_name to map of w.names -> db
local active_triggered_screen = nil

function reload()
    widget_db = {}
    active_hotspot_widgets = {}
    active_viewscreen_widgets = {}
    active_triggered_screen = nil
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
