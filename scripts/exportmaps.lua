-- Export all detail maps from legends
-- version 1.0 by Kurik Amudnil (http://dffd.wimbli.com/file.php?id=7696)

gui = require 'gui'

local vs = dfhack.gui.getCurViewscreen()
local i = 0

local MAPS = {
	[0] = "Standard biome+site map",
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
}
function wait_for_legends_vs()
	vs = dfhack.gui.getCurViewscreen()
	if i < 15 then
		if df.viewscreen_legendsst:is_instance(vs) then
			gui.simulateInput(vs, 'LEGENDS_EXPORT_DETAILED_MAP') -- "d" on screen some number internally 
			dfhack.timeout(10,'frames',wait_for_export_maps_vs)
		else
			dfhack.timeout(10,'frames',wait_for_legends_vs)
		end
	end
end
function wait_for_export_maps_vs()
	vs = dfhack.gui.getCurViewscreen()
	if df.viewscreen_export_graphical_mapst:is_instance(vs) then
		vs.anon_13 = i -- anon_13 appears to be the selection cursor for this viewscreen
		print('Exporting:  '..MAPS[i])
		i = i + 1
		gui.simulateInput(vs, 'SELECT') -- 1 internally, enter on screen
		dfhack.timeout(10,'frames',wait_for_legends_vs)
	else
		dfhack.timeout(10,'frames',wait_for_export_maps_vs)
	end
end

if df.viewscreen_legendsst:is_instance( vs ) then -- dfhack.gui.getCurFocus() == "legends" 
	wait_for_legends_vs()
elseif df.viewscreen_export_graphical_mapst:is_instance(vs) then
	wait_for_export_maps_vs()
else
	dfhack.printerr('Not in legends view')
end
