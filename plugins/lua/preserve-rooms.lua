local _ENV = mkmodule('plugins.preserve-tombs')

local gui = require('gui')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

----------------------
-- BadgeWidget
--

BadgeWidget = defclass(BadgeWidget, overlay.OverlayWidget)
BadgeWidget.ATTRS{
    desc='Shows an indicator that a zone is managed by preserve-rooms.',
    default_enabled=true,
    default_pos={x=30, y=10},
    viewscreens={
        'dwarfmode/Zone/Some/Office',
    },
    frame={w=40, h=3},
    frame_style=gui.FRAME_MEDIUM,
}

function BadgeWidget:init()
    self:addviews{
        widgets.Label{
            frame={l=0, t=0},
            text={
                'Autoassigned to role:',
                {gap=1, text=get_current_role_assignment, pen=COLOR_MAGENTA},
            },
        },
    }
end

function BadgeWidget:render(dc)
    if not has_current_role_assignment() then
        return
    end
    BadgetWidget.super.render(self, dc)
end

----------------------
-- RoleAssignWidget
--

RoleAssignWidget = defclass(RoleAssignWidget, overlay.OverlayWidget)
RoleAssignWidget.ATTRS{
    desc='Adds a configuration dropdown for autoassigning rooms to noble roles.',
    default_enabled=true,
    default_pos={x=40, y=8},
    viewscreens={
        'dwarfmode/UnitSelector/ZONE_OFFICE_ASSIGNMENT',
    },
    frame={w=20, h=1},
}

function RoleAssignWidget:init()
    self:addviews{
        widgets.TextLabel{
            frame={l=0, t=0, w=10},
            label='Assign to role',
            key='CUSTOM_SHIFT_S',
        },
    }
end

OVERLAY_WIDGETS = {
    badge=BadgeWidget,
    roleassign=RoleAssignWidget,
}

return _ENV
