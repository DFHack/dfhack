# scan the map for ore veins
=begin

locate-ore
==========
Scan the map for metal ores.

Finds and designate for digging one tile of a specific metal ore.
Only works for native metal ores, does not handle reaction stuff (eg STEEL).

When invoked with the ``list`` argument, lists metal ores available on the map.

Examples::

    locate-ore list
    locate-ore hematite
    locate-ore iron

=end

target_ore = $script_args[0]

def find_all_ore_veins
    puts 'scanning map...'
    $ore_veins = {}
    seen_mat = {}
    df.each_map_block { |block|
        block.block_events.grep(DFHack::BlockSquareEventMineralst).each { |vein|
            mat_index = vein.inorganic_mat
            if not seen_mat[mat_index] or $ore_veins[mat_index]
                seen_mat[mat_index] = true
                if df.world.raws.inorganics[mat_index].flags[:METAL_ORE]
                    $ore_veins[mat_index] ||= []
                    $ore_veins[mat_index] << [block.map_pos.x, block.map_pos.y, block.map_pos.z]
                end
            end
        }
    }

    df.onstatechange_register_once { |st|
        if st == :MAP_LOADED
            $ore_veins = nil    # invalidate veins cache
            true
        end
    }

    $ore_veins
end

$ore_veins ||= find_all_ore_veins

if not target_ore or target_ore == 'help'
    puts <<EOS
Scan the map to find one random tile of unmined ore.
It will center the game view on that tile and mark it for digging.
Only works with metal ores.

  Usage:
locate_ore list       list all existing vein materials (including mined ones)
locate_ore hematite   find one tile of unmined hematite ore
locate_ore iron       find one tile of unmined ore you can smelt into iron
EOS

elsif target_ore and mats = $ore_veins.keys.find_all { |k|
    ino = df.world.raws.inorganics[k]
    ino.id =~ /#{target_ore}/i or ino.metal_ore.mat_index.find { |m|
        df.world.raws.inorganics[m].id =~ /#{target_ore}/i
    }
} and not mats.empty?
    pos = nil
    dxs = (0..15).sort_by { rand }
    dys = (0..15).sort_by { rand }
    if found_mat = mats.sort_by { rand }.find { |mat|
        $ore_veins[mat].sort_by { rand }.find { |bx, by, bz|
            dys.find { |dy|
                dxs.find { |dx|
                    tile = df.map_tile_at(bx+dx, by+dy, bz)
                    if tile.tilemat == :MINERAL and tile.designation.dig == :No and tile.shape == :WALL and
                            tile.mat_index_vein == mat and
                            # ignore map borders
                            bx+dx > 0 and bx+dx < df.world.map.x_count-1 and by+dy > 0 and by+dy < df.world.map.y_count-1
                        pos = [bx+dx, by+dy, bz]
                    end
                }
            }
        }
    }
        df.center_viewscreen(*pos)
        df.map_tile_at(*pos).dig
        puts "Here is some #{df.world.raws.inorganics[found_mat].id}"
    else
        puts "Cannot find unmined #{mats.map { |mat| df.world.raws.inorganics[mat].id }.join(', ')}"
    end

else
    puts "Available ores:", $ore_veins.sort_by { |mat, pos| pos.length }.map { |mat, pos|
        ore = df.world.raws.inorganics[mat]
        metals = ore.metal_ore.mat_index.map { |m| df.world.raws.inorganics[m] }
        '  ' + ore.id.downcase + ' (' + metals.map { |m| m.id.downcase }.join(', ') + ')'
    }

end
