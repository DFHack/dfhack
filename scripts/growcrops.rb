# grow crops in farm plots. ex: growcrops helmet_plump 20

material = $script_args[0]
count_max = $script_args[1].to_i
count_max = 100 if count_max == 0

# cache information from the raws
@raws_plant_name ||= {}
@raws_plant_growdur ||= {}
if @raws_plant_name.empty?
	df.world.raws.plants.all.each_with_index { |p, idx|
		@raws_plant_name[idx] = p.id
		@raws_plant_growdur[idx] = p.growdur
	}
end

if !material or material == 'help' or material == 'list'
	# show a list of available crop types
	cnt = Hash.new(0)
	df.suspend {
		df.world.items.other[:SEEDS].each { |seed|
			next if not seed.flags.in_building
			next if not seed.itemrefs.find { |ref| ref._rtti_classname == :general_ref_building_holderst }
			next if seed.grow_counter >= @raws_plant_growdur[seed.mat_index]
			cnt[seed.mat_index] += 1
		}
	}

	cnt.sort_by { |mat, c| c }.each { |mat, c|
		name = df.world.raws.plants.all[mat].id
		puts " #{name} #{c}"
	}

else

	mat = df.match_rawname(material, @raws_plant_name.values)
	unless wantmat = @raws_plant_name.index(mat)
		raise "invalid plant material #{material}"
	end

	cnt = 0
	df.suspend {
		df.world.items.other[:SEEDS].each { |seed|
			next if seed.mat_index != wantmat
			next if not seed.flags.in_building
			next if not seed.itemrefs.find { |ref| ref._rtti_classname == :general_ref_building_holderst }
			next if seed.grow_counter >= @raws_plant_growdur[seed.mat_index]
			seed.grow_counter = @raws_plant_growdur[seed.mat_index]
			cnt += 1
		}
	}
	puts "Grown #{cnt} #{mat}"
end
