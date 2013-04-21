# remove all aquifers from the map

count = 0
df.each_map_block { |b|
	if b.designation[0][0].water_table or b.designation[8][8].water_table
		count += 1
		df.each_map_block_z(b.map_pos.z) { |bz|
			bz.designation.each { |dx| dx.each { |dy| dy.water_table = false } }
		}
	end
}

puts "cleared #{count} aquifer#{'s' if count > 1}"
