-- Export everything from legends mode
-- use "exportlegends maps" for detailed maps, or "exportlegends all" to also export legends

gui = require 'gui'

local args = {...}
local vs = dfhack.gui.getCurViewscreen()
local i = 1

local MAPS = {
    "Standard biome+site map",
    "Elevations including lake and ocean floors",
    "Elevations respecting water level",
    "Biome",
    "Hydrosphere",
    "Temperature",
    "Rainfall",
    "Drainage",
    "Savagery",
    "Volcanism",
    "Current vegetation",
    "Evil",
    "Salinity",
    "Structures/fields/roads/etc.",
    "Trade",
    "Nobility and Holdings",
    "Diplomacy",
}
function wait_for_legends_vs()
    vs = dfhack.gui.getCurViewscreen()
    if i <= #MAPS then
        if df.viewscreen_legendsst:is_instance(vs) then
            gui.simulateInput(vs, 'LEGENDS_EXPORT_DETAILED_MAP')
            dfhack.timeout(10,'frames',wait_for_export_maps_vs)
        else
            dfhack.timeout(10,'frames',wait_for_legends_vs)
        end
    end
end
function wait_for_export_maps_vs()
    vs = dfhack.gui.getCurViewscreen()
    if df.viewscreen_export_graphical_mapst:is_instance(vs) then
        vs.sel_idx = i
        print('    Exporting:  '..MAPS[i]..' map')
        i = i + 1
        gui.simulateInput(vs, 'SELECT')
        dfhack.timeout(10,'frames',wait_for_legends_vs)
    else
        dfhack.timeout(10,'frames',wait_for_export_maps_vs)
    end
end

if df.viewscreen_legendsst:is_instance( vs ) then -- dfhack.gui.getCurFocus() == "legends" 
    if args[1] == "all" then 
        gui.simulateInput(vs, df.interface_key.LEGENDS_EXPORT_MAP)
        print('    Exporting:  world map/gen info')
        gui.simulateInput(vs, df.interface_key.LEGENDS_EXPORT_XML)
        print('    Exporting:  legends xml')
        wait_for_legends_vs()
    elseif args[1] == "maps" then wait_for_legends_vs()
    end
elseif df.viewscreen_export_graphical_mapst:is_instance(vs) then
    if args[1] == "maps" or args[1] == "all" then wait_for_export_maps_vs() end
else
    dfhack.printerr('Not in legends view')
end
