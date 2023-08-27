local args = {...}

-- player civ references:
local p_civ_id = df.global.plotinfo.civ_id
local p_civ = df.historical_entity.find(df.global.plotinfo.civ_id)

-- get list of civs:
function get_civ_list()
    local civ_list = {}
    for _, entity in pairs(p_civ.relations.diplomacy) do
        local cur_civ_id = entity.group_id
        local cur_civ = df.historical_entity.find(cur_civ_id)
        if cur_civ.type == 0 then
            -- if true then
            rel_str = ""
            if entity.relation == 0 then
                rel_str = "Peace"
            elseif entity.relation == 1 then
                rel_str = "War"
            end
            matched = "No"
            for _, entity2 in pairs(cur_civ.relations.diplomacy) do
                if entity2.group_id == p_civ_id and entity2.relation == entity.relation then
                    matched = "Yes"
                end
            end
            table.insert(civ_list, {
            cur_civ_id,
            rel_str,
            matched,
            dfhack.TranslateName(cur_civ.name, true)
            })
        end
    end
    return civ_list
end

-- output civ list:
function output_civ_list()
    local civ_list = get_civ_list()
    if not next(civ_list) then
        print("Your civilisation has no diplomatic relations! This means something is going wrong, as it should have at least a relation to itself.")
    else
        print(([[%4s %12s %8s %30s]]):format("ID", "Relation", "Mutual", "Name"))
        for _, civ in pairs(civ_list) do
            print(([[%4s %12s %8s %30s]]):format(civ[1], civ[2], civ[3], civ[4]))
        end
    end
end

-- change relation:
function change_relation(civ_id, relation)
    print("Changing relation with " .. civ_id .. " to " .. (relation == 0 and "Peace" or "War"))
    for _, entity in pairs(p_civ.relations.diplomacy) do
        local cur_civ_id = entity.group_id
        local cur_civ = df.historical_entity.find(cur_civ_id)
        if cur_civ.type == 0 and cur_civ_id == civ_id then
            entity.relation = relation
            for _, entity2 in pairs(cur_civ.relations.diplomacy) do
                if entity2.group_id == p_civ_id then
                    entity2.relation = relation
                end
            end
        end
    end
end

-- parse relation string args:
function relation_parse(rel_str)
    if rel_str == "0"  or rel_str:lower() == "peace" then
        return 0
    elseif rel_str == "1" or rel_str:lower() == "war" then
        return 1
    else
        print(dfhack.script_help())
        qerror("Cannot parse relation: " .. rel_str)
    end
end

-- handle 'all' civ argument:
function handle_all(arg1, arg2)
    if arg1:lower() == "all" then
        local civ_list = get_civ_list()
        for _, civ in pairs(civ_list) do
            if civ[1] ~= p_civ_id then
                change_relation(civ[1], arg2)
            end
        end
    else
        change_relation(tonumber(arg1), arg2)
    end
end

-- if no civ ID is entered, just output list of civs:
if not args[1] then
    output_civ_list()
    return
end

-- make sure that there is a relation to change to:
if not args[2] then
    print(dfhack.script_help())
    qerror("Please specify 'peace' or 'war'")
end

-- change relation(s) according to args:
handle_all(args[1], relation_parse(args[2]))
output_civ_list()
