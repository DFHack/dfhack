-- Shows the warning about missing configuration file.
--[[=begin

gui/prerelease-warning
======================
Shows a warning on world load for pre-release builds.

=end]]

if not dfhack.isPrerelease() then qerror('not a prerelease build') end
-- Don't fire during worldgen
if dfhack.internal.getAddress('gametype') and df.global.gametype == df.game_type.NONE then
    return
end

local gui = require 'gui'
local dlg = require 'gui.dialogs'
local utils = require 'utils'

message = {
    'This is a prerelease build of DFHack. Some structures are likely', NEWLINE,
    'to be incorrect, resulting in crashes or save corruption', NEWLINE,
    {pen=COLOR_LIGHTRED, text='Make backups of your saves and avoid saving if possible.'},
}

pack_message = pack_message or [[

This should not be enabled by default in a pack.
If you are seeing this message and did not enable/install DFHack
yourself, please report this to your pack's maintainer.]]

path = dfhack.getHackPath():lower()
if #pack_message > 0 and (path:find('lnp') or path:find('starter') or path:find('newb') or path:find('lazy') or path:find('pack')) then
    for _, v in pairs(utils.split_string(pack_message, '\n')) do
        table.insert(message, NEWLINE)
        table.insert(message, {text=v, pen=COLOR_LIGHTMAGENTA})
    end
    pack_message = ''
end

dfhack.print('\n')

for k,v in ipairs(message) do
    if type(v) == 'table' then
        dfhack.color(v.pen)
        dfhack.print(v.text)
    else
        dfhack.color(COLOR_YELLOW)
        dfhack.print(v)
    end
end

dfhack.color(COLOR_RESET)
dfhack.print('\n\n')

dlg.showMessage('Warning', message, COLOR_YELLOW)
