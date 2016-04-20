-- Add Elven diplomats to negotiate tree caps
--[[=begin

fix/diplomats
=============
Adds a Diplomat position to all Elven civilizations, allowing them to negotiate
tree cutting quotas - and you to violate them and start wars.
This was vanilla behaviour until ``0.31.12``, in which the "bug" was "fixed".

=end]]


function update_pos(pos, ent)
    pos = df.entity_position:new()
    ent.positions.own:insert('#', pos)

    pos.code = "DIPLOMAT"
    pos.id = ent.positions.next_position_id + 1
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
    -- not sure what these flags do, but the game sets them for a valid diplomat
    pos.flags.unk_12 = true
    pos.flags.unk_1a = true
    pos.flags.unk_1b = true
    pos.name[0] = "Diplomat"
    pos.name[1] = "Diplomats"
    pos.precedence = 70
    pos.color[0] = 7
    pos.color[1] = 0
    pos.color[2] = 1

    return pos
end



checked = 0
fixed = 0

for _,ent in pairs(df.global.world.entities.all) do
    if ent.type == 0 and ent.entity_raw.flags.TREE_CAP_DIPLOMACY then
        checked = checked + 1

        update = true
        -- see if we need to add a new position or modify an existing one
        for _,pos in pairs(ent.positions.own) do
            if  pos.responsibilities.MAKE_INTRODUCTIONS and
                pos.responsibilities.MAKE_PEACE_AGREEMENTS and
                pos.responsibilities.MAKE_TOPIC_AGREEMENTS then
                    -- a diplomat position exists with the proper responsibilities - skip to the end
                    update = false
                    break
            end
            -- Diplomat position already exists, but has the wrong options - modify it instead of creating a new one
            if pos.code == "DIPLOMAT" then break end
            pos = nil
        end
        if update then
            -- either there's no diplomat, or there is one and it's got the wrong responsibilities
            if not pos then
                pos = add_guild_rep(pos, ent)
            end
            -- assign responsibilities
            pos.responsibilities.MAKE_INTRODUCTIONS = true
            pos.responsibilities.MAKE_PEACE_AGREEMENTS = true
            pos.responsibilities.MAKE_TOPIC_AGREEMENTS = true
        end
        -- make sure the diplomat position, whether we created it or not, is set up for proper assignment
        assign = true
        for _,p in pairs(ent.positions.assignments) do
            if p.position_id == pos.id then -- it is - nothing more to do here
                assign = false
                break
            end
        end
        if assign then -- it isn't - set it up
            asn = df.entity_position_assignment:new()
            ent.positions.assignments:insert('#', asn);

            asn.id = ent.positions.next_assignment_id
            ent.positions.next_assignment_id = asn.id + 1
            asn.position_id = pos.id
            asn.flags:resize(math.max(32, #asn.flags)) -- make room for 32 flags
            asn.flags[0] = true  -- and set the first one
        end
        if update or assign then
            fixed = fixed + 1
        end
    end
end

print("Enabled tree cap diplomacy for "..fixed.." of "..checked.." civilizations.")
