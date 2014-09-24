# View or set level of cavern adaptation for the selected unit or the whole fort
# based on removebadthoughts.rb

def usage(s)
    if nil != s
        puts(s)
    end
    puts "Usage: adaptation <show|set> <him|all> [value]"
    throw :script_finished
end

mode  = $script_args[0] || 'help'
who   = $script_args[1]
value = $script_args[2]

if 'help' == mode
    usage(nil)
elsif 'show' != mode && 'set' != mode
    usage("Invalid mode '#{mode}': must be either 'show' or 'set'")
end

if nil == who
    usage("Target not specified")
elsif 'him' != who && 'all' != who
    usage("Invalid target '#{who}'")
end

if 'set' == mode
    if nil == value
        usage("Value not specified")
    elsif !/[[:digit:]]/.match(value)
        usage("Invalid value '#{value}'")
    end

    if 0 > value.to_i || 800000 < value.to_i
        usage("Value must be between 0 and 800000")
    end
    value = value.to_i
end

num_set = 0

set_adaptation_value = lambda { |u,v|
    next if !df.unit_iscitizen(u)
    next if u.flags1.dead
    u.status.misc_traits.each { |t|
        if t.id == :CaveAdapt
            # TBD: expose the color_ostream console and color values of
            # t.value based on adaptation level
            if mode == 'show'
                puts "Unit #{u.id} (#{u.name}) has an adaptation of #{t.value}"
            elsif mode == 'set'
                puts "Unit #{u.id} (#{u.name}) changed from #{t.value} to #{v}"
                t.value = v
                num_set += 1
            end
        end
    }
}

case who
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
end

if 'set' == mode
    puts "#{num_set} unit#{'s' if num_set != 1} updated."
end
