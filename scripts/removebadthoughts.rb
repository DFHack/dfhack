# remove bad thoughts for the selected unit or the whole fort

dry_run = $script_args.delete('--dry-run') || $script_args.delete('-n')

$script_args << 'all' if dry_run and $script_args.empty?

seenbad = Hash.new(0)

clear_mind = lambda { |u|
	u.status.recent_events.each { |e|
		next if DFHack::UnitThoughtType::Value[e.type].to_s[0, 1] != '-'
		seenbad[e.type] += 1
		e.age = 0x1000_0000 unless dry_run
	}
}

summary = lambda {
	seenbad.sort_by { |thought, cnt| cnt }.each { |thought, cnt|
		puts " #{thought} #{cnt}"
	}
	count = seenbad.values.inject(0) { |sum, cnt| sum+cnt }
	puts "Removed #{count} bad thought#{'s' if count != 1}." if count > 0 and not dry_run
}

case $script_args[0]
when 'him'
	if u = df.unit_find
		clear_mind[u]
		summary[]
	else
		puts 'Please select a dwarf ingame'
	end
	
when 'all'
	df.unit_citizens.each { |uu|
		clear_mind[uu]
	}
	summary[]

else
	puts "Usage: removebadthoughts [--dry-run] <him|all>"

end
