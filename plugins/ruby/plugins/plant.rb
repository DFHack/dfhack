module DFHack
def self.each_tree(material=:any)
	@raw_tree_name ||= {}
	if @raw_tree_name.empty?
		df.world.raws.plants.all.each_with_index { |p, idx|
			@raw_tree_name[idx] = p.id if p.flags[PlantRawFlags::TREE]
		}
	end

	if material != :any
		mat = match_rawname(material, @raw_tree_name.values)
		unless wantmat = @raw_tree_name.index(mat)
			raise "invalid tree material #{material}"
		end
	end

	world.plants.all.each { |plant|
		next if not @raw_tree_name[plant.material]
		next if wantmat and plant.material != wantmat
		yield plant
	}
end

def self.each_shrub(material=:any)
	@raw_shrub_name ||= {}
	if @raw_tree_name.empty?
		df.world.raws.plants.all.each_with_index { |p, idx|
			@raw_shrub_name[idx] = p.id if not p.flags[PlantRawFlags::GRASS] and not p.flags[PlantRawFlags::TREE]
		}
	end

	if material != :any
		mat = match_rawname(material, @raw_shrub_name.values)
		unless wantmat = @raw_shrub_name.index(mat)
			raise "invalid shrub material #{material}"
		end
	end
end

SaplingToTreeAge = 120960
def self.cuttrees(material=nil, count_max=100)
	if !material
		# list trees
		cnt = Hash.new(0)
		each_tree { |plant|
			next if plant.grow_counter < SaplingToTreeAge
			next if find_map_designation(plant).hidden
			cnt[plant.material] += 1
		}
		cnt.sort_by { |mat, c| c }.each { |mat, c|
			name = @raw_tree_name[mat]
			puts " #{name} #{c}"
		}
	else
		cnt = 0
		each_tree(material) { |plant|
			next if plant.grow_counter < SaplingToTreeAge
			b = find_map_block(plant)
			d = b.designation[plant.pos.x%16][plant.pos.y%16]
			next if d.hidden
			if d.dig == TileDigDesignation::No
				d.dig = TileDigDesignation::Default
				b.flags.designated = true
				cnt += 1
				break if cnt == count_max
			end
		}
		puts "Updated #{cnt} plant designations"
	end
end

def self.growtrees(material=nil, count_max=100)
	if !material
		# list plants
		cnt = Hash.new(0)
		each_tree { |plant|
			next if plant.grow_counter >= SaplingToTreeAge
			next if find_map_designation(plant).hidden
			cnt[plant.material] += 1
		}
		cnt.sort_by { |mat, c| c }.each { |mat, c|
			name = @raw_tree_name[mat]
			puts " #{name} #{c}"
		}
	else
		cnt = 0
		each_tree(material) { |plant|
			next if plant.grow_counter >= SaplingToTreeAge
			next if find_map_designation(plant).hidden
			p.grow_counter = SaplingToTreeAge
			cnt += 1
			break if cnt == count_max
		}
		puts "Grown #{cnt} sapling"
	end
end
end
