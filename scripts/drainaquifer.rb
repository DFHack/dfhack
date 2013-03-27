# remove all aquifers from the map

count = 0
df.each_map_block { |b|
	if b.designation[0][0].water_table or b.designation[15][15].water_table
		count += 1
		b.designation.each { |dx| dx.each { |dy| dy.water_table = false } }
	end
}

puts "cleared #{count} map blocks"
