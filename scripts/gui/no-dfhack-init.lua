-- Shows the warning about missing configuration file.
--[[=begin

gui/no-dfhack-init
==================
Shows a warning at startup if no valid :file:`dfhack.init` file is found.

=end]]
local gui = require 'gui'
local dlg = require 'gui.dialogs'

local dfhack_init = { text = 'dfhack.init', pen = COLOR_LIGHTCYAN }
local dfhack_init_example = { text = 'dfhack.init-example', pen = COLOR_LIGHTCYAN }

local message = {
    'The ', dfhack_init, ' configuration file is missing. To customize', NEWLINE,
    'your DFHack installation, rename the ', dfhack_init_example, ' file', NEWLINE,
    'to ', dfhack_init, ' and edit it to suit your needs.', NEWLINE, NEWLINE,
    'For now, ', dfhack_init_example, ' will be used instead.'
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

dlg.showMessage('DFHack is not configured', message, COLOR_YELLOW)
