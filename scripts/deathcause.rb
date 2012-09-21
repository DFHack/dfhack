# show death cause of a creature

def display_event(e)
	p e if $DEBUG

	str = "#{e.victim_tg.name} died in year #{e.year}"
	str << " of #{e.death_cause}" if false
	str << " killed by the #{e.slayer_race_tg.name[0]} #{e.slayer_tg.name}" if e.slayer != -1
	str << " using a #{df.world.raws.itemdefs.weapons[e.weapon.item_subtype].name}" if e.weapon.item_type == :WEAPON
	str << ", shot by a #{df.world.raws.itemdefs.weapons[e.weapon.bow_item_subtype].name}" if e.weapon.bow_item_type == :WEAPON

	puts str + '.'
end

item = df.item_find(:selected)

if !item or !item.kind_of?(DFHack::ItemBodyComponent)
	item = df.world.items.other[:ANY_CORPSE].find { |i| df.at_cursor?(i) }
end

if !item or !item.kind_of?(DFHack::ItemBodyComponent)
	puts "Please select a corpse in the loo'k' menu"
else
	hfig = item.hist_figure_id
	if hfig == -1
		puts "Not a historical figure, cannot find info"
	else
		events = df.world.history.events
		(0...events.length).reverse_each { |i|
			if events[i].kind_of?(DFHack::HistoryEventHistFigureDiedst) and events[i].victim == hfig
				display_event(events[i])
				break
			end
		}
	end
end

