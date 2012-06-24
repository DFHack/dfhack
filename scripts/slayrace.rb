# slay all creatures of a given race (default = goblins)

race = $script_args[0] || 'GOBLIN'

all_races = df.world.raws.creatures.all.map { |cr| cr.creature_id }
raw_race = df.match_rawname(race, all_races)
raise 'invalid race' if not raw_race

race_nr = df.world.raws.creatures.all.index { |cr| cr.creature_id == raw_race }
count = 0

df.suspend {
	df.world.units.active.each { |u|
		if u.race == race_nr and u.body.blood_count != 0
			u.body.blood_count = 0
			count += 1
		end
	}
}

puts "slain #{count} #{raw_race}"
