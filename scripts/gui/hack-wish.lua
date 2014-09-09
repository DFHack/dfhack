-- hack-wish.lua
-- Allows for script-based wishing.
-- author Putnam
-- edited by expwnent

 
function getGenderString(gender)
 local genderStr
 if gender==0 then
  genderStr=string.char(12)
 elseif gender==1 then
  genderStr=string.char(11)
 else
  return ""
 end
 return string.char(40)..genderStr..string.char(41)
end
 
function getCreatureList()
 local crList={}
 for k,cr in ipairs(df.global.world.raws.creatures.alphabetic) do
  for kk,ca in ipairs(cr.caste) do
   local str=ca.caste_name[0]
   str=str..' '..getGenderString(ca.gender)
   table.insert(crList,{str,nil,ca})
  end
 end
 return crList
end
 
function getMatFilter(itemtype)
  local itemTypes={
   SEEDS=function(mat,parent,typ,idx)
    return mat.flags.SEED_MAT
   end,
   PLANT=function(mat,parent,typ,idx)
    return mat.flags.STRUCTURAL_PLANT_MAT
   end,
   LEAVES=function(mat,parent,typ,idx)
    return mat.flags.LEAF_MAT
   end,
   MEAT=function(mat,parent,typ,idx)
    return mat.flags.MEAT
   end,
   CHEESE=function(mat,parent,typ,idx)
    return (mat.flags.CHEESE_PLANT or mat.flags.CHEESE_CREATURE)
   end,
   LIQUID_MISC=function(mat,parent,typ,idx)
    return (mat.flags.LIQUID_MISC_PLANT or mat.flags.LIQUID_MISC_CREATURE or mat.flags.LIQUID_MISC_OTHER)
   end,
   POWDER_MISC=function(mat,parent,typ,idx)
    return (mat.flags.POWDER_MISC_PLANT or mat.flags.POWDER_MISC_CREATURE)
   end,
   DRINK=function(mat,parent,typ,idx)
    return (mat.flags.ALCOHOL_PLANT or mat.flags.ALCOHOL_CREATURE)
   end,
   GLOB=function(mat,parent,typ,idx)
    return (mat.flags.STOCKPILE_GLOB)
   end,
   WOOD=function(mat,parent,typ,idx)
    return (mat.flags.WOOD)
   end,
   THREAD=function(mat,parent,typ,idx)
    return (mat.flags.THREAD_PLANT)
   end,
   LEATHER=function(mat,parent,typ,idx)
    return (mat.flags.LEATHER)
   end
  }
  return itemTypes[df.item_type[itemtype]] or getRestrictiveMatFilter(itemtype)
end
 
function getRestrictiveMatFilter(itemType)
 if not args.veryRestrictive then return nil else
 local itemTypes={
   WEAPON=function(mat,parent,typ,idx)
    return (mat.flags.ITEMS_WEAPON or mat.flags.ITEMS_WEAPON_RANGED)
   end,
   AMMO=function(mat,parent,typ,idx)
    return (mat.flags.ITEMS_AMMO)
   end,
   ARMOR=function(mat,parent,typ,idx)
    return (mat.flags.ITEMS_ARMOR)
   end,
   SHOES,SHIELD,HELM,GLOVES=ARMOR,ARMOR,ARMOR,ARMOR,
   INSTRUMENT=function(mat,parent,typ,idx)
    return (mat.flags.ITEMS_HARD)
   end,
   GOBLET,FLASK,TOY,RING,CROWN,SCEPTER,FIGURINE,TOOL=INSTRUMENT,INSTRUMENT,INSTRUMENT,INSTRUMENT,INSTRUMENT,INSTRUMENT,INSTRUMENT,
   AMULET=function(mat,parent,typ,idx)
    return (mat.flags.ITEMS_SOFT or mat.flags.ITEMS_HARD)
   end,
   EARRING,BRACELET=AMULET,AMULET,
   ROCK=function(mat,parent,typ,idx)
    return (mat.flags.IS_STONE)
   end,
   BOULDER=ROCK,
   BAR=function(mat,parent,typ,idx)
    return (mat.flags.IS_METAL or mat.flags.SOAP or mat.id==COAL)
   end
   
  }
 return itemTypes[df.item_type[itemType]]
 end
end
 
function createItem(mat,itemType,quality,creator,description)
	dfhack.items.createItem(itemType[1], itemType[2], mat[1], mat[2], creator)
 if df.item_type[itemType[1]]=='SLAB' then
  item.description=description
 end
end
 
function qualityTable()
 return {{'None'},
 {'-Well-crafted-'},
 {'+Finely-crafted+'},
 {'*Superior*'},
 {string.char(240)..'Exceptional'..string.char(240)},
 {string.char(15)..'Masterwork'..string.char(15)}
 }
end
 
local script=require('gui.script')
 
function showItemPrompt(text,item_filter,hide_none)
 require('gui.materials').ItemTypeDialog{
  prompt=text,
  item_filter=item_filter,
  hide_none=hide_none,
  on_select=script.mkresume(true),
  on_cancel=script.mkresume(false),
  on_close=script.qresume(nil)
 }:show()
 
 return script.wait()
end
 
function showMaterialPrompt(title, prompt, filter, inorganic, creature, plant) --the one included with DFHack doesn't have a filter or the inorganic, creature, plant things available
 require('gui.materials').MaterialDialog{
  frame_title = title,
  prompt = prompt,
  mat_filter = filter,
  use_inorganic = inorganic,
  use_creature = creature,
  use_plant = plant,
  on_select = script.mkresume(true),
  on_cancel = script.mkresume(false),
  on_close = script.qresume(nil)
 }:show()
 
 return script.wait()
end
 
function usesCreature(itemtype)
 typesThatUseCreatures={REMAINS=true,FISH=true,FISH_RAW=true,VERMIN=true,PET=true,EGG=true,CORPSE=true,CORPSEPIECE=true}
 return typesThatUseCreatures[df.item_type[itemtype]]
end
 
function getCreatureRaceAndCaste(caste)
 return df.global.world.raws.creatures.list_creature[caste.index],df.global.world.raws.creatures.list_caste[caste.index]
end
 
function hackWish(unit)
 script.start(function()
  local amountok, amount
  local matok,mattype,matindex,matFilter
  local itemok,itemtype,itemsubtype=showItemPrompt('What item do you want?',function(itype) return df.item_type[itype]~='CORPSE' and df.item_type[itype]~='FOOD' end ,true)
  if not args.notRestrictive then
   matFilter=getMatFilter(itemtype)
  end
  if not usesCreature(itemtype) then
   matok,mattype,matindex=showMaterialPrompt('Wish','And what material should it be made of?',matFilter)
  else
   local creatureok,useless,creatureTable=script.showListPrompt('Wish','What creature should it be?',COLOR_LIGHTGREEN,getCreatureList())
   mattype,matindex=getCreatureRaceAndCaste(creatureTable[3])
  end
  local qualityok,quality=script.showListPrompt('Wish','What quality should it be?',COLOR_LIGHTGREEN,qualityTable())
  local description
  if df.item_type[itemtype]=='SLAB' then
   local descriptionok
   descriptionok,description=script.showInputPrompt('Slab','What should the slab say?',COLOR_WHITE)
  end
  if args.multi then
   repeat amountok,amount=script.showInputPrompt('Wish','How many do you want? (numbers only!)',COLOR_LIGHTGREEN) until tonumber(amount)
    if mattype and itemtype then
     for i=1,tonumber(amount) do
      createItem({mattype,matindex},{itemtype,itemsubtype},quality,unit,description)
     end
    end
  else
   if mattype and itemtype then
    createItem({mattype,matindex},{itemtype,itemsubtype},quality,unit,description)
   end   
  end
 end)
end
 
scriptArgs={...}

utils=require('utils')

validArgs = validArgs or utils.invert({
 'startup',
 'all',
 'restrictive',
 'unit',
 'multi'
})

args = utils.processArgs({...}, validArgs)
 
eventful=require('plugins.eventful')

if not args.startup then 
 local unit=args.unit and df.unit.find(args.unit) or dfhack.gui.getSelectedUnit(true)
 if unit then
  hackWish(unit)
 else
  qerror('A unit needs to be selected to use hackwish.')
 end
else
 eventful.onReactionComplete.hackWishP=function(reaction,unit,input_items,input_reagents,output_items,call_native)
  if not reaction.code:find('DFHACK_WISH') then return nil end
  hackWish(unit)
 end
end
