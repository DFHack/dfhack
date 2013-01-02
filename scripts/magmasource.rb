# create an infinite magma source at the cursor

$magma_sources ||= []

case $script_args[0]
when 'here'
    $magma_onupdate ||= df.onupdate_register('magmasource', 12) {
        # called every 12 game ticks (100x a dwarf day)
        if $magma_sources.empty?
            df.onupdate_unregister($magma_onupdate)
            $magma_onupdate = nil
        end

        $magma_sources.each { |x, y, z|
            if tile = df.map_tile_at(x, y, z) and tile.shape_passableflow
                des = tile.designation
                tile.spawn_magma(des.flow_size + 1) if des.flow_size < 7
            end
        }
    }

    if df.cursor.x != -30000
        if tile = df.map_tile_at(df.cursor)
            if tile.shape_passableflow
                $magma_sources << [df.cursor.x, df.cursor.y, df.cursor.z]
            else
                puts "Impassable tile: I'm afraid I can't do that, Dave"
            end
        else
            puts "Unallocated map block - build something here first"
        end
    else
        puts "Please put the game cursor where you want a magma source"
    end

when 'delete-here'
    $magma_sources.delete [df.cursor.x, df.cursor.y, df.cursor.z]

when 'stop'
    $magma_sources.clear

else
    puts <<EOS
Creates a new infinite magma source at the cursor.

Arguments:
 here - create a new source at the current cursor position
        (call multiple times for higher flow)
 delete-here - delete the source under the cursor
 stop - delete all created magma sources
EOS

    if $magma_sources.first
        puts '', 'Current magma sources:', $magma_sources.map { |s| " #{s.inspect}" }
    end
end
