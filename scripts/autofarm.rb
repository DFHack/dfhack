class AutoFarm

	def initialize
		@thresholds = Hash.new(50)
		@lastcounts = Hash.new(0)
	end
	
	def setthreshold(id, v)
		if df.world.raws.plants.all.find { |r| r.id == id }
			@thresholds[id] = v.to_i
		else
			puts "No plant with id #{id}"
		end
	end
	
	def setdefault(v)
		@thresholds.default = v.to_i
	end
	
	def is_plantable (plant)
		has_seed = plant.flags[:SEED]
		season = df.cur_season
		harvest = df.cur_season_tick + plant.growdur * 10
		will_finish =  harvest < 10080
		can_plant = has_seed && plant.flags[season]
		can_plant = can_plant && (will_finish || plant.flags[(season+1)%4])
		can_plant
	end
	
	def find_plantable_plants
		plantable = {}
		counts = Hash.new(0)
		
		df.world.items.other[:SEEDS].each { |i|
			if (!i.flags.dump && !i.flags.forbid && !i.flags.garbage_collect &&
				!i.flags.hostile && !i.flags.on_fire && !i.flags.rotten &&
				!i.flags.trader && !i.flags.in_building && !i.flags.construction &&
				!i.flags.artifact)
				counts[i.mat_index] = counts[i.mat_index] + i.stack_size
			end
		}

		counts.keys.each { |i|
			if df.ui.tasks.known_plants[i]
				plant = df.world.raws.plants.all[i]
				if is_plantable(plant)
					plantable[i] = :Surface     if (plant.underground_depth_min == 0 || plant.underground_depth_max == 0)
					plantable[i] = :Underground if (plant.underground_depth_min > 0 || plant.underground_depth_max > 0)
				end
			end
		}
		
		return plantable
	end
	
	def set_farms( plants, farms)
		return if farms.length == 0
		if plants.length == 0
			plants = [-1]
		end
		
		season = df.cur_season
		
		idx = 0
		
		farms.each { |f|
			f.plant_id[season] = plants[idx]
			idx = (idx + 1) % plants.length
		}
	end

	def process
		return false unless @running
		
		plantable = find_plantable_plants
		counts = Hash.new(0)
		
		df.world.items.other[:PLANT].each { |i|
			if (!i.flags.dump && !i.flags.forbid && !i.flags.garbage_collect &&
				!i.flags.hostile && !i.flags.on_fire && !i.flags.rotten &&
				!i.flags.trader && !i.flags.in_building && !i.flags.construction &&
				!i.flags.artifact && plantable.has_key?(i.mat_index))
				counts[i.mat_index] = counts[i.mat_index] + i.stack_size
			end
		}
				
		plants_s = []
		plants_u = []
		
		@lastcounts.clear
		
		plantable.each_key { |k|
			plant = df.world.raws.plants.all[k]
			if (counts[k] < @thresholds[plant.id])
				plants_s.push(k) if plantable[k] == :Surface
				plants_u.push(k) if plantable[k] == :Underground
			end
			@lastcounts[plant.id] = counts[k]
		}
		
		farms_s = []
		farms_u = []
		df.world.buildings.other[:FARM_PLOT].each { |f|
			if (f.flags.exists)
				outside = df.map_designation_at(f.centerx,f.centery,f.z).outside
				farms_s.push(f) if outside
				farms_u.push(f) unless outside
			end
		}
		
		set_farms(plants_s, farms_s)
		set_farms(plants_u, farms_u)
		
	end
	
	def start
		@onupdate = df.onupdate_register('autofarm', 100) { process }
		@running = true
	end
	
	def stop
		df.onupdate_unregister(@onupdate)
		@running = false
	end
	
	def status
		stat = @running ? "Running." : "Stopped."
		@thresholds.each { |k,v|
			stat += "\n#{k} limit #{v} current #{@lastcounts[k]}"
		}
		stat += "\nDefault: #{@thresholds.default}"
		stat
	end
		
end	

$AutoFarm = AutoFarm.new unless $AutoFarm

case $script_args[0]
when 'start'
    $AutoFarm.start

when 'end', 'stop'
    $AutoFarm.stop
	
when 'default'
	$AutoFarm.setdefault($script_args[1])
	
when 'threshold'
	t = $script_args[1]
	$script_args[2..-1].each {|i|
		$AutoFarm.setthreshold(i, t)
	}
	
when 'delete'
	$AutoFarm.stop
	$AutoFarm = nil
		
else
    if $AutoFarm
        puts $AutoFarm.status
    else
        puts "AI not started"
    end
end
