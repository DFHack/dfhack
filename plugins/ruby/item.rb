module DFHack
    class << self
        # return an Item
        # arg similar to unit.rb/unit_find; no arg = 'k' menu
        def item_find(what=:selected)
            if what == :selected
                if curview._rtti_classname == :viewscreen_itemst
                    ref = curview.entry_ref[curview.cursor_pos]
                    ref.item_tg if ref.kind_of?(GeneralRefItem)
                else
                    case ui.main.mode
                    when :LookAround
                        k = ui_look_list.items[ui_look_cursor]
                        k.item if k.type == :Item
                    when :BuildingItems
                        bld = world.selected_building
                        bld.contained_items[ui_building_item_cursor].item if bld
                    when :ViewUnits
                        u = world.units.active[ui_selected_unit]
                        u.inventory[ui_look_cursor].item if u and u.pos.z == cursor.z and
                                ui_unit_view_mode == :Inventory and u.inventory[ui_look_cursor]
                    end
                end
            elsif what.kind_of?(Integer)
                world.items.all.binsearch(what)
            elsif what.respond_to?(:x) or what.respond_to?(:pos)
                world.items.all.find { |i| same_pos?(what, i) }
            else
                raise "what what?"
            end
        end
    end
end
