module DFHack
    class << self
        # return an Item
        # arg similar to unit.rb/unit_find; no arg = 'k' menu
        def item_find(what=:selected, y=nil, z=nil)
            if what == :selected
                case curview._rtti_classname
                when :viewscreen_itemst
                    ref = curview.entry_ref[curview.cursor_pos]
                    ref.item_tg if ref.kind_of?(GeneralRefItem)
                when :viewscreen_storesst   # z/stocks
                    if curview.in_group_mode == 0 and curview.in_right_list == 1
                        curview.items[curview.item_cursor]
                    end
                else
                    case ui.main.mode
                    when :LookAround
                        k = ui_look_list.items[ui_look_cursor]
                        case k.type
                        when :Item
                            k.item
                        when :Building
                            # hilight a constructed bed/coffer
                            mats = k.building.contained_items.find_all { |i| i.use_mode == 2 }
                            mats[0].item if mats.length == 1
                        end
                    when :BuildingItems
                        bld = world.selected_building
                        bld.contained_items[ui_building_item_cursor].item if bld
                    when :ViewUnits
                        u = world.units.active[ui_selected_unit]
                        u.inventory[ui_look_cursor].item if u and u.pos.z == cursor.z and
                                ui_unit_view_mode.value == :Inventory and u.inventory[ui_look_cursor]
                    else
                        ui.follow_item_tg if ui.follow_item != -1
                    end
                end
            elsif what.kind_of?(Integer)
                # search by id
                return world.items.all.binsearch(what) if not z
                # search by position
                x = what
                world.items.all.find { |i| i.pos.x == x and i.pos.y == y and i.pos.z == z }
            elsif what.respond_to?(:x) or what.respond_to?(:pos)
                world.items.all.find { |i| same_pos?(what, i) }
            else
                raise "what what?"
            end
        end
    end
end
