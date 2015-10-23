-- lave/load stockpile settings with a GUI
--[[=begin

gui/stockpiles
==============
An in-game interface for `stocksettings`, to
load and save stockpile settings from the :kbd:`q` menu.

Usage:

:gui/stockpiles -save:         to save the current stockpile
:gui/stockpiles -load:         to load settings into the current stockpile
:gui/stockpiles -dir <path>:   set the default directory to save settings into
:gui/stockpiles -help:         to see this message

Don't forget to ``enable stockpiles`` and create the ``stocksettings`` directory in
the DF folder before trying to use the GUI.

=end]]
local stock = require 'plugins.stockpiles'

function check_enabled()
    return stock.isEnabled()
end

function world_guard()
    if not dfhack.isMapLoaded() then
        qerror("World is not loaded")
        return false
    end
    return true
end

function guard()
    if not string.match(dfhack.gui.getCurFocus(), '^dwarfmode/QueryBuilding/Some/Stockpile') then
        qerror("This script requires a stockpile selected in the 'q' mode")
        return false
    end
    return true
end

utils = require('utils')
validArgs = validArgs or utils.invert({
    'help',
    'load',
    'save',
    'dir',
})

args = utils.processArgs({...}, validArgs)

function usage()
    print("")
    print("Stockpile Settings. Arguments: ")
    print("-save        to save the current stockpile")
    print("-load        to load settings into the current stockpile")
    print("-dir <path>  set the default directory to save settings into")
    if dfhack.isMapLoaded() then
        print("             Current directory is: " .. stock.get_path())
    end
    print("")
end

if not check_enabled() then
    qerror("Stockpiles plugin not enabled. Enable it with: enable stockpiles")
elseif args.load then
    if not guard() then return end
    stock.load_settings()
elseif args.save then
    if not guard() then return end
    local sp = dfhack.gui.getSelectedBuilding(true)
    stock.save_settings(sp)
elseif args.dir then
    if not world_guard() then return end
    stock.set_path(args.dir)
else
    usage()
end
