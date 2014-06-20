local _ENV = mkmodule('plugins.stockflow')

local gui = require "gui"
local utils = require "utils"

reaction_list = reaction_list or {}
saved_orders = saved_orders or {}
jobs_to_create = jobs_to_create or {}

triggers = {
    {filled = false, divisor = 1, name = "Per empty space"},
    {filled = true,  divisor = 1, name = "Per stored item"},
    {filled = false, divisor = 2, name = "Per two empty spaces"},
    {filled = true,  divisor = 2, name = "Per two stored items"},
    {filled = false, divisor = 3, name = "Per three empty spaces"},
    {filled = true,  divisor = 3, name = "Per three stored items"},
    {filled = false, divisor = 4, name = "Per four empty spaces"},
    {filled = true,  divisor = 4, name = "Per four stored items"},
    {name = "Never"},
}

local job_types = df.job_type

entry_ints = {
    stockpile_id = 1,
    order_number = 2,
    trigger_number = 3,
}

PageSize = 16
FirstRow = 4
CenterCol = 38

-- Populate the reaction and stockpile order lists.
-- To be called whenever a world is loaded.
function initialize_world()
    reaction_list = collect_reactions()
    saved_orders = collect_orders()
    jobs_to_create = {}
end

-- Clear all caches.
-- Called when a world is loaded, or when the plugin is disabled.
function clear_caches()
    -- Free the C++ objects in the reaction list.
    for _, value in ipairs(reaction_list) do
        value.order:delete()
    end
    reaction_list = {}
    saved_orders = {}
    jobs_to_create = {}
end

function trigger_name(cache)
    local trigger = triggers[cache.entry.ints[entry_ints.trigger_number]]
    return trigger and trigger.name or "Never"
end

function list_orders()
    local listed = false
    for _, spec in pairs(saved_orders) do
        local num = spec.stockpile.stockpile_number
        local name = spec.entry.value
        local trigger = trigger_name(spec)
        print("Stockpile #"..num, name, trigger)
        listed = true
    end
    
    if not listed then
        print("No manager jobs have been set for your stockpiles.")
        print("Use j in a stockpile menu to create one...")
    end
end

-- Save the stockpile jobs for later creation.
-- Called when the bookkeeper starts updating stockpile records.
function start_bookkeeping()
    result = {}
    for reaction_id, quantity in pairs(check_stockpiles()) do
        local amount = order_quantity(reaction_list[reaction_id].order, quantity)
        if amount > 0 then
            result[reaction_id] = amount
        end
    end
    
    jobs_to_create = result
end

-- Insert any saved jobs.
-- Called when the bookkeeper finishes updating stockpile records.
function finish_bookkeeping()
    for reaction, amount in pairs(jobs_to_create) do
        create_orders(reaction_list[reaction].order, amount)
    end
    
    jobs_to_create = {}
end

function stockpile_settings(sp)
    local order = saved_orders[sp.id]
    if not order then
        return "No job selected", ""
    end
    
    return order.entry.value, trigger_name(order)
end

-- Toggle the trigger condition for a stockpile.
function toggle_trigger(sp)
    local saved = saved_orders[sp.id]
    if saved then
        saved.entry.ints[entry_ints.trigger_number] = (saved.entry.ints[entry_ints.trigger_number] % #triggers) + 1
        saved.entry:save()
    end
end

function collect_orders()
    local result = {}
    local entries = dfhack.persistent.get_all("stockflow/entry", true)
    if entries then
        local stockpiles = df.global.world.buildings.other.STOCKPILE
        for _, entry in ipairs(entries) do
            local spid = entry.ints[entry_ints.stockpile_id]
            local stockpile = utils.binsearch(stockpiles, spid, "id")
            if stockpile then
                -- Todo: What if entry.value ~= reaction_list[order_number].name?
                result[spid] = {
                    stockpile = stockpile,
                    entry = entry,
                }
            end
        end
    end
    
    return result
end

-- Choose an order that the stockpile should add to the manager queue.
function select_order(stockpile)
    screen:reset(stockpile)
    screen:show()
end

function reaction_entry(job_type, values, name)
    local order = df.manager_order:new()
    -- These defaults differ from the newly created order's.
    order:assign{
        job_type = job_type,
        unk_2 = -1,
        item_subtype = -1,
        mat_type = -1,
        mat_index = -1,
    }
    
    if values then
        -- Override default attributes.
        order:assign(values)
    end
    
    return {
        name = name or df.job_type.attrs[job_type].caption,
        order = order,
    }
end

function resource_reactions(reactions, job_type, mat_info, keys, items, options)
    local values = {}
    for key, value in pairs(mat_info.management) do
        values[key] = value
    end
    
    for _, itemid in ipairs(keys) do
        local itemdef = items[itemid]
        local start = options.verb or mat_info.verb or "Make"
        if options.adjective then
            start = start.." "..itemdef.adjective
        end
        
        if (not options.permissible) or options.permissible(itemdef) then
            local item_name = " "..itemdef[options.name_field or "name"]
            if options.capitalize then
                item_name = string.gsub(item_name, " .", string.upper)
            end
            
            values.item_subtype = itemid
            table.insert(reactions, reaction_entry(job_type, values, start.." "..mat_info.adjective..item_name))
        end
    end
end

function material_reactions(reactions, itemtypes, mat_info)
    -- Expects a list of {job_type, verb, item_name} tuples.
    for _, row in ipairs(itemtypes) do
        local line = row[2].." "..mat_info.adjective
        if row[3] then
            line = line.." "..row[3]
        end
        
        table.insert(reactions, reaction_entry(row[1], mat_info.management, line))
    end
end

function clothing_reactions(reactions, mat_info, filter)
    local resources = df.historical_entity.find(df.global.ui.civ_id).resources
    local itemdefs = df.global.world.raws.itemdefs
    resource_reactions(reactions, job_types.MakeArmor,  mat_info, resources.armor_type,  itemdefs.armor,  {permissible = filter})
    resource_reactions(reactions, job_types.MakePants,  mat_info, resources.pants_type,  itemdefs.pants,  {permissible = filter})
    resource_reactions(reactions, job_types.MakeGloves, mat_info, resources.gloves_type, itemdefs.gloves, {permissible = filter})
    resource_reactions(reactions, job_types.MakeHelm,   mat_info, resources.helm_type,   itemdefs.helms,  {permissible = filter})
    resource_reactions(reactions, job_types.MakeShoes,  mat_info, resources.shoes_type,  itemdefs.shoes,  {permissible = filter})
end

-- Find the reaction types that should be listed in the management interface.
function collect_reactions()
    -- The sequence here tries to match the native manager screen.
    -- It should also be possible to collect the sequence from somewhere native,
    -- but I currently can only find it while the job selection screen is active.
    -- Even that list doesn't seem to include their names.
    local result = {}
    
    local materials = {
        rock = {
            adjective = "rock",
            management = {mat_type = 0},
        },
    }
    
    for _, name in ipairs{"wood", "cloth", "leather", "silk", "yarn", "bone", "shell", "tooth", "horn", "pearl"} do
        materials[name] = {
            adjective = name,
            management = {material_category = {[name] = true}},
        }
    end
    
    materials.wood.adjective = "wooden"
    materials.tooth.adjective = "ivory/tooth"
    materials.leather.clothing_flag = "LEATHER"
    
    -- Collection and Entrapment
    table.insert(result, reaction_entry(job_types.CollectWebs))
    table.insert(result, reaction_entry(job_types.CollectSand))
    table.insert(result, reaction_entry(job_types.CollectClay))
    table.insert(result, reaction_entry(job_types.CatchLiveLandAnimal))
    table.insert(result, reaction_entry(job_types.CatchLiveFish))
    
    -- Cutting, encrusting, and metal extraction.
    local rock_types = df.global.world.raws.inorganics
    for rock_id = #rock_types-1, 0, -1 do
        local material = rock_types[rock_id].material
        local rock_name = material.state_adj.Solid
        if material.flags.IS_STONE or material.flags.IS_GEM then
            table.insert(result, reaction_entry(job_types.CutGems, {
                mat_type = 0,
                mat_index = rock_id,
            }, "Cut "..rock_name))
            
            table.insert(result, reaction_entry(job_types.EncrustWithGems, {
                mat_type = 0,
                mat_index = rock_id,
                item_category = {finished_goods = true},
            }, "Encrust Finished Goods With "..rock_name))
            
            table.insert(result, reaction_entry(job_types.EncrustWithGems, {
                mat_type = 0,
                mat_index = rock_id,
                item_category = {furniture = true},
            }, "Encrust Furniture With "..rock_name))
            
            table.insert(result, reaction_entry(job_types.EncrustWithGems, {
                mat_type = 0,
                mat_index = rock_id,
                item_category = {ammo = true},
            }, "Encrust Ammo With "..rock_name))
        end
        
        if #rock_types[rock_id].metal_ore.mat_index > 0 then
            table.insert(result, reaction_entry(job_types.SmeltOre, {mat_type = 0, mat_index = rock_id}, "Smelt "..rock_name.." Ore"))
        end
        
        if #rock_types[rock_id].thread_metal.mat_index > 0 then
            table.insert(result, reaction_entry(job_types.ExtractMetalStrands, {mat_type = 0, mat_index = rock_id}))
        end
    end
    
    -- Glass cutting and encrusting, with different job numbers.
    -- We could search the entire table, but glass is less subject to raws.
    local glass_types = df.global.world.raws.mat_table.builtin
    local glasses = {}
    for glass_id = 3, 5 do
        local material = glass_types[glass_id]
        local glass_name = material.state_adj.Solid
        if material.flags.IS_GLASS then
            -- For future use.
            table.insert(glasses, {
                adjective = glass_name,
                management = {mat_type = glass_id},
            })
                
            table.insert(result, reaction_entry(job_types.CutGlass, {mat_type = glass_id}, "Cut "..glass_name))
            
            table.insert(result, reaction_entry(job_types.EncrustWithGlass, {
                mat_type = glass_id,
                item_category = {finished_goods = true},
            }, "Encrust Finished Goods With "..glass_name))
            
            table.insert(result, reaction_entry(job_types.EncrustWithGlass, {
                mat_type = glass_id,
                item_category = {furniture = true},
            }, "Encrust Furniture With "..glass_name))
            
            table.insert(result, reaction_entry(job_types.EncrustWithGlass, {
                mat_type = glass_id,
                item_category = {ammo = true},
            }, "Encrust Ammo With "..glass_name))
        end
    end
    
    -- Dyeing
    table.insert(result, reaction_entry(job_types.DyeThread))
    table.insert(result, reaction_entry(job_types.DyeCloth))
    
    -- Sew Image
    cloth_mats = {materials.cloth, materials.silk, materials.yarn, materials.leather}
    for _, material in ipairs(cloth_mats) do
        material_reactions(result, {{job_types.SewImage, "Sew", "Image"}}, material)
    end
    
    for _, spec in ipairs{materials.bone, materials.shell, materials.tooth, materials.horn, materials.pearl} do
        material_reactions(result, {{job_types.DecorateWith, "Decorate With"}}, spec)
    end
    
    table.insert(result, reaction_entry(job_types.MakeTotem))
    table.insert(result, reaction_entry(job_types.ButcherAnimal))
    table.insert(result, reaction_entry(job_types.MillPlants))
    table.insert(result, reaction_entry(job_types.MakePotashFromLye))
    table.insert(result, reaction_entry(job_types.MakePotashFromAsh))
    
    -- Kitchen
    table.insert(result, reaction_entry(job_types.PrepareMeal, {mat_type = 2}, "Prepare Easy Meal"))
    table.insert(result, reaction_entry(job_types.PrepareMeal, {mat_type = 3}, "Prepare Fine Meal"))
    table.insert(result, reaction_entry(job_types.PrepareMeal, {mat_type = 4}, "Prepare Lavish Meal"))
    
    -- Brew Drink
    table.insert(result, reaction_entry(job_types.BrewDrink))
    
    -- Weaving
    table.insert(result, reaction_entry(job_types.WeaveCloth, {material_category = {plant = true}}, "Weave Thread into Cloth"))
    table.insert(result, reaction_entry(job_types.WeaveCloth, {material_category = {silk = true}}, "Weave Thread into Silk"))
    table.insert(result, reaction_entry(job_types.WeaveCloth, {material_category = {yarn = true}}, "Weave Yarn into Cloth"))
    
    -- Extracts, farmer's workshop, and wood burning
    table.insert(result, reaction_entry(job_types.ExtractFromPlants))
    table.insert(result, reaction_entry(job_types.ExtractFromRawFish))
    table.insert(result, reaction_entry(job_types.ExtractFromLandAnimal))
    table.insert(result, reaction_entry(job_types.PrepareRawFish))
    table.insert(result, reaction_entry(job_types.MakeCheese))
    table.insert(result, reaction_entry(job_types.MilkCreature))
    table.insert(result, reaction_entry(job_types.ShearCreature))
    table.insert(result, reaction_entry(job_types.SpinThread))
    table.insert(result, reaction_entry(job_types.MakeLye))
    table.insert(result, reaction_entry(job_types.ProcessPlants))
    table.insert(result, reaction_entry(job_types.ProcessPlantsBag))
    table.insert(result, reaction_entry(job_types.ProcessPlantsVial))
    table.insert(result, reaction_entry(job_types.ProcessPlantsBarrel))
    table.insert(result, reaction_entry(job_types.MakeCharcoal))
    table.insert(result, reaction_entry(job_types.MakeAsh))
    
    -- Reactions defined in the raws.
    -- Not all reactions are allowed to the civilization.
    -- That includes "Make sharp rock" by default.
    local entity = df.historical_entity.find(df.global.ui.civ_id)
    for _, reaction_id in ipairs(entity.entity_raw.workshops.permitted_reaction_id) do
        local reaction = df.global.world.raws.reactions[reaction_id]
        local name = string.gsub(reaction.name, "^.", string.upper)
        table.insert(result, reaction_entry(job_types.CustomReaction, {reaction_name = reaction.code}, name))
    end
    
    -- Metal forging
    local itemdefs = df.global.world.raws.itemdefs
    for rock_id = 0, #rock_types - 1 do
        local material = rock_types[rock_id].material
        local rock_name = material.state_adj.Solid
        local mat_flags = {
            adjective = rock_name,
            management = {mat_type = 0, mat_index = rock_id},
            verb = "Forge",
        }
        
        if material.flags.IS_METAL then
            table.insert(result, reaction_entry(job_types.StudWith, mat_flags.management, "Stud With "..rock_name))
            
            if material.flags.ITEMS_WEAPON then
                -- Todo: Are these really the right flags to check?
                resource_reactions(result, job_types.MakeWeapon, mat_flags, entity.resources.weapon_type, itemdefs.weapons, {
                    permissible = (function(itemdef) return itemdef.skill_ranged == -1 end),
                })
                
                -- Is this entirely disconnected from the entity?
                material_reactions(result, {{MakeBallistaArrowHead, "Forge", "Ballista Arrow Head"}}, mat_flags)
                
                resource_reactions(result, job_types.MakeTrapComponent, mat_flags, entity.resources.trapcomp_type, itemdefs.trapcomps, {
                    adjective = true,
                })
                
                resource_reactions(result, job_types.AssembleSiegeAmmo, mat_flags, entity.resources.siegeammo_type, itemdefs.siege_ammo, {
                    verb = "Assemble",
                })
            end
            
            if material.flags.ITEMS_WEAPON_RANGED then
                resource_reactions(result, job_types.MakeWeapon, mat_flags, entity.resources.weapon_type, itemdefs.weapons, {
                    permissible = (function(itemdef) return itemdef.skill_ranged >= 0 end),
                })
            end
            
            if material.flags.ITEMS_DIGGER then
                -- Todo: Ranged or training digging weapons?
                resource_reactions(result, job_types.MakeWeapon, mat_flags, entity.resources.digger_type, itemdefs.weapons, {
                })
            end
            
            if material.flags.ITEMS_AMMO then
                resource_reactions(result, job_types.MakeAmmo, mat_flags, entity.resources.ammo_type, itemdefs.ammo, {
                    name_field = "name_plural",
                })
            end
            
            if material.flags.ITEMS_ANVIL then
                material_reactions(result, {{job_types.ForgeAnvil, "Forge", "Anvil"}}, mat_flags)
            end
            
            if material.flags.ITEMS_ARMOR then
                local metalclothing = (function(itemdef) return itemdef.props.flags.METAL end)
                clothing_reactions(result, mat_flags, metalclothing)
                resource_reactions(result, job_types.MakeShield, mat_flags, entity.resources.shield_type, itemdefs.shields, {
                })
            end
            
            if material.flags.ITEMS_SOFT then
                local metalclothing = (function(itemdef) return itemdef.props.flags.SOFT and not itemdef.props.flags.METAL end)
                clothing_reactions(result, mat_flags, metalclothing)
            end
            
            if material.flags.ITEMS_HARD then
                resource_reactions(result, job_types.MakeTool, mat_flags, entity.resources.tool_type, itemdefs.tools, {
                    permissible = (function(itemdef) return itemdef.flags.HARD_MAT end),
                    capitalize = true,
                })
            end
            
            if material.flags.ITEMS_METAL then
                resource_reactions(result, job_types.MakeTool, mat_flags, entity.resources.tool_type, itemdefs.tools, {
                    permissible = (function(itemdef) return itemdef.flags.METAL_MAT end),
                    capitalize = true,
                })
            end
            
            if material.flags.ITEMS_HARD then
                material_reactions(result, {
                    {job_types.ConstructDoor, "Construct", "Door"},
                    {job_types.ConstructFloodgate, "Construct", "Floodgate"},
                    {job_types.ConstructHatchCover, "Construct", "Hatch Cover"},
                    {job_types.ConstructGrate, "Construct", "Grate"},
                    {job_types.ConstructThrone, "Construct", "Throne"},
                    {job_types.ConstructCoffin, "Construct", "Sarcophagus"},
                    {job_types.ConstructTable, "Construct", "Table"},
                    {job_types.ConstructSplint, "Construct", "Splint"},
                    {job_types.ConstructCrutch, "Construct", "Crutch"},
                    {job_types.ConstructArmorStand, "Construct", "Armor Stand"},
                    {job_types.ConstructWeaponRack, "Construct", "Weapon Rack"},
                    {job_types.ConstructCabinet, "Construct", "Cabinet"},
                    {job_types.MakeGoblet, "Forge", "Goblet"},
                    {job_types.MakeInstrument, "Forge", "Instrument"},
                    {job_types.MakeToy, "Forge", "Toy"},
                    {job_types.ConstructStatue, "Construct", "Statue"},
                    {job_types.ConstructBlocks, "Construct", "Blocks"},
                    {job_types.MakeAnimalTrap, "Forge", "Animal Trap"},
                    {job_types.MakeBarrel, "Forge", "Barrel"},
                    {job_types.MakeBucket, "Forge", "Bucket"},
                    {job_types.ConstructBin, "Construct", "Bin"},
                    {job_types.MakePipeSection, "Forge", "Pipe Section"},
                    {job_types.MakeCage, "Forge", "Cage"},
                    {job_types.MintCoins, "Mint", "Coins"},
                    {job_types.ConstructChest, "Construct", "Chest"},
                    {job_types.MakeFlask, "Forge", "Flask"},
                    {job_types.MakeChain, "Forge", "Chain"},
                    {job_types.MakeCrafts, "Make", "Crafts"},
                }, mat_flags)
            end
            
            if material.flags.ITEMS_SOFT then
                material_reactions(result, {
                    {job_types.MakeBackpack, "Make", "Backpack"},
                    {job_types.MakeQuiver, "Make", "Quiver"},
                    {job_types.ConstructCatapultParts, "Construct", "Catapult Parts"},
                    {job_types.ConstructBallistaParts, "Construct", "Ballista Parts"},
                }, mat_flags)
            end
        end
    end
    
    -- Traction Bench
    table.insert(result, reaction_entry(job_types.ConstructTractionBench))
    
    -- Non-metal weapons
    resource_reactions(result, job_types.MakeWeapon, materials.wood, entity.resources.weapon_type, itemdefs.weapons, {
        permissible = (function(itemdef) return itemdef.skill_ranged >= 0 end),
    })
    
    resource_reactions(result, job_types.MakeWeapon, materials.wood, entity.resources.training_weapon_type, itemdefs.weapons, {
    })
    
    resource_reactions(result, job_types.MakeWeapon, materials.bone, entity.resources.weapon_type, itemdefs.weapons, {
        permissible = (function(itemdef) return itemdef.skill_ranged >= 0 end),
    })
    
    resource_reactions(result, job_types.MakeWeapon, materials.rock, entity.resources.weapon_type, itemdefs.weapons, {
        permissible = (function(itemdef) return itemdef.flags.CAN_STONE end),
    })
    
    -- Wooden items
    -- Closely related to the ITEMS_HARD list.
    material_reactions(result, {
        {job_types.ConstructDoor, "Construct", "Door"},
        {job_types.ConstructFloodgate, "Construct", "Floodgate"},
        {job_types.ConstructHatchCover, "Construct", "Hatch Cover"},
        {job_types.ConstructGrate, "Construct", "Grate"},
        {job_types.ConstructThrone, "Construct", "Chair"},
        {job_types.ConstructCoffin, "Construct", "Casket"},
        {job_types.ConstructTable, "Construct", "Table"},
        {job_types.ConstructArmorStand, "Construct", "Armor Stand"},
        {job_types.ConstructWeaponRack, "Construct", "Weapon Rack"},
        {job_types.ConstructCabinet, "Construct", "Cabinet"},
        {job_types.MakeGoblet, "Make", "Cup"},
        {job_types.MakeInstrument, "Make", "Instrument"},
    }, materials.wood)
    
    resource_reactions(result, job_types.MakeTool, materials.wood, entity.resources.tool_type, itemdefs.tools, {
        -- permissible = (function(itemdef) return itemdef.flags.WOOD_MAT end),
        capitalize = true,
    })
    
    material_reactions(result, {
        {job_types.MakeToy, "Make", "Toy"},
        {job_types.ConstructBlocks, "Construct", "Blocks"},
        {job_types.ConstructSplint, "Construct", "Splint"},
        {job_types.ConstructCrutch, "Construct", "Crutch"},
        {job_types.MakeAnimalTrap, "Make", "Animal Trap"},
        {job_types.MakeBarrel, "Make", "Barrel"},
        {job_types.MakeBucket, "Make", "Bucket"},
        {job_types.ConstructBin, "Construct", "Bin"},
        {job_types.MakeCage, "Make", "Cage"},
        {job_types.MakePipeSection, "Make", "Pipe Section"},
    }, materials.wood)
    
    resource_reactions(result, job_types.MakeTrapComponent, materials.wood, entity.resources.trapcomp_type, itemdefs.trapcomps, {
        permissible = (function(itemdef) return itemdef.flags.WOOD end),
        adjective = true,
    })
    
    -- Rock items
    material_reactions(result, {
        {job_types.ConstructDoor, "Construct", "Door"},
        {job_types.ConstructFloodgate, "Construct", "Floodgate"},
        {job_types.ConstructHatchCover, "Construct", "Hatch Cover"},
        {job_types.ConstructGrate, "Construct", "Grate"},
        {job_types.ConstructThrone, "Construct", "Throne"},
        {job_types.ConstructCoffin, "Construct", "Coffin"},
        {job_types.ConstructTable, "Construct", "Table"},
        {job_types.ConstructArmorStand, "Construct", "Armor Stand"},
        {job_types.ConstructWeaponRack, "Construct", "Weapon Rack"},
        {job_types.ConstructCabinet, "Construct", "Cabinet"},
        {job_types.MakeGoblet, "Make", "Mug"},
        {job_types.MakeInstrument, "Make", "Instrument"},
    }, materials.rock)
    
    resource_reactions(result, job_types.MakeTool, materials.rock, entity.resources.tool_type, itemdefs.tools, {
        permissible = (function(itemdef) return itemdef.flags.HARD_MAT end),
        capitalize = true,
    })
    
    material_reactions(result, {
        {job_types.MakeToy, "Make", "Toy"},
        {job_types.ConstructQuern, "Construct", "Quern"},
        {job_types.ConstructMillstone, "Construct", "Millstone"},
        {job_types.ConstructSlab, "Construct", "Slab"},
        {job_types.ConstructStatue, "Construct", "Statue"},
        {job_types.ConstructBlocks, "Construct", "Blocks"},
    }, materials.rock)
    
    -- Glass items
    for _, mat_info in ipairs(glasses) do
        material_reactions(result, {
            {job_types.ConstructDoor, "Construct", "Portal"},
            {job_types.ConstructFloodgate, "Construct", "Floodgate"},
            {job_types.ConstructHatchCover, "Construct", "Hatch Cover"},
            {job_types.ConstructGrate, "Construct", "Grate"},
            {job_types.ConstructThrone, "Construct", "Throne"},
            {job_types.ConstructCoffin, "Construct", "Coffin"},
            {job_types.ConstructTable, "Construct", "Table"},
            {job_types.ConstructArmorStand, "Construct", "Armor Stand"},
            {job_types.ConstructWeaponRack, "Construct", "Weapon Rack"},
            {job_types.ConstructCabinet, "Construct", "Cabinet"},
            {job_types.MakeGoblet, "Make", "Goblet"},
            {job_types.MakeInstrument, "Make", "Instrument"},
        }, mat_info)
        
        resource_reactions(result, job_types.MakeTool, mat_info, entity.resources.tool_type, itemdefs.tools, {
            permissible = (function(itemdef) return itemdef.flags.HARD_MAT end),
            capitalize = true,
        })
        
        material_reactions(result, {
            {job_types.MakeToy, "Make", "Toy"},
            {job_types.ConstructStatue, "Construct", "Statue"},
            {job_types.ConstructBlocks, "Construct", "Blocks"},
            {job_types.MakeCage, "Make", "Terrarium"},
            {job_types.MakePipeSection, "Make", "Tube"},
        }, mat_info)
        
        resource_reactions(result, job_types.MakeTrapComponent, mat_info, entity.resources.trapcomp_type, itemdefs.trapcomps, {
            adjective = true,
        })
    end
    
    -- Bed, specified as wooden.
    table.insert(result, reaction_entry(job_types.ConstructBed, materials.wood.management))
    
    -- Windows
    for _, mat_info in ipairs(glasses) do
        material_reactions(result, {
            {job_types.MakeWindow, "Make", "Window"},
        }, mat_info)
    end
    
    -- Rock Mechanisms
    table.insert(result, reaction_entry(job_types.ConstructMechanisms, materials.rock.management))
    
    resource_reactions(result, job_types.AssembleSiegeAmmo, materials.wood, entity.resources.siegeammo_type, itemdefs.siege_ammo, {
        verb = "Assemble",
    })
    
    for _, mat_info in ipairs(glasses) do
        material_reactions(result, {
            {job_types.MakeRawGlass, "Make Raw", nil},
        }, mat_info)
    end
    
    material_reactions(result, {
        {job_types.MakeBackpack, "Make", "Backpack"},
        {job_types.MakeQuiver, "Make", "Quiver"},
    }, materials.leather)
    
    for _, material in ipairs(cloth_mats) do
        clothing_reactions(result, material, (function(itemdef) return itemdef.props.flags[material.clothing_flag or "SOFT"] end))
    end
    
    -- Boxes, Bags, and Ropes
    boxmats = {
        {mats = {materials.wood}, box = "Chest"},
        {mats = {materials.rock}, box = "Coffer"},
        {mats = glasses, box = "Box", flask = "Vial"},
        {mats = {materials.cloth}, box = "Bag", chain = "Rope"},
        {mats = {materials.leather}, box = "Bag", flask = "Waterskin"},
        {mats = {materials.silk, materials.yarn}, box = "Bag", chain = "Rope"},
    }
    for _, boxmat in ipairs(boxmats) do
        for _, mat in ipairs(boxmat.mats) do
            material_reactions(result, {{job_types.ConstructChest, "Construct", boxmat.box}}, mat)
            if boxmat.chain then
                material_reactions(result, {{job_types.MakeChain, "Make", boxmat.chain}}, mat)
            end
            if boxmat.flask then
                material_reactions(result, {{job_types.MakeFlask, "Make", boxmat.flask}}, mat)
            end
        end
    end
    
    for _, mat in ipairs{
        materials.wood,
        materials.rock,
        materials.cloth,
        materials.leather,
        materials.shell,
        materials.bone,
        materials.silk,
        materials.tooth,
        materials.horn,
        materials.pearl,
        materials.yarn,
    } do
        material_reactions(result, {{job_types.MakeCrafts, "Make", "Crafts"}}, mat)
    end
    
    -- Siege engine parts
    table.insert(result, reaction_entry(job_types.ConstructCatapultParts, materials.wood.management))
    table.insert(result, reaction_entry(job_types.ConstructBallistaParts, materials.wood.management))
    
    for _, mat in ipairs{materials.wood, materials.bone} do
        resource_reactions(result, job_types.MakeAmmo, mat, entity.resources.ammo_type, itemdefs.ammo, {
            name_field = "name_plural",
        })
    end
    
    -- BARRED and SCALED as flag names don't quite seem to fit, here.
    clothing_reactions(result, materials.bone, (function(itemdef) return itemdef.props.flags.BARRED end))
    clothing_reactions(result, materials.shell, (function(itemdef) return itemdef.props.flags.SCALED end))
    
    for _, mat in ipairs{materials.wood, materials.leather} do
        resource_reactions(result, job_types.MakeShield, mat, entity.resources.shield_type, itemdefs.shields, {})
    end
    
    -- Melt a Metal Object
    table.insert(result, reaction_entry(job_types.MeltMetalObject))
    
    return result
end

screen = gui.FramedScreen {
    frame_title = "Select Stockpile Order",
}

function screen:onRenderBody(dc)
    -- Emulates the built-in manager screen.
    dc:seek(1, 1):string("Type in parts of the name to narrow your search.  ", COLOR_WHITE)
    dc:string(gui.getKeyDisplay("LEAVESCREEN"), COLOR_LIGHTGREEN)
    dc:string(" to abort.", COLOR_WHITE)
    dc:seek(1, PageSize + 5):string(self.search_string, COLOR_LIGHTCYAN)
    for _, item in ipairs(self.displayed) do
        dc:seek(item.x, item.y):string(item.name, item.color)
    end
end

function screen:onInput(keys)
    if keys.LEAVESCREEN then
        self:dismiss()
    elseif keys.SELECT then
        self:dismiss()
        local selected = self.reactions[self.position]
        if selected then
            store_order(self.stockpile, selected.index)
        end
    elseif keys.STANDARDSCROLL_UP then
        self.position = self.position - 1
    elseif keys.STANDARDSCROLL_DOWN then
        self.position = self.position + 1
    elseif keys.STANDARDSCROLL_LEFT then
        self.position = self.position - PageSize
    elseif keys.STANDARDSCROLL_RIGHT then
        self.position = self.position + PageSize
    elseif keys.STANDARDSCROLL_PAGEUP then
        -- Moves to the first item displayed on the new page, for some reason.
        self.position = self.position - PageSize*2 - ((self.position-1) % (PageSize*2))
    elseif keys.STANDARDSCROLL_PAGEDOWN then
        -- Moves to the first item displayed on the new page, for some reason.
        self.position = self.position + PageSize*2 - ((self.position-1) % (PageSize*2))
    elseif keys.STRING_A000 then
        -- This seems like an odd way to check for Backspace.
        self.search_string = string.sub(self.search_string, 1, -2)
    elseif keys._STRING and keys._STRING >= 32 then
        -- This interface only accepts letters and spaces.
        local char = string.char(keys._STRING)
        if char == " " or string.find(char, "^%a") then
            self.search_string = self.search_string .. string.upper(char)
        end
    end
    
    self:refilter()
end

function screen:reset(stockpile)
    self.stockpile = stockpile
    self.search_string = ""
    self.position = 1
    self:refilter()
end

function matchall(haystack, needles)
    for _, needle in ipairs(needles) do
        if not string.find(haystack, needle) then
            return false
        end
    end
    
    return true
end

function splitstring(full, pattern)
    local last = string.len(full)
    local result = {}
    local n = 1
    while n <= last do
        local start, stop = string.find(full, pattern, n)
        if not start then
            result[#result+1] = string.sub(full, n)
            break
        elseif start > n then
            result[#result+1] = string.sub(full, n, start - 1)
        end
        
        if stop < n then
            -- The pattern matches an empty string.
            -- Avoid an infinite loop.
            break
        end
        
        n = stop + 1
    end
    
    return result
end

function screen:refilter()
    local filtered = {}
    local needles = splitstring(self.search_string, " ")
    for key, value in ipairs(reaction_list) do
        if matchall(string.upper(value.name), needles) then
            filtered[#filtered+1] = {
                index = key,
                name = value.name
            }
        end
    end
    
    if self.position < 1 then
        self.position = #filtered
    elseif self.position > #filtered then
        self.position = 1
    end
    
    local start = 1
    while self.position >= start + PageSize*2 do
        start = start + PageSize*2
    end
    
    local displayed = {}
    for n = 0, PageSize*2 - 1 do
        local item = filtered[start + n]
        if not item then
            break
        end
        local name = item.name
        
        local x = 1
        local y = FirstRow + n
        if n >= PageSize then
            x = CenterCol
            y = y - PageSize
            name = " "..name
        end
        
        local color = COLOR_CYAN
        if start + n == self.position then
            color = COLOR_LIGHTCYAN
        end
        
        displayed[n + 1] = {
            x = x,
            y = y,
            name = name,
            color = color,
        }
    end
    
    self.reactions = filtered
    self.displayed = displayed
end

function store_order(stockpile, order_number)
    local name = reaction_list[order_number].name
    -- print("Setting stockpile #"..stockpile.stockpile_number.." to "..name.." (#"..order_number..")")
    local saved = saved_orders[stockpile.id]
    if saved then
        saved.entry.value = name
        saved.entry.ints[entry_ints.order_number] = order_number
        saved.entry:save()
    else
        saved_orders[stockpile.id] = {
            stockpile = stockpile,
            entry = dfhack.persistent.save{
                key = "stockflow/entry/"..stockpile.id,
                value = name,
                ints = {
                    stockpile.id,
                    order_number,
                    1,
                },
            },
        }
    end
end

-- Compare the job specification of two orders.
function orders_match(a, b)
    local fields = {
        "job_type",
        "item_subtype",
        "reaction_name",
        "mat_type",
        "mat_index",
    }
    
    for _, fieldname in ipairs(fields) do
        if a[fieldname] ~= b[fieldname] then
            return false
        end
    end
    
    local subtables = {
        "item_category",
        "material_category",
    }
    
    for _, fieldname in ipairs(subtables) do
        local aa = a[fieldname]
        local bb = b[fieldname]
        for key, value in ipairs(aa) do
            if bb[key] ~= value then
                return false
            end
        end
    end
    
    return true
end

-- Reduce the quantity by the number of matching orders in the queue.
function order_quantity(order, quantity)
    local amount = quantity
    for _, managed in ipairs(df.global.world.manager_orders) do
        if orders_match(order, managed) then
            amount = amount - managed.amount_left
            if amount < 0 then
                return 0
            end
        end
    end
    
    if amount > 30 then
        -- Respect the quantity limit.
        -- With this many in the queue, we can wait for the next cycle.
        return 30
    end
    
    return amount
end

-- Place a new copy of the order onto the manager's queue.
function create_orders(order, amount)
    local new_order = order:new()
    new_order.amount_left = amount
    new_order.amount_total = amount
    -- Todo: Create in a validated state if the fortress is small enough?
    new_order.is_validated = 0
    df.global.world.manager_orders:insert('#', new_order)
end

function findItemsAtTile(x, y, z)
    -- There should be a faster and easier way to do this...
    local found = {}
    for _, item in ipairs(df.global.world.items.all) do
        -- local ix, iy, iz = dfhack.items.getPosition(item)
        if item.pos.x == x and item.pos.y == y and
            item.pos.z == z and item.flags.on_ground then
            found[#found+1] = item
        end
    end
    
    return found
end

function countContents(container, settings)
    local total = 0
    local blocking = false
    for _, item in ipairs(dfhack.items.getContainedItems(container)) do
        if item.flags.container then
            -- Recursively count the total of items contained.
            -- Not likely to go more than two levels deep.
            local subtotal, subblocked = countContents(item, settings)
            if subtotal > 0 then
                -- Ignore the inner container itself;
                -- generally, only the contained items matter.
                total = total + subtotal
            elseif subblocked then
                blocking = true
            elseif matches_stockpile(item, settings) then
                -- The container may or may not be empty,
                -- but is stockpiled as a container itself.
                total = total + 1
            else
                blocking = true
            end
        elseif matches_stockpile(item, settings) then
            total = total + 1
        else
            blocking = true
        end
    end
    
    return total, blocking
end

function check_stockpiles(verbose)
    local result = {}
    for _, spec in pairs(saved_orders) do
        local trigger = triggers[spec.entry.ints[entry_ints.trigger_number]]
        if trigger and trigger.divisor then
            local reaction = spec.entry.ints[entry_ints.order_number]
            local filled, empty = check_pile(spec.stockpile, verbose)
            local amount = trigger.filled and filled or empty
            amount = (amount - (amount % trigger.divisor)) / trigger.divisor
            result[reaction] = (result[reaction] or 0) + amount
        end
    end
    
    return result
end

function check_pile(sp, verbose)
    local numspaces = 0
    local filled = 0
    local empty = 0
    for y = sp.y1, sp.y2 do
        for x = sp.x1, sp.x2 do
            -- Sadly, the obvious check currently fails when y == sp.y2
            if dfhack.buildings.containsTile(sp, x, y) or
                (y == sp.y2 and dfhack.buildings.findAtTile(x, y, sp.z) == sp) then
                numspaces = numspaces + 1
                local designation, occupancy = dfhack.maps.getTileFlags(x, y, sp.z)
                if not designation.liquid_type then
                    if not occupancy.item then
                        empty = empty + 1
                    else
                        local item_count, blocked = count_pile_items(sp, x, y)
                        if item_count > 0 then
                            filled = filled + item_count
                        elseif not blocked then
                            empty = empty + 1
                        end
                    end
                end
            end
        end
    end
    
    if verbose then
        print("Stockpile #"..sp.stockpile_number,
            string.format("%3d spaces", numspaces),
            string.format("%4d items", filled),
            string.format("%4d empty spaces", empty))
    end
    
    return filled, empty
end

function count_pile_items(sp, x, y)
    local item_count = 0
    local blocked = false
    for _, item in ipairs(findItemsAtTile(x, y, sp.z)) do
        if item:isAssignedToThisStockpile(sp.id) then
            -- This is a bin or barrel associated with the stockpile.
            -- If it's empty, it doesn't count as blocking the stockpile space.
            -- Oddly, when empty, item.flags.container might be false.
            local subtotal, subblocked = countContents(item, sp.settings)
            item_count = item_count + subtotal
            if subblocked then
                blocked = true
            end
        elseif matches_stockpile(item, sp.settings) then
            item_count = item_count + 1
        else
            blocked = true
        end
    end
    
    return item_count, blocked
end

function matches_stockpile(item, settings)
    -- Check whether the item matches the stockpile.
    -- FIXME: This is starting to look like a whole lot of work.
    if df.item_barst:is_instance(item) then
        return settings.flags.bars_blocks
    elseif df.item_blocksst:is_instance(item) then
        return settings.flags.bars_blocks
    elseif df.item_smallgemst:is_instance(item) then
        return settings.flags.gems
    elseif df.item_boulderst:is_instance(item) then
        return settings.flags.stone
    elseif df.item_woodst:is_instance(item) then
        return settings.flags.wood
    elseif df.item_seedsst:is_instance(item) then
        return settings.flags.food
    elseif df.item_meatst:is_instance(item) then
        return settings.flags.food
    elseif df.item_plantst:is_instance(item) then
        return settings.flags.food
    elseif df.item_leavesst:is_instance(item) then
        return settings.flags.food
    elseif df.item_cheesest:is_instance(item) then
        return settings.flags.food
    elseif df.item_globst:is_instance(item) then
        return settings.flags.food
    elseif df.item_fishst:is_instance(item) then
        return settings.flags.food
    elseif df.item_fish_rawst:is_instance(item) then
        return settings.flags.food
    elseif df.item_foodst:is_instance(item) then
        return settings.flags.food
    elseif df.item_drinkst:is_instance(item) then
        return settings.flags.food
    elseif df.item_eggst:is_instance(item) then
        return settings.flags.food
    elseif df.item_skin_tannedst:is_instance(item) then
        return settings.flags.leather
    elseif df.item_remainsst:is_instance(item) then
        return settings.flags.refuse
    elseif df.item_verminst:is_instance(item) then
        return settings.flags.animals
    elseif df.item_petst:is_instance(item) then
        return settings.flags.animals
    elseif df.item_threadst:is_instance(item) then
        return settings.flags.cloth
    end
    
    return true
end

return _ENV
