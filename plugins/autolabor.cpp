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

#include <MiscUtils.h>

#include "modules/MapCache.h"
#include "modules/Items.h"

using std::string;
using std::endl;
using std::vector;
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

DFHACK_PLUGIN_IS_ENABLED(enable_autolabor);

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
    /* HANDLE_VEHICLES */       {HAULERS, false, 1, 200, 0},
    /* HAUL_TRADE */            {HAULERS, false, 1, 200, 0},
    /* PULL_LEVER */            {HAULERS, false, 1, 200, 0},
    /* REMOVE_CONSTRUCTION */   {HAULERS, false, 1, 200, 0},
    /* HAUL_WATER */            {HAULERS, false, 1, 200, 0}
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
    int highest_skill;
    int total_skill;
    int mastery_penalty;
    int assigned_jobs;
    dwarf_state state;
    bool has_exclusive_labor;
    int noble_penalty; // penalty for assignment due to noble status
    bool medical; // this dwarf has medical responsibility
    bool trader;  // this dwarf has trade responsibility
    bool diplomacy; // this dwarf meets with diplomats
    int single_labor; // this dwarf will be exclusively assigned to one labor (-1/NONE for none)
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
    enable_autolabor = false;
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
        "  To prevent particular dwarves from being managed by autolabor, put them\n"
        "  in any burrow.\n"
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

// sorting objects
struct dwarfinfo_sorter
{
    dwarfinfo_sorter(std::vector <dwarf_info_t> & info):dwarf_info(info){};
    bool operator() (int i,int j)
    {
        if (dwarf_info[i].state == IDLE && dwarf_info[j].state != IDLE)
            return true;
        if (dwarf_info[i].state != IDLE && dwarf_info[j].state == IDLE)
            return false;
        return dwarf_info[i].mastery_penalty > dwarf_info[j].mastery_penalty;
    };
    std::vector <dwarf_info_t> & dwarf_info;
};
struct laborinfo_sorter
{
    bool operator() (int i,int j)
    {
        return labor_infos[i].mode() < labor_infos[j].mode();
    };
};

struct values_sorter
{
    values_sorter(std::vector <int> & values):values(values){};
    bool operator() (int i,int j)
    {
        return values[i] > values[j];
    };
    std::vector<int> & values;
};


static void assign_labor(unit_labor::unit_labor labor,
    int n_dwarfs,
    std::vector<dwarf_info_t>& dwarf_info,
    bool trader_requested,
    std::vector<df::unit *>& dwarfs,
    bool has_butchers,
    bool has_fishery,
    color_ostream& out)
{
    df::job_skill skill = labor_to_skill[labor];

        if (labor_infos[labor].mode() != AUTOMATIC)
            return;

        int best_dwarf = 0;
        int best_value = -10000;

        std::vector<int> values(n_dwarfs);
        std::vector<int> candidates;
        std::map<int, int> dwarf_skill;
        std::vector<bool> previously_enabled(n_dwarfs);

        auto mode = labor_infos[labor].mode();

        // Find candidate dwarfs, and calculate a preference value for each dwarf
        for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
        {
            if (dwarf_info[dwarf].state == CHILD)
                continue;
            if (dwarf_info[dwarf].state == MILITARY)
                continue;
            if (dwarf_info[dwarf].trader && trader_requested)
                continue;
            if (dwarf_info[dwarf].diplomacy)
                continue;

            if (labor_infos[labor].is_exclusive && dwarf_info[dwarf].has_exclusive_labor)
                continue;

            int value = dwarf_info[dwarf].mastery_penalty;

            if (skill != job_skill::NONE)
            {
                int skill_level = 0;
                int skill_experience = 0;

                for (auto s = dwarfs[dwarf]->status.souls[0]->skills.begin(); s < dwarfs[dwarf]->status.souls[0]->skills.end(); s++)
                {
                    if ((*s)->id == skill)
                    {
                        skill_level = (*s)->rating;
                        skill_experience = (*s)->experience;
                        break;
                    }
                }

                dwarf_skill[dwarf] = skill_level;

                value += skill_level * 100;
                value += skill_experience / 20;
                if (skill_level > 0 || skill_experience > 0)
                    value += 200;
                if (skill_level >= 15)
                    value += 1000 * (skill_level - 14);
            }
            else
            {
                dwarf_skill[dwarf] = 0;
            }

            if (dwarfs[dwarf]->status.labors[labor])
            {
                value += 5;
                if (labor_infos[labor].is_exclusive)
                    value += 350;
            }

            // bias by happiness

            value += dwarfs[dwarf]->status.happiness;

            values[dwarf] = value;

            candidates.push_back(dwarf);

        }

        // Sort candidates by preference value
        values_sorter ivs(values);
        std::sort(candidates.begin(), candidates.end(), ivs);

        // Disable the labor on everyone
        for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
        {
            if (dwarf_info[dwarf].state == CHILD)
                continue;

            previously_enabled[dwarf] = dwarfs[dwarf]->status.labors[labor];
            dwarfs[dwarf]->status.labors[labor] = false;
        }

        int min_dwarfs = labor_infos[labor].minimum_dwarfs();
        int max_dwarfs = labor_infos[labor].maximum_dwarfs();

        // Special - don't assign hunt without a butchers, or fish without a fishery
        if (unit_labor::HUNT == labor && !has_butchers)
            min_dwarfs = max_dwarfs = 0;
        if (unit_labor::FISH == labor && !has_fishery)
            min_dwarfs = max_dwarfs = 0;

        bool want_idle_dwarf = true;
        if (state_count[IDLE] < 2)
            want_idle_dwarf = false;

        /*
         * Assign dwarfs to this labor. We assign at least the minimum number of dwarfs, in
         * order of preference, and then assign additional dwarfs that meet any of these conditions:
         * - The dwarf is idle and there are no idle dwarves assigned to this labor
         * - The dwarf has nonzero skill associated with the labor
         * - The labor is mining, hunting, or woodcutting and the dwarf currently has it enabled.
         * We stop assigning dwarfs when we reach the maximum allowed.
         * Note that only idle and busy dwarfs count towards the number of dwarfs. "Other" dwarfs
         * (sleeping, eating, on break, etc.) will have labors assigned, but will not be counted.
         * Military and children/nobles will not have labors assigned.
         * Dwarfs with the "health management" responsibility are always assigned DIAGNOSIS.
         */
        for (int i = 0; i < candidates.size() && labor_infos[labor].active_dwarfs < max_dwarfs; i++)
        {
            int dwarf = candidates[i];

            assert(dwarf >= 0);
            assert(dwarf < n_dwarfs);

            bool preferred_dwarf = false;
            if (want_idle_dwarf && dwarf_info[dwarf].state == IDLE)
                preferred_dwarf = true;
            if (dwarf_skill[dwarf] > 0)
                preferred_dwarf = true;
            if (previously_enabled[dwarf] && labor_infos[labor].is_exclusive)
                preferred_dwarf = true;
            if (dwarf_info[dwarf].medical && labor == df::unit_labor::DIAGNOSE)
                preferred_dwarf = true;
            if (dwarf_info[dwarf].trader && trader_requested)
                continue;
            if (dwarf_info[dwarf].diplomacy)
                continue;

            if (labor_infos[labor].active_dwarfs >= min_dwarfs && !preferred_dwarf)
                continue;

            if (!dwarfs[dwarf]->status.labors[labor])
                dwarf_info[dwarf].assigned_jobs++;

            dwarfs[dwarf]->status.labors[labor] = true;

            if (labor_infos[labor].is_exclusive)
            {
                dwarf_info[dwarf].has_exclusive_labor = true;
                // all the exclusive labors require equipment so this should force the dorf to reequip if needed
                dwarfs[dwarf]->military.pickup_flags.bits.update = 1;
            }

            if (print_debug)
                out.print("Dwarf %i \"%s\" assigned %s: value %i %s %s\n", dwarf, dwarfs[dwarf]->name.first_name.c_str(), ENUM_KEY_STR(unit_labor, labor).c_str(), values[dwarf], dwarf_info[dwarf].trader ? "(trader)" : "", dwarf_info[dwarf].diplomacy ? "(diplomacy)" : "");

            if (dwarf_info[dwarf].state == IDLE || dwarf_info[dwarf].state == BUSY)
                labor_infos[labor].active_dwarfs++;

            if (dwarf_info[dwarf].state == IDLE)
                want_idle_dwarf = false;
        }
}


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

    uint32_t race = ui->race_id;
    uint32_t civ = ui->civ_id;

    std::vector<df::unit *> dwarfs;

    bool has_butchers = false;
    bool has_fishery = false;
    bool trader_requested = false;

    for (int i = 0; i < world->buildings.all.size(); ++i)
    {
        df::building *build = world->buildings.all[i];
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
            trader_requested = trader_requested || depot->trade_flags.bits.trader_requested;
            if (print_debug)
            {
                if (trader_requested)
                    out.print("Trade depot found and trader requested, trader will be excluded from all labors.\n");
                else
                    out.print("Trade depot found but trader is not requested.\n");
            }
        }
    }

    for (int i = 0; i < world->units.active.size(); ++i)
    {
        df::unit* cre = world->units.active[i];
        if (Units::isCitizen(cre))
        {
            if (cre->burrows.size() > 0)
                continue;        // dwarfs assigned to burrows are skipped entirely
            dwarfs.push_back(cre);
        }
    }

    int n_dwarfs = dwarfs.size();

    if (n_dwarfs == 0)
        return CR_OK;

    std::vector<dwarf_info_t> dwarf_info(n_dwarfs);

    // Find total skill and highest skill for each dwarf. More skilled dwarves shouldn't be used for minor tasks.

    for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
    {
        dwarf_info[dwarf].single_labor = -1;

        if (dwarfs[dwarf]->status.souls.size() <= 0)
            continue;

        // compute noble penalty

        int noble_penalty = 0;

        df::historical_figure* hf = df::historical_figure::find(dwarfs[dwarf]->hist_figure_id);
        if(hf!=NULL) //can be NULL. E.g. script created citizens
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

                for (int n = 0; n < 25; n++)
                    if (position->responsibilities[n])
                        noble_penalty += responsibility_penalties[n];

                if (position->responsibilities[df::entity_position_responsibility::HEALTH_MANAGEMENT])
                    dwarf_info[dwarf].medical = true;

                if (position->responsibilities[df::entity_position_responsibility::TRADE])
                    dwarf_info[dwarf].trader = true;

            }

        }

        dwarf_info[dwarf].noble_penalty = noble_penalty;

        // identify dwarfs who are needed for meetings and mark them for exclusion

        for (int i = 0; i < ui->activities.size(); ++i)
        {
            df::activity_info *act = ui->activities[i];
            if (!act) continue;
            bool p1 = act->unit_actor == dwarfs[dwarf];
            bool p2 = act->unit_noble == dwarfs[dwarf];

            if (p1 || p2)
            {
                dwarf_info[dwarf].diplomacy = true;
                if (print_debug)
                    out.print("Dwarf %i \"%s\" has a meeting, will be cleared of all labors\n", dwarf, dwarfs[dwarf]->name.first_name.c_str());
                break;
            }
        }

        for (auto s = dwarfs[dwarf]->status.souls[0]->skills.begin(); s != dwarfs[dwarf]->status.souls[0]->skills.end(); s++)
        {
            df::job_skill skill = (*s)->id;

            df::job_skill_class skill_class = ENUM_ATTR(job_skill, type, skill);

            int skill_level = (*s)->rating;
            int skill_experience = (*s)->experience;

            // Track total & highest skill among normal/medical skills. (We don't care about personal or social skills.)

            if (skill_class != job_skill_class::Normal && skill_class != job_skill_class::Medical)
                continue;

            if (dwarf_info[dwarf].highest_skill < skill_level)
                dwarf_info[dwarf].highest_skill = skill_level;
            dwarf_info[dwarf].total_skill += skill_level;
        }
    }

    // Calculate a base penalty for using each dwarf for a task he isn't good at.

    for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
    {
        dwarf_info[dwarf].mastery_penalty -= 40 * dwarf_info[dwarf].highest_skill;
        dwarf_info[dwarf].mastery_penalty -= 10 * dwarf_info[dwarf].total_skill;
        dwarf_info[dwarf].mastery_penalty -= dwarf_info[dwarf].noble_penalty;

        FOR_ENUM_ITEMS(unit_labor, labor)
        {
            if (labor == unit_labor::NONE)
                continue;

            if (labor_infos[labor].is_exclusive && dwarfs[dwarf]->status.labors[labor])
                dwarf_info[dwarf].mastery_penalty -= 100;
        }
    }

    // Find the activity state for each dwarf. It's important to get this right - a dwarf who we think is IDLE but
    // can't work will gum everything up. In the future I might add code to auto-detect slacker dwarves.

    state_count.clear();
    state_count.resize(NUM_STATE);

    for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
    {
        bool is_on_break = false;

        for (auto p = dwarfs[dwarf]->status.misc_traits.begin(); p < dwarfs[dwarf]->status.misc_traits.end(); p++)
        {
            if ((*p)->id == misc_trait_type::Migrant || (*p)->id == misc_trait_type::OnBreak)
                is_on_break = true;
        }

        if (dwarfs[dwarf]->profession == profession::BABY ||
            dwarfs[dwarf]->profession == profession::CHILD ||
            dwarfs[dwarf]->profession == profession::DRUNK)
        {
            dwarf_info[dwarf].state = CHILD;
        }
        else if (ENUM_ATTR(profession, military, dwarfs[dwarf]->profession))
            dwarf_info[dwarf].state = MILITARY;
        else if (dwarfs[dwarf]->job.current_job == NULL)
        {
            if (is_on_break)
                dwarf_info[dwarf].state = OTHER;
            else if (dwarfs[dwarf]->specific_refs.size() > 0)
                dwarf_info[dwarf].state = OTHER;
            else
                dwarf_info[dwarf].state = IDLE;
        }
        else
        {
            int job = dwarfs[dwarf]->job.current_job->job_type;
            if (job >= 0 && job < ARRAY_COUNT(dwarf_states))
                dwarf_info[dwarf].state = dwarf_states[job];
            else
            {
                out.print("Dwarf %i \"%s\" has unknown job %i\n", dwarf, dwarfs[dwarf]->name.first_name.c_str(), job);
                dwarf_info[dwarf].state = OTHER;
            }
        }

        state_count[dwarf_info[dwarf].state]++;

        if (print_debug)
            out.print("Dwarf %i \"%s\": penalty %i, state %s\n", dwarf, dwarfs[dwarf]->name.first_name.c_str(), dwarf_info[dwarf].mastery_penalty, state_names[dwarf_info[dwarf].state]);
    }

    std::vector<df::unit_labor> labors;

    FOR_ENUM_ITEMS(unit_labor, labor)
    {
        if (labor == unit_labor::NONE)
            continue;

        labor_infos[labor].active_dwarfs = 0;

        labors.push_back(labor);
    }
    laborinfo_sorter lasorter;
    std::sort(labors.begin(), labors.end(), lasorter);

    // Handle DISABLED skills (just bookkeeping)
    for (auto lp = labors.begin(); lp != labors.end(); ++lp)
    {
        auto labor = *lp;

        if (labor_infos[labor].mode() != DISABLE)
            continue;

        for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
        {
            if ((dwarf_info[dwarf].trader && trader_requested) ||
                dwarf_info[dwarf].diplomacy)
            {
                dwarfs[dwarf]->status.labors[labor] = false;
            }

            if (dwarfs[dwarf]->status.labors[labor])
            {
                if (labor_infos[labor].is_exclusive)
                    dwarf_info[dwarf].has_exclusive_labor = true;

                dwarf_info[dwarf].assigned_jobs++;
            }
        }
    }

    // Handle all skills except those marked HAULERS

    for (auto lp = labors.begin(); lp != labors.end(); ++lp)
    {
        auto labor = *lp;

        assign_labor(labor, n_dwarfs, dwarf_info, trader_requested, dwarfs, has_butchers, has_fishery, out);
    }

    // Set about 1/3 of the dwarfs as haulers. The haulers have all HAULER labors enabled. Having a lot of haulers helps
    // make sure that hauling jobs are handled quickly rather than building up.

    int num_haulers = state_count[IDLE] + state_count[BUSY] * hauler_pct / 100;

    if (num_haulers < 1)
        num_haulers = 1;

    std::vector<int> hauler_ids;
    for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
    {
        if ((dwarf_info[dwarf].trader && trader_requested) ||
            dwarf_info[dwarf].diplomacy)
        {
            FOR_ENUM_ITEMS(unit_labor, labor)
            {
                if (labor == unit_labor::NONE)
                    continue;
                if (labor_infos[labor].mode() != HAULERS)
                    continue;
                dwarfs[dwarf]->status.labors[labor] = false;
            }
            continue;
        }

        if (dwarf_info[dwarf].state == IDLE || dwarf_info[dwarf].state == BUSY)
            hauler_ids.push_back(dwarf);
    }
    dwarfinfo_sorter sorter(dwarf_info);
    // Idle dwarves come first, then we sort from least-skilled to most-skilled.
    std::sort(hauler_ids.begin(), hauler_ids.end(), sorter);

    // don't set any haulers if everyone is off drinking or something
    if (hauler_ids.size() == 0) {
        num_haulers = 0;
    }

    FOR_ENUM_ITEMS(unit_labor, labor)
    {
        if (labor == unit_labor::NONE)
            continue;

        if (labor_infos[labor].mode() != HAULERS)
            continue;

        for (int i = 0; i < num_haulers; i++)
        {
            assert(i < hauler_ids.size());

            int dwarf = hauler_ids[i];

            assert(dwarf >= 0);
            assert(dwarf < n_dwarfs);
            dwarfs[dwarf]->status.labors[labor] = true;
            dwarf_info[dwarf].assigned_jobs++;

            if (dwarf_info[dwarf].state == IDLE || dwarf_info[dwarf].state == BUSY)
                labor_infos[labor].active_dwarfs++;

            if (print_debug)
                out.print("Dwarf %i \"%s\" assigned %s: hauler\n", dwarf, dwarfs[dwarf]->name.first_name.c_str(), ENUM_KEY_STR(unit_labor, labor).c_str());
        }

        for (int i = num_haulers; i < hauler_ids.size(); i++)
        {
            assert(i < hauler_ids.size());

            int dwarf = hauler_ids[i];

            assert(dwarf >= 0);
            assert(dwarf < n_dwarfs);

            dwarfs[dwarf]->status.labors[labor] = false;
        }
    }

    print_debug = 0;

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

DFhackCExport command_result plugin_enable ( color_ostream &out, bool enable )
{
    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("World is not loaded: please load a game first.\n");
        return CR_FAILURE;
    }

    if (enable && !enable_autolabor)
    {
        enable_plugin(out);
    }
    else if(!enable && enable_autolabor)
    {
        enable_autolabor = false;
        setOptionEnabled(CF_ENABLED, false);

        out << "Autolabor is disabled." << endl;
    }

    return CR_OK;
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

        return plugin_enable(out, enable);
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
