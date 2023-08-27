-- copy of stockflow logic for use until stockflow becomes available
--@ module = true

local function reaction_entry(reactions, job_type, values, name)
    if not job_type then
        -- Perhaps df.job_type.something returned nil for an unknown job type.
        -- We could warn about it; in any case, don't add it to the list.
        return
    end

    local order = df.manager_order:new()
    -- These defaults differ from the newly created order's.
    order:assign{
        job_type = job_type,
        item_type = -1,
        item_subtype = -1,
        mat_type = -1,
        mat_index = -1,
    }

    if values then
        -- Override default attributes.
        order:assign(values)
    end

    table.insert(reactions, {
        name = name or df.job_type.attrs[job_type].caption,
        order = order,
    })
end

local function resource_reactions(reactions, job_type, mat_info, keys, items, options)
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
            reaction_entry(reactions, job_type, values, start.." "..mat_info.adjective..item_name)
        end
    end
end

local function material_reactions(reactions, itemtypes, mat_info)
    -- Expects a list of {job_type, verb, item_name} tuples.
    for _, row in ipairs(itemtypes) do
        local line = row[2].." "..mat_info.adjective
        if row[3] then
            line = line.." "..row[3]
        end

        reaction_entry(reactions, row[1], mat_info.management, line)
    end
end

local function clothing_reactions(reactions, mat_info, filter)
    local resources = df.historical_entity.find(df.global.plotinfo.civ_id).resources
    local itemdefs = df.global.world.raws.itemdefs
    local job_types = df.job_type
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

    -- Caching the enumeration might not be important, but saves lookups.
    local job_types = df.job_type

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
    materials.shell.short = true
    materials.pearl.short = true

    -- Collection and Entrapment
    reaction_entry(result, job_types.CollectWebs)
    reaction_entry(result, job_types.CollectSand)
    reaction_entry(result, job_types.CollectClay)
    reaction_entry(result, job_types.CatchLiveLandAnimal)
    reaction_entry(result, job_types.CatchLiveFish)

    -- Cutting, encrusting, and metal extraction.
    local rock_types = df.global.world.raws.inorganics
    for rock_id = #rock_types-1, 0, -1 do
        local material = rock_types[rock_id].material
        local rock_name = material.state_adj.Solid
        if material.flags.IS_STONE or material.flags.IS_GEM then
            reaction_entry(result, job_types.CutGems, {
                mat_type = 0,
                mat_index = rock_id,
            }, "Cut "..rock_name)

            reaction_entry(result, job_types.EncrustWithGems, {
                mat_type = 0,
                mat_index = rock_id,
                item_category = {finished_goods = true},
            }, "Encrust Finished Goods With "..rock_name)

            reaction_entry(result, job_types.EncrustWithGems, {
                mat_type = 0,
                mat_index = rock_id,
                item_category = {furniture = true},
            }, "Encrust Furniture With "..rock_name)

            reaction_entry(result, job_types.EncrustWithGems, {
                mat_type = 0,
                mat_index = rock_id,
                item_category = {ammo = true},
            }, "Encrust Ammo With "..rock_name)
        end

        if #rock_types[rock_id].metal_ore.mat_index > 0 then
            reaction_entry(result, job_types.SmeltOre, {mat_type = 0, mat_index = rock_id}, "Smelt "..rock_name.." Ore")
        end

        if #rock_types[rock_id].thread_metal.mat_index > 0 then
            reaction_entry(result, job_types.ExtractMetalStrands, {mat_type = 0, mat_index = rock_id})
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

            reaction_entry(result, job_types.CutGlass, {mat_type = glass_id}, "Cut "..glass_name)

            reaction_entry(result, job_types.EncrustWithGlass, {
                mat_type = glass_id,
                item_category = {finished_goods = true},
            }, "Encrust Finished Goods With "..glass_name)

            reaction_entry(result, job_types.EncrustWithGlass, {
                mat_type = glass_id,
                item_category = {furniture = true},
            }, "Encrust Furniture With "..glass_name)

            reaction_entry(result, job_types.EncrustWithGlass, {
                mat_type = glass_id,
                item_category = {ammo = true},
            }, "Encrust Ammo With "..glass_name)
        end
    end

    -- Dyeing
    reaction_entry(result, job_types.DyeThread)
    reaction_entry(result, job_types.DyeCloth)

    -- Sew Image
    local cloth_mats = {materials.cloth, materials.silk, materials.yarn, materials.leather}
    for _, material in ipairs(cloth_mats) do
        material_reactions(result, {{job_types.SewImage, "Sew", "Image"}}, material)
        material.cloth = true
    end

    for _, spec in ipairs{materials.bone, materials.shell, materials.tooth, materials.horn, materials.pearl} do
        material_reactions(result, {{job_types.DecorateWith, "Decorate With"}}, spec)
    end

    reaction_entry(result, job_types.MakeTotem)
    reaction_entry(result, job_types.ButcherAnimal)
    reaction_entry(result, job_types.MillPlants)
    reaction_entry(result, job_types.MakePotashFromLye)
    reaction_entry(result, job_types.MakePotashFromAsh)

    -- Kitchen
    reaction_entry(result, job_types.PrepareMeal, {mat_type = 2}, "Prepare Easy Meal")
    reaction_entry(result, job_types.PrepareMeal, {mat_type = 3}, "Prepare Fine Meal")
    reaction_entry(result, job_types.PrepareMeal, {mat_type = 4}, "Prepare Lavish Meal")

    -- Brew Drink
    reaction_entry(result, job_types.BrewDrink)

    -- Weaving
    reaction_entry(result, job_types.WeaveCloth, {material_category = {plant = true}}, "Weave Thread into Cloth")
    reaction_entry(result, job_types.WeaveCloth, {material_category = {silk = true}}, "Weave Thread into Silk")
    reaction_entry(result, job_types.WeaveCloth, {material_category = {yarn = true}}, "Weave Yarn into Cloth")

    -- Extracts, farmer's workshop, and wood burning
    reaction_entry(result, job_types.ExtractFromPlants)
    reaction_entry(result, job_types.ExtractFromRawFish)
    reaction_entry(result, job_types.ExtractFromLandAnimal)
    reaction_entry(result, job_types.PrepareRawFish)
    reaction_entry(result, job_types.MakeCheese)
    reaction_entry(result, job_types.MilkCreature)
    reaction_entry(result, job_types.ShearCreature)
    reaction_entry(result, job_types.SpinThread, {material_category = {strand = true}})
    reaction_entry(result, job_types.MakeLye)
    reaction_entry(result, job_types.ProcessPlants)
    reaction_entry(result, job_types.ProcessPlantsBag)
    reaction_entry(result, job_types.ProcessPlantsVial)
    reaction_entry(result, job_types.ProcessPlantsBarrel)
    reaction_entry(result, job_types.MakeCharcoal)
    reaction_entry(result, job_types.MakeAsh)

    -- Reactions defined in the raws.
    -- Not all reactions are allowed to the civilization.
    -- That includes "Make sharp rock" by default.
    local entity = df.historical_entity.find(df.global.plotinfo.civ_id)
    if not entity then
        -- No global civilization; arena mode?
        -- Anyway, skip remaining reactions, since many depend on the civ.
        return result
    end

    for _, reaction_id in ipairs(entity.entity_raw.workshops.permitted_reaction_id) do
        local reaction = df.global.world.raws.reactions.reactions[reaction_id]
        local name = string.gsub(reaction.name, "^.", string.upper)
        reaction_entry(result, job_types.CustomReaction, {reaction_name = reaction.code}, name)
    end

    -- Reactions generated by the game.
    for _, reaction in ipairs(df.global.world.raws.reactions.reactions) do
        if reaction.source_enid == entity.id then
            local name = string.gsub(reaction.name, "^.", string.upper)
            reaction_entry(result, job_types.CustomReaction, {reaction_name = reaction.code}, name)
        end
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
            reaction_entry(result, job_types.StudWith, mat_flags.management, "Stud With "..rock_name)

            if material.flags.ITEMS_WEAPON then
                -- Todo: Are these really the right flags to check?
                resource_reactions(result, job_types.MakeWeapon, mat_flags, entity.resources.weapon_type, itemdefs.weapons, {
                    permissible = (function(itemdef) return itemdef.skill_ranged == -1 end),
                })

                -- Is this entirely disconnected from the entity?
                material_reactions(result, {{job_types.MakeBallistaArrowHead, "Forge", "Ballista Arrow Head"}}, mat_flags)

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

            resource_reactions(result, job_types.MakeTool, mat_flags, entity.resources.tool_type, itemdefs.tools, {
                permissible = (function(itemdef) return ((material.flags.ITEMS_HARD and itemdef.flags.HARD_MAT) or (material.flags.ITEMS_METAL and itemdef.flags.METAL_MAT)) and not itemdef.flags.NO_DEFAULT_JOB end),
                capitalize = true,
            })

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
                    {job_types.MakeFigurine, "Make", "Figurine"},
                    {job_types.MakeAmulet, "Make", "Amulet"},
                    {job_types.MakeScepter, "Make", "Scepter"},
                    {job_types.MakeCrown, "Make", "Crown"},
                    {job_types.MakeRing, "Make", "Ring"},
                    {job_types.MakeEarring, "Make", "Earring"},
                    {job_types.MakeBracelet, "Make", "Bracelet"},
                    {job_types.MakeGem, "Make Large", "Gem"},
                    {job_types.ConstructMechanisms, "Construct", "Mechanisms"},
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
    reaction_entry(result, job_types.ConstructTractionBench)

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
        -- permissible = (function(itemdef) return itemdef.flags.WOOD_MAT and not itemdef.flags.NO_DEFAULT_JOB end),
        permissible = (function(itemdef) return not itemdef.flags.NO_DEFAULT_JOB end),
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
    reaction_entry(result, job_types.ConstructBed, materials.wood.management)

    -- Windows
    for _, mat_info in ipairs(glasses) do
        material_reactions(result, {
            {job_types.MakeWindow, "Make", "Window"},
        }, mat_info)
    end

    -- Rock Mechanisms
    reaction_entry(result, job_types.ConstructMechanisms, materials.rock.management)

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
    local boxmats = {
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

    -- Crafts
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
        material_reactions(result, {
            {job_types.MakeCrafts, "Make", "Crafts"},
            {job_types.MakeAmulet, "Make", "Amulet"},
            {job_types.MakeBracelet, "Make", "Bracelet"},
            {job_types.MakeEarring, "Make", "Earring"},
        }, mat)

        if not mat.cloth then
            material_reactions(result, {
                {job_types.MakeCrown, "Make", "Crown"},
                {job_types.MakeFigurine, "Make", "Figurine"},
                {job_types.MakeRing, "Make", "Ring"},
                {job_types.MakeGem, "Make Large", "Gem"},
            }, mat)

            if not mat.short then
                material_reactions(result, {
                    {job_types.MakeScepter, "Make", "Scepter"},
                }, mat)
            end
        end
    end

    -- Siege engine parts
    reaction_entry(result, job_types.ConstructCatapultParts, materials.wood.management)
    reaction_entry(result, job_types.ConstructBallistaParts, materials.wood.management)

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
    reaction_entry(result, job_types.MeltMetalObject)

    return result
end

-- Place a new copy of the order onto the manager's queue.
function create_orders(order, amount)
    local new_order = order:new()
    amount = math.floor(amount)
    new_order.amount_left = amount
    new_order.amount_total = amount
    -- Todo: Create in a validated state if the fortress is small enough?
    new_order.status.validated = false
    new_order.status.active = false
    new_order.id = df.global.world.manager_order_next_id
    df.global.world.manager_order_next_id = df.global.world.manager_order_next_id + 1
    df.global.world.manager_orders:insert('#', new_order)
end
