# designate an area for digging according to a plan in csv format

fname = $script_args[0].to_s
print(fname)

if not $script_args[0] then 
    puts "  Usage: digfort <plan filename>"
    throw :script_finished
end
if not fname[-4..-1] == ".csv" then
    puts "  The plan file must be in .csv format."
    throw :script_finished
end
if not File.file?(fname) then
    puts "  The specified file does not exist."
    throw :script_finished
end

planfile = File.read(fname)

if df.cursor.x == -30000
    puts "place the game cursor to the top-left corner of the design"
    throw :script_finished
end

# a sample CSV file
# empty lines are ignored
# a special comment with start(dx, dy) means the actual patterns starts at cursor.x-dx, cursor.y-dy
# the CSV file should be saved in the main DF directory, alongside of Dwarf Fortress.exe
sample_csv = <<EOS
# start(3, 4)
 ,d,d,d,d,d,d
d, , , , , , ,d
d, , , , , , ,d
d, ,d, , ,d, ,d
h, , , , , , ,h
h,h,h,h,h,h,h,h
h,h, ,d,d, ,h,h
h,h,h,h,h,h,h,h
 ,h,h,h,h,h,h 
 , , ,h,h,h
 , , , ,h,h
EOS

offset = [0, 0]
tiles = []
planfile.each_line { |l|
    if l =~ /#.*start\s*\(\s*(-?\d+)\s*[,;]\s*(-?\d+)/
        raise "Error: multiple start() comments" if offset != [0, 0]
        offset = [$1.to_i, $2.to_i]
    end

    l = l.chomp.sub(/#.*/, '')
    next if l == ''
    tiles << l.split(/[;,]/).map { |t|
        t = t.strip
        (t[0] == ?") ? t[1..-2] : t
    }
}

x = x0 = df.cursor.x - offset[0]
y = df.cursor.y - offset[1]
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

puts '  done'
