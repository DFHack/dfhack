local gui = require('gui')
local json = require('json')
local widgets = require('gui.widgets')
local utils = require('utils')

local force = ({...})[1] == 'force'
local config = json.open('dfhack-config/prerelease-warning.json')

if config.data.hide and not force then
    return
end
if not dfhack.isPrerelease() and not force then
    qerror('not a prerelease build')
end
-- Don't fire during worldgen
if dfhack.internal.getAddress('gametype') and df.global.gametype == df.game_type.NONE and not force then
    return
end

local state = dfhack.getDFHackRelease():lower():match('([a-z]+)%d*$')
if not utils.invert{'alpha', 'beta', 'rc', 'r'}[state] then
    dfhack.printerr('warning: unknown release state: ' .. state)
    state = 'unknown'
end

local message = ({
    alpha = {
        'Warning',
        COLOR_YELLOW,
        'This is an alpha build of DFHack. Some structures are likely', NEWLINE,
        'to be incorrect, resulting in crashes or save corruption', NEWLINE,
        {pen=COLOR_LIGHTRED, text='Make backups of your saves often!'}
    },
    beta = {
        'Warning',
        COLOR_YELLOW,
        'This is a beta release of DFHack. It is more stable than an', NEWLINE,
        'alpha release, but bugs are still possible, possibly including', NEWLINE,
        'crashes and save corruption.', NEWLINE,
        'Make backups of your saves beforehand to be safe.'
    },
    rc = {
        'Notice',
        COLOR_YELLOW,
        'This is a DFHack release candidate. It is fairly stable but', NEWLINE,
        'likely contains newer features that are not fully-tested.', NEWLINE,
        'Crashes are unlikely, but always make backups of your saves', NEWLINE,
        'to be safe.'
    },
    r = {
        'Error',
        COLOR_LIGHTRED,
        'This release is flagged as a prerelease but named as a', NEWLINE,
        'stable release.', NEWLINE,
        {pen=COLOR_LIGHTMAGENTA, text='Please report this to the DFHack team.'}
    },
    unknown = {
        'Error',
        COLOR_LIGHTMAGENTA,
        'Unknown prerelease DFHack build. This should never happen!', NEWLINE,
        'Please report this to the DFHack team.'
    }
})[state]

local title = table.remove(message, 1)
local color = table.remove(message, 1)

local pack_message = [[

This should not be enabled by default in a pack.
If you are seeing this message and did not enable/install DFHack
yourself, please report this to your pack's maintainer.]]

local path = dfhack.getHackPath():lower()
if path:find('lnp') or path:find('starter') or path:find('newb') or path:find('lazy') or
        path:find('pack') then
    for _, v in pairs(pack_message:split('\n')) do
        table.insert(message, NEWLINE)
        table.insert(message, {text=v, pen=COLOR_LIGHTMAGENTA})
    end
end

for _, v in pairs(([[

REMINDER: Please report any issues you encounter while
using this DFHack build on GitHub (github.com/DFHack/dfhack/issues)
or the Bay12 forums (dfhack.org/bay12).]]):split('\n')) do
    table.insert(message, NEWLINE)
    table.insert(message, {text=v, pen=COLOR_LIGHTCYAN})
end

local nightly_message = [[

You appear to be using a nightly build of DFHack. If you
experience problems, check dfhack.org/builds for updates.]]
if dfhack.getDFHackBuildID() ~= '' then
    for _, v in pairs(nightly_message:split('\n')) do
        table.insert(message, NEWLINE)
        table.insert(message, {text=v, pen=COLOR_LIGHTGREEN})
    end
end

dfhack.print('\n')

for _, v in ipairs(message) do
    if type(v) == 'table' then
        dfhack.color(v.pen)
        dfhack.print(v.text)
    else
        dfhack.color(color)
        dfhack.print(v)
    end
end

dfhack.color(COLOR_RESET)
dfhack.print('\n\n')

WarningBoxScreen = defclass(WarningBoxScreen, gui.ZScreenModal)
WarningBoxScreen.ATTRS{focus_path='prerelease-warning'}

function WarningBoxScreen:init()
    self:addviews{
        widgets.Window{
            frame_title=title,
            frame={w=74, h=14},
            resizable=true,
            resize_min={h=11},
            subviews={
                widgets.Label{
                    frame={t=0, b=2},
                    text=message,
                    text_pen=color,
                },
                widgets.HotkeyLabel{
                    frame={b=0, l=0},
                    key='CUSTOM_D',
                    label='Do not show again',
                    auto_width=true,
                    on_activate=function()
                        config.data.hide = true
                        config:write()
                        self:dismiss()
                    end,
                },
                widgets.HotkeyLabel{
                    frame={b=0, l=30},
                    key='SELECT',
                    label='Dismiss',
                    auto_width=true,
                    on_activate=function() self:dismiss() end,
                },
            },
        },
    }
end

-- never reset view; we only want to show this dialog once
view = view or WarningBoxScreen{}:show()
