# script to fix loyalty cascade, when you order your militia to kill friendly units

def fixunit(unit)
	return if unit.race != df.ui.race_id or unit.civ_id != df.ui.civ_id
	links = unit.hist_figure_tg.entity_links
	fixed = false

	# check if the unit is a civ renegade
	if i1 = links.index { |l|
		l.kind_of?(DFHack::HistfigEntityLinkFormerMemberst) and
		l.entity_id == df.ui.civ_id
	} and i2 = links.index { |l|
		l.kind_of?(DFHack::HistfigEntityLinkEnemyst) and
		l.entity_id == df.ui.civ_id
	}
		fixed = true
		i1, i2 = i2, i1 if i1 > i2
		links.delete_at i2
		links.delete_at i1
		links << DFHack::HistfigEntityLinkMemberst.cpp_new(:entity_id => df.ui.civ_id, :link_strength => 100)
		df.add_announcement "fixloyalty: #{unit.name} is now a member of #{df.ui.civ_tg.name} again"
	end

	# check if the unit is a group renegade
	if i1 = links.index { |l|
		l.kind_of?(DFHack::HistfigEntityLinkFormerMemberst) and
		l.entity_id == df.ui.group_id
	} and i2 = links.index { |l|
		l.kind_of?(DFHack::HistfigEntityLinkEnemyst) and
		l.entity_id == df.ui.group_id
	}
		fixed = true
		i1, i2 = i2, i1 if i1 > i2
		links.delete_at i2
		links.delete_at i1
		links << DFHack::HistfigEntityLinkMemberst.cpp_new(:entity_id => df.ui.group_id, :link_strength => 100)
		df.add_announcement "fixloyalty: #{unit.name} is now a member of #{df.ui.group_tg.name} again"
	end

	# fix the 'is an enemy' cache matrix (mark to be recalculated by the game when needed)
	if fixed and unit.enemy.enemy_status_slot != -1
		i = unit.enemy.enemy_status_slot
		unit.enemy.enemy_status_slot = -1
		cache = df.world.enemy_status_cache
		cache.slot_used[i] = false
		cache.rel_map[i].map! { -1 }
		cache.rel_map.each { |a| a[i] = -1 }
		cache.next_slot = i if cache.next_slot > i
	end

	# return true if we actually fixed the unit
	fixed
end

count = 0
df.unit_citizens.each { |u|
	count += 1 if fixunit(u)
}

if count > 0
	puts "loyalty cascade fixed (#{count} dwarves)"
else
	puts "no loyalty cascade found"
end
