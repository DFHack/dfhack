-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta

---@alias item_flags bitfield

---@alias item_flags2 bitfield

---@enum item_magicness_type
df.item_magicness_type = {
    Sparkle = 0,
    AirWarped = 1,
    Whistle = 2,
    OddlySquare = 3,
    SmallBumps = 4,
    EarthSmell = 5,
    Lightning = 6,
    GrayHairs = 7, -- with value of 10 or higher, creatures that look at the item cannot think negative thoughts
    RustlingLeaves = 8,
}

---@class item_magicness
---@field type item_magicness_type
---@field value integer boosts item value by 50*this
---@field unk_1 integer
---@field flags integer 1=does not show up in item description or alter item value
df.item_magicness = nil

---@class temperaturest
---@field whole integer
---@field fraction integer
df.temperaturest = nil

---@class spatter_common
---@field mat_type integer
---@field mat_index integer
---@field mat_state matter_state
---@field temperature temperaturest
---@field size integer 1-24=spatter, 25-49=smear, 50-* = coating
---@field base_flags bitfield since v0.40.13
---@field pad_1 integer needed for proper alignment of spatter on gcc
df.spatter_common = nil

---@class spatter
-- inherited from spatter_common
---@field mat_type integer
---@field mat_index integer
---@field mat_state matter_state
---@field temperature temperaturest
---@field size integer 1-24=spatter, 25-49=smear, 50-* = coating
---@field base_flags bitfield since v0.40.13
---@field pad_1 integer needed for proper alignment of spatter on gcc
-- end spatter_common
---@field body_part_id integer
---@field flags bitfield
df.spatter = nil

---@enum item_quality
df.item_quality = {
    Ordinary = 0,
    WellCrafted = 1,
    FinelyCrafted = 2,
    Superior = 3,
    Exceptional = 4,
    Masterful = 5,
    Artifact = 6,
}

---@enum slab_engraving_type
df.slab_engraving_type = {
    Slab = -1,
    Memorial = 0,
    CraftShopSign = 1,
    WeaponsmithShopSign = 2,
    ArmorsmithShopSign = 3,
    GeneralStoreSign = 4,
    FoodShopSign = 5,
    Secrets = 6, -- from the gods?
    FoodImportsSign = 7,
    ClothingImportsSign = 8,
    GeneralImportsSign = 9,
    ClothShopSign = 10,
    LeatherShopSign = 11,
    WovenClothingShopSign = 12,
    LeatherClothingShopSign = 13,
    BoneCarverShopSign = 14,
    GemCutterShopSign = 15,
    WeaponsmithShopSign2 = 16,
    BowyerShopSign = 17,
    BlacksmithShopSign = 18,
    ArmorsmithShopSign2 = 19,
    MetalCraftShopSign = 20,
    LeatherGoodsShopSign = 21,
    CarpenterShopSign = 22,
    StoneFurnitureShopSign = 23,
    MetalFurnitureShopSign = 24,
    DemonIdentity = 25, -- when a demon assumes identity?
    TavernSign = 26,
}

---@return item_type
function df.item.getType() end

---@return integer
function df.item.getSubtype() end

---@return integer
function df.item.getMaterial() end

---@return integer
function df.item.getMaterialIndex() end

---@param arg_0 integer
function df.item.setSubtype(arg_0) end

---@param arg_0 integer
function df.item.setMaterial(arg_0) end

---@param arg_0 integer
function df.item.setMaterialIndex(arg_0) end

---@return integer
function df.item.getActualMaterial() end -- returns an actual material type, never a race

---@return integer
function df.item.getActualMaterialIndex() end -- returns an actual material index, never a caste

---@return integer
function df.item.getRace() end -- only if the object is made of a "specific creature mat"

---@return integer
function df.item.getCaste() end -- only if the object is made of a "specific creature mat"

---@return integer
function df.item.getPlantID() end -- only if the object is made of a plant material

---@return integer
function df.item.getGrowthPrint() end -- since v0.40.01

---@param print integer
function df.item.setGrowthPrint(print) end -- since v0.40.01

---@return integer
function df.item.getDimension() end -- since v0.40.01

---@return integer
function df.item.getTotalDimension() end

---@param amount integer
function df.item.setDimension(amount) end

---@param amount integer
---@return boolean
function df.item.subtractDimension(amount) end

---@return boolean
function df.item.isFoodStorage() end

---@return boolean
function df.item.isTrackCart() end

---@return boolean
function df.item.isWheelbarrow() end

---@return integer
function df.item.getVehicleID() end

---@return boolean
function df.item.isAmmo() end

---@return item_stockpile_ref
function df.item.getStockpile() end

---@return boolean
function df.item.containsPlaster() end

---@return boolean
function df.item.isPlaster() end

---@param arg_0 integer
---@return boolean
function df.item.getColorOverride(arg_0) end

---@return integer
function df.item.getHistoryInfo() end

---@param use tool_uses
---@return boolean
function df.item.hasToolUse(use) end

---@return boolean
function df.item.hasInvertedTile() end

function df.item.becomePaste() end

function df.item.becomePressed() end

function df.item.calculateWeight() end

---@return boolean
function df.item.isSharpStone() end

---@return boolean
function df.item.isCrystalGlassable() end

---@param matIndex integer
---@return boolean
function df.item.isMetalOre(matIndex) end

function df.item.clearLastTempUpdateTS() end

---@param string_ptr string
function df.item.listNotableKills(string_ptr) end

---@return integer
function df.item.getSpecHeat() end

---@return integer
function df.item.getIgnitePoint() end

---@return integer
function df.item.getHeatdamPoint() end

---@return integer
function df.item.getColddamPoint() end

---@return integer
function df.item.getBoilingPoint() end

---@return integer
function df.item.getMeltingPoint() end

---@return integer
function df.item.getFixedTemp() end

---@return integer
function df.item.getSolidDensity() end

---@return boolean
function df.item.materialRots() end

---@return integer
function df.item.getTemperature() end

---@param target integer
---@param unk integer
---@return boolean
function df.item.adjustTemperature(target, unk) end

function df.item.unnamed_method() end

function df.item.extinguish() end

---@return integer
function df.item.getGloveHandedness() end

---@param arg_0 integer
function df.item.setGloveHandedness(arg_0) end

---@return boolean
function df.item.isSpike() end

---@return boolean
function df.item.isScrew() end

---@return boolean
function df.item.isBuildMat() end

---@param arg_0 integer
---@return boolean
function df.item.isTemperatureSafe(arg_0) end

---@param entity_id integer
function df.item.setRandSubtype(entity_id) end

---@return integer
function df.item.getWeaponSize() end -- weapon racks have capacity 5

---@return integer
function df.item.getWear() end

---@param arg_0 integer
function df.item.setWear(arg_0) end

---@return integer
function df.item.getMaker() end

---@param unit_id integer
function df.item.setMaker(unit_id) end

---@param prace integer
---@param pcaste integer
---@param phfig integer
---@param punit integer
function df.item.getCorpseInfo(prace, pcaste, phfig, punit) end

function df.item.unnamed_method() end

---@return integer
function df.item.getGloveFlags() end

---@return string
function df.item.getItemShapeDesc() end -- a statue/figurine of "string goes here"

function df.item.unnamed_method() end

function df.item.unnamed_method() end

---@param arg_0 item_filter_spec
---@return boolean
function df.item.isMatchingAmmoItem(arg_0) end

---@param id integer
---@param subid integer
function df.item.getImageRef(id, subid) end

---@param civ_id integer
---@param site_id integer
function df.item.getImageCivSite(civ_id, site_id) end

---@param civ_id integer
---@param site_id integer
function df.item.setImageCivSite(civ_id, site_id) end

---@param level integer
function df.item.setSeedsPlantSkillLevel(level) end

---@return integer
function df.item.getCorpseSize() end -- size_info.size_cur

---@param amount integer
---@return boolean
function df.item.ageItem(amount) end

---@return integer
function df.item.getCritterAirdrownTimer() end

---@param arg_0 integer
function df.item.setCritterAirdrownTimer(arg_0) end

function df.item.incrementCritterAirdrownTimer() end

---@return integer
function df.item.getRotTimer() end

---@param val integer
function df.item.setRotTimer(val) end

function df.item.incrementRotTimer() end

---@return boolean
function df.item.isBogeymanCorpse() end

---@param mat_flag material_flags
---@return boolean
function df.item.testMaterialFlag(mat_flag) end -- return true if item satisfies flag

---@param arg_1 string
---@return string
function df.item.getAmmoType(arg_1) end

---@return boolean
function df.item.isLiquidPowder() end

---@return boolean
function df.item.isLiquid() end

---@return boolean
function df.item.isLiveAnimal() end -- vermin, pet, or critter

---@return integer
function df.item.getVolume() end -- for putting in containers, building clutter

---@param imp_type improvement_type
---@param job job
---@param unit unit
---@param mat_type integer
---@param mat_index integer
---@param shape integer
---@param forced_quality integer
---@param entity historical_entity
---@param site world_site
---@param unk integer
---@param unshaped boolean
---@param arg_12 boolean
---@param arg_13 integer
---@return itemimprovement
function df.item.addImprovementFromJob(imp_type, job, unit, mat_type, mat_index, shape, forced_quality, entity, site, unk, unshaped, arg_12, arg_13) end

---@return boolean
function df.item.isWeapon() end

---@return boolean
function df.item.isArmorNotClothing() end

---@return boolean
function df.item.isMillable() end

---@return boolean
function df.item.isProcessableThread() end

---@return boolean
function df.item.isProcessableVial() end

---@return boolean
function df.item.isProcessableBarrel() end

---@return boolean
function df.item.isEdiblePlant() end

---@param hunger integer
---@return boolean
function df.item.isEdibleRaw(hunger) end

---@param hunger integer
---@return boolean
function df.item.isEdibleCarnivore(hunger) end

---@param hunger integer
---@return boolean
function df.item.isEdibleBonecarn(hunger) end

---@param x integer
---@param y integer
---@param z integer
---@return boolean
function df.item.moveToGround(x, y, z) end

---@param in_play boolean
function df.item.categorize(in_play) end -- Add item to world.items.other.*

function df.item.uncategorize() end -- Remove item from world.items.other.*

---@param empty boolean
---@return boolean
function df.item.isFurniture(empty) end

---@return boolean
function df.item.isPressed() end

---@return boolean
function df.item.isAnimal() end -- stored in Animal stockpiles

---@param maker unit
---@param job_skill job_skill
---@return item_quality
function df.item.assignQuality(maker, job_skill) end

---@param maker unit
---@param job_skill job_skill
---@param skill_roll integer
---@return item_quality
function df.item.assignQuality2(maker, job_skill, skill_roll) end

---@param maker unit
---@param arg_1 integer
---@param arg_2 integer
function df.item.notifyCreatedMasterwork(maker, arg_1, arg_2) end

function df.item.notifyLostMasterwork() end

---@param arg_0 integer
---@param arg_1 integer
---@param arg_2 integer
function df.item.addMagic(arg_0, arg_1, arg_2) end

---@param arg_0 integer
---@param arg_1 integer
function df.item.magic_unk1(arg_0, arg_1) end

---@param arg_0 integer
---@param arg_1 integer
function df.item.magic_unk2(arg_0, arg_1) end

---@param arg_0 integer
function df.item.magic_unk3(arg_0) end

---@param arg_0 integer
---@param arg_1 integer
---@param arg_2 integer
function df.item.magic_unk4(arg_0, arg_1, arg_2) end

---@param arg_0 integer
function df.item.setDisplayColor(arg_0) end

---@return boolean
function df.item.isDamagedByHeat() end

---@param arg_0 integer
---@return boolean
function df.item.needTwoHandedWield(arg_0) end

---@param stack_size integer
---@param preserve_containment boolean
---@return item
function df.item.splitStack(stack_size, preserve_containment) end

---@return boolean
function df.item.isTameableVermin() end

---@return boolean
function df.item.isDye() end

---@param arg_0 integer
---@param arg_1 integer
---@return boolean
function df.item.isMilkable(arg_0, arg_1) end

---@return boolean
function df.item.isSandBearing() end

---@return boolean
function df.item.isLyeBearing() end

---@return boolean
function df.item.isAnimalProduct() end

---@param item_type integer
---@param material_category integer
function df.item.getStorageInfo(item_type, material_category) end

---@param delta integer
---@param simple boolean
---@param lose_masterwork boolean
---@return boolean
function df.item.addWear(delta, simple, lose_masterwork) end

---@param delta integer
---@return boolean
function df.item.incWearTimer(delta) end

---@param simple boolean
---@param lose_masterwork boolean
---@return boolean
function df.item.checkWearDestroy(simple, lose_masterwork) end

---@param mat_type integer
---@param mat_index integer
---@param mat_state matter_state
---@param temp integer
---@param size integer
---@param body_part_id integer
---@param flags integer
function df.item.addContaminant(mat_type, mat_index, mat_state, temp, size, body_part_id, flags) end

---@param index integer
---@param amount integer
function df.item.removeContaminantByIdx(index, amount) end

---@param mat_type integer
---@param mat_index integer
---@param amount integer
function df.item.removeContaminant(mat_type, mat_index, amount) end

---@param arg_0 unit
---@param body_part_id integer
function df.item.tradeUnitContaminants(arg_0, body_part_id) end

---@param arg_0 item
function df.item.tradeItemContaminants(arg_0) end -- calls item.tIC2(this)

---@param arg_0 item_actual
function df.item.tradeItemContaminants2(arg_0) end

---@param arg_0 unit
---@param arg_1 unit_wound
---@param shift integer
---@param body_part_id integer
function df.item.contaminateWound(arg_0, arg_1, shift, body_part_id) end

---@param file file_compressorst
function df.item.write_file(file) end

---@param file file_compressorst
---@param loadversion save_version
function df.item.read_file(file, loadversion) end

---@return integer
function df.item.getWeaponAttacks() end

---@return boolean
function df.item.isNotHeld() end

---@return boolean
function df.item.isSplittable() end -- if false, you throw the entire stack at once

---@param arg_0 historical_entity
function df.item.addDefaultThreadImprovement(arg_0) end

---@param arg_0 item
---@param arg_1 historical_entity
function df.item.addThreadImprovement(arg_0, arg_1) end

function df.item.propagateUnitRefs() end

---@return boolean
function df.item.isSand() end

---@return integer
function df.item.unnamed_method() end -- returns item_actual.unk_1

---@return integer
function df.item.getStackSize() end

---@param amount integer
function df.item.addStackSize(amount) end

---@param amount integer
function df.item.setStackSize(amount) end

---@param arg_0 string
---@return boolean
function df.item.isAmmoClass(arg_0) end

---@return boolean
function df.item.isAutoClean() end -- delete on_ground every season when in ANY_AUTO_CLEAN; default true

---@param x integer
---@param y integer
---@param z integer
---@param arg_3 boolean
---@param contained boolean
---@return boolean
function df.item.setTemperatureFromMapTile(x, y, z, arg_3, contained) end

---@param arg_0 boolean
---@param contained boolean
---@return boolean
function df.item.setTemperatureFromMap(arg_0, contained) end

---@param temp integer
---@param arg_1 boolean
---@param contained boolean
---@return boolean
function df.item.setTemperature(temp, arg_1, contained) end

---@param arg_0 boolean
---@param contained boolean
---@param adjust boolean
---@param multiplier integer
---@return boolean
function df.item.updateTempFromMap(arg_0, contained, adjust, multiplier) end

---@param temp integer
---@param arg_1 boolean
---@param contained boolean
---@param adjust boolean
---@param multiplier integer
---@return boolean
function df.item.updateTemperature(temp, arg_1, contained, adjust, multiplier) end

---@return boolean
function df.item.updateFromWeather() end

---@return boolean
function df.item.updateContaminants() end

---@return boolean
function df.item.checkTemperatureDamage() end

---@return boolean
function df.item.checkHeatColdDamage() end

---@return boolean
function df.item.checkMeltBoil() end

---@return integer
function df.item.getMeleeSkill() end

---@return integer
function df.item.getRangedSkill() end

---@param quality integer
function df.item.setQuality(quality) end

---@return integer
function df.item.getQuality() end

---@return integer
function df.item.getOverallQuality() end

---@return integer
function df.item.getImprovementQuality() end

---@return integer
function df.item.getProjectileSize() end

---@param arg_0 job
---@param mat_type integer
---@param mat_index integer
---@return boolean
function df.item.isImprovable(arg_0, mat_type, mat_index) end

---@param item_quality integer
---@param unk1 integer
function df.item.setSharpness(item_quality, unk1) end

---@return integer
function df.item.getSharpness() end

---@return boolean
function df.item.isTotemable() end

---@return boolean
function df.item.isDyeable() end

---@return boolean
function df.item.isNotDyed() end

---@return boolean
function df.item.isDyed() end

---@return boolean
function df.item.canSewImage() end

---@return boolean
function df.item.unnamed_method() end

---@return boolean
function df.item.isProcessableVialAtStill() end

---@param item_type item_type
---@param item_subtype integer
---@param mat_type integer
---@param mat_index integer
---@return boolean
function df.item.isSimilarToItem(item_type, item_subtype, mat_type, mat_index) end

---@return integer
function df.item.getBlockChance() end

---@return integer
function df.item.unnamed_method() end -- returns 10 for tools and weapons, 0 for most other item types

---@return integer
function df.item.getMakerRace() end

---@param arg_0 integer
function df.item.setMakerRace(arg_0) end

---@return integer
function df.item.getEffectiveArmorLevel() end -- adds 1 if it has [METAL_ARMOR_LEVELS] and it's made of an inorganic mat

---@return boolean
function df.item.unnamed_method() end

---@return boolean
function df.item.isItemOrganicCloth() end

---@return boolean
function df.item.isMadeOfOrganicCloth() end

---@param mat_type integer
---@param mat_index integer
---@param mat_state matter_state
---@param temperature integer
function df.item.coverWithContaminant(mat_type, mat_index, mat_state, temperature) end -- also stops fire; used for rain

---@param imp_type improvement_type
---@return boolean
function df.item.hasSpecificImprovements(imp_type) end

---@return boolean
function df.item.hasImprovements() end

---@return boolean
function df.item.isImproved() end

---@return integer
function df.item.getMagic() end

---@param arg_0 string
---@param plurality integer
function df.item.getItemDescription(arg_0, plurality) end

---@param arg_0 string
---@param mode integer
function df.item.getItemDescriptionPrefix(arg_0, mode) end -- "a " or "the "

---@param arg_0 string
function df.item.getItemBasicName(arg_0) end -- usually just "item"

---@param caravan caravan_state
---@return integer
function df.item.getImprovementsValue(caravan) end

---@return boolean
function df.item.isExtractBearingFish() end

---@return boolean
function df.item.isExtractBearingVermin() end

---@return integer
function df.item.unnamed_method() end -- for armor, clothing, weapons, and tools, returns material size from the corresponding itemdef

---@return integer
function df.item.getBaseWeight() end

---@return integer
function df.item.getWeightShiftBits() end

---@return boolean
function df.item.isCollected() end

---@return boolean
function df.item.isEdibleVermin() end

---@return integer
function df.item.drawSelf() end

---@return boolean
function df.item.isRangedWeapon() end

---@return boolean
function df.item.isClothing() end

---@return boolean
function df.item.isWet() end

---@param appraiser historical_entity
---@return integer
function df.item.getCurrencyValue(appraiser) end -- that is, value of coins

---@return boolean
function df.item.isAssignedToStockpile() end

---@param arg_0 integer
---@return boolean
function df.item.isAssignedToThisStockpile(arg_0) end

function df.item.detachStockpileAssignment() end -- also removes links from the pile

function df.item.removeStockpileAssignment() end -- just wipes the fields

---@return item_stockpile_ref
function df.item.getStockpile2() end

---@param mat_type integer
---@param mat_index integer
---@param u unit
---@param j job
function df.item.randomizeThreadImprovement(mat_type, mat_index, u, j) end -- this updates the quality of a thread improvement already added to the item (or adjusts the quality of a thread item) based on the skill of the dyer

---@param arg_0 integer
---@param arg_1 integer
---@param arg_2 integer
---@param arg_3 integer
---@param arg_4 integer
---@param arg_5 integer
---@param arg_6 integer
---@param arg_7 integer
---@param arg_8 integer
---@param arg_9 integer
---@param arg_10 integer
---@param arg_11 integer
function df.item.unnamed_method(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8, arg_9, arg_10, arg_11) end

---@param arg_0 integer
function df.item.unnamed_method(arg_0) end

---@param caravan caravan_state
---@return integer
function df.item.getThreadDyeValue(caravan) end

---@param colors integer
---@param shapes integer
function df.item.getColorAndShape(colors, shapes) end

---@return boolean
function df.item.unnamed_method() end

---@return boolean
function df.item.isArmor() end -- for armor user skill encumberance

---@param arg_0 squad_uniform_spec
---@param exact_match boolean
---@param best_any job_skill
---@param best_melee job_skill
---@param best_ranged job_skill
---@return integer
function df.item.calcUniformScore(arg_0, exact_match, best_any, best_melee, best_ranged) end

---@return integer
function df.item.calcBaseUniformScore() end

---@param arg_0 integer
---@param arg_1 integer
---@param arg_2 integer
function df.item.unnamed_method(arg_0, arg_1, arg_2) end

---@return boolean
function df.item.unnamed_method() end

---@return boolean
function df.item.unnamed_method() end

---@param arg_0 integer
---@param arg_1 integer
function df.item.unnamed_method(arg_0, arg_1) end

---@param arg_0 integer
---@param arg_1 integer
---@param arg_2 integer
---@param arg_3 integer
---@param arg_4 integer
---@param arg_5 integer
---@param arg_6 integer
---@param arg_7 integer
function df.item.unnamed_method(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7) end

function df.item.unnamed_method() end

function df.item.unnamed_method() end

---@return slab_engraving_type
function df.item.getSlabEngravingType() end

---@return integer
function df.item.getAbsorption() end

---@return boolean
function df.item.unnamed_method() end

---@return boolean
function df.item.isGemMaterial() end

---@param shape integer
function df.item.setGemShape(shape) end

---@return boolean
function df.item.hasGemShape() end

---@return integer
function df.item.getGemShape() end

---@return boolean
function df.item.unnamed_method() end -- since v0.40.01

---@return boolean
function df.item.hasWriting() end

---@return boolean
function df.item.unnamed_method() end -- since v0.47.01

---@class item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
df.item = nil

---@class item_kill_info
---@field targets historical_kills
---@field slayers integer[]
---@field slayer_kill_counts integer[]
df.item_kill_info = nil

---@class item_history_info
---@field kills item_kill_info
---@field attack_counter integer increments by 1 each time the item is fired, thrown or used in an attack
---@field defence_counter integer increments by 1 each time the item is used in an attempt to block or parry
df.item_history_info = nil

---@class item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
df.item_actual = nil

---@class item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
df.item_crafted = nil

---@class item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
df.item_constructed = nil

---@alias body_part_status bitfield

---@alias body_layer_status bitfield

---@class body_component_info
---@field body_part_status body_part_status[]
---@field numbered_masks integer[] 1 bit per instance of a numbered body part
---@field nonsolid_remaining integer[] 0-100%
---@field layer_status body_layer_status[]
---@field layer_wound_area integer[]
---@field layer_cut_fraction integer[] 0-10000
---@field layer_dent_fraction integer[] 0-10000
---@field layer_effect_fraction integer[] 0-1000000000
df.body_component_info = nil

---@class body_size_info
---@field size_cur integer
---@field size_base integer
---@field area_cur integer size_cur^0.666
---@field area_base integer size_base^0.666
---@field length_cur integer (size_cur*10000)^0.333
---@field length_base integer (size_base*10000)^0.333
df.body_size_info = nil

---@enum corpse_material_type
df.corpse_material_type = {
    Plant = 0,
    Silk = 1,
    Leather = 2,
    Bone = 3,
    Shell = 4,
    unk_5 = 5,
    Soap = 6,
    Tooth = 7,
    Horn = 8,
    Pearl = 9,
    HairWool = 10,
    Yarn = 11,
}

---@class item_body_component_bone2
---@field mat_type integer
---@field mat_index integer

---@class item_body_component_bone1
---@field mat_type integer
---@field mat_index integer

---@class item_body_component_appearance
---@field colors integer[]
---@field tissue_style integer[]
---@field tissue_style_civ_id integer[]
---@field tissue_style_id integer[]
---@field tissue_style_type integer[]

---@class item_body_component_body
---@field wounds unit_wound[]
---@field unk_100 integer[] unit.body.unk_39c
---@field wound_next_id integer
---@field components body_component_info
---@field physical_attr_value integer[]
---@field physical_attr_soft_demotion integer[]
---@field size_info body_size_info
---@field body_part_relsize integer[] =unit.enemy.body_part_relsize
---@field body_modifiers integer[]
---@field bp_modifiers integer[]

---@class item_body_component
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field race integer
---@field hist_figure_id integer
---@field unit_id integer
---@field caste integer
---@field sex pronoun_type
---@field normal_race integer unit.enemy.normal_race
---@field normal_caste integer unit.enemy.normal_caste
---@field rot_timer integer
---@field unk_8c integer
---@field body item_body_component_body
---@field size_modifier integer =unit.appearance.size_modifier
---@field birth_year integer
---@field birth_time integer
---@field curse_year integer since v0.34.01
---@field curse_time integer since v0.34.01
---@field birth_year_bias integer since v0.34.01
---@field birth_time_bias integer since v0.34.01
---@field death_year integer
---@field death_time integer
---@field appearance item_body_component_appearance
---@field blood_count integer
---@field stored_fat integer
---@field hist_figure_id2 integer
---@field undead_unit_id integer since v0.34.01
---@field unit_id2 integer
---@field corpse_flags bitfield
---@field material_amount integer[]
---@field bone1 item_body_component_bone1
---@field bone2 item_body_component_bone2
df.item_body_component = nil

---@class item_corpsest
-- inherited from item_body_component
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field race integer
---@field hist_figure_id integer
---@field unit_id integer
---@field caste integer
---@field sex pronoun_type
---@field normal_race integer unit.enemy.normal_race
---@field normal_caste integer unit.enemy.normal_caste
---@field rot_timer integer
---@field unk_8c integer
---@field body item_body_component_body
---@field size_modifier integer =unit.appearance.size_modifier
---@field birth_year integer
---@field birth_time integer
---@field curse_year integer since v0.34.01
---@field curse_time integer since v0.34.01
---@field birth_year_bias integer since v0.34.01
---@field birth_time_bias integer since v0.34.01
---@field death_year integer
---@field death_time integer
---@field appearance item_body_component_appearance
---@field blood_count integer
---@field stored_fat integer
---@field hist_figure_id2 integer
---@field undead_unit_id integer since v0.34.01
---@field unit_id2 integer
---@field corpse_flags bitfield
---@field material_amount integer[]
---@field bone1 item_body_component_bone1
---@field bone2 item_body_component_bone2
-- end item_body_component
---@field unused_380 integer
---@field unused_388 integer
---@field unused_390 integer
---@field unused_398 integer
---@field unused_39c integer
---@field unused_3a0 integer
df.item_corpsest = nil

---@class item_corpsepiecest
-- inherited from item_body_component
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field race integer
---@field hist_figure_id integer
---@field unit_id integer
---@field caste integer
---@field sex pronoun_type
---@field normal_race integer unit.enemy.normal_race
---@field normal_caste integer unit.enemy.normal_caste
---@field rot_timer integer
---@field unk_8c integer
---@field body item_body_component_body
---@field size_modifier integer =unit.appearance.size_modifier
---@field birth_year integer
---@field birth_time integer
---@field curse_year integer since v0.34.01
---@field curse_time integer since v0.34.01
---@field birth_year_bias integer since v0.34.01
---@field birth_time_bias integer since v0.34.01
---@field death_year integer
---@field death_time integer
---@field appearance item_body_component_appearance
---@field blood_count integer
---@field stored_fat integer
---@field hist_figure_id2 integer
---@field undead_unit_id integer since v0.34.01
---@field unit_id2 integer
---@field corpse_flags bitfield
---@field material_amount integer[]
---@field bone1 item_body_component_bone1
---@field bone2 item_body_component_bone2
-- end item_body_component
df.item_corpsepiecest = nil

---@class item_critter
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field race integer
---@field caste integer
---@field milk_timer integer
---@field airdrown_timer integer
---@field name language_name
df.item_critter = nil

---@alias item_matstate bitfield

---@class item_liquipowder
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_state bitfield
---@field dimension integer
df.item_liquipowder = nil

---@class item_liquid
-- inherited from item_liquipowder
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_state bitfield
---@field dimension integer
-- end item_liquipowder
---@field mat_type integer
---@field mat_index integer
df.item_liquid = nil

---@class item_powder
-- inherited from item_liquipowder
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_state bitfield
---@field dimension integer
-- end item_liquipowder
---@field mat_type integer
---@field mat_index integer
df.item_powder = nil

---@class item_barst
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field subtype integer
---@field mat_type integer
---@field mat_index integer
---@field dimension integer
df.item_barst = nil

---@class item_smallgemst
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field shape integer
df.item_smallgemst = nil

---@class item_blocksst
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
df.item_blocksst = nil

---@class item_roughst
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
df.item_roughst = nil

---@class item_boulderst
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
df.item_boulderst = nil

---@class item_woodst
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
df.item_woodst = nil

---@class item_branchst
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
df.item_branchst = nil

---@class item_rockst
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field sharpness integer
---@field unk_84 integer
df.item_rockst = nil

---@class item_seedsst
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field grow_counter integer
---@field planting_skill integer
df.item_seedsst = nil

---@class item_skin_tannedst
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field unk_80 integer
df.item_skin_tannedst = nil

---@class item_meatst
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field rot_timer integer
df.item_meatst = nil

---@class item_plantst
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field rot_timer integer
df.item_plantst = nil

---@class item_plant_growthst
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field subtype integer
---@field growth_print integer
---@field mat_type integer
---@field mat_index integer
---@field rot_timer integer
df.item_plant_growthst = nil

---@class item_cheesest
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field rot_timer integer
df.item_cheesest = nil

---@class item_globst
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field rot_timer integer
---@field mat_state bitfield
---@field dimension integer
df.item_globst = nil

---@class item_remainsst
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field race integer
---@field caste integer
---@field rot_timer integer
df.item_remainsst = nil

---@class item_fishst
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field race integer
---@field caste integer
---@field rot_timer integer
df.item_fishst = nil

---@class item_fish_rawst
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field race integer
---@field caste integer
---@field rot_timer integer
df.item_fish_rawst = nil

---@class item_foodst
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field subtype itemdef_foodst
---@field entity integer
---@field recipe_id integer
---@field ingredients any[]
---@field rot_timer integer
df.item_foodst = nil

---@class item_verminst
-- inherited from item_critter
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field race integer
---@field caste integer
---@field milk_timer integer
---@field airdrown_timer integer
---@field name language_name
-- end item_critter
df.item_verminst = nil

---@class item_petst
-- inherited from item_critter
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field race integer
---@field caste integer
---@field milk_timer integer
---@field airdrown_timer integer
---@field name language_name
-- end item_critter
---@field owner_id integer
---@field pet_flags bitfield
df.item_petst = nil

---@class item_drinkst
-- inherited from item_liquid
-- inherited from item_liquipowder
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_state bitfield
---@field dimension integer
-- end item_liquipowder
---@field mat_type integer
---@field mat_index integer
-- end item_liquid
df.item_drinkst = nil

---@class item_powder_miscst
-- inherited from item_powder
-- inherited from item_liquipowder
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_state bitfield
---@field dimension integer
-- end item_liquipowder
---@field mat_type integer
---@field mat_index integer
-- end item_powder
df.item_powder_miscst = nil

---@class item_liquid_miscst
-- inherited from item_liquid
-- inherited from item_liquipowder
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_state bitfield
---@field dimension integer
-- end item_liquipowder
---@field mat_type integer
---@field mat_index integer
-- end item_liquid
---@field unk_88 integer
df.item_liquid_miscst = nil

---@class item_threadst
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field dye_mat_type integer
---@field dye_mat_index integer
---@field dyer integer
---@field dye_masterpiece_event integer
---@field dye_quality integer
---@field unk_92 integer
---@field unk_94 integer
---@field unk_98 integer
---@field dimension integer
df.item_threadst = nil

---@class item_eggst
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field race integer
---@field caste integer
---@field unk_7c integer
---@field egg_materials material_vec_ref
---@field egg_flags bitfield
---@field incubation_counter integer increments when fertile in unforbidden nestbox, hatch at >= 100800 (3 months)
---@field hatchling_civ_id integer hatchlings will have this civ_id
---@field hatchling_population_id integer hatchlings will have this population_id
---@field hatchling_unit_unk_c0 integer hatchlings will have this unit.unk_c0 value
---@field unk_2 integer since v0.40.01
---@field mothers_genes unit_genes object owned by egg item
---@field mothers_caste integer
---@field unk_3 integer since v0.40.01
---@field fathers_genes unit_genes object owned by egg item
---@field fathers_caste integer -1 if no father genes
---@field unk_4 integer since v0.40.01
---@field hatchling_flags1 bitfield used primarily for bit_flag:tame
---@field hatchling_flags2 bitfield used primarily for bit_flag:roaming_wilderness_population_source
---@field hatchling_flags3 bitfield not normally used, most/all do not stick
---@field unk_v42_1 integer since v0.42.01
---@field hatchling_training_level animal_training_level
---@field hatchling_animal_population world_population_ref
---@field hatchling_mother_id integer
---@field size integer
df.item_eggst = nil

---@class item_doorst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_doorst = nil

---@class item_floodgatest
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_floodgatest = nil

---@class item_bedst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_bedst = nil

---@class item_chairst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_chairst = nil

---@class item_chainst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_chainst = nil

---@class item_flaskst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_flaskst = nil

---@class item_gobletst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_gobletst = nil

---@class item_windowst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_windowst = nil

---@class item_cagest
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_cagest = nil

---@class item_bucketst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_bucketst = nil

---@class item_animaltrapst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_animaltrapst = nil

---@class item_tablest
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_tablest = nil

---@class item_coffinst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_coffinst = nil

---@class item_bagst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_bagst = nil

---@class item_boxst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_boxst = nil

---@class item_armorstandst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_armorstandst = nil

---@class item_weaponrackst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_weaponrackst = nil

---@class item_cabinetst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_cabinetst = nil

---@class item_amuletst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_amuletst = nil

---@class item_scepterst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_scepterst = nil

---@class item_crownst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_crownst = nil

---@class item_ringst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_ringst = nil

---@class item_earringst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_earringst = nil

---@class item_braceletst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_braceletst = nil

---@class item_anvilst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_anvilst = nil

---@class item_backpackst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_backpackst = nil

---@class item_quiverst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_quiverst = nil

---@class item_catapultpartsst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_catapultpartsst = nil

---@class item_ballistapartsst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_ballistapartsst = nil

---@class item_trappartsst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_trappartsst = nil

---@class item_pipe_sectionst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_pipe_sectionst = nil

---@class item_hatch_coverst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_hatch_coverst = nil

---@class item_gratest
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_gratest = nil

---@class item_quernst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_quernst = nil

---@class item_millstonest
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_millstonest = nil

---@class item_splintst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_splintst = nil

---@class item_crutchst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_crutchst = nil

---@class item_traction_benchst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
df.item_traction_benchst = nil

---@class item_instrumentst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field subtype itemdef_instrumentst
df.item_instrumentst = nil

---@class item_toyst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field subtype itemdef_toyst
df.item_toyst = nil

---@class item_armorst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field subtype itemdef_armorst
df.item_armorst = nil

---@class item_shoesst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field subtype itemdef_shoesst
df.item_shoesst = nil

---@class item_shieldst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field subtype itemdef_shieldst
df.item_shieldst = nil

---@class item_helmst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field subtype itemdef_helmst
df.item_helmst = nil

---@class item_glovesst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field subtype itemdef_glovesst
---@field handedness any[] 1 right, 2 left
df.item_glovesst = nil

---@class item_pantsst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field subtype itemdef_pantsst
df.item_pantsst = nil

---@class item_siegeammost
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field subtype itemdef_siegeammost
---@field sharpness integer since v0.40.15
df.item_siegeammost = nil

---@class item_weaponst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field subtype itemdef_weaponst
---@field sharpness integer
df.item_weaponst = nil

---@class item_ammost
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field subtype itemdef_ammost
---@field sharpness integer
df.item_ammost = nil

---@class item_trapcompst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field subtype itemdef_trapcompst
---@field sharpness integer
df.item_trapcompst = nil

---@class item_toolst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field subtype itemdef_toolst
---@field sharpness integer
---@field stockpile item_stockpile_ref
---@field vehicle_id integer since v0.34.08
---@field unk_2 integer since v0.47.01
---@field unk_3 integer since v0.47.01
df.item_toolst = nil

---@class item_stockpile_ref
---@field id integer
---@field x integer
---@field y integer
df.item_stockpile_ref = nil

---@class item_barrelst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field stockpile item_stockpile_ref
df.item_barrelst = nil

---@class item_binst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field stockpile item_stockpile_ref
df.item_binst = nil

---@class item_gemst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field shape integer
df.item_gemst = nil

---@class item_statuest
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field image art_image_ref
---@field description string
---@field unk_110 integer
---@field unk_114 integer
df.item_statuest = nil

---@class item_figurinest
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field image art_image_ref
---@field description string
df.item_figurinest = nil

---@class item_slabst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field description string
---@field topic integer or interaction id for secrets?
---@field engraving_type slab_engraving_type
df.item_slabst = nil

---@class item_orthopedic_castst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field body_part string
---@field material string
df.item_orthopedic_castst = nil

---@class item_coinst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field coin_batch integer
df.item_coinst = nil

---@class item_totemst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field race integer
---@field caste integer
---@field body_part_idx integer
df.item_totemst = nil

---@class item_clothst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field dimension integer
df.item_clothst = nil

---@class item_bookst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field title string
df.item_bookst = nil

---@class item_ballistaarrowheadst
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field sharpness integer since v0.40.15
df.item_ballistaarrowheadst = nil

---@class item_sheetst
-- inherited from item_constructed
-- inherited from item_crafted
-- inherited from item_actual
-- inherited from item
---@field pos coord
---@field flags bitfield
---@field flags2 bitfield since v0.34.08
---@field age integer
---@field id integer
---@field specific_refs specific_ref[]
---@field general_refs general_ref[]
---@field world_data_id integer since v0.34.01
---@field world_data_subid integer since v0.34.01
---@field stockpile_countdown integer -1 per 50 frames; then check if needs moving
---@field stockpile_delay integer used to reset countdown; randomly varies
---@field unk2 integer
---@field base_uniform_score integer
---@field walkable_id integer from map_block.walkable
---@field spec_heat integer
---@field ignite_point integer
---@field heatdam_point integer
---@field colddam_point integer
---@field boiling_point integer
---@field melting_point integer
---@field fixed_temp integer
---@field weight integer if flags.weight_computed
---@field weight_fraction integer 1e-6
-- end item
---@field stack_size integer
---@field history_info integer
---@field magic integer
---@field contaminants integer
---@field temperature temperaturest
---@field wear integer
---@field wear_timer integer counts up to 806400
---@field unk_1 integer since v0.34.01
---@field temp_updated_frame integer
-- end item_actual
---@field mat_type integer
---@field mat_index integer
---@field maker_race integer
---@field quality item_quality
---@field skill_rating skill_rating at the moment of creation
---@field maker integer
---@field masterpiece_event integer
-- end item_crafted
---@field improvements itemimprovement[]
-- end item_constructed
---@field dimension integer
---@field unk_2 integer
df.item_sheetst = nil


