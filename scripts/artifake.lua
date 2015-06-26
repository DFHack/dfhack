local utils = require 'utils'

validArgs = validArgs or utils.invert({
 'help',
 'mat',
 'item',
 'name',
 'r',
 'l' 
})

local args = utils.processArgs({...}, validArgs)

if args.help then
 print(
[[artifake.lua
arguments:
    -help
        print this help message
    -mat matstring
        specify the material of the item to be created
        examples:
            INORGANIC:IRON
            CREATURE_MAT:DWARF:BRAIN
            PLANT_MAT:MUSHROOM_HELMET_PLUMP:DRINK
    -item itemstring
        specify the itemdef of the item to be created
        examples:
            WEAPON:ITEM_WEAPON_PICK
    -name namestring
    	specify a first name if desired
    -r
	for right handed gloves
    -l
	for left handed gloves
]])
 return
end

if dfhack.gui.getSelectedUnit(true) then
 args.creator = dfhack.gui.getSelectedUnit()
 else args.creator = df.global.world.units.active[0]
end
if not args.item then
 error 'Invalid item.'
end
local itemType = dfhack.items.findType(args.item)
if itemType == -1 then
 error 'Invalid item.'
end
local itemSubtype = dfhack.items.findSubtype(args.item)

args.mat = dfhack.matinfo.find(args.mat)
if not args.mat then
 error 'Invalid material.'
end


local item = dfhack.items.createItem(itemType, itemSubtype, args.mat['type'], args.mat.index, args.creator)

 local base=df.item.find(df.global.item_next_id-1)
 df.global.world.artifacts.all:new()
 df.global.world.artifacts.all:insert('#',{new=df.artifact_record})
local facts = df.global.world.artifacts.all
 for _,k in ipairs(facts) do
  if k.id==0 then
   local fake=k
   fake.id=df.global.artifact_next_id
   fake.item = {new=base}
		fake.item.flags.artifact = true
		fake.item.flags.artifact_mood = true
		fake.item.id = base.id
		fake.item.general_refs:insert('#',{new =  df.general_ref_is_artifactst})
		fake.item.general_refs[0].artifact_id = fake.id
		fake.item.spec_heat = base.spec_heat
		fake.item.ignite_point = base.ignite_point
		fake.item.heatdam_point = base.heatdam_point
		fake.item.colddam_point = base.colddam_point 
		fake.item.boiling_point = base.boiling_point
		fake.item.fixed_temp = base.fixed_temp
		fake.item.weight = base.weight
		fake.item.weight_fraction = base.weight_fraction
		fake.item.improvements:insert('#',{new = df.itemimprovement_spikesst,mat_type=25,mat_index=474,quality=0,skill_rating=15})
		fake.item.improvements:insert('#',{new = df.itemimprovement_spikesst,mat_type=25,mat_index=493,quality=0,skill_rating=15})
		fake.item.improvements:insert('#',{new = df.itemimprovement_art_imagest,mat_type=22,mat_index=474,quality=5,skill_rating=15})
		fake.item.improvements:insert('#',{new = df.itemimprovement_art_imagest,mat_type=42,mat_index=480,quality=5,skill_rating=15})
		fake.item.improvements:insert('#',{new = df.itemimprovement_art_imagest,mat_type=22,mat_index=497,quality=5,skill_rating=15})
   fake.anon_1 = -1000000
   fake.anon_2 = -1000000
   fake.anon_3 = -1000000
     base.flags.artifact = true
     base.flags.artifact_mood = true
     base.general_refs = fake.item.general_refs
     base.improvements = fake.item.improvements
     fake.item:setQuality(5)
     base:setQuality(5)
     if fake.item == 'WEAPON' then item:setSharpness(1,0) end
     if base == 'WEAPON' then item:setSharpness(1,0) end
     df.global.artifact_next_id=df.global.artifact_next_id+1
 df.global.world.history.events:new()
 df.global.world.history.events:insert('#',{new=df.history_event_artifact_createdst,
		year = df.global.cur_year,
		seconds = df.global.cur_year_tick_advmode,
		id = df.global.hist_event_next_id,
		artifact_id = fake.id,
		unit_id = args.creator.id,
		hfid = args.creator.hist_figure_id,
		}
		)
   df.global.hist_event_next_id = df.global.hist_event_next_id+1 
if args.r then
	base.handedness[0] = true
	fake.item.handedness[0] = true
end
if args.l then
	base.handedness[1] = true
	fake.item.handedness[1] = true
end
 if args.name then do
  fake.name.first_name = args.name
  fake.name.language = 0
  fake.name.has_name = true 
         end
      end
   end
end
