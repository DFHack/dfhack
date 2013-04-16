# create an infinite magma/water source/drain at the cursor

$sources ||= []

cur_source = {
	:liquid => 'water',
	:amount => 7,
	:pos => [df.cursor.x, df.cursor.y, df.cursor.z]
}
cmd = 'help'

$script_args.each { |a|
	case a.downcase
	when 'water', 'magma'
		cur_source[:liquid] = a.downcase
	when /^\d+$/
		cur_source[:amount] = a.to_i
	when 'add', 'del', 'delete', 'clear', 'help', 'list'
		cmd = a.downcase
	else
		puts "source: unhandled argument #{a}"
	end
}

case cmd
when 'add'
	$sources_onupdate ||= df.onupdate_register('sources', 12) {
		# called every 12 game ticks (100x a dwarf day)
		$sources.each { |s|
			if tile = df.map_tile_at(*s[:pos]) and tile.shape_passableflow
				# XXX does not check current liquid_type
				des = tile.designation
				cur = des.flow_size
				if cur != s[:amount]
					tile.spawn_liquid((cur > s[:amount] ? cur-1 : cur+1), s[:liquid] == 'magma')
				end
			end
		}
		if $sources.empty?
			df.onupdate_unregister($sources_onupdate)
			$sources_onupdate = nil
		end
	}

	if cur_source[:pos][0] >= 0
		if tile = df.map_tile_at(*cur_source[:pos])
			if tile.shape_passableflow
				$sources << cur_source
			else
				puts "Impassable tile: I'm afraid I can't do that, Dave"
			end
		else
			puts "Unallocated map block - build something here first"
		end
	else
		puts "Please put the game cursor where you want a source"
	end

when 'del', 'delete'
	$sources.delete_if { |s| s[:pos] == cur_source[:pos] }

when 'clear'
	$sources.clear

when 'list'
	puts "Source list:", $sources.map { |s|
		" #{s[:pos].inspect} #{s[:liquid]} #{s[:amount]}"
	}
	puts "Current cursor pos: #{[df.cursor.x, df.cursor.y, df.cursor.z].inspect}" if df.cursor.x >= 0

else
	puts <<EOS
Creates a new infinite liquid source at the cursor.

Examples:
  source add water     - create a water source under cursor
  source add water 0   - create a water drain
  source add magma 5   - create a magma source, up to 5/7 deep
  source delete        - delete source under cursor
  source clear         - remove all sources
  source list
EOS
end
