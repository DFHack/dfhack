# fix doors that are frozen in 'open' state

# door is stuck in open state if the map occupancy flag incorrectly indicates
# that an unit is present (and creatures will prone to pass through)

count = 0
df.world.buildings.all.each { |bld|
	# for all doors
	next if bld._rtti_classname != :building_doorst
	# check if it is open
	next if bld.close_timer == 0
	# check if occupancy is set
	occ = df.map_occupancy_at(bld.x1, bld.y1, bld.z)
	next if not occ.unit
	# check if an unit is present
	next if df.world.units.active.find { |u| u.pos.x == bld.x1 and u.pos.y == bld.y1 and u.pos.z == bld.z }
	count += 1
	occ.unit = false
}
puts "unstuck #{count} doors"
