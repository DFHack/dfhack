# Set level of cavern adaptation for the selected unit or the whole fort
# based on removebadthoughts.rb

def usage
    puts "Usage: set-adaptation [--dry-run] <him|all> <value>"
end

dry_run = $script_args.delete('--dry-run') || $script_args.delete('-n')

$script_args << 'all' if dry_run and $script_args.empty?

num_set = 0

set_adaptation_value = lambda { |u,v|
    next if !df.unit_iscitizen(u)
    u.status.misc_traits.each { |t|
        if t.id == :CaveAdapt
            puts "Unit #{u.id} (#{u.name}) changed from #{t.value} to #{v} #{'(not really)' if dry_run}"
            unless dry_run
                t.value = v
                num_set += 1
            end
        end
    }
}

if nil == $script_args[1] 
    puts "<value> parameter missing"
    usage
elsif 0 > $script_args[1].to_i || 800000 < $script_args[1].to_i
    puts "<value> must be a value between 0 and 800000"
    usage
else
    value = $script_args[1].to_i
    case $script_args[0]
    when 'him'
        if u = df.unit_find
            set_adaptation_value[u,value]
        else
            puts 'Please select a dwarf ingame'
        end
    when 'all'
        df.unit_citizens.each { |uu|
            set_adaptation_value[uu,value]
        }
    else
        usage
    end
end

puts "#{num_set} unit#{'s' if num_set != 1} updated."
