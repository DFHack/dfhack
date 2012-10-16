# release pet from the dead owners. "you are not 'hachi'."

if df.unit_find
    u = df.unit_find
    owner_id = u.relations.pet_owner_id

    if not owner_id == -1 and df.unit_find(owner_id).flags1.dead
        u.relations.pet_owner_id = -1
        puts "released #{u.name} from the dead owner."
    end
end