module DFHack
    class << self
        # return an Item
        # arg similar to unit.rb/unit_find; no arg = 'k' menu
        def item_find(what=:selected)
            if what == :selected
                case ui.main.mode
                when :LookAround
                    k = ui_look_list.items[ui_look_cursor]
                    k.item if k.type == :Item
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
