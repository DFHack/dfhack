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

function createItem(mat,itemType,quality,pos,description)
 local item=df[df.item_type.attrs[itemType[1]].classname]:new()
 item.id=df.global.item_next_id
 df.global.world.items.all:insert('#',item)
 df.global.item_next_id=df.global.item_next_id+1
 item:setSubtype(itemType[2])
 item:setMaterial(mat[1])
 item:setMaterialIndex(mat[2])
 if df.item_type[itemType[1]]=='EGG' then
  local creature=df.creature_raw.find(mat[1])
  local eggMat={}
  eggMat[1]=dfhack.matinfo.find(creature.creature_id..':EGGSHELL')
  if eggMat[1] then
   eggMat[2]=dfhack.matinfo.find(creature.creature_id..':EGG_WHITE')
   eggMat[3]=dfhack.matinfo.find(creature.creature_id..'EGG_YOLK')
   for k,v in ipairs(eggMat) do
    item.egg_materials.mat_type:insert('#',v.type)
    item.egg_materials.mat_index:insert('#',v.index)
   end
  else
   eggMat=dfhack.matinfo.find(creature.creature_id..':MUSCLE')
   item.egg_materials.mat_type:insert('#',eggMat.type)
   item.egg_materials.mat_index:insert('#',eggMat.index)
  end
 end
 item:categorize(true)
 item.flags.removed=true
 item:setSharpness(1,0)
 item:setQuality(quality-1)
 if df.item_type[itemType[1]]=='SLAB' then
  item.description=description
 end
 dfhack.items.moveToGround(item,{x=pos.x,y=pos.y,z=pos.z})
end

--TODO: should this be a function?
function qualityTable()
 return {{'None'},
  {'-Well-crafted-'},
  {'+Finely-crafted+'},
  {'*Superior*'},
  {string.char(240)..'Exceptional'..string.char(240)},
  {string.char(15)..'Masterwork'..string.char(15)}
 }
end

local script=require('gui/script')
local guimaterials=require('gui.materials')

function showItemPrompt(text,item_filter,hide_none)
 guimaterials.ItemTypeDialog{
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
 guimaterials.MaterialDialog{
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

function hackWish(posOrUnit)
 local pos = df.unit:is_instance(posOrUnit) and posOrUnit.pos or posOrUnit
 script.start(function()
  --local amountok, amount
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
  --repeat amountok,amount=script.showInputPrompt('Wish','How many do you want? (numbers only!)',COLOR_LIGHTGREEN) until tonumber(amount)
  if mattype and itemtype then
   --for i=1,tonumber(amount) do
   createItem({mattype,matindex},{itemtype,itemsubtype},quality,pos,description)
   --end
  end
 end)
end

scriptArgs={...}

args={}

for k,v in ipairs(scriptArgs) do
 v=v:lower()
 if v=='startup' then args.startup=true end
 if v=='all' then args.notRestrictive=true end
 if v=='restrictive' then args.veryRestrictive=true end
 if v=='unit' then args.unitNum=args[k+1] end
 if v=='x' then args.x=args[k+1] end
 if v=='y' then args.y=args[k+1] end
 if v=='z' then args.z=args[k+1] end
end

eventful=require('plugins.eventful')

function posIsValid(pos)
 return pos.x~=-30000 and pos or false
end

if not args.startup then 
 local posOrUnit=args.x and {x=args.x,y=args.y,z=args.z} or args.unitNum and df.unit.find(args.unitNum) or posIsValid(df.global.cursor) or dfhack.gui.getSelectedUnit(true)
 hackWish(posOrUnit)
else
 eventful.onReactionComplete.hackWishP=function(reaction,unit,input_items,input_reagents,output_items, call_native)
  if not reaction.code:find('DFHACK_WISH') then return nil end
  hackWish(unit)
 end
end

