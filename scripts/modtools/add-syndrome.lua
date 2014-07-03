--modtools/add-syndrome.lua
--author expwnent
--add syndromes to a target, or remove them

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
 'skipImmunities'
})

local args = utils.processArgs({...}, validArgs)

if args.help then
 print('add-syndrome usage:')
 print(' -help')
 print('   print this help message')
 print(' -syndrome [name]')
 print('   select a syndrome by name')
 print(' -resetPolicy {default = NewInstance}')
 print('   DoNothing')
 print('     if the target already has an instance of the syndrome, do nothing')
 print('   ResetDuration')
 print('     if the target already has an instance of the syndrome, reset the duration to maximum')
 print('   AddDuration')
 print('     if the target already has an instance of the syndrome, add the maximum duration to the time remaining')
 print('   NewInstance')
 print('     if the target already has an instance of the syndrome, add a new instance of the syndrome')
 print(' -erase')
 print('   instead of adding a syndrome, erase one')
 print(' -eraseAll')
 print('   instead of adding a syndrome, erase every instance of it')
 print(' -target [id]')
 print('   set the target unit for infection or uninfection')
 print(' -skipImmunities')
 print('   don\'t check syn_class immune/affected stuff when adding the syndrome')
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

if not args.target then
 error 'Specify a target.'
end
local targ = df.unit.find(tonumber(args.target))
if not targ then
 error ('Could not find target: ' .. args.target)
end
args.target = targ

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

