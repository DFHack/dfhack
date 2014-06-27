
local syndromeUtil = require 'syndromeUtil'
local utils = require 'utils'

mode = mode or utils.invert({
 'add',
 'erase',
 'eraseAll'
})

local args = {...}
local i = 1
local syndrome
local resetPolicy = syndromeUtil.ResetPolicy.NewInstance
local currentMode = mode.add
local target
local skipImmunities = false
while i <= #args do
 if args[i] == '-help' then
  print('add-syndrome usage:')
  print(' -help')
  print('  print this help message')
  print(' -syndrome [name]')
  print('  select a syndrome by name')
  print(' -resetPolicy DoNothing')
  print('  if the target already has an instance of the syndrome, do nothing')
  print(' -resetPolicy ResetDuration')
  print('  if the target already has an instance of the syndrome, reset the duration to maximum')
  print(' -resetPolicy AddDuration')
  print('  if the target already has an instance of the syndrome, add the maximum duration to the time remaining')
  print(' -resetPolicy NewInstance')
  print('  if the target already has an instance of the syndrome, add a new instance of the syndrome')
  print(' -erase')
  print('  instead of adding a syndrome, erase one')
  print(' -eraseAll')
  print('  instead of adding a syndrome, erase every instance of it')
  print(' -target [id]')
  print('  set the target unit for infection')
  print(' -skipImmunities')
  print('  don\'t check syn_class immune/affected stuff.')
  return
 elseif args[i] == '-syndrome' then
  local syn_name = args[i+1]
  for _,syn in ipairs(df.global.world.raws.syndromes.all) do
   if syn.syn_name == syn_name then
    syndrome = syn
    break
   end
  end
  i = i+2
 elseif args[i] == '-resetPolicy' then
  resetPolicy = syndromeUtil.ResetPolicy[args[i+1]]
  if not resetPolicy then 
   qerror('Invalid arguments to add-syndrome.')
  end
  i = i+2
 elseif args[i] == '-erase' then
  currentMode = mode.erase
  i = i+1
 elseif args[i] == '-eraseAll' then
  currentMode = mode.eraseAll
  i = i+1
 elseif args[i] == '-add' then
  currentMode = mode.add
  i = i+1
 elseif args[i] == '-target' then
  target = df.unit.find(tonumber(args[i+1]))
  if not target then 
   qerror('Invalid unit id argument to add-syndrome.')
  end
  i = i+2
 elseif args[i] == '-skipImmunities' then
  skipImmunities = true
  i = i+1
 else
  qerror('Invalid arguments to add-syndrome.')
 end
end

if skipImmunities then
 syndromeUtil.infectWithSyndrome(target,syndrome,resetPolicy)
else
 syndromeUtil.infectWithSyndromeIfValidTarget(target,syndrome,resetPolicy)
end

