// This is a generic plugin that does nothing useful apart from acting as an example... of a plugin that does nothing :D

// some headers required for a plugin. Nothing special, just the basics.
#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

#include <vector>
#include <algorithm>

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

using namespace DFHack;
using namespace df::enums;
using df::global::ui;
using df::global::world;

#define ARRAY_COUNT(array) (sizeof(array)/sizeof((array)[0]))

static int enable_autolabor = 0;

static bool print_debug = 0;


// Here go all the command declarations...
// mostly to allow having the mandatory stuff on top of the file and commands on the bottom
command_result autolabor (color_ostream &out, std::vector <std::string> & parameters);

// A plugin must be able to return its name and version.
// The name string provided must correspond to the filename - autolabor.plug.so or autolabor.plug.dll in this case
DFHACK_PLUGIN("autolabor");

enum labor_mode {
	DISABLE,
	HAULERS,
	EVERYONE,
	MINIMUM,
	EXACTLY,
	MAXIMUM
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
	OTHER /* ExecuteCriminal */
};

struct labor_info
{
	labor_mode mode;
	bool is_exclusive;
	int minimum_dwarfs;
};

static struct labor_info* labor_infos;

static const struct labor_info default_labor_infos[] = {
    /* MINE */				{MINIMUM, true, 2},
    /* HAUL_STONE */		{HAULERS, false, 1},
    /* HAUL_WOOD */			{HAULERS, false, 1},
    /* HAUL_BODY */			{HAULERS, false, 1},
    /* HAUL_FOOD */			{HAULERS, false, 1},
    /* HAUL_REFUSE */		{HAULERS, false, 1},
    /* HAUL_ITEM */			{HAULERS, false, 1},
    /* HAUL_FURNITURE */	{HAULERS, false, 1},
    /* HAUL_ANIMAL */		{HAULERS, false, 1},
    /* CLEAN */				{HAULERS, false, 1},
    /* CUTWOOD */			{MINIMUM, true, 1},
    /* CARPENTER */			{MINIMUM, false, 1},
    /* DETAIL */			{MINIMUM, false, 1},
    /* MASON */				{MINIMUM, false, 1},
    /* ARCHITECT */			{MINIMUM, false, 1},
    /* ANIMALTRAIN */		{MINIMUM, false, 1},
    /* ANIMALCARE */		{MINIMUM, false, 1},
    /* DIAGNOSE */			{MINIMUM, false, 1},
    /* SURGERY */			{MINIMUM, false, 1},
    /* BONE_SETTING */		{MINIMUM, false, 1},
    /* SUTURING */			{MINIMUM, false, 1},
    /* DRESSING_WOUNDS */	{MINIMUM, false, 1},
	/* FEED_WATER_CIVILIANS */ {EVERYONE, false, 1},
    /* RECOVER_WOUNDED */	{HAULERS, false, 1},
    /* BUTCHER */			{MINIMUM, false, 1},
    /* TRAPPER */			{MINIMUM, false, 1},
    /* DISSECT_VERMIN */	{MINIMUM, false, 1},
    /* LEATHER */			{MINIMUM, false, 1},
    /* TANNER */			{MINIMUM, false, 1},
    /* BREWER */			{MINIMUM, false, 1},
    /* ALCHEMIST */			{MINIMUM, false, 1},
    /* SOAP_MAKER */		{MINIMUM, false, 1},
    /* WEAVER */			{MINIMUM, false, 1},
    /* CLOTHESMAKER */		{MINIMUM, false, 1},
    /* MILLER */			{MINIMUM, false, 1},
    /* PROCESS_PLANT */		{MINIMUM, false, 1},
    /* MAKE_CHEESE */		{MINIMUM, false, 1},
    /* MILK */				{MINIMUM, false, 1},
    /* COOK */				{MINIMUM, false, 1},
    /* PLANT */				{MINIMUM, false, 1},
    /* HERBALIST */			{MINIMUM, false, 1},
    /* FISH */				{EXACTLY, false, 1},
    /* CLEAN_FISH */		{MINIMUM, false, 1},
    /* DISSECT_FISH */		{MINIMUM, false, 1},
    /* HUNT */				{EXACTLY, true, 1},
    /* SMELT */				{MINIMUM, false, 1},
    /* FORGE_WEAPON */		{MINIMUM, false, 1},
    /* FORGE_ARMOR */		{MINIMUM, false, 1},
    /* FORGE_FURNITURE */	{MINIMUM, false, 1},
    /* METAL_CRAFT */		{MINIMUM, false, 1},
    /* CUT_GEM */			{MINIMUM, false, 1},
    /* ENCRUST_GEM */		{MINIMUM, false, 1},
    /* WOOD_CRAFT */		{MINIMUM, false, 1},
    /* STONE_CRAFT */		{MINIMUM, false, 1},
    /* BONE_CARVE */		{MINIMUM, false, 1},
    /* GLASSMAKER */		{MINIMUM, false, 1},
    /* EXTRACT_STRAND */	{MINIMUM, false, 1},
    /* SIEGECRAFT */		{MINIMUM, false, 1},
    /* SIEGEOPERATE */		{MINIMUM, false, 1},
    /* BOWYER */			{MINIMUM, false, 1},
    /* MECHANIC */			{MINIMUM, false, 1},
    /* POTASH_MAKING */		{MINIMUM, false, 1},
    /* LYE_MAKING */		{MINIMUM, false, 1},
    /* DYER */				{MINIMUM, false, 1},
    /* BURN_WOOD */			{MINIMUM, false, 1},
    /* OPERATE_PUMP */		{MINIMUM, false, 1},
    /* SHEARER */			{MINIMUM, false, 1},
    /* SPINNER */			{MINIMUM, false, 1},
    /* POTTERY */			{MINIMUM, false, 1},
    /* GLAZING */			{MINIMUM, false, 1},
    /* PRESSING */			{MINIMUM, false, 1},
    /* BEEKEEPING */		{MINIMUM, false, 1},
	/* WAX_WORKING */		{MINIMUM, false, 1},
};

static const df::job_skill noble_skills[] = {
	df::enums::job_skill::APPRAISAL,
	df::enums::job_skill::ORGANIZATION,
	df::enums::job_skill::RECORD_KEEPING,
};

struct dwarf_info_t
{
	int highest_skill;
	int total_skill;
	bool is_best_noble;
	int mastery_penalty;
	int assigned_jobs;
	dwarf_state state;
	bool has_exclusive_labor;
};

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    // initialize labor infos table from default table
    if(ARRAY_COUNT(default_labor_infos) != ENUM_LAST_ITEM(unit_labor) + 1)
        return CR_FAILURE;
    labor_infos = new struct labor_info[ARRAY_COUNT(default_labor_infos)];
    for (int i = 0; i < ARRAY_COUNT(default_labor_infos); i++) {
        labor_infos[i] = default_labor_infos[i];
    }
    // Fill the command list with your commands.
    commands.push_back(PluginCommand(
        "autolabor", "Automatically manage dwarf labors.",
        autolabor, false, /* true means that the command can't be used from non-interactive user interface */
        // Extended help string. Used by CR_WRONG_USAGE and the help command:
        "  autolabor enable\n"
		"  autolabor disable\n"
		"    Enables or disables the plugin.\n"
		"  autolabor miners <n>\n"
		"    Set number of desired miners (defaults to 2)\n"
		"Function:\n"
		"  When enabled, autolabor periodically checks your dwarves and enables or\n"
		"  disables labors. It tries to keep as many dwarves as possible busy but\n"
		"  also tries to have dwarves specialize in specific skills.\n"
		"  Warning: autolabor will override any manual changes you make to labors\n"
		"  while it is enabled.\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
	// release the labor info table;
	delete [] labor_infos;

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
        return labor_infos[i].mode < labor_infos[j].mode;
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

DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
	static int step_count = 0;
    // check run conditions
    if(!world->map.block_index || !enable_autolabor)
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

	for (int i = 0; i < world->buildings.all.size(); ++i)
	{
		df::building *build = world->buildings.all[i];
		auto type = build->getType();
		if (df::enums::building_type::Workshop == type)
		{
			auto subType = build->getSubtype();
			if (df::enums::workshop_type::Butchers == subType)
				has_butchers = true;
			if (df::enums::workshop_type::Fishery == subType)
				has_fishery = true;
		}
	}

    for (int i = 0; i < world->units.all.size(); ++i)
    {
        df::unit* cre = world->units.all[i];
		if (cre->race == race && cre->civ_id == civ && !cre->flags1.bits.marauder && !cre->flags1.bits.diplomat && !cre->flags1.bits.merchant &&
			!cre->flags1.bits.dead && !cre->flags1.bits.forest) {
			dwarfs.push_back(cre);
        }
    }

	int n_dwarfs = dwarfs.size();

	if (n_dwarfs == 0)
		return CR_OK;

	std::vector<dwarf_info_t> dwarf_info(n_dwarfs);

    std::vector<int> best_noble(ARRAY_COUNT(noble_skills));
    std::vector<int> highest_noble_skill(ARRAY_COUNT(noble_skills));
    std::vector<int> highest_noble_experience(ARRAY_COUNT(noble_skills));

	// Find total skill and highest skill for each dwarf. More skilled dwarves shouldn't be used for minor tasks.

	for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
	{
		assert(dwarfs[dwarf]->status.souls.size() > 0);

		for (auto s = dwarfs[dwarf]->status.souls[0]->skills.begin(); s != dwarfs[dwarf]->status.souls[0]->skills.end(); s++)
		{
			df::job_skill skill = (*s)->id;

			df::job_skill_class skill_class = ENUM_ATTR(job_skill, type, skill);

			int skill_level = (*s)->rating;
			int skill_experience = (*s)->experience;

			// Track the dwarfs with the best Appraisal, Organization, and Record Keeping skills.
			// They are likely to have appointed noble positions, so should be kept free where possible.

			int noble_skill_id = -1;
            for (int i = 0; i < ARRAY_COUNT(noble_skills); i++)
			{
				if (skill == noble_skills[i])
					noble_skill_id = i;
			}

			if (noble_skill_id >= 0)
			{
                assert(noble_skill_id < ARRAY_COUNT(noble_skills));

				if (highest_noble_skill[noble_skill_id] < skill_level ||
					(highest_noble_skill[noble_skill_id] == skill_level &&
						highest_noble_experience[noble_skill_id] < skill_experience))
				{
					highest_noble_skill[noble_skill_id] = skill_level;
					highest_noble_experience[noble_skill_id] = skill_experience;
					best_noble[noble_skill_id] = dwarf;
				}
			}

			// Track total & highest skill among normal/medical skills. (We don't care about personal or social skills.)

			if (skill_class != df::enums::job_skill_class::Normal && skill_class != df::enums::job_skill_class::Medical)
				continue;

			if (dwarf_info[dwarf].highest_skill < skill_level)
				dwarf_info[dwarf].highest_skill = skill_level;
			dwarf_info[dwarf].total_skill += skill_level;
		}
	}

	// Mark the best nobles, so we try to keep them non-busy. (It would be better to find the actual assigned nobles.)

	for (int i = 0; i < ARRAY_COUNT(noble_skills); i++)
	{
		assert(best_noble[i] >= 0);
		assert(best_noble[i] < n_dwarfs);

		dwarf_info[best_noble[i]].is_best_noble = true;
	}

	// Calculate a base penalty for using each dwarf for a task he isn't good at.

	for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
	{
		dwarf_info[dwarf].mastery_penalty -= 40 * dwarf_info[dwarf].highest_skill;
		dwarf_info[dwarf].mastery_penalty -= 10 * dwarf_info[dwarf].total_skill;
		if (dwarf_info[dwarf].is_best_noble)
			dwarf_info[dwarf].mastery_penalty -= 250;

		for (int labor = ENUM_FIRST_ITEM(unit_labor); labor <= ENUM_LAST_ITEM(unit_labor); labor++)
		{
			if (labor == df::enums::unit_labor::NONE)
				continue;

            /*
			assert(labor >= 0);
			assert(labor < ARRAY_COUNT(labor_infos));
			*/

			if (labor_infos[labor].is_exclusive && dwarfs[dwarf]->status.labors[labor])
				dwarf_info[dwarf].mastery_penalty -= 100;
		}
	}

	// Find the activity state for each dwarf. It's important to get this right - a dwarf who we think is IDLE but
	// can't work will gum everything up. In the future I might add code to auto-detect slacker dwarves.

	std::vector<int> state_count(5);

	for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
	{
		bool is_on_break = false;

		for (auto p = dwarfs[dwarf]->status.misc_traits.begin(); p < dwarfs[dwarf]->status.misc_traits.end(); p++)
		{
			// 7 / 0x7 = Newly arrived migrant, will not work yet
			// 17 / 0x11 = On break
			if ((*p)->id == 0x07 || (*p)->id == 0x11)
				is_on_break = true;
		}

		if (dwarfs[dwarf]->profession == df::enums::profession::BABY ||
			dwarfs[dwarf]->profession == df::enums::profession::CHILD ||
			dwarfs[dwarf]->profession == df::enums::profession::DRUNK)
		{
			dwarf_info[dwarf].state = CHILD;
		}
		else if (dwarfs[dwarf]->job.current_job == NULL)
		{
			if (ENUM_ATTR(profession, military, dwarfs[dwarf]->profession))
				dwarf_info[dwarf].state = MILITARY;
			else if (is_on_break)
				dwarf_info[dwarf].state = OTHER;
			else if (dwarfs[dwarf]->meetings.size() > 0)
				dwarf_info[dwarf].state = OTHER;
			else
				dwarf_info[dwarf].state = IDLE;
		}
		else
		{
			int job = dwarfs[dwarf]->job.current_job->job_type;

            /*
			assert(job >= 0);
			assert(job < ARRAY_COUNT(dwarf_states));
			*/

			dwarf_info[dwarf].state = dwarf_states[job];
		}

		state_count[dwarf_info[dwarf].state]++;

		if (print_debug)
			out.print("Dwarf %i \"%s\": penalty %i, state %s\n", dwarf, dwarfs[dwarf]->name.first_name.c_str(), dwarf_info[dwarf].mastery_penalty, state_names[dwarf_info[dwarf].state]);
	}

	// Generate labor -> skill mapping

	df::job_skill labor_to_skill[ENUM_LAST_ITEM(unit_labor) + 1];
	for (int i = 0; i <= ENUM_LAST_ITEM(unit_labor); i++)
		labor_to_skill[i] = df::enums::job_skill::NONE;

	FOR_ENUM_ITEMS(job_skill, skill)
	{
		int labor = ENUM_ATTR(job_skill, labor, skill);
		if (labor != df::enums::unit_labor::NONE)
		{
            /*
			assert(labor >= 0);
			assert(labor < ARRAY_COUNT(labor_to_skill));
			*/

			labor_to_skill[labor] = skill;
		}
	}

	std::vector<df::unit_labor> labors;

	FOR_ENUM_ITEMS(unit_labor, labor)
	{
		if (labor == df::enums::unit_labor::NONE)
			continue;

        /*
		assert(labor >= 0);
		assert(labor < ARRAY_COUNT(labor_infos));
		*/

		labors.push_back(labor);
	}
    laborinfo_sorter lasorter;
	std::sort(labors.begin(), labors.end(), lasorter);

	// Handle DISABLED skills (just bookkeeping)
	for (auto lp = labors.begin(); lp != labors.end(); ++lp)
	{
		auto labor = *lp;

		if (labor_infos[labor].mode != DISABLE)
			continue;

		for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
		{
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

        /*
		assert(labor >= 0);
		assert(labor < ARRAY_COUNT(labor_infos));
		*/

		df::job_skill skill = labor_to_skill[labor];

		if (labor_infos[labor].mode == HAULERS)
			continue;
		if (labor_infos[labor].mode == DISABLE)
			continue;

		int best_dwarf = 0;
		int best_value = -10000;

		std::vector<int> values(n_dwarfs);
		std::vector<int> candidates;
		std::map<int, int> dwarf_skill;

		auto mode = labor_infos[labor].mode;

		int min_dwarfs = 0;
		int max_dwarfs = 0;

		switch (mode)
		{
		case EVERYONE:
			min_dwarfs = max_dwarfs = n_dwarfs;
			break;

		case MINIMUM:
			min_dwarfs = labor_infos[labor].minimum_dwarfs;
			max_dwarfs = n_dwarfs;
			break;

		case EXACTLY:
			min_dwarfs = max_dwarfs = labor_infos[labor].minimum_dwarfs;
			break;

		case MAXIMUM:
			min_dwarfs = 0;
			max_dwarfs = labor_infos[labor].minimum_dwarfs;
			break;
		}

		for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
		{
			if (dwarf_info[dwarf].state == CHILD)
				continue;
			if (dwarf_info[dwarf].state == MILITARY)
				continue;

			if (labor_infos[labor].is_exclusive && dwarf_info[dwarf].has_exclusive_labor)
				continue;

			int value = dwarf_info[dwarf].mastery_penalty; // - dwarf_info[dwarf].assigned_jobs * 50;

			if (skill != df::enums::job_skill::NONE)
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

			values[dwarf] = value;

			candidates.push_back(dwarf);

		}

		if (labor_infos[labor].mode != EVERYONE)
        {
            values_sorter ivs(values);
			std::sort(candidates.begin(), candidates.end(), ivs);
        }

		int active_dwarfs = 0;

		for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
		{
			if (dwarf_info[dwarf].state == CHILD)
				continue;
			
			dwarfs[dwarf]->status.labors[labor] = false;
		}

		// Special - don't assign hunt without a butchers, or fish without a fishery
		if (df::enums::unit_labor::HUNT == labor && !has_butchers)
			min_dwarfs = max_dwarfs = 0;
		if (df::enums::unit_labor::FISH == labor && !has_fishery)
			min_dwarfs = max_dwarfs = 0;

		for (int i = 0; i < candidates.size() && active_dwarfs < max_dwarfs; i++)
		{
			int dwarf = candidates[i];

			assert(dwarf >= 0);
			assert(dwarf < n_dwarfs);

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
				out.print("Dwarf %i \"%s\" assigned %s: value %i\n", dwarf, dwarfs[dwarf]->name.first_name.c_str(), ENUM_KEY_STR(unit_labor, labor).c_str(), values[dwarf]);

			if (dwarf_info[dwarf].state == IDLE || dwarf_info[dwarf].state == BUSY)
				active_dwarfs++;

			if (active_dwarfs >= min_dwarfs && (dwarf_info[dwarf].state == IDLE || state_count[IDLE] < 2))
				break;
		}
	}

	// Set about 1/3 of the dwarfs as haulers. The haulers have all HAULER labors enabled. Having a lot of haulers helps
	// make sure that hauling jobs are handled quickly rather than building up.

	int num_haulers = state_count[IDLE] + state_count[BUSY] / 3;
	if (num_haulers < 1)
		num_haulers = 1;

	std::vector<int> hauler_ids;
	for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
	{
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
		if (labor == df::enums::unit_labor::NONE)
			continue;

        /*
		assert(labor >= 0);
		assert(labor < ARRAY_COUNT(labor_infos));
		*/

		if (labor_infos[labor].mode != HAULERS)
			continue;

		for (int i = 0; i < num_haulers; i++)
		{
			assert(i < hauler_ids.size());

			int dwarf = hauler_ids[i];

			assert(dwarf >= 0);
			assert(dwarf < n_dwarfs);
			dwarfs[dwarf]->status.labors[labor] = true;
			dwarf_info[dwarf].assigned_jobs++;

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

// A command! It sits around and looks pretty. And it's nice and friendly.
command_result autolabor (color_ostream &out, std::vector <std::string> & parameters)
{
	if (parameters.size() == 1 && 
		(parameters[0] == "0" || parameters[0] == "enable" || 
		 parameters[0] == "1" || parameters[0] == "disable"))
    {
        if (parameters[0] == "0" || parameters[0] == "disable")
            enable_autolabor = 0;
        else
            enable_autolabor = 1;
        out.print("autolabor %sactivated.\n", (enable_autolabor ? "" : "de"));
    }
    else if (parameters.size() == 2 && parameters[0] == "miners") {
		int nminers = atoi (parameters[1].c_str());
		if (nminers >= 0) {
			labor_infos[0].minimum_dwarfs = nminers;
			out.print("miner count set to %d.\n", nminers);
		} else {
			out.print("Syntax: autolabor miners <n>, where n is 0 or more.\n"
				"Current miner count: %d\n", labor_infos[0].minimum_dwarfs);
		}
	} 
	else if (parameters.size() == 1 && parameters[0] == "debug") {
		print_debug = 1;
	}
	else
    {
        out.print("Automatically assigns labors to dwarves.\n"
            "Activate with 'autolabor 1', deactivate with 'autolabor 0'.\n"
            "Current state: %d.\n", enable_autolabor);
    }

    return CR_OK;
}
