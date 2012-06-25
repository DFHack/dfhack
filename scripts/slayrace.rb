# slay all creatures of a given race

race = $script_args[0]

all_races = df.world.units.active.map { |u|
	u.race_tg.creature_id if not u.flags1.dead and not df.map_designation_at(u).hidden
}.compact.uniq.sort

if !race
	puts all_races
else
	raw_race = df.match_rawname(race, all_races)
	raise 'invalid race' if not raw_race

	race_nr = df.world.raws.creatures.all.index { |cr| cr.creature_id == raw_race }
	count = 0

	df.world.units.active.each { |u|
		if u.race == race_nr and u.body.blood_count != 0 and not u.flags1.dead
			u.body.blood_count = 0
			count += 1
		end
	}

	puts "slain #{count} #{raw_race}"
end
