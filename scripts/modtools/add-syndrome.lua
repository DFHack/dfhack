--modtools/add-syndrome.lua
--author expwnent
--[[=begin

modtools/add-syndrome
=====================
This allows adding and removing syndromes from units.

=end]]
local syndromeUtil = require 'syndrome-util'
local utils = require 'utils'

validArgs = validArgs or utils.invert({
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
 print([[scripts/modtools/add-syndrome usage:
arguments:
    -help
        print this help message
    -syndrome name
        the name of the syndrome to operate on
        examples:
            "gila monster bite"
    -resetPolicy policy
        specify a policy of what to do if the unit already has an instance of the syndrome
        examples:
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
        add the syndrome to the target regardless of whether it is immune to the syndrome
]])
 return
end

if not args.target then
 error 'Specify a target.'
end
local targ = df.unit.find(tonumber(args.target))
if not targ then
 error ('Could not find target: ' .. args.target)
end
args.target = targ

if args.eraseClass then
 local count = syndromeUtil.eraseSyndromeClass(args.target, args.eraseClass)
 --print('deleted ' .. tostring(count) .. ' syndromes')
 return
end

if args.resetPolicy then
 args.resetPolicy = syndromeUtil.ResetPolicy[args.resetPolicy]
 if not args.resetPolicy then
  error ('Invalid reset policy.')
 end
end

if not args.syndrome then
 error 'Specify a syndrome name.'
end

local syndrome
for _,syn in ipairs(df.global.world.raws.syndromes.all) do
 if syn.syn_name == args.syndrome then
  syndrome = syn
  break
 end
end
if not syndrome then
 error ('Invalid syndrome: ' .. args.syndrome)
end
args.syndrome = syndrome

if args.erase then
 syndromeUtil.eraseSyndrome(args.target,args.syndrome.id,args.eraseOldest)
 return
end

if args.eraseAll then
 syndromeUtil.eraseSyndromes(args.target,args.syndrome.id)
 return
end

if skipImmunities then
 syndromeUtil.infectWithSyndrome(args.target,args.syndrome,args.resetPolicy)
else
 syndromeUtil.infectWithSyndromeIfValidTarget(args.target,args.syndrome,args.resetPolicy)
end

