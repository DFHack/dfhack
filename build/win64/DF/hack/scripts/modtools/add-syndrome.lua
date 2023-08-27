-- Add or remove syndromes from units
--author expwnent
local usage = [====[

modtools/add-syndrome
=====================
This allows adding and removing syndromes from units.

Arguments::

    -syndrome name|id
        the name or id of the syndrome to operate on
        examples:
            "gila monster bite"
            14
    -resetPolicy policy
        specify a policy of what to do if the unit already has an
        instance of the syndrome.  examples:
            NewInstance
                default behavior: create a new instance of the syndrome
            DoNothing
            ResetDuration
            AddDuration
    -erase
        instead of adding an instance of the syndrome, erase one
    -eraseAll
        erase every instance of the syndrome
    -eraseClass SYN_CLASS
        erase every instance of every syndrome with the given SYN_CLASS
    -target id
        the unit id of the target unit
        examples:
            0
            28
    -skipImmunities
        add the syndrome to the target even if it is immune to the syndrome

]====]
local syndromeUtil = require 'syndrome-util'
local utils = require 'utils'

local validArgs = utils.invert({
 'help',
 'syndrome',
 'resetPolicy',
 'erase',
 'eraseOldest',
 'eraseAll',
 'target',
 'skipImmunities',
 'eraseClass'
})

local args = utils.processArgs({...}, validArgs)

if args.help then
 print(usage)
 return
end

if not args.target then
 error 'Specify a target.'
end
local targ = df.unit.find(tonumber(args.target))
if not targ then
 error ('Could not find target: ' .. args.target)
end

if args.eraseClass then
 local count = syndromeUtil.eraseSyndromeClass(targ, args.eraseClass)
 --print('deleted ' .. tostring(count) .. ' syndromes')
 return
end

local resetPolicy = syndromeUtil.ResetPolicy.NewInstance
if args.resetPolicy then
 resetPolicy = syndromeUtil.ResetPolicy[args.resetPolicy]
 if not resetPolicy then
  error ('Invalid reset policy.')
 end
end

if not args.syndrome then
 error 'Specify a syndrome name or id.'
end

local syndrome
if tonumber(args.syndrome) then
 syndrome = df.syndrome.find(tonumber(args.syndrome))
else
 for _,syn in ipairs(df.global.world.raws.syndromes.all) do
  if syn.syn_name == args.syndrome then
   syndrome = syn
   break
  end
 end
end
if not syndrome then
 error ('Invalid syndrome: ' .. args.syndrome)
end

if args.erase then
 syndromeUtil.eraseSyndrome(targ,syndrome.id,args.eraseOldest)
 return
end

if args.eraseAll then
 syndromeUtil.eraseSyndromes(targ,syndrome.id)
 return
end

if args.skipImmunities then
 syndromeUtil.infectWithSyndrome(targ,syndrome,resetPolicy)
else
 syndromeUtil.infectWithSyndromeIfValidTarget(targ,syndrome,resetPolicy)
end
