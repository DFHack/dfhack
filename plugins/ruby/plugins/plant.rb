module DFHack
def self.each_tree(material=:any)
	@raws_tree_name ||= {}
	if @raws_tree_name.empty?
		df.world.raws.plants.all.each_with_index { |p, idx|
			@raws_tree_name[idx] = p.id if p.flags[:TREE]
		}
	end

	if material != :any
		mat = match_rawname(material, @raws_tree_name.values)
		unless wantmat = @raws_tree_name.index(mat)
			raise "invalid tree material #{material}"
		end
	end

	world.plants.all.each { |plant|
		next if not @raws_tree_name[plant.material]
		next if wantmat and plant.material != wantmat
		yield plant
	}
end

def self.each_shrub(material=:any)
	@raws_shrub_name ||= {}
	if @raws_tree_name.empty?
		df.world.raws.plants.all.each_with_index { |p, idx|
			@raws_shrub_name[idx] = p.id if not p.flags[:GRASS] and not p.flags[:TREE]
		}
	end

	if material != :any
		mat = match_rawname(material, @raws_shrub_name.values)
		unless wantmat = @raws_shrub_name.index(mat)
			raise "invalid shrub material #{material}"
		end
	end
end

SaplingToTreeAge = 120960
def self.cuttrees(material=nil, count_max=100)
	if !material
		# list trees
		cnt = Hash.new(0)
		suspend {
			each_tree { |plant|
				next if plant.grow_counter < SaplingToTreeAge
				next if map_designation_at(plant).hidden
				cnt[plant.material] += 1
			}
		}
		cnt.sort_by { |mat, c| c }.each { |mat, c|
			name = @raws_tree_name[mat]
			puts " #{name} #{c}"
		}
	else
		cnt = 0
		suspend {
			each_tree(material) { |plant|
				next if plant.grow_counter < SaplingToTreeAge
				b = map_block_at(plant)
				d = b.designation[plant.pos.x%16][plant.pos.y%16]
				next if d.hidden
				if d.dig == :No
					d.dig = :Default
					b.flags.designated = true
					cnt += 1
					break if cnt == count_max
				end
			}
		}
		puts "Updated #{cnt} plant designations"
	end
end

def self.growtrees(material=nil, count_max=100)
	if !material
		# list plants
		cnt = Hash.new(0)
		suspend {
			each_tree { |plant|
				next if plant.grow_counter >= SaplingToTreeAge
				next if map_designation_at(plant).hidden
				cnt[plant.material] += 1
			}
		}
		cnt.sort_by { |mat, c| c }.each { |mat, c|
			name = @raws_tree_name[mat]
			puts " #{name} #{c}"
		}
	else
		cnt = 0
		suspend {
			each_tree(material) { |plant|
				next if plant.grow_counter >= SaplingToTreeAge
				next if map_designation_at(plant).hidden
				plant.grow_counter = SaplingToTreeAge
				cnt += 1
				break if cnt == count_max
			}
		}
		puts "Grown #{cnt} saplings"
	end
end

def self.growcrops(material=nil, count_max=100)
	@raws_plant_name ||= {}
	@raws_plant_growdur ||= {}
	if @raws_plant_name.empty?
		df.world.raws.plants.all.each_with_index { |p, idx|
			@raws_plant_name[idx] = p.id
			@raws_plant_growdur[idx] = p.growdur
		}
	end

	if !material
		cnt = Hash.new(0)
		suspend {
			world.items.other[:SEEDS].each { |seed|
				next if not seed.flags.in_building
				next if not seed.itemrefs.find { |ref| ref._rtti_classname == :general_ref_building_holderst }
				next if seed.grow_counter >= @raws_plant_growdur[seed.mat_index]
				cnt[seed.mat_index] += 1
			}
		}
		cnt.sort_by { |mat, c| c }.each { |mat, c|
			name = world.raws.plants.all[mat].id
			puts " #{name} #{c}"
		}
	else
		if material != :any
			mat = match_rawname(material, @raws_plant_name.values)
			unless wantmat = @raws_plant_name.index(mat)
				raise "invalid plant material #{material}"
			end
		end

		cnt = 0
		suspend {
			world.items.other[:SEEDS].each { |seed|
				next if wantmat and seed.mat_index != wantmat
				next if not seed.flags.in_building
				next if not seed.itemrefs.find { |ref| ref._rtti_classname == :general_ref_building_holderst }
				next if seed.grow_counter >= @raws_plant_growdur[seed.mat_index]
				seed.grow_counter = @raws_plant_growdur[seed.mat_index]
				cnt += 1
			}
		}
		puts "Grown #{cnt} crops"
	end
end
end
