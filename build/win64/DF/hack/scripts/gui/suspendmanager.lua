-- config ui for suspendmanager

local gui = require("gui")
local widgets = require("gui.widgets")
local suspendmanager = reqscript("suspendmanager")

---
-- Suspendmanager
---

SuspendmanagerWindow = defclass(SuspendmanagerWindow, widgets.Window)
SuspendmanagerWindow.ATTRS{
    frame_title = "Suspendmanager",
    frame = {w=38, h=11},
}

function SuspendmanagerWindow:init()
    self:addviews{
        widgets.ToggleHotkeyLabel{
            frame={t=0, l=0, w=34},
            label="Suspendmanager is",
            key="CUSTOM_CTRL_E",
            options={{value=true, label="Enabled", pen=COLOR_GREEN},
                     {value=false, label="Disabled", pen=COLOR_RED}},
            initial_option = suspendmanager.isEnabled(),
            on_change=function(val) dfhack.run_command{val and "enable" or "disable", "suspendmanager"} end
        },
        widgets.ToggleHotkeyLabel{
            frame={t=2, l=0, w=33},
            label="Prevent blocking jobs:",
            key="CUSTOM_ALT_B",
            options={{value=true, label="Yes", pen=COLOR_GREEN},
                     {value=false, label="No", pen=COLOR_RED}},
            initial_option = suspendmanager.preventBlockingEnabled(),
            on_change=function(val)
                suspendmanager.update_setting("preventblocking", val)
            end
        },

    }
end

---
-- SuspendmanagerScreen
---
SuspendmanagerScreen = defclass(SuspendmanagerScreen, gui.ZScreen)
SuspendmanagerScreen.ATTRS{focus_path = "suspendmanager"}

function SuspendmanagerScreen:init()
    self:addviews{SuspendmanagerWindow{}}
end

function SuspendmanagerScreen:onDismiss()
    view = nil
end
if not dfhack.isMapLoaded then
    qerror("suspendmanager requires a map to be loaded")
end

view = view and view:raise() or SuspendmanagerScreen{}:show()
