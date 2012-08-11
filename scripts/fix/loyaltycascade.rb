# script to fix loyalty cascade, when you order your militia to kill friendly units

def fixunit(u)
	return if u.race != df.ui.race_id or u.civ_id != df.ui.civ_id
	links = u.hist_figure_tg.entity_links
	fixed = false

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
		df.add_announcement "fixloyalty: #{u.name} is now a member of #{df.ui.civ_tg.name} again"
	end

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
		df.add_announcement "fixloyalty: #{u.name} is now a member of #{df.ui.group_tg.name} again"
	end

	fixed
end

fixed = 0
df.unit_citizens.each { |u|
	fixed += 1 if fixunit(u)
}

if fixed > 0
	df.popup_announcement "Fixed a loyalty cascade, you should save and reload now"
	puts "loyalty cascade fixed (#{fixed} dwarves), you should save and reload"
else
	puts "no loyalty cascade found"
end
