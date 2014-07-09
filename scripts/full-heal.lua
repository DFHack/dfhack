--full-heal.lua
--author Kurik Amudnil, Urist DaVinci
--edited by expwnent

-- attempt to fully heal a selected unit, option -r to attempt to resurrect the unit
local args = {...}
local resurrect = false
local i=0
for _,arg in ipairs(args) do
 if arg == '-r' or arg == '-R' then
  resurrect = true
 elseif tonumber(arg) then
  unit = df.unit.find(tonumber(arg))
 elseif arg == 'help' or arg == '-help' or arg == '-h' then
  print('full-heal: heal a unit completely from anything, optionally including death.')
  print(' full-heal [unitId]')
  print('  heal the unit with the given id')
  print(' full-heal -r [unitId]')
  print('  heal the unit with the given id and bring them back from death if they are dead')
  print(' full-heal')
  print('  heal the currently selected unit')
  print(' full-heal -r')
  print('  heal the currently selected unit and bring them back from death if they are dead')
  print(' full-heal help')
  print('  print this help message')
  return
 end
end
unit = unit or dfhack.gui.getSelectedUnit()
if not unit then
 qerror('Error: please select a unit or pass its id as an argument.')
end

if unit then
	if resurrect then
		if unit.flags1.dead then
			--print("Resurrecting...")
			unit.flags2.slaughter = false
			unit.flags3.scuttle = false
		end
		unit.flags1.dead = false
		unit.flags2.killed = false
		unit.flags3.ghostly = false
		--unit.unk_100 = 3
	end
	
	--print("Erasing wounds...")
	while #unit.body.wounds > 0 do
		unit.body.wounds:erase(#unit.body.wounds-1)
	end
	unit.body.wound_next_id=1

	--print("Refilling blood...")
	unit.body.blood_count=unit.body.blood_max

	--print("Resetting grasp/stand status...")
	unit.status2.limbs_stand_count=unit.status2.limbs_stand_max
	unit.status2.limbs_grasp_count=unit.status2.limbs_grasp_max

	--print("Resetting status flags...")
	unit.flags2.has_breaks=false
	unit.flags2.gutted=false
	unit.flags2.circulatory_spray=false
	unit.flags2.vision_good=true
	unit.flags2.vision_damaged=false
	unit.flags2.vision_missing=false
	unit.flags2.breathing_good=true
	unit.flags2.breathing_problem=false

	unit.flags2.calculated_nerves=false
	unit.flags2.calculated_bodyparts=false
	unit.flags2.calculated_insulation=false
	unit.flags3.compute_health=true

	--print("Resetting counters...")
	unit.counters.winded=0
	unit.counters.stunned=0
	unit.counters.unconscious=0
	unit.counters.webbed=0
	unit.counters.pain=0
	unit.counters.nausea=0
	unit.counters.dizziness=0

	unit.counters2.paralysis=0
	unit.counters2.fever=0
	unit.counters2.exhaustion=0
	unit.counters2.hunger_timer=0
	unit.counters2.thirst_timer=0
	unit.counters2.sleepiness_timer=0
	unit.counters2.vomit_timeout=0
	
	--print("Resetting body part status...")
	v=unit.body.components
	for i=0,#v.nonsolid_remaining - 1,1 do
		v.nonsolid_remaining[i] = 100	-- percent remaining of fluid layers (Urist Da Vinci)
	end

	v=unit.body.components
	for i=0,#v.layer_wound_area - 1,1 do
		v.layer_status[i].whole = 0		-- severed, leaking layers (Urist Da Vinci)
		v.layer_wound_area[i] = 0		-- wound contact areas (Urist Da Vinci)
		v.layer_cut_fraction[i] = 0		-- 100*surface percentage of cuts/fractures on the body part layer (Urist Da Vinci)
		v.layer_dent_fraction[i] = 0		-- 100*surface percentage of dents on the body part layer (Urist Da Vinci)
		v.layer_effect_fraction[i] = 0		-- 100*surface percentage of "effects" on the body part layer (Urist Da Vinci)
	end
	
	v=unit.body.components.body_part_status
	for i=0,#v-1,1 do
		v[i].on_fire = false
		v[i].missing = false
		v[i].organ_loss = false
		v[i].organ_damage = false
		v[i].muscle_loss = false
		v[i].muscle_damage = false
		v[i].bone_loss = false
		v[i].bone_damage = false
		v[i].skin_damage = false
		v[i].motor_nerve_severed = false
		v[i].sensory_nerve_severed = false
	end
	
	if unit.job.current_job and unit.job.current_job.job_type == df.job_type.Rest then
		--print("Wake from rest -> clean self...")
		unit.job.current_job = df.job_type.CleanSelf
	end
end

