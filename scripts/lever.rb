# control your levers from the dfhack console
=begin

lever
=====
Allow manipulation of in-game levers from the dfhack console.

Can list levers, including state and links, with::

    lever list

To queue a job so that a dwarf will pull the lever 42, use ``lever pull 42``.
This is the same as :kbd:`q` querying the building and queue a :kbd:`P` pull request.

To magically toggle the lever immediately, use::

    lever pull 42 --now

=end

def lever_pull_job(bld)
    ref = DFHack::GeneralRefBuildingHolderst.cpp_new
    ref.building_id = bld.id

    job = DFHack::Job.cpp_new
    job.job_type = :PullLever
    job.pos = [bld.centerx, bld.centery, bld.z]
    job.general_refs << ref
    bld.jobs << job
    df.job_link job

    puts lever_descr(bld)
end

def lever_pull_cheat(bld)
    bld.linked_mechanisms.each { |i|
        i.general_refs.grep(DFHack::GeneralRefBuildingHolderst).each { |r|
            r.building_tg.setTriggerState(bld.state)
        }
    }

    bld.state = (bld.state == 0 ? 1 : 0)

    puts lever_descr(bld)
end

def lever_descr(bld, idx=nil)
    ret = []

    # lever description
    descr = ''
    descr << "#{idx}: " if idx
    descr << "lever ##{bld.id} "
    descr << "(#{bld.name}) " if bld.name.length != 0
    descr << "@[#{bld.centerx}, #{bld.centery}, #{bld.z}] #{bld.state == 0 ? '\\' : '/'}"
    bld.jobs.each { |j|
        if j.job_type == :PullLever
            flags = ''
            flags << ', repeat' if j.flags.repeat
            flags << ', suspended' if j.flags.suspend
            descr << " (pull order#{flags})"
        end
    }

    bld.linked_mechanisms.map { |i|
        i.general_refs.grep(DFHack::GeneralRefBuildingHolderst)
    }.flatten.each { |r|
        # linked building description
        tg = r.building_tg
        state = ''
        if tg.respond_to?(:gate_flags)
            state << (tg.gate_flags.closed ? 'closed' : 'opened')
            state << ", closing (#{tg.timer})" if tg.gate_flags.closing
            state << ", opening (#{tg.timer})" if tg.gate_flags.opening
        end

        ret << (descr + " linked to #{tg._rtti_classname} ##{tg.id} @[#{tg.centerx}, #{tg.centery}, #{tg.z}] #{state}")

        # indent other links
        descr = descr.gsub(/./, ' ')
    }

    ret << descr if ret.empty?

    ret
end

def lever_list
    @lever_list = []
    df.world.buildings.other[:TRAP].find_all { |bld|
        bld.trap_type == :Lever
    }.sort_by { |bld| bld.id }.each { |bld|
        puts lever_descr(bld, @lever_list.length)
        @lever_list << bld.id
    }
end


@lever_list ||= []

case $script_args[0]
when 'pull'
    cheat = $script_args.delete('--cheat') || $script_args.delete('--now')

    if $script_args[1].nil?
        bld = df.building_find(:selected) if not bld
        raise 'no lever under cursor and no lever id given' if not bld
    else
        id = $script_args[1].to_i
        id = @lever_list[id] || id
        bld = df.building_find(id)
        raise 'invalid lever id' if not bld
    end

    if cheat
        lever_pull_cheat(bld)
    else
        lever_pull_job(bld)
    end

when 'list'
    lever_list

when /^\d+$/
    id = $script_args[0].to_i
    id = @lever_list[id] || id
    bld = df.building_find(id)
    raise 'invalid lever id' if not bld

    puts lever_descr(bld)

else

    puts <<EOS
Lever control from the dfhack console

Usage:
lever list
 shows the list of levers in the fortress, with their id, name and links

lever pull
 order the dwarves to pull the lever under the cursor

lever pull 42
 order the dwarves to pull lever 42

lever pull 42 --cheat
 magically pull lever 42 immediately
EOS

end

