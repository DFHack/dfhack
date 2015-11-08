# exterminate creatures
=begin

exterminate
===========
Kills any unit of a given race.

With no argument, lists the available races and count eligible targets.

With the special argument ``him``, targets only the selected creature.

With the special argument ``undead``, targets all undeads on the map,
regardless of their race.

When specifying a race, a caste can be specified to further restrict the
targeting. To do that, append and colon and the caste name after the race.

Any non-dead non-caged unit of the specified race gets its ``blood_count``
set to 0, which means immediate death at the next game tick. For creatures
such as vampires, it also sets animal.vanish_countdown to 2.

An alternate mode is selected by adding a 2nd argument to the command,
``magma``. In this case, a column of 7/7 magma is generated on top of the
targets until they die (Warning: do not call on magma-safe creatures. Also,
using this mode on birds is not recommended.)  The final alternate mode
is ``butcher``, which marks them for butchering but does not kill.

Will target any unit on a revealed tile of the map, including ambushers,
but ignore caged/chained creatures.

Ex::

    exterminate gob
    exterminate gob:male

To kill a single creature, select the unit with the 'v' cursor and::

    exterminate him

To purify all elves on the map with fire (may have side-effects)::

    exterminate elve magma

=end

race = $script_args[0]

# if the 2nd parameter is 'magma', magma rain for the targets instead of instant death
# if it is 'butcher' mark all units for butchering (wont work with hostiles)
kill_by = $script_args[1]

case kill_by
when 'magma'
    slain = 'burning'
when 'slaughter', 'butcher'
    slain = 'marked for butcher'
when nil
    slain = 'slain'
else
    race = 'help'
end

checkunit = lambda { |u|
    (u.body.blood_count != 0 or u.body.blood_max == 0) and
    not u.flags1.dead and
    not u.flags1.caged and not u.flags1.chained and
    #not u.flags1.hidden_in_ambush and
    not df.map_designation_at(u).hidden
}

slayit = lambda { |u|
    case kill_by
    when 'magma'
        # it's getting hot around here
        # !!WARNING!! do not call on a magma-safe creature
        ouh = df.onupdate_register("exterminate ensure #{u.id}", 1) {
            if u.flags1.dead
                df.onupdate_unregister(ouh)
            else
                x, y, z = u.pos.x, u.pos.y, u.pos.z
                z += 1 while tile = df.map_tile_at(x, y, z+1) and
                    tile.shape_passableflow and tile.shape_passablelow
                df.map_tile_at(x, y, z).spawn_magma(7)
            end
        }
    when 'butcher', 'slaughter'
        # mark for slaughter at butcher's shop
        u.flags2.slaughter = true
    else
        # just make them drop dead
        u.body.blood_count = 0
        # some races dont mind having no blood, ensure they are still taken care of.
        u.animal.vanish_countdown = 2
    end
}

all_races = Hash.new(0)

df.world.units.active.map { |u|
    if checkunit[u]
        if (u.enemy.undead or
            (u.curse.add_tags1.OPPOSED_TO_LIFE and not
             u.curse.rem_tags1.OPPOSED_TO_LIFE))
            all_races['Undead'] += 1
        else
            all_races[u.race_tg.creature_id] += 1
        end
    end
}

case race
when nil
    all_races.sort_by { |race, cnt| [cnt, race] }.each{ |race, cnt| puts " #{race} #{cnt}" }

when 'help', '?'
    puts <<EOS
Kills all creatures of a given race.
With no argument, lists possible targets with their head count.
With the special argument 'him' or 'her', kill only the currently selected creature.
With the special argument 'undead', kill all undead creatures/thralls.

The targets will bleed out on the next game tick, or if they are immune to that, will vanish in a puff of smoke.

The special final argument 'magma' will make magma rain on the targets instead.
The special final argument 'butcher' will mark the targets for butchering instead.

Ex: exterminate gob
    exterminate gob:male
    exterminate elve magma
    exterminate him
    exterminate pig butcher
EOS

when 'him', 'her', 'it', 'that'
    if him = df.unit_find
        case him.race_tg.caste[him.caste].gender
        when 0; puts 'its a she !' if race != 'her'
        when 1; puts 'its a he !'  if race != 'him'
        else;   puts 'its an it !' if race != 'it' and race != 'that'
        end
        slayit[him]
    else
        puts "Select a target ingame"
    end

when /^undead/i
    count = 0
    df.world.units.active.each { |u|
        if (u.enemy.undead or
            (u.curse.add_tags1.OPPOSED_TO_LIFE and not
             u.curse.rem_tags1.OPPOSED_TO_LIFE)) and
           checkunit[u]
            slayit[u]
            count += 1
        end
    }
    puts "#{slain} #{count} undeads"

else
    if race.index(':')
        race, caste = race.split(':')
    end

    raw_race = df.match_rawname(race, all_races.keys)
    if not raw_race
        puts "Invalid race, use one of #{all_races.keys.sort.join(' ')}"
        throw :script_finished
    end

    race_nr = df.world.raws.creatures.all.index { |cr| cr.creature_id == raw_race }

    if caste
        all_castes = df.world.raws.creatures.all[race_nr].caste.map { |c| c.caste_id }
        raw_caste = df.match_rawname(caste, all_castes)
        if not raw_caste
            puts "Invalid caste, use one of #{all_castes.sort.join(' ')}"
            throw :script_finished
        end
        caste_nr = all_castes.index(raw_caste)
    end

    count = 0
    df.world.units.active.each { |u|
        if u.race == race_nr and checkunit[u]
            next if caste_nr and u.caste != caste_nr
            slayit[u]
            count += 1
        end
    }
    puts "#{slain} #{count} #{raw_caste} #{raw_race}"

end
