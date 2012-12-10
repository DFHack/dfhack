/*
* Autolabor 2.0 module for dfhack
*
* */


#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

#include <vector>
#include <algorithm>
#include <queue>
#include <map>
#include <iterator>

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
#include <df/building_furnacest.h>
#include <df/building_def.h>
#include <df/reaction.h>
#include <df/job_item.h>
#include <df/job_item_ref.h>
#include <df/unit_health_info.h>
#include <df/unit_health_flags.h>
#include <df/building_design.h>
#include <df/vehicle.h>
#include <df/units_other_id.h>
#include <df/ui.h>
#include <df/training_assignment.h>
#include <df/general_ref_contains_itemst.h>

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

static int enable_autolabor = 0;

static bool print_debug = 0;

static std::vector<int> state_count(5);

static PersistentDataItem config;

enum ConfigFlags {
    CF_ENABLED = 1,
    CF_ALLOW_FISHING = 2,
    CF_ALLOW_HUNTING = 4,
};


// Here go all the command declarations...
// mostly to allow having the mandatory stuff on top of the file and commands on the bottom
command_result autolabor (color_ostream &out, std::vector <std::string> & parameters);

// A plugin must be able to return its name and version.
// The name string provided must correspond to the filename - autolabor.plug.so or autolabor.plug.dll in this case
DFHACK_PLUGIN("autolabor");

static void generate_labor_to_skill_map();

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

    int active_dwarfs;

    int priority() { return config.ival(1); }
    void set_priority(int priority) { config.ival(1) = priority; }

    int maximum_dwarfs() { return config.ival(2); }
    void set_maximum_dwarfs(int maximum_dwarfs) { config.ival(2) = maximum_dwarfs; }

    int time_since_last_assigned() 
    { 
        return (*df::global::cur_year - config.ival(3)) * 403200 + *df::global::cur_year_tick - config.ival(4); 
    }
    void mark_assigned() {
        config.ival(3) = (* df::global::cur_year);
        config.ival(4) = (* df::global::cur_year_tick);
    }

};

struct labor_default
{
    int priority;
    int maximum_dwarfs;
};

static std::vector<struct labor_info> labor_infos;

static const struct labor_default default_labor_infos[] = {
    /* MINE */                  {200, 0},
    /* HAUL_STONE */            {100, 0},
    /* HAUL_WOOD */             {100, 0},
    /* HAUL_BODY */             {200, 0},
    /* HAUL_FOOD */             {300, 0},
    /* HAUL_REFUSE */           {100, 0},
    /* HAUL_ITEM */             {100, 0},
    /* HAUL_FURNITURE */        {100, 0},
    /* HAUL_ANIMAL */           {100, 0},
    /* CLEAN */                 {200, 0},
    /* CUTWOOD */               {200, 0},
    /* CARPENTER */             {200, 0},
    /* DETAIL */                {200, 0},
    /* MASON */                 {200, 0},
    /* ARCHITECT */             {400, 0},
    /* ANIMALTRAIN */           {200, 0},
    /* ANIMALCARE */            {200, 0},
    /* DIAGNOSE */              {1000, 0},
    /* SURGERY */               {1000, 0},
    /* BONE_SETTING */          {1000, 0},
    /* SUTURING */              {1000, 0},
    /* DRESSING_WOUNDS */       {1000, 0},
    /* FEED_WATER_CIVILIANS */  {1000, 0},
    /* RECOVER_WOUNDED */       {200, 0},
    /* BUTCHER */               {200, 0},
    /* TRAPPER */               {200, 0},
    /* DISSECT_VERMIN */        {200, 0},
    /* LEATHER */               {200, 0},
    /* TANNER */                {200, 0},
    /* BREWER */                {200, 0},
    /* ALCHEMIST */             {200, 0},
    /* SOAP_MAKER */            {200, 0},
    /* WEAVER */                {200, 0},
    /* CLOTHESMAKER */          {200, 0},
    /* MILLER */                {200, 0},
    /* PROCESS_PLANT */         {200, 0},
    /* MAKE_CHEESE */           {200, 0},
    /* MILK */                  {200, 0},
    /* COOK */                  {200, 0},
    /* PLANT */                 {200, 0},
    /* HERBALIST */             {200, 0},
    /* FISH */                  {100, 0},
    /* CLEAN_FISH */            {200, 0},
    /* DISSECT_FISH */          {200, 0},
    /* HUNT */                  {100, 0},
    /* SMELT */                 {200, 0},
    /* FORGE_WEAPON */          {200, 0},
    /* FORGE_ARMOR */           {200, 0},
    /* FORGE_FURNITURE */       {200, 0},
    /* METAL_CRAFT */           {200, 0},
    /* CUT_GEM */               {200, 0},
    /* ENCRUST_GEM */           {200, 0},
    /* WOOD_CRAFT */            {200, 0},
    /* STONE_CRAFT */           {200, 0},
    /* BONE_CARVE */            {200, 0},
    /* GLASSMAKER */            {200, 0},
    /* EXTRACT_STRAND */        {200, 0},
    /* SIEGECRAFT */            {200, 0},
    /* SIEGEOPERATE */          {200, 0},
    /* BOWYER */                {200, 0},
    /* MECHANIC */              {200, 0},
    /* POTASH_MAKING */         {200, 0},
    /* LYE_MAKING */            {200, 0},
    /* DYER */                  {200, 0},
    /* BURN_WOOD */             {200, 0},
    /* OPERATE_PUMP */          {200, 0},
    /* SHEARER */               {200, 0},
    /* SPINNER */               {200, 0},
    /* POTTERY */               {200, 0},
    /* GLAZING */               {200, 0},
    /* PRESSING */              {200, 0},
    /* BEEKEEPING */            {200, 1}, // reduce risk of stuck beekeepers (see http://www.bay12games.com/dwarves/mantisbt/view.php?id=3981)
    /* WAX_WORKING */           {200, 0},
    /* PUSH_HAUL_VEHICLES */    {200, 0}
};

struct dwarf_info_t
{
    df::unit* dwarf;
    dwarf_state state;

    bool clear_all;

    bool has_axe;
    bool has_pick;
    bool has_crossbow;

    int high_skill;

    bool has_children;
    bool armed;

    df::unit_labor using_labor;

    dwarf_info_t(df::unit* dw) : dwarf(dw), clear_all(false), has_axe(false), has_pick(false), has_crossbow(false), 
        state(OTHER), high_skill(0), has_children(false), armed(false)
    {
    }

    void set_labor(df::unit_labor labor) 
    {
        if (labor >= 0 && labor <= ENUM_LAST_ITEM(unit_labor))
        {
            dwarf->status.labors[labor] = true;
            if ((labor == df::unit_labor::MINE    && !has_pick) ||
                (labor == df::unit_labor::CUTWOOD && !has_axe) ||
                (labor == df::unit_labor::HUNT    && !has_crossbow))
                dwarf->military.pickup_flags.bits.update = 1;
        }
    }

    void clear_labor(df::unit_labor labor) 
    {
        if (labor >= 0 && labor <= ENUM_LAST_ITEM(unit_labor))
        {
            dwarf->status.labors[labor] = false;
            if ((labor == df::unit_labor::MINE    && has_pick) ||
                (labor == df::unit_labor::CUTWOOD && has_axe) ||
                (labor == df::unit_labor::HUNT    && has_crossbow))
                dwarf->military.pickup_flags.bits.update = 1;
        }
    }

};

/* 
* Here starts all the complicated stuff to try to deduce labors from jobs.
* This is all way more complicated than it really ought to be, but I have
* not found a way to make it simpler.
*/

static df::unit_labor hauling_labor_map[] =
{
    df::unit_labor::HAUL_ITEM,	/* BAR */
    df::unit_labor::HAUL_STONE,	/* SMALLGEM */
    df::unit_labor::HAUL_ITEM,	/* BLOCKS */
    df::unit_labor::HAUL_STONE,	/* ROUGH */
    df::unit_labor::HAUL_STONE,	/* BOULDER */
    df::unit_labor::HAUL_WOOD,	/* WOOD */
    df::unit_labor::HAUL_FURNITURE,	/* DOOR */
    df::unit_labor::HAUL_FURNITURE,	/* FLOODGATE */
    df::unit_labor::HAUL_FURNITURE,	/* BED */
    df::unit_labor::HAUL_FURNITURE,	/* CHAIR */
    df::unit_labor::HAUL_ITEM,	/* CHAIN */
    df::unit_labor::HAUL_ITEM,	/* FLASK */
    df::unit_labor::HAUL_ITEM,	/* GOBLET */
    df::unit_labor::HAUL_ITEM,	/* INSTRUMENT */
    df::unit_labor::HAUL_ITEM,	/* TOY */
    df::unit_labor::HAUL_FURNITURE,	/* WINDOW */
    df::unit_labor::HAUL_ANIMAL,	/* CAGE */
    df::unit_labor::HAUL_ITEM,	/* BARREL */
    df::unit_labor::HAUL_ITEM,	/* BUCKET */
    df::unit_labor::HAUL_ANIMAL,	/* ANIMALTRAP */
    df::unit_labor::HAUL_FURNITURE,	/* TABLE */
    df::unit_labor::HAUL_FURNITURE,	/* COFFIN */
    df::unit_labor::HAUL_FURNITURE,	/* STATUE */
    df::unit_labor::HAUL_REFUSE,	/* CORPSE */
    df::unit_labor::HAUL_ITEM,	/* WEAPON */
    df::unit_labor::HAUL_ITEM,	/* ARMOR */
    df::unit_labor::HAUL_ITEM,	/* SHOES */
    df::unit_labor::HAUL_ITEM,	/* SHIELD */
    df::unit_labor::HAUL_ITEM,	/* HELM */
    df::unit_labor::HAUL_ITEM,	/* GLOVES */
    df::unit_labor::HAUL_FURNITURE,	/* BOX */
    df::unit_labor::HAUL_ITEM,	/* BIN */
    df::unit_labor::HAUL_FURNITURE,	/* ARMORSTAND */
    df::unit_labor::HAUL_FURNITURE,	/* WEAPONRACK */
    df::unit_labor::HAUL_FURNITURE,	/* CABINET */
    df::unit_labor::HAUL_ITEM,	/* FIGURINE */
    df::unit_labor::HAUL_ITEM,	/* AMULET */
    df::unit_labor::HAUL_ITEM,	/* SCEPTER */
    df::unit_labor::HAUL_ITEM,	/* AMMO */
    df::unit_labor::HAUL_ITEM,	/* CROWN */
    df::unit_labor::HAUL_ITEM,	/* RING */
    df::unit_labor::HAUL_ITEM,	/* EARRING */
    df::unit_labor::HAUL_ITEM,	/* BRACELET */
    df::unit_labor::HAUL_ITEM,	/* GEM */
    df::unit_labor::HAUL_FURNITURE,	/* ANVIL */
    df::unit_labor::HAUL_REFUSE,	/* CORPSEPIECE */
    df::unit_labor::HAUL_REFUSE,	/* REMAINS */
    df::unit_labor::HAUL_FOOD,	/* MEAT */
    df::unit_labor::HAUL_FOOD,	/* FISH */
    df::unit_labor::HAUL_FOOD,	/* FISH_RAW */
    df::unit_labor::HAUL_REFUSE,	/* VERMIN */
    df::unit_labor::HAUL_ITEM,	/* PET */
    df::unit_labor::HAUL_FOOD,	/* SEEDS */
    df::unit_labor::HAUL_FOOD,	/* PLANT */
    df::unit_labor::HAUL_ITEM,	/* SKIN_TANNED */
    df::unit_labor::HAUL_FOOD,	/* LEAVES */
    df::unit_labor::HAUL_ITEM,	/* THREAD */
    df::unit_labor::HAUL_ITEM,	/* CLOTH */
    df::unit_labor::HAUL_ITEM,	/* TOTEM */
    df::unit_labor::HAUL_ITEM,	/* PANTS */
    df::unit_labor::HAUL_ITEM,	/* BACKPACK */
    df::unit_labor::HAUL_ITEM,	/* QUIVER */
    df::unit_labor::HAUL_FURNITURE,	/* CATAPULTPARTS */
    df::unit_labor::HAUL_FURNITURE,	/* BALLISTAPARTS */
    df::unit_labor::HAUL_FURNITURE,	/* SIEGEAMMO */
    df::unit_labor::HAUL_FURNITURE,	/* BALLISTAARROWHEAD */
    df::unit_labor::HAUL_FURNITURE,	/* TRAPPARTS */
    df::unit_labor::HAUL_FURNITURE,	/* TRAPCOMP */
    df::unit_labor::HAUL_FOOD,	/* DRINK */
    df::unit_labor::HAUL_FOOD,	/* POWDER_MISC */
    df::unit_labor::HAUL_FOOD,	/* CHEESE */
    df::unit_labor::HAUL_FOOD,	/* FOOD */
    df::unit_labor::HAUL_FOOD,	/* LIQUID_MISC */
    df::unit_labor::HAUL_ITEM,	/* COIN */
    df::unit_labor::HAUL_FOOD,	/* GLOB */
    df::unit_labor::HAUL_STONE,	/* ROCK */
    df::unit_labor::HAUL_FURNITURE,	/* PIPE_SECTION */
    df::unit_labor::HAUL_FURNITURE,	/* HATCH_COVER */
    df::unit_labor::HAUL_FURNITURE,	/* GRATE */
    df::unit_labor::HAUL_FURNITURE,	/* QUERN */
    df::unit_labor::HAUL_FURNITURE,	/* MILLSTONE */
    df::unit_labor::HAUL_ITEM,	/* SPLINT */
    df::unit_labor::HAUL_ITEM,	/* CRUTCH */
    df::unit_labor::HAUL_FURNITURE,	/* TRACTION_BENCH */
    df::unit_labor::HAUL_ITEM,	/* ORTHOPEDIC_CAST */
    df::unit_labor::HAUL_ITEM,	/* TOOL */
    df::unit_labor::HAUL_FURNITURE,	/* SLAB */
    df::unit_labor::HAUL_FOOD,	/* EGG */
    df::unit_labor::HAUL_ITEM,	/* BOOK */
};

static df::unit_labor workshop_build_labor[] = 
{
    /* Carpenters */		df::unit_labor::CARPENTER,
    /* Farmers */			df::unit_labor::PROCESS_PLANT,
    /* Masons */			df::unit_labor::MASON,
    /* Craftsdwarfs */		df::unit_labor::STONE_CRAFT,
    /* Jewelers */			df::unit_labor::CUT_GEM,
    /* MetalsmithsForge */	df::unit_labor::METAL_CRAFT,
    /* MagmaForge */		df::unit_labor::METAL_CRAFT,
    /* Bowyers */			df::unit_labor::BOWYER,
    /* Mechanics */			df::unit_labor::MECHANIC,
    /* Siege */				df::unit_labor::SIEGECRAFT,
    /* Butchers */			df::unit_labor::BUTCHER,
    /* Leatherworks */		df::unit_labor::LEATHER,
    /* Tanners */			df::unit_labor::TANNER,
    /* Clothiers */			df::unit_labor::CLOTHESMAKER,
    /* Fishery */			df::unit_labor::FISH,
    /* Still */				df::unit_labor::BREWER,
    /* Loom */				df::unit_labor::WEAVER,
    /* Quern */				df::unit_labor::MILLER,
    /* Kennels */			df::unit_labor::ANIMALTRAIN,
    /* Kitchen */			df::unit_labor::COOK,
    /* Ashery */			df::unit_labor::LYE_MAKING,
    /* Dyers */				df::unit_labor::DYER,
    /* Millstone */			df::unit_labor::MILLER,
    /* Custom */			df::unit_labor::NONE,
    /* Tool */				df::unit_labor::NONE
};

static df::building* get_building_from_job(df::job* j)
{
    for (auto r = j->general_refs.begin(); r != j->general_refs.end(); r++)
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

static df::unit_labor construction_build_labor (df::item* i) 
{
    MaterialInfo matinfo;
    if (matinfo.decode(i))
    {
        if (matinfo.material->flags.is_set(df::material_flags::IS_METAL))
            return df::unit_labor::METAL_CRAFT;
        if (matinfo.material->flags.is_set(df::material_flags::WOOD))
            return df::unit_labor::CARPENTER;
    }
    return df::unit_labor::MASON;
}

color_ostream* debug_stream;

void debug (char* fmt, ...)
{
    if (debug_stream)
    {
        va_list args;
        va_start(args,fmt);
        debug_stream->vprint(fmt, args);
        va_end(args);
    }
}


class JobLaborMapper {

private:
    class jlfunc 
    {
    public:
        virtual df::unit_labor get_labor(df::job* j) = 0;
    };

    class jlfunc_const : public jlfunc 
    {
    private:
        df::unit_labor labor;
    public:
        df::unit_labor get_labor(df::job* j) 
        {
            return labor;
        }
        jlfunc_const(df::unit_labor l) : labor(l) {};
    };

    class jlfunc_hauling : public jlfunc 
    {
    public:
        df::unit_labor get_labor(df::job* j)
        {
            df::item* item = 0;
            if (j->job_type == df::job_type::StoreItemInStockpile && j->item_subtype != -1)
                return (df::unit_labor) j->item_subtype;

            for (auto i = j->items.begin(); i != j->items.end(); i++)
            {
                if ((*i)->role == 7)
                {
                    item = (*i)->item;
                    break;
                }
            }

            if (item && item->flags.bits.container) 
            {
                for (auto a = item->general_refs.begin(); a != item->general_refs.end(); a++)
                {
                    if ((*a)->getType() == df::general_ref_type::CONTAINS_ITEM)
                    {
                        int item_id = ((df::general_ref_contains_itemst *) (*a))->item_id;
                        item = binsearch_in_vector(world->items.all, item_id);
                        break;
                    }
                }
            }

            df::unit_labor l = item ? hauling_labor_map[item->getType()] : df::unit_labor::HAUL_ITEM;
            if (item && l == df::unit_labor::HAUL_REFUSE && item->flags.bits.dead_dwarf)
                l = df::unit_labor::HAUL_BODY;
            return l;
        }
        jlfunc_hauling() {};
    };

    class jlfunc_construct_bld : public jlfunc 
    {
    public:
        df::unit_labor get_labor(df::job* j)
        {
            df::building* bld = get_building_from_job (j);
            switch (bld->getType()) 
            {
            case df::building_type::Workshop:
                {
                    df::building_workshopst* ws = (df::building_workshopst*) bld;
                    if (ws->design && !ws->design->flags.bits.designed)
                        return df::unit_labor::ARCHITECT;
                    if (ws->type == df::workshop_type::Custom) 
                    {
                        df::building_def* def = df::building_def::find(ws->custom_type);
                        return def->build_labors[0];
                    } 
                    else 
                        return workshop_build_labor[ws->type];
                }
                break;
            case df::building_type::Furnace:
            case df::building_type::TradeDepot:
            case df::building_type::Construction:
            case df::building_type::Bridge:
            case df::building_type::ArcheryTarget:
                {
                    df::building_actual* b = (df::building_actual*) bld;
                    if (b->design && !b->design->flags.bits.designed)
                        return df::unit_labor::ARCHITECT;
                    return construction_build_labor(j->items[0]->item);
                }
                break;
            case df::building_type::FarmPlot:
                return df::unit_labor::PLANT;
            case df::building_type::Chair:
            case df::building_type::Bed:
            case df::building_type::Table:
            case df::building_type::Coffin:
            case df::building_type::Door:
            case df::building_type::Floodgate:
            case df::building_type::Box:
            case df::building_type::Weaponrack:
            case df::building_type::Armorstand:
            case df::building_type::Cabinet:
            case df::building_type::Statue:
            case df::building_type::WindowGlass:
            case df::building_type::WindowGem:
            case df::building_type::Cage:
            case df::building_type::NestBox:
            case df::building_type::TractionBench:
            case df::building_type::Slab:
                return df::unit_labor::HAUL_FURNITURE;
            case df::building_type::Trap:
                return df::unit_labor::MECHANIC;
            }

            debug ("AUTOLABOR: Cannot deduce labor for construct building job of type %s\n",
                ENUM_KEY_STR(building_type, bld->getType()).c_str());

            return df::unit_labor::NONE;
        }
        jlfunc_construct_bld() {}
    };

    class jlfunc_destroy_bld : public jlfunc 
    {
    public:
        df::unit_labor get_labor(df::job* j)
        {
            df::building* bld = get_building_from_job (j);
            df::building_type type = bld->getType();

            switch (bld->getType()) 
            {
            case df::building_type::Workshop:
                {
                    df::building_workshopst* ws = (df::building_workshopst*) bld;
                    if (ws->type == df::workshop_type::Custom) 
                    {
                        df::building_def* def = df::building_def::find(ws->custom_type);
                        return def->build_labors[0];
                    } 
                    else 
                        return workshop_build_labor[ws->type];
                }
                break;
            case df::building_type::Furnace:
            case df::building_type::TradeDepot:
            case df::building_type::Construction:
            case df::building_type::Wagon:
                {
                    df::building_actual* b = (df::building_actual*) bld;
                    return construction_build_labor(b->contained_items[0]->item);
                }
                break;
            case df::building_type::FarmPlot:
                return df::unit_labor::PLANT;
            }

            debug ("AUTOLABOR: Cannot deduce labor for destroy building job of type %s\n",
                ENUM_KEY_STR(building_type, bld->getType()).c_str());

            return df::unit_labor::NONE;
        }
        jlfunc_destroy_bld() {}
    };

    class jlfunc_make : public jlfunc 
    {
    private:
        df::unit_labor metaltype;
    public:
        df::unit_labor get_labor(df::job* j)
        {
            df::building* bld = get_building_from_job(j);
            if (bld->getType() == df::building_type::Workshop)
            {
                df::workshop_type type = ((df::building_workshopst*)(bld))->type;
                switch (type) 
                {
                case df::workshop_type::Craftsdwarfs:
                    {
                        df::item_type jobitem = j->job_items[0]->item_type;
                        switch (jobitem) 
                        {
                        case df::item_type::BOULDER:
                            return df::unit_labor::STONE_CRAFT;
                        case df::item_type::NONE:
                            if (j->material_category.bits.bone)
                                return df::unit_labor::BONE_CARVE;
                            else
                            {
                                debug ("AUTOLABOR: Cannot deduce labor for make crafts job (not bone)\n");
                                return df::unit_labor::NONE; 
                            }
                        default:
                            debug ("AUTOLABOR: Cannot deduce labor for make crafts job, item type %s\n", 
                                ENUM_KEY_STR(item_type, jobitem).c_str());
                            return df::unit_labor::NONE; 
                        }
                    }
                case df::workshop_type::Masons:
                    return df::unit_labor::MASON;
                case df::workshop_type::Carpenters:
                    return df::unit_labor::CARPENTER;
                case df::workshop_type::Leatherworks:
                    return df::unit_labor::LEATHER;
                case df::workshop_type::Clothiers:
                    return df::unit_labor::CLOTHESMAKER;
                case df::workshop_type::MagmaForge:
                case df::workshop_type::MetalsmithsForge:
                    return metaltype;
                default:
                    debug ("AUTOLABOR: Cannot deduce labor for make job, workshop type %s\n",
                        ENUM_KEY_STR(workshop_type, type).c_str());
                    return df::unit_labor::NONE; 
                }
            }
            else if (bld->getType() == df::building_type::Furnace)
            {
                df::furnace_type type = ((df::building_furnacest*)(bld))->type;
                switch (type) 
                {
                case df::furnace_type::MagmaGlassFurnace:
                case df::furnace_type::GlassFurnace:
                    return df::unit_labor::GLASSMAKER;
                default:
                    debug ("AUTOLABOR: Cannot deduce labor for make job, furnace type %s\n",
                        ENUM_KEY_STR(furnace_type, type).c_str());
                    return df::unit_labor::NONE; 
                }
            }

            debug ("AUTOLABOR: Cannot deduce labor for make job, building type %s\n",
                ENUM_KEY_STR(building_type, bld->getType()).c_str());

            return df::unit_labor::NONE; 
        }

        jlfunc_make (df::unit_labor mt) : metaltype(mt) {}
    };

    class jlfunc_custom : public jlfunc 
    {
    public:
        df::unit_labor get_labor(df::job* j)
        {
            for (auto r = world->raws.reactions.begin(); r != world->raws.reactions.end(); r++)
            {
                if ((*r)->code == j->reaction_name)
                {
                    df::job_skill skill = (*r)->skill;
                    df::unit_labor labor = ENUM_ATTR(job_skill, labor, skill);
                    return labor;
                }
            }
            return df::unit_labor::NONE;
        }
        jlfunc_custom() {}
    };

    map<df::unit_labor, jlfunc*> jlf_cache;

    jlfunc* jlf_const(df::unit_labor l) {
        jlfunc* jlf;
        if (jlf_cache.count(l) == 0)
        {
            jlf = new jlfunc_const(l);
            jlf_cache[l] = jlf;
        }
        else
            jlf = jlf_cache[l];

        return jlf;
    }
private:
    std::map<df::job_type,jlfunc*> job_to_labor_table;

public:
    ~JobLaborMapper() 
    {
        std::set<jlfunc*> log;

        for (auto i = jlf_cache.begin(); i != jlf_cache.end(); i++)
        {
            if (!log.count(i->second))
            {
                log.insert(i->second);
                delete i->second;
            }
            i->second = 0;
        }

        FOR_ENUM_ITEMS (job_type, j)
        {
            if (j < 0) 
                continue;

            jlfunc* p = job_to_labor_table[j];
            if (!log.count(p))
            {
                log.insert(p);
                delete p;
            }
            job_to_labor_table[j] = 0;
        }
    }

    JobLaborMapper() 
    {
        jlfunc* jlf_hauling		   = new jlfunc_hauling();
        jlfunc* jlf_make_furniture = new jlfunc_make(df::unit_labor::FORGE_FURNITURE);
        jlfunc* jlf_make_object    = new jlfunc_make(df::unit_labor::METAL_CRAFT);
        jlfunc* jlf_make_armor     = new jlfunc_make(df::unit_labor::FORGE_ARMOR);
        jlfunc* jlf_make_weapon    = new jlfunc_make(df::unit_labor::FORGE_WEAPON);

        jlfunc* jlf_no_labor = jlf_const(df::unit_labor::NONE);

        job_to_labor_table[df::job_type::CarveFortification]	= jlf_const(df::unit_labor::DETAIL);
        job_to_labor_table[df::job_type::DetailWall]			= jlf_const(df::unit_labor::DETAIL);
        job_to_labor_table[df::job_type::DetailFloor]			= jlf_const(df::unit_labor::DETAIL);
        job_to_labor_table[df::job_type::Dig]					= jlf_const(df::unit_labor::MINE);
        job_to_labor_table[df::job_type::CarveUpwardStaircase]	= jlf_const(df::unit_labor::MINE);
        job_to_labor_table[df::job_type::CarveDownwardStaircase] = jlf_const(df::unit_labor::MINE);
        job_to_labor_table[df::job_type::CarveUpDownStaircase]	= jlf_const(df::unit_labor::MINE);
        job_to_labor_table[df::job_type::CarveRamp]				= jlf_const(df::unit_labor::MINE);
        job_to_labor_table[df::job_type::DigChannel]			= jlf_const(df::unit_labor::MINE);
        job_to_labor_table[df::job_type::FellTree]				= jlf_const(df::unit_labor::CUTWOOD);
        job_to_labor_table[df::job_type::GatherPlants]			= jlf_const(df::unit_labor::HERBALIST);
        job_to_labor_table[df::job_type::RemoveConstruction]	= jlf_no_labor;
        job_to_labor_table[df::job_type::CollectWebs]			= jlf_const(df::unit_labor::WEAVER);
        job_to_labor_table[df::job_type::BringItemToDepot]		= jlf_no_labor;
        job_to_labor_table[df::job_type::BringItemToShop]		= jlf_no_labor;
        job_to_labor_table[df::job_type::Eat]					= jlf_no_labor;
        job_to_labor_table[df::job_type::GetProvisions]			= jlf_no_labor;
        job_to_labor_table[df::job_type::Drink]					= jlf_no_labor;
        job_to_labor_table[df::job_type::Drink2]				= jlf_no_labor;
        job_to_labor_table[df::job_type::FillWaterskin]			= jlf_no_labor;
        job_to_labor_table[df::job_type::FillWaterskin2]		= jlf_no_labor;
        job_to_labor_table[df::job_type::Sleep]					= jlf_no_labor;
        job_to_labor_table[df::job_type::CollectSand]			= jlf_const(df::unit_labor::HAUL_ITEM);
        job_to_labor_table[df::job_type::Fish]					= jlf_const(df::unit_labor::FISH);
        job_to_labor_table[df::job_type::Hunt]					= jlf_const(df::unit_labor::HUNT);
        job_to_labor_table[df::job_type::HuntVermin]			= jlf_no_labor;
        job_to_labor_table[df::job_type::Kidnap]				= jlf_no_labor;
        job_to_labor_table[df::job_type::BeatCriminal]			= jlf_no_labor;
        job_to_labor_table[df::job_type::StartingFistFight]		= jlf_no_labor;
        job_to_labor_table[df::job_type::CollectTaxes]			= jlf_no_labor;
        job_to_labor_table[df::job_type::GuardTaxCollector]		= jlf_no_labor;
        job_to_labor_table[df::job_type::CatchLiveLandAnimal]	= jlf_const(df::unit_labor::HUNT);
        job_to_labor_table[df::job_type::CatchLiveFish]			= jlf_const(df::unit_labor::FISH);
        job_to_labor_table[df::job_type::ReturnKill]			= jlf_no_labor;
        job_to_labor_table[df::job_type::CheckChest]			= jlf_no_labor;
        job_to_labor_table[df::job_type::StoreOwnedItem]		= jlf_no_labor;
        job_to_labor_table[df::job_type::PlaceItemInTomb]		= jlf_const(df::unit_labor::HAUL_BODY);
        job_to_labor_table[df::job_type::StoreItemInStockpile]	= jlf_hauling;
        job_to_labor_table[df::job_type::StoreItemInBag]		= jlf_hauling;
        job_to_labor_table[df::job_type::StoreItemInHospital]	= jlf_hauling;
        job_to_labor_table[df::job_type::StoreItemInChest]		= jlf_hauling;
        job_to_labor_table[df::job_type::StoreItemInCabinet]	= jlf_hauling;
        job_to_labor_table[df::job_type::StoreWeapon]			= jlf_hauling;
        job_to_labor_table[df::job_type::StoreArmor]			= jlf_hauling;
        job_to_labor_table[df::job_type::StoreItemInBarrel]		= jlf_hauling;
        job_to_labor_table[df::job_type::StoreItemInBin]		= jlf_const(df::unit_labor::HAUL_ITEM);
        job_to_labor_table[df::job_type::SeekArtifact]			= jlf_no_labor;
        job_to_labor_table[df::job_type::SeekInfant]			= jlf_no_labor;
        job_to_labor_table[df::job_type::AttendParty]			= jlf_no_labor;
        job_to_labor_table[df::job_type::GoShopping]			= jlf_no_labor;
        job_to_labor_table[df::job_type::GoShopping2]			= jlf_no_labor;
        job_to_labor_table[df::job_type::Clean]					= jlf_const(df::unit_labor::CLEAN);
        job_to_labor_table[df::job_type::Rest]					= jlf_no_labor;
        job_to_labor_table[df::job_type::PickupEquipment]		= jlf_no_labor;
        job_to_labor_table[df::job_type::DumpItem]				= jlf_const(df::unit_labor::HAUL_REFUSE);
        job_to_labor_table[df::job_type::StrangeMoodCrafter]	= jlf_no_labor;
        job_to_labor_table[df::job_type::StrangeMoodJeweller]	= jlf_no_labor;
        job_to_labor_table[df::job_type::StrangeMoodForge]		= jlf_no_labor;
        job_to_labor_table[df::job_type::StrangeMoodMagmaForge] = jlf_no_labor;
        job_to_labor_table[df::job_type::StrangeMoodBrooding]	= jlf_no_labor;
        job_to_labor_table[df::job_type::StrangeMoodFell]		= jlf_no_labor;
        job_to_labor_table[df::job_type::StrangeMoodCarpenter]	= jlf_no_labor;
        job_to_labor_table[df::job_type::StrangeMoodMason]		= jlf_no_labor;
        job_to_labor_table[df::job_type::StrangeMoodBowyer]		= jlf_no_labor;
        job_to_labor_table[df::job_type::StrangeMoodTanner]		= jlf_no_labor;
        job_to_labor_table[df::job_type::StrangeMoodWeaver]		= jlf_no_labor;
        job_to_labor_table[df::job_type::StrangeMoodGlassmaker] = jlf_no_labor;
        job_to_labor_table[df::job_type::StrangeMoodMechanics]	= jlf_no_labor;
        job_to_labor_table[df::job_type::ConstructBuilding]     = new jlfunc_construct_bld();
        job_to_labor_table[df::job_type::ConstructDoor]			= jlf_make_furniture;
        job_to_labor_table[df::job_type::ConstructFloodgate]	= jlf_make_furniture;
        job_to_labor_table[df::job_type::ConstructBed]			= jlf_make_furniture;
        job_to_labor_table[df::job_type::ConstructThrone]		= jlf_make_furniture;
        job_to_labor_table[df::job_type::ConstructCoffin]		= jlf_make_furniture;
        job_to_labor_table[df::job_type::ConstructTable]		= jlf_make_furniture;
        job_to_labor_table[df::job_type::ConstructChest]		= jlf_make_furniture;
        job_to_labor_table[df::job_type::ConstructBin]			= jlf_make_furniture;
        job_to_labor_table[df::job_type::ConstructArmorStand]	= jlf_make_furniture;
        job_to_labor_table[df::job_type::ConstructWeaponRack]	= jlf_make_furniture;
        job_to_labor_table[df::job_type::ConstructCabinet]		= jlf_make_furniture;
        job_to_labor_table[df::job_type::ConstructStatue]		= jlf_make_furniture;
        job_to_labor_table[df::job_type::ConstructBlocks]		= jlf_make_furniture;
        job_to_labor_table[df::job_type::MakeRawGlass]			= jlf_const(df::unit_labor::GLASSMAKER);
        job_to_labor_table[df::job_type::MakeCrafts]			= jlf_make_object;
        job_to_labor_table[df::job_type::MintCoins]				= jlf_const(df::unit_labor::METAL_CRAFT);
        job_to_labor_table[df::job_type::CutGems]				= jlf_const(df::unit_labor::CUT_GEM);
        job_to_labor_table[df::job_type::CutGlass]				= jlf_const(df::unit_labor::CUT_GEM);
        job_to_labor_table[df::job_type::EncrustWithGems]		= jlf_const(df::unit_labor::ENCRUST_GEM);
        job_to_labor_table[df::job_type::EncrustWithGlass]		= jlf_const(df::unit_labor::ENCRUST_GEM);
        job_to_labor_table[df::job_type::DestroyBuilding]		= new jlfunc_destroy_bld();
        job_to_labor_table[df::job_type::SmeltOre]				= jlf_const(df::unit_labor::SMELT);
        job_to_labor_table[df::job_type::MeltMetalObject]		= jlf_const(df::unit_labor::SMELT);
        job_to_labor_table[df::job_type::ExtractMetalStrands]	= jlf_const(df::unit_labor::EXTRACT_STRAND);
        job_to_labor_table[df::job_type::PlantSeeds]			= jlf_const(df::unit_labor::PLANT);
        job_to_labor_table[df::job_type::HarvestPlants]			= jlf_const(df::unit_labor::PLANT);
        job_to_labor_table[df::job_type::TrainHuntingAnimal]	= jlf_const(df::unit_labor::ANIMALTRAIN);
        job_to_labor_table[df::job_type::TrainWarAnimal]		= jlf_const(df::unit_labor::ANIMALTRAIN);
        job_to_labor_table[df::job_type::MakeWeapon]			= jlf_make_weapon;
        job_to_labor_table[df::job_type::ForgeAnvil]			= jlf_make_furniture;
        job_to_labor_table[df::job_type::ConstructCatapultParts] = jlf_const(df::unit_labor::SIEGECRAFT);
        job_to_labor_table[df::job_type::ConstructBallistaParts] = jlf_const(df::unit_labor::SIEGECRAFT);
        job_to_labor_table[df::job_type::MakeArmor]				= jlf_make_armor;
        job_to_labor_table[df::job_type::MakeHelm]				= jlf_make_armor;
        job_to_labor_table[df::job_type::MakePants]				= jlf_make_armor;
        job_to_labor_table[df::job_type::StudWith]				= jlf_make_object;
        job_to_labor_table[df::job_type::ButcherAnimal]			= jlf_const(df::unit_labor::BUTCHER);
        job_to_labor_table[df::job_type::PrepareRawFish]		= jlf_const(df::unit_labor::CLEAN_FISH);
        job_to_labor_table[df::job_type::MillPlants]			= jlf_const(df::unit_labor::MILLER);
        job_to_labor_table[df::job_type::BaitTrap]				= jlf_const(df::unit_labor::TRAPPER);
        job_to_labor_table[df::job_type::MilkCreature]			= jlf_const(df::unit_labor::MILK);
        job_to_labor_table[df::job_type::MakeCheese]			= jlf_const(df::unit_labor::MAKE_CHEESE);
        job_to_labor_table[df::job_type::ProcessPlants]			= jlf_const(df::unit_labor::PROCESS_PLANT);
        job_to_labor_table[df::job_type::ProcessPlantsBag]		= jlf_const(df::unit_labor::PROCESS_PLANT);
        job_to_labor_table[df::job_type::ProcessPlantsVial]		= jlf_const(df::unit_labor::PROCESS_PLANT);
        job_to_labor_table[df::job_type::ProcessPlantsBarrel]	= jlf_const(df::unit_labor::PROCESS_PLANT);
        job_to_labor_table[df::job_type::PrepareMeal]			= jlf_const(df::unit_labor::COOK);
        job_to_labor_table[df::job_type::WeaveCloth]			= jlf_const(df::unit_labor::WEAVER);
        job_to_labor_table[df::job_type::MakeGloves]			= jlf_make_armor;
        job_to_labor_table[df::job_type::MakeShoes]				= jlf_make_armor;
        job_to_labor_table[df::job_type::MakeShield]			= jlf_make_armor;
        job_to_labor_table[df::job_type::MakeCage]				= jlf_make_furniture;
        job_to_labor_table[df::job_type::MakeChain]				= jlf_make_object;
        job_to_labor_table[df::job_type::MakeFlask]				= jlf_make_object;
        job_to_labor_table[df::job_type::MakeGoblet]			= jlf_make_object;
        job_to_labor_table[df::job_type::MakeInstrument]		= jlf_make_object;
        job_to_labor_table[df::job_type::MakeToy]				= jlf_make_object;
        job_to_labor_table[df::job_type::MakeAnimalTrap]		= jlf_const(df::unit_labor::TRAPPER);
        job_to_labor_table[df::job_type::MakeBarrel]			= jlf_make_furniture;
        job_to_labor_table[df::job_type::MakeBucket]			= jlf_make_furniture;
        job_to_labor_table[df::job_type::MakeWindow]			= jlf_make_furniture;
        job_to_labor_table[df::job_type::MakeTotem]				= jlf_const(df::unit_labor::BONE_CARVE);
        job_to_labor_table[df::job_type::MakeAmmo]				= jlf_make_weapon;
        job_to_labor_table[df::job_type::DecorateWith]			= jlf_make_object;
        job_to_labor_table[df::job_type::MakeBackpack]			= jlf_make_object;
        job_to_labor_table[df::job_type::MakeQuiver]			= jlf_make_armor;
        job_to_labor_table[df::job_type::MakeBallistaArrowHead] = jlf_make_weapon;
        job_to_labor_table[df::job_type::AssembleSiegeAmmo]		= jlf_const(df::unit_labor::SIEGECRAFT);
        job_to_labor_table[df::job_type::LoadCatapult]			= jlf_const(df::unit_labor::SIEGEOPERATE);
        job_to_labor_table[df::job_type::LoadBallista]			= jlf_const(df::unit_labor::SIEGEOPERATE);
        job_to_labor_table[df::job_type::FireCatapult]			= jlf_const(df::unit_labor::SIEGEOPERATE);
        job_to_labor_table[df::job_type::FireBallista]			= jlf_const(df::unit_labor::SIEGEOPERATE);
        job_to_labor_table[df::job_type::ConstructMechanisms]	= jlf_const(df::unit_labor::MECHANIC);
        job_to_labor_table[df::job_type::MakeTrapComponent]		= jlf_const(df::unit_labor::MECHANIC) ;
        job_to_labor_table[df::job_type::LoadCageTrap]			= jlf_const(df::unit_labor::MECHANIC) ;
        job_to_labor_table[df::job_type::LoadStoneTrap]			= jlf_const(df::unit_labor::MECHANIC) ;
        job_to_labor_table[df::job_type::LoadWeaponTrap]		= jlf_const(df::unit_labor::MECHANIC) ;
        job_to_labor_table[df::job_type::CleanTrap]				= jlf_const(df::unit_labor::MECHANIC) ;
        job_to_labor_table[df::job_type::CastSpell]				= jlf_no_labor;
        job_to_labor_table[df::job_type::LinkBuildingToTrigger] = jlf_const(df::unit_labor::MECHANIC) ;
        job_to_labor_table[df::job_type::PullLever]				= jlf_no_labor;
        job_to_labor_table[df::job_type::BrewDrink]				= jlf_const(df::unit_labor::BREWER) ;
        job_to_labor_table[df::job_type::ExtractFromPlants]		= jlf_const(df::unit_labor::HERBALIST) ;
        job_to_labor_table[df::job_type::ExtractFromRawFish]	= jlf_const(df::unit_labor::DISSECT_FISH) ;
        job_to_labor_table[df::job_type::ExtractFromLandAnimal] = jlf_const(df::unit_labor::DISSECT_VERMIN) ;
        job_to_labor_table[df::job_type::TameVermin]			= jlf_const(df::unit_labor::ANIMALTRAIN) ;
        job_to_labor_table[df::job_type::TameAnimal]			= jlf_const(df::unit_labor::ANIMALTRAIN) ;
        job_to_labor_table[df::job_type::ChainAnimal]			= jlf_no_labor;
        job_to_labor_table[df::job_type::UnchainAnimal]			= jlf_no_labor;
        job_to_labor_table[df::job_type::UnchainPet]			= jlf_no_labor;
        job_to_labor_table[df::job_type::ReleaseLargeCreature]	= jlf_no_labor;
        job_to_labor_table[df::job_type::ReleasePet]			= jlf_no_labor;
        job_to_labor_table[df::job_type::ReleaseSmallCreature]	= jlf_no_labor;
        job_to_labor_table[df::job_type::HandleSmallCreature]	= jlf_no_labor;
        job_to_labor_table[df::job_type::HandleLargeCreature]	= jlf_no_labor;
        job_to_labor_table[df::job_type::CageLargeCreature]		= jlf_no_labor;
        job_to_labor_table[df::job_type::CageSmallCreature]		= jlf_no_labor;
        job_to_labor_table[df::job_type::RecoverWounded]		= jlf_const(df::unit_labor::RECOVER_WOUNDED);
        job_to_labor_table[df::job_type::DiagnosePatient]		= jlf_const(df::unit_labor::DIAGNOSE) ;
        job_to_labor_table[df::job_type::ImmobilizeBreak]		= jlf_const(df::unit_labor::BONE_SETTING) ;
        job_to_labor_table[df::job_type::DressWound]			= jlf_const(df::unit_labor::DRESSING_WOUNDS) ;
        job_to_labor_table[df::job_type::CleanPatient]			= jlf_const(df::unit_labor::CLEAN) ;
        job_to_labor_table[df::job_type::Surgery]				= jlf_const(df::unit_labor::SURGERY) ;
        job_to_labor_table[df::job_type::Suture]				= jlf_const(df::unit_labor::SUTURING);
        job_to_labor_table[df::job_type::SetBone]				= jlf_const(df::unit_labor::BONE_SETTING) ;
        job_to_labor_table[df::job_type::PlaceInTraction]		= jlf_const(df::unit_labor::BONE_SETTING) ;
        job_to_labor_table[df::job_type::DrainAquarium]			= jlf_no_labor;
        job_to_labor_table[df::job_type::FillAquarium]			= jlf_no_labor;
        job_to_labor_table[df::job_type::FillPond]				= jlf_no_labor;
        job_to_labor_table[df::job_type::GiveWater]				= jlf_const(df::unit_labor::FEED_WATER_CIVILIANS) ;
        job_to_labor_table[df::job_type::GiveFood]				= jlf_const(df::unit_labor::FEED_WATER_CIVILIANS) ;
        job_to_labor_table[df::job_type::GiveWater2]			= jlf_no_labor;
        job_to_labor_table[df::job_type::GiveFood2]				= jlf_no_labor;
        job_to_labor_table[df::job_type::RecoverPet]			= jlf_no_labor;
        job_to_labor_table[df::job_type::PitLargeAnimal]		= jlf_no_labor;
        job_to_labor_table[df::job_type::PitSmallAnimal]		= jlf_no_labor;
        job_to_labor_table[df::job_type::SlaughterAnimal]		= jlf_const(df::unit_labor::BUTCHER);
        job_to_labor_table[df::job_type::MakeCharcoal]			= jlf_const(df::unit_labor::BURN_WOOD);
        job_to_labor_table[df::job_type::MakeAsh]				= jlf_const(df::unit_labor::BURN_WOOD);
        job_to_labor_table[df::job_type::MakeLye]				= jlf_const(df::unit_labor::LYE_MAKING);
        job_to_labor_table[df::job_type::MakePotashFromLye]		= jlf_const(df::unit_labor::POTASH_MAKING);
        job_to_labor_table[df::job_type::FertilizeField]		= jlf_const(df::unit_labor::PLANT);
        job_to_labor_table[df::job_type::MakePotashFromAsh]		= jlf_const(df::unit_labor::POTASH_MAKING);
        job_to_labor_table[df::job_type::DyeThread]				= jlf_const(df::unit_labor::DYER);
        job_to_labor_table[df::job_type::DyeCloth]				= jlf_const(df::unit_labor::DYER);
        job_to_labor_table[df::job_type::SewImage]				= jlf_make_object;
        job_to_labor_table[df::job_type::MakePipeSection]		= jlf_make_furniture;
        job_to_labor_table[df::job_type::OperatePump]			= jlf_const(df::unit_labor::OPERATE_PUMP);
        job_to_labor_table[df::job_type::ManageWorkOrders]		= jlf_no_labor;
        job_to_labor_table[df::job_type::UpdateStockpileRecords] = jlf_no_labor;
        job_to_labor_table[df::job_type::TradeAtDepot]			= jlf_no_labor;
        job_to_labor_table[df::job_type::ConstructHatchCover]	= jlf_make_furniture;
        job_to_labor_table[df::job_type::ConstructGrate]		= jlf_make_furniture;
        job_to_labor_table[df::job_type::RemoveStairs]			= jlf_const(df::unit_labor::MINE);
        job_to_labor_table[df::job_type::ConstructQuern]		= jlf_make_furniture;
        job_to_labor_table[df::job_type::ConstructMillstone]	= jlf_make_furniture;
        job_to_labor_table[df::job_type::ConstructSplint]		= jlf_make_furniture;
        job_to_labor_table[df::job_type::ConstructCrutch]		= jlf_make_furniture;
        job_to_labor_table[df::job_type::ConstructTractionBench] = jlf_const(df::unit_labor::MECHANIC);
        job_to_labor_table[df::job_type::CleanSelf]				= jlf_no_labor;
        job_to_labor_table[df::job_type::BringCrutch]			= jlf_no_labor;
        job_to_labor_table[df::job_type::ApplyCast]				= jlf_const(df::unit_labor::BONE_SETTING);
        job_to_labor_table[df::job_type::CustomReaction]        = new jlfunc_custom();
        job_to_labor_table[df::job_type::ConstructSlab]			= jlf_make_furniture;
        job_to_labor_table[df::job_type::EngraveSlab]			= jlf_const(df::unit_labor::DETAIL);
        job_to_labor_table[df::job_type::ShearCreature]			= jlf_const(df::unit_labor::SHEARER);
        job_to_labor_table[df::job_type::SpinThread]			= jlf_const(df::unit_labor::SPINNER);
        job_to_labor_table[df::job_type::PenLargeAnimal]		= jlf_no_labor;
        job_to_labor_table[df::job_type::PenSmallAnimal]		= jlf_no_labor;
        job_to_labor_table[df::job_type::MakeTool]				= jlf_make_furniture;
        job_to_labor_table[df::job_type::CollectClay]			= jlf_const(df::unit_labor::POTTERY);
        job_to_labor_table[df::job_type::InstallColonyInHive]	= jlf_const(df::unit_labor::BEEKEEPING);
        job_to_labor_table[df::job_type::CollectHiveProducts]	= jlf_const(df::unit_labor::BEEKEEPING);
        job_to_labor_table[df::job_type::CauseTrouble]			= jlf_no_labor;
        job_to_labor_table[df::job_type::DrinkBlood]			= jlf_no_labor;
        job_to_labor_table[df::job_type::ReportCrime]			= jlf_no_labor;
        job_to_labor_table[df::job_type::ExecuteCriminal]		= jlf_no_labor;
        job_to_labor_table[df::job_type::TrainAnimal]			= jlf_const(df::unit_labor::ANIMALTRAIN);
        job_to_labor_table[df::job_type::CarveTrack]			= jlf_const(df::unit_labor::DETAIL);
        job_to_labor_table[df::job_type::PushTrackVehicle]		= jlf_const(df::unit_labor::PUSH_HAUL_VEHICLE);
        job_to_labor_table[df::job_type::PlaceTrackVehicle]		= jlf_const(df::unit_labor::PUSH_HAUL_VEHICLE);
        job_to_labor_table[df::job_type::StoreItemInVehicle]    = jlf_const(df::unit_labor::PUSH_HAUL_VEHICLE);
    };

    df::unit_labor find_job_labor(df::job* j)
    {
        if (j->job_type == df::job_type::CustomReaction)
        {
            for (auto r = world->raws.reactions.begin(); r != world->raws.reactions.end(); r++)
            {
                if ((*r)->code == j->reaction_name)
                {
                    df::job_skill skill = (*r)->skill;
                    return ENUM_ATTR(job_skill, labor, skill);
                }
            }
            return df::unit_labor::NONE;
        } 


        df::unit_labor labor;
        labor = job_to_labor_table[j->job_type]->get_labor(j);

        return labor;
    }
};

/* End of labor deducer */

static JobLaborMapper* labor_mapper = 0;

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
    labor_infos[labor].set_priority(default_labor_infos[labor].priority);
    labor_infos[labor].set_maximum_dwarfs(default_labor_infos[labor].maximum_dwarfs);
}

static void init_state()
{
    config = World::GetPersistentData("autolabor/2.0/config");
    if (config.isValid() && config.ival(0) == -1)
        config.ival(0) = 0;

    enable_autolabor = isOptionEnabled(CF_ENABLED);

    if (!enable_autolabor)
        return;

    // Load labors from save
    labor_infos.resize(ARRAY_COUNT(default_labor_infos));

    std::vector<PersistentDataItem> items;
    World::GetPersistentData(&items, "autolabor/2.0/labors/", true);

    for (auto p = items.begin(); p != items.end(); p++)
    {
        string key = p->key();
        df::unit_labor labor = (df::unit_labor) atoi(key.substr(strlen("autolabor/2.0/labors/")).c_str());
        if (labor >= 0 && labor <= labor_infos.size())
        {
            labor_infos[labor].config = *p;
            labor_infos[labor].active_dwarfs = 0;
        }
    }

    // Add default labors for those not in save
    for (int i = 0; i < ARRAY_COUNT(default_labor_infos); i++) {
        if (labor_infos[i].config.isValid())
            continue;

        std::stringstream name;
        name << "autolabor/2.0/labors/" << i;

        labor_infos[i].config = World::AddPersistentData(name.str());
        labor_infos[i].mark_assigned();
        labor_infos[i].active_dwarfs = 0;
        reset_labor((df::unit_labor) i);
    }

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
        config = World::AddPersistentData("autolabor/2.0/config");
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
        "  autolabor max <labor> <maximum>\n"
        "    Set max number of dwarves assigned to a labor.\n"
        "  autolabor max <labor> none\n"
        "    Unrestrict the number of dwarves assigned to a labor.\n"
        "  autolabor priority <labor> <priority>\n"
        "    Change the assignment priority of a labor (default is 100)\n"
        "  autolabor reset <labor>\n"
        "    Return a labor to the default handling.\n"
        "  autolabor reset-all\n"
        "    Return all labors to the default handling.\n"
        "  autolabor list\n"
        "    List current status of all labors.\n"
        "  autolabor status\n"
        "    Show basic status information.\n"
        "Function:\n"
        "  When enabled, autolabor periodically checks your dwarves and enables or\n"
        "  disables labors.  Generally, each dwarf will be assigned exactly one labor.\n"
        "  Warning: autolabor will override any manual changes you make to labors\n"
        "  while it is enabled.\n"
        ));

    generate_labor_to_skill_map();

    labor_mapper = new JobLaborMapper();

    init_state();

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    cleanup_state();

    delete labor_mapper;

    return CR_OK;
}

class AutoLaborManager {
    color_ostream& out;

public: 
    AutoLaborManager(color_ostream& o) : out(o)
    {
        dwarf_info.clear();
    }

    ~AutoLaborManager()
    {
        for (auto d = dwarf_info.begin(); d != dwarf_info.end(); d++)
        {
            delete (*d);
        }
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

    int cnt_recover_wounded;
    int cnt_diagnosis;
    int cnt_immobilize;	
    int cnt_dressing;
    int cnt_cleaning;
    int cnt_surgery;
    int cnt_suture;
    int cnt_setting;
    int cnt_traction;
    int cnt_crutch;

    int need_food_water;

    std::map<df::unit_labor, int> labor_needed;
    std::map<df::unit_labor, bool> labor_outside;
    std::vector<dwarf_info_t*> dwarf_info;
    std::list<dwarf_info_t*> available_dwarfs;

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
        F(in_building); F(construction); F(artifact);
#undef F

        auto& v = world->items.all;
        for (auto i = v.begin(); i != v.end(); i++)
        {
            df::item* item = *i;

            if (item->flags.bits.dump)
                labor_needed[df::unit_labor::HAUL_REFUSE]++;

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
        labor_needed.clear();

        for (df::job_list_link* jll = world->job_list.next; jll; jll = jll->next)
        {
            df::job* j = jll->item;
            if (!j) 
                continue;

            if (j->flags.bits.suspend || j->flags.bits.item_lost)
                continue;

            int worker = -1;
            int bld = -1;

            for (int r = 0; r < j->general_refs.size(); ++r)
            {
                if (j->general_refs[r]->getType() == df::general_ref_type::UNIT_WORKER)
                    worker = ((df::general_ref_unit_workerst *)(j->general_refs[r]))->unit_id;
                if (j->general_refs[r]->getType() == df::general_ref_type::BUILDING_HOLDER)
                    bld = ((df::general_ref_building_holderst *)(j->general_refs[r]))->building_id;
            }

            if (bld != -1) 
            {
                df::building* b = binsearch_in_vector(world->buildings.all, bld);
                int fjid = -1;
                for (int jn = 0; jn < b->jobs.size(); jn++)
                {
                    if (b->jobs[jn]->flags.bits.suspend)
                        continue;
                    fjid = b->jobs[jn]->id;
                    break;
                }
                // check if this job is the first nonsuspended job on this building; if not, ignore it
                // (except for farms)
                if (fjid != j->id && b->getType() != df::building_type::FarmPlot) {
                    continue;
                }

            }

            df::unit_labor labor = labor_mapper->find_job_labor (j);

            if (labor != df::unit_labor::NONE) 
            {
                labor_needed[labor]++;

                if (worker != -1)
                    labor_infos[labor].mark_assigned();

                if (j->pos.isValid())
                {
                    df::tile_designation* d = Maps::getTileDesignation(j->pos);
                    if (d->bits.outside)
                        labor_outside[labor] = true;
                }
            }
        }

    }

    void collect_dwarf_list()
    {

        for (auto u = world->units.active.begin(); u != world->units.active.end(); ++u)
        {
            df::unit* cre = *u;

            if (Units::isCitizen(cre))
            {
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

                // check to see if dwarf has minor children

                for (auto u2 = world->units.active.begin(); u2 != world->units.active.end(); ++u2)
                {
                    if ((*u2)->relations.mother_id == dwarf->dwarf->id &&
                        !(*u2)->flags1.bits.dead &&
                        ((*u2)->profession == df::profession::CHILD || (*u2)->profession == df::profession::BABY))
                    {
                        dwarf->has_children = true;
                        if (print_debug)
                            out.print("Dwarf %s has minor children\n", dwarf->dwarf->name.first_name.c_str());
                        break;
                    }
                }

                // Find the activity state for each dwarf

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
                else if (dwarf->dwarf->military.cur_uniform != 0)
                    state = MILITARY;
                else if (dwarf->dwarf->job.current_job == NULL)
                {
                    if (is_on_break)
                        state = OTHER;
                    else if (dwarf->dwarf->burrows.size() > 0)
                        state = OTHER;        // dwarfs assigned to burrows are treated as if permanently busy
                    else if (dwarf->dwarf->status2.able_grasp_impair == 0)
                    {
                        state = OTHER;      // dwarfs unable to grasp are incapable of nearly all labors
                        dwarf->clear_all = true;
                        if (print_debug)
                            out.print ("Dwarf %s is disabled, will not be assigned labors\n", dwarf->dwarf->name.first_name.c_str());
                    }
                    else
                        state = IDLE;
                }
                else
                {
                    df::job_type job = dwarf->dwarf->job.current_job->job_type;
                    if (job >= 0 && job < ARRAY_COUNT(dwarf_states))
                        state = dwarf_states[job];
                    else
                    {
                        out.print("Dwarf \"%s\" has unknown job %i\n", dwarf->dwarf->name.first_name.c_str(), job);
                        state = OTHER;
                    }
                    if (state == BUSY)
                    {
                        df::unit_labor labor = labor_mapper->find_job_labor(dwarf->dwarf->job.current_job);
                        if (labor != df::unit_labor::NONE)
                        {
                            dwarf->using_labor = labor;
                            if (!dwarf->dwarf->status.labors[labor] && print_debug)
                            {
                                out.print("AUTOLABOR: dwarf %s (id %d) is doing job %s(%d) but is not enabled for labor %s(%d).\n",
                                    dwarf->dwarf->name.first_name.c_str(), dwarf->dwarf->id,
                                    ENUM_KEY_STR(job_type, job).c_str(), job, ENUM_KEY_STR(unit_labor, labor).c_str(), labor);
                            }
                        }
                    }
                }

                dwarf->state = state;

                if (print_debug)
                    out.print("Dwarf \"%s\": state %s %d\n", dwarf->dwarf->name.first_name.c_str(), state_names[dwarf->state], dwarf->clear_all);

                // determine if dwarf has medical needs
                if (dwarf->dwarf->health) 
                {
                    if (dwarf->dwarf->health->flags.bits.needs_recovery) 
                        cnt_recover_wounded++;
                    if (dwarf->dwarf->health->flags.bits.rq_diagnosis) 
                        cnt_diagnosis++;
                    if (dwarf->dwarf->health->flags.bits.rq_immobilize) 
                        cnt_immobilize++;
                    if (dwarf->dwarf->health->flags.bits.rq_dressing) 
                        cnt_dressing++;
                    if (dwarf->dwarf->health->flags.bits.rq_cleaning) 
                        cnt_cleaning++;
                    if (dwarf->dwarf->health->flags.bits.rq_surgery) 
                        cnt_surgery++;
                    if (dwarf->dwarf->health->flags.bits.rq_suture) 
                        cnt_suture++;
                    if (dwarf->dwarf->health->flags.bits.rq_setting) 
                        cnt_setting++;
                    if (dwarf->dwarf->health->flags.bits.rq_traction) 
                        cnt_traction++;
                    if (dwarf->dwarf->health->flags.bits.rq_crutch) 
                        cnt_crutch++;
                }

                if (dwarf->dwarf->counters2.hunger_timer > 60000 || dwarf->dwarf->counters2.thirst_timer > 40000)
                    need_food_water++;

                // find dwarf's highest effective skill

                int high_skill = 0;

                FOR_ENUM_ITEMS (unit_labor, labor)
                {
                    if (labor == df::unit_labor::NONE)
                        continue;

                    df::job_skill skill = labor_to_skill[labor];
                    if (skill != df::job_skill::NONE)
                    {
                        int	skill_level = Units::getNominalSkill(dwarf->dwarf, skill, false);
                        high_skill = std::max(high_skill, skill_level);
                    }
                }

                dwarf->high_skill = high_skill;
                // check if dwarf has an axe, pick, or crossbow

                for (int j = 0; j < dwarf->dwarf->inventory.size(); j++)
                {
                    df::unit_inventory_item* ui = dwarf->dwarf->inventory[j];
                    if (ui->mode == df::unit_inventory_item::Weapon && ui->item->isWeapon())
                    {
                        dwarf->armed = true;
                        df::itemdef_weaponst* weapondef = ((df::item_weaponst*)(ui->item))->subtype;
                        df::job_skill weaponsk = (df::job_skill) weapondef->skill_melee;
                        df::job_skill rangesk = (df::job_skill) weapondef->skill_ranged;
                        if (weaponsk == df::job_skill::AXE)
                        {
                            dwarf->has_axe = 1;
                            if (state != IDLE) 
                                axe_count--;
                        }
                        else if (weaponsk == df::job_skill::MINING)
                        {
                            dwarf->has_pick = 1;
                            if (state != IDLE) 
                                pick_count--;
                        }
                        else if (rangesk == df::job_skill::CROSSBOW)
                        {
                            dwarf->has_crossbow = 1;
                        }
                    }
                }

                // clear labors of dwarfs with clear_all set

                if (dwarf->clear_all) 
                {
                    FOR_ENUM_ITEMS(unit_labor, labor)
                    {
                        if (labor == unit_labor::NONE) 
                            continue;

                        dwarf->clear_labor(labor);
                    }
                }
                else if (state == IDLE || state == BUSY)
                    available_dwarfs.push_back(dwarf);

            }

        }
    }			

public:
    void process()
    {
        dwarf_info.clear();

        dig_count = tree_count = plant_count = detail_count = pick_count = axe_count = 0;
        cnt_recover_wounded = cnt_diagnosis = cnt_immobilize = cnt_dressing = cnt_cleaning = cnt_surgery = cnt_suture =
            cnt_setting = cnt_traction = cnt_crutch = 0;
        need_food_water = 0;

        trader_requested = false;

        FOR_ENUM_ITEMS(unit_labor, l)
        {
            if (l == df::unit_labor::NONE)
                continue;

            labor_infos[l].active_dwarfs = 0;
        }

        // scan for specific buildings of interest

        scan_buildings();

        // count number of squares designated for dig, wood cutting, detailing, and plant harvesting

        count_map_designations();

        // collect current job list 

        collect_job_list();

        // count number of picks and axes available for use

        count_tools();

        // collect list of dwarfs

        collect_dwarf_list();

        // add job entries for designation-related jobs

        labor_needed[df::unit_labor::MINE]      += std::min(pick_count, dig_count);
        labor_needed[df::unit_labor::CUTWOOD]   += std::min(axe_count, tree_count);
        labor_needed[df::unit_labor::DETAIL]    += detail_count;
        labor_needed[df::unit_labor::HERBALIST] += plant_count;

        // add job entries for health care

        labor_needed[df::unit_labor::RECOVER_WOUNDED] += cnt_recover_wounded;
        labor_needed[df::unit_labor::DIAGNOSE]		  += cnt_diagnosis;
        labor_needed[df::unit_labor::BONE_SETTING]    += cnt_immobilize;
        labor_needed[df::unit_labor::DRESSING_WOUNDS] += cnt_dressing;
        labor_needed[df::unit_labor::CLEAN]			  += cnt_cleaning;
        labor_needed[df::unit_labor::SURGERY]         += cnt_surgery;
        labor_needed[df::unit_labor::SUTURING]        += cnt_suture;
        labor_needed[df::unit_labor::BONE_SETTING]    += cnt_setting;
        labor_needed[df::unit_labor::BONE_SETTING]    += cnt_traction;
        labor_needed[df::unit_labor::HAUL_ITEM]       += cnt_crutch;

        labor_needed[df::unit_labor::FEED_WATER_CIVILIANS] += need_food_water;

        // add entries for hauling jobs

        labor_needed[df::unit_labor::HAUL_STONE]     += world->stockpile.num_jobs[1];
        labor_needed[df::unit_labor::HAUL_WOOD]      += world->stockpile.num_jobs[2];
        labor_needed[df::unit_labor::HAUL_ITEM]      += world->stockpile.num_jobs[3];
        labor_needed[df::unit_labor::HAUL_BODY]      += world->stockpile.num_jobs[5];
        labor_needed[df::unit_labor::HAUL_FOOD]      += world->stockpile.num_jobs[6];
        labor_needed[df::unit_labor::HAUL_REFUSE]    += world->stockpile.num_jobs[7];
        labor_needed[df::unit_labor::HAUL_FURNITURE] += world->stockpile.num_jobs[8];
        labor_needed[df::unit_labor::HAUL_ANIMAL]    += world->stockpile.num_jobs[9];

        // add entries for vehicle hauling

        for (auto v = world->vehicles.all.begin(); v != world->vehicles.all.end(); v++)
            if ((*v)->route_id != -1) 
                labor_needed[df::unit_labor::PUSH_HAUL_VEHICLE]++;

        // add fishing & hunting

        if (isOptionEnabled(CF_ALLOW_FISHING) && has_fishery)
            labor_needed[df::unit_labor::FISH] ++;

        if (isOptionEnabled(CF_ALLOW_HUNTING) && has_butchers)
            labor_needed[df::unit_labor::HUNT] ++;

        /* add animal trainers */
        for (auto a = df::global::ui->equipment.training_assignments.begin();
            a != df::global::ui->equipment.training_assignments.end();
            a++)
        {
            labor_needed[df::unit_labor::ANIMALTRAIN]++;
            // note: this doesn't test to see if the trainer is actually needed, and thus will overallocate trainers.  bleah.
        }

        if (print_debug)
        {
            for (auto i = labor_needed.begin(); i != labor_needed.end(); i++)
            {
                out.print ("labor_needed [%s] = %d, outside = %d\n", ENUM_KEY_STR(unit_labor, i->first).c_str(), i->second,
                    labor_outside[i->first]);
            }	
        }

        priority_queue<pair<int, df::unit_labor>> pq;

        for (auto i = labor_needed.begin(); i != labor_needed.end(); i++)
        {
            df::unit_labor l = i->first;
            if (labor_infos[l].maximum_dwarfs() > 0 &&
                i->second > labor_infos[l].maximum_dwarfs())
                i->second = labor_infos[l].maximum_dwarfs();
            if (i->second > 0) 
            {
                int priority = labor_infos[l].priority();
                priority += labor_infos[l].time_since_last_assigned()/12;
                pq.push(make_pair(priority, l));
            }
        }

        if (print_debug)
            out.print("available count = %d, distinct labors needed = %d\n", available_dwarfs.size(), pq.size());

        std::map<df::unit_labor, int> to_assign;

        to_assign.clear();

        int av = available_dwarfs.size();

        while (!pq.empty() && av > 0) 
        {
            df::unit_labor labor = pq.top().second;
            int priority = pq.top().first;
            to_assign[labor]++;
            pq.pop();
            av--;

            if (print_debug)
                out.print("Will assign: %s priority %d (%d)\n", ENUM_KEY_STR(unit_labor, labor).c_str(), priority, to_assign[labor]);

            if (--labor_needed[labor] > 0) 
            {
                priority /= 2;
                pq.push(make_pair(priority, labor));
            }

        }

        int canary = (1 << df::unit_labor::HAUL_STONE) |
            (1 << df::unit_labor::HAUL_WOOD) |
            (1 << df::unit_labor::HAUL_BODY) |
            (1 << df::unit_labor::HAUL_FOOD) |
            (1 << df::unit_labor::HAUL_REFUSE) |
            (1 << df::unit_labor::HAUL_ITEM) |
            (1 << df::unit_labor::HAUL_FURNITURE) |
            (1 << df::unit_labor::HAUL_ANIMAL);

        while (!available_dwarfs.empty())
        {
            std::list<dwarf_info_t*>::iterator bestdwarf = available_dwarfs.begin();

            int best_score = INT_MIN;
            df::unit_labor best_labor = df::unit_labor::NONE;

            for (auto j = to_assign.begin(); j != to_assign.end(); j++) 
            {
                if (j->second <= 0) 
                    continue;

                df::unit_labor labor = j->first;
                df::job_skill skill = labor_to_skill[labor];

                for (std::list<dwarf_info_t*>::iterator k = available_dwarfs.begin(); k != available_dwarfs.end(); k++)
                {
                    dwarf_info_t* d = (*k);
                    int skill_level = 0;
                    int xp = 0;
                    if (skill != df::job_skill::NONE) 
                    {
                        skill_level = Units::getEffectiveSkill(d->dwarf, skill);
                        xp = Units::getExperience(d->dwarf, skill, false);
                    }
                    int score = skill_level * 100 - (d->high_skill - skill_level) * 500 + (xp / (skill_level + 5));
                    if (d->dwarf->status.labors[labor])
                        score += 500;
                    if ((labor == df::unit_labor::MINE && d->has_pick) ||
                        (labor == df::unit_labor::CUTWOOD && d->has_axe) ||
                        (labor == df::unit_labor::HUNT && d->has_crossbow))
                        score += 500;
                    if (d->has_children && labor_outside[labor])
                        score -= 5000;
                    if (d->armed && labor_outside[labor])
                        score += 1000;
                    if (d->state == BUSY && d->using_labor == labor)
                        score += 5000;
                    if (score > best_score)
                    {
                        bestdwarf = k;
                        best_score = score;
                        best_labor = labor;
                    }
                }
            }

            if (print_debug)
                out.print("assign \"%s\" labor %s score=%d\n", (*bestdwarf)->dwarf->name.first_name.c_str(), ENUM_KEY_STR(unit_labor, best_labor).c_str(), best_score);

            FOR_ENUM_ITEMS(unit_labor, l)
            {
                if (l == df::unit_labor::NONE)
                    continue;

                if (l == best_labor)
                    (*bestdwarf)->set_labor(l);
                else
                    (*bestdwarf)->clear_labor(l);
            }

            if (best_labor >= df::unit_labor::HAUL_STONE && best_labor <= df::unit_labor::HAUL_ANIMAL)
                canary &= ~(1 << best_labor);

            if (best_labor != df::unit_labor::NONE)
            {
                labor_infos[best_labor].active_dwarfs++;
                to_assign[best_labor]--;
            }

            available_dwarfs.erase(bestdwarf);
        }

        if (canary != 0)
        {
            dwarf_info_t* d = dwarf_info.front();
            FOR_ENUM_ITEMS (unit_labor, l)
            {
                if (l >= df::unit_labor::HAUL_STONE && l <= df::unit_labor::HAUL_ANIMAL &&
                    canary & (1 << l))
                    d->set_labor(l);
            }
            if (print_debug)
                out.print ("Setting %s as the hauling canary\n", d->dwarf->name.first_name.c_str());
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

    debug_stream = &out;
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
    out << "priority " << labor_infos[labor].priority()
        << ", maximum " << labor_infos[labor].maximum_dwarfs()
        << ", currently " << labor_infos[labor].active_dwarfs << " dwarfs" << endl;
}

df::unit_labor lookup_labor_by_name (std::string& name)
{
    df::unit_labor labor = df::unit_labor::NONE;

    FOR_ENUM_ITEMS(unit_labor, test_labor)
    {
        if (name == ENUM_KEY_STR(unit_labor, test_labor))
            labor = test_labor;
    }

    return labor;
}


command_result autolabor (color_ostream &out, std::vector <std::string> & parameters)
{
    CoreSuspender suspend;

    if (!Core::getInstance().isWorldLoaded()) {
        out.printerr("World is not loaded: please load a game first.\n");
        return CR_FAILURE;
    }

    if (parameters.size() == 1 &&
        (parameters[0] == "enable" || parameters[0] == "disable"))
    {
        bool enable = (parameters[0] == "enable");
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
    else if (parameters.size() == 3 && 
        (parameters[0] == "max" || parameters[0] == "priority"))
    {
        if (!enable_autolabor)
        {
            out << "Error: The plugin is not enabled." << endl;
            return CR_FAILURE;
        }

        df::unit_labor labor = lookup_labor_by_name(parameters[1]);

        if (labor == df::unit_labor::NONE)
        {
            out.printerr("Could not find labor %s.\n", parameters[0].c_str());
            return CR_WRONG_USAGE;
        }

        int v;

        if (parameters[2] == "none")
            v = 0;
        else 
            v = atoi (parameters[2].c_str());

        if (parameters[0] == "max")
            labor_infos[labor].set_maximum_dwarfs(v);
        else if (parameters[0] == "priority")
            labor_infos[labor].set_priority(v);

        print_labor(labor, out);
        return CR_OK;
    }
    else if (parameters.size() == 2 && parameters[0] == "reset")
    {
        if (!enable_autolabor)
        {
            out << "Error: The plugin is not enabled." << endl;
            return CR_FAILURE;
        }

        df::unit_labor labor = lookup_labor_by_name(parameters[1]);

        if (labor == df::unit_labor::NONE)
        {
            out.printerr("Could not find labor %s.\n", parameters[0].c_str());
            return CR_WRONG_USAGE;
        }
        reset_labor(labor);
        print_labor(labor, out);
        return CR_OK;
    }
    else if (parameters.size() == 1 && (parameters[0] == "allow-fishing" || parameters[0] == "forbid-fishing"))
    {
        if (!enable_autolabor)
        {
            out << "Error: The plugin is not enabled." << endl;
            return CR_FAILURE;
        }

        setOptionEnabled(CF_ALLOW_FISHING, (parameters[0] == "allow-fishing"));
        return CR_OK;
    }
    else if (parameters.size() == 1 && (parameters[0] == "allow-hunting" || parameters[0] == "forbid-hunting"))
    {
        if (!enable_autolabor)
        {
            out << "Error: The plugin is not enabled." << endl;
            return CR_FAILURE;
        }

        setOptionEnabled(CF_ALLOW_HUNTING, (parameters[0] == "allow-hunting"));
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
            "Activate with 'autolabor enable', deactivate with 'autolabor disable'.\n"
            "Current state: %s.\n", enable_autolabor ? "enabled" : "disabled");

        return CR_OK;
    }
}

