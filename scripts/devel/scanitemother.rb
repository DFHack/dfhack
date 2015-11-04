# list indices in world.item.other[] where current selected item appears
=begin

devel/scanitemother
===================
List indices in ``world.item.other[]`` where current selected item appears.

=end
tg = df.item_find
raise 'select an item' if not tg

o = df.world.items.other
# discard ANY/BAD
o._indexenum::ENUM.sort.transpose[1][1..-2].each { |k|
    puts k if o[k].find { |i| i == tg }
}
