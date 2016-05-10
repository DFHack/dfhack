-- Allow humans to make trade agreements
--[[=begin

fix/merchants
=============
Adds the Guild Representative position to all Human civilizations,
allowing them to make trade agreements.  This was the default behaviour in
``0.28.181.40d`` and earlier.

=end]]


function add_guild_rep(ent)
    -- there was no guild rep - create it
    local pos = df.entity_position:new()
    ent.positions.own:insert('#', pos)

    pos.code = "GUILD_REPRESENTATIVE"
    pos.id = ent.positions.next_position_id + 1
    ent.positions.next_position_id = ent.positions.next_position_id + 1

    pos.flags.DO_NOT_CULL = true
    pos.flags.MENIAL_WORK_EXEMPTION = true
    pos.flags.SLEEP_PRETENSION = true
    pos.flags.PUNISHMENT_EXEMPTION = true
    pos.flags.ACCOUNT_EXEMPT = true
    pos.flags.DUTY_BOUND = true
    pos.flags.COLOR = true
    pos.flags.HAS_RESPONSIBILITIES = true
    pos.flags.IS_DIPLOMAT = true
    pos.flags.IS_LEADER = true
    -- not sure what these flags do, but the game sets them for a valid guild rep
    pos.flags.unk_12 = true
    pos.flags.unk_1a = true
    pos.flags.unk_1b = true
    pos.name[0] = "Guild Representative"
    pos.name[1] = "Guild Representatives"
    pos.precedence = 40
    pos.color[0] = 7
    pos.color[1] = 0
    pos.color[2] = 1
    
    return pos
end

local checked = 0
local fixed = 0

for _,ent in pairs(df.global.world.entities.all) do
    if ent.type == df.historical_entity_type.Civilization and ent.entity_raw.flags.MERCHANT_NOBILITY then
        checked = checked + 1

        update = true
        -- see if we need to add a new position or modify an existing one
        local found_position
        for _,pos in pairs(ent.positions.own) do
            if pos.responsibilities.TRADE and pos.responsibilities.ESTABLISH_COLONY_TRADE_AGREEMENTS then
                -- a guild rep exists with the proper responsibilities - skip to the end
                update = false
                found_position=pos
                break
            end
            -- Guild Representative position already exists, but has the wrong options - modify it instead of creating a new one
            if pos.code == "GUILD_REPRESENTATIVE" then
                found_position=pos
                break
            end
        end
        if update then
            -- either there's no guild rep, or there is one and it's got the wrong responsibilities
            if not found_position then
                found_position = add_guild_rep(ent)
            end
            -- assign responsibilities
            found_position.responsibilities.ESTABLISH_COLONY_TRADE_AGREEMENTS = true
            found_position.responsibilities.TRADE=true
        end

        -- make sure the guild rep position, whether we created it or not, is set up for proper assignment
        local assign = true
        for _,p in pairs(ent.positions.assignments) do
            if p.position_id == found_position.id then -- it is - nothing more to do here
                assign = false
                break
            end
        end
        if assign then
            -- it isn't - set it up
            local asn = df.entity_position_assignment:new()
            ent.positions.assignments:insert('#', asn)

            asn.id = ent.positions.next_assignment_id
            ent.positions.next_assignment_id = asn.id + 1
            asn.position_id = found_position.id
            asn.flags:resize(math.max(32, #asn.flags)) -- make room for 32 flags
            asn.flags[0] = true  -- and set the first one
        end
        if update or assign then
            fixed = fixed + 1
        end
    end
end

print("Added merchant nobility for "..fixed.." of "..checked.." civilizations.")
