# fix doors that are frozen in 'open' state

# this may happen after people mess with the game by (incorrectly) teleporting units or items
# a door may stick open if the map occupancy flags are wrong
=begin

fix/stuckdoors
==============
Fix doors that are stuck open due to incorrect map occupancy flags, eg due to
incorrect use of `teleport`.

=end
count = 0
df.world.buildings.all.each { |bld|
    # for all doors
    next if bld._rtti_classname != :building_doorst
    # check if it is open
    next if bld.close_timer == 0
    # check if occupancy is set
    occ = df.map_occupancy_at(bld.x1, bld.y1, bld.z)
    if (occ.unit or occ.unit_grounded) and not
        # check if an unit is present
        df.world.units.active.find { |u| u.pos.x == bld.x1 and u.pos.y == bld.y1 and u.pos.z == bld.z }
        count += 1
        occ.unit = occ.unit_grounded = false
    end
    if occ.item and not df.world.items.all.find { |i| i.pos.x == bld.x1 and i.pos.y == bld.y1 and i.pos.z == bld.z }
        count += 1
        occ.item = false
    end
}
puts "unstuck #{count} doors"
