// This is a generic plugin that does nothing useful apart from acting as an example... of a plugin that does nothing :D

// some headers required for a plugin. Nothing special, just the basics.
#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

#include <vector>
#include <algorithm>

#include "modules/Units.h"
#include "modules/World.h"

// DF data structure definition headers
#include "DataDefs.h"
#include <df/ui.h>
#include <df/world.h>
#include <df/unit.h>
#include <df/unit_soul.h>
#include <df/unit_labor.h>
#include <df/unit_skill.h>
#include <df/job.h>
#include <df/building.h>
#include <df/workshop_type.h>
#include <df/unit_misc_trait.h>
#include <df/entity_position_responsibility.h>
#include <df/historical_figure.h>
#include <df/historical_entity.h>
#include <df/histfig_entity_link.h>
#include <df/histfig_entity_link_positionst.h>
#include <df/entity_position_assignment.h>
#include <df/entity_position.h>
#include <df/building_tradedepotst.h>
#include <df/building_stockpilest.h>
#include <df/items_other_id.h>
#include <df/ui.h>
#include <df/activity_info.h>
#include <df/tile_dig_designation.h>
#include <df/item_weaponst.h>
#include <df/itemdef_weaponst.h>
#include <df/general_ref_unit_workerst.h>
#include <df/general_ref_building_holderst.h>
#include <df/building_workshopst.h>
#include <df/reaction.h>


#include <MiscUtils.h>

#include "modules/MapCache.h"
#include "modules/Items.h"

using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;
using df::global::ui;
using df::global::world;

#define ARRAY_COUNT(array) (sizeof(array)/sizeof((array)[0]))

/*
 * Autolabor module for dfhack
 *
 * The idea behind this module is to constantly adjust labors so that the right dwarves
 * are assigned to new tasks. The key is that, for almost all labors, once a dwarf begins
 * a job it will finish that job even if the associated labor is removed. Thus the
 * strategy is to frequently decide, for each labor, which dwarves should possibly take
 * a new job for that labor if it comes in and which shouldn't, and then set the labors
 * appropriately. The updating should happen as often as can be reasonably done without
 * causing lag.
 *
 * The obvious thing to do is to just set each labor on a single idle dwarf who is best
 * suited to doing new jobs of that labor. This works in a way, but it leads to a lot
 * of idle dwarves since only one dwarf will be dispatched for each labor in an update
 * cycle, and dwarves that finish tasks will wait for the next update before being
 * dispatched. An improvement is to also set some labors on dwarves that are currently
 * doing a job, so that they will immediately take a new job when they finish. The
 * details of which dwarves should have labors set is mostly a heuristic.
 *
 * A complication to the above simple scheme is labors that have associated equipment.
 * Enabling/disabling these labors causes dwarves to change equipment, and disabling
 * them in the middle of a job may cause the job to be abandoned. Those labors
 * (mining, hunting, and woodcutting) need to be handled carefully to minimize churn.
 */

static int enable_autolabor = 0;

static bool print_debug = 0;

static std::vector<int> state_count(5);

static PersistentDataItem config;

enum ConfigFlags {
    CF_ENABLED = 1,
};


// Here go all the command declarations...
// mostly to allow having the mandatory stuff on top of the file and commands on the bottom
command_result autolabor (color_ostream &out, std::vector <std::string> & parameters);

// A plugin must be able to return its name and version.
// The name string provided must correspond to the filename - autolabor.plug.so or autolabor.plug.dll in this case
DFHACK_PLUGIN("autolabor");

static void generate_labor_to_skill_map();

enum labor_mode {
    DISABLE,
    HAULERS,
    AUTOMATIC,
};

enum dwarf_state {
    // Ready for a new task
    IDLE,

    // Busy with a useful task
    BUSY,

    // In the military, can't work
    MILITARY,

    // Child or noble, can't work
    CHILD,

    // Doing something that precludes working, may be busy for a while
    OTHER
};

const int NUM_STATE = 5;

static const char *state_names[] = {
    "IDLE",
    "BUSY",
    "MILITARY",
    "CHILD",
    "OTHER",
};

static const dwarf_state dwarf_states[] = {
    BUSY /* CarveFortification */,
    BUSY /* DetailWall */,
    BUSY /* DetailFloor */,
    BUSY /* Dig */,
    BUSY /* CarveUpwardStaircase */,
    BUSY /* CarveDownwardStaircase */,
    BUSY /* CarveUpDownStaircase */,
    BUSY /* CarveRamp */,
    BUSY /* DigChannel */,
    BUSY /* FellTree */,
    BUSY /* GatherPlants */,
    BUSY /* RemoveConstruction */,
    BUSY /* CollectWebs */,
    BUSY /* BringItemToDepot */,
    BUSY /* BringItemToShop */,
    OTHER /* Eat */,
    OTHER /* GetProvisions */,
    OTHER /* Drink */,
    OTHER /* Drink2 */,
    OTHER /* FillWaterskin */,
    OTHER /* FillWaterskin2 */,
    OTHER /* Sleep */,
    BUSY /* CollectSand */,
    BUSY /* Fish */,
    BUSY /* Hunt */,
    OTHER /* HuntVermin */,
    BUSY /* Kidnap */,
    BUSY /* BeatCriminal */,
    BUSY /* StartingFistFight */,
    BUSY /* CollectTaxes */,
    BUSY /* GuardTaxCollector */,
    BUSY /* CatchLiveLandAnimal */,
    BUSY /* CatchLiveFish */,
    BUSY /* ReturnKill */,
    BUSY /* CheckChest */,
    BUSY /* StoreOwnedItem */,
    BUSY /* PlaceItemInTomb */,
    BUSY /* StoreItemInStockpile */,
    BUSY /* StoreItemInBag */,
    BUSY /* StoreItemInHospital */,
    BUSY /* StoreItemInChest */,
    BUSY /* StoreItemInCabinet */,
    BUSY /* StoreWeapon */,
    BUSY /* StoreArmor */,
    BUSY /* StoreItemInBarrel */,
    BUSY /* StoreItemInBin */,
    BUSY /* SeekArtifact */,
    BUSY /* SeekInfant */,
    OTHER /* AttendParty */,
    OTHER /* GoShopping */,
    OTHER /* GoShopping2 */,
    BUSY /* Clean */,
    OTHER /* Rest */,
    BUSY /* PickupEquipment */,
    BUSY /* DumpItem */,
    OTHER /* StrangeMoodCrafter */,
    OTHER /* StrangeMoodJeweller */,
    OTHER /* StrangeMoodForge */,
    OTHER /* StrangeMoodMagmaForge */,
    OTHER /* StrangeMoodBrooding */,
    OTHER /* StrangeMoodFell */,
    OTHER /* StrangeMoodCarpenter */,
    OTHER /* StrangeMoodMason */,
    OTHER /* StrangeMoodBowyer */,
    OTHER /* StrangeMoodTanner */,
    OTHER /* StrangeMoodWeaver */,
    OTHER /* StrangeMoodGlassmaker */,
    OTHER /* StrangeMoodMechanics */,
    BUSY /* ConstructBuilding */,
    BUSY /* ConstructDoor */,
    BUSY /* ConstructFloodgate */,
    BUSY /* ConstructBed */,
    BUSY /* ConstructThrone */,
    BUSY /* ConstructCoffin */,
    BUSY /* ConstructTable */,
    BUSY /* ConstructChest */,
    BUSY /* ConstructBin */,
    BUSY /* ConstructArmorStand */,
    BUSY /* ConstructWeaponRack */,
    BUSY /* ConstructCabinet */,
    BUSY /* ConstructStatue */,
    BUSY /* ConstructBlocks */,
    BUSY /* MakeRawGlass */,
    BUSY /* MakeCrafts */,
    BUSY /* MintCoins */,
    BUSY /* CutGems */,
    BUSY /* CutGlass */,
    BUSY /* EncrustWithGems */,
    BUSY /* EncrustWithGlass */,
    BUSY /* DestroyBuilding */,
    BUSY /* SmeltOre */,
    BUSY /* MeltMetalObject */,
    BUSY /* ExtractMetalStrands */,
    BUSY /* PlantSeeds */,
    BUSY /* HarvestPlants */,
    BUSY /* TrainHuntingAnimal */,
    BUSY /* TrainWarAnimal */,
    BUSY /* MakeWeapon */,
    BUSY /* ForgeAnvil */,
    BUSY /* ConstructCatapultParts */,
    BUSY /* ConstructBallistaParts */,
    BUSY /* MakeArmor */,
    BUSY /* MakeHelm */,
    BUSY /* MakePants */,
    BUSY /* StudWith */,
    BUSY /* ButcherAnimal */,
    BUSY /* PrepareRawFish */,
    BUSY /* MillPlants */,
    BUSY /* BaitTrap */,
    BUSY /* MilkCreature */,
    BUSY /* MakeCheese */,
    BUSY /* ProcessPlants */,
    BUSY /* ProcessPlantsBag */,
    BUSY /* ProcessPlantsVial */,
    BUSY /* ProcessPlantsBarrel */,
    BUSY /* PrepareMeal */,
    BUSY /* WeaveCloth */,
    BUSY /* MakeGloves */,
    BUSY /* MakeShoes */,
    BUSY /* MakeShield */,
    BUSY /* MakeCage */,
    BUSY /* MakeChain */,
    BUSY /* MakeFlask */,
    BUSY /* MakeGoblet */,
    BUSY /* MakeInstrument */,
    BUSY /* MakeToy */,
    BUSY /* MakeAnimalTrap */,
    BUSY /* MakeBarrel */,
    BUSY /* MakeBucket */,
    BUSY /* MakeWindow */,
    BUSY /* MakeTotem */,
    BUSY /* MakeAmmo */,
    BUSY /* DecorateWith */,
    BUSY /* MakeBackpack */,
    BUSY /* MakeQuiver */,
    BUSY /* MakeBallistaArrowHead */,
    BUSY /* AssembleSiegeAmmo */,
    BUSY /* LoadCatapult */,
    BUSY /* LoadBallista */,
    BUSY /* FireCatapult */,
    BUSY /* FireBallista */,
    BUSY /* ConstructMechanisms */,
    BUSY /* MakeTrapComponent */,
    BUSY /* LoadCageTrap */,
    BUSY /* LoadStoneTrap */,
    BUSY /* LoadWeaponTrap */,
    BUSY /* CleanTrap */,
    BUSY /* CastSpell */,
    BUSY /* LinkBuildingToTrigger */,
    BUSY /* PullLever */,
    BUSY /* BrewDrink */,
    BUSY /* ExtractFromPlants */,
    BUSY /* ExtractFromRawFish */,
    BUSY /* ExtractFromLandAnimal */,
    BUSY /* TameVermin */,
    BUSY /* TameAnimal */,
    BUSY /* ChainAnimal */,
    BUSY /* UnchainAnimal */,
    BUSY /* UnchainPet */,
    BUSY /* ReleaseLargeCreature */,
    BUSY /* ReleasePet */,
    BUSY /* ReleaseSmallCreature */,
    BUSY /* HandleSmallCreature */,
    BUSY /* HandleLargeCreature */,
    BUSY /* CageLargeCreature */,
    BUSY /* CageSmallCreature */,
    BUSY /* RecoverWounded */,
    BUSY /* DiagnosePatient */,
    BUSY /* ImmobilizeBreak */,
    BUSY /* DressWound */,
    BUSY /* CleanPatient */,
    BUSY /* Surgery */,
    BUSY /* Suture */,
    BUSY /* SetBone */,
    BUSY /* PlaceInTraction */,
    BUSY /* DrainAquarium */,
    BUSY /* FillAquarium */,
    BUSY /* FillPond */,
    BUSY /* GiveWater */,
    BUSY /* GiveFood */,
    BUSY /* GiveWater2 */,
    BUSY /* GiveFood2 */,
    BUSY /* RecoverPet */,
    BUSY /* PitLargeAnimal */,
    BUSY /* PitSmallAnimal */,
    BUSY /* SlaughterAnimal */,
    BUSY /* MakeCharcoal */,
    BUSY /* MakeAsh */,
    BUSY /* MakeLye */,
    BUSY /* MakePotashFromLye */,
    BUSY /* FertilizeField */,
    BUSY /* MakePotashFromAsh */,
    BUSY /* DyeThread */,
    BUSY /* DyeCloth */,
    BUSY /* SewImage */,
    BUSY /* MakePipeSection */,
    BUSY /* OperatePump */,
    OTHER /* ManageWorkOrders */,
    OTHER /* UpdateStockpileRecords */,
    OTHER /* TradeAtDepot */,
    BUSY /* ConstructHatchCover */,
    BUSY /* ConstructGrate */,
    BUSY /* RemoveStairs */,
    BUSY /* ConstructQuern */,
    BUSY /* ConstructMillstone */,
    BUSY /* ConstructSplint */,
    BUSY /* ConstructCrutch */,
    BUSY /* ConstructTractionBench */,
    BUSY /* CleanSelf */,
    BUSY /* BringCrutch */,
    BUSY /* ApplyCast */,
    BUSY /* CustomReaction */,
    BUSY /* ConstructSlab */,
    BUSY /* EngraveSlab */,
    BUSY /* ShearCreature */,
    BUSY /* SpinThread */,
    BUSY /* PenLargeAnimal */,
    BUSY /* PenSmallAnimal */,
    BUSY /* MakeTool */,
    BUSY /* CollectClay */,
    BUSY /* InstallColonyInHive */,
    BUSY /* CollectHiveProducts */,
    OTHER /* CauseTrouble */,
    OTHER /* DrinkBlood */,
    OTHER /* ReportCrime */,
    OTHER /* ExecuteCriminal */,
    BUSY /* TrainAnimal */,
    BUSY /* CarveTrack */,
    BUSY /* PushTrackVehicle */,
    BUSY /* PlaceTrackVehicle */,
    BUSY /* StoreItemInVehicle */
};

struct labor_info
{
    PersistentDataItem config;

    bool is_exclusive;
    int active_dwarfs;

    labor_mode mode() { return (labor_mode) config.ival(0); }
    void set_mode(labor_mode mode) { config.ival(0) = mode; }

    int minimum_dwarfs() { return config.ival(1); }
    void set_minimum_dwarfs(int minimum_dwarfs) { config.ival(1) = minimum_dwarfs; }

    int maximum_dwarfs() { return config.ival(2); }
    void set_maximum_dwarfs(int maximum_dwarfs) { config.ival(2) = maximum_dwarfs; }

};

struct labor_default
{
    labor_mode mode;
    bool is_exclusive;
    int minimum_dwarfs;
    int maximum_dwarfs;
    int active_dwarfs;
};

static int hauler_pct = 33;

static std::vector<struct labor_info> labor_infos;

static const struct labor_default default_labor_infos[] = {
    /* MINE */                  {AUTOMATIC, true, 2, 200, 0},
    /* HAUL_STONE */            {HAULERS, false, 1, 200, 0},
    /* HAUL_WOOD */             {HAULERS, false, 1, 200, 0},
    /* HAUL_BODY */             {HAULERS, false, 1, 200, 0},
    /* HAUL_FOOD */             {HAULERS, false, 1, 200, 0},
    /* HAUL_REFUSE */           {HAULERS, false, 1, 200, 0},
    /* HAUL_ITEM */             {HAULERS, false, 1, 200, 0},
    /* HAUL_FURNITURE */        {HAULERS, false, 1, 200, 0},
    /* HAUL_ANIMAL */           {HAULERS, false, 1, 200, 0},
    /* CLEAN */                 {HAULERS, false, 1, 200, 0},
    /* CUTWOOD */               {AUTOMATIC, true, 1, 200, 0},
    /* CARPENTER */             {AUTOMATIC, false, 1, 200, 0},
    /* DETAIL */                {AUTOMATIC, false, 1, 200, 0},
    /* MASON */                 {AUTOMATIC, false, 1, 200, 0},
    /* ARCHITECT */             {AUTOMATIC, false, 1, 200, 0},
    /* ANIMALTRAIN */           {AUTOMATIC, false, 1, 200, 0},
    /* ANIMALCARE */            {AUTOMATIC, false, 1, 200, 0},
    /* DIAGNOSE */              {AUTOMATIC, false, 1, 200, 0},
    /* SURGERY */               {AUTOMATIC, false, 1, 200, 0},
    /* BONE_SETTING */          {AUTOMATIC, false, 1, 200, 0},
    /* SUTURING */              {AUTOMATIC, false, 1, 200, 0},
    /* DRESSING_WOUNDS */       {AUTOMATIC, false, 1, 200, 0},
    /* FEED_WATER_CIVILIANS */  {AUTOMATIC, false, 200, 200, 0},
    /* RECOVER_WOUNDED */       {HAULERS, false, 1, 200, 0},
    /* BUTCHER */               {AUTOMATIC, false, 1, 200, 0},
    /* TRAPPER */               {AUTOMATIC, false, 1, 200, 0},
    /* DISSECT_VERMIN */        {AUTOMATIC, false, 1, 200, 0},
    /* LEATHER */               {AUTOMATIC, false, 1, 200, 0},
    /* TANNER */                {AUTOMATIC, false, 1, 200, 0},
    /* BREWER */                {AUTOMATIC, false, 1, 200, 0},
    /* ALCHEMIST */             {AUTOMATIC, false, 1, 200, 0},
    /* SOAP_MAKER */            {AUTOMATIC, false, 1, 200, 0},
    /* WEAVER */                {AUTOMATIC, false, 1, 200, 0},
    /* CLOTHESMAKER */          {AUTOMATIC, false, 1, 200, 0},
    /* MILLER */                {AUTOMATIC, false, 1, 200, 0},
    /* PROCESS_PLANT */         {AUTOMATIC, false, 1, 200, 0},
    /* MAKE_CHEESE */           {AUTOMATIC, false, 1, 200, 0},
    /* MILK */                  {AUTOMATIC, false, 1, 200, 0},
    /* COOK */                  {AUTOMATIC, false, 1, 200, 0},
    /* PLANT */                 {AUTOMATIC, false, 1, 200, 0},
    /* HERBALIST */             {AUTOMATIC, false, 1, 200, 0},
    /* FISH */                  {AUTOMATIC, false, 1, 1, 0},
    /* CLEAN_FISH */            {AUTOMATIC, false, 1, 200, 0},
    /* DISSECT_FISH */          {AUTOMATIC, false, 1, 200, 0},
    /* HUNT */                  {AUTOMATIC, true, 1, 1, 0},
    /* SMELT */                 {AUTOMATIC, false, 1, 200, 0},
    /* FORGE_WEAPON */          {AUTOMATIC, false, 1, 200, 0},
    /* FORGE_ARMOR */           {AUTOMATIC, false, 1, 200, 0},
    /* FORGE_FURNITURE */       {AUTOMATIC, false, 1, 200, 0},
    /* METAL_CRAFT */           {AUTOMATIC, false, 1, 200, 0},
    /* CUT_GEM */               {AUTOMATIC, false, 1, 200, 0},
    /* ENCRUST_GEM */           {AUTOMATIC, false, 1, 200, 0},
    /* WOOD_CRAFT */            {AUTOMATIC, false, 1, 200, 0},
    /* STONE_CRAFT */           {AUTOMATIC, false, 1, 200, 0},
    /* BONE_CARVE */            {AUTOMATIC, false, 1, 200, 0},
    /* GLASSMAKER */            {AUTOMATIC, false, 1, 200, 0},
    /* EXTRACT_STRAND */        {AUTOMATIC, false, 1, 200, 0},
    /* SIEGECRAFT */            {AUTOMATIC, false, 1, 200, 0},
    /* SIEGEOPERATE */          {AUTOMATIC, false, 1, 200, 0},
    /* BOWYER */                {AUTOMATIC, false, 1, 200, 0},
    /* MECHANIC */              {AUTOMATIC, false, 1, 200, 0},
    /* POTASH_MAKING */         {AUTOMATIC, false, 1, 200, 0},
    /* LYE_MAKING */            {AUTOMATIC, false, 1, 200, 0},
    /* DYER */                  {AUTOMATIC, false, 1, 200, 0},
    /* BURN_WOOD */             {AUTOMATIC, false, 1, 200, 0},
    /* OPERATE_PUMP */          {AUTOMATIC, false, 1, 200, 0},
    /* SHEARER */               {AUTOMATIC, false, 1, 200, 0},
    /* SPINNER */               {AUTOMATIC, false, 1, 200, 0},
    /* POTTERY */               {AUTOMATIC, false, 1, 200, 0},
    /* GLAZING */               {AUTOMATIC, false, 1, 200, 0},
    /* PRESSING */              {AUTOMATIC, false, 1, 200, 0},
    /* BEEKEEPING */            {AUTOMATIC, false, 1, 1, 0}, // reduce risk of stuck beekeepers (see http://www.bay12games.com/dwarves/mantisbt/view.php?id=3981)
    /* WAX_WORKING */           {AUTOMATIC, false, 1, 200, 0},
    /* PUSH_HAUL_VEHICLES */    {HAULERS, false, 1, 200, 0}
};

static const int responsibility_penalties[] = {
    0,      /* LAW_MAKING */
    0,      /* LAW_ENFORCEMENT */
    3000,   /* RECEIVE_DIPLOMATS */
    0,      /* MEET_WORKERS */
    1000,   /* MANAGE_PRODUCTION */
    3000,   /* TRADE */
    1000,   /* ACCOUNTING */
    0,      /* ESTABLISH_COLONY_TRADE_AGREEMENTS */
    0,      /* MAKE_INTRODUCTIONS */
    0,      /* MAKE_PEACE_AGREEMENTS */
    0,      /* MAKE_TOPIC_AGREEMENTS */
    0,      /* COLLECT_TAXES */
    0,      /* ESCORT_TAX_COLLECTOR */
    0,      /* EXECUTIONS */
    0,      /* TAME_EXOTICS */
    0,      /* RELIGION */
    0,      /* ATTACK_ENEMIES */
    0,      /* PATROL_TERRITORY */
    0,      /* MILITARY_GOALS */
    0,      /* MILITARY_STRATEGY */
    0,      /* UPGRADE_SQUAD_EQUIPMENT */
    0,      /* EQUIPMENT_MANIFESTS */
    0,      /* SORT_AMMUNITION */
    0,      /* BUILD_MORALE */
    5000    /* HEALTH_MANAGEMENT */
};

struct dwarf_info_t
{
	df::unit* dwarf;
    dwarf_state state;

	bool clear_all;

	bool has_axe;
	bool has_pick;
	bool has_crossbow;

	dwarf_info_t(df::unit* dw) : dwarf(dw), clear_all(0), has_axe(0), has_pick(0), has_crossbow(0), state(OTHER) {}

	void set_labor(df::unit_labor labor) 
	{
		dwarf->status.labors[labor] = true;
		if ((labor == df::unit_labor::MINE    && !has_pick) ||
			(labor == df::unit_labor::CUTWOOD && !has_axe) ||
			(labor == df::unit_labor::HUNT    && !has_crossbow))
			dwarf->military.pickup_flags.bits.update = 1;
	}
};

static bool isOptionEnabled(unsigned flag)
{
    return config.isValid() && (config.ival(0) & flag) != 0;
}

static void setOptionEnabled(ConfigFlags flag, bool on)
{
    if (!config.isValid())
        return;

    if (on)
        config.ival(0) |= flag;
    else
        config.ival(0) &= ~flag;
}

static void cleanup_state()
{
    labor_infos.clear();
}

static void reset_labor(df::unit_labor labor)
{
    labor_infos[labor].set_minimum_dwarfs(default_labor_infos[labor].minimum_dwarfs);
    labor_infos[labor].set_maximum_dwarfs(default_labor_infos[labor].maximum_dwarfs);
    labor_infos[labor].set_mode(default_labor_infos[labor].mode);
}

static void init_state()
{
    config = World::GetPersistentData("autolabor/config");
    if (config.isValid() && config.ival(0) == -1)
        config.ival(0) = 0;

    enable_autolabor = isOptionEnabled(CF_ENABLED);

    if (!enable_autolabor)
        return;

    auto cfg_haulpct = World::GetPersistentData("autolabor/haulpct");
    if (cfg_haulpct.isValid())
    {
        hauler_pct = cfg_haulpct.ival(0);
    }
    else
    {
        hauler_pct = 33;
    }

    // Load labors from save
    labor_infos.resize(ARRAY_COUNT(default_labor_infos));

    std::vector<PersistentDataItem> items;
    World::GetPersistentData(&items, "autolabor/labors/", true);

    for (auto p = items.begin(); p != items.end(); p++)
    {
        string key = p->key();
        df::unit_labor labor = (df::unit_labor) atoi(key.substr(strlen("autolabor/labors/")).c_str());
        if (labor >= 0 && labor <= labor_infos.size())
        {
            labor_infos[labor].config = *p;
            labor_infos[labor].is_exclusive = default_labor_infos[labor].is_exclusive;
            labor_infos[labor].active_dwarfs = 0;
        }
    }

    // Add default labors for those not in save
    for (int i = 0; i < ARRAY_COUNT(default_labor_infos); i++) {
        if (labor_infos[i].config.isValid())
            continue;

        std::stringstream name;
        name << "autolabor/labors/" << i;

        labor_infos[i].config = World::AddPersistentData(name.str());

        labor_infos[i].is_exclusive = default_labor_infos[i].is_exclusive;
        labor_infos[i].active_dwarfs = 0;
        reset_labor((df::unit_labor) i);
    }

    generate_labor_to_skill_map();

}

static df::job_skill labor_to_skill[ENUM_LAST_ITEM(unit_labor) + 1];

static void generate_labor_to_skill_map()
{
    // Generate labor -> skill mapping

    for (int i = 0; i <= ENUM_LAST_ITEM(unit_labor); i++)
        labor_to_skill[i] = job_skill::NONE;

    FOR_ENUM_ITEMS(job_skill, skill)
    {
        int labor = ENUM_ATTR(job_skill, labor, skill);
        if (labor != unit_labor::NONE)
        {
            /*
            assert(labor >= 0);
            assert(labor < ARRAY_COUNT(labor_to_skill));
            */

            labor_to_skill[labor] = skill;
        }
    }

}


static void enable_plugin(color_ostream &out)
{
    if (!config.isValid())
    {
        config = World::AddPersistentData("autolabor/config");
        config.ival(0) = 0;
    }

    setOptionEnabled(CF_ENABLED, true);
    enable_autolabor = true;
    out << "Enabling the plugin." << endl;

    cleanup_state();
    init_state();
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    // initialize labor infos table from default table
    if(ARRAY_COUNT(default_labor_infos) != ENUM_LAST_ITEM(unit_labor) + 1)
        return CR_FAILURE;

    // Fill the command list with your commands.
    commands.push_back(PluginCommand(
        "autolabor", "Automatically manage dwarf labors.",
        autolabor, false, /* true means that the command can't be used from non-interactive user interface */
        // Extended help string. Used by CR_WRONG_USAGE and the help command:
        "  autolabor enable\n"
        "  autolabor disable\n"
        "    Enables or disables the plugin.\n"
        "  autolabor <labor> <minimum> [<maximum>]\n"
        "    Set number of dwarves assigned to a labor.\n"
        "  autolabor <labor> haulers\n"
        "    Set a labor to be handled by hauler dwarves.\n"
        "  autolabor <labor> disable\n"
        "    Turn off autolabor for a specific labor.\n"
        "  autolabor <labor> reset\n"
        "    Return a labor to the default handling.\n"
        "  autolabor reset-all\n"
        "    Return all labors to the default handling.\n"
        "  autolabor list\n"
        "    List current status of all labors.\n"
        "  autolabor status\n"
        "    Show basic status information.\n"
        "Function:\n"
        "  When enabled, autolabor periodically checks your dwarves and enables or\n"
        "  disables labors. It tries to keep as many dwarves as possible busy but\n"
        "  also tries to have dwarves specialize in specific skills.\n"
        "  Warning: autolabor will override any manual changes you make to labors\n"
        "  while it is enabled.\n"
        "Examples:\n"
        "  autolabor MINE 2\n"
        "    Keep at least 2 dwarves with mining enabled.\n"
        "  autolabor CUT_GEM 1 1\n"
        "    Keep exactly 1 dwarf with gemcutting enabled.\n"
        "  autolabor FEED_WATER_CIVILIANS haulers\n"
        "    Have haulers feed and water wounded dwarves.\n"
        "  autolabor CUTWOOD disable\n"
        "    Turn off autolabor for wood cutting.\n"
    ));

    init_state();

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    cleanup_state();

    return CR_OK;
}


static df::building* get_building_from_job(df::job* j)
{
	for (auto r = j->references.begin(); r != j->references.end(); r++)
	{
		if ((*r)->getType() == df::general_ref_type::BUILDING_HOLDER)
		{
			int32_t id = ((df::general_ref_building_holderst*)(*r))->building_id;
			df::building* bld = binsearch_in_vector(world->buildings.all, id);
			return bld;
		}
	}
	return 0;
}

static df::job_skill workshop_build_labor[] = 
{
	/* Carpenters */		df::job_skill::CARPENTRY,
	/* Farmers */			df::job_skill::PROCESSPLANTS,
    /* Masons */			df::job_skill::MASONRY,
	/* Craftsdwarfs */		df::job_skill::STONECRAFT,
	/* Jewelers */			df::job_skill::CUTGEM,
	/* MetalsmithsForge */	df::job_skill::METALCRAFT,
	/* MagmaForge */		df::job_skill::METALCRAFT,
	/* Bowyers */			df::job_skill::BOWYER,
	/* Mechanics */			df::job_skill::MECHANICS,
	/* Siege */				df::job_skill::SIEGECRAFT,
	/* Butchers */			df::job_skill::BUTCHER,
	/* Leatherworks */		df::job_skill::LEATHERWORK,
	/* Tanners */			df::job_skill::TANNER,
	/* Clothiers */			df::job_skill::CLOTHESMAKING,
	/* Fishery */			df::job_skill::FISH,
	/* Still */				df::job_skill::BREWING,
	/* Loom */				df::job_skill::WEAVING,
	/* Quern */				df::job_skill::MILLING,
	/* Kennels */			df::job_skill::ANIMALTRAIN,
	/* Kitchen */			df::job_skill::COOK,
	/* Ashery */			df::job_skill::LYE_MAKING,
	/* Dyers */				df::job_skill::DYER,
	/* Millstone */			df::job_skill::MILLING,
	/* Custom */			(df::job_skill) -1,
	/* Tool */				(df::job_skill) -1
};

static void find_job_skill_labor(df::job* j, df::job_skill &skill, df::unit_labor &labor)
{

	switch (j->job_type) 
	{
	case df::job_type::ConstructBuilding:
		{
			df::building* bld = get_building_from_job (j);
			switch (bld->getType()) 
			{
			case df::building_type::Workshop:
				df::building_workshopst* ws = (df::building_workshopst*) bld;
				skill = workshop_build_labor[ws->type];
				break;
			}
		}
		break;
	case df::job_type::CustomReaction:
		for (auto r = world->raws.reactions.begin(); r != world->raws.reactions.end(); r++)
		{
			if ((*r)->code == j->reaction_name)
			{
				skill = (*r)->skill;
				break;
			}
		}
		break;
	default:
		skill = ENUM_ATTR(job_type, skill, j->job_type);
	} 

	if (skill != df::job_skill::NONE)
		labor = ENUM_ATTR(job_skill, labor, skill);

	if (labor == df::unit_labor::NONE)
		labor = ENUM_ATTR(job_type, labor, j->job_type);

	if (labor == -1) 
	{
	}

}

class AutoLaborManager {
	color_ostream& out;

public: 
	AutoLaborManager(color_ostream& o) : out(o)
	{
	}

	~AutoLaborManager()
	{
		for (std::vector<dwarf_info_t*>::iterator i = dwarf_info.begin(); 
			i != dwarf_info.end(); i++)
			delete (*i);
	}

	dwarf_info_t* add_dwarf(df::unit* u)
	{
		dwarf_info_t* dwarf = new dwarf_info_t(u);
		dwarf_info.push_back(dwarf);
		return dwarf;
	}


private:
	bool has_butchers;
	bool has_fishery;
	bool trader_requested;

	int dig_count;
	int tree_count;
	int plant_count;
	int detail_count;
	int pick_count;
	int axe_count;

	std::vector<df::job*> jobs;
	std::vector<dwarf_info_t*> dwarf_info;
	std::deque<dwarf_info_t*> idle_dwarfs;

private:
	void scan_buildings()
	{
		for (auto b = world->buildings.all.begin(); b != world->buildings.all.end(); b++)
		{
			df::building *build = *b;
			auto type = build->getType();
			if (building_type::Workshop == type)
			{
				df::workshop_type subType = (df::workshop_type)build->getSubtype();
				if (workshop_type::Butchers == subType)
					has_butchers = true;
				if (workshop_type::Fishery == subType)
					has_fishery = true;
			}
			else if (building_type::TradeDepot == type)
			{
				df::building_tradedepotst* depot = (df::building_tradedepotst*) build;
				trader_requested = depot->trade_flags.bits.trader_requested;
				if (print_debug)
				{
					if (trader_requested)
						out.print("Trade depot found and trader requested, trader will be excluded from all labors.\n");
					else
						out.print("Trade depot found but trader is not requested.\n");
				}
			}
		}
	}

	void count_map_designations()
	{
		dig_count = 0;
		tree_count = 0; 
		plant_count = 0;
		detail_count = 0;

		for (int i = 0; i < world->map.map_blocks.size(); ++i)
		{
			df::map_block* bl = world->map.map_blocks[i];

			if (!bl->flags.bits.designated)
				continue;

			if (print_debug)
				out.print ("block with designations found: %d, %d, %d\n", bl->map_pos.x, bl->map_pos.y, bl->map_pos.z);

			for (int x = 0; x < 16; x++)
				for (int y = 0; y < 16; y++)
				{
					df::tile_dig_designation dig = bl->designation[x][y].bits.dig;
					if (dig != df::enums::tile_dig_designation::No) 
					{
						df::tiletype tt = bl->tiletype[x][y];
						df::tiletype_shape tts = ENUM_ATTR(tiletype, shape, tt);
						switch (tts) 
						{
						case df::enums::tiletype_shape::TREE:
							tree_count++; break;
						case df::enums::tiletype_shape::SHRUB:
							plant_count++; break;
						default:
							dig_count++; break;
						}
					}
					if (bl->designation[x][y].bits.smooth != 0)
						detail_count++;
				}
		}

		if (print_debug)
			out.print("Dig count = %d, Cut tree count = %d, gather plant count = %d, detail count = %d\n", dig_count, tree_count, plant_count, detail_count);

	}

	void count_tools()
	{
		pick_count = 0;
		axe_count = 0;

		df::item_flags bad_flags;
		bad_flags.whole = 0;

#define F(x) bad_flags.bits.x = true;
		F(dump); F(forbid); F(garbage_collect);
		F(hostile); F(on_fire); F(rotten); F(trader);
		F(in_building); F(construction); F(artifact1);
#undef F

		for (int i = 0; i < world->items.all.size(); ++i)
		{
			df::item* item = world->items.all[i];
			if (item->flags.whole & bad_flags.whole)
				continue;
	
			if (!item->isWeapon()) 
				continue;

			df::itemdef_weaponst* weapondef = ((df::item_weaponst*)item)->subtype;
			df::job_skill weaponsk = (df::job_skill) weapondef->skill_melee;
			if (weaponsk == df::job_skill::AXE)
				axe_count++;
			else if (weaponsk == df::job_skill::MINING)
				pick_count++;
		}

		if (print_debug)
			out.print("Axes = %d, picks = %d\n", axe_count, pick_count);

	}

	void collect_job_list()
	{
		std::vector<df::job*> repeating_jobs;
		

		for (df::job_list_link* jll = world->job_list.next; jll; jll = jll->next)
		{
			df::job* j = jll->item;
			if (!j) 
				continue;

			if (j->flags.bits.suspend)
				continue;

			int worker = -1;

			for (int r = 0; r < j->references.size(); ++r)
				if (j->references[r]->getType() == df::general_ref_type::UNIT_WORKER)
					worker = ((df::general_ref_unit_workerst *)(j->references[r]))->unit_id;

			if (worker != -1)
				continue;

			if (j->flags.bits.repeat)
				repeating_jobs.push_back(j);
			else
				jobs.push_back(j);
		}

		if (print_debug)
			out.print("%d repeating jobs, %d nonrepeating jobs\n", repeating_jobs.size(), jobs.size());

		for (int i = 0; i < repeating_jobs.size(); i++) 
			jobs.push_back(repeating_jobs[i]);

	}

	void collect_dwarf_list()
	{

		for (auto u = world->units.active.begin(); u != world->units.active.end(); ++u)
		{
			df::unit* cre = *u;

			if (Units::isCitizen(cre))
			{
				if (cre->burrows.size() > 0)
					continue;        // dwarfs assigned to burrows are skipped entirely

				dwarf_info_t* dwarf = add_dwarf(cre);

				df::historical_figure* hf = df::historical_figure::find(dwarf->dwarf->hist_figure_id);
				for (int i = 0; i < hf->entity_links.size(); i++)
				{
					df::histfig_entity_link* hfelink = hf->entity_links.at(i);
					if (hfelink->getType() == df::histfig_entity_link_type::POSITION)
					{
						df::histfig_entity_link_positionst *epos =
							(df::histfig_entity_link_positionst*) hfelink;
						df::historical_entity* entity = df::historical_entity::find(epos->entity_id);
						if (!entity)
							continue;
						df::entity_position_assignment* assignment = binsearch_in_vector(entity->positions.assignments, epos->assignment_id);
						if (!assignment)
							continue;
						df::entity_position* position = binsearch_in_vector(entity->positions.own, assignment->position_id);
						if (!position)
							continue;

						if (position->responsibilities[df::entity_position_responsibility::TRADE])
							if (trader_requested)
								dwarf->clear_all = true;
					}

				}

				// identify dwarfs who are needed for meetings and mark them for exclusion

				for (int i = 0; i < ui->activities.size(); ++i)
				{
					df::activity_info *act = ui->activities[i];
					if (!act) continue;
					bool p1 = act->person1 == dwarf->dwarf;
					bool p2 = act->person2 == dwarf->dwarf;

					if (p1 || p2)
					{
						dwarf->clear_all = true;
						if (print_debug)
							out.print("Dwarf \"%s\" has a meeting, will be cleared of all labors\n", dwarf->dwarf->name.first_name.c_str());
						break;
					}
				}

				// Find the activity state for each dwarf-> 

				bool is_on_break = false;
				dwarf_state state = OTHER;

				for (auto p = dwarf->dwarf->status.misc_traits.begin(); p < dwarf->dwarf->status.misc_traits.end(); p++)
				{
					if ((*p)->id == misc_trait_type::Migrant || (*p)->id == misc_trait_type::OnBreak)
						is_on_break = true;
				}

				if (dwarf->dwarf->profession == profession::BABY ||
					dwarf->dwarf->profession == profession::CHILD ||
					dwarf->dwarf->profession == profession::DRUNK)
				{
					state = CHILD;
				}
				else if (ENUM_ATTR(profession, military, dwarf->dwarf->profession))
					state = MILITARY;
				else if (dwarf->dwarf->job.current_job == NULL)
				{
					if (is_on_break)
						state = OTHER;
					else if (dwarf->dwarf->specific_refs.size() > 0)
						state = OTHER;
					else
						state = IDLE;
				}
				else
				{
					int job = dwarf->dwarf->job.current_job->job_type;
					if (job >= 0 && job < ARRAY_COUNT(dwarf_states))
						state = dwarf_states[job];
					else
					{
						out.print("Dwarf \"%s\" has unknown job %i\n", dwarf->dwarf->name.first_name.c_str(), job);
						state = OTHER;
					}
				}

				dwarf->state = state;

				if (print_debug)
					out.print("Dwarf \"%s\": state %s\n", dwarf->dwarf->name.first_name.c_str(), state_names[dwarf->state]);

				// check if dwarf has an axe, pick, or crossbow

				for (int j = 0; j < dwarf->dwarf->inventory.size(); j++)
				{
					df::unit_inventory_item* ui = dwarf->dwarf->inventory[j];
					if (ui->mode == df::unit_inventory_item::Weapon && ui->item->isWeapon())
					{
						df::itemdef_weaponst* weapondef = ((df::item_weaponst*)(ui->item))->subtype;
						df::job_skill weaponsk = (df::job_skill) weapondef->skill_melee;
						df::job_skill rangesk = (df::job_skill) weapondef->skill_ranged;
						if (weaponsk == df::job_skill::AXE)
						{
							dwarf->has_axe = 1;
							if (state != IDLE) 
								axe_count--;
							if (print_debug)
								out.print("Dwarf \"%s\" has an axe\n", dwarf->dwarf->name.first_name.c_str());
						}
						else if (weaponsk == df::job_skill::MINING)
						{
							dwarf->has_pick = 1;
							if (state != IDLE) 
								pick_count--;
							if (print_debug)
								out.print("Dwarf \"%s\" has an pick\n", dwarf->dwarf->name.first_name.c_str());
						}
						else if (rangesk == df::job_skill::CROSSBOW)
						{
							dwarf->has_crossbow = 1;
							if (print_debug)
								out.print("Dwarf \"%s\" has a crossbow\n", dwarf->dwarf->name.first_name.c_str());
						}
					}
				}

				// clear labors if currently idle

				if (state == IDLE || dwarf->clear_all) 
				{
					FOR_ENUM_ITEMS(unit_labor, labor)
					{
						if (labor == unit_labor::NONE) 
							continue;

						dwarf->dwarf->status.labors[labor] = false;
					}
				}

				if (state == IDLE && !dwarf->clear_all)
					idle_dwarfs.push_back(dwarf);

			}

		}
	}

	void assign_one_dwarf (df::job_skill skill, df::unit_labor labor)
	{
		if (labor == df::unit_labor::NONE)
			return;

		std::deque<dwarf_info_t*>::iterator bestdwarf = idle_dwarfs.begin();

		if (skill != df::job_skill::NONE) 
		{
			int best_skill_level = -1;

			for (std::deque<dwarf_info_t*>::iterator k = idle_dwarfs.begin(); k != idle_dwarfs.end(); k++)
			{
				dwarf_info_t* d = (*k);
				int	skill_level = Units::getEffectiveSkill(d->dwarf, skill);

				if (skill_level > best_skill_level) 
				{
					bestdwarf = k;
					best_skill_level = skill_level;
				}
			}
		}

		if (print_debug)
			out.print("assign \"%s\" labor %d\n", (*bestdwarf)->dwarf->name.first_name.c_str(), labor);
		(*bestdwarf)->set_labor(labor);

		idle_dwarfs.erase(bestdwarf);
	}

public:
	void process()
	{
		// scan for specific buildings of interest

		scan_buildings();

		// count number of squares designated for dig, wood cutting, detailing, and plant harvesting

		count_map_designations();

		// count number of picks and axes available for use

		count_tools();

		// collect current job list 

		collect_job_list();

		// collect list of dwarfs

		collect_dwarf_list();

		// assign jobs requiring skill to idle dwarfs

		int jobs_to_assign = jobs.size();
		if (jobs_to_assign > idle_dwarfs.size()) 
			jobs_to_assign = idle_dwarfs.size();

		if (print_debug)
			out.print ("Assign idle (skilled): %d\n", jobs_to_assign);

		std::vector<df::job*> jobs2;

		for (int i = 0; i < jobs_to_assign; ++i)
		{
			df::job* j = jobs[i];

			df::job_skill skill;
			df::unit_labor labor;

			find_job_skill_labor(j, skill, labor);

			if(print_debug)
				out.print("Job skill = %d labor = %d\n", skill, labor);

			if (labor == -1)
				out.print("Invalid labor for job (%d)\n", j->job_type);

			if (skill == df::job_skill::NONE)
				jobs2.push_back(j);
			else 
				assign_one_dwarf(skill, labor);
		}

		// assign mining jobs to idle dwarfs

		int dig_max = dig_count;
		if (dig_max > pick_count) 
			dig_max = pick_count; 

		jobs_to_assign = dig_max - jobs2.size();
		if (jobs_to_assign > idle_dwarfs.size()) 
			jobs_to_assign = idle_dwarfs.size();

		if (print_debug)
			out.print ("Assign idle (mining): %d\n", jobs_to_assign);

		for (int i = 0; i < jobs_to_assign; ++i)
		{
			df::job_skill skill = df::job_skill::MINING;
			df::unit_labor labor = df::unit_labor::MINE;

			assign_one_dwarf(skill, labor);
		}

		// now assign unskilled jobs to idle dwarfs

		jobs_to_assign = jobs2.size();
		if (jobs_to_assign > idle_dwarfs.size()) 
			jobs_to_assign = idle_dwarfs.size();

		if (print_debug)
			out.print ("Assign idle (unskilled): %d\n", jobs_to_assign);

		for (int i = 0; i < jobs_to_assign; ++i)
		{
			df::job* j = jobs2[i];

			df::job_skill skill;
			df::unit_labor labor;

			find_job_skill_labor(j, skill, labor);
			assign_one_dwarf(skill, labor);
		}

		print_debug = 0;

		}

};


DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        cleanup_state();
        init_state();
        break;
    case SC_MAP_UNLOADED:
        cleanup_state();
        break;
    default:
        break;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    static int step_count = 0;
    // check run conditions
    if(!world || !world->map.block_index || !enable_autolabor)
    {
        // give up if we shouldn't be running'
        return CR_OK;
    }

    if (++step_count < 60)
        return CR_OK;
    step_count = 0;

	AutoLaborManager alm(out);

	alm.process();


    return CR_OK;
}

void print_labor (df::unit_labor labor, color_ostream &out)
{
    string labor_name = ENUM_KEY_STR(unit_labor, labor);
    out << labor_name << ": ";
    for (int i = 0; i < 20 - (int)labor_name.length(); i++)
        out << ' ';
    if (labor_infos[labor].mode() == DISABLE)
        out << "disabled" << endl;
    else
    {
        if (labor_infos[labor].mode() == HAULERS)
            out << "haulers";
        else
            out << "minimum " << labor_infos[labor].minimum_dwarfs() << ", maximum " << labor_infos[labor].maximum_dwarfs();
        out << ", currently " << labor_infos[labor].active_dwarfs << " dwarfs" << endl;
    }
}


command_result autolabor (color_ostream &out, std::vector <std::string> & parameters)
{
    CoreSuspender suspend;

    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("World is not loaded: please load a game first.\n");
        return CR_FAILURE;
    }

    if (parameters.size() == 1 &&
        (parameters[0] == "0" || parameters[0] == "enable" ||
         parameters[0] == "1" || parameters[0] == "disable"))
    {
        bool enable = (parameters[0] == "1" || parameters[0] == "enable");
        if (enable && !enable_autolabor)
        {
            enable_plugin(out);
        }
        else if(!enable && enable_autolabor)
        {
            enable_autolabor = false;
            setOptionEnabled(CF_ENABLED, false);

            out << "The plugin is disabled." << endl;
        }

        return CR_OK;
    }
    else if (parameters.size() == 2 && parameters[0] == "haulpct")
    {
        if (!enable_autolabor)
        {
            out << "Error: The plugin is not enabled." << endl;
            return CR_FAILURE;
        }

        int pct = atoi (parameters[1].c_str());
        hauler_pct = pct;
        return CR_OK;
    }
    else if (parameters.size() == 2 || parameters.size() == 3)
    {
        if (!enable_autolabor)
        {
            out << "Error: The plugin is not enabled." << endl;
            return CR_FAILURE;
        }

        df::unit_labor labor = unit_labor::NONE;

        FOR_ENUM_ITEMS(unit_labor, test_labor)
        {
            if (parameters[0] == ENUM_KEY_STR(unit_labor, test_labor))
                labor = test_labor;
        }

        if (labor == unit_labor::NONE)
        {
            out.printerr("Could not find labor %s.\n", parameters[0].c_str());
            return CR_WRONG_USAGE;
        }

        if (parameters[1] == "haulers")
        {
            labor_infos[labor].set_mode(HAULERS);
            print_labor(labor, out);
            return CR_OK;
        }
        if (parameters[1] == "disable")
        {
            labor_infos[labor].set_mode(DISABLE);
            print_labor(labor, out);
            return CR_OK;
        }
        if (parameters[1] == "reset")
        {
            reset_labor(labor);
            print_labor(labor, out);
            return CR_OK;
        }

        int minimum = atoi (parameters[1].c_str());
        int maximum = 200;
        if (parameters.size() == 3)
            maximum = atoi (parameters[2].c_str());

        if (maximum < minimum || maximum < 0 || minimum < 0)
        {
            out.printerr("Syntax: autolabor <labor> <minimum> [<maximum>]\n", maximum, minimum);
            return CR_WRONG_USAGE;
        }

        labor_infos[labor].set_minimum_dwarfs(minimum);
        labor_infos[labor].set_maximum_dwarfs(maximum);
        labor_infos[labor].set_mode(AUTOMATIC);
        print_labor(labor, out);

        return CR_OK;
    }
    else if (parameters.size() == 1 && parameters[0] == "reset-all")
    {
        if (!enable_autolabor)
        {
            out << "Error: The plugin is not enabled." << endl;
            return CR_FAILURE;
        }

        for (int i = 0; i < labor_infos.size(); i++)
        {
            reset_labor((df::unit_labor) i);
        }
        out << "All labors reset." << endl;
        return CR_OK;
    }
    else if (parameters.size() == 1 && parameters[0] == "list" || parameters[0] == "status")
    {
        if (!enable_autolabor)
        {
            out << "Error: The plugin is not enabled." << endl;
            return CR_FAILURE;
        }

        bool need_comma = 0;
        for (int i = 0; i < NUM_STATE; i++)
        {
            if (state_count[i] == 0)
                continue;
            if (need_comma)
                out << ", ";
            out << state_count[i] << ' ' << state_names[i];
            need_comma = 1;
        }
        out << endl;

        if (parameters[0] == "list")
        {
            FOR_ENUM_ITEMS(unit_labor, labor)
            {
                if (labor == unit_labor::NONE)
                    continue;

                print_labor(labor, out);
            }
        }

        return CR_OK;
    }
    else if (parameters.size() == 1 && parameters[0] == "debug")
    {
        if (!enable_autolabor)
        {
            out << "Error: The plugin is not enabled." << endl;
            return CR_FAILURE;
        }

        print_debug = 1;

        return CR_OK;
    }
    else
    {
        out.print("Automatically assigns labors to dwarves.\n"
            "Activate with 'autolabor 1', deactivate with 'autolabor 0'.\n"
            "Current state: %d.\n", enable_autolabor);

        return CR_OK;
    }
}

