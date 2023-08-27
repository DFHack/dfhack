-- config ui for autofish

local gui = require("gui")
local widgets = require("gui.widgets")
local autofish = reqscript("autofish")

local REFRESH_MS = 10000

---
-- Autofish
---

Autofish = defclass(Autofish, widgets.Window)
Autofish.ATTRS{
    frame_title = "Autofish",
    frame = {w=35, h=11},
    resizable = false
}

function Autofish:init()
    self:addviews{
        widgets.ToggleHotkeyLabel{
            view_id="enable_toggle",
            frame={t=0, l=0, w=31},
            label="Autofish is",
            key="CUSTOM_CTRL_E",
            options={{value=true, label="Enabled", pen=COLOR_GREEN},
                     {value=false, label="Disabled", pen=COLOR_RED}},
            on_change=function(val) dfhack.run_command{val and "enable" or "disable", "autofish"} end
        },
        widgets.EditField{
            view_id="maximum",
            frame={t=2,l=0},
            label_text="Maximum fish target: ",
            key="CUSTOM_M",
            on_char=function(ch) return ch:match("%d") end,
            on_submit=function(text)
                autofish.set_maxFish(tonumber(text))
                autofish.set_minFish(math.floor(tonumber(text)*0.75))
                self:refresh_data()
            end
        },
        widgets.ToggleHotkeyLabel{
            view_id="useRawFish",
            frame={t=3, l=0, w=31},
            label="Counting raw fish:",
            key="CUSTOM_ALT_R",
            options={{value=true, label="Yes", pen=COLOR_GREEN},
                     {value=false, label="No", pen=COLOR_RED}},
            on_change=function(val)
                autofish.set_useRaw(val)
            end
        },
        widgets.Label{
            view_id="fish_count",
            frame={t=5, l=0, h=1},
            auto_height=false
        },
        widgets.Label{
            view_id="current_mode",
            frame={t=6, l=0, h=1},
            auto_height=false,
            visible=autofish.isEnabled
        }

    }
    self:refresh_data()
end

function get_count_pen(val, min, max)
    if val <= min then return COLOR_RED end
    if val <= max then return COLOR_YELLOW end
    return COLOR_GREEN
end

function get_count_text(val, min, max)
    return {text=tostring(val), pen=get_count_pen(val, min, max)}
end

function get_fishing_text(val)
    if not val then return {text="not ", pen=COLOR_RED} end
    return ""
end

function Autofish:refresh_data()
    self.subviews.enable_toggle:setOption(autofish.isEnabled())

    --self.subviews.minimum:setText(tostring(autofish.set_minFish))
    self.subviews.maximum:setText(tostring(autofish.s_maxFish))
    self.subviews.useRawFish:setOption(autofish.s_useRaw)

    local prep, raw = autofish.count_fish()
    local total = prep+raw
    local counts_text = {
        "Fish: ",
        get_count_text(total, autofish.s_minFish, autofish.s_maxFish),
        " (",
        {text=tostring(prep), pen=COLOR_GREY}, " prepared, ",
        {text=tostring(raw), pen=COLOR_GREY}, " raw)"
    }
    self.subviews.fish_count:setText(counts_text)
    local fishingText = {
        "Currently ",
        get_fishing_text(autofish.isFishing),
        "fishing."
    }
    self.subviews.current_mode:setText(fishingText)
    self.next_refresh_ms = dfhack.getTickCount() + REFRESH_MS
end

function Autofish:onRenderBody()
    if self.next_refresh_ms <= dfhack.getTickCount() then
        self:refresh_data()
    end
end


---
-- AutofishScreen
---
AutofishScreen = defclass(AutofishScreen, gui.ZScreen)
AutofishScreen.ATTRS{focus_path = "autofish"}

function AutofishScreen:init()
    self:addviews{Autofish{}}
end

function AutofishScreen:onDismiss()
    view = nil
end
if not dfhack.isMapLoaded then
    qerror("autofish requires a map to be loaded")
end

view = view and view:raise() or AutofishScreen{}:show()
