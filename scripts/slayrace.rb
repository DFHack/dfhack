# slay all creatures of a given race

# race = name of the race to eradicate, use 'him' to target only the selected creature
# use 'undead' to target all undeads
race = $script_args[0]

# if the 2nd parameter is 'magma', magma rain for the targets instead of instant death
magma = ($script_args[1] == 'magma')

checkunit = lambda { |u|
	(u.body.blood_count != 0 or u.body.blood_max == 0) and
	not u.flags1.dead and
	not u.flags1.caged and not u.flags1.chained and
	#not u.flags1.hidden_in_ambush and
	not df.map_designation_at(u).hidden
}

slayit = lambda { |u|
	if not magma
		# just make them drop dead
		u.body.blood_count = 0
		# some races dont mind having no blood, ensure they are still taken care of.
		u.animal.vanish_countdown = 2
	else
		# it's getting hot around here
		# !!WARNING!! do not call on a magma-safe creature
		ouh = df.onupdate_register("slayrace ensure #{u.id}", 1) {
			if u.flags1.dead
				df.onupdate_unregister(ouh)
			else
				x, y, z = u.pos.x, u.pos.y, u.pos.z
				z += 1 while tile = df.map_tile_at(x, y, z+1) and
					tile.shape_passableflow and tile.shape_passablelow
				df.map_tile_at(x, y, z).spawn_magma(7)
			end
		}
	end
}

all_races = Hash.new(0)

df.world.units.active.map { |u|
	if checkunit[u]
		if (u.enemy.undead or
		    (u.curse.add_tags1.OPPOSED_TO_LIFE and not
		     u.curse.rem_tags1.OPPOSED_TO_LIFE))
			all_races['Undead'] += 1
		else
			all_races[u.race_tg.creature_id] += 1
		end
	end
}

case race
when nil
	all_races.sort_by { |race, cnt| [cnt, race] }.each{ |race, cnt| puts " #{race} #{cnt}" }

when 'him'
	if him = df.unit_find
		slayit[him]
	else
		puts "Select a target ingame"
	end

when /^undead/i
	count = 0
	df.world.units.active.each { |u|
		if (u.enemy.undead or
		    (u.curse.add_tags1.OPPOSED_TO_LIFE and not
		     u.curse.rem_tags1.OPPOSED_TO_LIFE)) and
		   checkunit[u]
			slayit[u]
			count += 1
		end
	}
	puts "slain #{count} undeads"

else
	raw_race = df.match_rawname(race, all_races.keys)
	raise 'invalid race' if not raw_race

	race_nr = df.world.raws.creatures.all.index { |cr| cr.creature_id == raw_race }

	count = 0
	df.world.units.active.each { |u|
		if u.race == race_nr and checkunit[u]
			slayit[u]
			count += 1
		end
	}
	puts "slain #{count} #{raw_race}"

end
