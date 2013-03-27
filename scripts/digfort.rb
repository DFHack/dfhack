# designate an area for digging according to a plan in csv format

raise "usage: digfort <plan filename>" if not $script_args[0]
planfile = File.read($script_args[0])

if df.cursor.x == -30000
	raise "place the game cursor to the top-left corner of the design"
end

tiles = planfile.lines.map { |l|
	l.sub(/#.*/, '').split(';').map { |t| t.strip }
}

x = x0 = df.cursor.x
y = df.cursor.y
z = df.cursor.z

tiles.each { |line|
	next if line.empty? or line == ['']
	line.each { |tile|
		t = df.map_tile_at(x, y, z)
		s = t.shape_basic
		case tile
		when 'd'; t.dig(:Default) if s == :Wall
		when 'u'; t.dig(:UpStair) if s == :Wall
		when 'j'; t.dig(:DownStair) if s == :Wall or s == :Floor
		when 'i'; t.dig(:UpDownStair) if s == :Wall
		when 'h'; t.dig(:Channel) if s == :Wall or s == :Floor
		when 'r'; t.dig(:Ramp) if s == :Wall
		when 'x'; t.dig(:No)
		end
		x += 1
	}
	x = x0
	y += 1
}

puts 'done'
