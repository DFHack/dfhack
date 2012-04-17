require 'hack/ruby-autogen'

module DFHack
    class << self
        def suspend
            if block_given?
                begin
                    do_suspend
                    yield
                ensure
                    resume
                end
            else
                do_suspend
            end
        end

        def puts(*a)
            a.flatten.each { |l|
                print_str(l.to_s.chomp + "\n")
            }
            nil
        end

        def puts_err(*a)
            a.flatten.each { |l|
                print_err(l.to_s.chomp + "\n")
            }
            nil
        end

        # return an Unit
        # with no arg, return currently selected unit in df UI (v or k menu)
        # with numeric arg, search unit by unit.id
        # with an argument that respond to x/y/z (eg cursor), find first unit at this position
        def find_unit(what=nil)
            if what == nil
                case ui.main.mode
                when UiSidebarMode::ViewUnits
                    # nobody selected => idx == 0
                    v = world.units.other[0][ui_selected_unit]
                    v if v and v.z == cursor.z
                when UiSidebarMode::LookAround
                    k = ui_look_list.items[ui_look_cursor]
                    k.unit if k.type == MemHack::UiLookList::Unit
                end
            elsif what.kind_of?(Integer)
                world.units.all.find { |u| u.id == what }
            elsif what.respond_to?(:x) or what.respond_to?(:pos)
                what = what.pos if what.respond_to?(:pos)
                x, y, z = what.x, what.y, what.z
                world.units.all.find { |u| u.pos.x == x and u.pos.y == y and u.pos.z == z }
            else
                raise "what what?"
            end
        end

        # return an Item
        # arg similar to find_unit
        def find_item(what=nil)
            if what == nil
                case ui.main.mode
                when UiSidebarMode::LookAround
                    k = ui_look_list.items[ui_look_cursor]
                    k.item if k.type == MemHack::UiLookList::Item
                end
            elsif what.kind_of?(Integer)
                world.items.all.find { |i| i.id == what }
            elsif what.respond_to?(:x) or what.respond_to?(:pos)
                what = what.pos if what.respond_to?(:pos)
                x, y, z = what.x, what.y, what.z
                world.items.all.find { |i| i.pos.x == x and i.pos.y == y and i.pos.z == z }
            else
                raise "what what?"
            end
        end

        # return a map block
        # can use find_map_block(cursor) or anything that respond to x/y/z
        def find_map_block(x=cursor, y=nil, z=nil)
            x = x.pos if x.respond_to?(:pos)
            x, y, z = x.x, x.y, x.z if x.respond_to?(:x)
            if x >= 0 and x < world.map.x_count and y >= 0 and y < world.map.y_count and z >= 0 and z < world.map.z_count
                world.map.block_index[x/16][y/16][z]
            end
        end

        def test
            puts "starting"

            suspend {
                puts "cursor pos: #{cursor.x} #{cursor.y} #{cursor.z}"

                if u = find_unit
                    puts "selected unit id: #{u.id}"
                end

                if b = find_map_block
                    b.designation[cursor.x%16][cursor.y%16].dig = TileDigDesignation::Default
                    b.flags.designated = true
                    puts "dug cursor tile"
                end
            }

            puts "done"
        end
    end
end

# load user-specified startup file
load 'ruby_custom.rb' if File.exist?('ruby_custom.rb')
