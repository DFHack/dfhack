/**
 * All includes copied from autolabor for now
 */
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
#include "modules/Units.h"

// Not sure what this does, but may have to figure it out later
#define ARRAY_COUNT(array) (sizeof(array)/sizeof((array)[0]))

// I can see a reason for having all of these
using std::string;
using std::endl;
using std::vector;
using namespace DFHack;
using namespace df::enums;

// idk what this does
DFHACK_PLUGIN("autohauler");
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(world);

/*
 * Autohauler module for dfhack
 * Fork of autolabor, DFHack version 0.40.24-r2
 *
 * Rather than the all-of-the-above means of autolabor, autohauler will instead
 * only manage hauling labors and leave skilled labors entirely to the user, who
 * will probably use Dwarf Therapist to do so.
 * Idle dwarves will be assigned the hauling labors; everyone else (including
 * those currently hauling) will have the hauling labors removed. This is to
 * encourage every dwarf to do their assigned skilled labors whenever possible,
 * but resort to hauling when those jobs are not available. This also implies
 * that the user will have a very tight skill assignment, with most skilled
 * labors only being assigned to just one dwarf, no dwarf having more than two
 * active skilled labors, and almost every non-military dwarf having at least
 * one skilled labor assigned.
 * Autohauler allows skills to be flagged as to prevent hauling labors from
 * being assigned when the skill is present. By default this is the unused
 * ALCHEMIST labor but can be changed by the user.
 * It is noteworthy that, as stated in autolabor.cpp, "for almost all labors,
 * once a dwarf begins a job it will finish that job even if the associated
 * labor is removed." This is why we can remove hauling labors by default to try
 * to force dwarves to do "real" jobs whenever they can.
 * This is a standalone plugin. However, it would be wise to delete
 * autolabor.plug.dll as this plugin is mutually exclusive with it.
 */

// Yep...don't know what it does
DFHACK_PLUGIN_IS_ENABLED(enable_autohauler);

// This is the configuration saved into the world save file
static PersistentDataItem config;

// There is a possibility I will add extensive, line-by-line debug capability
// later
static bool print_debug = false;

// Default number of frames between autohauler updates
const static int DEFAULT_FRAME_SKIP = 30;

// Number of frames between autohauler updates
static int frame_skip;

// Don't know what this does
command_result autohauler (color_ostream &out, std::vector <std::string> & parameters);

// Don't know what this does either
static bool isOptionEnabled(unsigned flag)
{
    return config.isValid() && (config.ival(0) & flag) != 0;
}

// Not sure about the purpose of this
enum ConfigFlags {
    CF_ENABLED = 1,
};

// Don't know what this does
static void setOptionEnabled(ConfigFlags flag, bool on)
{
    if (!config.isValid())
        return;

    if (on)
        config.ival(0) |= flag;
    else
        config.ival(0) &= ~flag;
}

// This is a vector of states and number of dwarves in that state
static std::vector<int> state_count(5);

// Employment status of dwarves
enum dwarf_state {
    // Ready for a new task
    IDLE,

    // Busy with a useful task
    BUSY,

    // In the military, can't work
    MILITARY,

    // Baby or Child, can't work
    CHILD,

    // Doing something that precludes working, may be busy for a while
    OTHER
};

// I presume this is the number of states in the following enumeration.
static const int NUM_STATE = 5;

// This is a list of strings to be associated with aforementioned dwarf_state
// struct
static const char *state_names[] = {
    "IDLE",
    "BUSY",
    "MILITARY",
    "CHILD",
    "OTHER"
};

// List of possible activites of a dwarf that will be further narrowed to states
// IDLE - Specifically waiting to be assigned a task (No Job)
// BUSY - Performing a toggleable labor, or a support action for that labor.
// OTHER - Doing something else

static const dwarf_state dwarf_states[] = {
    BUSY  /* CarveFortification */,
    BUSY  /* DetailWall */,
    BUSY  /* DetailFloor */,
    BUSY  /* Dig */,
    BUSY  /* CarveUpwardStaircase */,
    BUSY  /* CarveDownwardStaircase */,
    BUSY  /* CarveUpDownStaircase */,
    BUSY  /* CarveRamp */,
    BUSY  /* DigChannel */,
    BUSY  /* FellTree */,
    BUSY  /* GatherPlants */,
    BUSY  /* RemoveConstruction */,
    BUSY  /* CollectWebs */,
    BUSY  /* BringItemToDepot */,
    BUSY  /* BringItemToShop */,
    OTHER /* Eat */,
    OTHER /* GetProvisions */,
    OTHER /* Drink */,
    OTHER /* Drink2 */,
    OTHER /* FillWaterskin */,
    OTHER /* FillWaterskin2 */,
    OTHER /* Sleep */,
    BUSY  /* CollectSand */,
    BUSY  /* Fish */,
    BUSY  /* Hunt */,
    BUSY  /* HuntVermin */,
    OTHER /* Kidnap */,
    OTHER /* BeatCriminal */,
    OTHER /* StartingFistFight */,
    OTHER /* CollectTaxes */,
    OTHER /* GuardTaxCollector */,
    BUSY  /* CatchLiveLandAnimal */,
    BUSY  /* CatchLiveFish */,
    OTHER /* ReturnKill */,
    OTHER /* CheckChest */,
    OTHER /* StoreOwnedItem */,
    BUSY  /* PlaceItemInTomb */,
    BUSY  /* StoreItemInStockpile */,
    BUSY  /* StoreItemInBag */,
    BUSY  /* StoreItemInHospital */,
    BUSY  /* StoreItemInChest */,
    BUSY  /* StoreItemInCabinet */,
    BUSY  /* StoreWeapon */,
    BUSY  /* StoreArmor */,
    BUSY  /* StoreItemInBarrel */,
    BUSY  /* StoreItemInBin */,
    OTHER /* SeekArtifact */,
    OTHER /* SeekInfant */,
    OTHER /* AttendParty */,
    OTHER /* GoShopping */,
    OTHER /* GoShopping2 */,
    OTHER /* Clean */,
    OTHER /* Rest */,
    BUSY  /* PickupEquipment */,
    BUSY  /* DumpItem */,
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
    BUSY  /* ConstructBuilding */,
    BUSY  /* ConstructDoor */,
    BUSY  /* ConstructFloodgate */,
    BUSY  /* ConstructBed */,
    BUSY  /* ConstructThrone */,
    BUSY  /* ConstructCoffin */,
    BUSY  /* ConstructTable */,
    BUSY  /* ConstructChest */,
    BUSY  /* ConstructBin */,
    BUSY  /* ConstructArmorStand */,
    BUSY  /* ConstructWeaponRack */,
    BUSY  /* ConstructCabinet */,
    BUSY  /* ConstructStatue */,
    BUSY  /* ConstructBlocks */,
    BUSY  /* MakeRawGlass */,
    BUSY  /* MakeCrafts */,
    BUSY  /* MintCoins */,
    BUSY  /* CutGems */,
    BUSY  /* CutGlass */,
    BUSY  /* EncrustWithGems */,
    BUSY  /* EncrustWithGlass */,
    BUSY  /* DestroyBuilding */,
    BUSY  /* SmeltOre */,
    BUSY  /* MeltMetalObject */,
    BUSY  /* ExtractMetalStrands */,
    BUSY  /* PlantSeeds */,
    BUSY  /* HarvestPlants */,
    BUSY  /* TrainHuntingAnimal */,
    BUSY  /* TrainWarAnimal */,
    BUSY  /* MakeWeapon */,
    BUSY  /* ForgeAnvil */,
    BUSY  /* ConstructCatapultParts */,
    BUSY  /* ConstructBallistaParts */,
    BUSY  /* MakeArmor */,
    BUSY  /* MakeHelm */,
    BUSY  /* MakePants */,
    BUSY  /* StudWith */,
    BUSY  /* ButcherAnimal */,
    BUSY  /* PrepareRawFish */,
    BUSY  /* MillPlants */,
    BUSY  /* BaitTrap */,
    BUSY  /* MilkCreature */,
    BUSY  /* MakeCheese */,
    BUSY  /* ProcessPlants */,
    BUSY  /* ProcessPlantsBag */,
    BUSY  /* ProcessPlantsVial */,
    BUSY  /* ProcessPlantsBarrel */,
    BUSY  /* PrepareMeal */,
    BUSY  /* WeaveCloth */,
    BUSY  /* MakeGloves */,
    BUSY  /* MakeShoes */,
    BUSY  /* MakeShield */,
    BUSY  /* MakeCage */,
    BUSY  /* MakeChain */,
    BUSY  /* MakeFlask */,
    BUSY  /* MakeGoblet */,
    BUSY  /* MakeInstrument */,
    BUSY  /* MakeToy */,
    BUSY  /* MakeAnimalTrap */,
    BUSY  /* MakeBarrel */,
    BUSY  /* MakeBucket */,
    BUSY  /* MakeWindow */,
    BUSY  /* MakeTotem */,
    BUSY  /* MakeAmmo */,
    BUSY  /* DecorateWith */,
    BUSY  /* MakeBackpack */,
    BUSY  /* MakeQuiver */,
    BUSY  /* MakeBallistaArrowHead */,
    BUSY  /* AssembleSiegeAmmo */,
    BUSY  /* LoadCatapult */,
    BUSY  /* LoadBallista */,
    BUSY  /* FireCatapult */,
    BUSY  /* FireBallista */,
    BUSY  /* ConstructMechanisms */,
    BUSY  /* MakeTrapComponent */,
    BUSY  /* LoadCageTrap */,
    BUSY  /* LoadStoneTrap */,
    BUSY  /* LoadWeaponTrap */,
    BUSY  /* CleanTrap */,
    OTHER /* CastSpell */,
    BUSY  /* LinkBuildingToTrigger */,
    BUSY  /* PullLever */,
    BUSY  /* BrewDrink */,
    BUSY  /* ExtractFromPlants */,
    BUSY  /* ExtractFromRawFish */,
    BUSY  /* ExtractFromLandAnimal */,
    BUSY  /* TameVermin */,
    BUSY  /* TameAnimal */,
    BUSY  /* ChainAnimal */,
    BUSY  /* UnchainAnimal */,
    BUSY  /* UnchainPet */,
    BUSY  /* ReleaseLargeCreature */,
    BUSY  /* ReleasePet */,
    BUSY  /* ReleaseSmallCreature */,
    BUSY  /* HandleSmallCreature */,
    BUSY  /* HandleLargeCreature */,
    BUSY  /* CageLargeCreature */,
    BUSY  /* CageSmallCreature */,
    BUSY  /* RecoverWounded */,
    BUSY  /* DiagnosePatient */,
    BUSY  /* ImmobilizeBreak */,
    BUSY  /* DressWound */,
    BUSY  /* CleanPatient */,
    BUSY  /* Surgery */,
    BUSY  /* Suture */,
    BUSY  /* SetBone */,
    BUSY  /* PlaceInTraction */,
    BUSY  /* DrainAquarium */,
    BUSY  /* FillAquarium */,
    BUSY  /* FillPond */,
    BUSY  /* GiveWater */,
    BUSY  /* GiveFood */,
    BUSY  /* GiveWater2 */,
    BUSY  /* GiveFood2 */,
    BUSY  /* RecoverPet */,
    BUSY  /* PitLargeAnimal */,
    BUSY  /* PitSmallAnimal */,
    BUSY  /* SlaughterAnimal */,
    BUSY  /* MakeCharcoal */,
    BUSY  /* MakeAsh */,
    BUSY  /* MakeLye */,
    BUSY  /* MakePotashFromLye */,
    BUSY  /* FertilizeField */,
    BUSY  /* MakePotashFromAsh */,
    BUSY  /* DyeThread */,
    BUSY  /* DyeCloth */,
    BUSY  /* SewImage */,
    BUSY  /* MakePipeSection */,
    BUSY  /* OperatePump */,
    OTHER /* ManageWorkOrders */,
    OTHER /* UpdateStockpileRecords */,
    OTHER /* TradeAtDepot */,
    BUSY  /* ConstructHatchCover */,
    BUSY   /* ConstructGrate */,
    BUSY  /* RemoveStairs */,
    BUSY  /* ConstructQuern */,
    BUSY  /* ConstructMillstone */,
    BUSY  /* ConstructSplint */,
    BUSY  /* ConstructCrutch */,
    BUSY  /* ConstructTractionBench */,
    OTHER /* CleanSelf */,
    BUSY  /* BringCrutch */,
    BUSY  /* ApplyCast */,
    BUSY  /* CustomReaction */,
    BUSY  /* ConstructSlab */,
    BUSY  /* EngraveSlab */,
    BUSY  /* ShearCreature */,
    BUSY  /* SpinThread */,
    BUSY  /* PenLargeAnimal */,
    BUSY  /* PenSmallAnimal */,
    BUSY  /* MakeTool */,
    BUSY  /* CollectClay */,
    BUSY  /* InstallColonyInHive */,
    BUSY  /* CollectHiveProducts */,
    OTHER /* CauseTrouble */,
    OTHER /* DrinkBlood */,
    OTHER /* ReportCrime */,
    OTHER /* ExecuteCriminal */,
    BUSY  /* TrainAnimal */,
    BUSY  /* CarveTrack */,
    BUSY  /* PushTrackVehicle */,
    BUSY  /* PlaceTrackVehicle */,
    BUSY  /* StoreItemInVehicle */,
    BUSY  /* GeldAnimal */
};

// Mode assigned to labors. Either it's a hauling job, or it's not.
enum labor_mode {
    ALLOW,
    HAULERS,
    FORBID
};

// This is the default treatment of a particular labor.
struct labor_default
{
    labor_mode mode;
    int active_dwarfs;
};

// This is the current treatment of a particular labor.
// This would have been more cleanly presented as a class
struct labor_info
{
    // It seems as if this is the means of accessing the world data
    PersistentDataItem config;

    // Number of dwarves assigned this labor
    int active_dwarfs;

    // Set the persistent data item associated with this labor treatment
    // We Java folk hate pointers, but that's what the parameter will be
    void set_config(PersistentDataItem a) { config = a; }

    // Return the labor_mode associated with this labor
    labor_mode mode() { return (labor_mode) config.ival(0); }

    // Set the labor_mode associated with this labor
    void set_mode(labor_mode mode) { config.ival(0) = mode; }
};

// This is a vector of all the current labor treatments
static std::vector<struct labor_info> labor_infos;

// This is just an array of all the labors, whether it should be untouched
// (DISABLE) or treated as a last-resort job (HAULERS).
static const struct labor_default default_labor_infos[] = {
    /* MINE */                  {ALLOW,   0},
    /* HAUL_STONE */            {HAULERS, 0},
    /* HAUL_WOOD */             {HAULERS, 0},
    /* HAUL_BODY */             {HAULERS, 0},
    /* HAUL_FOOD */             {HAULERS, 0},
    /* HAUL_REFUSE */           {HAULERS, 0},
    /* HAUL_ITEM */             {HAULERS, 0},
    /* HAUL_FURNITURE */        {HAULERS, 0},
    /* HAUL_ANIMAL */           {HAULERS, 0},
    /* CLEAN */                 {HAULERS, 0},
    /* CUTWOOD */               {ALLOW,   0},
    /* CARPENTER */             {ALLOW,   0},
    /* DETAIL */                {ALLOW,   0},
    /* MASON */                 {ALLOW,   0},
    /* ARCHITECT */             {ALLOW,   0},
    /* ANIMALTRAIN */           {ALLOW,   0},
    /* ANIMALCARE */            {ALLOW,   0},
    /* DIAGNOSE */              {ALLOW,   0},
    /* SURGERY */               {ALLOW,   0},
    /* BONE_SETTING */          {ALLOW,   0},
    /* SUTURING */              {ALLOW,   0},
    /* DRESSING_WOUNDS */       {ALLOW,   0},
    /* FEED_WATER_CIVILIANS */  {HAULERS, 0}, // This could also be ALLOW
    /* RECOVER_WOUNDED */       {HAULERS, 0},
    /* BUTCHER */               {ALLOW,   0},
    /* TRAPPER */               {ALLOW,   0},
    /* DISSECT_VERMIN */        {ALLOW,   0},
    /* LEATHER */               {ALLOW,   0},
    /* TANNER */                {ALLOW,   0},
    /* BREWER */                {ALLOW,   0},
    /* ALCHEMIST */             {FORBID,  0},
    /* SOAP_MAKER */            {ALLOW,   0},
    /* WEAVER */                {ALLOW,   0},
    /* CLOTHESMAKER */          {ALLOW,   0},
    /* MILLER */                {ALLOW,   0},
    /* PROCESS_PLANT */         {ALLOW,   0},
    /* MAKE_CHEESE */           {ALLOW,   0},
    /* MILK */                  {ALLOW,   0},
    /* COOK */                  {ALLOW,   0},
    /* PLANT */                 {ALLOW,   0},
    /* HERBALIST */             {ALLOW,   0},
    /* FISH */                  {ALLOW,   0},
    /* CLEAN_FISH */            {ALLOW,   0},
    /* DISSECT_FISH */          {ALLOW,   0},
    /* HUNT */                  {ALLOW,   0},
    /* SMELT */                 {ALLOW,   0},
    /* FORGE_WEAPON */          {ALLOW,   0},
    /* FORGE_ARMOR */           {ALLOW,   0},
    /* FORGE_FURNITURE */       {ALLOW,   0},
    /* METAL_CRAFT */           {ALLOW,   0},
    /* CUT_GEM */               {ALLOW,   0},
    /* ENCRUST_GEM */           {ALLOW,   0},
    /* WOOD_CRAFT */            {ALLOW,   0},
    /* STONE_CRAFT */           {ALLOW,   0},
    /* BONE_CARVE */            {ALLOW,   0},
    /* GLASSMAKER */            {ALLOW,   0},
    /* EXTRACT_STRAND */        {ALLOW,   0},
    /* SIEGECRAFT */            {ALLOW,   0},
    /* SIEGEOPERATE */          {ALLOW,   0},
    /* BOWYER */                {ALLOW,   0},
    /* MECHANIC */              {ALLOW,   0},
    /* POTASH_MAKING */         {ALLOW,   0},
    /* LYE_MAKING */            {ALLOW,   0},
    /* DYER */                  {ALLOW,   0},
    /* BURN_WOOD */             {ALLOW,   0},
    /* OPERATE_PUMP */          {ALLOW,   0},
    /* SHEARER */               {ALLOW,   0},
    /* SPINNER */               {ALLOW,   0},
    /* POTTERY */               {ALLOW,   0},
    /* GLAZING */               {ALLOW,   0},
    /* PRESSING */              {ALLOW,   0},
    /* BEEKEEPING */            {ALLOW,   0},
    /* WAX_WORKING */           {ALLOW,   0},
    /* HANDLE_VEHICLES */       {HAULERS, 0},
    /* HAUL_TRADE */            {HAULERS, 0},
    /* PULL_LEVER */            {HAULERS, 0},
    /* REMOVE_CONSTRUCTION */   {HAULERS, 0},
    /* HAUL_WATER */            {HAULERS, 0},
    /* GELD */                  {ALLOW,   0},
    /* BUILD_ROAD */            {HAULERS, 0},
    /* BUILD_CONSTRUCTION */    {HAULERS, 0}
};

/**
 * Reset labor to default treatment
 */
static void reset_labor(df::unit_labor labor)
{
    labor_infos[labor].set_mode(default_labor_infos[labor].mode);
}

/**
 * This is individualized dwarf info populated in plugin_onupdate
 */
struct dwarf_info_t
{
    // Current simplified employment status of dwarf
    dwarf_state state;

    // Set to true if for whatever reason we are exempting this dwarf
    // from hauling
    bool haul_exempt;
};

/**
 * Disable autohauler labor lists
 */
static void cleanup_state()
{
    enable_autohauler = false;
    labor_infos.clear();
}

/**
 * Initialize the plugin labor lists
 */
static void init_state()
{
    // This obtains the persistent data from the world save file
    config = World::GetPersistentData("autohauler/config");

    // Check to ensure that the persistent data item actually exists and that
    // the first item in the array of ints isn't -1 (implies disabled)
    if (config.isValid() && config.ival(0) == -1)
        config.ival(0) = 0;

    // Check to see if the plugin is enabled in the persistent data, if so then
    // enable the local flag for autohauler being enabled
    enable_autohauler = isOptionEnabled(CF_ENABLED);

    // If autohauler is not enabled then it's pretty pointless to do the rest
    if (!enable_autohauler)
        return;

    // First get the frame skip from persistent data, or create the item
    // if not present
    auto cfg_frameskip = World::GetPersistentData("autohauler/frameskip");
    if (cfg_frameskip.isValid())
    {
        frame_skip = cfg_frameskip.ival(0);
    }
    else
    {
        // Add to persistent data then get it to assert it's actually there
        cfg_frameskip = World::AddPersistentData("autohauler/frameskip");
        cfg_frameskip.ival(0) = DEFAULT_FRAME_SKIP;
        frame_skip = cfg_frameskip.ival(0);
    }

    /* Here we are going to populate the labor list by loading persistent data
     * from the world save */

    // This is a vector of all the persistent data items from config
    std::vector<PersistentDataItem> items;

    // This populates the aforementioned vector
    World::GetPersistentData(&items, "autohauler/labors/", true);

    // Resize the list of current labor treatments to size of list of default
    // labor treatments
    labor_infos.resize(ARRAY_COUNT(default_labor_infos));

    // For every persistent data item...
    for (auto p = items.begin(); p != items.end(); p++)
    {
        // Load as a string the key associated with the persistent data item
        string key = p->key();

        // Translate the string into a labor defined by global dfhack constants
        df::unit_labor labor = (df::unit_labor) atoi(key.substr(strlen("autohauler/labors/")).c_str());

        // Ensure that the labor is defined in the existing list
        if (labor >= 0 && labor <= labor_infos.size())
        {
            // Link the labor treatment with the associated persistent data item
            labor_infos[labor].set_config(*p);

            // Set the number of dwarves associated with labor to zero
            labor_infos[labor].active_dwarfs = 0;
        }
    }

    // Add default labors for those not in save
    for (int i = 0; i < ARRAY_COUNT(default_labor_infos); i++) {

        // Determine if the labor is already present. If so, exit the for loop
        if (labor_infos[i].config.isValid())
            continue;

        // Not sure of the mechanics, but it seems to give an output stream
        // giving a string for the new persistent data item
        std::stringstream name;
        name << "autohauler/labors/" << i;

        // Add a new persistent data item as it is not currently in the save
        labor_infos[i].set_config(World::AddPersistentData(name.str()));

        // Set the active counter to zero
        labor_infos[i].active_dwarfs = 0;

        // Reset labor to default treatment
        reset_labor((df::unit_labor) i);
    }

}

/**
 * Call this method to enable the plugin.
 */
static void enable_plugin(color_ostream &out)
{
    // If there is no config persistent item, make one
    if (!config.isValid())
    {
        config = World::AddPersistentData("autohauler/config");
        config.ival(0) = 0;
    }

    // I think this is already done in init_state(), but it can't hurt
    setOptionEnabled(CF_ENABLED, true);
    enable_autohauler = true;

    // Output to console that the plugin is enabled
    out << "Enabling the plugin." << endl;

    // Disable autohauler and clear the labor list
    cleanup_state();

    // Initialize the plugin
    init_state();
}

/**
 * Initialize the plugin
 */
DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    // This seems to verify that the default labor list and the current labor
    // list are the same size
    if(ARRAY_COUNT(default_labor_infos) != ENUM_LAST_ITEM(unit_labor) + 1)
    {
        out.printerr("autohauler: labor size mismatch\n");
        return CR_FAILURE;
    }

    // Essentially an introduction dumped to the console
    commands.push_back(PluginCommand(
        "autohauler", "Automatically manage hauling labors.",
        autohauler, false, /* true means that the command can't be used from non-interactive user interface */
        // Extended help string. Used by CR_WRONG_USAGE and the help command:
        "  autohauler enable\n"
        "  autohauler disable\n"
        "    Enables or disables the plugin.\n"
        "  autohauler <labor> haulers\n"
        "    Set a labor to be handled by hauler dwarves.\n"
        "  autohauler <labor> allow\n"
        "    Allow hauling if a specific labor is enabled.\n"
        "  autohauler <labor> forbid\n"
        "    Forbid hauling if a specific labor is enabled.\n"
        "  autohauler <labor> reset\n"
        "    Return a labor to the default handling.\n"
        "  autohauler reset-all\n"
        "    Return all labors to the default handling.\n"
        "  autohauler frameskip <int>\n"
        "    Set the number of frames between runs of autohauler.\n"
        "  autohauler list\n"
        "    List current status of all labors.\n"
        "  autohauler status\n"
        "    Show basic status information.\n"
        "  autohauler debug\n"
        "    In the next cycle, will output the state of every dwarf.\n"
        "Function:\n"
        "  When enabled, autohauler periodically checks your dwarves and assigns\n"
        "  hauling jobs to idle dwarves while removing them from busy dwarves.\n"
        "  This plugin, in contrast to autolabor, is explicitly designed to be\n"
        "  used alongside Dwarf Therapist.\n"
        "  Warning: autohauler will override any manual changes you make to\n"
        "  hauling labors while it is enabled...but why would you make them?\n"
        "Examples:\n"
        "  autohauler HAUL_STONE haulers\n"
        "    Set stone hauling as a hauling labor.\n"
        "  autohauler BOWYER allow\n"
        "    Allow hauling when the bowyer labor is enabled.\n"
        "  autohauler MINE forbid\n"
        "    Forbid hauling while the mining labor is disabled."
    ));

    // Initialize plugin labor lists
    init_state();

    return CR_OK;
}

/**
 * Shut down the plugin
 */
DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    cleanup_state();

    return CR_OK;
}

/**
 * This method responds to the map being loaded, or unloaded.
 */
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

/**
 * This method is called every frame in Dwarf Fortress from my understanding.
 */
DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    // This makes it so that the plugin is only run every 60 steps, in order to
    // save FPS. Since it is static, this is declared before this method is called
    static int step_count = 0;

    // Cancel run if the world doesn't exist or plugin isn't enabled
    if(!world || !world->map.block_index || !enable_autohauler) { return CR_OK; }

    // Increment step count
    step_count++;

    // Run aforementioned step count and return unless threshold is reached.
    if (step_count < frame_skip) return CR_OK;

    // Reset step count since at this point it has reached 60
    step_count = 0;

    // xxx I don't know what this does
    uint32_t race = ui->race_id;
    uint32_t civ = ui->civ_id;

    // Create a vector of units. This will be populated in the following for loop.
    std::vector<df::unit *> dwarfs;

    // Scan the world and look for any citizens in the player's civilization.
    // Add these to the list of dwarves.
    // xxx Does it need to be ++i?
    for (int i = 0; i < world->units.active.size(); ++i)
    {
        df::unit* cre = world->units.active[i];
        if (Units::isCitizen(cre))
        {
            dwarfs.push_back(cre);
        }
    }

    // This just keeps track of the number of civilians from the previous for loop.
    int n_dwarfs = dwarfs.size();

    // This will return if there are no civilians. Otherwise could call
    // nonexistent elements of array.
    if (n_dwarfs == 0)
        return CR_OK;

    // This is a matching of assigned jobs with a dwarf's state
    // xxx but wouldn't it be better if this and "dwarfs" were in the same object?
    std::vector<dwarf_info_t> dwarf_info(n_dwarfs);

    // Reset the counter for number of dwarves in states to zero
    state_count.clear();
    state_count.resize(NUM_STATE);

    // Find the activity state for each dwarf
    for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
    {
        /* Before determining how to handle employment status, handle
         * hauling exemptions first */

        // Default deny condition of on break for later else-if series
        bool is_migrant = false;

        // Scan every labor. If a labor that disallows hauling is present
        // for the dwarf, the dwarf is hauling exempt
        FOR_ENUM_ITEMS(unit_labor, labor)
        {
            if (!(labor == unit_labor::NONE))
            {
                bool test1 = labor_infos[labor].mode() == FORBID;
                bool test2 = dwarfs[dwarf]->status.labors[labor];

                if(test1 && test2) dwarf_info[dwarf].haul_exempt = true;
            }
        }

        // Scan a dwarf's miscellaneous traits for on break or migrant status.
        // If either of these are present, disable hauling because we want them
        // to try to find real jobs first
        for (auto p = dwarfs[dwarf]->status.misc_traits.begin(); p < dwarfs[dwarf]->status.misc_traits.end(); p++)
        {
            if ((*p)->id == misc_trait_type::Migrant)
                is_migrant = true;
        }

        /* Now determine a dwarf's employment status and decide whether
         * to assign hauling */

        // I don't think you can set the labors for babies and children, but let's
        // ignore them anyway
        if (Units::isBaby(dwarfs[dwarf]) || Units::isChild(dwarfs[dwarf]))
        {
            dwarf_info[dwarf].state = CHILD;
        }
        // Account for any hauling exemptions here
        else if (dwarf_info[dwarf].haul_exempt)
        {
            dwarf_info[dwarf].state = BUSY;
        }
        // Account for the military
        else if (ENUM_ATTR(profession, military, dwarfs[dwarf]->profession))
            dwarf_info[dwarf].state = MILITARY;
        // Account for dwarves on break or migrants
        // DF leaves the OnBreak trait type on some dwarves while they're not actually on break
        // Since they have no current job, they will default to IDLE
        else if (is_migrant)
        // Dwarf is unemployed with null job
        {
            dwarf_info[dwarf].state = OTHER;
        }
        else if (dwarfs[dwarf]->job.current_job == NULL)
        {
            dwarf_info[dwarf].state = IDLE;
        }
        // If it gets to this point we look at the task and assign either BUSY or OTHER
        else
        {
            int job = dwarfs[dwarf]->job.current_job->job_type;
            if (job >= 0 && job < ARRAY_COUNT(dwarf_states))
                dwarf_info[dwarf].state = dwarf_states[job];
            else
            {
                // Warn the console that the dwarf has an unregistered labor, default to BUSY
                out.print("Dwarf %i \"%s\" has unknown job %i\n", dwarf, dwarfs[dwarf]->name.first_name.c_str(), job);
                dwarf_info[dwarf].state = BUSY;
            }
        }

        // Debug: Output dwarf job and state data
        if(print_debug)
            out.print("Dwarf %i %s State: %i\n", dwarf, dwarfs[dwarf]->name.first_name.c_str(),
                      dwarf_info[dwarf].state);

        // Increment corresponding labor in default_labor_infos struct
        state_count[dwarf_info[dwarf].state]++;

    }

    // At this point the debug if present has been completed
    print_debug = false;

    // This is a vector of all the labors
    std::vector<df::unit_labor> labors;

    // For every labor...
    FOR_ENUM_ITEMS(unit_labor, labor)
    {
        // Ignore all nonexistent labors
        if (labor == unit_labor::NONE)
            continue;

        // Set number of active dwarves for this job to zero
        labor_infos[labor].active_dwarfs = 0;

        // And add the labor to the aforementioned vector of labors
        labors.push_back(labor);
    }

    // This is a different algorithm than Autolabor. Instead, the intent is to
    // have "real" jobs filled first, then if nothing is available the dwarf
    // instead resorts to hauling.

    // IDLE     - Enable hauling
    // BUSY     - Disable hauling
    // OTHER    - Enable hauling
    // MILITARY - Enable hauling

    // There was no reason to put potential haulers in an array. All of them are
    // covered in the following for loop.

    // Equivalent of Java for(unit_labor : labor)
    // For every labor...
    FOR_ENUM_ITEMS(unit_labor, labor)
    {
        // If this is a non-labor continue to next item
        if (labor == unit_labor::NONE)
            continue;

        // If this is not a hauling labor continue to next item
        if (labor_infos[labor].mode() != HAULERS)
            continue;

        // For every dwarf...
        for(int dwarf = 0; dwarf < dwarfs.size(); dwarf++)
        {
            if (!Units::isValidLabor(dwarfs[dwarf], labor))
                continue;

            // Set hauling labors based on employment states
            if(dwarf_info[dwarf].state == IDLE) {
                dwarfs[dwarf]->status.labors[labor] = true;
            }
            else if(dwarf_info[dwarf].state == MILITARY) {
                dwarfs[dwarf]->status.labors[labor] = true;
            }
            else if(dwarf_info[dwarf].state == OTHER) {
                dwarfs[dwarf]->status.labors[labor] = true;
            }
            else if(dwarf_info[dwarf].state == BUSY) {
                dwarfs[dwarf]->status.labors[labor] = false;
            }
            // If at the end of this the dwarf has the hauling labor, increment the
            // counter
            if(dwarfs[dwarf]->status.labors[labor])
            {
                labor_infos[labor].active_dwarfs++;
            }

            // CHILD ignored
        }

    // Let's play a game of "find the missing bracket!" I hope this is correct.
    }

    // This would be the last statement of the method
    return CR_OK;
}

/**
 * xxx Isn't this a repeat of enable_plugin? If it is separately called, then
 * passing a constructor should suffice.
 */
DFhackCExport command_result plugin_enable ( color_ostream &out, bool enable )
{
    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("World is not loaded: please load a game first.\n");
        return CR_FAILURE;
    }

    if (enable && !enable_autohauler)
    {
        enable_plugin(out);
    }
    else if(!enable && enable_autohauler)
    {
        enable_autohauler = false;
        setOptionEnabled(CF_ENABLED, false);

        out << "Autohauler is disabled." << endl;
    }

    return CR_OK;
}

/**
 * Print the aggregate labor status to the console.
 */
void print_labor (df::unit_labor labor, color_ostream &out)
{
    string labor_name = ENUM_KEY_STR(unit_labor, labor);
    out << labor_name << ": ";
    for (int i = 0; i < 20 - (int)labor_name.length(); i++)
        out << ' ';
    if (labor_infos[labor].mode() == ALLOW) out << "allow" << endl;
    else if(labor_infos[labor].mode() == FORBID) out << "forbid" << endl;
    else if(labor_infos[labor].mode() == HAULERS)
    {
        out << "haulers, currently " << labor_infos[labor].active_dwarfs << " dwarfs" << endl;
    }
    else
    {
        out << "Warning: Invalid labor mode!" << endl;
    }
}

/**
 * This responds to input from the command prompt.
 */
command_result autohauler (color_ostream &out, std::vector <std::string> & parameters)
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
    else if (parameters.size() == 2 && parameters[0] == "frameskip")
    {
        auto cfg_frameskip = World::GetPersistentData("autohauler/frameskip");
        if(cfg_frameskip.isValid())
        {
            int newValue = atoi(parameters[1].c_str());
            cfg_frameskip.ival(0) = newValue;
            out << "Setting frame skip to " << newValue << endl;
            frame_skip = cfg_frameskip.ival(0);
            return CR_OK;
        }
        else
        {
            out << "Warning! No persistent data for frame skip!" << endl;
            return CR_OK;
        }
    }
    else if (parameters.size() >= 2 && parameters.size() <= 4)
    {
        if (!enable_autohauler)
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
        if (parameters[1] == "allow")
        {
            labor_infos[labor].set_mode(ALLOW);
            print_labor(labor, out);
            return CR_OK;
        }
        if (parameters[1] == "forbid")
        {
            labor_infos[labor].set_mode(FORBID);
            print_labor(labor, out);
            return CR_OK;
        }
        if (parameters[1] == "reset")
        {
            reset_labor(labor);
            print_labor(labor, out);
            return CR_OK;
        }

        print_labor(labor, out);

        return CR_OK;
    }
    else if (parameters.size() == 1 && parameters[0] == "reset-all")
    {
        if (!enable_autohauler)
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
        if (!enable_autohauler)
        {
            out << "Error: The plugin is not enabled." << endl;
            return CR_FAILURE;
        }

        bool need_comma = false;
        for (int i = 0; i < NUM_STATE; i++)
        {
            if (state_count[i] == 0)
                continue;
            if (need_comma)
                out << ", ";
            out << state_count[i] << ' ' << state_names[i];
            need_comma = true;
        }
        out << endl;

        out << "Autohauler is running every " << frame_skip << " frames." << endl;

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
        if (!enable_autohauler)
        {
            out << "Error: The plugin is not enabled." << endl;
            return CR_FAILURE;
        }

        print_debug = true;

        return CR_OK;
    }
    else
    {
        out.print("Automatically assigns hauling labors to dwarves.\n"
            "Activate with 'enable autohauler', deactivate with 'disable autohauler'.\n"
            "Current state: %d.\n", enable_autohauler);

        return CR_OK;
    }
}
