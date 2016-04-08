-- Shows the warning about missing configuration file.
--[[=begin

gui/prerelease-warning
======================
Shows a warning on world load for pre-release builds.

With no arguments passed, the warning is shown unless the "do not show again"
option has been selected. With the ``force`` argument, the warning is always
shown.

=end]]

local gui = require 'gui'
local dlg = require 'gui.dialogs'
local json = require 'json'
local utils = require 'utils'

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

local state = dfhack.getDFHackRelease():lower():match('[a-z]+')
if not utils.invert{'alpha', 'beta', 'rc', 'r'}[state] then
    dfhack.printerr('warning: unknown release state: ' .. state)
    state = 'unknown'
end


message = ({
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
        {pen=COLOR_LIGHTMAGENTA, text='Please report this to the DFHack team or a pack maintainer.'}
    },
    unknown = {
        'Error',
        COLOR_LIGHTMAGENTA,
        'Unknown prerelease DFHack build. This should never happen!', NEWLINE,
        'Please report this to the DFHack team or a pack maintainer.'
    }
})[state]

title = table.remove(message, 1)
color = table.remove(message, 1)

pack_message = [[

This should not be enabled by default in a pack.
If you are seeing this message and did not enable/install DFHack
yourself, please report this to your pack's maintainer.]]

path = dfhack.getHackPath():lower()
if #pack_message > 0 and (path:find('lnp') or path:find('starter') or path:find('newb') or path:find('lazy') or path:find('pack')) then
    for _, v in pairs(utils.split_string(pack_message, '\n')) do
        table.insert(message, NEWLINE)
        table.insert(message, {text=v, pen=COLOR_LIGHTMAGENTA})
    end
end

dfhack.print('\n')

for k,v in ipairs(message) do
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

WarningBox = defclass(nil, dlg.MessageBox)

function WarningBox:getWantedFrameSize()
    local w, h = WarningBox.super.getWantedFrameSize(self)
    return w, h + 2
end

function WarningBox:onRenderFrame(dc,rect)
    WarningBox.super.onRenderFrame(self,dc,rect)
    dc:pen(COLOR_WHITE):key_pen(COLOR_LIGHTRED)
        :seek(rect.x1 + 2, rect.y2 - 2)
        :key('CUSTOM_D'):string(': Do not show again')
        :advance(10)
        :key('LEAVESCREEN'):string('/')
        :key('SELECT'):string(': Dismiss')
end

function WarningBox:onInput(keys)
    if keys.CUSTOM_D then
        config.data.hide = true
        config:write()
        keys.LEAVESCREEN = true
    end
    WarningBox.super.onInput(self, keys)
end

WarningBox{
    frame_title = title,
    text = message,
    text_pen = color
}:show()
