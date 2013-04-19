# pit all caged creatures in a zone

case $script_args[0]
when '?', 'help'
	puts <<EOS
Run this script with the cursor on top of a pit/pond activity zone, or with a zone identifier as argument.
It will mark all caged creatures on tiles covered by the zone to be dumped.
Works best with an animal stockpile on top of the pit/pond zone.
EOS
	throw :script_finished

when /(\d+)/
	nr = $1.to_i
	bld = df.world.buildings.other[:ACTIVITY_ZONE].find { |zone| zone.zone_num == nr }

else
	bld = df.world.buildings.other[:ACTIVITY_ZONE].find { |zone|
		zone.zone_flags.pit_pond and zone.z == df.cursor.z and 
		zone.x1 <= df.cursor.x and zone.x2 >= df.cursor.x and zone.y1 <= df.cursor.y and zone.y2 >= df.cursor.y
	}

end

if not bld
	puts "Please select a pit/pond zone"
	throw :script_finished
end

found = 0
df.world.items.other[:CAGE].each { |cg|
	next if not cg.flags.on_ground
	next if cg.pos.z != bld.z or cg.pos.x < bld.x1 or cg.pos.x > bld.x2 or cg.pos.y < bld.y1 or cg.pos.y > bld.y2
	next if not uref = cg.general_refs.grep(DFHack::GeneralRefContainsUnitst).first
	found += 1
	u = uref.unit_tg
	puts "Pitting #{u.race_tg.name[0]} #{u.id} #{u.name}"
	u.general_refs << DFHack::GeneralRefBuildingCivzoneAssignedst.cpp_new(:building_id => bld.id)
	bld.assigned_creature << u.id
}
puts "No creature available for pitting" if found == 0
