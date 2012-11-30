# show death cause of a creature

def display_death_event(e)
	str = "The #{e.victim_hf_tg.race_tg.name[0]} #{e.victim_hf_tg.name} died in year #{e.year}"
	str << " (cause: #{e.death_cause.to_s.downcase}),"
	str << " killed by the #{e.slayer_race_tg.name[0]} #{e.slayer_hf_tg.name}" if e.slayer_hf != -1
	str << " using a #{df.world.raws.itemdefs.weapons[e.weapon.item_subtype].name}" if e.weapon.item_type == :WEAPON
	str << ", shot by a #{df.world.raws.itemdefs.weapons[e.weapon.bow_item_subtype].name}" if e.weapon.bow_item_type == :WEAPON

	puts str.chomp(',') + '.'
end

item = df.item_find(:selected)

if !item or !item.kind_of?(DFHack::ItemBodyComponent)
	item = df.world.items.other[:ANY_CORPSE].find { |i| df.at_cursor?(i) }
end

if !item or !item.kind_of?(DFHack::ItemBodyComponent)
	puts "Please select a corpse in the loo'k' menu"
else
	hf = item.hist_figure_id
	if hf == -1
		# TODO try to retrieve info from the unit (u = item.unit_tg)
		puts "Not a historical figure, cannot death find info"
	else
		events = df.world.history.events
		found = false
		(0...events.length).reverse_each { |i|
			e = events[i]
			if e.kind_of?(DFHack::HistoryEventHistFigureDiedst) and e.victim_hf == hf
				display_death_event(e)
				found = true
				break
			end
		}
		if not found
			u = item.unit_tg
			puts "#{u.name} is not dead yet !" if u and not u.flags1.dead
		end
	end
end

