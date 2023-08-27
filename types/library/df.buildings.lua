-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@enum building_type
df.building_type = {
    NONE = -1,
    Chair = 0,
    Bed = 1,
    Table = 2,
    Coffin = 3,
    FarmPlot = 4,
    Furnace = 5,
    TradeDepot = 6,
    Shop = 7,
    Door = 8,
    Floodgate = 9,
    Box = 10,
    Weaponrack = 11,
    Armorstand = 12,
    Workshop = 13,
    Cabinet = 14,
    Statue = 15,
    WindowGlass = 16,
    WindowGem = 17,
    Well = 18,
    Bridge = 19,
    RoadDirt = 20,
    RoadPaved = 21,
    SiegeEngine = 22,
    Trap = 23,
    AnimalTrap = 24,
    Support = 25,
    ArcheryTarget = 26,
    Chain = 27,
    Cage = 28,
    Stockpile = 29,
    Civzone = 30,
    Weapon = 31,
    Wagon = 32,
    ScrewPump = 33,
    Construction = 34,
    Hatch = 35,
    GrateWall = 36,
    GrateFloor = 37,
    BarsVertical = 38,
    BarsFloor = 39,
    GearAssembly = 40,
    AxleHorizontal = 41,
    AxleVertical = 42,
    WaterWheel = 43,
    Windmill = 44,
    TractionBench = 45,
    Slab = 46,
    Nest = 47,
    NestBox = 48,
    Hive = 49,
    Rollers = 50,
    Instrument = 51,
    Bookcase = 52,
    DisplayFurniture = 53,
    OfferingPlace = 54,
}

---@alias building_flags bitfield

---@alias door_flags bitfield

---@alias gate_flags bitfield

---@enum building_extents_type
df.building_extents_type = {
    None = 0,
    Stockpile = 1,
    Wall = 2,
    Interior = 3,
    DistanceBoundary = 4,
}

---@class building_extents
---@field extents any[]
---@field x integer
---@field y integer
---@field width integer
---@field height integer
df.building_extents = nil

---@class building_drawbuffer
---@field texpos1 any[]
---@field texpos2 any[]
---@field texpos3 any[]
---@field tile any[]
---@field fore any[]
---@field back any[]
---@field bright any[]
---@field x1 integer
---@field x2 integer
---@field y1 integer
---@field y2 integer
df.building_drawbuffer = nil

---@return integer
function df.building.getCustomType() end

---@param type integer
function df.building.setCustomType(type) end

---@param supplies hospital_supplies
function df.building.countHospitalSupplies(supplies) end

---@return stockpile_links
function df.building.getStockpileLinks() end

function df.building.detachWorldData() end -- ?

---@return boolean
function df.building.canLinkToStockpile() end -- used by the give to building ui of stockpile

---@return building_users
function df.building.getUsers() end

---@param delta_x integer
---@param delta_y integer
---@param delta_z integer
function df.building.moveBuilding(delta_x, delta_y, delta_z) end

---@param abs_x integer
---@param abs_y integer
function df.building.initOccupancy(abs_x, abs_y) end

---@param arg_0 job_type
---@param arg_1 integer
function df.building.setFillTimer(arg_0, arg_1) end

---@return boolean
function df.building.isOnFire() end

---@return boolean
function df.building.isUnpowered() end -- from magma OR machine power

---@return boolean
function df.building.canCollapse() end

---@return integer
function df.building.getPassableOccupancy() end

---@return integer
function df.building.getImpassableOccupancy() end

---@return boolean
function df.building.isPowerSource() end

function df.building.updateFromWeather() end

function df.building.updateTemperature() end

function df.building.updateItems() end

---@param temp integer
---@param arg_1 boolean
---@param arg_2 boolean
function df.building.updateTempFromTile(temp, arg_1, arg_2) end

---@return boolean
function df.building.isNormalFurniture() end

---@return boolean
function df.building.isFarmPlot() end

---@return workshop_profile
function df.building.getWorkshopProfile() end

---@return machine_info
function df.building.getMachineInfo() end

---@param power_info power_info
function df.building.getPowerInfo(power_info) end

---@param arg_0 machine_tile_set
---@return boolean
function df.building.canConnectToMachine(arg_0) end

---@return building_type
function df.building.getType() end

---@return integer
function df.building.getSubtype() end

---@param subtype integer
function df.building.setSubtype(subtype) end

---@return boolean
function df.building.isActual() end

---@return boolean
function df.building.isCoffin2() end

function df.building.updateAction() end

---@return boolean
function df.building.isStatueOrRestraint() end

---@param arg_0 integer
function df.building.setMaterialAmount(arg_0) end

---@param stage integer
function df.building.setBuildStage(stage) end

---@return integer
function df.building.getBuildStage() end

---@return integer
function df.building.getMaxBuildStage() end

---@return integer
function df.building.getArchitectureValue() end -- bridge: material*10 rough, material*30 smooth

---@return boolean
function df.building.isSettingOccupancy() end

---@return boolean
function df.building.isActual2() end

---@return boolean
function df.building.isExtentShaped() end

---@param abs_x integer
---@param abs_y integer
function df.building.updateOccupancy(abs_x, abs_y) end

---@param arg_0 unit
---@return integer
function df.building.getPersonalValue(arg_0) end

---@return boolean
function df.building.canBeRoom() end

---@return integer
function df.building.getConstructionValue() end -- bridge: material*10

function df.building.queueDestroy() end -- same as querying and pressing X

---@param rel_x integer
---@param rel_y integer
---@return boolean
function df.building.isImpassableTile(rel_x, rel_y) end

---@param subtract_pending_jobs boolean
---@return integer
function df.building.getFreeCapacity(subtract_pending_jobs) end

---@param arg_0 item
---@param subtract_pending_jobs boolean
---@return boolean
function df.building.canStoreItem(arg_0, subtract_pending_jobs) end

---@param name string
function df.building.getName(name) end

function df.building.getNameColor() end

function df.building.initFarmSeasons() end

---@return integer
function df.building.getClutterLevel() end -- 1..10

---@return boolean
function df.building.needsDesign() end

---@param arg_0 job_type
---@return boolean
function df.building.canUseForMood(arg_0) end

---@return boolean
function df.building.canBeRoomSubset() end

---@return boolean
function df.building.isCoffin() end

---@return boolean
function df.building.canUseSpouseRoom() end

---@return boolean
function df.building.canMakeRoom() end -- false if already, or cannot be

---@return boolean
function df.building.isAssigned() end -- building is assigned to a unit (if a zone) or is in a zone that is assigned to a unit

---@return boolean
function df.building.isJusticeRestraint() end

---@param arg_0 unit
function df.building.detachRestrainedUnit(arg_0) end

---@param file file_compressorst
function df.building.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.building.read_file(file, loadversion) end

---@return boolean
function df.building.isImpassableAtCreation() end -- the true set looks like things where the unit should stand aside

---@param in_play boolean
function df.building.categorize(in_play) end -- Add to world.buildings.other.*

function df.building.uncategorize() end -- Remove from world.buildings.other.*

---@return integer
function df.building.getBaseValue() end

---@param new_state integer
function df.building.setTriggerState(new_state) end

---@return boolean
function df.building.needsMagma() end

---@param noscatter boolean
---@param lost boolean
function df.building.removeUses(noscatter, lost) end

---@param noscatter boolean
---@param lost boolean
function df.building.deconstructItems(noscatter, lost) end

function df.building.cleanupMap() end

---@param fire_type integer
---@return boolean
function df.building.isFireSafe(fire_type) end -- checks contained_items[0] for FIREIMMUNE

function df.building.fillSidebarMenu() end

---@return boolean
function df.building.isForbidden() end

---@return boolean
function df.building.unnamed_method() end

---@return boolean
function df.building.isHidden() end

---@return boolean
function df.building.isVisibleInUI() end -- not hidden, or hide/unhide UI mode

---@param viewport map_viewport
---@return boolean
function df.building.isVisibleInViewport(viewport) end -- checks coordinates, calls isVisibleInUI and checks window_xy

---@param buffer building_drawbuffer
function df.building.getDrawExtents(buffer) end

---@param unk_item integer
---@param buffer building_drawbuffer
---@param z_offset integer
function df.building.drawBuilding(unk_item, buffer, z_offset) end

function df.building.unnamed_method() end

---@return integer
function df.building.getSpecificSquad() end

---@return integer
function df.building.getSpecificPosition() end

---@param arg_0 integer
---@param arg_1 integer
function df.building.setSpecificSquadPos(arg_0, arg_1) end

function df.building.clearSpecificSquad() end

function df.building.unnamed_method() end -- related to tavern beds, since v0.42.01

---@class building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
df.building = nil

---@class stockpile_links
---@field give_to_pile building[]
---@field take_from_pile building[]
---@field give_to_workshop building[]
---@field take_from_workshop building[]
df.stockpile_links = nil

---@class building_stockpilest
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field settings stockpile_settings
---@field max_barrels integer
---@field max_bins integer
---@field max_wheelbarrows integer
---@field container_type any[]
---@field container_item_id integer[]
---@field container_x integer[]
---@field container_y integer[]
---@field links stockpile_links
---@field use_links_only integer
---@field stockpile_number integer
---@field linked_stops hauling_stop[]
df.building_stockpilest = nil

---@class hospital_supplies
---@field supplies_needed bitfield
---@field max_splints integer
---@field max_thread integer
---@field max_cloth integer
---@field max_crutches integer
---@field max_plaster integer
---@field max_buckets integer
---@field max_soap integer
---@field cur_splints integer
---@field cur_thread integer
---@field cur_cloth integer
---@field cur_crutches integer
---@field cur_plaster integer
---@field cur_buckets integer
---@field cur_soap integer
---@field supply_recheck_timer integer
df.hospital_supplies = nil

---@enum civzone_type
df.civzone_type = {
    Home = 0,
    Depot = 1,
    Stockpile = 2,
    NobleQuarters = 3,
    unk_4 = 4,
    unk_5 = 5,
    unk_6 = 6,
    MeadHall = 7,
    ThroneRoom = 8,
    unk_9 = 9,
    Temple = 10,
    Kitchen = 11,
    CaptiveRoom = 12,
    TowerTop = 13,
    Courtyard = 14,
    Treasury = 15,
    GuardPost = 16,
    Entrance = 17,
    SecretLibrary = 18,
    Library = 19,
    Plot = 20,
    MarketStall = 21,
    unk_22 = 22,
    Campground = 23,
    CommandTent = 24,
    Tent = 25,
    CommandTentBld = 26,
    TentBld = 27,
    MechanismRoom = 28,
    DungeonCell = 29,
    AnimalPit = 30,
    ClothPit = 31,
    TanningPit = 32,
    ClothClothingPit = 33,
    LeatherClothingPit = 34,
    BoneCarvingPit = 35,
    GemCuttingPit = 36,
    WeaponsmithingPit = 37,
    BowmakingPit = 38,
    BlacksmithingPit = 39,
    ArmorsmithingPit = 40,
    MetalCraftingPit = 41,
    LeatherworkingPit = 42,
    CarpentryPit = 43,
    StoneworkingPit = 44,
    ForgingPit = 45,
    FightingPit = 46,
    unk_47 = 47,
    unk_48 = 48,
    unk_49 = 49,
    unk_50 = 50,
    unk_51 = 51,
    unk_52 = 52,
    AnimalWorkshop = 53,
    ClothWorkshop = 54,
    TanningWorkshop = 55,
    ClothClothingWorkshop = 56,
    LeatherClothingWorkshop = 57,
    BoneCarvingWorkshop = 58,
    GemCuttingWorkshop = 59,
    WeaponsmithingWorkshop = 60,
    BowmakingWorkshop = 61,
    BlacksmithingWorkshop = 62,
    ArmorsmithingWorkshop = 63,
    MetalCraftingWorkshop = 64,
    LeatherworkingShop = 65,
    CarpentryWorkshop = 66,
    StoneworkingWorkshop = 67,
    ForgingWorkshop = 68,
    CountingHouseOffices = 69,
    CountingHouseStorage = 70,
    GuildhallOffices = 71,
    GuildhallStorage = 72,
    TowerEntrance = 73,
    TowerFeasthall = 74,
    TowerBedroom = 75,
    TowerTreasury = 76,
    TowerDungeon = 77,
    TowerAttic = 78,
    Dormitory = 79,
    DiningHall = 80,
    unk_81 = 81,
    WaterSource = 82,
    Dump = 83,
    SandCollection = 84,
    FishingArea = 85,
    Pond = 86,
    MeetingHall = 87,
    Pen = 88,
    ClayCollection = 89,
    AnimalTraining = 90,
    PlantGathering = 91,
    Bedroom = 92,
    Office = 93,
    ArcheryRange = 94,
    Barracks = 95,
    Dungeon = 96,
    Tomb = 97,
}

---@class building_civzonest
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field assigned_units integer[]
---@field assigned_items integer[]
---@field type civzone_type only saved as int16
---@field is_active integer 0 is paused, 8 is active
---@field unnamed_building_civzonest_5 integer
---@field unnamed_building_civzonest_6 integer
---@field zone_num integer
---@field zone_settings whole | gather | pen | tomb | archery | pit_pond
---@field unnamed_building_civzonest_9 integer
---@field unnamed_building_civzonest_10 integer[]
---@field contained_buildings building[] includes eg workshops and beds
---@field assigned_unit_id integer
---@field unnamed_building_civzonest_13 integer
---@field assigned_unit unit
---@field squad_room_info any[]
---@field unnamed_building_civzonest_16 integer
---@field unnamed_building_civzonest_17 integer
df.building_civzonest = nil

---@return boolean
function df.building_actual.isDestroyedByItemRemoval() end

---@return boolean
function df.building_actual.unnamed_method() end -- default false

---@return boolean
function df.building_actual.unnamed_method() end -- default false

---@class building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
df.building_actual = nil

---@class building_design
---@field builder1 integer
---@field unk5 integer
---@field build_skill integer
---@field build_timer1 integer +1 per 10 frames while building
---@field builder2 integer
---@field build_timer2 integer
---@field quality1 item_quality
---@field flags bitfield
---@field hitpoints integer
---@field max_hitpoints integer
df.building_design = nil

---@enum furnace_type
df.furnace_type = {
    WoodFurnace = 0,
    Smelter = 1,
    GlassFurnace = 2,
    Kiln = 3,
    MagmaSmelter = 4,
    MagmaGlassFurnace = 5,
    MagmaKiln = 6,
    Custom = 7,
}

---@class building_furnacest
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field melt_remainder integer[]
---@field unk_108 integer
---@field type furnace_type
---@field profile workshop_profile
---@field custom_type integer
df.building_furnacest = nil

---@enum workshop_type
df.workshop_type = {
    Carpenters = 0,
    Farmers = 1,
    Masons = 2,
    Craftsdwarfs = 3,
    Jewelers = 4,
    MetalsmithsForge = 5,
    MagmaForge = 6,
    Bowyers = 7,
    Mechanics = 8,
    Siege = 9,
    Butchers = 10,
    Leatherworks = 11,
    Tanners = 12,
    Clothiers = 13,
    Fishery = 14,
    Still = 15,
    Loom = 16,
    Quern = 17,
    Kennels = 18,
    Kitchen = 19,
    Ashery = 20,
    Dyers = 21,
    Millstone = 22,
    Custom = 23,
    Tool = 24,
}

---@class workshop_profile
---@field permitted_workers integer[]
---@field min_level integer
---@field max_level integer
---@field links stockpile_links
---@field max_general_orders integer
---@field block_general_orders boolean
---@field pad_1 integer
---@field blocked_labors boolean[]
df.workshop_profile = nil

---@class building_workshopst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field type workshop_type
---@field profile workshop_profile
---@field machine machine_info
---@field custom_type integer
df.building_workshopst = nil

---@class building_animaltrapst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field bait_type integer
---@field fill_timer integer
df.building_animaltrapst = nil

---@class building_archerytargetst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
df.building_archerytargetst = nil

---@class building_armorstandst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field unk_c0 integer
---@field specific_squad integer
---@field specific_position integer
df.building_armorstandst = nil

---@class building_bars_verticalst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field gate_flags bitfield
---@field timer integer
df.building_bars_verticalst = nil

---@class building_bars_floorst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field gate_flags bitfield
---@field timer integer
df.building_bars_floorst = nil

---@class building_users
---@field unit integer[]
---@field mode integer[]
df.building_users = nil

---@class building_bedst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field specific_squad integer
---@field specific_position integer
---@field users building_users
df.building_bedst = nil

---@class building_bookcasest
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
df.building_bookcasest = nil

---@class building_boxst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field unk_1 integer
---@field specific_squad integer
---@field specific_position integer
df.building_boxst = nil

---@enum building_bridgest_direction
df.building_bridgest_direction = {
    Retracting = -1,
    Left = 0,
    Right = 1,
    Up = 2,
    Down = 3,
}
---@class building_bridgest
-- inherit building_actual
---@field gate_flags bitfield
---@field timer integer
---@field building_bridgest_direction building_bridgest_direction
---@field material_amount integer
df.building_bridgest = nil

---@class building_cabinetst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field unk_1 integer
---@field specific_squad integer
---@field specific_position integer
df.building_cabinetst = nil

---@class building_cagest
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field assigned_units integer[]
---@field assigned_items integer[]
---@field cage_flags bitfield
---@field fill_timer integer
df.building_cagest = nil

---@class building_chainst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field assigned unit
---@field chained unit
---@field chain_flags bitfield
df.building_chainst = nil

---@class building_chairst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field unk_1 integer
---@field users building_users
df.building_chairst = nil

---@class building_coffinst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
df.building_coffinst = nil

---@enum construction_type
df.construction_type = {
    NONE = -1, -- unused
    Fortification = 0,
    Wall = 1,
    Floor = 2,
    UpStair = 3,
    DownStair = 4,
    UpDownStair = 5,
    Ramp = 6,
    TrackN = 7,
    TrackS = 8,
    TrackE = 9,
    TrackW = 10,
    TrackNS = 11,
    TrackNE = 12,
    TrackNW = 13,
    TrackSE = 14,
    TrackSW = 15,
    TrackEW = 16,
    TrackNSE = 17,
    TrackNSW = 18,
    TrackNEW = 19,
    TrackSEW = 20,
    TrackNSEW = 21,
    TrackRampN = 22,
    TrackRampS = 23,
    TrackRampE = 24,
    TrackRampW = 25,
    TrackRampNS = 26,
    TrackRampNE = 27,
    TrackRampNW = 28,
    TrackRampSE = 29,
    TrackRampSW = 30,
    TrackRampEW = 31,
    TrackRampNSE = 32,
    TrackRampNSW = 33,
    TrackRampNEW = 34,
    TrackRampSEW = 35,
    TrackRampNSEW = 36,
}

---@class building_constructionst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field type construction_type
df.building_constructionst = nil

---@class building_display_furniturest
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field displayed_items integer[]
df.building_display_furniturest = nil

---@class building_doorst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field door_flags bitfield
---@field close_timer integer
df.building_doorst = nil

---@class building_farmplotst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field plant_id any[]
---@field material_amount integer
---@field farm_flags bitfield
---@field last_season season
---@field current_fertilization integer
---@field max_fertilization integer
---@field terrain_purge_timer integer
df.building_farmplotst = nil

---@class building_floodgatest
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field gate_flags bitfield
---@field timer integer
df.building_floodgatest = nil

---@class building_grate_floorst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field gate_flags bitfield
---@field timer integer
df.building_grate_floorst = nil

---@class building_grate_wallst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field gate_flags bitfield
---@field timer integer
df.building_grate_wallst = nil

---@class building_hatchst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field door_flags bitfield
---@field close_timer integer
df.building_hatchst = nil

---@alias hive_flags bitfield

---@class building_hivest
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field hive_flags bitfield
---@field split_timer integer up to 100800
---@field activity_timer integer up to 100800000; checks timer%hive_product.time[i]==0
---@field install_timer integer down from 1200
---@field gather_timer integer down from 1200
df.building_hivest = nil

---@class building_instrumentst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field unk_1 integer
df.building_instrumentst = nil

---@class building_nestst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
df.building_nestst = nil

---@class building_nest_boxst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field claimed_by integer
---@field claim_timeout integer counts up if the nest box is claimed but empty. when it hits 8400 ticks, the nest box is unclaimed.
df.building_nest_boxst = nil

---@class building_offering_placest
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
df.building_offering_placest = nil

---@class building_roadst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
df.building_roadst = nil

---@class building_road_dirtst
-- inherited from building_roadst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
-- end building_roadst
---@field material_amount integer
df.building_road_dirtst = nil

---@class building_road_pavedst
-- inherited from building_roadst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
-- end building_roadst
---@field material_amount integer
---@field unnamed_building_road_pavedst_2 integer
---@field terrain_purge_timer integer
df.building_road_pavedst = nil

---@enum shop_type
df.shop_type = {
    GeneralStore = 0,
    CraftsMarket = 1,
    ClothingShop = 2,
    ExoticClothingShop = 3,
}

---@class building_shopst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field owner unit
---@field timer integer increments until reaching 200,000,000
---@field shop_flags bitfield
---@field type shop_type
df.building_shopst = nil

---@enum siegeengine_type
df.siegeengine_type = {
    Catapult = 0,
    Ballista = 1,
}

---@enum building_siegeenginest_action
df.building_siegeenginest_action = {
    NotInUse = 0,
    PrepareToFire = 1,
    FireAtWill = 2,
}
---@enum building_siegeenginest_facing
df.building_siegeenginest_facing = {
    Left = 0,
    Up = 1,
    Right = 2,
    Down = 3,
}
---@class building_siegeenginest
-- inherit building_actual
---@field type siegeengine_type
---@field building_siegeenginest_facing building_siegeenginest_facing
---@field building_siegeenginest_action building_siegeenginest_action
---@field fire_timer integer
---@field fill_timer integer
df.building_siegeenginest = nil

---@class building_slabst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field unk_1 integer
df.building_slabst = nil

---@class building_statuest
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field unk_1 integer
df.building_statuest = nil

---@class building_supportst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field support_flags bitfield
df.building_supportst = nil

---@class building_tablest
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field table_flags bitfield
---@field users building_users
df.building_tablest = nil

---@class building_traction_benchst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field unk_1 integer
---@field users building_users
df.building_traction_benchst = nil

---@class building_tradedepotst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field trade_flags bitfield
---@field accessible integer
df.building_tradedepotst = nil

---@enum trap_type
df.trap_type = {
    Lever = 0,
    PressurePlate = 1,
    CageTrap = 2,
    StoneFallTrap = 3,
    WeaponTrap = 4,
    TrackStop = 5,
}

---@class pressure_plate_info
---@field unit_min integer
---@field unit_max integer
---@field water_min integer
---@field water_max integer
---@field magma_min integer
---@field magma_max integer
---@field track_min integer since v0.34.08
---@field track_max integer since v0.34.08
---@field flags bitfield
df.pressure_plate_info = nil

---@class building_trapst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field trap_type trap_type
---@field state integer !=0 = pulled, tripped/needs reloading
---@field ready_timeout integer plate not active if > 0
---@field fill_timer integer
---@field stop_flags bitfield
---@field linked_mechanisms item[]
---@field observed_by_civs integer[]
---@field profile workshop_profile
---@field plate_info pressure_plate_info
---@field friction integer since v0.34.08
---@field use_dump integer since v0.34.08
---@field dump_x_shift integer since v0.34.08
---@field dump_y_shift integer since v0.34.08
---@field stop_trigger_timer integer since v0.34.08
df.building_trapst = nil

---@class building_wagonst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
df.building_wagonst = nil

---@class building_weaponst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field gate_flags bitfield
---@field timer integer
df.building_weaponst = nil

---@class building_squad_use
---@field squad_id integer
---@field mode bitfield
df.building_squad_use = nil

---@class building_weaponrackst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field rack_flags integer
---@field specific_squad integer
df.building_weaponrackst = nil

---@class building_wellst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field well_flags bitfield
---@field unk_1 integer
---@field bucket_z integer
---@field bucket_timer integer 0-9; counts up when raising, down when lowering
---@field check_water_timer integer
df.building_wellst = nil

---@class building_windowst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field unk_1 integer
df.building_windowst = nil

---@class building_window_glassst
-- inherited from building_windowst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field unk_1 integer
-- end building_windowst
df.building_window_glassst = nil

---@class building_window_gemst
-- inherited from building_windowst
-- inherited from building_actual
-- inherited from building
---@field x1 integer top left
---@field y1 integer
---@field centerx integer work location
---@field x2 integer bottom right
---@field y2 integer
---@field centery integer
---@field z integer
---@field flags bitfield
---@field mat_type integer
---@field mat_index integer
---@field room building_extents
---@field age integer
---@field race integer
---@field id integer
---@field jobs job[]
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field relations building_civzonest[] zone(s) this building is in
---@field job_claim_suppress any[] after Remv Cre, prevents unit from taking jobs at building
---@field name string
---@field activities any[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field unk_v40_2 integer since v0.40.01
---@field site_id integer since v0.42.01
---@field location_id integer since v0.42.01
-- end building
---@field construction_stage integer 0 not started, then 1 or 3 max depending on type
---@field contained_items any[]
---@field design building_design
-- end building_actual
---@field unk_1 integer
-- end building_windowst
df.building_window_gemst = nil

---@enum dfhack_room_quality_level
df.dfhack_room_quality_level = {
    Meager = 0,
    Modest = 1,
    Normal = 2,
    Decent = 3,
    Fine = 4,
    Great = 5,
    Grand = 6,
    Royal = 7,
}


