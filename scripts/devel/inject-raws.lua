-- Injects new reaction, item and building defs into the world.

-- The savegame contains a list of the relevant definition tokens in
-- the right order, but all details are read from raws every time.
-- This allows just adding stub definitions, and simply saving and
-- reloading the game.

-- Usage example:
--[[
devel/inject-raws trapcomp ITEM_TRAPCOMP_STEAM_PISTON workshop STEAM_ENGINE MAGMA_STEAM_ENGINE reaction STOKE_BOILER
]]

local utils = require 'utils'

local raws = df.global.world.raws

print[[
WARNING: THIS SCRIPT CAN PERMANENLY DAMAGE YOUR SAVE.

This script attempts to inject new raw objects into your
world. If the injected references do not match the actual
edited raws, your save will refuse to load, or load but crash.
]]

if not utils.prompt_yes_no('Did you make a backup?') then
    qerror('Not backed up.')
end

df.global.pause_state = true

local changed = false

function inject_reaction(name)
    for _,v in ipairs(raws.reactions) do
        if v.code == name then
            print('Reaction '..name..' already exists.')
            return
        end
    end

    print('Injecting reaction '..name)
    changed = true

    raws.reactions:insert('#', {
        new = true,
        code = name,
        name = 'Dummy reaction '..name,
        index = #raws.reactions,
    })
end

local building_types = {
    workshop = { df.building_def_workshopst, raws.buildings.workshops },
    furnace = { df.building_def_furnacest, raws.buildings.furnaces },
}

function inject_building(btype, name)
    for _,v in ipairs(raws.buildings.all) do
        if v.code == name then
            print('Building '..name..' already exists.')
            return
        end
    end

    print('Injecting building '..name)
    changed = true

    local typeinfo = building_types[btype]

    local id = raws.buildings.next_id
    raws.buildings.next_id = id+1

    raws.buildings.all:insert('#', {
        new = typeinfo[1],
        code = name,
        name = 'Dummy '..btype..' '..name,
        id = id,
    })

    typeinfo[2]:insert('#', raws.buildings.all[#raws.buildings.all-1])
end

local itemdefs = raws.itemdefs
local item_types = {
    weapon = { df.itemdef_weaponst, itemdefs.weapons, 'weapon_type' },
    trainweapon = { df.itemdef_weaponst, itemdefs.weapons, 'training_weapon_type' },
    pick = { df.itemdef_weaponst, itemdefs.weapons, 'digger_type' },
    trapcomp = { df.itemdef_trapcompst, itemdefs.trapcomps, 'trapcomp_type' },
    toy = { df.itemdef_toyst, itemdefs.toys, 'toy_type' },
    tool = { df.itemdef_toolst, itemdefs.tools, 'tool_type' },
    instrument = { df.itemdef_instrumentst, itemdefs.instruments, 'instrument_type' },
    armor = { df.itemdef_armorst, itemdefs.armor, 'armor_type' },
    ammo = { df.itemdef_ammost, itemdefs.ammo, 'ammo_type' },
    siegeammo = { df.itemdef_siegeammost, itemdefs.siege_ammo, 'siegeammo_type' },
    gloves = { df.itemdef_glovest, itemdefs.gloves, 'gloves_type' },
    shoes = { df.itemdef_shoest, itemdefs.shoes, 'shoes_type' },
    shield = { df.itemdef_shieldst, itemdefs.shields, 'shield_type' },
    helm = { df.itemdef_helmst, itemdefs.helms, 'helm_type' },
    pants = { df.itemdef_pantsst, itemdefs.pants, 'pants_type' },
    food = { df.itemdef_foodst, itemdefs.food },
}

function add_to_civ(entity, bvec, id)
    for _,v in ipairs(entity.resources[bvec]) do
        if v == id then
            return
        end
    end

    entity.resources[bvec]:insert('#', id)
end

function add_to_dwarf_civs(btype, id)
    local typeinfo = item_types[btype]
    if not typeinfo[3] then
        print('Not adding to civs.')
    end

    for _,entity in ipairs(df.global.world.entities.all) do
        if entity.race == df.global.ui.race_id then
            add_to_civ(entity, typeinfo[3], id)
        end
    end
end

function inject_item(btype, name)
    for _,v in ipairs(itemdefs.all) do
        if v.id == name then
            print('Itemdef '..name..' already exists.')
            return
        end
    end

    print('Injecting item '..name)
    changed = true

    local typeinfo = item_types[btype]
    local vec = typeinfo[2]
    local id = #vec

    vec:insert('#', {
        new = typeinfo[1],
        id = name,
        subtype = id,
        name = name,
        name_plural = name,
    })

    itemdefs.all:insert('#', vec[id])

    add_to_dwarf_civs(btype, id)
end

local args = {...}
local mode = nil
local ops = {}

for _,kv in ipairs(args) do
    if mode and string.match(kv, '^[%u_]+$') then
        table.insert(ops, curry(mode, kv))
    elseif kv == 'reaction' then
        mode = inject_reaction
    elseif building_types[kv] then
        mode = curry(inject_building, kv)
    elseif item_types[kv] then
        mode = curry(inject_item, kv)
    else
        qerror('Invalid option: '..kv)
    end
end

if #ops > 0 then
    print('')
    for _,v in ipairs(ops) do
        v()
    end
end

if changed then
    print('\nNow without unpausing save and reload the game to re-read raws.')
else
    print('\nNo changes made.')
end
