-- Removes webs and frees webbed units.
-- Author: Atomic Chicken

local usage = [====[

clear-webs
==========
This script removes all webs that are currently on the map,
and also frees any creatures who have been caught in one.

Note that it does not affect sprayed webs until
they settle on the ground.

Usable in both fortress and adventurer mode.

Web removal and unit release happen together by default.
The following may be used to isolate one of these actions:

Arguments::

    -unitsOnly
        Include this if you want to free all units from webs
        without removing any webs

    -websOnly
        Include this if you want to remove all webs
        without freeing any units

See also `fix/drop-webs`.

]====]

local utils = require 'utils'
local validArgs = utils.invert({
  'unitsOnly',
  'websOnly',
  'help'
})
local args = utils.processArgs({...}, validArgs)

if args.help then
  print(usage)
  return
end

if args.unitsOnly and args.websOnly then
  qerror("You have specified both -unitsOnly and -websOnly. These cannot be used together.")
end

local webCount = 0
if not args.unitsOnly then
  for i = #df.global.world.items.other.ANY_WEBS-1, 0, -1 do
    dfhack.items.remove(df.global.world.items.other.ANY_WEBS[i])
    webCount = webCount + 1
  end
end

local unitCount = 0
if not args.websOnly then
  for _, unit in ipairs(df.global.world.units.all) do
    if unit.counters.webbed > 0 and not unit.flags2.killed and not unit.flags1.inactive then -- the webbed status is retained in death
      unitCount = unitCount + 1
      unit.counters.webbed = 0
    end
  end
end

if not args.unitsOnly then
  if webCount == 0 then
    print("No webs detected!")
  else
    print("Removed " .. webCount .. " web" .. (webCount == 1 and "" or "s") .. "!")
  end
end

if not args.websOnly then
  if unitCount == 0 then
    print("No captured units detected!")
  else
    print("Freed " .. unitCount .. " unit" .. (unitCount == 1 and "" or "s") .. "!")
  end
end
