-- teleport.lua
-- teleports a unit to a location
-- author Putnam
-- edited by expwnent
--[[=begin

teleport
========
Teleports a unit to given coordinates.  Examples:

:teleport -showunitid:  prints unitid beneath cursor
:teleport -showpos:     prints coordinates beneath cursor
:teleport -unit 1234 -x 56 -y 115 -z 26:
                        teleports unit 1234 to 56,115,26

=end]]

function teleport(unit,pos)
 local unitoccupancy = dfhack.maps.getTileBlock(unit.pos).occupancy[unit.pos.x%16][unit.pos.y%16]
 local newoccupancy = dfhack.maps.getTileBlock(pos).occupancy[pos.x%16][pos.y%16]
 if newoccupancy.unit then
  unit.flags1.on_ground=true
 end
 unit.pos.x = pos.x
 unit.pos.y = pos.y
 unit.pos.z = pos.z
 if not unit.flags1.on_ground then unitoccupancy.unit = false else unitoccupancy.unit_grounded = false end
end

utils = require('utils')

validArgs = validArgs or utils.invert({
 'unit',
 'x',
 'y',
 'z',
 'showunitid',
 'showpos'
})

if moduleMode then
 return
end

local args = utils.processArgs({...}, validArgs)

if args.showunitid or args.showpos then
 if args.showunitid then
  print(dfhack.gui.getSelectedUnit(true).id)
 else
  printall(df.global.cursor)
 end
else
 local unit = args.unit and df.unit.find(args.unit) or dfhack.gui.getSelectedUnit(true)
 local pos = not(not args.x or not args.y or not args.z) and {x=args.x,y=args.y,z=args.z} or {x=df.global.cursor.x,y=df.global.cursor.y,z=df.global.cursor.z}
 if not unit then qerror('A unit needs to be selected or specified. Use teleport -showunitid to get a unit\'s ID.') end
 if not pos.x or pos.x==-30000 then qerror('A position needs to be highlighted or specified. Use teleport -showpos to get a position\'s exact xyz values.') end
 teleport(unit,pos)
end
