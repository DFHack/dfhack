--scripts/modtools/transform-unit.lua
--author expwnent
--based on shapechange by Putnam
--[[=begin

modtools/transform-unit
=======================
Transforms a unit into another unit type, possibly permanently.
Warning: this will crash arena mode if you view the unit on the
same tick that it transforms.  If you wait until later, it will be fine.

=end]]
local utils = require 'utils'

normalRace = normalRace or {}

local function transform(unit,race,caste)
 unit.enemy.normal_race = race
 unit.enemy.normal_caste = caste
 unit.enemy.were_race = race
 unit.enemy.were_caste = caste
end

validArgs = validArgs or utils.invert({
 'clear',
 'help',
 'unit',
 'duration',
 'setPrevRace',
 'keepInventory',
 'race',
 'caste',
 'suppressAnnouncement',
 'untransform',
})

local args = utils.processArgs({...}, validArgs)

if args.help then
 print([[scripts/modtools/transform-unit.lua
arguments
    -help
        print this help message
    -clear
        clear records of normal races
    -unit id
        set the target unit
    -duration ticks
        how long it should last, or "forever"
    -setPrevRace
        make a record of the previous race so that you can change it back with -untransform
    -keepInventory
        move items back into inventory after transformation
    -race raceName
    -caste casteName
    -suppressAnnouncement
        don't show the Unit has transformed into a Blah! event
    -untransform
        turn the unit back into what it was before
]])
 return
end

if args.clear then
 normalRace = {}
end

if not args.unit then
 error 'Specify a unit.'
end

if not args.duration then
 args.duration = 'forever'
end

local raceIndex
local race
local caste
if args.untransform then
 local unit = df.unit.find(tonumber(args.unit))
 raceIndex = normalRace[args.unit].race
 race = df.creature_raw.find(raceIndex)
 caste = normalRace[args.unit].caste
 normalRace[args.unit] = nil
else
 if not args.race or not args.caste then
  error 'Specficy a target form.'
 end

 --find race
 for i,v in ipairs(df.global.world.raws.creatures.all) do
  if v.creature_id == args.race then
   raceIndex = i
   race = v
   break
  end
 end

 if not race then
  error 'Invalid race.'
 end

 for i,v in ipairs(race.caste) do
  if v.caste_id == args.caste then
   caste = i
   break
  end
 end

 if not caste then
  error 'Invalid caste.'
 end
end

local unit = df.unit.find(tonumber(args.unit))
local oldRace = unit.enemy.normal_race
local oldCaste = unit.enemy.normal_caste
if args.setPrevRace then
 normalRace[args.unit] = {}
 normalRace[args.unit].race = oldRace
 normalRace[args.unit].caste = oldCaste
end
transform(unit,raceIndex,caste,args.setPrevRace)

local inventoryItems = {}

local function getInventory()
 local result = {}
 for _,item in ipairs(unit.inventory) do
  table.insert(result, item:new());
 end
 return result
end

local function restoreInventory()
 dfhack.timeout(1, 'ticks', function()
  for _,item in ipairs(inventoryItems) do
   dfhack.items.moveToInventory(item.item, unit, item.mode, item.body_part_id)
   item:delete()
  end
  inventoryItems = {}
 end)
end

if args.keepInventory then
 inventoryItems = getInventory()
end

if args.keepInventory then
 restoreInventory()
end
if args.duration and args.duration ~= 'forever' then
 --when the timeout ticks down, transform them back
 dfhack.timeout(tonumber(args.duration), 'ticks', function()
  if args.keepInventory then
   inventoryItems = getInventory()
  end
  transform(unit,oldRace,oldCaste)
  if args.keepInventory then
   restoreInventory()
  end
 end)
end

