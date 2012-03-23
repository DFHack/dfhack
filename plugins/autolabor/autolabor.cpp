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

// our own, empty header.
#include "autolabor.h"

#define ARRAY_COUNT(array) (sizeof(array)/sizeof((array)[0]))

static int enable_autolabor;


// Here go all the command declarations...
// mostly to allow having the mandatory stuff on top of the file and commands on the bottom
command_result autolabor (color_ostream &out, std::vector <std::string> & parameters);

// A plugin must be able to return its name and version.
// The name string provided must correspond to the filename - autolabor.plug.so or autolabor.plug.dll in this case
DFHACK_PLUGIN("autolabor");

enum labor_mode {
	FIXED,
	AUTOMATIC,
	EVERYONE,
	HAULERS,
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

static const struct labor_info labor_infos[] = {
    /* MINE */				{AUTOMATIC, true, 2},
    /* HAUL_STONE */		{HAULERS, false, 1},
    /* HAUL_WOOD */			{HAULERS, false, 1},
    /* HAUL_BODY */			{HAULERS, false, 1},
    /* HAUL_FOOD */			{HAULERS, false, 1},
    /* HAUL_REFUSE */		{HAULERS, false, 1},
    /* HAUL_ITEM */			{HAULERS, false, 1},
    /* HAUL_FURNITURE */	{HAULERS, false, 1},
    /* HAUL_ANIMAL */		{HAULERS, false, 1},
    /* CLEAN */				{HAULERS, false, 1},
    /* CUTWOOD */			{AUTOMATIC, true, 1},
    /* CARPENTER */			{AUTOMATIC, false, 1},
    /* DETAIL */			{AUTOMATIC, false, 1},
    /* MASON */				{AUTOMATIC, false, 1},
    /* ARCHITECT */			{AUTOMATIC, false, 1},
    /* ANIMALTRAIN */		{AUTOMATIC, false, 1},
    /* ANIMALCARE */		{AUTOMATIC, false, 1},
    /* DIAGNOSE */			{AUTOMATIC, false, 1},
    /* SURGERY */			{AUTOMATIC, false, 1},
    /* BONE_SETTING */		{AUTOMATIC, false, 1},
    /* SUTURING */			{AUTOMATIC, false, 1},
    /* DRESSING_WOUNDS */	{AUTOMATIC, false, 1},
	/* FEED_WATER_CIVILIANS */ {EVERYONE, false, 1},
    /* RECOVER_WOUNDED */	{HAULERS, false, 1},
    /* BUTCHER */			{AUTOMATIC, false, 1},
    /* TRAPPER */			{AUTOMATIC, false, 1},
    /* DISSECT_VERMIN */	{AUTOMATIC, false, 1},
    /* LEATHER */			{AUTOMATIC, false, 1},
    /* TANNER */			{AUTOMATIC, false, 1},
    /* BREWER */			{AUTOMATIC, false, 1},
    /* ALCHEMIST */			{AUTOMATIC, false, 1},
    /* SOAP_MAKER */		{AUTOMATIC, false, 1},
    /* WEAVER */			{AUTOMATIC, false, 1},
    /* CLOTHESMAKER */		{AUTOMATIC, false, 1},
    /* MILLER */			{AUTOMATIC, false, 1},
    /* PROCESS_PLANT */		{AUTOMATIC, false, 1},
    /* MAKE_CHEESE */		{AUTOMATIC, false, 1},
    /* MILK */				{AUTOMATIC, false, 1},
    /* COOK */				{AUTOMATIC, false, 1},
    /* PLANT */				{AUTOMATIC, false, 1},
    /* HERBALIST */			{AUTOMATIC, false, 1},
    /* FISH */				{FIXED, false, 1},
    /* CLEAN_FISH */		{AUTOMATIC, false, 1},
    /* DISSECT_FISH */		{AUTOMATIC, false, 1},
    /* HUNT */				{FIXED, true, 1},
    /* SMELT */				{AUTOMATIC, false, 1},
    /* FORGE_WEAPON */		{AUTOMATIC, false, 1},
    /* FORGE_ARMOR */		{AUTOMATIC, false, 1},
    /* FORGE_FURNITURE */	{AUTOMATIC, false, 1},
    /* METAL_CRAFT */		{AUTOMATIC, false, 1},
    /* CUT_GEM */			{AUTOMATIC, false, 1},
    /* ENCRUST_GEM */		{AUTOMATIC, false, 1},
    /* WOOD_CRAFT */		{AUTOMATIC, false, 1},
    /* STONE_CRAFT */		{AUTOMATIC, false, 1},
    /* BONE_CARVE */		{AUTOMATIC, false, 1},
    /* GLASSMAKER */		{AUTOMATIC, false, 1},
    /* EXTRACT_STRAND */	{AUTOMATIC, false, 1},
    /* SIEGECRAFT */		{AUTOMATIC, false, 1},
    /* SIEGEOPERATE */		{AUTOMATIC, false, 1},
    /* BOWYER */			{AUTOMATIC, false, 1},
    /* MECHANIC */			{AUTOMATIC, false, 1},
    /* POTASH_MAKING */		{AUTOMATIC, false, 1},
    /* LYE_MAKING */		{AUTOMATIC, false, 1},
    /* DYER */				{AUTOMATIC, false, 1},
    /* BURN_WOOD */			{AUTOMATIC, false, 1},
    /* OPERATE_PUMP */		{AUTOMATIC, false, 1},
    /* SHEARER */			{AUTOMATIC, false, 1},
    /* SPINNER */			{AUTOMATIC, false, 1},
    /* POTTERY */			{AUTOMATIC, false, 1},
    /* GLAZING */			{AUTOMATIC, false, 1},
    /* PRESSING */			{AUTOMATIC, false, 1},
    /* BEEKEEPING */		{AUTOMATIC, false, 1},
	/* WAX_WORKING */		{AUTOMATIC, false, 1},
};

static const df::job_skill noble_skills[] = {
	df::enums::job_skill::APPRAISAL,
	df::enums::job_skill::ORGANIZATION,
	df::enums::job_skill::RECORD_KEEPING,
};

struct dwarf_info
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
	assert(ARRAY_COUNT(labor_infos) > ENUM_LAST_ITEM(unit_labor));

	// Fill the command list with your commands.
    commands.clear();
    commands.push_back(PluginCommand(
        "autolabor", "Automatically manage dwarf labors.",
        autolabor, false, /* true means that the command can't be used from non-interactive user interface */
        // Extended help string. Used by CR_WRONG_USAGE and the help command:
        "  autolabor enable\n"
		"  autolabor disable\n"
		"    Enables or disables the plugin.\n"
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
	return CR_OK;
}

DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
	static int step_count = 0;

	if (!enable_autolabor)
		return CR_OK;

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
		if (cre->race == race && cre->civ_id == civ && !cre->flags1.bits.marauder && !cre->flags1.bits.diplomat && !cre->flags1.bits.merchant && !cre->flags1.bits.dead) {
			dwarfs.push_back(cre);
        }
    }

	int n_dwarfs = dwarfs.size();

	if (n_dwarfs == 0)
		return CR_OK;

	std::vector<dwarf_info> dwarf_info(n_dwarfs);

	std::vector<int> best_noble(_countof(noble_skills));
	std::vector<int> highest_noble_skill(_countof(noble_skills));
	std::vector<int> highest_noble_experience(_countof(noble_skills));

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
			for (int i = 0; i < _countof(noble_skills); i++)
			{
				if (skill == noble_skills[i])
					noble_skill_id = i;
			}

			if (noble_skill_id >= 0)
			{
				assert(noble_skill_id < _countof(noble_skills));

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

	for (int i = 0; i < _countof(noble_skills); i++)
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

			assert(labor >= 0);
			assert(labor < _countof(labor_infos));

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

			assert(job >= 0);
			assert(job < _countof(dwarf_states));

			dwarf_info[dwarf].state = dwarf_states[job];
		}

		state_count[dwarf_info[dwarf].state]++;
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
			assert(labor >= 0);
			assert(labor < _countof(labor_to_skill));

			labor_to_skill[labor] = skill;
		}
	}

	std::vector<df::unit_labor> labors;

	FOR_ENUM_ITEMS(unit_labor, labor)
	{
		if (labor == df::enums::unit_labor::NONE)
			continue;

		assert(labor >= 0);
		assert(labor < _countof(labor_infos));

		labors.push_back(labor);
	}

	std::sort(labors.begin(), labors.end(), [] (int i, int j) { return labor_infos[i].mode < labor_infos[j].mode; });

	// Handle all skills except those marked HAULERS

	for (auto lp = labors.begin(); lp != labors.end(); ++lp)
	{
		auto labor = *lp;

		assert(labor >= 0);
		assert(labor < _countof(labor_infos));

		df::job_skill skill = labor_to_skill[labor];

		if (labor_infos[labor].mode == HAULERS)
			continue;

		int best_dwarf = 0;
		int best_value = -10000;

		std::vector<int> values(n_dwarfs);
		std::vector<int> candidates;
		std::vector<int> backup_candidates;
		std::map<int, int> dwarf_skill;

		auto mode = labor_infos[labor].mode;
		if (AUTOMATIC == mode && state_count[IDLE] == 0)
			mode = FIXED;

		for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
		{
			if (dwarf_info[dwarf].state != IDLE && dwarf_info[dwarf].state != BUSY && mode != EVERYONE)
				continue;

			if (labor_infos[labor].is_exclusive && dwarf_info[dwarf].has_exclusive_labor)
				continue;

			int value = dwarf_info[dwarf].mastery_penalty - dwarf_info[dwarf].assigned_jobs;

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

			if (mode == AUTOMATIC && dwarf_info[dwarf].state != IDLE)
				backup_candidates.push_back(dwarf);
			else
				candidates.push_back(dwarf);

		}

		if (candidates.size() == 0)
		{
			candidates = backup_candidates;
			mode = FIXED;
		}

		if (labor_infos[labor].mode != EVERYONE)
			std::sort(candidates.begin(), candidates.end(), [&values] (int i, int j) { return values[i] > values[j]; });

		for (int dwarf = 0; dwarf < n_dwarfs; dwarf++)
		{
			bool allow_labor = false;
			
			if (dwarf_info[dwarf].state == BUSY && 
				  mode == AUTOMATIC && 
				  (labor_infos[labor].is_exclusive || dwarf_skill[dwarf] > 0))
			{
				allow_labor = true;
			}

			if (dwarf_info[dwarf].state == OTHER &&
				  mode == AUTOMATIC && 
				  dwarf_skill[dwarf] > 0 &&
				  !dwarf_info[dwarf].is_best_noble)
			{
				allow_labor = true;
			}

			if (dwarfs[dwarf]->status.labors[labor] &&
				allow_labor && 
				!(labor_infos[labor].is_exclusive && dwarf_info[dwarf].has_exclusive_labor))
			{
				if (labor_infos[labor].is_exclusive)
					dwarf_info[dwarf].has_exclusive_labor = true;

				dwarf_info[dwarf].assigned_jobs++;
			}
			else
			{
				dwarfs[dwarf]->status.labors[labor] = false;
			}
		}

		int minimum_dwarfs = labor_infos[labor].minimum_dwarfs;

		if (labor_infos[labor].mode == EVERYONE)
			minimum_dwarfs = n_dwarfs;

		// Special - don't assign hunt without a butchers, or fish without a fishery
		if (df::enums::unit_labor::HUNT == labor && !has_butchers)
			minimum_dwarfs = 0;
		if (df::enums::unit_labor::FISH == labor && !has_fishery)
			minimum_dwarfs = 0;

		for (int i = 0; i < candidates.size() && i < minimum_dwarfs; i++)
		{
			int dwarf = candidates[i];

			assert(dwarf >= 0);
			assert(dwarf < n_dwarfs);

			if (!dwarfs[dwarf]->status.labors[labor])
				dwarf_info[dwarf].assigned_jobs++;

			dwarfs[dwarf]->status.labors[labor] = true;

			if (labor_infos[labor].is_exclusive)
				dwarf_info[dwarf].has_exclusive_labor = true;
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

	// Idle dwarves come first, then we sort from least-skilled to most-skilled.
	
	std::sort(hauler_ids.begin(), hauler_ids.end(), [&dwarf_info] (int i, int j) -> bool
	{ 
		if (dwarf_info[i].state == IDLE && dwarf_info[j].state != IDLE)
			return true;
		if (dwarf_info[i].state != IDLE && dwarf_info[j].state == IDLE)
			return false;
		return dwarf_info[i].mastery_penalty > dwarf_info[j].mastery_penalty; 
	});

	FOR_ENUM_ITEMS(unit_labor, labor)
	{
		if (labor == df::enums::unit_labor::NONE)
			continue;

		assert(labor >= 0);
		assert(labor < _countof(labor_infos));

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

    return CR_OK;
}

// A command! It sits around and looks pretty. And it's nice and friendly.
command_result autolabor (color_ostream &out, std::vector <std::string> & parameters)
{
    if (parameters.size() == 1 && (parameters[0] == "0" || parameters[0] == "1"))
    {
        if (parameters[0] == "0")
            enable_autolabor = 0;
        else
            enable_autolabor = 1;
        out.print("autolabor %sactivated.\n", (enable_autolabor ? "" : "de"));
    }
    else
    {
        out.print("Automatically assigns labors to dwarves.\n"
            "Activate with 'autolabor 1', deactivate with 'autolabor 0'.\n"
            "Current state: %d.\n", enable_autolabor);
    }

    return CR_OK;
}
