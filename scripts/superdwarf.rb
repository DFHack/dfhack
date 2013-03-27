# give super-dwarven speed to an unit

$superdwarf_onupdate ||= nil
$superdwarf_ids ||= []

case $script_args[0]
when 'add'
	if u = df.unit_find
		$superdwarf_ids |= [u.id]

		$superdwarf_onupdate ||= df.onupdate_register('superdwarf', 1) {
			if $superdwarf_ids.empty?
				df.onupdate_unregister($superdwarf_onupdate)
				$superdwarf_onupdate = nil
			else
				$superdwarf_ids.each { |id|
					if u = df.unit_find(id) and not u.flags1.dead
						# faster walk/work
						if u.counters.job_counter > 0
							u.counters.job_counter = 0
						end

						# no sleep
						if u.counters2.sleepiness_timer > 10000
							u.counters2.sleepiness_timer = 1
						end

						# no break
						if b = u.status.misc_traits.find { |t| t.id == :OnBreak }
							b.value = 500_000
						end
					else
						$superdwarf_ids.delete id
					end
				}
			end
		}
	else
		puts "Select a creature using 'v'"
	end

when 'del'
	if u = df.unit_find
		$superdwarf_ids.delete u.id
	else
		puts "Select a creature using 'v'"
	end

when 'clear'
	$superdwarf_ids.clear

when 'list'
	puts "current superdwarves:", $superdwarf_ids.map { |id| df.unit_find(id).name }

else
	puts "Usage:",
		" - superdwarf add: give superspeed to currently selected creature",
		" - superdwarf del: remove superspeed to current creature",
		" - superdwarf clear: remove all superpowers",
		" - superdwarf list: list super-dwarves"
end
