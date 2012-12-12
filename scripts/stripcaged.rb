# mark stuff inside of cages for dumping.

def plural(nr, name)
	# '1 cage' / '4 cages'
	"#{nr} #{name}#{'s' if nr > 1}"
end

def cage_dump_items(list)
	count = 0
	count_cage = 0
	list.each { |cage|
		pre_count = count
		cage.general_refs.each { |ref|
			next unless ref.kind_of?(DFHack::GeneralRefContainsItemst)
			next if ref.item_tg.flags.dump
			count += 1
			ref.item_tg.flags.dump = true
		}
		count_cage += 1 if pre_count != count
	}

	puts "Dumped #{plural(count, 'item')} in #{plural(count_cage, 'cage')}"
end

def cage_dump_armor(list)
	count = 0
	count_cage = 0
	list.each { |cage|
		pre_count = count
		cage.general_refs.each { |ref|
			next unless ref.kind_of?(DFHack::GeneralRefContainsUnitst)
			ref.unit_tg.inventory.each { |it|
				next if it.mode != :Worn
				next if it.item.flags.dump
				count += 1
				it.item.flags.dump = true
			}
		}
		count_cage += 1 if pre_count != count
	}

	puts "Dumped #{plural(count, 'armor piece')} in #{plural(count_cage, 'cage')}"
end

def cage_dump_weapons(list)
	count = 0
	count_cage = 0
	list.each { |cage|
		pre_count = count
		cage.general_refs.each { |ref|
			next unless ref.kind_of?(DFHack::GeneralRefContainsUnitst)
			ref.unit_tg.inventory.each { |it|
				next if it.mode != :Weapon
				next if it.item.flags.dump
				count += 1
				it.item.flags.dump = true
			}
		}
		count_cage += 1 if pre_count != count
	}

	puts "Dumped #{plural(count, 'weapon')} in #{plural(count_cage, 'cage')}"
end

def cage_dump_all(list)
	count = 0
	count_cage = 0
	list.each { |cage|
		pre_count = count
		cage.general_refs.each { |ref|
			case ref
			when DFHack::GeneralRefContainsItemst
				next if ref.item_tg.flags.dump
				count += 1
				ref.item_tg.flags.dump = true
			when DFHack::GeneralRefContainsUnitst
				ref.unit_tg.inventory.each { |it|
					next if it.item.flags.dump
					count += 1
					it.item.flags.dump = true
				}
			end
		}
		count_cage += 1 if pre_count != count
	}

	puts "Dumped #{plural(count, 'item')} in #{plural(count_cage, 'cage')}"
end


def cage_dump_list(list)
	count_total = Hash.new(0)
	empty_cages = 0
	list.each { |cage|
		count = Hash.new(0)

		cage.general_refs.each { |ref|
			case ref
			when DFHack::GeneralRefContainsItemst
				count[ref.item_tg._rtti_classname] += 1
			when DFHack::GeneralRefContainsUnitst
				ref.unit_tg.inventory.each { |it|
					count[it.item._rtti_classname] += 1
				}
			# TODO vermin ?
			else
				puts "unhandled ref #{ref.inspect}" if $DEBUG
			end
		}

		type = case cage
		       when DFHack::ItemCagest; 'Cage'
		       when DFHack::ItemAnimaltrapst; 'Animal trap'
		       else cage._rtti_classname
		       end

		if count.empty?
			empty_cages += 1
		else
			puts "#{type} ##{cage.id}: ", count.sort_by { |k, v| v }.map { |k, v| " #{v} #{k}" }
		end

		count.each { |k, v| count_total[k] += v }
	}

	if list.length > 2
		puts '', "Total: ", count_total.sort_by { |k, v| v }.map { |k, v| " #{v} #{k}" }
		puts "with #{plural(empty_cages, 'empty cage')}"
	end
end


# handle magic script arguments
here_only = $script_args.delete 'here'
if here_only
	it = df.item_find
	list = [it]
	if not it.kind_of?(DFHack::ItemCagest) and not it.kind_of?(DFHack::ItemAnimaltrapst)
		list = df.world.items.other[:ANY_CAGE_OR_TRAP].find_all { |i| df.at_cursor?(i) }
	end
	if list.empty?
		puts 'Please select a cage'
		throw :script_finished
	end

elsif ids = $script_args.find_all { |arg| arg =~ /^\d+$/ } and ids.first
	list = []
	ids.each { |id|
		$script_args.delete id
		if not it = df.item_find(id.to_i)
			puts "Invalid item id #{id}"
		elsif not it.kind_of?(DFHack::ItemCagest) and not it.kind_of?(DFHack::ItemAnimaltrapst)
			puts "Item ##{id} is not a cage"
			list << it
		else
			list << it
		end
	}
	if list.empty?
		puts 'Please use a valid cage id'
		throw :script_finished
	end

else
	list = df.world.items.other[:ANY_CAGE_OR_TRAP]
end


# act
case $script_args[0]
when /^it/i
	cage_dump_items(list)
when /^arm/i
	cage_dump_armor(list)
when /^wea/i
	cage_dump_weapons(list)
when 'all'
	cage_dump_all(list)
when 'list'
	cage_dump_list(list)
else
	puts <<EOS
Marks items inside all cages for dumping.
Add 'here' to dump stuff only for selected cage.
Add a cage id to dump stuff for this cage only.

See 'autodump' to actually dump stuff.

Usage:
  stripcaged items
    dump items directly in cages (eg seeds after training)
 
  stripcaged [armor|weapons] here
    dump armor or weapons of caged creatures in selected cage

  stripcaged all 28 29
    dump every item in cage id 28 and 29, along with every item worn by creatures in there too
 
  stripcaged list
    show content of the cages

EOS

end
