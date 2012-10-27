# release pet from the dead owners. "you are not 'hachi'."

arg = $script_args[0]

forget_dead_owner = lambda { |u|
    return if u.flags1.dead
    owner_id = u.relations.pet_owner_id
    if not owner_id == -1 and df.unit_find(owner_id).flags1.dead
        u.relations.pet_owner_id = -1
        puts "released #{u.name}, #{u.profession} from the dead owner."
    end
}

if arg == 'all'
    df.world.units.all.each { |u|
        forget_dead_owner[u]
    }
elsif u = df.unit_find
    forget_dead_owner[u]
end
