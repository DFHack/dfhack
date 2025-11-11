local _ENV = mkmodule('plugins.buildingplan.copybuilding')

local gui = require('gui')
local overlay = require('plugins.overlay')
local widgets = require('gui.widgets')

local DESIGNATE_BUILDING_HOTKEY = 'D_BUILDING'

-- WORKSHOPS
-- category hotkey
local HOTKEY_BUILDING_WORKSHOP = 'HOTKEY_BUILDING_WORKSHOP'

-- CLOTHING AND LEATHER
-- subcategory hotkeys
local HOTKEY_BUILDING_WORKSHOPS_CLOTHING_LEATHER = 'HOTKEY_BUILDING_WORKSHOPS_CLOTHING_LEATHER'
-- building specific hotkeys
local HOTKEY_BUILDING_WORKSHOP_LEATHER = 'HOTKEY_BUILDING_WORKSHOP_LEATHER'
local HOTKEY_BUILDING_WORKSHOP_LOOM = 'HOTKEY_BUILDING_WORKSHOP_LOOM'
local HOTKEY_BUILDING_WORKSHOP_CLOTHES = 'HOTKEY_BUILDING_WORKSHOP_CLOTHES'
local HOTKEY_BUILDING_WORKSHOP_DYER = 'HOTKEY_BUILDING_WORKSHOP_DYER'

-- FARMING
-- subcategory hotkeys
local HOTKEY_BUILDING_FARMING = 'HOTKEY_BUILDING_FARMING'
-- building specific hotkeys
local HOTKEY_BUILDING_FARMPLOT = 'HOTKEY_BUILDING_FARMPLOT'
local HOTKEY_BUILDING_WORKSHOP_STILL = 'HOTKEY_BUILDING_WORKSHOP_STILL'
local HOTKEY_BUILDING_WORKSHOP_BUTCHER = 'HOTKEY_BUILDING_WORKSHOP_BUTCHER'
local HOTKEY_BUILDING_WORKSHOP_TANNER = 'HOTKEY_BUILDING_WORKSHOP_TANNER'
local HOTKEY_BUILDING_WORKSHOP_FISHERY = 'HOTKEY_BUILDING_WORKSHOP_FISHERY'
local HOTKEY_BUILDING_WORKSHOP_KITCHEN = 'HOTKEY_BUILDING_WORKSHOP_KITCHEN'
local HOTKEY_BUILDING_WORKSHOP_FARMER = 'HOTKEY_BUILDING_WORKSHOP_FARMER'
local HOTKEY_BUILDING_WORKSHOP_QUERN = 'HOTKEY_BUILDING_WORKSHOP_QUERN'
local HOTKEY_BUILDING_KENNEL = 'HOTKEY_BUILDING_KENNEL' -- vermin catcher's shop
local HOTKEY_BUILDING_NEST_BOX = 'HOTKEY_BUILDING_NEST_BOX'
local HOTKEY_BUILDING_HIVE = 'HOTKEY_BUILDING_HIVE'

-- FURNACES
-- category hotkey
local HOTKEY_BUILDING_FURNACE = 'HOTKEY_BUILDING_FURNACE'
-- building specific hotkeys
local HOTKEY_BUILDING_FURNACE_GLASS = 'HOTKEY_BUILDING_FURNACE_GLASS'
local HOTKEY_BUILDING_FURNACE_KILN = 'HOTKEY_BUILDING_FURNACE_KILN'
local HOTKEY_BUILDING_FURNACE_GLASS_LAVA = 'HOTKEY_BUILDING_FURNACE_GLASS_LAVA'
local HOTKEY_BUILDING_FURNACE_KILN_LAVA = 'HOTKEY_BUILDING_FURNACE_KILN_LAVA'
local HOTKEY_BUILDING_FURNACE_SMELTER_LAVA = 'HOTKEY_BUILDING_FURNACE_SMELTER_LAVA'
local HOTKEY_BUILDING_FURNACE_SMELTER = 'HOTKEY_BUILDING_FURNACE_SMELTER'
local HOTKEY_BUILDING_FURNACE_WOOD = 'HOTKEY_BUILDING_FURNACE_WOOD'

-- UNGROUPED WORKSHOPS
local HOTKEY_BUILDING_WORKSHOP_ASHERY = 'HOTKEY_BUILDING_WORKSHOP_ASHERY'
local HOTKEY_BUILDING_WORKSHOP_BOWYER = 'HOTKEY_BUILDING_WORKSHOP_BOWYER'
local HOTKEY_BUILDING_WORKSHOP_CARPENTER = 'HOTKEY_BUILDING_WORKSHOP_CARPENTER'
local HOTKEY_BUILDING_WORKSHOP_CRAFTSMAN = 'HOTKEY_BUILDING_WORKSHOP_CRAFTSMAN'
local HOTKEY_BUILDING_WORKSHOP_JEWELER = 'HOTKEY_BUILDING_WORKSHOP_JEWELER'
local HOTKEY_BUILDING_WORKSHOP_LAVAMILL = 'HOTKEY_BUILDING_WORKSHOP_LAVAMILL'
local HOTKEY_BUILDING_WORKSHOP_MECHANIC = 'HOTKEY_BUILDING_WORKSHOP_MECHANIC'
local HOTKEY_BUILDING_WORKSHOP_METALSMITH = 'HOTKEY_BUILDING_WORKSHOP_METALSMITH'
-- Screw press doesn't seem to have a dedicated hotkey
local HOTKEY_BUILDING_WORKSHOP_SCREW_PRESS = 'CUSTOM_SHIFT_R'
local HOTKEY_BUILDING_WORKSHOP_SIEGE = 'HOTKEY_BUILDING_WORKSHOP_SIEGE'
-- Soap maker's workshop doesn't seem to have a dedicated hotkey
local HOTKEY_BUILDING_WORKSHOP_SOAPMAKER = 'CUSTOM_SHIFT_P'
local HOTKEY_BUILDING_WORKSHOP_MASON = 'HOTKEY_BUILDING_WORKSHOP_MASON'

-- FURNITURE
-- category hotkey
local HOTKEY_BUILDING_FURNITURE = 'HOTKEY_BUILDING_FURNITURE'
-- building specific hotkeys
local HOTKEY_BUILDING_BED = 'HOTKEY_BUILDING_BED'
local HOTKEY_BUILDING_CHAIR = 'HOTKEY_BUILDING_CHAIR'
local HOTKEY_BUILDING_TABLE = 'HOTKEY_BUILDING_TABLE'
local HOTKEY_BUILDING_BOX = 'HOTKEY_BUILDING_BOX'
local HOTKEY_BUILDING_CABINET = 'HOTKEY_BUILDING_CABINET'
local HOTKEY_BUILDING_COFFIN = 'HOTKEY_BUILDING_COFFIN'
local HOTKEY_BUILDING_SLAB = 'HOTKEY_BUILDING_SLAB'
local HOTKEY_BUILDING_STATUE = 'HOTKEY_BUILDING_STATUE'
local HOTKEY_BUILDING_TRACTION_BENCH = 'HOTKEY_BUILDING_TRACTION_BENCH'
local HOTKEY_BUILDING_BOOKCASE = 'HOTKEY_BUILDING_BOOKCASE'
local HOTKEY_BUILDING_DISPLAY_FURNITURE = 'HOTKEY_BUILDING_DISPLAY_FURNITURE'
local HOTKEY_BUILDING_OFFERING_PLACE = 'HOTKEY_BUILDING_OFFERING_PLACE'
local HOTKEY_BUILDING_INSTRUMENT = 'HOTKEY_BUILDING_INSTRUMENT'

-- DOORS/HATCHES
-- category hotkey
local HOTKEY_BUILDING_PORTALS = 'HOTKEY_BUILDING_PORTALS'
-- building specific hotkeys
local HOTKEY_BUILDING_DOOR = 'HOTKEY_BUILDING_DOOR'
local HOTKEY_BUILDING_HATCH = 'HOTKEY_BUILDING_HATCH'

-- CUSTOM BUILDING TYPES
local SOAPMAKER_CUSTOM_TYPE = 0
local SCREW_PRESS_CUSTOM_TYPE = 1

-- CONSTRUCTIONS
-- category hotkey
local HOTKEY_BUILDING_CONSTRUCTION = 'HOTKEY_BUILDING_CONSTRUCTION'
-- building specific hotkeys
local HOTKEY_BUILDING_CONSTRUCTION_WALL = 'HOTKEY_BUILDING_CONSTRUCTION_WALL'
local HOTKEY_BUILDING_CONSTRUCTION_FLOOR = 'HOTKEY_BUILDING_CONSTRUCTION_FLOOR'
local HOTKEY_BUILDING_CONSTRUCTION_RAMP = 'HOTKEY_BUILDING_CONSTRUCTION_RAMP'
local HOTKEY_BUILDING_CONSTRUCTION_STAIR_UPDOWN = 'HOTKEY_BUILDING_CONSTRUCTION_STAIR_UPDOWN'
local HOTKEY_BUILDING_BRIDGE = 'HOTKEY_BUILDING_BRIDGE'
local HOTKEY_BUILDING_ROAD_PAVED = 'HOTKEY_BUILDING_ROAD_PAVED'
local HOTKEY_BUILDING_ROAD_DIRT = 'HOTKEY_BUILDING_ROAD_DIRT'
local HOTKEY_BUILDING_CONSTRUCTION_FORTIFICATION = 'HOTKEY_BUILDING_CONSTRUCTION_FORTIFICATION'
local HOTKEY_BUILDING_GRATE_WALL = 'HOTKEY_BUILDING_GRATE_WALL'
local HOTKEY_BUILDING_GRATE_FLOOR = 'HOTKEY_BUILDING_GRATE_FLOOR'
local HOTKEY_BUILDING_BARS_VERTICAL = 'HOTKEY_BUILDING_BARS_VERTICAL'
local HOTKEY_BUILDING_BARS_FLOOR = 'HOTKEY_BUILDING_BARS_FLOOR'
local HOTKEY_BUILDING_WINDOW_GLASS = 'HOTKEY_BUILDING_WINDOW_GLASS'
local HOTKEY_BUILDING_WINDOW_GEM = 'HOTKEY_BUILDING_WINDOW_GEM'
local HOTKEY_BUILDING_SUPPORT = 'HOTKEY_BUILDING_SUPPORT'
local HOTKEY_BUILDING_CONSTRUCTION_TRACK = 'HOTKEY_BUILDING_CONSTRUCTION_TRACK'
local HOTKEY_BUILDING_CONSTRUCTION_TRACK_STOP = 'HOTKEY_BUILDING_CONSTRUCTION_TRACK_STOP'

--MACHINES/FLUIDS
-- category hotkey
local HOTKEY_BUILDING_MACHINE = 'HOTKEY_BUILDING_MACHINE'
-- building specific hotkeys
local HOTKEY_BUILDING_TRAP_LEVER = 'HOTKEY_BUILDING_TRAP_LEVER'
local HOTKEY_BUILDING_WELL = 'HOTKEY_BUILDING_WELL'
local HOTKEY_BUILDING_FLOODGATE = 'HOTKEY_BUILDING_FLOODGATE'
local HOTKEY_BUILDING_MACHINE_SCREW_PUMP = 'HOTKEY_BUILDING_MACHINE_SCREW_PUMP'
local HOTKEY_BUILDING_MACHINE_WATER_WHEEL = 'HOTKEY_BUILDING_MACHINE_WATER_WHEEL'
local HOTKEY_BUILDING_MACHINE_WINDMILL = 'HOTKEY_BUILDING_MACHINE_WINDMILL'
local HOTKEY_BUILDING_MACHINE_GEAR_ASSEMBLY = 'HOTKEY_BUILDING_MACHINE_GEAR_ASSEMBLY'
local HOTKEY_BUILDING_MACHINE_AXLE_HORIZONTAL = 'HOTKEY_BUILDING_MACHINE_AXLE_HORIZONTAL'
local HOTKEY_BUILDING_MACHINE_AXLE_VERTICAL = 'HOTKEY_BUILDING_MACHINE_AXLE_VERTICAL'
local HOTKEY_BUILDING_WORKSHOP_MILLSTONE = 'HOTKEY_BUILDING_WORKSHOP_MILLSTONE'
local HOTKEY_BUILDING_MACHINE_ROLLERS = 'HOTKEY_BUILDING_MACHINE_ROLLERS'

--CAGES/RESTRAINTS
-- category hotkey
local HOTKEY_BUILDING_CAGES_CHAINS = 'HOTKEY_BUILDING_CAGES_CHAINS'
-- building specific hotkeys
local HOTKEY_BUILDING_CHAIN = 'HOTKEY_BUILDING_CHAIN'
local HOTKEY_BUILDING_CAGE = 'HOTKEY_BUILDING_CAGE'
local HOTKEY_BUILDING_ANIMALTRAP = 'HOTKEY_BUILDING_ANIMALTRAP'

--TRAPS
-- category hotkey
local HOTKEY_BUILDING_TRAP = 'HOTKEY_BUILDING_TRAP'
-- building specific hotkeys
local HOTKEY_BUILDING_TRAP_TRIGGER = 'HOTKEY_BUILDING_TRAP_TRIGGER'
local HOTKEY_BUILDING_TRAP_STONE = 'HOTKEY_BUILDING_TRAP_STONE'
local HOTKEY_BUILDING_TRAP_WEAPON = 'HOTKEY_BUILDING_TRAP_WEAPON'
local HOTKEY_BUILDING_TRAP_CAGE = 'HOTKEY_BUILDING_TRAP_CAGE'
local HOTKEY_BUILDING_TRAP_SPIKE = 'HOTKEY_BUILDING_TRAP_SPIKE'

local LEVER_TRAP_TYPE = df.trap_type.Lever
local PRESSURE_PLATE_TRAP_TYPE = df.trap_type.PressurePlate
local CAGE_TRAP_TRAP_TYPE = df.trap_type.CageTrap
local STONE_FALL_TRAP_TYPE = df.trap_type.StoneFallTrap
local WEAPON_TRAP_TRAP_TYPE = df.trap_type.WeaponTrap
local TRACKSTOP_TRAP_TYPE = df.trap_type.TrackStop

--MILITARY
-- category hotkey
local HOTKEY_BUILDING_MILITARY = 'HOTKEY_BUILDING_MILITARY'
-- building specific hotkeys
local HOTKEY_BUILDING_ARCHERYTARGET = 'HOTKEY_BUILDING_ARCHERYTARGET'
local HOTKEY_BUILDING_WEAPONRACK = 'HOTKEY_BUILDING_WEAPONRACK'
local HOTKEY_BUILDING_ARMORSTAND = 'HOTKEY_BUILDING_ARMORSTAND'
local HOTKEY_BUILDING_SIEGEENGINE_BALLISTA = 'HOTKEY_BUILDING_SIEGEENGINE_BALLISTA'
local HOTKEY_BUILDING_SIEGEENGINE_CATAPULT = 'HOTKEY_BUILDING_SIEGEENGINE_CATAPULT'

-- TRADE DEPOT
-- building specific hotkey
local HOTKEY_BUILDING_TRADEDEPOT = 'HOTKEY_BUILDING_TRADEDEPOT'

-- Building type to hotkey sequence mapping
local BUILDING_HOTKEYS = {
    -- Furniture
    [df.building_type.Bed] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_FURNITURE,
        HOTKEY_BUILDING_BED
    },
    [df.building_type.Chair] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_FURNITURE,
        HOTKEY_BUILDING_CHAIR
    },
    [df.building_type.Table] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_FURNITURE,
        HOTKEY_BUILDING_TABLE
    },
    [df.building_type.Box] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_FURNITURE,
        HOTKEY_BUILDING_BOX
    },
    [df.building_type.Cabinet] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_FURNITURE,
        HOTKEY_BUILDING_CABINET
    },
    [df.building_type.Coffin] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_FURNITURE,
        HOTKEY_BUILDING_COFFIN
    },
    [df.building_type.Slab] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_FURNITURE,
        HOTKEY_BUILDING_SLAB
    },
    [df.building_type.Statue] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_FURNITURE,
        HOTKEY_BUILDING_STATUE
    },
    [df.building_type.TractionBench] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_FURNITURE,
        HOTKEY_BUILDING_TRACTION_BENCH
    },
    [df.building_type.Bookcase] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_FURNITURE,
        HOTKEY_BUILDING_BOOKCASE
    },
    [df.building_type.DisplayFurniture] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_FURNITURE,
        HOTKEY_BUILDING_DISPLAY_FURNITURE
    },
    [df.building_type.OfferingPlace] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_FURNITURE,
        HOTKEY_BUILDING_OFFERING_PLACE
    },
    [df.building_type.Instrument] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_FURNITURE,
        HOTKEY_BUILDING_INSTRUMENT
    },

    -- Farming/Food Production
    [df.building_type.FarmPlot] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_FARMING,
        HOTKEY_BUILDING_FARMPLOT
    },
    [df.building_type.NestBox] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_FARMING,
        HOTKEY_BUILDING_NEST_BOX
    },
    [df.building_type.Hive] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_FARMING,
        HOTKEY_BUILDING_HIVE
    },
    [df.building_type.Door] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_PORTALS,
        HOTKEY_BUILDING_DOOR
    },
    [df.building_type.Hatch] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_PORTALS,
        HOTKEY_BUILDING_HATCH
    },
    [df.building_type.Bridge] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_CONSTRUCTION,
        HOTKEY_BUILDING_BRIDGE
    },
    [df.building_type.RoadPaved] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_CONSTRUCTION,
        HOTKEY_BUILDING_ROAD_PAVED
    },
    [df.building_type.RoadDirt] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_CONSTRUCTION,
        HOTKEY_BUILDING_ROAD_DIRT
    },
    [df.building_type.GrateWall] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_CONSTRUCTION,
        HOTKEY_BUILDING_GRATE_WALL
    },
    [df.building_type.GrateFloor] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_CONSTRUCTION,
        HOTKEY_BUILDING_GRATE_FLOOR
    },
    [df.building_type.BarsVertical] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_CONSTRUCTION,
        HOTKEY_BUILDING_BARS_VERTICAL
    },
    [df.building_type.BarsFloor] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_CONSTRUCTION,
        HOTKEY_BUILDING_BARS_FLOOR
    },
    [df.building_type.WindowGlass] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_CONSTRUCTION,
        HOTKEY_BUILDING_WINDOW_GLASS
    },
    [df.building_type.WindowGem] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_CONSTRUCTION,
        HOTKEY_BUILDING_WINDOW_GEM
    },
    [df.building_type.Support] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_CONSTRUCTION,
        HOTKEY_BUILDING_SUPPORT
    },
    [df.building_type.Trap .. ":" .. tostring(TRACKSTOP_TRAP_TYPE)] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_CONSTRUCTION,
        HOTKEY_BUILDING_CONSTRUCTION_TRACK_STOP
    },
    [df.building_type.Trap .. ":" .. tostring(LEVER_TRAP_TYPE)] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_MACHINE,
        HOTKEY_BUILDING_TRAP_LEVER
    },
    [df.building_type.Well] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_MACHINE,
        HOTKEY_BUILDING_WELL
    },
    [df.building_type.Floodgate] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_MACHINE,
        HOTKEY_BUILDING_FLOODGATE
    },
    [df.building_type.ScrewPump] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_MACHINE,
        HOTKEY_BUILDING_MACHINE_SCREW_PUMP
    },
    [df.building_type.WaterWheel] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_MACHINE,
        HOTKEY_BUILDING_MACHINE_WATER_WHEEL
    },
    [df.building_type.Windmill] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_MACHINE,
        HOTKEY_BUILDING_MACHINE_WINDMILL
    },
    [df.building_type.GearAssembly] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_MACHINE,
        HOTKEY_BUILDING_MACHINE_GEAR_ASSEMBLY
    },
    [df.building_type.AxleHorizontal] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_MACHINE,
        HOTKEY_BUILDING_MACHINE_AXLE_HORIZONTAL
    },
    [df.building_type.AxleVertical] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_MACHINE,
        HOTKEY_BUILDING_MACHINE_AXLE_VERTICAL
    },
    [df.building_type.Rollers] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_MACHINE,
        HOTKEY_BUILDING_MACHINE_ROLLERS
    },
    [df.building_type.Chain] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_CAGES_CHAINS,
        HOTKEY_BUILDING_CHAIN
    },
    [df.building_type.Cage] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_CAGES_CHAINS,
        HOTKEY_BUILDING_CAGE
    },
    [df.building_type.AnimalTrap] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_CAGES_CHAINS,
        HOTKEY_BUILDING_ANIMALTRAP
    },
    [df.building_type.Trap .. ":" .. tostring(PRESSURE_PLATE_TRAP_TYPE)] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_TRAP,
        HOTKEY_BUILDING_TRAP_TRIGGER
    },
    [df.building_type.Trap .. ":" .. tostring(STONE_FALL_TRAP_TYPE)] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_TRAP,
        HOTKEY_BUILDING_TRAP_STONE
    },
    [df.building_type.Trap .. ":" .. tostring(WEAPON_TRAP_TRAP_TYPE)] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_TRAP,
        HOTKEY_BUILDING_TRAP_WEAPON
    },
    [df.building_type.Trap .. ":" .. tostring(CAGE_TRAP_TRAP_TYPE)] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_TRAP,
        HOTKEY_BUILDING_TRAP_CAGE
    },
    [df.building_type.Weapon] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_TRAP,
        HOTKEY_BUILDING_TRAP_SPIKE
    },
    [df.building_type.ArcheryTarget] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_MILITARY,
        HOTKEY_BUILDING_ARCHERYTARGET
    },
    [df.building_type.Weaponrack] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_MILITARY,
        HOTKEY_BUILDING_WEAPONRACK
    },
    [df.building_type.Armorstand] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_MILITARY,
        HOTKEY_BUILDING_ARMORSTAND
    },
    [df.building_type.TradeDepot] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_TRADEDEPOT
    }
}

-- Workshop type to hotkey sequence mapping (separate table to avoid collisions)
local WORKSHOP_HOTKEYS = {
    -- Clothing and Leather Workshops
    [df.workshop_type.Leatherworks] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_WORKSHOPS_CLOTHING_LEATHER,
        HOTKEY_BUILDING_WORKSHOP_LEATHER
    },
    [df.workshop_type.Loom] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_WORKSHOPS_CLOTHING_LEATHER,
        HOTKEY_BUILDING_WORKSHOP_LOOM
    },
    [df.workshop_type.Clothiers] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_WORKSHOPS_CLOTHING_LEATHER,
        HOTKEY_BUILDING_WORKSHOP_CLOTHES
    },
    [df.workshop_type.Dyers] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_WORKSHOPS_CLOTHING_LEATHER,
        HOTKEY_BUILDING_WORKSHOP_DYER
    },

    -- Farming Workshop Subtypes
    [df.workshop_type.Still] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_FARMING,
        HOTKEY_BUILDING_WORKSHOP_STILL
    },
    [df.workshop_type.Butchers] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_FARMING,
        HOTKEY_BUILDING_WORKSHOP_BUTCHER
    },
    [df.workshop_type.Tanners] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_FARMING,
        HOTKEY_BUILDING_WORKSHOP_TANNER
    },
    [df.workshop_type.Fishery] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_FARMING,
        HOTKEY_BUILDING_WORKSHOP_FISHERY
    },
    [df.workshop_type.Kitchen] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_FARMING,
        HOTKEY_BUILDING_WORKSHOP_KITCHEN
    },
    [df.workshop_type.Farmers] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_FARMING,
        HOTKEY_BUILDING_WORKSHOP_FARMER
    },
    [df.workshop_type.Quern] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_FARMING,
        HOTKEY_BUILDING_WORKSHOP_QUERN
    },
    [df.workshop_type.Kennels] = { -- kennel
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_FARMING,
        HOTKEY_BUILDING_KENNEL
    },
    [df.workshop_type.Custom .. ":" .. tostring(SOAPMAKER_CUSTOM_TYPE)] = { -- Soap maker's workshop
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_WORKSHOP_SOAPMAKER,
    },
    [df.workshop_type.Custom .. ":" .. tostring(SCREW_PRESS_CUSTOM_TYPE)] = { -- Screw press
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_WORKSHOP_SCREW_PRESS,
    },
    [df.workshop_type.Millstone] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_MACHINE,
        HOTKEY_BUILDING_WORKSHOP_MILLSTONE
    },
    [df.workshop_type.Siege] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_WORKSHOP_SIEGE
    },
    [df.workshop_type.Masons] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_WORKSHOP_MASON
    },
    [df.workshop_type.Ashery] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_WORKSHOP_ASHERY
    },
    [df.workshop_type.Bowyers] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_WORKSHOP_BOWYER
    },
    [df.workshop_type.Carpenters] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_WORKSHOP_CARPENTER
    },
    [df.workshop_type.Craftsdwarfs] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_WORKSHOP_CRAFTSMAN
    },
    [df.workshop_type.Jewelers] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_WORKSHOP_JEWELER
    },
    [df.workshop_type.MagmaForge] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_WORKSHOP_LAVAMILL
    },
    [df.workshop_type.Mechanics] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_WORKSHOP_MECHANIC
    },
    [df.workshop_type.MetalsmithsForge] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_WORKSHOP_METALSMITH
    },
}

local FURNACE_HOTKEYS = {
    -- all furnaces tested working
    [df.furnace_type.WoodFurnace] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_FURNACE,
        HOTKEY_BUILDING_FURNACE_WOOD
    },
    [df.furnace_type.Smelter] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_FURNACE,
        HOTKEY_BUILDING_FURNACE_SMELTER
    },
    [df.furnace_type.GlassFurnace] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_FURNACE,
        HOTKEY_BUILDING_FURNACE_GLASS
    },
    [df.furnace_type.Kiln] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_FURNACE,
        HOTKEY_BUILDING_FURNACE_KILN
    },
    [df.furnace_type.MagmaSmelter] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_FURNACE,
        HOTKEY_BUILDING_FURNACE_SMELTER_LAVA
    },
    [df.furnace_type.MagmaGlassFurnace] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_FURNACE,
        HOTKEY_BUILDING_FURNACE_GLASS_LAVA
    },
    [df.furnace_type.MagmaKiln] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_WORKSHOP,
        HOTKEY_BUILDING_FURNACE,
        HOTKEY_BUILDING_FURNACE_KILN_LAVA
    },
}

local function create_construction_hotkeys()
    local hotkeys = {
        -- Basic constructions
        [df.construction_type.Wall] = {
            DESIGNATE_BUILDING_HOTKEY,
            HOTKEY_BUILDING_CONSTRUCTION,
            HOTKEY_BUILDING_CONSTRUCTION_WALL
        },
        [df.construction_type.Floor] = {
            DESIGNATE_BUILDING_HOTKEY,
            HOTKEY_BUILDING_CONSTRUCTION,
            HOTKEY_BUILDING_CONSTRUCTION_FLOOR
        },
        [df.construction_type.Ramp] = {
            DESIGNATE_BUILDING_HOTKEY,
            HOTKEY_BUILDING_CONSTRUCTION,
            HOTKEY_BUILDING_CONSTRUCTION_RAMP
        },
        [df.construction_type.UpStair] = {
            DESIGNATE_BUILDING_HOTKEY,
            HOTKEY_BUILDING_CONSTRUCTION,
            HOTKEY_BUILDING_CONSTRUCTION_STAIR_UPDOWN
        },
        [df.construction_type.DownStair] = {
            DESIGNATE_BUILDING_HOTKEY,
            HOTKEY_BUILDING_CONSTRUCTION,
            HOTKEY_BUILDING_CONSTRUCTION_STAIR_UPDOWN
        },
        [df.construction_type.UpDownStair] = {
            DESIGNATE_BUILDING_HOTKEY,
            HOTKEY_BUILDING_CONSTRUCTION,
            HOTKEY_BUILDING_CONSTRUCTION_STAIR_UPDOWN
        },
        [df.construction_type.Fortification] = {
            DESIGNATE_BUILDING_HOTKEY,
            HOTKEY_BUILDING_CONSTRUCTION,
            HOTKEY_BUILDING_CONSTRUCTION_FORTIFICATION
        },
    }

    local track_directions = {
        'N', 'W', 'E', 'S', 'NS', 'NE', 'NW', 'SE', 'SW', 'EW', 'NSE', 'NSW', 'NEW', 'SEW', 'NSEW'
    }

    for _, dir in ipairs(track_directions) do
        local track_type = df.construction_type['Track' .. dir]
        if track_type then
            hotkeys[track_type] = {
                DESIGNATE_BUILDING_HOTKEY,
                HOTKEY_BUILDING_CONSTRUCTION,
                HOTKEY_BUILDING_CONSTRUCTION_TRACK
            }
        end
    end

    for _, dir in ipairs(track_directions) do
        local track_ramp_type = df.construction_type['TrackRamp' .. dir]
        if track_ramp_type then
            hotkeys[track_ramp_type] = {
                DESIGNATE_BUILDING_HOTKEY,
                HOTKEY_BUILDING_CONSTRUCTION,
                HOTKEY_BUILDING_CONSTRUCTION_TRACK
            }
        end
    end

    return hotkeys
end

-- dynamically create these to avoid tons of repetitive code due to track directions
local CONSTRUCTION_HOTKEYS = create_construction_hotkeys()

local copy_history = {}
MAX_HISTORY = 9

local function add_to_copy_history(hotkey_sequence)
    for i, sequence in ipairs(copy_history) do
        if sequence == hotkey_sequence then
            table.remove(copy_history, i)
            break
        end
    end

    table.insert(copy_history, 1, hotkey_sequence)

    while #copy_history > MAX_HISTORY do
        table.remove(copy_history)
    end
end

local SIEGEENGINE_HOTKEYS = {
    [df.siegeengine_type.Ballista] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_MILITARY,
        HOTKEY_BUILDING_SIEGEENGINE_BALLISTA
    },
    [df.siegeengine_type.Catapult] = {
        DESIGNATE_BUILDING_HOTKEY,
        HOTKEY_BUILDING_MILITARY,
        HOTKEY_BUILDING_SIEGEENGINE_CATAPULT
    },
}

local function simulate_input_to_df_viewscreen(hotkey_sequence)
    local viewscreen = dfhack.gui.getDFViewscreen()

    if not viewscreen then
        return
    end

    gui.simulateInput(viewscreen, '_MOUSE_R')

    if type(hotkey_sequence) == 'table' and hotkey_sequence.on_before_sequence then
        hotkey_sequence.on_before_sequence()
        hotkey_sequence = hotkey_sequence.sequence
    end

    for _, hotkey in ipairs(hotkey_sequence) do
        gui.simulateInput(viewscreen, hotkey)
    end

    if type(hotkey_sequence) == 'table' and hotkey_sequence.on_after_sequence then
        hotkey_sequence.on_after_sequence()
    end
end

local function get_building_hotkey_sequence(building)
    if not building then
        return nil
    end

    local building_type = building:getType()
    local key = building_type

    if building_type == df.building_type.Workshop then
        local workshop_type = building:getSubtype()
        if workshop_type == df.workshop_type.Custom then
            key = workshop_type .. ":" .. tostring(building.custom_type)
        else
            key = workshop_type
        end
        return WORKSHOP_HOTKEYS[key]
    end

    if building_type == df.building_type.Furnace then
        local furnace_type = building:getSubtype()
        key = furnace_type
        return FURNACE_HOTKEYS[key]
    end

    if building_type == df.building_type.Construction then
        local construction_type = building:getSubtype()
        key = construction_type

        if construction_type == df.construction_type.ReinforcedWall then
            -- reinforced wall doesn't have a keyboard shortcut so we set it dynamically on the button
            -- use a random key that is unlikely to conflict
            return {sequence={'STRING_A207'}, on_before_sequence=function()
                simulate_input_to_df_viewscreen{DESIGNATE_BUILDING_HOTKEY, HOTKEY_BUILDING_CONSTRUCTION}
                df.global.game.main_interface.construction.page[1].bb_button[1].hotkey = df.interface_key['STRING_A207']
            end}
        end

        return CONSTRUCTION_HOTKEYS[key]
    end

    if building_type == df.building_type.SiegeEngine then
        local siegeengine_type = building:getSubtype()
        key = siegeengine_type

        if siegeengine_type == df.siegeengine_type.BoltThrower then
            -- Bolt Thrower doesn't have a keyboard shortcut so we set it dynamically
            -- use a random key that is unlikely to conflict
            return {sequence={'STRING_A207'}, on_before_sequence=function()
                simulate_input_to_df_viewscreen{DESIGNATE_BUILDING_HOTKEY, HOTKEY_BUILDING_MILITARY}
                df.global.game.main_interface.construction.page[1].bb_button[5].hotkey = df.interface_key['STRING_A207']
            end}
        end

        return SIEGEENGINE_HOTKEYS[key]
    end

    if building_type == df.building_type.Trap then
        local trap_type = building:getSubtype()
        key = building_type .. ":" .. tostring(trap_type)
        return BUILDING_HOTKEYS[key]
    end

    if building_type == df.building_type.Stockpile then
        return {sequence={}, on_before_sequence=function()
            df.global.game.main_interface.bottom_mode_selected = df.main_bottom_mode_type.STOCKPILE_PAINT
        end}
    end

    return BUILDING_HOTKEYS[key]
end

local function copy_civzone(civzone)
    if not civzone then
        return
    end

    local civzone_type = civzone.type
    local hotkey_sequence = {
        on_before_sequence=function()
            df.global.game.main_interface.civzone.cur_bld = nil
            df.global.game.main_interface.civzone.adding_new_type = civzone_type
            df.global.game.main_interface.bottom_mode_selected = df.main_bottom_mode_type.ZONE_PAINT
        end,
        sequence={},
    }

    add_to_copy_history(hotkey_sequence)
    simulate_input_to_df_viewscreen(hotkey_sequence)
end

local function copy_building(hotkey_sequence, building)
    local civzone = dfhack.gui.getSelectedCivZone(true)
    if civzone then
        copy_civzone(civzone)
        return
    end
    building = not hotkey_sequence and (dfhack.gui.getSelectedBuilding(true) or building) or nil
    if not building and not hotkey_sequence then
        return
    end

    hotkey_sequence = hotkey_sequence or get_building_hotkey_sequence(building)
    if not hotkey_sequence then
        return
    end

    add_to_copy_history(hotkey_sequence)
    simulate_input_to_df_viewscreen(hotkey_sequence)
end

function copy_building_at_cursor()
    local civzone = dfhack.gui.getSelectedCivZone(true)
    if civzone then
        copy_civzone(civzone)
        return
    end

    local building = dfhack.gui.getSelectedBuilding(true)
    if building then
        copy_building(nil, building)
        return
    end

    local cursor = dfhack.gui.getMousePos()
    if not cursor then
        return false, "No cursor position"
    end

    building = dfhack.buildings.findAtTile(cursor)

    if building then
        copy_building(nil, building)
        return
    end

    if not building then
        local tt = dfhack.maps.getTileType(cursor)
        if tt then
            local attrs = df.tiletype.attrs[tt]
            if attrs then
                local shape = attrs.shape
                local hotkey_sequence

                if shape == df.tiletype_shape.WALL then
                    hotkey_sequence = CONSTRUCTION_HOTKEYS[df.construction_type.Wall]
                    local construction = dfhack.constructions.findAtTile(cursor)

                    if construction and construction.flags.reinforced then
                        hotkey_sequence = {sequence={'STRING_A207'}, on_before_sequence=function()
                            simulate_input_to_df_viewscreen{DESIGNATE_BUILDING_HOTKEY, HOTKEY_BUILDING_CONSTRUCTION}
                            df.global.game.main_interface.construction.page[1].bb_button[1].hotkey = df.interface_key['STRING_A207']
                        end}
                    else
                        hotkey_sequence = CONSTRUCTION_HOTKEYS[df.construction_type.Wall]
                    end
                elseif shape == df.tiletype_shape.FLOOR then
                    hotkey_sequence = CONSTRUCTION_HOTKEYS[df.construction_type.Floor]
                elseif shape == df.tiletype_shape.RAMP then
                    hotkey_sequence = CONSTRUCTION_HOTKEYS[df.construction_type.Ramp]
                elseif shape == df.tiletype_shape.STAIR_UP then
                    hotkey_sequence = CONSTRUCTION_HOTKEYS[df.construction_type.UpStair]
                elseif shape == df.tiletype_shape.STAIR_DOWN then
                    hotkey_sequence = CONSTRUCTION_HOTKEYS[df.construction_type.DownStair]
                elseif shape == df.tiletype_shape.STAIR_UPDOWN then
                    hotkey_sequence = CONSTRUCTION_HOTKEYS[df.construction_type.UpDownStair]
                elseif shape == df.tiletype_shape.FORTIFICATION then
                    hotkey_sequence = CONSTRUCTION_HOTKEYS[df.construction_type.Fortification]
                end

                if hotkey_sequence then
                    copy_building(hotkey_sequence)
                    return
                end
            end
        end
    end

    return false, "Could not get anything to copy"
end

function copy_building_from_history(history_index)
    if history_index < 1 or history_index > #copy_history then
        return false, "No building at history position " .. history_index
    end

    local hotkey_sequence = copy_history[history_index]
    simulate_input_to_df_viewscreen(hotkey_sequence)
    return true, "Building from history position " .. history_index
end

--------------------------------
-- CopyBuildingOverlay
--

CopyBuildingOverlay = defclass(CopyBuildingOverlay, overlay.OverlayWidget)
CopyBuildingOverlay.ATTRS{
    desc='Adds a copy button to building inspection that starts building construction of the same type.',
    default_pos={x=-30,y=6},
    default_enabled=true,
    viewscreens="dwarfmode/ViewSheets/BUILDING",
    frame={w=14,h=3},
    frame_style=gui.FRAME_MEDIUM,
    frame_background=gui.CLEAR_PEN,
}

function CopyBuildingOverlay:init()
    self:addviews{
        widgets.HotkeyLabel{
            frame={t=0, l=0},
            label='Copy',
            key='CUSTOM_ALT_C',
            on_activate=copy_building,
        },
    }
end

function CopyBuildingOverlay:render(dc)
    local current_focus_strings = dfhack.gui.getCurFocus()
    if not current_focus_strings or #current_focus_strings == 0 then
        return
    end
    local current_focus = current_focus_strings[#current_focus_strings]
    if not current_focus:endswith('/Items') or #current_focus_strings > 2 then
        return
    end
    if current_focus_strings[1]:endswith('/WorkOrders') then
        return
    end
    CopyBuildingOverlay.super.render(self, dc)
end

return _ENV
