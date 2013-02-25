local dfhack = dfhack
local _ENV = dfhack.BASE_G
local buildings = dfhack.buildings

local utils = require 'utils'

-- Uninteresting values for filter attributes when reading them from DF memory.
-- Differs from the actual defaults of the job_item constructor in allow_artifact.

buildings.input_filter_defaults = {
    item_type = -1,
    item_subtype = -1,
    mat_type = -1,
    mat_index = -1,
    flags1 = {},
    -- Instead of noting those that allow artifacts, mark those that forbid them.
    -- Leaves actually enabling artifacts to the discretion of the API user,
    -- which is the right thing because unlike the game UI these filters are
    -- used in a way that does not give the user a chance to choose manually.
    flags2 = { allow_artifact = true },
    flags3 = {},
    flags4 = 0,
    flags5 = 0,
    reaction_class = '',
    has_material_reaction_product = '',
    metal_ore = -1,
    min_dimension = -1,
    has_tool_use = -1,
    quantity = 1
}

--[[ Building input material table. ]]

local building_inputs = {
    [df.building_type.Chair] = { { item_type=df.item_type.CHAIR, vector_id=df.job_item_vector_id.CHAIR } },
    [df.building_type.Bed] = { { item_type=df.item_type.BED, vector_id=df.job_item_vector_id.BED } },
    [df.building_type.Table] = { { item_type=df.item_type.TABLE, vector_id=df.job_item_vector_id.TABLE } },
    [df.building_type.Coffin] = { { item_type=df.item_type.COFFIN, vector_id=df.job_item_vector_id.COFFIN } },
    [df.building_type.FarmPlot] = { },
    [df.building_type.TradeDepot] = { { flags2={ building_material=true, non_economic=true }, quantity=3 } },
    [df.building_type.Door] = { { item_type=df.item_type.DOOR, vector_id=df.job_item_vector_id.DOOR } },
    [df.building_type.Floodgate] = {
        {
            item_type=df.item_type.FLOODGATE,
            vector_id=df.job_item_vector_id.FLOODGATE
        }
    },
    [df.building_type.Box] = {
        {
            flags1={ empty=true },
            item_type=df.item_type.BOX,
            vector_id=df.job_item_vector_id.BOX
        }
    },
    [df.building_type.Weaponrack] = {
        {
            item_type=df.item_type.WEAPONRACK,
            vector_id=df.job_item_vector_id.WEAPONRACK
        }
    },
    [df.building_type.Armorstand] = {
        {
            item_type=df.item_type.ARMORSTAND,
            vector_id=df.job_item_vector_id.ARMORSTAND
        }
    },
    [df.building_type.Cabinet] = {
        { item_type=df.item_type.CABINET, vector_id=df.job_item_vector_id.CABINET }
    },
    [df.building_type.Statue] = { { item_type=df.item_type.STATUE, vector_id=df.job_item_vector_id.STATUE } },
    [df.building_type.WindowGlass] = { { item_type=df.item_type.WINDOW, vector_id=df.job_item_vector_id.WINDOW } },
    [df.building_type.WindowGem] = {
        {
            item_type=df.item_type.SMALLGEM,
            quantity=3,
            vector_id=df.job_item_vector_id.ANY_GENERIC35
        }
    },
    [df.building_type.Well] = {
        {
            item_type=df.item_type.BLOCKS,
            vector_id=df.job_item_vector_id.ANY_GENERIC35
        },
        {
            name='bucket',
            flags2={ lye_milk_free=true },
            item_type=df.item_type.BUCKET,
            vector_id=df.job_item_vector_id.BUCKET
        },
        {
            name='chain',
            item_type=df.item_type.CHAIN,
            vector_id=df.job_item_vector_id.CHAIN
        },
        {
            name='mechanism',
            item_type=df.item_type.TRAPPARTS,
            vector_id=df.job_item_vector_id.TRAPPARTS
        }
    },
    [df.building_type.Bridge] = { { flags2={ building_material=true, non_economic=true }, quantity=-1 } },
    [df.building_type.RoadDirt] = { },
    [df.building_type.RoadPaved] = { { flags2={ building_material=true, non_economic=true }, quantity=-1 } },
    [df.building_type.AnimalTrap] = {
        {
            flags1={ empty=true },
            item_type=df.item_type.ANIMALTRAP,
            vector_id=df.job_item_vector_id.ANIMALTRAP
        }
    },
    [df.building_type.Support] = { { flags2={ building_material=true, non_economic=true } } },
    [df.building_type.ArcheryTarget] = { { flags2={ building_material=true, non_economic=true } } },
    [df.building_type.Chain] = { { item_type=df.item_type.CHAIN, vector_id=df.job_item_vector_id.CHAIN } },
    [df.building_type.Cage] = { { item_type=df.item_type.CAGE, vector_id=df.job_item_vector_id.CAGE } },
    [df.building_type.Weapon] = { { name='weapon', vector_id=df.job_item_vector_id.ANY_SPIKE } },
    [df.building_type.ScrewPump] = {
        {
            item_type=df.item_type.BLOCKS,
            vector_id=df.job_item_vector_id.ANY_GENERIC35
        },
        {
            name='screw',
            flags2={ screw=true },
            item_type=df.item_type.TRAPCOMP,
            vector_id=df.job_item_vector_id.ANY_WEAPON
        },
        {
            name='pipe',
            item_type=df.item_type.PIPE_SECTION,
            vector_id=df.job_item_vector_id.PIPE_SECTION
        }
    },
    [df.building_type.Construction] = { { flags2={ building_material=true, non_economic=true } } },
    [df.building_type.Hatch] = {
        {
            item_type=df.item_type.HATCH_COVER,
            vector_id=df.job_item_vector_id.HATCH_COVER
        }
    },
    [df.building_type.GrateWall] = { { item_type=df.item_type.GRATE, vector_id=df.job_item_vector_id.GRATE } },
    [df.building_type.GrateFloor] = { { item_type=df.item_type.GRATE, vector_id=df.job_item_vector_id.GRATE } },
    [df.building_type.BarsVertical] = {
        { item_type=df.item_type.BAR, vector_id=df.job_item_vector_id.ANY_GENERIC35 }
    },
    [df.building_type.BarsFloor] = {
        { item_type=df.item_type.BAR, vector_id=df.job_item_vector_id.ANY_GENERIC35 }
    },
    [df.building_type.GearAssembly] = {
        {
            name='mechanism',
            item_type=df.item_type.TRAPPARTS,
            vector_id=df.job_item_vector_id.TRAPPARTS
        }
    },
    [df.building_type.AxleHorizontal] = {
        { item_type=df.item_type.WOOD, vector_id=df.job_item_vector_id.WOOD, quantity=-1 }
    },
    [df.building_type.AxleVertical] = { { item_type=df.item_type.WOOD, vector_id=df.job_item_vector_id.WOOD } },
    [df.building_type.WaterWheel] = {
        {
            item_type=df.item_type.WOOD,
            quantity=3,
            vector_id=df.job_item_vector_id.WOOD
        }
    },
    [df.building_type.Windmill] = {
        {
            item_type=df.item_type.WOOD,
            quantity=4,
            vector_id=df.job_item_vector_id.WOOD
        }
    },
    [df.building_type.TractionBench] = {
        {
            item_type=df.item_type.TRACTION_BENCH,
            vector_id=df.job_item_vector_id.TRACTION_BENCH
        }
    },
    [df.building_type.Slab] = { { item_type=df.item_type.SLAB } },
    [df.building_type.NestBox] = { { has_tool_use=df.tool_uses.NEST_BOX, item_type=df.item_type.TOOL } },
    [df.building_type.Hive] = { { has_tool_use=df.tool_uses.HIVE, item_type=df.item_type.TOOL } },
    [df.building_type.Rollers] = {
        {
            name='mechanism',
            item_type=df.item_type.TRAPPARTS,
            quantity=-1,
            vector_id=df.job_item_vector_id.TRAPPARTS
        },
        {
            name='chain',
            item_type=df.item_type.CHAIN,
            vector_id=df.job_item_vector_id.CHAIN
        }
    }
}

--[[ Furnace building input material table. ]]

local furnace_inputs = {
    [df.furnace_type.WoodFurnace] = { { flags2={ building_material=true, fire_safe=true, non_economic=true } } },
    [df.furnace_type.Smelter] = { { flags2={ building_material=true, fire_safe=true, non_economic=true } } },
    [df.furnace_type.GlassFurnace] = { { flags2={ building_material=true, fire_safe=true, non_economic=true } } },
    [df.furnace_type.Kiln] = { { flags2={ building_material=true, fire_safe=true, non_economic=true } } },
    [df.furnace_type.MagmaSmelter] = { { flags2={ building_material=true, magma_safe=true, non_economic=true } } },
    [df.furnace_type.MagmaGlassFurnace] = { { flags2={ building_material=true, magma_safe=true, non_economic=true } } },
    [df.furnace_type.MagmaKiln] = { { flags2={ building_material=true, magma_safe=true, non_economic=true } } }
}

--[[ Workshop building input material table. ]]

local workshop_inputs = {
    [df.workshop_type.Carpenters] = { { flags2={ building_material=true, non_economic=true } } },
    [df.workshop_type.Farmers] = { { flags2={ building_material=true, non_economic=true } } },
    [df.workshop_type.Masons] = { { flags2={ building_material=true, non_economic=true } } },
    [df.workshop_type.Craftsdwarfs] = { { flags2={ building_material=true, non_economic=true } } },
    [df.workshop_type.Jewelers] = { { flags2={ building_material=true, non_economic=true } } },
    [df.workshop_type.MetalsmithsForge] = {
        {
            name='anvil',
            flags2={ fire_safe=true },
            item_type=df.item_type.ANVIL,
            vector_id=df.job_item_vector_id.ANVIL
        },
        { flags2={ building_material=true, fire_safe=true, non_economic=true } }
    },
    [df.workshop_type.MagmaForge] = {
        {
            name='anvil',
            flags2={ magma_safe=true },
            item_type=df.item_type.ANVIL,
            vector_id=df.job_item_vector_id.ANVIL
        },
        { flags2={ building_material=true, magma_safe=true, non_economic=true } }
    },
    [df.workshop_type.Bowyers] = { { flags2={ building_material=true, non_economic=true } } },
    [df.workshop_type.Mechanics] = { { flags2={ building_material=true, non_economic=true } } },
    [df.workshop_type.Siege] = { { flags2={ building_material=true, non_economic=true }, quantity=3 } },
    [df.workshop_type.Butchers] = { { flags2={ building_material=true, non_economic=true } } },
    [df.workshop_type.Leatherworks] = { { flags2={ building_material=true, non_economic=true } } },
    [df.workshop_type.Tanners] = { { flags2={ building_material=true, non_economic=true } } },
    [df.workshop_type.Clothiers] = { { flags2={ building_material=true, non_economic=true } } },
    [df.workshop_type.Fishery] = { { flags2={ building_material=true, non_economic=true } } },
    [df.workshop_type.Still] = { { flags2={ building_material=true, non_economic=true } } },
    [df.workshop_type.Loom] = { { flags2={ building_material=true, non_economic=true } } },
    [df.workshop_type.Quern] = { { item_type=df.item_type.QUERN, vector_id=df.job_item_vector_id.QUERN } },
    [df.workshop_type.Kennels] = { { flags2={ building_material=true, non_economic=true } } },
    [df.workshop_type.Kitchen] = { { flags2={ building_material=true, non_economic=true } } },
    [df.workshop_type.Ashery] = {
        {
            item_type=df.item_type.BLOCKS,
            vector_id=df.job_item_vector_id.ANY_GENERIC35
        },
        {
            name='barrel',
            flags1={ empty=true },
            item_type=df.item_type.BARREL,
            vector_id=df.job_item_vector_id.BARREL
        },
        {
            name='bucket',
            flags2={ lye_milk_free=true },
            item_type=df.item_type.BUCKET,
            vector_id=df.job_item_vector_id.BUCKET
        }
    },
    [df.workshop_type.Dyers] = {
        {
            name='barrel',
            flags1={ empty=true },
            item_type=df.item_type.BARREL,
            vector_id=df.job_item_vector_id.BARREL
        },
        {
            name='bucket',
            flags2={ lye_milk_free=true },
            item_type=df.item_type.BUCKET,
            vector_id=df.job_item_vector_id.BUCKET
        }
    },
    [df.workshop_type.Millstone] = {
        {
            item_type=df.item_type.MILLSTONE,
            vector_id=df.job_item_vector_id.MILLSTONE
        },
        {
            name='mechanism',
            item_type=df.item_type.TRAPPARTS,
            vector_id=df.job_item_vector_id.TRAPPARTS
        }
    }
}

--[[ Trap building input material table. ]]

local trap_inputs = {
    [df.trap_type.StoneFallTrap] = {
        {
            name='mechanism',
            item_type=df.item_type.TRAPPARTS,
            vector_id=df.job_item_vector_id.TRAPPARTS
        }
    },
    [df.trap_type.WeaponTrap] = {
        {
            name='mechanism',
            item_type=df.item_type.TRAPPARTS,
            vector_id=df.job_item_vector_id.TRAPPARTS
        },
        {
            name='weapon',
            vector_id=df.job_item_vector_id.ANY_WEAPON
        }
    },
    [df.trap_type.Lever] = {
        {
            name='mechanism',
            item_type=df.item_type.TRAPPARTS,
            vector_id=df.job_item_vector_id.TRAPPARTS
        }
    },
    [df.trap_type.PressurePlate] = {
        {
            name='mechanism',
            item_type=df.item_type.TRAPPARTS,
            vector_id=df.job_item_vector_id.TRAPPARTS
        }
    },
    [df.trap_type.CageTrap] = {
        {
            name='mechanism',
            item_type=df.item_type.TRAPPARTS,
            vector_id=df.job_item_vector_id.TRAPPARTS
        }
    },
    [df.trap_type.TrackStop] = { { flags2={ building_material=true, non_economic=true } } }
}
local siegeengine_input = {
    [df.siegeengine_type.Catapult] = {
        {
            item_type=df.item_type.CATAPULTPARTS,
            vector_id=df.job_item_vector_id.CATAPULTPARTS,
            quantity=3
        }
    },
    [df.siegeengine_type.Ballista] = {
        {
            item_type=df.item_type.BALLISTAPARTS,
            vector_id=df.job_item_vector_id.BALLISTAPARTS,
            quantity=3
        }
    },
}
--[[ Functions for lookup in tables. ]]

local function get_custom_inputs(custom)
    local defn = df.building_def.find(custom)
    if defn ~= nil then
        return utils.clone_with_default(defn.build_items, buildings.input_filter_defaults)
    end
end

local function get_inputs_by_type(type,subtype,custom)
    if type == df.building_type.Workshop then
        if subtype == df.workshop_type.Custom then
            return get_custom_inputs(custom)
        else
            return workshop_inputs[subtype]
        end
    elseif type == df.building_type.Furnace then
        if subtype == df.furnace_type.Custom then
            return get_custom_inputs(custom)
        else
            return furnace_inputs[subtype]
        end
    elseif type == df.building_type.Trap then
        return trap_inputs[subtype]
    elseif type == df.building_type.SiegeEngine then
        return siegeengine_input[subtype]
    else
        return building_inputs[type]
    end
end

local function augment_input(input, argtable)
    local rv = {}
    local arg = argtable[input.name or 'material']

    if arg then
        utils.assign(rv, arg)
    end

    utils.assign(rv, input)

    if rv.mat_index and safe_index(rv, 'flags2', 'non_economic') then
        rv.flags2.non_economic = false
    end

    rv.new = true
    rv.name = nil
    return rv
end

function buildings.getFiltersByType(argtable,type,subtype,custom)
    local inputs = get_inputs_by_type(type,subtype,custom)
    if not inputs then
        return nil
    end
    local rv = {}
    for i,v in ipairs(inputs) do
        rv[i] = augment_input(v, argtable)
    end
    return rv
end

--[[
    Wraps all steps necessary to create a building with
    a construct job into one function.

    dfhack.buildings.constructBuilding{
      -- Position:
        pos = { x = ..., y = ..., z = ... },
      -- OR
        x = ..., y = ..., z = ...,

      -- Type:
        type = df.building_type.FOO, subtype = ..., custom = ...,

      -- Field initialization:
        fields = { ... },

      -- Size and orientation:
        width = ..., height = ..., direction = ...,

      -- Abort if not all tiles in the rectangle are available:
        full_rectangle = true,

      -- Materials:
        items = { item, item ... },
      -- OR
        filters = { { ... }, { ... }... }
      -- OR
        abstract = true
      -- OR
        material = { filter_properties... }
        mechanism = { filter_properties... }
        barrel, bucket, chain, anvil, screw, pipe
    }

    Returns: the created building, or 'nil, error'
--]]

function buildings.constructBuilding(info)
    local btype = info.type
    local subtype = info.subtype or -1
    local custom = info.custom or -1
    local filters = info.filters

    if not (info.pos or info.x) then
        error('position is required')
    end
    if not (info.abstract or info.items or filters) then
        filters = buildings.getFiltersByType(info,btype,subtype,custom)
        if not filters then
            error('one of items, filters or abstract is required')
        end
    elseif filters then
        for _,v in ipairs(filters) do
            v.new = true
        end
    end
    if type(btype) ~= 'number' or not df.building_type[btype] then
        error('Invalid building type: '..tostring(btype))
    end

    local pos = info.pos or xyz2pos(info.x, info.y, info.z)

    local instance = buildings.allocInstance(pos, btype, subtype, custom)
    if not instance then
        error('Could not create building of type '..df.building_type[btype])
    end

    local to_delete = instance
    return dfhack.with_finalize(
        function()
            df.delete(to_delete)
        end,
        function()
            if info.fields then
                instance:assign(info.fields)
            end
            local ok,w,h,area,r_area = buildings.setSize(
                instance,info.width,info.height,info.direction
            )
            if not ok then
                return nil, "cannot place at this position"
            end
            if info.full_rectangle and area ~= r_area then
                return nil, "not all tiles can be used"
            end
            if info.abstract then
                ok = buildings.constructAbstract(instance)
            elseif info.items then
                ok = buildings.constructWithItems(instance, info.items)
            else
                ok = buildings.constructWithFilters(instance, filters)
            end
            if not ok then
                return nil, "could not construct the building"
            end
            -- Success
            to_delete = nil
            return instance
        end
    )
end

return buildings