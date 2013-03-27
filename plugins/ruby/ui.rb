# df user-interface related methods
module DFHack
    class << self
        # returns the current active viewscreen
        def curview
            ret = gview.view
            ret = ret.child while ret.child
            ret
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
        def add_announcement(str, color=nil, bright=nil)
            cont = false
            while str.length > 0
                rep = Report.cpp_new
                rep.color = color if color
                rep.bright = ((bright && bright != 0) ? 1 : 0) if bright != nil
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
                yield rep if block_given?
            end
        end

        # add an announcement to display in a game popup message
        # (eg "the megabeast foobar arrived")
        def popup_announcement(str, color=nil, bright=nil)
            pop = PopupMessage.cpp_new(:text => str)
            pop.color = color if color
            pop.bright = bright if bright
            world.status.popups << pop
        end
    end

    class Viewscreen
        def feed_keys(*keys)
            keyset = StlSet.cpp_new(keys, InterfaceKey)
            ret = feed(keyset)
            keyset._cpp_delete
            ret
        end
    end
end
