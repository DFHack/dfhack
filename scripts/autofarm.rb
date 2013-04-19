class AutoFarm

	def initialize
		@thresholds = Hash.new(50)
		@lastcounts = Hash.new(0)
	end
	
	def setthreshold(id, v)
		list = df.world.raws.plants.all.find_all { |plt| plt.flags[:SEED] }.map { |plt| plt.id }
		if tok = df.match_rawname(id, list)
			@thresholds[tok] = v.to_i
		else
			puts "No plant with id #{id}, try one of " +
				list.map { |w| w =~ /[^\w]/ ? w.inspect : w }.sort.join(' ')
		end
	end
	
	def setdefault(v)
		@thresholds.default = v.to_i
	end
	
	def is_plantable(plant)
		has_seed = plant.flags[:SEED]
		season = df.cur_season
		harvest = df.cur_season_tick + plant.growdur * 10
		will_finish = harvest < 10080
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
				counts[i.mat_index] += i.stack_size
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
	
	def set_farms(plants, farms)
		return if farms.length == 0
		if plants.length == 0
			plants = [-1]
		end
		
		season = df.cur_season
		
		farms.each_with_index { |f, idx|
			f.plant_id[season] = plants[idx % plants.length]
		}
	end

	def process
		plantable = find_plantable_plants
		@lastcounts = Hash.new(0)
		
		df.world.items.other[:PLANT].each { |i|
			if (!i.flags.dump && !i.flags.forbid && !i.flags.garbage_collect &&
				!i.flags.hostile && !i.flags.on_fire && !i.flags.rotten &&
				!i.flags.trader && !i.flags.in_building && !i.flags.construction &&
				!i.flags.artifact && plantable.has_key?(i.mat_index))
				id = df.world.raws.plants.all[i.mat_index].id
				@lastcounts[id] += i.stack_size
			end
		}
				
		return unless @running

		plants_s = []
		plants_u = []
		
		plantable.each_key { |k|
			plant = df.world.raws.plants.all[k]
			if (@lastcounts[plant.id] < @thresholds[plant.id])
				plants_s.push(k) if plantable[k] == :Surface
				plants_u.push(k) if plantable[k] == :Underground
			end
		}
		
		farms_s = []
		farms_u = []
		df.world.buildings.other[:FARM_PLOT].each { |f|
			if (f.flags.exists)
				underground = df.map_designation_at(f.centerx,f.centery,f.z).subterranean
				farms_s.push(f) unless underground
				farms_u.push(f) if underground
			end
		}
		
		set_farms(plants_s, farms_s)
		set_farms(plants_u, farms_u)
	end
	
	def start
		return if @running
		@onupdate = df.onupdate_register('autofarm', 1200) { process }
		@running = true
	end
	
	def stop
		df.onupdate_unregister(@onupdate)
		@running = false
	end
	
	def status
		stat = @running ? "Running." : "Stopped."
		@lastcounts.each { |k,v|
			stat << "\n#{k} limit #{@thresholds.fetch(k, 'default')} current #{v}"
		}
		@thresholds.each { |k,v|
			stat << "\n#{k} limit #{v} current 0" unless @lastcounts.has_key?(k)
		}
		stat << "\nDefault: #{@thresholds.default}"
		stat
	end
		
end	

$AutoFarm ||= AutoFarm.new

case $script_args[0]
when 'start', 'enable'
	$AutoFarm.start
	puts $AutoFarm.status

when 'end', 'stop', 'disable'
	$AutoFarm.stop
	puts 'Stopped.'
	
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
		
when 'help', '?'
	puts <<EOS
Automatically handle crop selection in farm plots based on current plant stocks.
Selects a crop for planting if current stock is below a threshold.
Selected crops are dispatched on all farmplots.

Usage:
 autofarm start
 autofarm default 30
 autofarm threshold 150 helmet_plump tail_pig
EOS

else
	$AutoFarm.process
	puts $AutoFarm.status
end
