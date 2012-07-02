# remove bad thoughts for the selected unit or the whole fort

# with removebadthoughts -v, dump the bad thoughts types we removed
verbose = $script_args.delete('-v')

if u = df.unit_find(:selected)
	targets = [u]
else
	targets = df.unit_citizens
end

seenbad = Hash.new(0)

targets.each { |u|
	u.status.recent_events.each { |e|
		next if DFHack::UnitThoughtType::Value[e.type].to_s[0, 1] != '-'
		seenbad[e.type] += 1
		e.age = 0x1000_0000
	}
}

if verbose
	seenbad.sort_by { |k, v| v }.each { |k, v| puts " #{v} #{k}" }
end

count = seenbad.values.inject(0) { |s, v| s+v }
puts "removed #{count} bad thought#{'s' if count != 1}"
