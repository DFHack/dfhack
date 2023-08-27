-- Shows the sexual orientation of units
local help = [====[

gaydar
======
Shows the sexual orientation of units, useful for social engineering or checking
the viability of livestock breeding programs.

Targets:

:-all:          shows orientation of every creature
:-citizens:     shows only orientation of citizens in fort mode
:-named:        shows orientation of all named units on map
:(no target):   shows orientation of the unit under the cursor

Orientation filters:

:-notStraight:  only creatures who are not strictly straight
:-gayOnly:      only creatures who are strictly gay
:-biOnly:       only creatures who can get into romances with both sexes
:-straightOnly: only creatures who are strictly straight
:-asexualOnly:  only creatures who are strictly asexual

]====]
local utils = require('utils')

local validArgs = utils.invert({
  'all',
  'citizens',
  'named',
  'notStraight',
  'gayOnly',
  'biOnly',
  'straightOnly',
  'asexualOnly',
  'help'
})


local args = utils.processArgs({...}, validArgs)

if args.help then
    print(help)
    return
end

function dfprint(s)
  print(dfhack.df2console(s))
end

function getSexString(sex)
  local sym = df.pronoun_type.attrs[sex].symbol
  if not sym then
    return ""
  end
  return "("..sym..")"
end

local function determineorientation(unit)
 if unit.sex~=-1 and unit.status.current_soul then
  local return_string=''
  local orientation=unit.status.current_soul.orientation_flags
  if orientation.indeterminate then
   return ' indeterminate (probably adventurer)'
  end
  local male_interested,asexual=false,true
  if orientation.romance_male then
   return_string=return_string..' likes males'
   male_interested=true
   asexual=false
  elseif orientation.marry_male then
   return_string=return_string..' will marry males'
   male_interested=true
   asexual=false
  end
  if orientation.romance_female then
   if male_interested then
 return_string=return_string..' and likes females'
   else
    return_string=return_string..' likes females'
   end
   asexual=false
  elseif orientation.marry_female then
   if male_interested then
 return_string=return_string..' and will marry females'
   else
    return_string=return_string..' will marry females'
   end
   asexual=false
  end
  if asexual then
   return_string=' is asexual'
  end
  return return_string
 else
  return " is not biologically capable of sex"
 end
end

local function nameOrSpeciesAndNumber(unit)
 if unit.name.has_name then
  return dfhack.TranslateName(dfhack.units.getVisibleName(unit))..' '..getSexString(unit.sex),true
 else
  return 'Unit #'..unit.id..' ('..df.creature_raw.find(unit.race).caste[unit.caste].caste_name[0]..' '..getSexString(unit.sex)..')',false
 end
end

local orientations={} --as:string[]

if args.citizens then
 for k,v in ipairs(df.global.world.units.active) do
  if dfhack.units.isCitizen(v) then
   table.insert(orientations,nameOrSpeciesAndNumber(v) .. determineorientation(v))
  end
 end
elseif args.all then
 for k,v in ipairs(df.global.world.units.active) do
  table.insert(orientations,nameOrSpeciesAndNumber(v)..determineorientation(v))
 end
elseif args.named then
 for k,v in ipairs(df.global.world.units.active) do
  local name,ok=nameOrSpeciesAndNumber(v)
  if ok then
   table.insert(orientations,name..determineorientation(v))
  end
 end
else
 local unit=dfhack.gui.getSelectedUnit(true)
 local name,ok=nameOrSpeciesAndNumber(unit)
 dfprint(name..determineorientation(unit))
 return
end

function isNotStraight(v)
 if v:find(string.char(12)) and v:find(' female') then return true end
 if v:find(string.char(11)) and v:find(' male') then return true end
 if v:find('asexual') then return true end
 if v:find('indeterminate') then return true end
 return false
end

function isGay(v)
 if v:find('asexual') then return false end
 if v:find(string.char(12)) and not v:find(' male') then return true end
 if v:find(string.char(11)) and not v:find(' female') then return true end
 return false
end

function isAsexual(v)
 if v:find('asexual') or v:find('indeterminate') then return true else return false end
end

function isBi(v)
 if v:find(' female') and v:find(' male') then return true else return false end
end

if args.notStraight then
 local totalNotShown=0
 for k,v in ipairs(orientations) do
  if isNotStraight(v) then dfprint(v) else totalNotShown=totalNotShown+1 end
 end
 print('Total not shown: '..totalNotShown)
elseif args.gayOnly then
 local totalNotShown=0
 for k,v in ipairs(orientations) do
  if isGay(v) then dfprint(v) else totalNotShown=totalNotShown+1 end
 end
 print('Total not shown: '..totalNotShown)
elseif args.asexualOnly then
 local totalNotShown=0
 for k,v in ipairs(orientations) do
  if isAsexual(v) then dfprint(v) else totalNotShown=totalNotShown+1 end
 end
 print('Total not shown: '..totalNotShown)
elseif args.straightOnly then
 local totalNotShown=0
 for k,v in ipairs(orientations) do
  if not isNotStraight(v) then dfprint(v) else totalNotShown=totalNotShown+1 end
 end
 print('Total not shown: '..totalNotShown)
elseif args.biOnly then
 local totalNotShown=0
 for k,v in ipairs(orientations) do
  if isBi(v) then dfprint(v) else totalNotShown=totalNotShown+1 end
 end
 print('Total not shown: '..totalNotShown)
else
 for k,v in ipairs(orientations) do
  dfprint(v)
 end
end
