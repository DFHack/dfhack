-- Launch unit to cursor location
-- Based on propel.lua by Roses, molested by Rumrusher and I until this happened, sorry.
--[====[

launch
======
Activate with a cursor on screen and you will go there rapidly. Attack
something first to ride them there.

]====]
function launch(unitSource,unitRider)
    local curpos
    if df.global.adventure.menu == df.ui_advmode_menu.Look then
        curpos = df.global.cursor
    elseif df.global.gamemode == df.game_mode.ADVENTURE then
        qerror("No [l] cursor located! You would have slammed into the ground and exploded.")
    else
        qerror("Must be used in adventurer mode or the arena!")
    end


    local count=0
    local l = df.global.world.proj_list
    local lastlist=l
    l=l.next
    while l do
        count=count+1
        if l.next==nil then
            lastlist=l
        end
        l = l.next
    end

    local resultx = curpos.x - unitSource.pos.x
    local resulty = curpos.y - unitSource.pos.y
    local resultz = curpos.z - unitSource.pos.z

    local newlist = df.proj_list_link:new()
    lastlist.next=newlist
    newlist.prev=lastlist
    local proj = df.proj_unitst:new()
    newlist.item=proj
    proj.link=newlist
    proj.id=df.global.proj_next_id
    df.global.proj_next_id=df.global.proj_next_id+1
    proj.unit=unitSource
    proj.origin_pos.x=unitSource.pos.x
    proj.origin_pos.y=unitSource.pos.y
    proj.origin_pos.z=unitSource.pos.z
    proj.target_pos.x=curpos.x
    proj.target_pos.y=curpos.y
    proj.target_pos.z=curpos.z
    proj.prev_pos.x=unitSource.pos.x
    proj.prev_pos.y=unitSource.pos.y
    proj.prev_pos.z=unitSource.pos.z
    proj.cur_pos.x=unitSource.pos.x
    proj.cur_pos.y=unitSource.pos.y
    proj.cur_pos.z=unitSource.pos.z
    proj.flags.no_impact_destroy=true
    proj.flags.piercing=true
    proj.flags.high_flying=true --this probably doesn't do anything, let me know if you figure out what it is
    proj.flags.parabolic=true
    proj.flags.no_collide=true
    proj.flags.unk9=true
    proj.speed_x=resultx*10000
    proj.speed_y=resulty*10000
    proj.speed_z=resultz*15000 --higher z speed makes it easier to reach a target safely
    if df.global.world.units.active[0].job.hunt_target==nil then
        proj.flags.safe_landing=true
    elseif df.global.world.units.active[0].job.hunt_target then
        proj.flags.safe_landing=false
    end
    local unitoccupancy = dfhack.maps.ensureTileBlock(unitSource.pos).occupancy[unitSource.pos.x%16][unitSource.pos.y%16]
    if not unitSource.flags1.on_ground then
        unitoccupancy.unit = false
    else
        unitoccupancy.unit_grounded = false
    end
    unitSource.flags1.projectile=true
    unitSource.flags1.on_ground=false
end

local unitSource = df.global.world.units.active[0]
local unitRider = nil --as:df.unit
if unitSource.job.hunt_target ~= nil then
    unitRider = unitSource
    unitSource = df.global.world.units.active[0].job.hunt_target
    unitSource.general_refs:insert("#",{new=df.general_ref_unit_riderst,unit_id=unitRider.id})
    unitRider.relationship_ids.RiderMount=unitSource.id
    unitRider.flags1.rider=true
    unitSource.flags1.ridden=true
    require("utils").insert_sorted(df.global.world.units.other.ANY_RIDER,unitRider,"id")
end

launch(unitSource,unitRider)
