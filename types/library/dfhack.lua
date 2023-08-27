-- THIS FILE WAS AUTOMATICALLY GENERATED. DO NOT EDIT.

---@meta


-- CXX SIGNATURE -> string Translation::TranslateName(const df::language_name * name, bool inEnglish, bool onlyLastPart)
---@param name language_name
---@param inEnglish boolean
---@param onlyLastPart boolean
---@return string
function dfhack.translation.TranslateName(name, inEnglish, onlyLastPart) end

-- CXX SIGNATURE -> df::viewscreen *Gui::getCurViewscreen(bool skip_dismissed)
---@param skip_dismissed boolean
---@return viewscreen
function dfhack.gui.getCurViewscreen(skip_dismissed) end

-- CXX SIGNATURE -> df::viewscreen *Gui::getDFViewscreen(bool skip_dismissed, df::viewscreen *screen)
---@param skip_dismissed boolean
---@param screen viewscreen
---@return viewscreen
function dfhack.gui.getDFViewscreen(skip_dismissed, screen) end

-- CXX SIGNATURE -> df::job *Gui::getSelectedWorkshopJob(color_ostream &out, bool quiet)
---@param quiet boolean
---@return job
function dfhack.gui.getSelectedWorkshopJob(quiet) end

-- CXX SIGNATURE -> df::job *Gui::getSelectedJob(color_ostream &out, bool quiet)
---@param quiet boolean
---@return job
function dfhack.gui.getSelectedJob(quiet) end

-- CXX SIGNATURE -> df::unit *Gui::getSelectedUnit(color_ostream &out, bool quiet)
---@param quiet boolean
---@return unit
function dfhack.gui.getSelectedUnit(quiet) end

-- CXX SIGNATURE -> df::item *Gui::getSelectedItem(color_ostream &out, bool quiet)
---@param quiet boolean
---@return item
function dfhack.gui.getSelectedItem(quiet) end

-- CXX SIGNATURE -> df::building *Gui::getSelectedBuilding(color_ostream &out, bool quiet)
---@param quiet boolean
---@return building
function dfhack.gui.getSelectedBuilding(quiet) end

-- CXX SIGNATURE -> df::building_civzonest *Gui::getSelectedCivZone(color_ostream &out, bool quiet)
---@param quiet boolean
---@return building_civzonest
function dfhack.gui.getSelectedCivZone(quiet) end

-- CXX SIGNATURE -> df::building_stockpilest* Gui::getSelectedStockpile(color_ostream& out, bool quiet)
---@param quiet boolean
---@return building_stockpilest
function dfhack.gui.getSelectedStockpile(quiet) end

-- CXX SIGNATURE -> df::plant *Gui::getSelectedPlant(color_ostream &out, bool quiet)
---@param quiet boolean
---@return plant
function dfhack.gui.getSelectedPlant(quiet) end

-- CXX SIGNATURE -> df::unit *Gui::getAnyUnit(df::viewscreen *top)
---@param top viewscreen
---@return unit
function dfhack.gui.getAnyUnit(top) end

-- CXX SIGNATURE -> df::item *Gui::getAnyItem(df::viewscreen *top)
---@param top viewscreen
---@return item
function dfhack.gui.getAnyItem(top) end

-- CXX SIGNATURE -> df::building *Gui::getAnyBuilding(df::viewscreen *top)
---@param top viewscreen
---@return building
function dfhack.gui.getAnyBuilding(top) end

-- CXX SIGNATURE -> df::plant *Gui::getAnyPlant(df::viewscreen *top)
---@param top viewscreen
---@return plant
function dfhack.gui.getAnyPlant(top) end

-- CXX SIGNATURE -> void Gui::writeToGamelog(std::string message)
---@param message string
---@return nil
function dfhack.gui.writeToGamelog(message) end

-- CXX SIGNATURE -> int Gui::makeAnnouncement(df::announcement_type type, df::announcement_flags flags, df::coord pos, std::string message, int color, bool bright)
---@param type announcement_type
---@param flags announcement_flags
---@param pos coord
---@param message string
---@param color integer
---@param bright boolean
---@return integer
function dfhack.gui.makeAnnouncement(type, flags, pos, message, color, bright) end

-- CXX SIGNATURE -> bool Gui::addCombatReport(df::unit *unit, df::unit_report_type slot, int report_index)
---@param unit unit
---@param slot unit_report_type
---@param report_index integer
---@return boolean
function dfhack.gui.addCombatReport(unit, slot, report_index) end

-- CXX SIGNATURE -> bool Gui::addCombatReportAuto(df::unit *unit, df::announcement_flags mode, int report_index)
---@param unit unit
---@param mode announcement_flags
---@param report_index integer
---@return boolean
function dfhack.gui.addCombatReportAuto(unit, mode, report_index) end

-- CXX SIGNATURE -> void Gui::showAnnouncement(std::string message, int color, bool bright)
---@param message string
---@param color integer
---@param bright boolean
---@return nil
function dfhack.gui.showAnnouncement(message, color, bright) end

-- CXX SIGNATURE -> void Gui::showZoomAnnouncement(    df::announcement_type type, df::coord pos, std::string message, int color, bool bright)
---@param type announcement_type
---@param pos coord
---@param message string
---@param color integer
---@param bright boolean
---@return nil
function dfhack.gui.showZoomAnnouncement(type, pos, message, color, bright) end

-- CXX SIGNATURE -> void Gui::showPopupAnnouncement(std::string message, int color, bool bright)
---@param message string
---@param color integer
---@param bright boolean
---@return nil
function dfhack.gui.showPopupAnnouncement(message, color, bright) end

-- CXX SIGNATURE -> void Gui::showAutoAnnouncement(    df::announcement_type type, df::coord pos, std::string message, int color, bool bright,    df::unit *unit1, df::unit *unit2)
---@param type announcement_type
---@param pos coord
---@param message string
---@param color integer
---@param bright boolean
---@param unit1 unit
---@param unit2 unit
---@return nil
function dfhack.gui.showAutoAnnouncement(type, pos, message, color, bright, unit1, unit2) end

-- CXX SIGNATURE -> void Gui::resetDwarfmodeView(bool pause)
---@param pause boolean
---@return nil
function dfhack.gui.resetDwarfmodeView(pause) end

-- CXX SIGNATURE -> bool Gui::refreshSidebar()
---@return boolean
function dfhack.gui.refreshSidebar() end

-- CXX SIGNATURE -> bool Gui::inRenameBuilding()
---@return boolean
function dfhack.gui.inRenameBuilding() end

-- CXX SIGNATURE -> int Gui::getDepthAt (int32_t x, int32_t y)
---@param x integer
---@param y integer
---@return integer
function dfhack.gui.getDepthAt(x, y) end

-- CXX SIGNATURE -> bool Gui::matchFocusString(std::string focus_string, df::viewscreen *top)
---@param focus_string string
---@param top viewscreen
---@return boolean
function dfhack.gui.matchFocusString(focus_string, top) end

-- CXX SIGNATURE -> long Textures::getDfhackLogoTexposStart()
---@return number
function dfhack.textures.getDfhackLogoTexposStart() end

-- CXX SIGNATURE -> long Textures::getGreenPinTexposStart()
---@return number
function dfhack.textures.getGreenPinTexposStart() end

-- CXX SIGNATURE -> long Textures::getRedPinTexposStart()
---@return number
function dfhack.textures.getRedPinTexposStart() end

-- CXX SIGNATURE -> long Textures::getIconsTexposStart()
---@return number
function dfhack.textures.getIconsTexposStart() end

-- CXX SIGNATURE -> long Textures::getOnOffTexposStart()
---@return number
function dfhack.textures.getOnOffTexposStart() end

-- CXX SIGNATURE -> long Textures::getMapUnsuspendTexposStart()
---@return number
function dfhack.textures.getMapUnsuspendTexposStart() end

-- CXX SIGNATURE -> long Textures::getControlPanelTexposStart()
---@return number
function dfhack.textures.getControlPanelTexposStart() end

-- CXX SIGNATURE -> long Textures::getThinBordersTexposStart()
---@return number
function dfhack.textures.getThinBordersTexposStart() end

-- CXX SIGNATURE -> long Textures::getMediumBordersTexposStart()
---@return number
function dfhack.textures.getMediumBordersTexposStart() end

-- CXX SIGNATURE -> long Textures::getBoldBordersTexposStart()
---@return number
function dfhack.textures.getBoldBordersTexposStart() end

-- CXX SIGNATURE -> long Textures::getPanelBordersTexposStart()
---@return number
function dfhack.textures.getPanelBordersTexposStart() end

-- CXX SIGNATURE -> long Textures::getWindowBordersTexposStart()
---@return number
function dfhack.textures.getWindowBordersTexposStart() end

-- CXX SIGNATURE -> bool Units::isUnitInBox(df::unit* u,                        int16_t x1, int16_t y1, int16_t z1,                        int16_t x2, int16_t y2, int16_t z2)
---@param u unit
---@param x1 integer
---@param y1 integer
---@param z1 integer
---@param x2 integer
---@param y2 integer
---@param z2 integer
---@return boolean
function dfhack.units.isUnitInBox(u, x1, y1, z1, x2, y2, z2) end

-- CXX SIGNATURE -> bool Units::isActive(df::unit *unit)
---@param unit unit
---@return boolean
function dfhack.units.isActive(unit) end

-- CXX SIGNATURE -> bool Units::isVisible(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isVisible(unit) end

-- CXX SIGNATURE -> bool Units::isCitizen(df::unit *unit, bool ignore_sanity)
---@param unit unit
---@param ignore_sanity boolean
---@return boolean
function dfhack.units.isCitizen(unit, ignore_sanity) end

-- CXX SIGNATURE -> bool Units::isFortControlled(df::unit *unit)
---@param unit unit
---@return boolean
function dfhack.units.isFortControlled(unit) end

-- CXX SIGNATURE -> bool Units::isOwnCiv(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isOwnCiv(unit) end

-- CXX SIGNATURE -> bool Units::isOwnGroup(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isOwnGroup(unit) end

-- CXX SIGNATURE -> bool Units::isOwnRace(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isOwnRace(unit) end

-- CXX SIGNATURE -> bool Units::isAlive(df::unit *unit)
---@param unit unit
---@return boolean
function dfhack.units.isAlive(unit) end

-- CXX SIGNATURE -> bool Units::isDead(df::unit *unit)
---@param unit unit
---@return boolean
function dfhack.units.isDead(unit) end

-- CXX SIGNATURE -> bool Units::isKilled(df::unit *unit)
---@param unit unit
---@return boolean
function dfhack.units.isKilled(unit) end

-- CXX SIGNATURE -> bool Units::isSane(df::unit *unit)
---@param unit unit
---@return boolean
function dfhack.units.isSane(unit) end

-- CXX SIGNATURE -> bool Units::isCrazed(df::unit *unit)
---@param unit unit
---@return boolean
function dfhack.units.isCrazed(unit) end

-- CXX SIGNATURE -> bool Units::isGhost(df::unit *unit)
---@param unit unit
---@return boolean
function dfhack.units.isGhost(unit) end

-- CXX SIGNATURE -> bool Units::isHidden(df::unit *unit)
---@param unit unit
---@return boolean
function dfhack.units.isHidden(unit) end

-- CXX SIGNATURE -> bool Units::isHidingCurse(df::unit *unit)
---@param unit unit
---@return boolean
function dfhack.units.isHidingCurse(unit) end

-- CXX SIGNATURE -> bool Units::isMale(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isMale(unit) end

-- CXX SIGNATURE -> bool Units::isFemale(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isFemale(unit) end

-- CXX SIGNATURE -> bool Units::isBaby(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isBaby(unit) end

-- CXX SIGNATURE -> bool Units::isChild(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isChild(unit) end

-- CXX SIGNATURE -> bool Units::isAdult(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isAdult(unit) end

-- CXX SIGNATURE -> bool Units::isGay(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isGay(unit) end

-- CXX SIGNATURE -> bool Units::isNaked(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isNaked(unit) end

-- CXX SIGNATURE -> bool Units::isVisiting(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isVisiting(unit) end

-- CXX SIGNATURE -> bool Units::isTrainableHunting(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isTrainableHunting(unit) end

-- CXX SIGNATURE -> bool Units::isTrainableWar(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isTrainableWar(unit) end

-- CXX SIGNATURE -> bool Units::isTrained(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isTrained(unit) end

-- CXX SIGNATURE -> bool Units::isHunter(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isHunter(unit) end

-- CXX SIGNATURE -> bool Units::isWar(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isWar(unit) end

-- CXX SIGNATURE -> bool Units::isTame(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isTame(unit) end

-- CXX SIGNATURE -> bool Units::isTamable(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isTamable(unit) end

-- CXX SIGNATURE -> bool Units::isDomesticated(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isDomesticated(unit) end

-- CXX SIGNATURE -> bool Units::isMarkedForTraining(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isMarkedForTraining(unit) end

-- CXX SIGNATURE -> bool Units::isMarkedForTaming(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isMarkedForTaming(unit) end

-- CXX SIGNATURE -> bool Units::isMarkedForWarTraining(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isMarkedForWarTraining(unit) end

-- CXX SIGNATURE -> bool Units::isMarkedForHuntTraining(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isMarkedForHuntTraining(unit) end

-- CXX SIGNATURE -> bool Units::isMarkedForSlaughter(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isMarkedForSlaughter(unit) end

-- CXX SIGNATURE -> bool Units::isMarkedForGelding(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isMarkedForGelding(unit) end

-- CXX SIGNATURE -> bool Units::isGeldable(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isGeldable(unit) end

-- CXX SIGNATURE -> bool Units::isGelded(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isGelded(unit) end

-- CXX SIGNATURE -> bool Units::isEggLayer(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isEggLayer(unit) end

-- CXX SIGNATURE -> bool Units::isEggLayerRace(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isEggLayerRace(unit) end

-- CXX SIGNATURE -> bool Units::isGrazer(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isGrazer(unit) end

-- CXX SIGNATURE -> bool Units::isMilkable(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isMilkable(unit) end

-- CXX SIGNATURE -> bool Units::isForest(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isForest(unit) end

-- CXX SIGNATURE -> bool Units::isMischievous(df::unit *unit)
---@param unit unit
---@return boolean
function dfhack.units.isMischievous(unit) end

-- CXX SIGNATURE -> bool Units::isAvailableForAdoption(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isAvailableForAdoption(unit) end

-- CXX SIGNATURE -> bool Units::isPet(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isPet(unit) end

-- CXX SIGNATURE -> bool Units::hasExtravision(df::unit *unit)
---@param unit unit
---@return boolean
function dfhack.units.hasExtravision(unit) end

-- CXX SIGNATURE -> bool Units::isOpposedToLife(df::unit *unit)
---@param unit unit
---@return boolean
function dfhack.units.isOpposedToLife(unit) end

-- CXX SIGNATURE -> bool Units::isBloodsucker(df::unit *unit)
---@param unit unit
---@return boolean
function dfhack.units.isBloodsucker(unit) end

-- CXX SIGNATURE -> bool Units::isDwarf(df::unit *unit)
---@param unit unit
---@return boolean
function dfhack.units.isDwarf(unit) end

-- CXX SIGNATURE -> bool Units::isAnimal(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isAnimal(unit) end

-- CXX SIGNATURE -> bool Units::isMerchant(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isMerchant(unit) end

-- CXX SIGNATURE -> bool Units::isDiplomat(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isDiplomat(unit) end

-- CXX SIGNATURE -> bool Units::isVisitor(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isVisitor(unit) end

-- CXX SIGNATURE -> bool Units::isInvader(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isInvader(unit) end

-- CXX SIGNATURE -> bool Units::isUndead(df::unit* unit, bool include_vamps)
---@param unit unit
---@param include_vamps boolean
---@return boolean
function dfhack.units.isUndead(unit, include_vamps) end

-- CXX SIGNATURE -> bool Units::isNightCreature(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isNightCreature(unit) end

-- CXX SIGNATURE -> bool Units::isSemiMegabeast(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isSemiMegabeast(unit) end

-- CXX SIGNATURE -> bool Units::isMegabeast(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isMegabeast(unit) end

-- CXX SIGNATURE -> bool Units::isTitan(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isTitan(unit) end

-- CXX SIGNATURE -> bool Units::isDemon(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isDemon(unit) end

-- CXX SIGNATURE -> bool Units::isDanger(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isDanger(unit) end

-- CXX SIGNATURE -> bool Units::isGreatDanger(df::unit* unit)
---@param unit unit
---@return boolean
function dfhack.units.isGreatDanger(unit) end

-- CXX SIGNATURE -> bool Units::teleport(df::unit *unit, df::coord target_pos)
---@param unit unit
---@param target_pos coord
---@return boolean
function dfhack.units.teleport(unit, target_pos) end

-- CXX SIGNATURE -> df::general_ref *Units::getGeneralRef(df::unit *unit, df::general_ref_type type)
---@param unit unit
---@param type general_ref_type
---@return general_ref
function dfhack.units.getGeneralRef(unit, type) end

-- CXX SIGNATURE -> df::specific_ref *Units::getSpecificRef(df::unit *unit, df::specific_ref_type type)
---@param unit unit
---@param type specific_ref_type
---@return specific_ref
function dfhack.units.getSpecificRef(unit, type) end

-- CXX SIGNATURE -> df::item *Units::getContainer(df::unit *unit)
---@param unit unit
---@return item
function dfhack.units.getContainer(unit) end

-- CXX SIGNATURE -> void Units::setNickname(df::unit *unit, std::string nick)
---@param unit unit
---@param nick string
---@return nil
function dfhack.units.setNickname(unit, nick) end

-- CXX SIGNATURE -> df::language_name *Units::getVisibleName(df::unit *unit)
---@param unit unit
---@return language_name
function dfhack.units.getVisibleName(unit) end

-- CXX SIGNATURE -> df::identity *Units::getIdentity(df::unit *unit)
---@param unit unit
---@return identity
function dfhack.units.getIdentity(unit) end

-- CXX SIGNATURE -> df::nemesis_record *Units::getNemesis(df::unit *unit)
---@param unit unit
---@return nemesis_record
function dfhack.units.getNemesis(unit) end

-- CXX SIGNATURE -> int Units::getPhysicalAttrValue(df::unit *unit, df::physical_attribute_type attr)
---@param unit unit
---@param attr physical_attribute_type
---@return integer
function dfhack.units.getPhysicalAttrValue(unit, attr) end

-- CXX SIGNATURE -> int Units::getMentalAttrValue(df::unit *unit, df::mental_attribute_type attr)
---@param unit unit
---@param attr mental_attribute_type
---@return integer
function dfhack.units.getMentalAttrValue(unit, attr) end

-- CXX SIGNATURE -> bool Units::casteFlagSet(int race, int caste, df::caste_raw_flags flag)
---@param race integer
---@param caste integer
---@param flag caste_raw_flags
---@return boolean
function dfhack.units.casteFlagSet(race, caste, flag) end

-- CXX SIGNATURE -> df::unit_misc_trait *Units::getMiscTrait(df::unit *unit, df::misc_trait_type type, bool create)
---@param unit unit
---@param type misc_trait_type
---@param create boolean
---@return unit_misc_trait
function dfhack.units.getMiscTrait(unit, type, create) end

-- CXX SIGNATURE -> double Units::getAge(df::unit *unit, bool true_age)
---@param unit unit
---@param true_age boolean
---@return number
function dfhack.units.getAge(unit, true_age) end

-- CXX SIGNATURE -> int Units::getKillCount(df::unit *unit)
---@param unit unit
---@return integer
function dfhack.units.getKillCount(unit) end

-- CXX SIGNATURE -> int Units::getNominalSkill(df::unit *unit, df::job_skill skill_id, bool use_rust)
---@param unit unit
---@param skill_id job_skill
---@param use_rust boolean
---@return integer
function dfhack.units.getNominalSkill(unit, skill_id, use_rust) end

-- CXX SIGNATURE -> int Units::getEffectiveSkill(df::unit *unit, df::job_skill skill_id)
---@param unit unit
---@param skill_id job_skill
---@return integer
function dfhack.units.getEffectiveSkill(unit, skill_id) end

-- CXX SIGNATURE -> int Units::getExperience(df::unit *unit, df::job_skill skill_id, bool total)
---@param unit unit
---@param skill_id job_skill
---@param total boolean
---@return integer
function dfhack.units.getExperience(unit, skill_id, total) end

-- CXX SIGNATURE -> bool Units::isValidLabor(df::unit *unit, df::unit_labor labor)
---@param unit unit
---@param labor unit_labor
---@return boolean
function dfhack.units.isValidLabor(unit, labor) end

-- CXX SIGNATURE -> bool Units::setLaborValidity(df::unit_labor labor, bool isValid)
---@param labor unit_labor
---@param isValid boolean
---@return boolean
function dfhack.units.setLaborValidity(labor, isValid) end

-- CXX SIGNATURE -> int Units::computeMovementSpeed(df::unit *unit)
---@param unit unit
---@return integer
function dfhack.units.computeMovementSpeed(unit) end

-- CXX SIGNATURE -> float Units::computeSlowdownFactor(df::unit *unit)
---@param unit unit
---@return number
function dfhack.units.computeSlowdownFactor(unit) end

-- CXX SIGNATURE -> std::string Units::getProfessionName(df::unit *unit, bool ignore_noble, bool plural)
---@param unit unit
---@param ignore_noble boolean
---@param plural boolean
---@return string
function dfhack.units.getProfessionName(unit, ignore_noble, plural) end

-- CXX SIGNATURE -> std::string Units::getCasteProfessionName(int race, int casteid, df::profession pid, bool plural)
---@param race integer
---@param casteid integer
---@param pid profession
---@param plural boolean
---@return string
function dfhack.units.getCasteProfessionName(race, casteid, pid, plural) end

-- CXX SIGNATURE -> int8_t Units::getProfessionColor(df::unit *unit, bool ignore_noble)
---@param unit unit
---@param ignore_noble boolean
---@return integer
function dfhack.units.getProfessionColor(unit, ignore_noble) end

-- CXX SIGNATURE -> int8_t Units::getCasteProfessionColor(int race, int casteid, df::profession pid)
---@param race integer
---@param casteid integer
---@param pid profession
---@return integer
function dfhack.units.getCasteProfessionColor(race, casteid, pid) end

-- CXX SIGNATURE -> df::goal_type Units::getGoalType(df::unit *unit, size_t goalIndex)
---@param unit unit
---@param goalIndex integer
---@return goal_type
function dfhack.units.getGoalType(unit, goalIndex) end

-- CXX SIGNATURE -> std::string Units::getGoalName(df::unit *unit, size_t goalIndex)
---@param unit unit
---@param goalIndex integer
---@return string
function dfhack.units.getGoalName(unit, goalIndex) end

-- CXX SIGNATURE -> bool Units::isGoalAchieved(df::unit *unit, size_t goalIndex)
---@param unit unit
---@param goalIndex integer
---@return boolean
function dfhack.units.isGoalAchieved(unit, goalIndex) end

-- CXX SIGNATURE -> string Units::getPhysicalDescription(df::unit* unit)
---@param unit unit
---@return string
function dfhack.units.getPhysicalDescription(unit) end

-- CXX SIGNATURE -> string Units::getRaceName(df::unit* unit)
---@param unit unit
---@return string
function dfhack.units.getRaceName(unit) end

-- CXX SIGNATURE -> string Units::getRaceNamePlural(df::unit* unit)
---@param unit unit
---@return string
function dfhack.units.getRaceNamePlural(unit) end

-- CXX SIGNATURE -> string Units::getRaceNameById(int32_t id)
---@param id integer
---@return string
function dfhack.units.getRaceNameById(id) end

-- CXX SIGNATURE -> string Units::getRaceBabyName(df::unit* unit)
---@param unit unit
---@return string
function dfhack.units.getRaceBabyName(unit) end

-- CXX SIGNATURE -> string Units::getRaceBabyNameById(int32_t id)
---@param id integer
---@return string
function dfhack.units.getRaceBabyNameById(id) end

-- CXX SIGNATURE -> string Units::getRaceChildName(df::unit* unit)
---@param unit unit
---@return string
function dfhack.units.getRaceChildName(unit) end

-- CXX SIGNATURE -> string Units::getRaceChildNameById(int32_t id)
---@param id integer
---@return string
function dfhack.units.getRaceChildNameById(id) end

-- CXX SIGNATURE -> string Units::getReadableName(df::unit* unit)
---@param unit unit
---@return string
function dfhack.units.getReadableName(unit) end

-- CXX SIGNATURE -> df::activity_entry *Units::getMainSocialActivity(df::unit *unit)
---@param unit unit
---@return activity_entry
function dfhack.units.getMainSocialActivity(unit) end

-- CXX SIGNATURE -> df::activity_event *Units::getMainSocialEvent(df::unit *unit)
---@param unit unit
---@return activity_event
function dfhack.units.getMainSocialEvent(unit) end

-- CXX SIGNATURE -> int Units::getStressCategory(df::unit *unit)
---@param unit unit
---@return integer
function dfhack.units.getStressCategory(unit) end

-- CXX SIGNATURE -> int Units::getStressCategoryRaw(int32_t stress_level)
---@param stress_level integer
---@return integer
function dfhack.units.getStressCategoryRaw(stress_level) end

-- CXX SIGNATURE -> void Units::subtractActionTimers(color_ostream &out, df::unit *unit, int32_t amount, df::unit_action_type affectedActionType)
---@param unit unit
---@param amount integer
---@param affectedActionType unit_action_type
---@return nil
function dfhack.units.subtractActionTimers(unit, amount, affectedActionType) end

-- CXX SIGNATURE -> void Units::subtractGroupActionTimers(color_ostream &out, df::unit *unit, int32_t amount, df::unit_action_type_group affectedActionTypeGroup)
---@param unit unit
---@param amount integer
---@param affectedActionTypeGroup unit_action_type_group
---@return nil
function dfhack.units.subtractGroupActionTimers(unit, amount, affectedActionTypeGroup) end

-- CXX SIGNATURE -> void Units::multiplyActionTimers(color_ostream &out, df::unit *unit, float amount, df::unit_action_type affectedActionType)
---@param unit unit
---@param amount number
---@param affectedActionType unit_action_type
---@return nil
function dfhack.units.multiplyActionTimers(unit, amount, affectedActionType) end

-- CXX SIGNATURE -> void Units::multiplyGroupActionTimers(color_ostream &out, df::unit *unit, float amount, df::unit_action_type_group affectedActionTypeGroup)
---@param unit unit
---@param amount number
---@param affectedActionTypeGroup unit_action_type_group
---@return nil
function dfhack.units.multiplyGroupActionTimers(unit, amount, affectedActionTypeGroup) end

-- CXX SIGNATURE -> void Units::setActionTimers(color_ostream &out, df::unit *unit, int32_t amount, df::unit_action_type affectedActionType)
---@param unit unit
---@param amount integer
---@param affectedActionType unit_action_type
---@return nil
function dfhack.units.setActionTimers(unit, amount, affectedActionType) end

-- CXX SIGNATURE -> void Units::setGroupActionTimers(color_ostream &out, df::unit *unit, int32_t amount, df::unit_action_type_group affectedActionTypeGroup)
---@param unit unit
---@param amount integer
---@param affectedActionTypeGroup unit_action_type_group
---@return nil
function dfhack.units.setGroupActionTimers(unit, amount, affectedActionTypeGroup) end

-- CXX SIGNATURE -> df::unit *Units::getUnitByNobleRole(string noble)
---@param noble string
---@return unit
function dfhack.units.getUnitByNobleRole(noble) end

-- CXX SIGNATURE -> df::squad* Military::makeSquad(int32_t assignment_id)
---@param assignment_id integer
---@return squad
function dfhack.military.makeSquad(assignment_id) end

-- CXX SIGNATURE -> void Military::updateRoomAssignments(int32_t squad_id, int32_t civzone_id, df::squad_use_flags flags)
---@param squad_id integer
---@param civzone_id integer
---@param flags squad_use_flags
---@return nil
function dfhack.military.updateRoomAssignments(squad_id, civzone_id, flags) end

-- CXX SIGNATURE -> std::string Military::getSquadName(int32_t squad_id)
---@param squad_id integer
---@return string
function dfhack.military.getSquadName(squad_id) end

-- CXX SIGNATURE -> df::general_ref *Items::getGeneralRef(df::item *item, df::general_ref_type type)
---@param item item
---@param type general_ref_type
---@return general_ref
function dfhack.items.getGeneralRef(item, type) end

-- CXX SIGNATURE -> df::specific_ref *Items::getSpecificRef(df::item *item, df::specific_ref_type type)
---@param item item
---@param type specific_ref_type
---@return specific_ref
function dfhack.items.getSpecificRef(item, type) end

-- CXX SIGNATURE -> df::unit *Items::getOwner(df::item * item)
---@param item item
---@return unit
function dfhack.items.getOwner(item) end

-- CXX SIGNATURE -> bool Items::setOwner(df::item *item, df::unit *unit)
---@param item item
---@param unit unit
---@return boolean
function dfhack.items.setOwner(item, unit) end

-- CXX SIGNATURE -> df::item *Items::getContainer(df::item * item)
---@param item item
---@return item
function dfhack.items.getContainer(item) end

-- CXX SIGNATURE -> df::building *Items::getHolderBuilding(df::item * item)
---@param item item
---@return building
function dfhack.items.getHolderBuilding(item) end

-- CXX SIGNATURE -> df::unit *Items::getHolderUnit(df::item * item)
---@param item item
---@return unit
function dfhack.items.getHolderUnit(item) end

-- CXX SIGNATURE -> std::string Items::getBookTitle(df::item *item)
---@param item item
---@return string
function dfhack.items.getBookTitle(item) end

-- CXX SIGNATURE -> std::string Items::getDescription(df::item *item, int type, bool decorate)
---@param item item
---@param type integer
---@param decorate boolean
---@return string
function dfhack.items.getDescription(item, type, decorate) end

-- CXX SIGNATURE -> bool Items::isCasteMaterial(df::item_type itype)
---@param itype item_type
---@return boolean
function dfhack.items.isCasteMaterial(itype) end

-- CXX SIGNATURE -> int Items::getSubtypeCount(df::item_type itype)
---@param itype item_type
---@return integer
function dfhack.items.getSubtypeCount(itype) end

-- CXX SIGNATURE -> df::itemdef *Items::getSubtypeDef(df::item_type itype, int subtype)
---@param itype item_type
---@param subtype integer
---@return itemdef
function dfhack.items.getSubtypeDef(itype, subtype) end

-- CXX SIGNATURE -> int Items::getItemBaseValue(int16_t item_type, int16_t item_subtype, int16_t mat_type, int32_t mat_subtype)
---@param item_type integer
---@param item_subtype integer
---@param mat_type integer
---@param mat_subtype integer
---@return integer
function dfhack.items.getItemBaseValue(item_type, item_subtype, mat_type, mat_subtype) end

-- CXX SIGNATURE -> int Items::getValue(df::item *item, df::caravan_state *caravan)
---@param item item
---@param caravan caravan_state
---@return integer
function dfhack.items.getValue(item, caravan) end

-- CXX SIGNATURE -> bool Items::isRequestedTradeGood(df::item *item, df::caravan_state *caravan)
---@param item item
---@param caravan caravan_state
---@return boolean
function dfhack.items.isRequestedTradeGood(item, caravan) end

-- CXX SIGNATURE -> int32_t Items::createItem(df::item_type item_type, int16_t item_subtype, int16_t mat_type, int32_t mat_index, df::unit* unit)
---@param item_type item_type
---@param item_subtype integer
---@param mat_type integer
---@param mat_index integer
---@param unit unit
---@return integer
function dfhack.items.createItem(item_type, item_subtype, mat_type, mat_index, unit) end

-- CXX SIGNATURE -> bool Items::checkMandates(df::item *item)
---@param item item
---@return boolean
function dfhack.items.checkMandates(item) end

-- CXX SIGNATURE -> bool Items::canTrade(df::item *item)
---@param item item
---@return boolean
function dfhack.items.canTrade(item) end

-- CXX SIGNATURE -> bool Items::canTradeWithContents(df::item *item)
---@param item item
---@return boolean
function dfhack.items.canTradeWithContents(item) end

-- CXX SIGNATURE -> bool Items::canTradeAnyWithContents(df::item *item)
---@param item item
---@return boolean
function dfhack.items.canTradeAnyWithContents(item) end

-- CXX SIGNATURE -> bool Items::markForTrade(df::item *item, df::building_tradedepotst *depot)
---@param item item
---@param depot building_tradedepotst
---@return boolean
function dfhack.items.markForTrade(item, depot) end

-- CXX SIGNATURE -> bool Items::isRouteVehicle(df::item *item)
---@param item item
---@return boolean
function dfhack.items.isRouteVehicle(item) end

-- CXX SIGNATURE -> bool Items::isSquadEquipment(df::item *item)
---@param item item
---@return boolean
function dfhack.items.isSquadEquipment(item) end

-- CXX SIGNATURE -> bool DFHack::Items::moveToGround(MapExtras::MapCache &mc, df::item *item, df::coord pos)
---@param item item
---@param pos coord
---@return boolean
function dfhack.items.moveToGround(item, pos) end

-- CXX SIGNATURE -> bool DFHack::Items::moveToContainer(MapExtras::MapCache &mc, df::item *item, df::item *container)
---@param item item
---@param container item
---@return boolean
function dfhack.items.moveToContainer(item, container) end

-- CXX SIGNATURE -> bool DFHack::Items::moveToInventory(    MapExtras::MapCache &mc, df::item *item, df::unit *unit,    df::unit_inventory_item::T_mode mode, int body_part)
---@param item item
---@param unit unit
---@param mode unit_inventory_item__T_mode -- unknown
---@param body_part integer
---@return boolean
function dfhack.items.moveToInventory(item, unit, mode, body_part) end

-- CXX SIGNATURE -> df::proj_itemst *Items::makeProjectile(MapExtras::MapCache &mc, df::item *item)
---@param item item
---@return proj_itemst
function dfhack.items.makeProjectile(item) end

-- CXX SIGNATURE -> bool Items::remove(MapExtras::MapCache &mc, df::item *item, bool no_uncat)
---@param item item
---@param no_uncat boolean
---@return boolean
function dfhack.items.remove(item, no_uncat) end

-- CXX SIGNATURE -> void Maps::enableBlockUpdates(df::map_block *blk, bool flow, bool temperature)
---@param blk map_block
---@param flow boolean
---@param temperature boolean
---@return nil
function dfhack.maps.enableBlockUpdates(blk, flow, temperature) end

-- CXX SIGNATURE -> df::feature_init *Maps::getGlobalInitFeature(int32_t index)
---@param index integer
---@return feature_init
function dfhack.maps.getGlobalInitFeature(index) end

-- CXX SIGNATURE -> df::feature_init *Maps::getLocalInitFeature(df::coord2d rgn_pos, int32_t index)
---@param rgn_pos coord2d
---@param index integer
---@return feature_init
function dfhack.maps.getLocalInitFeature(rgn_pos, index) end

-- CXX SIGNATURE -> bool Maps::canWalkBetween(df::coord pos1, df::coord pos2)
---@param pos1 coord
---@param pos2 coord
---@return boolean
function dfhack.maps.canWalkBetween(pos1, pos2) end

-- CXX SIGNATURE -> df::flow_info *Maps::spawnFlow(df::coord pos, df::flow_type type, int mat_type, int mat_index, int density)
---@param pos coord
---@param type flow_type
---@param mat_type integer
---@param mat_index integer
---@param density integer
---@return flow_info
function dfhack.maps.spawnFlow(pos, type, mat_type, mat_index, density) end

-- CXX SIGNATURE -> df::map_block *Maps::getBlock (int32_t blockx, int32_t blocky, int32_t blockz)
---@param blockx integer
---@param blocky integer
---@param blockz integer
---@return map_block
function dfhack.maps.getBlock(blockx, blocky, blockz) end

-- CXX SIGNATURE -> static bool hasTileAssignment(df::tile_bitmask *bm)
---@param bm tile_bitmask
---@return boolean
function dfhack.maps.hasTileAssignment(bm) end

-- CXX SIGNATURE -> static bool getTileAssignment(df::tile_bitmask *bm, int x, int y)
---@param bm tile_bitmask
---@param x integer
---@param y integer
---@return boolean
function dfhack.maps.getTileAssignment(bm, x, y) end

-- CXX SIGNATURE -> static void setTileAssignment(df::tile_bitmask *bm, int x, int y, bool val)
---@param bm tile_bitmask
---@param x integer
---@param y integer
---@param val boolean
---@return nil
function dfhack.maps.setTileAssignment(bm, x, y, val) end

-- CXX SIGNATURE -> static void resetTileAssignment(df::tile_bitmask *bm, bool val)
---@param bm tile_bitmask
---@param val boolean
---@return nil
function dfhack.maps.resetTileAssignment(bm, val) end

-- CXX SIGNATURE -> bool World::ReadPauseState()
---@return boolean
function dfhack.world.ReadPauseState() end

-- CXX SIGNATURE -> void World::SetPauseState(bool paused)
---@param paused boolean
---@return nil
function dfhack.world.SetPauseState(paused) end

-- CXX SIGNATURE -> uint32_t World::ReadCurrentTick()
---@return integer
function dfhack.world.ReadCurrentTick() end

-- CXX SIGNATURE -> uint32_t World::ReadCurrentYear()
---@return integer
function dfhack.world.ReadCurrentYear() end

-- CXX SIGNATURE -> uint32_t World::ReadCurrentMonth()
---@return integer
function dfhack.world.ReadCurrentMonth() end

-- CXX SIGNATURE -> uint32_t World::ReadCurrentDay()
---@return integer
function dfhack.world.ReadCurrentDay() end

-- CXX SIGNATURE -> uint8_t World::ReadCurrentWeather()
---@return integer
function dfhack.world.ReadCurrentWeather() end

-- CXX SIGNATURE -> void World::SetCurrentWeather(uint8_t weather)
---@param weather integer
---@return nil
function dfhack.world.SetCurrentWeather(weather) end

-- CXX SIGNATURE -> string World::ReadWorldFolder()
---@return string
function dfhack.world.ReadWorldFolder() end

-- CXX SIGNATURE -> df::burrow *Burrows::findByName(std::string name)
---@param name string
---@return burrow
function dfhack.burrows.findByName(name) end

-- CXX SIGNATURE -> void Burrows::clearUnits(df::burrow *burrow)
---@param burrow burrow
---@return nil
function dfhack.burrows.clearUnits(burrow) end

-- CXX SIGNATURE -> bool Burrows::isAssignedUnit(df::burrow *burrow, df::unit *unit)
---@param burrow burrow
---@param unit unit
---@return boolean
function dfhack.burrows.isAssignedUnit(burrow, unit) end

-- CXX SIGNATURE -> void Burrows::setAssignedUnit(df::burrow *burrow, df::unit *unit, bool enable)
---@param burrow burrow
---@param unit unit
---@param enable boolean
---@return nil
function dfhack.burrows.setAssignedUnit(burrow, unit, enable) end

-- CXX SIGNATURE -> void Burrows::clearTiles(df::burrow *burrow)
---@param burrow burrow
---@return nil
function dfhack.burrows.clearTiles(burrow) end

-- CXX SIGNATURE -> bool Burrows::isAssignedBlockTile(df::burrow *burrow, df::map_block *block, df::coord2d tile)
---@param burrow burrow
---@param block map_block
---@param tile coord2d
---@return boolean
function dfhack.burrows.isAssignedBlockTile(burrow, block, tile) end

-- CXX SIGNATURE -> bool Burrows::setAssignedBlockTile(df::burrow *burrow, df::map_block *block, df::coord2d tile, bool enable)
---@param burrow burrow
---@param block map_block
---@param tile coord2d
---@param enable boolean
---@return boolean
function dfhack.burrows.setAssignedBlockTile(burrow, block, tile, enable) end

-- CXX SIGNATURE -> df::general_ref *Buildings::getGeneralRef(df::building *building, df::general_ref_type type)
---@param building building
---@param type general_ref_type
---@return general_ref
function dfhack.buildings.getGeneralRef(building, type) end

-- CXX SIGNATURE -> df::specific_ref *Buildings::getSpecificRef(df::building *building, df::specific_ref_type type)
---@param building building
---@param type specific_ref_type
---@return specific_ref
function dfhack.buildings.getSpecificRef(building, type) end

-- CXX SIGNATURE -> bool Buildings::setOwner(df::building_civzonest *bld, df::unit *unit)
---@param bld building_civzonest
---@param unit unit
---@return boolean
function dfhack.buildings.setOwner(bld, unit) end

-- CXX SIGNATURE -> df::building *Buildings::allocInstance(df::coord pos, df::building_type type, int subtype, int custom)
---@param pos coord
---@param type building_type
---@param subtype integer
---@param custom integer
---@return building
function dfhack.buildings.allocInstance(pos, type, subtype, custom) end

-- CXX SIGNATURE -> bool Buildings::checkFreeTiles(df::coord pos, df::coord2d size,                               df::building_extents *ext,                               bool create_ext,                               bool allow_occupied,                               bool allow_wall)
---@param pos coord
---@param size coord2d
---@param ext building_extents
---@param create_ext boolean
---@param allow_occupied boolean
---@param allow_wall boolean
---@return boolean
function dfhack.buildings.checkFreeTiles(pos, size, ext, create_ext, allow_occupied, allow_wall) end

-- CXX SIGNATURE -> int Buildings::countExtentTiles(df::building_extents *ext, int defval)
---@param ext building_extents
---@param defval integer
---@return integer
function dfhack.buildings.countExtentTiles(ext, defval) end

-- CXX SIGNATURE -> bool Buildings::hasSupport(df::coord pos, df::coord2d size)
---@param pos coord
---@param size coord2d
---@return boolean
function dfhack.buildings.hasSupport(pos, size) end

-- CXX SIGNATURE -> bool Buildings::constructAbstract(df::building *bld)
---@param bld building
---@return boolean
function dfhack.buildings.constructAbstract(bld) end

-- CXX SIGNATURE -> bool Buildings::constructWithItems(df::building *bld, std::vector<df::item*> items)
---@param bld building
---@param items item[]
---@return boolean
function dfhack.buildings.constructWithItems(bld, items) end

-- CXX SIGNATURE -> bool Buildings::constructWithFilters(df::building *bld, std::vector<df::job_item*> items)
---@param bld building
---@param items job_item[]
---@return boolean
function dfhack.buildings.constructWithFilters(bld, items) end

-- CXX SIGNATURE -> bool Buildings::deconstruct(df::building *bld)
---@param bld building
---@return boolean
function dfhack.buildings.deconstruct(bld) end

-- CXX SIGNATURE -> void Buildings::notifyCivzoneModified(df::building* bld)
---@param bld building
---@return nil
function dfhack.buildings.notifyCivzoneModified(bld) end

-- CXX SIGNATURE -> bool Buildings::markedForRemoval(df::building *bld)
---@param bld building
---@return boolean
function dfhack.buildings.markedForRemoval(bld) end

-- CXX SIGNATURE -> std::string Buildings::getRoomDescription(df::building *building, df::unit *unit)
---@param building building
---@param unit unit
---@return string
function dfhack.buildings.getRoomDescription(building, unit) end

-- CXX SIGNATURE -> bool Buildings::isActivityZone(df::building * building)
---@param building building
---@return boolean
function dfhack.buildings.isActivityZone(building) end

-- CXX SIGNATURE -> bool Buildings::isPenPasture(df::building * building)
---@param building building
---@return boolean
function dfhack.buildings.isPenPasture(building) end

-- CXX SIGNATURE -> bool Buildings::isPitPond(df::building * building)
---@param building building
---@return boolean
function dfhack.buildings.isPitPond(building) end

-- CXX SIGNATURE -> bool Buildings::isActive(df::building * building)
---@param building building
---@return boolean
function dfhack.buildings.isActive(building) end

-- CXX SIGNATURE -> bool Buildings::containsTile(df::building *bld, df::coord2d tile)
---@param bld building
---@param tile coord2d
---@return boolean
function dfhack.buildings.containsTile(bld, tile) end

-- CXX SIGNATURE -> bool Constructions::designateNew(df::coord pos, df::construction_type type,                                 df::item_type item, int mat_index)
---@param pos coord
---@param type construction_type
---@param item item_type
---@param mat_index integer
---@return boolean
function dfhack.constructions.designateNew(pos, type, item, mat_index) end

-- CXX SIGNATURE -> bool Constructions::insert(df::construction * constr)
---@param constr construction
---@return boolean
function dfhack.constructions.insert(constr) end

-- CXX SIGNATURE -> bool Screen::inGraphicsMode()
---@return boolean
function dfhack.screen.inGraphicsMode() end

-- CXX SIGNATURE -> void clear()
---@return nil
function dfhack.screen.clear() end

-- CXX SIGNATURE -> bool Screen::invalidate()
---@return boolean
function dfhack.screen.invalidate() end

-- CXX SIGNATURE -> string Screen::getKeyDisplay(df::interface_key key)
---@param key interface_key
---@return string
function dfhack.screen.getKeyDisplay(key) end

-- CXX SIGNATURE -> std::string Filesystem::getcwd ()
---@return string
function dfhack.filesystem.getcwd() end

-- CXX SIGNATURE -> bool Filesystem::restore_cwd ()
---@return boolean
function dfhack.filesystem.restore_cwd() end

-- CXX SIGNATURE -> std::string Filesystem::get_initial_cwd ()
---@return string
function dfhack.filesystem.get_initial_cwd() end

-- CXX SIGNATURE -> bool Filesystem::chdir (std::string path)
---@param path string
---@return boolean
function dfhack.filesystem.chdir(path) end

-- CXX SIGNATURE -> bool Filesystem::mkdir (std::string path)
---@param path string
---@return boolean
function dfhack.filesystem.mkdir(path) end

-- CXX SIGNATURE -> bool Filesystem::mkdir_recursive (std::string path)
---@param path string
---@return boolean
function dfhack.filesystem.mkdir_recursive(path) end

-- CXX SIGNATURE -> bool Filesystem::rmdir (std::string path)
---@param path string
---@return boolean
function dfhack.filesystem.rmdir(path) end

-- CXX SIGNATURE -> bool Filesystem::exists (std::string path)
---@param path string
---@return boolean
function dfhack.filesystem.exists(path) end

-- CXX SIGNATURE -> bool Filesystem::isfile (std::string path)
---@param path string
---@return boolean
function dfhack.filesystem.isfile(path) end

-- CXX SIGNATURE -> bool Filesystem::isdir (std::string path)
---@param path string
---@return boolean
function dfhack.filesystem.isdir(path) end

-- CXX SIGNATURE -> bool Designations::markPlant(const df::plant *plant)
---@param plant plant
---@return boolean
function dfhack.designations.markPlant(plant) end

-- CXX SIGNATURE -> bool Designations::unmarkPlant(const df::plant *plant)
---@param plant plant
---@return boolean
function dfhack.designations.unmarkPlant(plant) end

-- CXX SIGNATURE -> bool Designations::canMarkPlant(const df::plant *plant)
---@param plant plant
---@return boolean
function dfhack.designations.canMarkPlant(plant) end

-- CXX SIGNATURE -> bool Designations::canUnmarkPlant(const df::plant *plant)
---@param plant plant
---@return boolean
function dfhack.designations.canUnmarkPlant(plant) end

-- CXX SIGNATURE -> bool Designations::isPlantMarked(const df::plant *plant)
---@param plant plant
---@return boolean
function dfhack.designations.isPlantMarked(plant) end

-- CXX SIGNATURE -> int Kitchen::findExclusion(df::kitchen_exc_type type,    df::item_type item_type, int16_t item_subtype,    int16_t mat_type, int32_t mat_index)
---@param type kitchen_exc_type
---@param item_type item_type
---@param item_subtype integer
---@param mat_type integer
---@param mat_index integer
---@return integer
function dfhack.kitchen.findExclusion(type, item_type, item_subtype, mat_type, mat_index) end

-- CXX SIGNATURE -> bool Kitchen::addExclusion(df::kitchen_exc_type type,    df::item_type item_type, int16_t item_subtype,    int16_t mat_type, int32_t mat_index)
---@param type kitchen_exc_type
---@param item_type item_type
---@param item_subtype integer
---@param mat_type integer
---@param mat_index integer
---@return boolean
function dfhack.kitchen.addExclusion(type, item_type, item_subtype, mat_type, mat_index) end

-- CXX SIGNATURE -> bool Kitchen::removeExclusion(df::kitchen_exc_type type,    df::item_type item_type, int16_t item_subtype,    int16_t mat_type, int32_t mat_index)
---@param type kitchen_exc_type
---@param item_type item_type
---@param item_subtype integer
---@param mat_type integer
---@param mat_index integer
---@return boolean
function dfhack.kitchen.removeExclusion(type, item_type, item_subtype, mat_type, mat_index) end

-- CXX SIGNATURE -> void clear()
---@return nil
function dfhack.console.clear() end

-- CXX SIGNATURE -> void flush()
---@return nil
function dfhack.console.flush() end

-- CXX SIGNATURE -> static inline uint32_tdecode(uint32_t* state, uint32_t* codep, uint8_t byte)
---@param state integer
---@param codep integer
---@param byte integer
---@return uint32_tdecod -- unknown
function dfhack.matinfo.decode(state, codep, byte) end

-- CXX SIGNATURE -> void clear()
---@return nil
function dfhack.penarray.clear() end

-- CXX SIGNATURE -> bool init()
---@return boolean
function dfhack.random.init() end

-- CXX SIGNATURE -> static int getCommandHistory(lua_State *state)
---@return integer
function dfhack.getCommandHistory() end

-- CXX SIGNATURE -> bool Gui::autoDFAnnouncement(df::report_init r, string message)
---@param r report_init
---@param message string
---@return boolean
function dfhack.gui.autoDFAnnouncement(r, message) end

-- CXX SIGNATURE -> Gui::DwarfmodeDims Gui::getDwarfmodeViewDims()
---@return Gui__DwarfmodeDims -- unknown
function dfhack.gui.getDwarfmodeViewDims() end

-- CXX SIGNATURE -> bool Gui::pauseRecenter(int32_t x, int32_t y, int32_t z, bool pause)
---@param x integer
---@param y integer
---@param z integer
---@param pause boolean
---@return boolean
function dfhack.gui.pauseRecenter(x, y, z, pause) end

-- CXX SIGNATURE -> bool Gui::revealInDwarfmodeMap(int32_t x, int32_t y, int32_t z, bool center)
---@param x integer
---@param y integer
---@param z integer
---@param center boolean
---@return boolean
function dfhack.gui.revealInDwarfmodeMap(x, y, z, center) end

-- CXX SIGNATURE -> df::coord Gui::getMousePos()
---@return coord
function dfhack.gui.getMousePos() end

-- CXX SIGNATURE -> std::vector<std::string> Gui::getFocusStrings(df::viewscreen* top)
---@param top viewscreen
---@return string[]
function dfhack.gui.getFocusStrings(top) end

-- CXX SIGNATURE -> bool DFHack::Job::listNewlyCreated(std::vector<df::job*> *pvec, int *id_var)
---@param pvec job[]
---@param id_var integer
---@return boolean
function dfhack.job.listNewlyCreated(pvec, id_var) end

-- CXX SIGNATURE -> df::coord Units::getPosition(df::unit *unit)
---@param unit unit
---@return coord
function dfhack.units.getPosition(unit) end

-- CXX SIGNATURE -> void Units::getOuterContainerRef(df::specific_ref &spec_ref, df::unit *unit, bool init_ref)
---@param spec_ref specific_ref
---@param unit unit
---@param init_ref boolean
---@return nil
function dfhack.units.getOuterContainerRef(spec_ref, unit, init_ref) end

-- CXX SIGNATURE -> bool Units::getNoblePositions(std::vector<NoblePosition> *pvec, df::unit *unit)
---@param pvec NoblePosition[] -- unknown
---@param unit unit
---@return boolean
function dfhack.units.getNoblePositions(pvec, unit) end

-- CXX SIGNATURE -> bool Units::getUnitsInBox (std::vector<df::unit*> &units,    int16_t x1, int16_t y1, int16_t z1,    int16_t x2, int16_t y2, int16_t z2)
---@param units unit[]
---@param x1 integer
---@param y1 integer
---@param z1 integer
---@param x2 integer
---@param y2 integer
---@param z2 integer
---@return boolean
function dfhack.units.getUnitsInBox(units, x1, y1, z1, x2, y2, z2) end

-- CXX SIGNATURE -> bool Units::getCitizens(std::vector<df::unit *> &citizens, bool ignore_sanity)
---@param citizens unit[]
---@param ignore_sanity boolean
---@return boolean
function dfhack.units.getCitizens(citizens, ignore_sanity) end

-- CXX SIGNATURE -> df::coord Items::getPosition(df::item *item)
---@param item item
---@return coord
function dfhack.items.getPosition(item) end

-- CXX SIGNATURE -> void Items::getOuterContainerRef(df::specific_ref &spec_ref, df::item *item, bool init_ref)
---@param spec_ref specific_ref
---@param item item
---@param init_ref boolean
---@return nil
function dfhack.items.getOuterContainerRef(spec_ref, item, init_ref) end

-- CXX SIGNATURE -> void Items::getContainedItems(df::item *item, std::vector<df::item*> *items)
---@param item item
---@param items item[]
---@return nil
function dfhack.items.getContainedItems(item, items) end

-- CXX SIGNATURE -> bool DFHack::Items::moveToBuilding(MapExtras::MapCache &mc, df::item *item, df::building_actual *building,    int16_t use_mode, bool force_in_building)
---@param item item
---@param building building_actual
---@param use_mode integer
---@param force_in_building boolean
---@return boolean
function dfhack.items.moveToBuilding(item, building, use_mode, force_in_building) end

-- CXX SIGNATURE -> bool Maps::isValidTilePos(int32_t x, int32_t y, int32_t z)
---@param x integer
---@param y integer
---@param z integer
---@return boolean
function dfhack.maps.isValidTilePos(x, y, z) end

-- CXX SIGNATURE -> bool Maps::isTileVisible(int32_t x, int32_t y, int32_t z)
---@param x integer
---@param y integer
---@param z integer
---@return boolean
function dfhack.maps.isTileVisible(x, y, z) end

-- CXX SIGNATURE -> df::map_block *Maps::getTileBlock (int32_t x, int32_t y, int32_t z)
---@param x integer
---@param y integer
---@param z integer
---@return map_block
function dfhack.maps.getTileBlock(x, y, z) end

-- CXX SIGNATURE -> df::map_block *Maps::ensureTileBlock (int32_t x, int32_t y, int32_t z)
---@param x integer
---@param y integer
---@param z integer
---@return map_block
function dfhack.maps.ensureTileBlock(x, y, z) end

-- CXX SIGNATURE -> df::tiletype *Maps::getTileType(int32_t x, int32_t y, int32_t z)
---@param x integer
---@param y integer
---@param z integer
---@return tiletype
function dfhack.maps.getTileType(x, y, z) end

-- CXX SIGNATURE -> df::region_map_entry *Maps::getRegionBiome(df::coord2d rgn_pos)
---@param rgn_pos coord2d
---@return region_map_entry
function dfhack.maps.getRegionBiome(rgn_pos) end

-- CXX SIGNATURE -> df::plant *Maps::getPlantAtTile(int32_t x, int32_t y, int32_t z)
---@param x integer
---@param y integer
---@param z integer
---@return plant
function dfhack.maps.getPlantAtTile(x, y, z) end

-- CXX SIGNATURE -> df::enums::biome_type::biome_type Maps::getBiomeType(int world_coord_x, int world_coord_y)
---@param world_coord_x integer
---@param world_coord_y integer
---@return biome_type
function dfhack.maps.getBiomeType(world_coord_x, world_coord_y) end

-- CXX SIGNATURE -> void Burrows::listBlocks(std::vector<df::map_block*> *pvec, df::burrow *burrow)
---@param pvec map_block[]
---@param burrow burrow
---@return nil
function dfhack.burrows.listBlocks(pvec, burrow) end

-- CXX SIGNATURE -> bool Buildings::setSize(df::building *bld, df::coord2d size, int direction)
---@param bld building
---@param size coord2d
---@param direction integer
---@return boolean
function dfhack.buildings.setSize(bld, size, direction) end

-- CXX SIGNATURE -> void Buildings::getStockpileContents(df::building_stockpilest *stockpile, std::vector<df::item*> *items)
---@param stockpile building_stockpilest
---@param items item[]
---@return nil
function dfhack.buildings.getStockpileContents(stockpile, items) end

-- CXX SIGNATURE -> bool Buildings::getCageOccupants(df::building_cagest *cage, vector<df::unit*> &units)
---@param cage building_cagest
---@param units unit[]
---@return boolean
function dfhack.buildings.getCageOccupants(cage, units) end

-- CXX SIGNATURE -> df::building *Buildings::findAtTile(df::coord pos)
---@param pos coord
---@return building
function dfhack.buildings.findAtTile(pos) end

-- CXX SIGNATURE -> bool Buildings::findCivzonesAt(std::vector<df::building_civzonest*> *pvec,                               df::coord pos)
---@param pvec building_civzonest[]
---@param pos coord
---@return boolean
function dfhack.buildings.findCivzonesAt(pvec, pos) end

-- CXX SIGNATURE -> bool Buildings::getCorrectSize(df::coord2d &size, df::coord2d &center,                               df::building_type type, int subtype, int custom, int direction)
---@param size coord2d
---@param center coord2d
---@param type building_type
---@param subtype integer
---@param custom integer
---@param direction integer
---@return boolean
function dfhack.buildings.getCorrectSize(size, center, type, subtype, custom, direction) end

-- CXX SIGNATURE -> df::building* Buildings::findPenPitAt(df::coord coord)
---@param coord coord
---@return building
function dfhack.buildings.findPenPitAt(coord) end

-- CXX SIGNATURE -> bool Constructions::designateRemove(df::coord pos, bool *immediate)
---@param pos coord
---@param immediate boolean
---@return boolean
function dfhack.constructions.designateRemove(pos, immediate) end

-- CXX SIGNATURE -> df::construction * Constructions::findAtTile(df::coord pos)
---@param pos coord
---@return construction
function dfhack.constructions.findAtTile(pos) end

-- CXX SIGNATURE -> void Screen::raise(df::viewscreen *screen)
---@param screen viewscreen
---@return nil
function dfhack.screen.raise(screen) end

-- CXX SIGNATURE -> bool Screen::show(std::unique_ptr<df::viewscreen> screen, df::viewscreen *before, Plugin *plugin)
---@param screen viewscreen
---@param before viewscreen
---@param plugin Plugin -- unknown
---@return boolean
function dfhack.screen.show(screen, before, plugin) end

-- CXX SIGNATURE -> void Screen::dismiss(df::viewscreen *screen, bool to_first)
---@param screen viewscreen
---@param to_first boolean
---@return nil
function dfhack.screen.dismiss(screen, to_first) end

-- CXX SIGNATURE -> bool Screen::isDismissed(df::viewscreen *screen)
---@param screen viewscreen
---@return boolean
function dfhack.screen.isDismissed(screen) end

-- CXX SIGNATURE -> df::coord2d Screen::getMousePos()
---@return coord2d
function dfhack.screen.getMousePos() end

-- CXX SIGNATURE -> df::coord2d Screen::getMousePixels()
---@return coord2d
function dfhack.screen.getMousePixels() end

-- CXX SIGNATURE -> df::coord2d Screen::getWindowSize()
---@return coord2d
function dfhack.screen.getWindowSize() end

-- CXX SIGNATURE -> bool Screen::paintTile(const Pen &pen, int x, int y, bool map)
---@param pen Pen -- unknown
---@param x integer
---@param y integer
---@param map boolean
---@return boolean
function dfhack.screen.paintTile(pen, x, y, map) end

-- CXX SIGNATURE -> Pen Screen::readTile(int x, int y, bool map)
---@param x integer
---@param y integer
---@param map boolean
---@return Pen -- unknown
function dfhack.screen.readTile(x, y, map) end

-- CXX SIGNATURE -> bool Screen::paintString(const Pen &pen, int x, int y, const std::string &text, bool map)
---@param pen Pen -- unknown
---@param x integer
---@param y integer
---@param text string
---@param map boolean
---@return boolean
function dfhack.screen.paintString(pen, x, y, text, map) end

-- CXX SIGNATURE -> bool Screen::fillRect(const Pen &pen, int x1, int y1, int x2, int y2, bool map)
---@param pen Pen -- unknown
---@param x1 integer
---@param y1 integer
---@param x2 integer
---@param y2 integer
---@param map boolean
---@return boolean
function dfhack.screen.fillRect(pen, x1, y1, x2, y2, map) end

-- CXX SIGNATURE -> bool Screen::findGraphicsTile(const std::string &pagename, int x, int y, int *ptile, int *pgs)
---@param pagename string
---@param x integer
---@param y integer
---@param ptile integer
---@param pgs integer
---@return boolean
function dfhack.screen.findGraphicsTile(pagename, x, y, ptile, pgs) end

-- CXX SIGNATURE -> int Screen::keyToChar(df::interface_key key)
---@param key interface_key
---@return integer
function dfhack.screen.keyToChar(key) end

-- CXX SIGNATURE -> df::interface_key Screen::charToKey(char code)
---@param code string
---@return interface_key
function dfhack.screen.charToKey(code) end

-- CXX SIGNATURE -> void Screen::zoom(df::zoom_commands cmd)
---@param cmd zoom_commands
---@return nil
function dfhack.screen.zoom(cmd) end


-- Unknown types
---@alias unit_inventory_item__T_mode unknown
---@alias Plugin unknown
---@alias uint32_tdecod unknown
---@alias NoblePosition unknown
---@alias Gui__DwarfmodeDims unknown
---@alias Pen unknown

