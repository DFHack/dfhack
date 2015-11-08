# unforbid all items
=begin

devel/unforbidall
=================
Unforbid all items.

=end

df.world.items.all.each { |i| i.flags.forbid = false }
