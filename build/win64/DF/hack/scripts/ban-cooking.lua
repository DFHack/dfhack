-- convenient way to ban cooking categories of food
-- based on ban-cooking.rb by Putnam: https://github.com/DFHack/scripts/pull/427/files
-- Putnams work completed by TBSTeun

local options = {}
local argparse = require('argparse')
local kitchen = df.global.plotinfo.kitchen
local already_banned = {}
local count = 0

local function make_key(mat_type, mat_index, type, subtype)
    return ('%s:%s:%s:%s'):format(mat_type, mat_index, type, subtype)
end

local function reaction_product_id_contains(reaction_product, str)
    for _, s in ipairs(reaction_product.id) do
        if s.value == str then
            return true
        end
    end

    return false
end

local function ban_cooking(print_name, mat_type, mat_index, type, subtype)
    local key = make_key(mat_type, mat_index, type, subtype)
    -- Skip adding a new entry further below, if the item is already banned.
    if already_banned[key] then
        return
    end
    -- The item hasn't already been banned, so we do that here by appending its values to the various arrays
    count = count + 1
    if options.verbose then
        print(print_name .. ' has been banned!')
    end

    kitchen.mat_types:insert('#', mat_type)
    kitchen.mat_indices:insert('#', mat_index)
    kitchen.item_types:insert('#', type)
    kitchen.item_subtypes:insert('#', subtype)
    kitchen.exc_types:insert('#', df.kitchen_exc_type.Cook)

    already_banned[key] = {}
    already_banned[key].mat_type = mat_type
    already_banned[key].mat_index = mat_index
    already_banned[key].type = type
    already_banned[key].subtype = subtype
end

-- Iterate over the elements of the kitchen.item_types list
for i = 0, #kitchen.item_types - 1 do
    -- Check if the kitchen.exc_types[i] element is equal to :Cook
    if kitchen.exc_types[i] == df.kitchen_exc_type.Cook then
        -- Add a new element to the already_banned dictionary
        already_banned_key = make_key(kitchen.mat_types[i], kitchen.mat_indices[i], kitchen.item_types[i], kitchen.item_subtypes[i])
        if not already_banned[already_banned_key] then
            already_banned[already_banned_key] = {}
            already_banned[already_banned_key].mat_type = kitchen.mat_types[i]
            already_banned[already_banned_key].mat_index = kitchen.mat_indices[i]
            already_banned[already_banned_key].type = kitchen.item_types[i]
            already_banned[already_banned_key].subtype = kitchen.item_subtypes[i]
        end
    end
end

local funcs = {}

funcs.booze = function()
    for _, p in ipairs(df.global.world.raws.plants.all) do
        for _, m in ipairs(p.material) do
            if m.flags.ALCOHOL and m.flags.EDIBLE_COOKED then
                local matinfo = dfhack.matinfo.find(p.id, m.id)
                ban_cooking(p.name .. ' ' .. m.id, matinfo.type, matinfo.index, df.item_type.DRINK, -1)
            end
        end
    end
    for _, c in ipairs(df.global.world.raws.creatures.all) do
        for _, m in ipairs(c.material) do
            if m.flags.ALCOHOL and m.flags.EDIBLE_COOKED then
                local matinfo = dfhack.matinfo.find(creature_id.id, m.id)
                ban_cooking(c.name[2] .. ' ' .. m.id, matinfo.type, matinfo.index, df.item_type.DRINK, -1)
            end
        end
    end
end

funcs.honey = function()
    local mat = dfhack.matinfo.find("CREATURE:HONEY_BEE:HONEY")
    ban_cooking('honey bee honey', mat.type, mat.index, df.item_type.LIQUID_MISC, -1)
end

funcs.tallow = function()
    for _, c in ipairs(df.global.world.raws.creatures.all) do
        for _, m in ipairs(c.material) do
            if m.flags.EDIBLE_COOKED then
                for _, s in ipairs(m.reaction_product.id) do
                    if s.value == "SOAP_MAT" then
                        local matinfo = dfhack.matinfo.find(c.creature_id, m.id)
                        ban_cooking(c.name[2] .. ' ' .. m.id, matinfo.type, matinfo.index, df.item_type.GLOB, -1)
                        break
                    end
                end
            end
        end
    end
end

funcs.milk = function()
    for _, c in ipairs(df.global.world.raws.creatures.all) do
        for _, m in ipairs(c.material) do
            if m.flags.EDIBLE_COOKED then
                for _, s in ipairs(m.reaction_product.id) do
                    if s.value == "CHEESE_MAT" then
                        local matinfo = dfhack.matinfo.find(c.creature_id, m.id)
                        ban_cooking(c.name[2] .. ' ' .. m.id, matinfo.type, matinfo.index, df.item_type.LIQUID_MISC, -1)
                        break
                    end
                end
            end
        end
    end
end

funcs.oil = function()
    for _, p in ipairs(df.global.world.raws.plants.all) do
        for _, m in ipairs(p.material) do
            if m.flags.EDIBLE_COOKED then
                for _, s in ipairs(m.reaction_product.id) do
                    if s.value == "SOAP_MAT" then
                        local matinfo = dfhack.matinfo.find(p.id, m.id)
                        ban_cooking(p.name .. ' ' .. m.id, matinfo.type, matinfo.index, df.item_type.LIQUID_MISC, -1)
                        break
                    end
                end
            end
        end
    end
end

funcs.seeds = function()
    for _, p in ipairs(df.global.world.raws.plants.all) do
        if p.material_defs.type.seed == -1 or p.material_defs.idx.seed == -1 or p.flags.TREE then goto continue end
        ban_cooking(p.name .. ' seeds', p.material_defs.type.seed, p.material_defs.idx.seed, df.item_type.SEEDS, -1)
        for _, m in ipairs(p.material) do
            if m.id == "STRUCTURAL" and m.flags.EDIBLE_COOKED then
                local has_drink = false
                local has_seed = false
                for _, s in ipairs(m.reaction_product.id) do
                    has_seed = has_seed or s.value == "SEED_MAT"
                    has_drink = has_drink or s.value == "DRINK_MAT"
                end
                if has_seed and has_drink then
                    local matinfo = dfhack.matinfo.find(p.id, m.id)
                    ban_cooking(p.name .. ' ' .. m.id, matinfo.type, matinfo.index, df.item_type.PLANT, -1)
                end
            end
        end
        for k, g in ipairs(p.growths) do
            local matinfo = dfhack.matinfo.decode(g)
            local m = matinfo.material
            if m.flags.EDIBLE_COOKED then
                local has_drink = false
                local has_seed = false
                for _, s in ipairs(m.reaction_product.id) do
                    has_seed = has_seed or s.value == "SEED_MAT"
                    has_drink = has_drink or s.value == "DRINK_MAT"
                end
                if has_seed and has_drink then
                    ban_cooking(p.name .. ' ' .. m.id, matinfo.type, matinfo.index, df.item_type.PLANT_GROWTH, k)
                end
            end
        end

        ::continue::
    end
end

funcs.brew = function()
    for _, p in ipairs(df.global.world.raws.plants.all) do
        if p.material_defs.type.drink == -1 or p.material_defs.idx.drink == -1 then goto continue end
        for _, m in ipairs(p.material) do
            if m.id == "STRUCTURAL" and m.flags.EDIBLE_COOKED then
                for _, s in ipairs(m.reaction_product.id) do
                    if s.value == "DRINK_MAT" then
                        local matinfo = dfhack.matinfo.find(p.id, m.id)
                        ban_cooking(p.name .. ' ' .. m.id, matinfo.type, matinfo.index, df.item_type.PLANT, -1)
                        break
                    end
                end
            end
        end
        for k, g in ipairs(p.growths) do
            local matinfo = dfhack.matinfo.decode(g)
            local m = matinfo.material
            if m.flags.EDIBLE_COOKED then
                for _, s in ipairs(m.reaction_product.id) do
                    if s.value == "DRINK_MAT" then
                        ban_cooking(p.name .. ' ' .. m.id, matinfo.type, matinfo.index, df.item_type.PLANT_GROWTH, k)
                        break
                    end
                end
            end
        end

        ::continue::
    end
end

funcs.mill = function()
    for _, p in ipairs(df.global.world.raws.plants.all) do
        if p.material_defs.idx.mill ~= -1 then
            for _, m in ipairs(p.material) do
                if m.id == "STRUCTURAL" and m.flags.EDIBLE_COOKED then
                    local matinfo = dfhack.matinfo.find(p.id, m.id)
                    ban_cooking(p.name .. ' ' .. m.id, matinfo.type, matinfo.index, df.item_type.PLANT, -1)
                end
            end
        end
    end
end

funcs.thread = function()
    for _, p in ipairs(df.global.world.raws.plants.all) do
        if p.material_defs.idx.thread == -1 then goto continue end
        for _, m in ipairs(p.material) do
            if m.id == "STRUCTURAL" and m.flags.EDIBLE_COOKED then
                for _, s in ipairs(m.reaction_product.id) do
                    if s.value == "THREAD" then
                        local matinfo = dfhack.matinfo.find(p.id, m.id)
                        ban_cooking(p.name .. ' ' .. m.id, matinfo.type, matinfo.index, df.item_type.PLANT, -1)
                        break
                    end
                end
            end
        end
        for k, g in ipairs(p.growths) do
            local matinfo = dfhack.matinfo.decode(g)
            local m = matinfo.material
            if m.flags.EDIBLE_COOKED then
                for _, s in ipairs(m.reaction_product.id) do
                    if s.value == "THREAD" then
                        ban_cooking(p.name .. ' ' .. m.id, matinfo.type, matinfo.index, df.item_type.PLANT_GROWTH, k)
                        break
                    end
                end
            end
        end

        ::continue::
    end
end

funcs.fruit = function()
    for _, p in ipairs(df.global.world.raws.plants.all) do
        for k, g in ipairs(p.growths) do
            local matinfo = dfhack.matinfo.decode(g)
            local m = matinfo.material
            if m.id == "FRUIT" and m.flags.EDIBLE_COOKED and m.flags.LEAF_MAT then
                for _, s in ipairs(m.reaction_product.id) do
                    if s.value == "DRINK_MAT" then
                        ban_cooking(p.name .. ' ' .. m.id, matinfo.type, matinfo.index, df.item_type.PLANT_GROWTH, k)
                        break
                    end
                end
            end
        end
    end
end

funcs.show = function()
    -- First put together a dictionary/hash table
    local type_list = {}
    local matinfo = nil

    -- cycle through all plants
    for i, p in ipairs(df.global.world.raws.plants.all) do
        -- The below three if statements initialize the dictionary/hash tables for their respective (cookable) plant/drink/seed
        -- And yes, this will create and then overwrite an entry when there is no (cookable) plant/drink/seed item for a specific plant,
        -- but since the -1 type and -1 index can't be added to the ban list, it's inconsequential to check for non-existent (cookable) plant/drink/seed items here

        -- need to create a key to reference values in the types_list dictionary.
        key_basic_mat = p.material_defs.type["basic_mat"] .. p.material_defs.idx.basic_mat
        if not(type_list[key_basic_mat]) then
            -- Initialize the type_list dictionary with a new element
            type_list[key_basic_mat] = {}
        end

        -- need to create a key to reference values in the types_list dictionary.
        key_drink = p.material_defs.type["drink"] .. p.material_defs.idx.drink
        if not(type_list[key_drink]) then
            type_list[key_drink] = {}
        end

        -- need to create a key to reference values in the types_list dictionary.
        key_seed = p.material_defs.type["seed"] .. p.material_defs.idx.seed
        if not(type_list[key_seed]) then
            type_list[key_seed] = {}
        end

        type_list[key_basic_mat].text = p.name .. " basic"
        -- basic materials for plants always appear to use the 'PLANT' item type tag
        type_list[key_basic_mat]["type"] = "PLANT"
        -- item subtype of 'PLANT' types appears to always be -1, as there is no growth array entry for the 'PLANT'
        type_list[key_basic_mat]["subtype"] = -1

        type_list[key_drink]["text"] = p.name .. " drink"
        -- drink materials for plants always appear to use the 'DRINK' item type tag
        type_list[key_drink]["type"] = "DRINK"
        -- item subtype of 'Drink' types appears to always be -1, as there is no growth array entry for the 'Drink'
        type_list[key_drink]["subtype"] = -1

        type_list[key_seed]["text"] = p.name .. " seed"
        -- drink materials for plants always appear to use the 'SEEDS' item type tag
        type_list[key_seed]["type"] = "SEEDS"
        -- item subtype of 'SEEDS' types appears to always be -1, as there is no growth array entry for the 'SEEDS'
        type_list[key_seed]["subtype"] = -1

        for r, g in ipairs(p.growths) do
            matinfo = dfhack.matinfo.decode(g)
            local m = matinfo.material

            if m.flags["EDIBLE_COOKED"] and m.flags["LEAF_MAT"] then
                for j, s in ipairs(p.material) do
                    if m.id == s.id then

                        -- need to create a key to reference values in the types_list dictionary.
                        local key_matinfo_type = matinfo.type .. i

                        if not(type_list[key_matinfo_type]) then
                            type_list[key_matinfo_type] = {}
                        else
                            print('Key exists for ' .. p.name, type_list[key_matinfo_type]["text"], type_list[key_matinfo_type]["type"], j + matinfo.type)
                        end

                        type_list[key_matinfo_type]["text"] = p.name .. " " .. m.id .. " growth"
                        -- item type for plant materials listed in the growths array appear to always use the 'PLANT_GROWTH' item type tag
                        type_list[key_matinfo_type]["type"] = "PLANT_GROWTH"
                        -- item subtype is equal to the array index of the cookable item in the growths table
                        type_list[key_matinfo_type]["subtype"] = r
                    end
                end
            end
        end
    end
    -- cycle through all creatures
    for i, c in ipairs(df.global.world.raws.creatures.all) do
        for j, m in ipairs(c.material) do
            -- need to create a key to reference values in the types_list dictionary.
            local key_matinfo_type = j + matinfo.type .. i

            if m.reaction_product and m.reaction_product.id and reaction_product_id_contains(m.reaction_product, "CHEESE_MAT") then
                if not(type_list[key_matinfo_type]) then
                    type_list[key_matinfo_type] = {}
                end

                type_list[key_matinfo_type]["text"] = c.name[1] .. " milk"
                -- item type for milk appears to use the 'LIQUID_MISC' tag
                type_list[key_matinfo_type]["type"] = "LIQUID_MISC"
                type_list[key_matinfo_type]["subtype"] = -1

            end

            if m.reaction_product and m.reaction_product.id and reaction_product_id_contains(m.reaction_product, "SOAP_MAT") then
                if not(type_list[key_matinfo_type]) then
                    type_list[key_matinfo_type] = {}
                end

                type_list[key_matinfo_type]["text"] = c.name[1] .. " tallow"
                -- item type for milk appears to use the 'GLOB' tag
                type_list[key_matinfo_type]["type"] = "GLOB"
                type_list[key_matinfo_type]["subtype"] = -1
            end
        end
    end

    local output = {}

    for i, b in pairs(already_banned) do
        -- initialize our output string with the array entry position (largely stays the same for each item on successive runs, except when items are added/removed)
        local cur = ''
        -- initialize our key for accessing our stored items info
        local key = b.mat_type .. b.mat_index

        -- It shouldn't be possible for there to not be a matching key entry by this point, but we'll be kinda safe here
        if type_list[key] then
            -- Add the item name to the first part of the string
            cur = type_list[key]['text'] .. ' |type '

            if type_list[key]['type'] == df.item_type[b.type] then
                cur = cur .. 'match: ' .. type_list[key]['type']
            else
                -- Aw crap.  The item type we EXpected doesn't match up with the ACtual item type.
                cur = cur .. "error: ex;" .. type_list[key]["type"] .. "/ac;" .. df.item_type[b.type]
            end

            cur = cur .. "|subtype "
            if type_list[key]["subtype"] == b.subtype then
                -- item sub type is a match, so we print that it's a match, as well as the item subtype index number (-1 means there is no subtype for this item)
                cur = cur .. "match: " .. type_list[key]["subtype"]
            else
                -- Something went wrong, and the EXpected item subtype index value doesn't match the ACtual index value
                cur = cur .. "error: ex;" .. type_list[key]["subtype"] .. "/ac;" .. b.subtype
            end
        else
            -- There's no entry for this item in our calculated list of cookable items.  So, it's not a plant, alcohol, tallow, or milk.  It's likely that it's a meat that has been banned.
            cur = cur .. '|"' .. '[' .. b.mat_type .. ', ' .. b.mat_index .. ']' .. ' unknown banned material type (meat?) " ' .. '|item type: "' .. tostring(df.item_type[b.type]) .. '"|item subtype: "' .. tostring(b.subtype)
        end

        table.insert(output, cur)
    end

    table.sort(output, function(a, b) return a < b end)

    for k, v in pairs(output) do
        print(k .. ": |" .. v)
    end
end

local commands = argparse.processArgsGetopt({...}, {
    {'h', 'help', handler=function() options.help = true end},
    {'v', 'verbose', handler=function() options.verbose = true end},
})

if options.help == true then
    print(dfhack.script_help())
    return
end

if commands[1] == 'all' then
    for func, _ in pairs(funcs) do
        if func ~= 'show' then
            funcs[func]()
        end
    end
end

for _, v in ipairs(commands) do
    if funcs[v] then
        funcs[v]()
    end
end

print('banned ' .. count .. ' items.')
