require 'hack/ruby-autogen'

module DFHack
    class << self
        # update the ruby.cpp version to accept a block
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

        module ::Kernel
            def puts(*a)
                a.flatten.each { |l|
                    DFHack.print_str(l.to_s.chomp + "\n")
                }
                nil
            end

            def puts_err(*a)
                a.flatten.each { |l|
                    DFHack.print_err(l.to_s.chomp + "\n")
                }
                nil
            end

            def p(*a)
                a.each { |e|
                    puts_err e.inspect
                }
            end
        end

        # register a callback to be called every gframe or more
        # ex: DFHack.onupdate_register { DFHack.world.units[0].counters.job_counter = 0 }
        def onupdate_register(&b)
            @onupdate_list ||= []
            @onupdate_list << b
            DFHack.onupdate_active = true
            @onupdate_list.last
        end

        # delete the callback for onupdate ; use the value returned by onupdate_register
        def onupdate_unregister(b)
            @onupdate_list.delete b
            DFHack.onupdate_active = false if @onupdate_list.empty?
        end

        # this method is called by dfhack every 'onupdate' if onupdate_active is true
        def onupdate
            @onupdate_list ||= []
            @onupdate_list.each { |cb| cb.call }
        end

        # register a callback to be called every gframe or more
        # ex: DFHack.onstatechange_register { |newstate| puts "state changed to #{newstate}" }
        def onstatechange_register(&b)
            @onstatechange_list ||= []
            @onstatechange_list << b
            @onstatechange_list.last
        end

        # delete the callback for onstatechange ; use the value returned by onstatechange_register
        def onstatechange_unregister(b)
            @onstatechange_list.delete b
        end

        # this method is called by dfhack every 'onstatechange'
        def onstatechange(newstate)
            @onstatechange_list ||= []
            @onstatechange_list.each { |cb| cb.call(newstate) }
        end


        # return an Unit
        # with no arg, return currently selected unit in df UI ('v' or 'k' menu)
        # with numeric arg, search unit by unit.id
        # with an argument that respond to x/y/z (eg cursor), find first unit at this position
        def find_unit(what=:selected)
            if what == :selected
                case ui.main.mode
                when UiSidebarMode::ViewUnits
                    # nobody selected => idx == 0
                    v = world.units.other[0][ui_selected_unit]
                    v if v and v.pos.z == cursor.z
                when UiSidebarMode::LookAround
                    k = ui_look_list.items[ui_look_cursor]
                    k.unit if k.type == UiLookList::Unit
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
        # arg similar to find_unit; no arg = 'k' menu
        def find_item(what=:selected)
            if what == :selected
                case ui.main.mode
                when UiSidebarMode::LookAround
                    k = ui_look_list.items[ui_look_cursor]
                    k.item if k.type == UiLookList::Item
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

        # return a map block by tile coordinates
        # you can also use find_map_block(cursor) or anything that respond to x/y/z
        def map_block_at(x, y=nil, z=nil)
            x = x.pos if x.respond_to?(:pos)
            x, y, z = x.x, x.y, x.z if x.respond_to?(:x)
            if x >= 0 and x < world.map.x_count and y >= 0 and y < world.map.y_count and z >= 0 and z < world.map.z_count
                world.map.block_index[x/16][y/16][z]
            end
        end

        def map_designation_at(x, y=nil, z=nil)
            x = x.pos if x.respond_to?(:pos)
            x, y, z = x.x, x.y, x.z if x.respond_to?(:x)
            if b = map_block_at(x, y, z)
                b.designation[x%16][y%16]
            end
        end

        def map_occupancy_at(x, y=nil, z=nil)
            x = x.pos if x.respond_to?(:pos)
            x, y, z = x.x, x.y, x.z if x.respond_to?(:x)
            if b = map_block_at(x, y, z)
                b.occupancy[x%16][y%16]
            end
        end

        # yields every map block
        def each_map_block
            (0...world.map.x_count_block).each { |xb|
                xl = world.map.block_index[xb]
                (0...world.map.y_count_block).each { |yb|
                    yl = xl[yb]
                    (0...world.map.z_count_block).each { |z|
                        p = yl[z]
                        yield p._getv if p._getp != 0
                    }
                }
            }
        end

        # yields every map block for a given z level
        def each_map_block_z(z)
            (0...world.map.x_count_block).each { |xb|
                xl = world.map.block_index[xb]
                (0...world.map.y_count_block).each { |yb|
                    p = xl[yb][z]
                    yield p._getv if p._getp != 0
                }
            }
        end

        # return true if the argument is under the cursor
        def at_cursor?(obj)
            same_pos?(obj, cursor)
        end

        # returns true if both arguments are at the same x/y/z
        def same_pos?(pos1, pos2)
            pos1 = pos1.pos if pos1.respond_to?(:pos)
            pos2 = pos2.pos if pos2.respond_to?(:pos)
            pos1.x == pos2.x and pos1.y == pos2.y and pos1.z == pos2.z
        end

        # center the DF screen on something
        # updates the cursor position if visible
        def center_viewscreen(x, y=nil, z=nil)
            x = x.pos if x.respond_to?(:pos)
            x, y, z = x.x, x.y, x.z if x.respond_to?(:x)

            # compute screen 'map' size (tiles)
            menuwidth = ui_menu_width
            # ui_menu_width shows only the 'tab' status
            menuwidth = 1 if menuwidth == 2 and ui_area_map_width == 2 and cursor.x != -30000
            menuwidth = 2 if menuwidth == 3 and cursor.x != -30000
            w_w = gps.dimx - 2
            w_h = gps.dimy - 2
            case menuwidth
            when 1; w_w -= 55
            when 2; w_w -= (ui_area_map_width == 2 ? 24 : 31)
            end

            # center view
            w_x = x - w_w/2
            w_y = y - w_h/2
            w_z = z
            # round view coordinates (optional)
            #w_x -= w_x % 10
            #w_y -= w_y % 10
            # crop to map limits
            w_x = [[w_x, world.map.x_count - w_w].min, 0].max
            w_y = [[w_y, world.map.y_count - w_h].min, 0].max

            self.window_x = w_x
            self.window_y = w_y
            self.window_z = w_z

            if cursor.x != -30000
                cursor.x, cursor.y, cursor.z = x, y, z
            end
        end

        # add an announcement
        # color = integer, bright = bool
        def add_announcement(str, color=7, bright=false)
            cont = false
            while str.length > 0
                rep = Report.cpp_new
                rep.color = color
                rep.bright = ((bright && bright != 0) ? 1 : 0)
                rep.year = cur_year
                rep.time = cur_year_tick
                rep.flags.continuation = cont
                cont = true
                rep.flags.announcement = true
                rep.text = str[0, 73]
                str = str[73..-1].to_s
                rep.id = world.status.next_report_id
                world.status.next_report_id += 1
                world.status.reports << rep
                world.status.announcements << rep
                world.status.display_timer = 2000
            end
        end

        # try to match a user-specified name to one from the raws
        # uses case-switching and substring matching
        # eg match_rawname('coal', ['COAL_BITUMINOUS', 'BAUXITE']) => 'COAL_BITUMINOUS'
        def match_rawname(name, rawlist)
            rawlist.each { |r| return r if name == r }
            rawlist.each { |r| return r if name.downcase == r.downcase }
            may = rawlist.find_all { |r| r.downcase.index(name.downcase) }
            may.first if may.length == 1
        end
    end
end

# global alias so we can write 'df.world.units.all[0]'
def df
    DFHack
end

# load user-specified startup file
load 'ruby_custom.rb' if File.exist?('ruby_custom.rb')
