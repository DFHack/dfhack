local _ENV = mkmodule('plugins.preserve-rooms')

local gui = require('gui')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

------------------
-- command line
--

local function print_status()
    local features = preserve_rooms_getState()
    print('Features:')
    for feature,enabled in pairs(features) do
        print(('  %20s: %s'):format(feature, enabled))
    end
end

local function do_set_feature(enabled, feature)
    if not preserve_rooms_setFeature(enabled, feature) then
        qerror(('unknown feature: "%s"'):format(feature))
    end
end

local function do_reset_feature(feature)
    if not preserve_rooms_resetFeatureState(feature) then
        qerror(('unknown feature: "%s"'):format(feature))
    end
end

function parse_commandline(args)
    local opts = {}
    local positionals = argparse.processArgsGetopt(args, {
        {'h', 'help', handler=function() opts.help = true end},
    })

    if opts.help or not positionals or positionals[1] == 'help' then
        return false
    end

    local command = table.remove(positionals, 1)
    if not command or command == 'status' then
        print_status()
    elseif command == 'now' then
        preserve_rooms_cycle()
    elseif command == 'enable' or command == 'disable' then
        do_set_feature(command == 'enable', positionals[1])
    elseif command == 'reset' then
        do_reset_feature(positionals[1])
    else
        return false
    end

    return true
end

----------------------
-- ReservedWidget
--

ReservedWidget = defclass(ReservedWidget, overlay.OverlayWidget)
ReservedWidget.ATTRS{
    desc='Shows whether a zone has been reserved for a unit or role.',
    default_enabled=true,
    default_pos={x=30, y=10},
    viewscreens={
        'dwarfmode/Zone/Some/Bedroom',
        'dwarfmode/Zone/Some/DiningHall',
        'dwarfmode/Zone/Some/Office',
        'dwarfmode/Zone/Some/Tomb',
    },
    frame={w=40, h=3},
}

function ReservedWidget:init()
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

function ReservedWidget:render(dc)
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
    badge=ReservedWidget,
    roleassign=RoleAssignWidget,
}

return _ENV
