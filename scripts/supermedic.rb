# repair him/her. type 'supermedic help' to show the descriptions.

args = $script_args.uniq

checkunit = lambda { |u|
    u.body.blood_count != 0 and
    not u.flags1.dead and
    not df.map_designation_at(u).hidden
}


find_nicknamed = lambda { |s|
    patients = []
    df.world.units.all.each { |u|
        if u.name.nickname == s
            next unless checkunit[u]
            patients << u
        end
    }
    puts "found #{patients.count} '#{s}'."
    return patients
}

clear_wounds = lambda { |u|
    if u.body.wounds.count > 0
    u.body.wounds = []
    u.body.wound_next_id = 1 #?
    puts "cleared all wounds."
    end
}

clear_requests = lambda { |u|
    return unless u.health
    u.health.body_part_8 = Array.new(u.health.body_part_8.count) {0}
    puts "cleared treatment requests."
}

repair_stand = lambda { |u|
    return unless u.status2
    if u.status2.able_stand < 2
        u.status2.able_stand = 2
        puts "repaired lost stand ability."
    end
    if u.status2.able_stand_impair < 2
        u.status2.able_stand_impair = 2
        puts "repaired impaired stand ability."
    end
}

repair_grasp = lambda { |u|
    return unless u.status2
    if u.status2.able_grasp < 2
        u.status2.able_grasp = 2
        puts "repaired lost grasp ability."
    end
    if u.status2.able_grasp_impair < 2
        u.status2.able_grasp_impair = 2
        puts "repaired impaired grasp ability."
    end
}

wakeup = lambda { |u|
    return unless u.job.current_job and u.job.current_job.job_type == :Rest
    u.job.current_job.job_type = :Sleep
    u.counters.unconscious = 0
    puts "released from 'Rest' job."
}

repair_him = lambda { |u|
    if args.include?("all") or args.empty?
        clear_wounds[u]
        clear_requests[u]
        repair_stand[u]
        repair_grasp[u]
        wakeup[u]
    else
        args.each { |arg|
            case arg
            when "wounds"
                clear_wounds[u]
            when "reqs"
                clear_requests[u]
            when "stand"
                repair_stand[u]
            when "grasp"
                repair_grasp[u]
            when "wake"
                wakeup[u]
            end
        }
    end
}

if args.include?("man"||"help"||"?")
    puts "Repair him/her. Use this when you see the 'unmovable dwarves' hospital bugs."
    puts "Please select by v or k or following target option."
    puts "Options(target):"
    puts "  nick:x - execute repair function(s) to that nicknamed creature(s)."
    puts "           i.e. nick:foo means select all creature(s) nicknamed as 'foo'."
    puts "           you can use spaces by using quotes."
    puts "Options(repair):"
    puts "  all    - execute all following repair functions to the patient."
    puts "           no options is the same as execute all."
    puts "  wounds - clear all wounds"
    puts "  reqs   - clear all treatment requests"
    puts "  stand  - force walkable (also impair)"
    puts "  grasp  - force graspable (also impair)"
    puts "  wake   - release from 'Rest' job"
    puts "Usage:"
    puts "  supermedic wounds reqs stand wake"
    puts "    - repair the selecting patient without grasp"
    puts "  supermedic nick:""Ignored Dwarf"""
    puts "    - do all repairs to nicknamed as ""Ignored Dwarf"""

else
    nick = ""
    patients = []
    args = args.delete_if { |x|
        if /nick:(.+)/ =~ x
            nick = Regexp.last_match[1]
        end
    }
    if /(['"])[^\1](.+)\1/ =~ nick # remove quotation chars
        nick = Regexp.last_match[2]
    end
    if nick.length > 0
        patients = find_nicknamed[nick]
    else
        patients << df.unit_find
    end
    if not patients.empty? and patients[0]
        patients.each { |u|
            puts "+ #{u.name} +"
            repair_him[u]
        }
    end
end
