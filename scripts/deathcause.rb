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
unit = df.unit_find(:selected)

if !item or !item.kind_of?(DFHack::ItemBodyComponent)
	item = df.world.items.other[:ANY_CORPSE].find { |i| df.at_cursor?(i) }
end

if item and item.kind_of?(DFHack::ItemBodyComponent)
	hf = item.hist_figure_id
elsif unit
	hf = unit.hist_figure_id
end

if not hf
	puts "Please select a corpse in the loo'k' menu, or an unit in the 'u'nitlist screen"

elsif hf == -1
	# TODO try to retrieve info from the unit (u = item.unit_tg)
	puts "Not a historical figure, cannot death find info"

else
	histfig = df.world.history.figures.binsearch(hf)
	unit = histfig ? df.unit_find(histfig.unit_id) : nil
	if unit and not unit.flags1.dead and not unit.flags3.ghostly
		puts "#{unit.name} is not dead yet !"

	else
		events = df.world.history.events
		(0...events.length).reverse_each { |i|
			e = events[i]
			if e.kind_of?(DFHack::HistoryEventHistFigureDiedst) and e.victim_hf == hf
				display_death_event(e)
				break
			end
		}
	end
end

