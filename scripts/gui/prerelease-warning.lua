-- Shows the warning about missing configuration file.
--[[=begin

gui/prerelease-warning
======================
Shows a warning on world load for pre-release builds.

=end]]

if not dfhack.isPrerelease() then qerror('not a prerelease build') end

local gui = require 'gui'
local dlg = require 'gui.dialogs'

local message = {
    'This is a prerelease build of DFHack. Some structures are likely', NEWLINE,
    'to be incorrect, resulting in crashes or save corruption', NEWLINE,
    {pen=COLOR_LIGHTRED, text='Make backups of your saves and avoid saving if possible.'}
}

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
