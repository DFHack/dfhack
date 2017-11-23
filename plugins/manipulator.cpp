// Dwarf Manipulator - a Therapist-style labor editor

#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <MiscUtils.h>
#include <modules/Screen.h>
#include <modules/Translation.h>
#include <modules/Units.h>
#include <modules/Filesystem.h>
#include <modules/Job.h>
#include <vector>
#include <string>
#include <set>
#include <algorithm>
#include <tuple>
#include <chrono>
#include <VTableInterpose.h>

#include "df/activity_event.h"
#include "df/world.h"
#include "df/ui.h"
#include "df/graphic.h"
#include "df/enabler.h"
#include "df/viewscreen_unitlistst.h"
#include "df/interface_key.h"

#include "df/unit.h"
#include "df/unit_soul.h"
#include "df/unit_skill.h"

#include "df/unit_preference.h"
#include "df/misc_trait_type.h"
#include "df/unit_misc_trait.h"

#include "df/creature_graphics_role.h"
#include "df/creature_raw.h"
#include "df/caste_raw.h"
#include "df/historical_entity.h"
#include "df/entity_raw.h"

#include "uicommon.h"
#include "listcolumn.h"

#include "df/unit_health_info.h"
#include "df/unit_health_flags.h"
#include "df/unit_wound.h"

using std::stringstream;
using std::set;
using std::vector;
using std::string;

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("manipulator");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(enabler);

#define CONFIG_PATH "manipulator"

struct SkillLevel
{
    const char *name;
    int points;
    char abbrev;
};

#define NUM_SKILL_LEVELS (sizeof(skill_levels) / sizeof(SkillLevel))

// The various skill rankings.
const SkillLevel skill_levels[] = {
    {"Dabbling",     500, '0'},
    {"Novice",       600, '1'},
    {"Adequate",     700, '2'},
    {"Competent",    800, '3'},
    {"Skilled",      900, '4'},
    {"Proficient",  1000, '5'},
    {"Talented",    1100, '6'},
    {"Adept",       1200, '7'},
    {"Expert",      1300, '8'},
    {"Professional",1400, '9'},
    {"Accomplished",1500, 'A'},
    {"Great",       1600, 'B'},
    {"Master",      1700, 'C'},
    {"High Master", 1800, 'D'},
    {"Grand Master",1900, 'E'},
    {"Legendary",   2000, 'U'},
    {"Legendary+1", 2100, 'V'},
    {"Legendary+2", 2200, 'W'},
    {"Legendary+3", 2300, 'X'},
    {"Legendary+4", 2400, 'Y'},
    {"Legendary+5",    0, 'Z'}
};

struct SkillColumn
{
    int group; // for navigation and mass toggling
    int8_t color; // for column headers
    df::profession profession; // to display graphical tiles
    df::unit_labor labor; // toggled when pressing Enter
    df::job_skill skill; // displayed rating
    char label[3]; // column header
    bool special; // specified labor is mutually exclusive with all other special labors
};

#define NUM_LABORS (sizeof(columns) / sizeof(SkillColumn))

// All of the skill/labor columns.
// The subgroup order of a few of these has been improved.
const SkillColumn columns[] = {
// Mining
    {0, 7, profession::MINER, unit_labor::MINE, job_skill::MINING, "Mi", true},
// Woodworking
    {1, 14, profession::CARPENTER, unit_labor::CARPENTER, job_skill::CARPENTRY, "Ca"},
    {1, 14, profession::BOWYER, unit_labor::BOWYER, job_skill::BOWYER, "Bw"},
    {1, 14, profession::WOODCUTTER, unit_labor::CUTWOOD, job_skill::WOODCUTTING, "WC", true},
// Stoneworking
    {2, 15, profession::MASON, unit_labor::MASON, job_skill::MASONRY, "Ma"},
    {2, 15, profession::ENGRAVER, unit_labor::DETAIL, job_skill::DETAILSTONE, "En"},
// Hunting/Related
    {3, 2, profession::ANIMAL_TRAINER, unit_labor::ANIMALTRAIN, job_skill::ANIMALTRAIN, "Tn"},
    {3, 2, profession::ANIMAL_CARETAKER, unit_labor::ANIMALCARE, job_skill::ANIMALCARE, "Ca"},
    {3, 2, profession::HUNTER, unit_labor::HUNT, job_skill::SNEAK, "Hu", true},
    {3, 2, profession::TRAPPER, unit_labor::TRAPPER, job_skill::TRAPPING, "Tr"},
    {3, 2, profession::ANIMAL_DISSECTOR, unit_labor::DISSECT_VERMIN, job_skill::DISSECT_VERMIN, "Di"},
// Healthcare
    {4, 5, profession::DIAGNOSER, unit_labor::DIAGNOSE, job_skill::DIAGNOSE, "Di"},
    {4, 5, profession::SURGEON, unit_labor::SURGERY, job_skill::SURGERY, "Su"},
    {4, 5, profession::BONE_SETTER, unit_labor::BONE_SETTING, job_skill::SET_BONE, "Bo"},
    {4, 5, profession::SUTURER, unit_labor::SUTURING, job_skill::SUTURE, "St"},
    {4, 5, profession::DOCTOR, unit_labor::DRESSING_WOUNDS, job_skill::DRESS_WOUNDS, "Dr"},
    {4, 5, profession::NONE, unit_labor::FEED_WATER_CIVILIANS, job_skill::NONE, "Fd"},
    {4, 5, profession::NONE, unit_labor::RECOVER_WOUNDED, job_skill::NONE, "Re"},
// Farming Related
    {5, 6, profession::BUTCHER, unit_labor::BUTCHER, job_skill::BUTCHER, "Bu"},
    {5, 6, profession::TANNER, unit_labor::TANNER, job_skill::TANNER, "Ta"},
    {5, 6, profession::COOK, unit_labor::COOK, job_skill::COOK, "Co"},
    {5, 6, profession::BREWER, unit_labor::BREWER, job_skill::BREWING, "Br"},
    {5, 6, profession::GELDER, unit_labor::GELD, job_skill::GELD, "Ge"},
    {5, 6, profession::PLANTER, unit_labor::PLANT, job_skill::PLANT, "Gr"},
    {5, 6, profession::HERBALIST, unit_labor::HERBALIST, job_skill::HERBALISM, "He"},
    {5, 6, profession::BEEKEEPER, unit_labor::BEEKEEPING, job_skill::BEEKEEPING, "Be"},
    {5, 6, profession::PRESSER, unit_labor::PRESSING, job_skill::PRESSING, "Pr"},
    {5, 6, profession::MILLER, unit_labor::MILLER, job_skill::MILLING, "Ml"},
    {5, 6, profession::THRESHER, unit_labor::PROCESS_PLANT, job_skill::PROCESSPLANTS, "Th"},
    {5, 6, profession::CHEESE_MAKER, unit_labor::MAKE_CHEESE, job_skill::CHEESEMAKING, "Ch"},
    {5, 6, profession::MILKER, unit_labor::MILK, job_skill::MILK, "Mk"},
    {5, 6, profession::SHEARER, unit_labor::SHEARER, job_skill::SHEARING, "Sh"},
    {5, 6, profession::SPINNER, unit_labor::SPINNER, job_skill::SPINNING, "Sp"},
    {5, 6, profession::DYER, unit_labor::DYER, job_skill::DYER, "Dy"},
    {5, 6, profession::SOAP_MAKER, unit_labor::SOAP_MAKER, job_skill::SOAP_MAKING, "So"},
    {5, 6, profession::LYE_MAKER, unit_labor::LYE_MAKING, job_skill::LYE_MAKING, "Ly"},
    {5, 6, profession::POTASH_MAKER, unit_labor::POTASH_MAKING, job_skill::POTASH_MAKING, "Po"},
    {5, 6, profession::WOOD_BURNER, unit_labor::BURN_WOOD, job_skill::WOOD_BURNING, "WB"},
// Fishing/Related
    {6, 1, profession::FISHERMAN, unit_labor::FISH, job_skill::FISH, "Fi"},
    {6, 1, profession::FISH_CLEANER, unit_labor::CLEAN_FISH, job_skill::PROCESSFISH, "Cl"},
    {6, 1, profession::FISH_DISSECTOR, unit_labor::DISSECT_FISH, job_skill::DISSECT_FISH, "Di"},
// Metalsmithing
    {7, 8, profession::FURNACE_OPERATOR, unit_labor::SMELT, job_skill::SMELT, "Fu"},
    {7, 8, profession::BLACKSMITH, unit_labor::FORGE_FURNITURE, job_skill::FORGE_FURNITURE, "Bl"},
    {7, 8, profession::WEAPONSMITH, unit_labor::FORGE_WEAPON, job_skill::FORGE_WEAPON, "We"},
    {7, 8, profession::ARMORER, unit_labor::FORGE_ARMOR, job_skill::FORGE_ARMOR, "Ar"},
    {7, 8, profession::METALCRAFTER, unit_labor::METAL_CRAFT, job_skill::METALCRAFT, "Cr"},
// Crafts
    {8, 9, profession::LEATHERWORKER, unit_labor::LEATHER, job_skill::LEATHERWORK, "Le"},
    {8, 9, profession::WOODCRAFTER, unit_labor::WOOD_CRAFT, job_skill::WOODCRAFT, "Wo"},
    {8, 9, profession::STONECRAFTER, unit_labor::STONE_CRAFT, job_skill::STONECRAFT, "St"},
    {8, 9, profession::BONE_CARVER, unit_labor::BONE_CARVE, job_skill::BONECARVE, "Bo"},
    {8, 9, profession::CLOTHIER, unit_labor::CLOTHESMAKER, job_skill::CLOTHESMAKING, "Cl"},
    {8, 9, profession::WEAVER, unit_labor::WEAVER, job_skill::WEAVING, "We"},
    {8, 9, profession::GLASSMAKER, unit_labor::GLASSMAKER, job_skill::GLASSMAKER, "Gl"},
    {8, 9, profession::POTTER, unit_labor::POTTERY, job_skill::POTTERY, "Po"},
    {8, 9, profession::GLAZER, unit_labor::GLAZING, job_skill::GLAZING, "Gl"},
    {8, 9, profession::WAX_WORKER, unit_labor::WAX_WORKING, job_skill::WAX_WORKING, "Wx"},
    {8, 9, profession::PAPERMAKER, unit_labor::PAPERMAKING, job_skill::PAPERMAKING, "Pa"},
    {8, 9, profession::BOOKBINDER, unit_labor::BOOKBINDING, job_skill::BOOKBINDING, "Bk"},
    {8, 9, profession::STRAND_EXTRACTOR, unit_labor::EXTRACT_STRAND, job_skill::EXTRACT_STRAND, "Ad"},
// Jewelry
    {9, 10, profession::GEM_CUTTER, unit_labor::CUT_GEM, job_skill::CUTGEM, "Cu"},
    {9, 10, profession::GEM_SETTER, unit_labor::ENCRUST_GEM, job_skill::ENCRUSTGEM, "Se"},
// Engineering
    {10, 12, profession::SIEGE_ENGINEER, unit_labor::SIEGECRAFT, job_skill::SIEGECRAFT, "En"},
    {10, 12, profession::SIEGE_OPERATOR, unit_labor::SIEGEOPERATE, job_skill::SIEGEOPERATE, "Op"},
    {10, 12, profession::MECHANIC, unit_labor::MECHANIC, job_skill::MECHANICS, "Me"},
    {10, 12, profession::PUMP_OPERATOR, unit_labor::OPERATE_PUMP, job_skill::OPERATE_PUMP, "Pu"},
// Hauling
    {11, 3, profession::NONE, unit_labor::HAUL_STONE, job_skill::NONE, "St"},
    {11, 3, profession::NONE, unit_labor::HAUL_WOOD, job_skill::NONE, "Wo"},
    {11, 3, profession::NONE, unit_labor::HAUL_ITEM, job_skill::NONE, "It"},
    {11, 3, profession::NONE, unit_labor::HAUL_BODY, job_skill::NONE, "Bu"},
    {11, 3, profession::NONE, unit_labor::HAUL_FOOD, job_skill::NONE, "Fo"},
    {11, 3, profession::NONE, unit_labor::HAUL_REFUSE, job_skill::NONE, "Re"},
    {11, 3, profession::NONE, unit_labor::HAUL_FURNITURE, job_skill::NONE, "Fu"},
    {11, 3, profession::NONE, unit_labor::HAUL_ANIMALS, job_skill::NONE, "An"},
    {11, 3, profession::NONE, unit_labor::HANDLE_VEHICLES, job_skill::NONE, "Ve"},
    {11, 3, profession::NONE, unit_labor::HAUL_TRADE, job_skill::NONE, "Tr"},
    {11, 3, profession::NONE, unit_labor::HAUL_WATER, job_skill::NONE, "Wa"},
// Other Jobs
    {12, 4, profession::ARCHITECT, unit_labor::ARCHITECT, job_skill::DESIGNBUILDING, "Ar"},
    {12, 4, profession::ALCHEMIST, unit_labor::ALCHEMIST, job_skill::ALCHEMY, "Al"},
    {12, 4, profession::NONE, unit_labor::CLEAN, job_skill::NONE, "Cl"},
    {12, 4, profession::NONE, unit_labor::PULL_LEVER, job_skill::NONE, "Lv"},
    {12, 4, profession::NONE, unit_labor::BUILD_ROAD, job_skill::NONE, "Ro"},
    {12, 4, profession::NONE, unit_labor::BUILD_CONSTRUCTION, job_skill::NONE, "Co"},
    {12, 4, profession::NONE, unit_labor::REMOVE_CONSTRUCTION, job_skill::NONE, "CR"},
// Military - Weapons
    {13, 7, profession::WRESTLER, unit_labor::NONE, job_skill::WRESTLING, "Wr"},
    {13, 7, profession::AXEMAN, unit_labor::NONE, job_skill::AXE, "Ax"},
    {13, 7, profession::SWORDSMAN, unit_labor::NONE, job_skill::SWORD, "Sw"},
    {13, 7, profession::MACEMAN, unit_labor::NONE, job_skill::MACE, "Mc"},
    {13, 7, profession::HAMMERMAN, unit_labor::NONE, job_skill::HAMMER, "Ha"},
    {13, 7, profession::SPEARMAN, unit_labor::NONE, job_skill::SPEAR, "Sp"},
    {13, 7, profession::CROSSBOWMAN, unit_labor::NONE, job_skill::CROSSBOW, "Cb"},
    {13, 7, profession::THIEF, unit_labor::NONE, job_skill::DAGGER, "Kn"},
    {13, 7, profession::BOWMAN, unit_labor::NONE, job_skill::BOW, "Bo"},
    {13, 7, profession::BLOWGUNMAN, unit_labor::NONE, job_skill::BLOWGUN, "Bl"},
    {13, 7, profession::PIKEMAN, unit_labor::NONE, job_skill::PIKE, "Pk"},
    {13, 7, profession::LASHER, unit_labor::NONE, job_skill::WHIP, "La"},
// Military - Other Combat
    {14, 15, profession::NONE, unit_labor::NONE, job_skill::BITE, "Bi"},
    {14, 15, profession::NONE, unit_labor::NONE, job_skill::GRASP_STRIKE, "St"},
    {14, 15, profession::NONE, unit_labor::NONE, job_skill::STANCE_STRIKE, "Ki"},
    {14, 15, profession::NONE, unit_labor::NONE, job_skill::MISC_WEAPON, "Mi"},
    {14, 15, profession::NONE, unit_labor::NONE, job_skill::MELEE_COMBAT, "Fg"},
    {14, 15, profession::NONE, unit_labor::NONE, job_skill::RANGED_COMBAT, "Ac"},
    {14, 15, profession::NONE, unit_labor::NONE, job_skill::ARMOR, "Ar"},
    {14, 15, profession::NONE, unit_labor::NONE, job_skill::SHIELD, "Sh"},
    {14, 15, profession::NONE, unit_labor::NONE, job_skill::DODGING, "Do"},
// Military - Misc
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::SWIMMING, "Sw"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::COORDINATION, "Cr"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::BALANCE, "Ba"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::CLIMBING, "Cl"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::SITUATIONAL_AWARENESS, "Ob"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::KNOWLEDGE_ACQUISITION, "St"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::TEACHING, "Te"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::CONCENTRATION, "Co"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::DISCIPLINE, "Di"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::LEADERSHIP, "Ld"},
// Noble
    {16, 5, profession::ADMINISTRATOR, unit_labor::NONE, job_skill::ORGANIZATION, "Or"},
    {16, 5, profession::TRADER, unit_labor::NONE, job_skill::APPRAISAL, "Ap"},
    {16, 5, profession::CLERK, unit_labor::NONE, job_skill::RECORD_KEEPING, "RK"},
// Social
    {17, 3, profession::NONE, unit_labor::NONE, job_skill::PERSUASION, "Pe"},
    {17, 3, profession::NONE, unit_labor::NONE, job_skill::NEGOTIATION, "Ne"},
    {17, 3, profession::NONE, unit_labor::NONE, job_skill::JUDGING_INTENT, "Ju"},
    {17, 3, profession::NONE, unit_labor::NONE, job_skill::LYING, "Li"},
    {17, 3, profession::NONE, unit_labor::NONE, job_skill::INTIMIDATION, "In"},
    {17, 3, profession::NONE, unit_labor::NONE, job_skill::CONVERSATION, "Cn"},
    {17, 3, profession::NONE, unit_labor::NONE, job_skill::COMEDY, "Cm"},
    {17, 3, profession::NONE, unit_labor::NONE, job_skill::FLATTERY, "Fl"},
    {17, 3, profession::NONE, unit_labor::NONE, job_skill::CONSOLE, "Cs"},
    {17, 3, profession::NONE, unit_labor::NONE, job_skill::PACIFY, "Pc"},
// Miscellaneous
    {18, 3, profession::NONE, unit_labor::NONE, job_skill::THROW, "Th"},
    {18, 3, profession::NONE, unit_labor::NONE, job_skill::CRUTCH_WALK, "CW"},
    {18, 3, profession::NONE, unit_labor::NONE, job_skill::KNAPPING, "Kn"},

    {19, 6, profession::NONE, unit_labor::NONE, job_skill::WRITING, "Wr"},
    {19, 6, profession::NONE, unit_labor::NONE, job_skill::PROSE, "Pr"},
    {19, 6, profession::NONE, unit_labor::NONE, job_skill::POETRY, "Po"},
    {19, 6, profession::NONE, unit_labor::NONE, job_skill::READING, "Rd"},
    {19, 6, profession::NONE, unit_labor::NONE, job_skill::SPEAKING, "Sp"},
    {19, 6, profession::NONE, unit_labor::NONE, job_skill::DANCE, "Dn"},
    {19, 6, profession::NONE, unit_labor::NONE, job_skill::MAKE_MUSIC, "MM"},
    {19, 6, profession::NONE, unit_labor::NONE, job_skill::SING_MUSIC, "SM"},
    {19, 6, profession::NONE, unit_labor::NONE, job_skill::PLAY_KEYBOARD_INSTRUMENT, "PK"},
    {19, 6, profession::NONE, unit_labor::NONE, job_skill::PLAY_STRINGED_INSTRUMENT, "PS"},
    {19, 6, profession::NONE, unit_labor::NONE, job_skill::PLAY_WIND_INSTRUMENT, "PW"},
    {19, 6, profession::NONE, unit_labor::NONE, job_skill::PLAY_PERCUSSION_INSTRUMENT, "PP"},

    {20, 4, profession::NONE, unit_labor::NONE, job_skill::CRITICAL_THINKING, "CT"},
    {20, 4, profession::NONE, unit_labor::NONE, job_skill::LOGIC, "Lo"},
    {20, 4, profession::NONE, unit_labor::NONE, job_skill::MATHEMATICS, "Ma"},
    {20, 4, profession::NONE, unit_labor::NONE, job_skill::ASTRONOMY, "As"},
    {20, 4, profession::NONE, unit_labor::NONE, job_skill::CHEMISTRY, "Ch"},
    {20, 4, profession::NONE, unit_labor::NONE, job_skill::GEOGRAPHY, "Ge"},
    {20, 4, profession::NONE, unit_labor::NONE, job_skill::OPTICS_ENGINEER, "OE"},
    {20, 4, profession::NONE, unit_labor::NONE, job_skill::FLUID_ENGINEER, "FE"},

    {21, 5, profession::NONE, unit_labor::NONE, job_skill::MILITARY_TACTICS, "MT"},
    {21, 5, profession::NONE, unit_labor::NONE, job_skill::TRACKING, "Tr"},
    {21, 5, profession::NONE, unit_labor::NONE, job_skill::MAGIC_NATURE, "Dr"},
};

struct UnitInfo
{
    df::unit *unit;
    bool allowEdit;
    string name;
    string transname;
    string lastname;
    string profession;
    string squad_info;
    string squad_effective_name;
    string job_desc;
    enum { IDLE, SOCIAL, JOB } job_mode;
    bool selected;

    struct {
        // Used for custom professions, 1-indexed
        int list_id;        // Position in list
        int list_id_group;  // Position in list by group (e.g. craftsdwarf)
        int list_id_prof;   // Position in list by profession (e.g. woodcrafter)
        void init() {
            list_id = 0;
            list_id_group = 0;
            list_id_prof = 0;
        }
    } ids;

    string notices;
    int8_t color;
    int age;
    int arrival_group;

    int work_aptitude = 0;
    int skill_aptitude = 0;

    int notice_score;
    int demand=0;
    int scholar=0;
    int performer=0;
    int martial=0;
    int topskilled=0;

    uint8_t column_aptitudes[NUM_LABORS];
    int8_t column_hints[NUM_LABORS];
    bool sort_grouping_hint;
};

int work_aptitude_avg = 0;
int skill_aptitude_avg = 0;

enum detail_cols {
    DETAIL_MODE_PROFESSION,
    DETAIL_MODE_SQUAD,
    DETAIL_MODE_JOB,
    DETAIL_MODE_NOTICE,
    DETAIL_MODE_ATTRIBUTE,
    DETAIL_MODE_MAX
};
const char * const detailmode_shortnames[] = {
  "Profession,  ",
  "Squads,      ",
  "Actions,     ",
  "Notices,     ",
  "Attributes,  ",
  "Oops"
};
const char * const detailmode_legend[] = {
  "Profession", "Squad", "Action", "Notice", "", "Oops"
};

enum wide_sorts {
    WIDESORT_UNDER=-1,
    WIDESORT_NONE=0,
    WIDESORT_SELECTED,
    WIDESORT_PROFESSION,
    WIDESORT_SQUAD,
    WIDESORT_JOB,
    WIDESORT_ARRIVAL,
    WIDESORT_OVER
};

const char * const widesort_names[] = {
  " All,        ",
  " Selection",
  " Profession",
  " Squads",
  " Actions",
  " Arrivals",
  " Oops",
};

const char * const widesort_gaps[] = {
  "", " ", "", "    ", "   ", "  ",
};

enum fine_sorts {
    FINESORT_UNDER=-1,
    FINESORT_NAME=0,
    FINESORT_AGE,
    FINESORT_COLUMN,
    FINESORT_DEMAND,
    FINESORT_TOPSKILLED,
    FINESORT_MARTIAL,
    FINESORT_PERFORMER,
    FINESORT_SCHOLAR,
    FINESORT_STRESS,
    FINESORT_NOTICES,
    FINESORT_OVER
};

const char * const finesort_names[] = {
  "Name",
  //"Surname",
  "Age",
  "Column",
  "Availed",
  "Civil",
  "Martial",
  "Perform",
  "Scholar",
  "Stress",
  "Notices",
  "Oops",
};

static string cur_world;
static int detail_mode=0;
static int color_mode = 0;

static int first_row=0;
static int display_rows_b=0;
static int first_column=0;
static int sel_column=0;

static wide_sorts widesort_mode =  WIDESORT_NONE;
static fine_sorts finesort_mode =  FINESORT_NAME;
static bool widesort_descend = false;
static bool finesort_descend = false;
static bool sorts_descend = false;
static int column_sort_column = -1;
int column_sort_last =0;
bool cancel_sort=false;

static map<int, bool> selection_stash;
static bool selection_changed = false;
static int sel_row=0;
static int sel_row_b=0;
static int sel_unit=0;

void stashSelection(UnitInfo* cur){
    selection_stash[cur->unit->id]=cur->selected;//sel;
}

void stashSelection(vector<UnitInfo *> &units){
    for (size_t i = 0; i < units.size(); i++){
        selection_stash[units[i]->unit->id]=units[i]->selected;
    }
}

void unstashSelection(vector<UnitInfo *> &units){
    for (size_t i = 0; i < units.size(); i++){
        if(selection_stash[units[i]->unit->id]==true)
            units[i]->selected=true;
        else
           units[i]->selected=false;
    }
}


int unitSkillRating(const UnitInfo *cur,df::job_skill skill)
{
    if(!cur->unit->status.current_soul) return 0;
    df::unit_skill *s = binsearch_in_vector<df::unit_skill,df::job_skill>(cur->unit->status.current_soul->skills, &df::unit_skill::id, skill);
    if(s!= NULL)
       return s->rating;
    else
        return 0;
}

int unitSkillExperience(const UnitInfo *cur,df::job_skill skill)
{
    if(!cur->unit->status.current_soul) return 0;
    df::unit_skill *s = binsearch_in_vector<df::unit_skill,df::job_skill>(cur->unit->status.current_soul->skills, &df::unit_skill::id, skill);
    if(s!= NULL)
        return s->experience;
    else
        return 0;
}

//widesorts (which form groups)
bool sortBySelected (const UnitInfo *d1, const UnitInfo *d2)
{
    return !sorts_descend ? (d1->selected > d2->selected) : (d1->selected < d2->selected);
}

bool sortByProfession (const UnitInfo *d1, const UnitInfo *d2)
{
    if (sorts_descend)
        return (d1->profession > d2->profession);
    else
        return (d1->profession < d2->profession);
}

bool sortBySquad (const UnitInfo *d1, const UnitInfo *d2)
{
    if(d1->unit->military.squad_id == d2->unit->military.squad_id)
        return false;
    if(d1->unit->military.squad_id ==-1 && d2->unit->military.squad_id!=-1)
        return false;
    if(d2->unit->military.squad_id ==-1 && d1->unit->military.squad_id!=-1)
        return true;

    return sorts_descend == d1->unit->military.squad_id > d2->unit->military.squad_id ;
}

bool sortByArrival (const UnitInfo *d1, const UnitInfo *d2)
{
    if (sorts_descend)
        return (d1->arrival_group > d2->arrival_group);
    else
        return (d1->arrival_group < d2->arrival_group);
}

bool sortByChild (const UnitInfo *d1, const UnitInfo *d2)
{
    if (d1->age>=12 && d2->age<12)
        return true;

    if (d1->allowEdit!=d2->allowEdit){
        if(d1->allowEdit){
            return true;
       }else{
            return false;
       }
   }
    return false;
}

bool sortByJob (const UnitInfo *d1, const UnitInfo *d2)
{
    if(d1->job_mode == UnitInfo::IDLE && d2->job_mode != UnitInfo::IDLE)
        return !sorts_descend;
    if(d2->job_mode == UnitInfo::IDLE)
        return sorts_descend;
    if(d1->job_mode == UnitInfo::SOCIAL && d2->job_mode != UnitInfo::SOCIAL)
        return !sorts_descend;
    if(d1->job_mode != UnitInfo::SOCIAL && d2->job_mode == UnitInfo::SOCIAL)
        return sorts_descend;
    if(d1->job_mode == UnitInfo::SOCIAL && d2->job_mode == UnitInfo::SOCIAL)
        return !sorts_descend == (d1->job_desc > d2->job_desc);

    if (d1->job_mode != d2->job_mode)
        return !sorts_descend == (d1->job_mode < d2->job_mode);

    return !sorts_descend == d1->job_desc < d2->job_desc;
}

//finesorts (do before widesorts)
bool sortByScholar (const UnitInfo *d1, const UnitInfo *d2){
    return (d1->scholar > d2->scholar);
}
bool sortByNotices (const UnitInfo *d1, const UnitInfo *d2){
    return (d1->notice_score > d2->notice_score);
}
bool sortByMartial (const UnitInfo *d1, const UnitInfo *d2){
    return (d1->martial > d2->martial);
}
bool sortByPerformer (const UnitInfo *d1, const UnitInfo *d2){
    return (d1->performer  > d2->performer);
}
bool sortByTopskilled (const UnitInfo *d1, const UnitInfo *d2){
    return (d1->topskilled  > d2->topskilled);
}
bool sortByDemand (const UnitInfo *d1, const UnitInfo *d2){
    return (d1->demand  > d2->demand);
}
bool sortByAge (const UnitInfo *d1, const UnitInfo *d2){
    return (d1->age  > d2->age);
}
bool sortByName (const UnitInfo *d1, const UnitInfo *d2){
    return (d1->name < d2->name);
}
bool sortBySurName (const UnitInfo *d1, const UnitInfo *d2)
{
    if (sorts_descend){
        return (d1->lastname  > d2->lastname)
             ||(d1->lastname == d2->lastname && d1->name > d2->name);
    }else{
        return (d1->lastname  < d2->lastname)
             ||(d1->lastname == d2->lastname && d1->name < d2->name);
    }
}
bool sortByStress (const UnitInfo *d1, const UnitInfo *d2)
{
    if (!d1->unit->status.current_soul)
        return !sorts_descend;
    if (!d2->unit->status.current_soul)
        return sorts_descend;

    if (sorts_descend)
        return (d1->unit->status.current_soul->personality.stress_level > d2->unit->status.current_soul->personality.stress_level);
    else
        return (d1->unit->status.current_soul->personality.stress_level < d2->unit->status.current_soul->personality.stress_level);
}

bool sortByColumn (const UnitInfo *d1, const UnitInfo *d2)
{
    if (!d1->unit->status.current_soul)
        return false;
    if (!d2->unit->status.current_soul)
        return true;

    df::job_skill sort_skill = columns[column_sort_column].skill;
    df::unit_labor sort_labor = columns[column_sort_column].labor;

    int l1 =0;
    int l2 =0;

    if (sort_skill != job_skill::NONE)
    {
        l1 = unitSkillRating(d1,sort_skill)*50;
        l1 += 25+unitSkillExperience(d1,sort_skill)*40/(skill_levels[l1/50].points+1);
        //eg skill 1 = 50, 5 = 250, 10 = 500
        //l1 += d1->column_aptitudes[column_sort_column];
        l1 *= d1->column_aptitudes[column_sort_column]+300;

        l2 = unitSkillRating(d2,sort_skill)*50;
        l2 += 25+unitSkillExperience(d2,sort_skill)*45/(skill_levels[l2/50].points+1);
        //l2 += d2->column_aptitudes[column_sort_column];
        l2 *= d2->column_aptitudes[column_sort_column]+300;
    }

    if(sort_labor != unit_labor::NONE){
        if(d1->unit->status.labors[sort_labor])
            l1+=10+l1/10;
        if(d2->unit->status.labors[sort_labor])
            l2+=10+l2/10;
    }

    return l1 > l2;

}

bool sortByUnitId (const UnitInfo *d1, const UnitInfo *d2 ){
    return (d1->unit->id > d2->unit->id);
}//used to find arrival_groups


string itos (int n)
{
    stringstream ss;
    ss << n;
    return ss.str();
}

PersistentDataItem config_manipulator;
void save_manipulator_config()
{
    config_manipulator = World::GetPersistentData("manipulator/config");
    if (!config_manipulator.isValid()){
        config_manipulator = World::AddPersistentData("manipulator/config");
        if (!config_manipulator.isValid())
            return;
    }

    config_manipulator.ival(0) = color_mode;
    config_manipulator.ival(1) = 0;
    config_manipulator.ival(2) = 0;
    config_manipulator.ival(3) = 0;
    config_manipulator.ival(4) = 0;
    config_manipulator.ival(5) = 0;
    config_manipulator.ival(6) = 0;
}

void read_manipulator_config()
{
    config_manipulator = World::GetPersistentData("manipulator/config");

    if (!config_manipulator.isValid()){
        save_manipulator_config();
        return;
    }
    //sel_row=config_manipulator.ival(0);
    color_mode=config_manipulator.ival(0);
}

template<typename T>
class StringFormatter {
public:
    typedef string(*T_callback)(T);
    typedef std::tuple<string, string, T_callback> T_opt;
    typedef vector<T_opt> T_optlist;
    static bool compare_opts(const string &first, const string &second)
    {
        return first.size() > second.size();
    }
    StringFormatter() {}
    void add_option(string spec, string help, string (*callback)(T))
    {
        opt_list.push_back(std::make_tuple(spec, help, callback));
    }
    T_optlist *get_options() { return &opt_list; }
    void clear_options()
    {
        opt_list.clear();
    }
    string grab_opt (string s, size_t start)
    {
        vector<string> candidates;
        for (auto it = opt_list.begin(); it != opt_list.end(); ++it)
        {
            string opt = std::get<0>(*it);
            string slice = s.substr(start, opt.size());
            if (slice == opt)
                candidates.push_back(slice);
        }
        if (!candidates.size())
            return "";
        // Select the longest candidate
        std::sort(candidates.begin(), candidates.end(), StringFormatter<T>::compare_opts);
        return candidates[0];
    }
    T_callback get_callback (string s)
    {
        for (auto it = opt_list.begin(); it != opt_list.end(); ++it)
        {
            if (std::get<0>(*it) == s)
                return std::get<2>(*it);
        }
        return NULL;
    }
    string format (T obj, string fmt)
    {
        string dest = "";
        bool in_opt = false;
        size_t i = 0;
        while (i < fmt.size())
        {
            if (in_opt)
            {
                if (fmt[i] == '%')
                {
                    // escape: %% -> %
                    in_opt = false;
                    dest.push_back('%');
                    i++;
                }
                else
                {
                    string opt = grab_opt(fmt, i);
                    if (opt.size())
                    {
                        T_callback func = get_callback(opt);
                        if (func != NULL)
                            dest += func(obj);
                        i += opt.size();
                        in_opt = false;
                        if (i < fmt.size() && fmt[i] == '$')
                            // Allow $ to terminate format options
                            i++;
                    }
                    else
                    {
                        // Unrecognized format option; replace with original text
                        dest.push_back('%');
                        in_opt = false;
                    }
                }
            }
            else
            {
                if (fmt[i] == '%')
                    in_opt = true;
                else
                    dest.push_back(fmt[i]);
                i++;
            }
        }
        return dest;
    }
protected:
    T_optlist opt_list;
};

static bool labor_skill_map_made = false;

namespace unit_info_ops{

    //these are copied from managelabor.cpp ( skill_attr_weight )
    struct skill_attrib_weight {
        int8_t phys_attr_weights [6];
        int8_t mental_attr_weights [13];
    };

    const skill_attrib_weight skills_attribs[] =
    { //weights can be 0 to 7,  bit3 set (8) means attr is not excercised
      //but might possibly affect skills work (like musical_feel > musician)
    //  S A T E R0D0   AA F W C I P M LASSMKSE SA
    { { 1,0,1,1,0,0 },{ 0,0,1,0,0,0,0,0,1,0,0,0,0 } } /* MINING */,
    { { 1,1,0,1,0,0 },{ 0,0,1,0,0,0,0,0,1,0,0,0,0 } } /* WOODCUTTING */,
    { { 1,1,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* CARPENTRY */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* DETAILSTONE */,
    { { 1,1,0,1,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* MASONRY */,
    { { 0,1,1,1,0,0 },{ 0,0,0,0,1,1,0,0,0,0,0,1,0 } } /* ANIMALTRAIN */,
    { { 0,1,0,0,0,0 },{ 1,0,0,0,0,0,1,0,0,0,0,1,0 } } /* ANIMALCARE */,
    { { 0,1,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* DISSECT_FISH */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* DISSECT_VERMIN */,
    { { 0,1,0,1,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* PROCESSFISH */,
    { { 0,1,0,1,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* BUTCHER */,
    { { 0,1,0,0,0,0 },{ 1,0,0,1,0,0,0,0,1,0,0,0,0 } } /* TRAPPING */,
    { { 0,1,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* TANNER */,
    { { 0,0,0,0,0,0 },{ 0,0,0,1,0,0,0,0,1,0,0,0,0 } } /* WEAVING */,
    { { 1,1,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* BREWING */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* ALCHEMY */,
    { { 0,1,0,0,0,0 },{ 0,0,0,1,0,0,0,0,1,0,0,0,0 } } /* CLOTHESMAKING */,
    { { 1,1,0,1,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* MILLING */,
    { { 1,1,0,1,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* PROCESSPLANTS */,
    { { 1,1,0,1,0,0 },{ 1,0,0,1,0,0,0,0,0,0,0,0,0 } } /* CHEESEMAKING */,
    { { 1,1,0,1,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* MILK */,
    { { 0,1,0,0,0,0 },{ 1,0,0,1,0,0,0,0,0,0,0,0,0 } } /* COOK */,
    { { 1,1,0,1,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* PLANT */,
    { { 0,1,0,0,0,0 },{ 0,0,0,0,0,0,1,0,0,0,0,0,0 } } /* HERBALISM */,
    { { 1,1,0,0,0,0 },{ 0,1,0,0,0,1,0,0,0,0,0,0,0 } } /* FISH */,
    { { 1,0,1,1,0,0 },{ 1,0,0,0,0,0,0,0,0,0,0,0,0 } } /* SMELT */,
    { { 1,1,0,1,0,0 },{ 1,0,0,0,0,0,0,0,0,0,0,0,0 } } /* EXTRACT_STRAND */,
    { { 1,1,0,1,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* FORGE_WEAPON */,
    { { 1,1,0,1,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* FORGE_ARMOR */,
    { { 1,1,0,1,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* FORGE_FURNITURE */,
    { { 0,1,0,0,0,0 },{ 1,0,0,0,0,0,0,0,0,0,0,0,0 } } /* CUTGEM */,
    { { 0,1,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* ENCRUSTGEM */,
    { { 0,1,0,0,0,0 },{ 0,0,0,1,0,0,0,0,1,0,0,0,0 } } /* WOODCRAFT */,
    { { 0,1,0,0,0,0 },{ 0,0,0,1,0,0,0,0,1,0,0,0,0 } } /* STONECRAFT */,
    { { 1,1,0,1,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* METALCRAFT */,
    { { 1,1,0,1,0,0 },{ 0,0,0,1,0,0,0,0,1,0,0,0,0 } } /* GLASSMAKER */,
    { { 1,1,0,1,0,0 },{ 0,0,0,1,0,0,0,0,1,0,0,0,0 } } /* LEATHERWORK */,
    { { 0,1,0,0,0,0 },{ 0,0,0,1,0,0,0,0,1,0,0,0,0 } } /* BONECARVE */,
    { { 1,1,1,0,9,9 },{ 0,0,0,0,0,0,0,0,1,0,0,0,9 } } /* AXE */,
    { { 1,1,1,0,9,9 },{ 0,0,0,0,0,0,0,0,1,0,0,0,9 } } /* SWORD */,
    { { 1,1,1,0,9,9 },{ 0,0,0,0,0,0,0,0,1,0,0,0,9 } } /* DAGGER */,
    { { 1,1,1,0,9,9 },{ 0,0,0,0,0,0,0,0,1,0,0,0,9 } } /* MACE */,
    { { 1,1,1,0,9,9 },{ 0,0,0,0,0,0,0,0,1,0,0,0,9 } } /* HAMMER */,
    { { 1,1,1,0,9,9 },{ 0,0,0,0,0,0,0,0,1,0,0,0,9 } } /* SPEAR */,
    { { 1,1,0,0,0,0 },{ 0,0,0,0,0,0,0,0,1,0,0,0,0 } } /* CROSSBOW */,
    { { 1,1,1,0,0,0 },{ 0,0,0,0,0,0,0,0,1,0,0,0,0 } } /* SHIELD */,
    { { 1,0,1,1,0,0 },{ 0,0,0,0,0,0,0,0,1,0,0,0,0 } } /* ARMOR */,
    { { 1,1,0,1,0,0 },{ 1,0,0,0,0,0,0,0,0,0,0,0,0 } } /* SIEGECRAFT */,
    { { 1,0,1,1,0,0 },{ 1,1,0,0,0,0,0,0,1,0,0,0,0 } } /* SIEGEOPERATE */,
    { { 0,1,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* BOWYER */,
    { { 1,1,1,0,9,9 },{ 0,0,0,0,0,0,0,0,1,0,0,0,9 } } /* PIKE */,
    { { 1,1,1,0,9,9 },{ 0,0,0,0,0,0,0,0,1,0,0,0,9 } } /* WHIP */,
    { { 1,1,0,0,0,0 },{ 0,1,0,0,0,0,0,0,1,0,0,0,0 } } /* BOW */,
    { { 1,1,0,0,0,0 },{ 0,0,0,0,0,0,0,0,1,0,0,0,0 } } /* BLOWGUN */,
    { { 1,1,0,0,0,0 },{ 0,0,0,0,0,0,0,0,1,0,0,0,0 } } /* THROW */,
    { { 1,1,0,1,0,0 },{ 1,0,0,0,0,0,0,0,0,0,0,0,0 } } /* MECHANICS */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* MAGIC_NATURE */,
    { { 0,0,0,0,0,0 },{ 0,1,0,0,0,0,0,0,1,0,0,0,0 } } /* SNEAK */,
    { { 0,0,0,0,0,0 },{ 1,0,0,1,0,0,0,0,1,0,0,0,0 } } /* DESIGNBUILDING */,
    { { 0,1,0,0,0,0 },{ 0,0,0,0,0,0,0,0,1,0,0,1,0 } } /* DRESS_WOUNDS */,
    { { 0,0,0,0,0,9 },{ 1,0,0,0,1,0,1,0,0,0,0,0,9 } } /* DIAGNOSE */,
    { { 0,1,0,0,0,9 },{ 0,1,0,0,0,0,0,0,1,0,0,0,9 } } /* SURGERY */,
    { { 1,1,0,0,0,0 },{ 0,1,0,0,0,0,0,0,1,0,0,0,0 } } /* SET_BONE */,
    { { 0,1,0,0,0,0 },{ 0,1,0,0,0,0,0,0,1,0,0,0,0 } } /* SUTURE */,
    { { 0,1,0,1,0,0 },{ 0,0,1,0,0,0,0,0,1,0,0,0,0 } } /* CRUTCH_WALK */,
    { { 1,0,1,1,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* WOOD_BURNING */,
    { { 1,0,1,1,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* LYE_MAKING */,
    { { 1,0,1,1,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* SOAP_MAKING */,
    { { 1,0,1,1,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* POTASH_MAKING */,
    { { 1,1,0,1,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* DYER */,
    { { 1,0,1,1,0,0 },{ 0,0,1,0,0,0,0,0,0,0,0,0,0 } } /* OPERATE_PUMP */,
    { { 1,1,0,1,0,0 },{ 0,0,1,0,0,0,0,0,1,0,0,0,0 } } /* SWIMMING */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,1,0,0,0,1,1 } } /* PERSUASION */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,1,0,0,0,1,1 } } /* NEGOTIATION */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,1,0,0,0,0,0,0,1,1 } } /* JUDGING_INTENT */,
    { { 0,0,0,0,0,0 },{ 1,0,0,0,1,0,1,0,0,0,0,0,0 } } /* APPRAISAL */,
    { { 0,0,0,0,0,0 },{ 1,0,0,1,0,0,0,0,0,0,0,0,1 } } /* ORGANIZATION */,
    { { 0,0,0,0,0,0 },{ 1,1,0,0,0,0,1,0,0,0,0,0,9 } } /* RECORD_KEEPING */,
    { { 0,0,0,0,0,0 },{ 0,0,0,1,0,0,0,1,0,0,0,0,1 } } /* LYING */,
    { { 0,1,0,0,0,0 },{ 0,0,0,0,0,0,0,1,0,0,0,0,9 } } /* INTIMIDATION */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,1,0,0,0,1,1 } } /* CONVERSATION */,
    { { 0,1,0,0,0,0 },{ 0,0,0,1,0,0,0,1,0,0,0,0,9 } } /* COMEDY */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,1,0,0,0,1,1 } } /* FLATTERY */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,1,0,0,0,1,9 } } /* CONSOLE */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,1,0,0,0,1,1 } } /* PACIFY */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* TRACKING */,
    { { 0,1,0,0,0,9 },{ 1,1,0,0,0,0,1,0,0,0,0,0,0 } } /* KNOWLEDGE_ACQUISITION*/,
    { { 0,0,0,0,0,0 },{ 0,1,1,0,0,1,0,0,0,0,0,0,0 } } /* CONCENTRATION */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* DISCIPLINE */,
    { { 0,0,0,0,0,0 },{ 0,1,0,0,1,0,0,0,1,0,0,0,9 } } /* SITUATIONAL_AWARENESS*/,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,9,0,0,0,0,0 } } /* WRITING */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,9,0,0,0,0,0 } } /* PROSE */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,9,0,0,0,0,0 } } /* POETRY */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* READING */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,9 } } /* SPEAKING */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* COORDINATION */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* BALANCE */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,1,0,0,0,1,1 } } /* LEADERSHIP */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,1,0,0,0,1,1 } } /* TEACHING */,
    { { 1,1,1,0,0,0 },{ 0,0,1,0,0,0,0,0,1,0,0,0,0 } } /* MELEE_COMBAT */,
    { { 1,1,0,0,0,9 },{ 0,0,0,0,0,0,0,0,1,0,0,0,0 } } /* RANGED_COMBAT */,
    { { 1,1,1,0,0,9 },{ 0,0,0,0,0,0,0,0,1,0,0,0,0 } } /* WRESTLING */,
    { { 1,0,1,1,0,0 },{ 0,0,0,0,0,0,0,0,1,0,0,0,0 } } /* BITE */,
    { { 1,1,1,0,0,0 },{ 0,0,0,0,0,0,0,0,1,0,0,0,0 } } /* GRASP_STRIKE */,
    { { 1,1,1,0,0,0 },{ 0,0,0,0,0,0,0,0,1,0,0,0,0 } } /* STANCE_STRIKE */,
    { { 0,1,0,1,0,0 },{ 0,0,0,0,0,0,0,0,1,0,0,0,0 } } /* DODGING */,
    { { 1,1,1,0,0,0 },{ 0,0,0,0,0,0,0,0,1,0,0,0,0 } } /* MISC_WEAPON */,
    { { 0,0,0,0,0,0 },{ 1,0,0,0,0,0,0,0,1,0,0,0,0 } } /* KNAPPING */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* MILITARY_TACTICS */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* SHEARING */,
    { { 1,1,0,1,0,0 },{ 0,0,0,0,0,0,0,0,1,0,0,0,0 } } /* SPINNING */,
    { { 0,1,0,0,0,0 },{ 0,0,0,1,0,0,0,0,1,0,0,0,0 } } /* POTTERY */,
    { { 0,1,0,0,0,0 },{ 0,0,0,1,0,0,0,0,1,0,0,0,0 } } /* GLAZING */,
    { { 1,1,0,1,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* PRESSING */,
    { { 1,1,0,1,0,0 },{ 1,0,0,0,0,0,0,0,0,0,0,0,0 } } /* BEEKEEPING */,
    { { 0,1,0,0,0,0 },{ 0,0,0,1,0,0,0,0,1,0,0,0,0 } } /* WAX_WORKING */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* CLIMBING */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* GELD */,
    { { 0,9,0,9,0,0 },{ 0,0,0,0,0,0,0,0,0,9,0,0,0 } } /* DANCE */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,9,0,0,0 } } /* MAKE_MUSIC */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,9,0,0,0 } } /* SING_MUSIC */,
    { { 0,0,0,0,0,0 },{ 0,9,0,0,0,0,0,0,0,9,0,0,0 } } /* PLAY_KEYBOARD_INST*/,
    { { 0,0,0,0,0,0 },{ 0,9,0,0,0,0,0,0,0,9,0,0,0 } } /* PLAY_STRINGED_INST*/,
    { { 0,0,0,0,0,0 },{ 0,9,0,0,0,0,0,0,0,9,0,0,0 } } /* PLAY_WIND_INSTRUMENT*/,
    { { 0,0,0,0,0,0 },{ 0,9,0,0,0,0,0,0,0,9,0,0,0 } } /* PLAY_PERCUSSION_INS*/,
    { { 0,0,0,0,0,0 },{ 9,0,0,0,0,0,0,0,0,0,0,0,0 } } /* CRITICAL_THINKING */,
    { { 0,0,0,0,0,0 },{ 9,0,0,0,0,0,9,0,0,0,0,0,0 } } /* LOGIC */,
    { { 0,0,0,0,0,0 },{ 9,9,0,9,9,0,9,0,0,0,0,0,0 } } /* MATHEMATICS */,
    { { 0,0,0,0,0,0 },{ 0,9,0,9,0,9,9,0,0,0,0,0,0 } } /* ASTRONOMY */,
    { { 0,0,0,0,0,0 },{ 9,9,0,0,0,9,9,0,0,0,0,0,0 } } /* CHEMISTRY */,
    { { 0,0,0,0,0,0 },{ 9,0,0,9,0,0,9,0,0,0,0,0,0 } } /* GEOGRAPHY */,
    { { 0,0,0,0,0,0 },{ 9,9,0,0,0,0,9,0,0,0,0,0,0 } } /* OPTICS_ENGINEER */,
    { { 0,0,0,0,0,0 },{ 9,9,0,0,0,0,9,0,0,0,0,0,0 } } /* FLUID_ENGINEER */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* PAPERMAKING */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* BOOKBINDING */
    };

    df::job_skill labor_skill_map[ENUM_LAST_ITEM(unit_labor) + 1];
    //similar to labormanager.cpp : generate_labor_to_skill_map()
    void make_labor_skill_map()
    {
        int e=ENUM_LAST_ITEM(unit_labor);
        for (int i = 0; i <= e; i++)
            labor_skill_map[i] = df::job_skill::NONE;

        FOR_ENUM_ITEMS(job_skill, skill)
        {
            int labor = ENUM_ATTR(job_skill, labor, skill);
            if (labor >-1 && labor <= e )
            {
                labor_skill_map[labor] = skill;
            }
        }
    }

    int aptitude_for_role(UnitInfo *d, df::unit_labor labor, df::job_skill skill)
    {
        if(!labor_skill_map_made)
        {
            make_labor_skill_map();
            labor_skill_map_made = false;
        }

        int attr_weight = 0;

        if (labor != unit_labor::NONE && skill == job_skill::NONE)
            skill = labor_skill_map[labor];

        if( skill == job_skill::NONE ) return 0;

        int weights=0, wg=0, bitmask=7;
        for (int pa = 0; pa < 6; pa++)
        {
            wg = bitmask & skills_attribs[skill].phys_attr_weights[pa];
            if( wg != 0 )
            {
                weights+=wg;
                attr_weight += wg * (d->unit->body.physical_attrs[pa].value );
            }
        }

        for (int ma = 0; ma < 13; ma++)
        {
            wg= bitmask & skills_attribs[skill].mental_attr_weights[ma];
            if( wg != 0 )
            {
                weights+=wg;
                attr_weight += wg * (d->unit->status.current_soul->mental_attrs[ma].value );
            }
        }

        if( weights > 1 ) attr_weight /= weights;

        if(attr_weight<13&&attr_weight>0)
        {   attr_weight=1; }
        else
        {   attr_weight=((attr_weight+12)/25); }

        if(attr_weight>200) attr_weight=200;

        return attr_weight;
    }

    void calcAptScores(vector<UnitInfo *> &units)  //!
    {
        int unit_workapt_sum = 0;
        int unit_skillapt_sum = 0;
        int alls_workapt_sum = 0;
        int alls_skillapt_sum = 0;
        int unit_work_count = 0;
        int unit_skill_count = 0;
        int all_work_count = 0;
        int all_skill_count = 0;
        int uinfo_avg_work_aptitude = 0;
        int uinfo_avg_skill_aptitude = 0;
        int column_total_apt[NUM_LABORS] = {};
        int column_total_skill[NUM_LABORS] = {};

        int unit_count = units.size();
        int col_end = NUM_LABORS-1;

        for (size_t i = 0; i < unit_count; i++)
        {
            UnitInfo *cur = units[i];

            for (size_t j = 0; j <= col_end; j++)
            {
                df::unit_labor cur_labor = columns[j].labor;
                df::job_skill cur_skill = columns[j].skill;

                int sco = unit_info_ops::aptitude_for_role( cur, cur_labor, cur_skill );

                cur->column_aptitudes[j] = (uint8_t)sco;

                if(cur_labor != unit_labor::NONE
                 ||columns[j].profession != profession::NONE)
                {
                    if(sco) unit_work_count++;
                    unit_workapt_sum += sco;
                }
                else //skill onlys
                {
                    if(sco) unit_skill_count++;
                    unit_skillapt_sum += sco;
                }

                int wskill = unitSkillRating(cur,cur_skill);

                column_total_apt[j] += sco;
                column_total_skill[j] += wskill;

                if ( j == col_end ) //last col. next unit...
                {
                    cur->work_aptitude = (unit_work_count==0)?0
                        :unit_workapt_sum/unit_work_count;

                    cur->skill_aptitude = (unit_skill_count==0)?0
                        :unit_skillapt_sum/unit_skill_count;

                    alls_workapt_sum += unit_workapt_sum;
                    alls_skillapt_sum += unit_skillapt_sum;

                    all_work_count   += unit_work_count;
                    all_skill_count  += unit_skill_count;

                    unit_workapt_sum  = unit_work_count  = 0;
                    unit_skillapt_sum = unit_skill_count = 0;
                }
            }
        }

        uinfo_avg_work_aptitude  = (alls_workapt_sum+1)/(all_work_count+1);
        uinfo_avg_skill_aptitude = (alls_skillapt_sum+1)/(all_skill_count+1);
        int uinfo_avg_aptitude =
            (alls_workapt_sum+alls_skillapt_sum+1)
            /(all_skill_count+all_work_count+1);

        //sweep again to set hints
        for (size_t i = 0; i < unit_count; i++)
        {
            UnitInfo *cur = units[i];

            for (size_t j = 0; j <= col_end; j++)
            {
                int col_avg = column_total_apt[j]/unit_count;
                int col_dif = ((col_avg-uinfo_avg_aptitude)*3)/4;
                int apt = cur->column_aptitudes[j];

                if(apt<1) { //0 apts are set to middle hint
                    cur->column_hints[j]=1;
                    continue;
                }

                int uinfo_avg, unit_avg;

                if((columns[j].labor != unit_labor::NONE
                 ||columns[j].profession != profession::NONE))
                {
                    uinfo_avg = uinfo_avg_work_aptitude;
                    unit_avg = cur->work_aptitude;
                }else{
                    uinfo_avg = uinfo_avg_skill_aptitude;
                    unit_avg = cur->skill_aptitude;
                }

                int higbar, lowbar, kinbar, hint;

                apt *= 100; //scale for integer math

                //adjust hint so whole columns dont express as much
                higbar=(unit_avg+col_dif)*124; //1.2
                lowbar=(unit_avg+col_dif)*83;  //0.8
                kinbar=(unit_avg*6+col_avg*2)*12;

                if(apt>((kinbar*117)/100)){ hint=1; } else { hint=0; }

                if(apt>higbar) hint++;
                if(apt>lowbar) hint++;

                if(apt<higbar && unit_avg<uinfo_avg){
                    //may have many lows, no mids ..
                    if(apt-col_dif > unit_avg*99) hint=1;
                }

                if(apt>lowbar && unit_avg>uinfo_avg){
                    //may have too many highs, no mids ..
                    if(apt-col_dif < unit_avg*106) hint=1;
                }

                cur->column_hints[j] = hint>2?2:hint;

            }//cols
        }//units

        //sweep again to set skill class scores
        for (size_t i = 0; i < unit_count; i++)
        {
            UnitInfo *cur = units[i];
            cur->scholar=0;
            cur->performer=0;
            cur->martial=0;
            cur->topskilled=0;

            for (size_t j = 0; j <= col_end; j++)
            {
                int avg_skill= column_total_skill[j]/unit_count;

                df::unit_labor col_labor = columns[j].labor;
                df::job_skill col_skill = columns[j].skill;

                int group = columns[j].group;

                int mskill = unitSkillRating(cur,col_skill);
                int wskill = unitSkillRating(cur,col_skill);
                //med is ~70
                wskill = wskill*4 + static_cast<int>(sqrt((wskill+3)*(150+cur->column_aptitudes[j])/4));

                //0 0 + sqrt(200)  ~14
                //1 12 + sqrt(250) ~28
                //8 100 + sqrt(500) ~122

                if( group<11 &&wskill>(avg_skill*3)/4 ){
                    if(col_skill==df::job_skill::WOODCUTTING
                     ||col_skill==df::job_skill::DISSECT_VERMIN
                     ||col_skill==df::job_skill::GELD
                     ||col_skill==df::job_skill::MILK
                     ||col_skill==df::job_skill::SHEARING
                     ||col_skill==df::job_skill::WOOD_BURNING){
                        cur->topskilled+=
                            static_cast<int>(wskill*sqrt(static_cast<double>(mskill)/static_cast<double>(10+avg_skill*2)));
                    }else{
                        cur->topskilled+=
                            static_cast<int>(wskill*sqrt(static_cast<double>(mskill)/static_cast<double>(5+avg_skill)));
                    }
                }

                //martial
                if( (group>12 && group < 16)
                  ||col_skill==df::job_skill::MILITARY_TACTICS
                  ||col_skill==job_skill::PACIFY){
                  if(col_skill==df::job_skill::TEACHING){
                      cur->martial+=(cur->martial*wskill)/(70*70*20);
                  }else if(col_skill==df::job_skill::KNOWLEDGE_ACQUISITION){
                      cur->martial+=wskill/3;
                  }else{
                      cur->martial+=wskill;
                 }

                  if(group<15 ||col_skill==df::job_skill::DISCIPLINE
                    ||col_skill==df::job_skill::DODGING)
                      cur->martial+=wskill/2; //extra important
                }

                //scholar
                if(col_skill==df::job_skill::TEACHING
                  ||col_skill==df::job_skill::CONCENTRATION
                  ||col_skill==df::job_skill::WRITING
                  ||col_skill==df::job_skill::READING
                  ||col_skill==df::job_skill::KNOWLEDGE_ACQUISITION
                  ||col_skill==df::job_skill::CONVERSATION
                  ||group==21){
                    cur->scholar+=wskill;
                }

                //performer
                if( group == 19 && col_skill != df::job_skill::WRITING
                ||col_skill==job_skill::COMEDY
                ||col_skill==job_skill::FLATTERY
                ||col_skill==job_skill::CONSOLE
                ||col_skill==job_skill::PACIFY){
                    if(group == 19)
                        cur->performer+=wskill;
                    else
                        cur->performer+=wskill/2;
                }
            }
        }
        work_aptitude_avg = uinfo_avg_work_aptitude ;
        skill_aptitude_avg = uinfo_avg_skill_aptitude;

    }//end of scoring.. next comes notice string gen

    //from cursecheck.cpp
    string determineCurse(df::unit * unit)
    {
        string cursetype = "";
        if(unit->curse_year == -1) return cursetype;

        cursetype = "Shade "; //an unresting soul ghost was this

        if(unit->flags3.bits.ghostly)
            cursetype = "Ghost "; //yet not this

        if( (unit->curse.add_tags1.bits.OPPOSED_TO_LIFE || unit->curse.add_tags1.bits.NOT_LIVING)
            && !unit->curse.add_tags1.bits.BLOODSUCKER )
            cursetype = "Zmbie ";

        if(!unit->curse.add_tags1.bits.NOT_LIVING
            && unit->curse.add_tags1.bits.NO_EAT
            && unit->curse.add_tags1.bits.NO_DRINK
            && unit->curse.add_tags2.bits.NO_AGING
            )
            cursetype = "Ncrmncr ";

        if(!unit->curse.add_tags1.bits.NOT_LIVING
            && !unit->curse.add_tags1.bits.NO_EAT
            && !unit->curse.add_tags1.bits.NO_DRINK
            && unit->curse.add_tags2.bits.NO_AGING )
            cursetype = "Werebst ";

        if(unit->curse.add_tags1.bits.BLOODSUCKER)
            cursetype = "Vampr ";

        return cursetype;
    }

    void calcNotices(vector<UnitInfo *> &units)
    {
        for (size_t i = 0; i < units.size(); i++)
        {
            df::unit *curu = units[i]->unit;

            if (!curu->status.current_soul)
                continue;

            string lks="";
            string iss="";
            int iscore=0;
            int newwnd=0;
            int oldwnd=0;

            for ( size_t a = 0; a < curu->body.wounds.size(); a++ ) {
                df::unit_wound* wound = curu->body.wounds[a];
                if ( wound->age!=0 && wound->age < 111 ) {
                    newwnd++;
                }else if ( wound->age >= 500 ) {
                    //oldwnd++;
                }
            }

            iss+=determineCurse(units[i]->unit);
            //these are so much easier on one line, fingersx lint passes...
            if(curu->flags2.bits.underworld){
                iss+="Undrwld "; iscore+=2000;
            }
            if(curu->flags1.bits.dead){
                iss+="Dead "; iscore+=1330;
            }
            if(curu->flags3.bits.available_for_adoption){
                iss+="Orphan "; iscore+=600;
            }
            if(curu->flags1.bits.marauder){
                iss+="Maraudr "; iscore+=900;
            }
            if(curu->flags1.bits.active_invader){
                iss+="Invadr "; iscore+=900;
            }
            else if(curu->flags2.bits.visitor_uninvited){
                iss+="Intrudr "; iscore+=900;
            }
            //~ if(!curu->flags1.bits.important_historical_figure){ iss+="Fauna "; iscore+=1;}

            if(curu->flags1.bits.has_mood){
                iss+="Mood "; iscore+=305;
            }
            if(curu->flags1.bits.drowning){
                iss+="Drowning "; iscore+=2000;
            }
            if(curu->flags1.bits.projectile){
                iss+="Falling "; iscore+=945;
            }
            if(curu->flags3.bits.dangerous_terrain){
                iss+="DngrTrrn "; iscore+=600;
            }
            if(curu->flags3.bits.floundering){
                iss+="Flundrg "; iscore+=600;
            }

            if(curu->counters.suffocation){
                iss+="Suffcatn "; iscore+=2000;
            }
            if(curu->counters.pain>10){
                iss+="Agony "; iscore+=700;
            }

            if(curu->health){
                if (curu->health->flags.bits.needs_recovery){
                    iss+="RqRescue "; iscore+=1600;
                }else if(curu->health->flags.bits.rq_diagnosis){
                    iss+="RqDoctor "; iscore+=1200;
                }else if (curu->health->flags.bits.rq_immobilize
                   ||curu->health->flags.bits.rq_surgery
                   ||curu->health->flags.bits.rq_traction
                   ||curu->health->flags.bits.rq_immobilize)
                {
                    iss+="NdSurgry "; iscore+=1300;
                }else if(curu->health->flags.bits.rq_dressing
                    ||curu->health->flags.bits.rq_cleaning
                    ||curu->health->flags.bits.rq_suture
                    ||curu->health->flags.bits.rq_setting
                    ||curu->health->flags.bits.rq_crutch)
                {
                    iss+="NdNurse "; iscore+=700;
                }else if(newwnd&&iscore<1308){
                    iss+="Ruffdup "; iscore+=403;
                }
            }

            if(curu->flags1.bits.caged){
                iss+=  "Trappd "; iscore+=500;
            }
            if(curu->flags1.bits.chained){
                iss+="Chaind "; iscore+=500;
            }

            int exh=curu->counters2.exhaustion;
            if(exh>5000){
                iss+="Xhaustd "; iscore+=601;
            }

            if(curu->flags2.bits.vision_damaged){
                iss+="VisnDmg "; iscore+=809;
            }
            if(curu->flags2.bits.breathing_problem){
                iss+="LungDmg "; iscore+=608;
            }
            if(curu->counters2.stored_fat<8000){
                iss+="Emaciatd "; iscore+=905;
            }
            else if(curu->counters2.stored_fat<3000&&curu->counters2.hunger_timer>3000){
                iss+="DyngStarvtn "; iscore+=2000;
            }
            if(curu->flags3.bits.injury_thought){
                iss+="Pained "; iscore+=402;
            }

            if(curu->syndromes.active.size()>2){
                iss+="vCranky "; iscore+=403;
            }
            else if(curu->syndromes.active.size()>1){
                iss+="Cranky "; iscore+=403;
            }
            //if(oldwnd){ iss+="Hurt "; iscore+=100; }

            if ( curu->status2.limbs_grasp_max == 2){
                if ( curu->status2.limbs_grasp_count == 1){
                    iss+="OneHndd "; iscore+=107;
                }else if ( curu->status2.limbs_grasp_count == 1){
                    iss+="NoGrasp "; iscore+=308;
                }
            }

            if ( curu->status2.limbs_stand_max == 2){
                if ( curu->status2.limbs_stand_count == 1){
                    iss+="OneLegd "; iscore+=103;
                }else if ( curu->status2.limbs_stand_count == 1){
                    iss+="BadLegs "; iscore+=303;
                }
            }

            if(curu->flags3.bits.emotionally_overloaded){
                iss+="Despair "; iscore+=1013;
            }

            //if(curu->counters.unconscious){ iss+="Unconsc. "; iscore+=800;}
            if(curu->counters2.numbness){
                iss+="Numb "; iscore+=1055;
            }
            if(curu->counters.stunned){
                iss+="Stund "; iscore+=633;
            }
            if(curu->counters.nausea>1000){
                iss+="Naus "; iscore+=210;
            }
            if(curu->counters.dizziness>1000){
                iss+="Dzzy "; iscore+=500;
            }
            if(curu->counters2.paralysis){
                iss+="Parlysd "; iscore+=3005;
            }
            if(curu->counters2.fever>10){
                iss+="Fever "; iscore+=1002;
            }
            if(curu->counters2.thirst_timer>30000){
                iss+="Dehydrt "; iscore+=1005;
            }
            if(curu->counters2.sleepiness_timer>55000){
                iss+="vSleepy "; iscore+=609;
            }
            if(exh<=5000&&exh>2500){
                iss+="Tired "; iscore+=202;
            }

            //units[i]->likes="na"; //not done yet
            units[i]->notices=iss;
            units[i]->notice_score=iscore;
        }
    }

}//namespace unit_info_ops

namespace unit_ops {
    string get_real_name(UnitInfo *u)
        { return Translation::TranslateName(&u->unit->name, false); }
    string get_nickname(UnitInfo *u)
        { return Translation::TranslateName(Units::getVisibleName(u->unit), false); }
    string get_real_name_eng(UnitInfo *u)
        { return Translation::TranslateName(&u->unit->name, true); }
    string get_nickname_eng(UnitInfo *u)
        { return Translation::TranslateName(Units::getVisibleName(u->unit), true); }
    string get_first_nickname(UnitInfo *u)
    {
        return Translation::capitalize(u->unit->name.nickname.size() ?
            u->unit->name.nickname : u->unit->name.first_name);
    }
    string get_first_name(UnitInfo *u)
        { return Translation::capitalize(u->unit->name.first_name); }
    string get_last_name(UnitInfo *u)
    {
        df::language_name name = u->unit->name;
        string ret = "";
        for (int i = 0; i < 2; i++)
        {
            if (name.words[i] >= 0)
                ret += *world->raws.language.translations[name.language]->words[name.words[i]];
        }
        return Translation::capitalize(ret);
    }
    string get_last_name_eng(UnitInfo *u)
    {
        df::language_name name = u->unit->name;
        string ret = "";
        for (int i = 0; i < 2; i++)
        {
            if (name.words[i] >= 0)
                ret += world->raws.language.words[name.words[i]]->forms[name.parts_of_speech[i].value];
        }
        return Translation::capitalize(ret);
    }
    string get_profname(UnitInfo *u)
        { return Units::getProfessionName(u->unit); }
    string get_real_profname(UnitInfo *u)
    {
        string tmp = u->unit->custom_profession;
        u->unit->custom_profession = "";
        string ret = get_profname(u);
        u->unit->custom_profession = tmp;
        return ret;
    }
    string get_base_profname(UnitInfo *u)
    {
        return ENUM_ATTR_STR(profession, caption, u->unit->profession);
    }
    string get_short_profname(UnitInfo *u)
    {
        for (int i = 0; i < NUM_LABORS; i++)
        {
            if (columns[i].profession == u->unit->profession)
                return string(columns[i].label);
        }
        return "??";
    }
    #define id_getter(id) \
    string get_##id(UnitInfo *u) \
        { return itos(u->ids.id); }
    id_getter(list_id);
    id_getter(list_id_prof);
    id_getter(list_id_group);
    #undef id_getter
    string get_unit_id(UnitInfo *u)
        { return itos(u->unit->id); }
    string get_age(UnitInfo *u)
        { return itos((int)Units::getAge(u->unit)); }
    string get_arrival(UnitInfo *u)
        { return itos(u->arrival_group); }

    void set_nickname(UnitInfo *u, std::string nick)
    {
        Units::setNickname(u->unit, nick);
        u->name = get_nickname(u);
        u->transname = get_nickname_eng(u);
    }
    void set_profname(UnitInfo *u, std::string prof)
    {
        u->unit->custom_profession = prof;
        u->profession = get_profname(u);
    }
}

struct ProfessionTemplate
{
    std::string name;
    bool mask;
    std::vector<df::unit_labor> labors;

    bool load(string directory, string file)
    {
        cerr << "Attempt to load " << file << endl;
        std::ifstream infile(directory + "/" + file);
        if (infile.bad()) {
            return false;
        }

        std::string line;
        name = file; // If no name is given we default to the filename
        mask = false;
        while (std::getline(infile, line)) {
            if (strcmp(line.substr(0,5).c_str(),"NAME ")==0)
            {
                auto nextInd = line.find(' ');
                name = line.substr(nextInd + 1);
                continue;
            }
            if (line == "MASK")
            {
                mask = true;
                continue;
            }

            for (int i = 0; i < NUM_LABORS; i++)
            {
                if (line == ENUM_KEY_STR(unit_labor, columns[i].labor))
                {
                    labors.push_back(columns[i].labor);
                }
            }
        }

        return true;
    }
    bool save(string directory)
    {
        std::ofstream outfile(directory + "/" + name);
        if (outfile.bad())
            return false;

        outfile << "NAME " << name << std::endl;
        if (mask)
            outfile << "MASK" << std::endl;

        for (int i = 0; i < NUM_LABORS; i++)
        {
            if (hasLabor(columns[i].labor))
            {
                outfile << ENUM_KEY_STR(unit_labor, columns[i].labor) << std::endl;
            }
        }

        outfile.flush();
        outfile.close();
        return true;
    }

    void apply(UnitInfo* u)
    {
        if (!mask && name.size() > 0)
            unit_ops::set_profname(u, name);

        for (int i = 0; i < NUM_LABORS; i++)
        {
            df::unit_labor labor = columns[i].labor;
            bool status = hasLabor(labor);

            if (!mask || status) {
                u->unit->status.labors[labor] = status;
            }
        }
    }

    void fromUnit(UnitInfo* u)
    {
        for (int i = 0; i < NUM_LABORS; i++)
        {
            if (u->unit->status.labors[columns[i].labor])
                labors.push_back(columns[i].labor);
        }

    }

    bool hasLabor (df::unit_labor labor)
    {
        return std::find(labors.begin(), labors.end(), labor) != labors.end();
    }
};

static std::string professions_folder = Filesystem::getcwd() + "/professions";
class ProfessionTemplateManager
{
public:
    std::vector<ProfessionTemplate> templates;

    void reload() {
        unload();
        load();
    }
    void unload() {
        templates.clear();
    }
    void load()
    {
        vector <string> files;

        cerr << "Attempting to load professions: " << professions_folder.c_str() << endl;
        if (!Filesystem::isdir(professions_folder) && !Filesystem::mkdir(professions_folder))
        {
            cerr << professions_folder << ": Does not exist and cannot be created" << endl;
            return;
        }
        Filesystem::listdir(professions_folder, files);
        std::sort(files.begin(), files.end());
        for(size_t i = 0; i < files.size(); i++)
        {
            if (files[i] == "." || files[i] == "..")
                continue;

            ProfessionTemplate t;
            if (t.load(professions_folder, files[i]))
            {
                templates.push_back(t);
            }
        }
    }
    void save_from_unit(UnitInfo *unit)
    {
        ProfessionTemplate t = {
            unit_ops::get_profname(unit)
        };

        t.fromUnit(unit);
        t.save(professions_folder);
        reload();
    }
};
static ProfessionTemplateManager manager;

class viewscreen_unitbatchopst : public dfhack_viewscreen {
public:
    enum page { MENU, NICKNAME, PROFNAME };
    viewscreen_unitbatchopst(vector<UnitInfo*> &base_units,
                             bool filter_selected = true,
                             bool *dirty_flag = NULL
                             )
        :cur_page(MENU), entry(""), selection_empty(false), dirty(dirty_flag)
    {
        menu_options.multiselect = false;
        menu_options.auto_select = true;
        menu_options.allow_search = false;
        menu_options.left_margin = 2;
        menu_options.bottom_margin = 2;
        menu_options.clear();
        menu_options.add("Change nickname", page::NICKNAME);
        menu_options.add("Change profession name", page::PROFNAME);
        menu_options.filterDisplay();
        formatter.add_option("n", "Displayed name (or nickname)", unit_ops::get_nickname);
        formatter.add_option("N", "Real name", unit_ops::get_real_name);
        formatter.add_option("en", "Displayed name (or nickname), in English", unit_ops::get_nickname_eng);
        formatter.add_option("eN", "Real name, in English", unit_ops::get_real_name_eng);
        formatter.add_option("fn", "Displayed first name (or nickname)", unit_ops::get_first_nickname);
        formatter.add_option("fN", "Real first name", unit_ops::get_first_name);
        formatter.add_option("ln", "Last name", unit_ops::get_last_name);
        formatter.add_option("eln", "Last name, in English", unit_ops::get_last_name_eng);
        formatter.add_option("p", "Displayed profession", unit_ops::get_profname);
        formatter.add_option("P", "Real profession (non-customized)", unit_ops::get_real_profname);
        formatter.add_option("bp", "Base profession (excluding nobles & other positions)", unit_ops::get_base_profname);
        formatter.add_option("sp", "Short (base) profession name (from manipulator headers)", unit_ops::get_short_profname);
        formatter.add_option("a", "Age (in years)", unit_ops::get_age);
        formatter.add_option("ag", "Arrival Group", unit_ops::get_arrival);
        formatter.add_option("i", "Position in list", unit_ops::get_list_id);
        formatter.add_option("pi", "Position in list, among dwarves with same profession", unit_ops::get_list_id_prof);
        formatter.add_option("gi", "Position in list, among dwarves in same profession group", unit_ops::get_list_id_group);
        formatter.add_option("ri", "Raw unit ID", unit_ops::get_unit_id);
        //formatter.add_option("ax", "Unit Aindex", unit_ops::get_unit_ax);
        selection_empty = true;
        for (auto it = base_units.begin(); it != base_units.end(); ++it)
        {
            UnitInfo* uinfo = *it;
            if (uinfo->selected || !filter_selected)
            {
                selection_empty = false;
                units.push_back(uinfo);
            }
        }
    }
    std::string getFocusString() { return "unitlabors/batch"; }
    void select_page (page p)
    {
        if (p == NICKNAME || p == PROFNAME)
            entry = "";
        cur_page = p;
    }
    void apply(void (*func)(UnitInfo*, string), string arg, StringFormatter<UnitInfo*> *arg_formatter)
    {
        if (dirty)
            *dirty = true;
        for (auto it = units.begin(); it != units.end(); ++it)
        {
            UnitInfo* u = (*it);
            if (!u || !u->unit || !u->allowEdit) continue;
            string cur_arg = arg_formatter->format(u, arg);
            func(u, cur_arg);
        }
    }
    void feed(set<df::interface_key> *events)
    {
        if (cur_page == MENU)
        {
            if (events->count(interface_key::LEAVESCREEN))
            {
                Screen::dismiss(this);
                return;
            }
            if (selection_empty)
                return;
            if (menu_options.feed(events))
            {
                // Allow left mouse button to trigger menu options
                if (menu_options.feed_mouse_set_highlight)
                    events->insert(interface_key::SELECT);
                else
                    return;
            }
            if (events->count(interface_key::SELECT))
                select_page(menu_options.getFirstSelectedElem());
        }
        else if (cur_page == NICKNAME || cur_page == PROFNAME)
        {
            if (events->count(interface_key::LEAVESCREEN))
                select_page(MENU);
            else if (events->count(interface_key::SELECT))
            {
                apply((cur_page == NICKNAME) ? unit_ops::set_nickname : unit_ops::set_profname, entry, &formatter);
                select_page(MENU);
            }
            else
            {
                for (auto it = events->begin(); it != events->end(); ++it)
                {
                    int ch = Screen::keyToChar(*it);
                    if (ch == 0 && entry.size())
                        entry.resize(entry.size() - 1);
                    else if (ch > 0)
                        entry.push_back(char(ch));
                }
            }
        }
    }
    void render()
    {
        dfhack_viewscreen::render();
        Screen::clear();
        int x = 2, y = 2;
        if (cur_page == MENU)
        {
            Screen::drawBorder("  Dwarf Manipulator - Batch Operations  ");
            if (selection_empty)
            {
                OutputString(COLOR_LIGHTRED, x, y, "No dwarves selected!");
                return;
            }
            menu_options.display(true);
        }
        OutputString(COLOR_LIGHTGREEN, x, y, itos(units.size()));
        OutputString(COLOR_GREY, x, y, string(" ") + (units.size() > 1 ? "dwarves" : "dwarf") + " selected: ");
        int max_x = gps->dimx - 2;
        size_t i = 0;
        for ( ; i < units.size(); i++)
        {
            string name = unit_ops::get_nickname(units[i]);
            if (name.size() + x + 12 >= max_x)   // 12 = "and xxx more"
                break;
            OutputString(COLOR_WHITE, x, y, name + ", ");
        }
        if (i == units.size())
        {
            x -= 2;
            OutputString(COLOR_WHITE, x, y, "  ");
        }
        else
        {
            OutputString(COLOR_GREY, x, y, "and " + itos(units.size() - i) + " more");
        }
        x = 2; y += 2;
        if (cur_page == NICKNAME || cur_page == PROFNAME)
        {
            std::string name_type = (cur_page == page::NICKNAME) ? "Nickname" : "Profession name";
            OutputString(COLOR_GREY, x, y, "Custom " + name_type + ":");
            x = 2; y += 1;
            OutputString(COLOR_WHITE, x, y, entry);
            OutputString(COLOR_LIGHTGREEN, x, y, "_");
            x = 2; y += 2;
            OutputString(COLOR_DARKGREY, x, y, "(Leave blank to use original name)");
            x = 2; y += 2;
            OutputString(COLOR_WHITE, x, y, "Format options:");
            StringFormatter<UnitInfo*>::T_optlist *format_options = formatter.get_options();
            for (auto it = format_options->begin(); it != format_options->end(); ++it)
            {
                x = 2; y++;
                auto opt = *it;
                OutputString(COLOR_LIGHTCYAN, x, y, "%" + string(std::get<0>(opt)));
                OutputString(COLOR_WHITE, x, y, ": " + string(std::get<1>(opt)));
            }
        }
    }
protected:
    ListColumn<page> menu_options;
    page cur_page;
    string entry;
    vector<UnitInfo*> units;
    StringFormatter<UnitInfo*> formatter;
    bool selection_empty;
    bool *dirty;
private:
    void resize(int32_t x, int32_t y)
    {
        dfhack_viewscreen::resize(x, y);
        menu_options.resize();
    }
};
class viewscreen_unitprofessionset : public dfhack_viewscreen {
public:
    viewscreen_unitprofessionset(vector<UnitInfo*> &base_units,
                             bool filter_selected = true
                             )
        :menu_options(-1) // default
    {
        menu_options.multiselect = false;
        menu_options.auto_select = true;
        menu_options.allow_search = false;
        menu_options.left_margin = 2;
        menu_options.bottom_margin = 2;
        menu_options.clear();

        manager.reload();
        for (size_t i = 0; i < manager.templates.size(); i++) {
            std::string name = manager.templates[i].name;
            if (manager.templates[i].mask)
                name += " (mask)";
            ListEntry<size_t> elem(name, i);
            menu_options.add(elem);
        }
        menu_options.filterDisplay();

        selection_empty = true;
        for (auto it = base_units.begin(); it != base_units.end(); ++it)
        {
            UnitInfo* uinfo = *it;
            if (uinfo->selected || !filter_selected)
            {
                selection_empty = false;
                units.push_back(uinfo);
            }
        }
    }
    std::string getFocusString() { return "unitlabors/profession"; }
    void feed(set<df::interface_key> *events)
    {
        if (events->count(interface_key::LEAVESCREEN))
        {
            Screen::dismiss(this);
            return;
        }
        if (menu_options.feed(events))
        {
            // Allow left mouse button to trigger menu options
            if (menu_options.feed_mouse_set_highlight)
                events->insert(interface_key::SELECT);
            else
                return;
        }
        if (events->count(interface_key::SELECT))
        {
            if (menu_options.hasSelection())
            {
                select_profession(menu_options.getFirstSelectedElem());
            }
            Screen::dismiss(this);
            return;
        }
    }
    void select_profession(size_t selected)
    {
        if (manager.templates.empty() || selected >= manager.templates.size())
            return;
        ProfessionTemplate prof = manager.templates[selected];

        for (UnitInfo *u : units)
        {
            if (!u || !u->unit || !u->allowEdit) continue;
            prof.apply(u);
        }
    }
    void render()
    {
        dfhack_viewscreen::render();
        Screen::clear();
        int x = 2, y = 2;
        Screen::drawBorder("  Dwarf Manipulator - Custom Profession  ");
        if (!manager.templates.size())
        {
            OutputString(COLOR_LIGHTRED, x, y, "No saved professions");
            return;
        }
        if (selection_empty)
        {
            OutputString(COLOR_LIGHTRED, x, y, "No dwarves selected!");
            return;
        }
        menu_options.display(true);
        OutputString(COLOR_LIGHTGREEN, x, y, itos(units.size()));
        OutputString(COLOR_GREY, x, y, string(" ") + (units.size() > 1 ? "dwarves" : "dwarf") + " selected: ");
        int max_x = gps->dimx - 2;
        size_t i = 0;
        for ( ; i < units.size(); i++)
        {
            string name = unit_ops::get_nickname(units[i]);
            if (name.size() + x + 12 >= max_x)   // 12 = "and xxx more"
                break;
            OutputString(COLOR_WHITE, x, y, name + ", ");
        }
        if (i == units.size())
        {
            x -= 2;
            OutputString(COLOR_WHITE, x, y, "  ");
        }
        else
        {
            OutputString(COLOR_GREY, x, y, "and " + itos(units.size() - i) + " more");
        }
    }
protected:
    bool selection_empty;
    ListColumn<size_t> menu_options;
    vector<UnitInfo*> units;
private:
    void resize(int32_t x, int32_t y)
    {
        dfhack_viewscreen::resize(x, y);
        menu_options.resize();
    }
};

enum display_columns {
    COLUMN_STRESS,
    COLUMN_SELECTED,
    COLUMN_NAME,
    COLUMN_DETAIL,
    COLUMN_LABORS,
    COLUMN_MAX,
};

class viewscreen_unitlaborsst : public dfhack_viewscreen {
public:
    void feed(set<df::interface_key> *events);

    void logic() {
        dfhack_viewscreen::logic();
        if (do_refresh_names)
            refreshNames();
    }

    void render();
    void resize(int w, int h) { sizeDisplay(); }

    void help() { }

    std::string getFocusString() { return "unitlabors"; }

    df::unit *getSelectedUnit();

    viewscreen_unitlaborsst(vector<df::unit*> &src, int cursor_pos);
    ~viewscreen_unitlaborsst() { };

private:
    vector<UnitInfo *> units;

    bool do_refresh_names;
    int row_hint;
    int col_hint;
    int display_rows;
    int last_selection;

    std::chrono::system_clock::time_point knock_ts;
    std::chrono::system_clock::time_point chronow;

    int column_size[COLUMN_MAX];
    int column_anchor[COLUMN_MAX];

    void refreshNames();
    void dualSort();
    void resetModes();

    void calcIDs();
    void calcArrivals();
    void calcUnitinfoDemands();

    void sizeDisplay();
    void checkScroll();

    bool scrollknock(int * reg, int stickval, int passval);
    bool scroll_knock=false;

    void paintAttributeRow(int &row ,UnitInfo *cur, df::unit* unit, bool header);
    void paintLaborRow(int &row,UnitInfo *cur, df::unit* unit);

    df::unit* findCPsActiveUnit(int cursor_pos);
    int findUnitsListPos(int unit_row);
};

df::unit* viewscreen_unitlaborsst::findCPsActiveUnit(int cursor_pos){

    df::unit *unit;

    if (VIRTUAL_CAST_VAR(unitlist, df::viewscreen_unitlistst, parent)){
        unit = unitlist->units[unitlist->page][cursor_pos];
    }

    return unit;
}

int viewscreen_unitlaborsst::findUnitsListPos(int unit_row){

    int ULcurpos = -1;
    if (VIRTUAL_CAST_VAR(unitlist, df::viewscreen_unitlistst, parent))
    for (int i = 0; i < unitlist->units[unitlist->page].size(); i++)
    {
        if (unitlist->units[unitlist->page][i] == units[unit_row]->unit){
            ULcurpos=i;
            break;
        }
    }

    return ULcurpos;
}

viewscreen_unitlaborsst::viewscreen_unitlaborsst(vector<df::unit*> &src, int cursor_pos)
{
    for (size_t i = 0; i < src.size(); i++)
    {
        df::unit *unit = src[i];

        if (!(unit&&unit->status.current_soul))
        {   //!cant currently employ 'cursor_pos', seems brk in master
            //~ if (cursor_pos > i)
                //~ cursor_pos--;
            continue;
        }

        UnitInfo *cur = new UnitInfo;

        cur->ids.init();
        cur->unit = unit;
        cur->age = (int)Units::getAge(unit);
        cur->allowEdit = true;
        cur->selected = false;

        if (!Units::isOwnCiv(unit))
            cur->allowEdit = false;

        if (!Units::isOwnGroup(unit))
            cur->allowEdit = false;

        if (unit->flags1.bits.dead)
            cur->allowEdit = false;

        if (unit->flags2.bits.visitor)
            cur->allowEdit = false;

        if (unit->flags3.bits.ghostly)
            cur->allowEdit = false;

        if (!ENUM_ATTR(profession, can_assign_labor, unit->profession))
            cur->allowEdit = false;

        cur->color = Units::getProfessionColor(unit);

        units.push_back(cur);
    }

    if(cur_world!=World::ReadWorldFolder()){
        cur_world = World::ReadWorldFolder();
        resetModes();
    }

    row_hint=0;
    col_hint=0;
    refreshNames();
    calcArrivals();

    /// How to pass selected unit back and fro the df unit listing ?
    /// When we go to view crea from here, df gets the selected unit
    /// but when we just leave here, it does not ....
    /*
    df::unit *cpunit = findCPsActiveUnit(cursor_pos);

    if(cpunit){
        int sel_acx = active_index[cpunit];
        for (size_t i = 0; i < units.size(); i++)
        {
            if(sel_acx==units[i]->active_index){
                sel_row=i;
                break;
            }
        }
    }
    */

    if(sel_row>=units.size())
        sel_row=units.size()-1;
    int sel_row_a = sel_row;

    calcIDs();
    unit_info_ops::calcAptScores(units);
    unit_info_ops::calcNotices(units);

    unstashSelection(units);
    dualSort();

    sizeDisplay();

    while (first_row < sel_row - display_rows + 1)
        first_row += display_rows + 1;
    // make sure the selection stays visible
    if (first_row > sel_row)
        first_row = sel_row - display_rows + 1;
    // don't scroll beyond the end
    if (first_row > units.size() - display_rows)
        first_row = units.size() - display_rows;

    last_selection = -1;

    read_manipulator_config();
}

void viewscreen_unitlaborsst::calcArrivals()
{
    int bi=-100;
    int ci=0;
    int guessed_group=0;

    sorts_descend=false;
    std::stable_sort(units.begin(), units.end(), sortByUnitId);

    for (size_t i = 0; i < units.size(); i++)
    {
        ci = units[i]->unit->id;
        if(abs(ci-bi)>40)
            guessed_group++;
        units[i]->arrival_group = guessed_group;
        bi=ci;
    }
}

void viewscreen_unitlaborsst::resetModes()
{
    detail_mode=0;
    first_row=0;
    sel_row=0;
    sel_row_b=0;
    sel_unit=0;
    display_rows_b=0;
    first_column=0;
    sel_column=0;
    column_sort_column = -1;
    wide_sorts widesort_mode =  WIDESORT_SELECTED;
    fine_sorts finesort_mode =  FINESORT_NAME;
    widesort_descend = false;
    finesort_descend = false;
    sorts_descend = false;
    selection_changed = false;
    selection_stash.clear();
}

void viewscreen_unitlaborsst::calcIDs()
{
    static int list_prof_ids[NUM_LABORS];
    static int list_group_ids[NUM_LABORS];
    static map<df::profession, int> group_map;
    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;
        resetModes();
        for (int i = 0; i < NUM_LABORS; i++)
            group_map.insert(std::pair<df::profession, int>(columns[i].profession, columns[i].group));
    }

    memset(list_prof_ids, 0, sizeof(list_prof_ids));
    memset(list_group_ids, 0, sizeof(list_group_ids));
    for (size_t i = 0; i < units.size(); i++)
    {
        UnitInfo *cur = units[i];
        cur->ids.list_id = (int)i + 1;
        cur->ids.list_id_prof = ++list_prof_ids[cur->unit->profession];
        cur->ids.list_id_group = 0;
        auto it = group_map.find(cur->unit->profession);
        if (it != group_map.end())
            cur->ids.list_id_group = ++list_group_ids[it->second];
    }
}

void viewscreen_unitlaborsst::refreshNames()
{
    do_refresh_names = false;

    for (size_t i = 0; i < units.size(); i++)
    {
        UnitInfo *cur = units[i];
        df::unit *unit = cur->unit;

        cur->name = Translation::TranslateName(Units::getVisibleName(unit), false);
        cur->transname = Translation::TranslateName(Units::getVisibleName(unit), true);
        string nm = cur->name;

        cur->lastname = nm.substr(nm.find(" ")+1);

        cur->profession = Units::getProfessionName(unit);

        if (unit->job.current_job == NULL) {
            df::activity_event *event = Units::getMainSocialEvent(unit);
            if (event) {
                event->getName(unit->id, &cur->job_desc);
                cur->job_mode = UnitInfo::SOCIAL;
            }
            else {
                cur->job_desc = "Idle";
                cur->job_mode = UnitInfo::IDLE;
            }
        } else {
            cur->job_desc = DFHack::Job::getName(unit->job.current_job);
            cur->job_mode = UnitInfo::JOB;
        }
        if (unit->military.squad_id > -1) {
            cur->squad_effective_name = Units::getSquadName(unit);
            string detail_str = cur->squad_effective_name;
            if(detail_str.substr(0,4)=="The ")
                detail_str=detail_str.substr(4);

            cur->squad_info = stl_sprintf("%i", unit->military.squad_position + 1) + "." + detail_str;
        } else {
            cur->squad_effective_name = "";
            cur->squad_info = "";
        }

        if(cur->name == ""){             //to list animals with no name
            cur->name = cur->profession;
            cur->profession = "";
        }
    }
    sizeDisplay();
}

void viewscreen_unitlaborsst::calcUnitinfoDemands(){
    for (size_t i = 0; i < units.size(); i++)
    {
        UnitInfo *cur = units[i];
        cur->demand=0;

        for (size_t j = 0; j < NUM_LABORS; j++)
        {
            df::unit_labor cur_labor = columns[j].labor;
            df::job_skill cur_skill = columns[j].skill;

            int dm=0;
            if(cur->unit->status.labors[cur_labor]){
                if(cur_skill==df::job_skill::NONE){
                    if(cur_labor==unit_labor::HAUL_STONE
                     ||cur_labor==unit_labor::HAUL_WOOD
                     ||cur_labor==unit_labor::HAUL_ITEM
                     ||cur_labor==unit_labor::HAUL_STONE
                     ||cur_labor==unit_labor::BUILD_CONSTRUCTION){
                        dm=12;
                    }else{
                        dm=4;
                    }
                }else{
                    dm = unitSkillRating(cur,cur_skill)*2+1;
                    if(unitSkillExperience(cur,cur_skill) > 0){
                        dm += 2;
                    }
                    dm = static_cast<int>(sqrt((dm)*(350+cur->column_aptitudes[j]))/8);
                }
            }
            cur->demand+=dm;
        }
    }
}

void viewscreen_unitlaborsst::dualSort()
{
    if(cancel_sort){
        cancel_sort=false;
        return;
    }

    int sel_unitid = units[sel_row]->unit->id;

    sorts_descend=finesort_descend; //this is inactive for simplicity
    switch (finesort_mode) {
    case FINESORT_COLUMN:
        if(column_sort_column==-1)
            column_sort_column=sel_column;

        column_sort_last = column_sort_column;
        std::stable_sort(units.begin(), units.end(), sortByColumn);

        col_hint=0;
        break;
    case FINESORT_NOTICES:
        std::stable_sort(units.begin(), units.end(), sortByNotices);
        break;
    case FINESORT_STRESS:
        std::stable_sort(units.begin(), units.end(), sortByStress);
        break;
    case FINESORT_AGE:
        std::stable_sort(units.begin(), units.end(), sortByAge);
        break;
    case FINESORT_NAME:
        std::stable_sort(units.begin(), units.end(), sortByName);
        break;
    //case FINESORT_SURNAME:
    //    std::stable_sort(units.begin(), units.end(), sortBySurName);
    //    break;
    case FINESORT_MARTIAL:
        std::stable_sort(units.begin(), units.end(), sortByMartial);
        break;
    case FINESORT_PERFORMER:
        std::stable_sort(units.begin(), units.end(), sortByPerformer);
        break;
    case FINESORT_SCHOLAR:
        std::stable_sort(units.begin(), units.end(), sortByScholar);
        break;
    case FINESORT_TOPSKILLED:
        std::stable_sort(units.begin(), units.end(), sortByTopskilled);
        break;
    case FINESORT_DEMAND:
        calcUnitinfoDemands();
        std::stable_sort(units.begin(), units.end(), sortByDemand);
        break;
    case FINESORT_OVER:
        break;
    }

    sorts_descend=widesort_descend;
    switch (widesort_mode) {
    case WIDESORT_NONE:
        break;
    case WIDESORT_SELECTED:
        std::stable_sort(units.begin(), units.end(), sortBySelected);
        break;
    case WIDESORT_PROFESSION:
        std::stable_sort(units.begin(), units.end(), sortByProfession);
        break;
    case WIDESORT_SQUAD:
        std::stable_sort(units.begin(), units.end(), sortBySquad);
        break;
    case WIDESORT_JOB:
        std::stable_sort(units.begin(), units.end(), sortByJob);
        break;
    case WIDESORT_ARRIVAL:
        std::stable_sort(units.begin(), units.end(), sortByChild);
        std::stable_sort(units.begin(), units.end(), sortByArrival);
        break;
    case WIDESORT_OVER:
        break;
    }

    if(finesort_mode != FINESORT_AGE
    && finesort_mode != FINESORT_NOTICES
    && widesort_mode != WIDESORT_ARRIVAL){
      std::stable_sort(units.begin(), units.end(), sortByChild);
    }

    for (size_t i = 0; i < units.size(); i++){
        if(sel_unitid == units[i]->unit->id)
            sel_row_b=i;
    }

    if(sel_row_b != sel_row){
        sel_row=sel_row_b;
        first_row=0;
        row_hint=0;
    }

    sel_row_b=sel_row;

    //for toggling row colors indicating sort groups
    bool toggle_sort_bg = false;
    bool bool_runner = false;
    int int_runner = -234492; //just a unlikely number
    string string_runner;

    for (int i = 0; i < units.size(); i++){
        UnitInfo *cur = units[i];

        switch (widesort_mode) {
        case WIDESORT_SELECTED:
            if(cur->selected != bool_runner){
                bool_runner=cur->selected;
                toggle_sort_bg=!toggle_sort_bg;
            }
            break;
        case WIDESORT_PROFESSION:
            if(cur->profession != string_runner){
                string_runner = cur->profession;
                toggle_sort_bg = !toggle_sort_bg;
            }
            break;
        case WIDESORT_SQUAD:
            if(cur->squad_effective_name != string_runner){
                string_runner = cur->squad_effective_name;
                toggle_sort_bg = !toggle_sort_bg;
            }
            break;
        case WIDESORT_JOB:
            if(cur->job_desc != string_runner){
                string_runner = cur->job_desc;
                toggle_sort_bg = !toggle_sort_bg;
            }
            break;
        case WIDESORT_ARRIVAL:
            if(cur->arrival_group != int_runner){
                int_runner = cur->arrival_group;
                toggle_sort_bg = !toggle_sort_bg;
            }
            break;
        case WIDESORT_OVER:
            break;
        }

        units[i]->sort_grouping_hint = toggle_sort_bg;
    }

    calcIDs();

}

void viewscreen_unitlaborsst::sizeDisplay()
{
    auto dim = Screen::getWindowSize();

    display_rows = dim.y - 11;
    if (display_rows > units.size())
        display_rows = units.size();

    int cn_stress    = 6;
    int cn_selected  = 1;
    int cn_name      = 16;
    int cn_detail    = 19;
    int cn_labor     = 25;
    int cn_border    =  2;
    int cn_dividers  =  4;
    int cn_total     = cn_stress+cn_selected+cn_name+cn_detail+cn_labor+cn_border+cn_dividers;

    int maxname   = cn_name;
    int maxdetail = cn_detail;

    // get max_name/max_prof from strings length
    for (size_t i = 0; i < units.size(); i++)
    {
        if (maxname < units[i]->name.size())
            maxname = units[i]->name.size();

        if(detail_mode==DETAIL_MODE_JOB||detail_mode==DETAIL_MODE_SQUAD||detail_mode==DETAIL_MODE_NOTICE){
            size_t detail_cmp;
            if(detail_mode==DETAIL_MODE_JOB){
                detail_cmp = units[i]->job_desc.size();
            }else if(detail_mode==DETAIL_MODE_SQUAD){
                detail_cmp = units[i]->squad_info.size();
            }else if(detail_mode==DETAIL_MODE_NOTICE){
                detail_cmp = units[i]->notices.size();
            }

            if (maxdetail < detail_cmp)
                maxdetail = detail_cmp;
        }
    }

    int maxpart=dim.x/5+6;
    int left = dim.x-cn_total;
    if( left>0){
        maxname=maxname>maxpart?maxpart:maxname;
        int increase = maxname-cn_name;
        if(increase>left){
            cn_name+=left;
        }else{
            cn_name+=increase;
            left-=increase;
            maxdetail=maxdetail>maxpart?maxpart:maxdetail;
            increase = maxdetail-cn_detail;
            if(increase>left){
                cn_detail+=left;
            }else{
                cn_detail+=increase;
            }
        }
    }

    int mk = 1; //border
    column_anchor[COLUMN_STRESS]  = mk;
    column_size[COLUMN_STRESS]    = cn_stress;
    mk += cn_stress+1;
    column_anchor[COLUMN_SELECTED]= mk;
    column_size[COLUMN_SELECTED]  = cn_selected;
    mk += cn_selected+1;
    column_anchor[COLUMN_NAME]    = mk;
    column_size[COLUMN_NAME]      = cn_name;
    mk += cn_name+1;
    column_anchor[COLUMN_DETAIL]  = mk;
    column_size[COLUMN_DETAIL]    = cn_detail;
    mk += cn_detail+1;
    column_anchor[COLUMN_LABORS]  = mk;
    column_size[COLUMN_LABORS]    = dim.x - mk - 1;

    // don't adjust scroll position immediately after the window opened
    if (units.size() == 0)
        return;

    checkScroll();
}

void viewscreen_unitlaborsst::checkScroll(){

    int bzone=display_rows/7;
    int bottom_hint=(display_rows<=units.size()&&display_rows>16)?1:0;

    if (first_row > units.size()-display_rows+bottom_hint ){
        first_row = units.size()-display_rows+bottom_hint;
    }

    if (first_row < 0)
        first_row = 0;

    if(sel_row!=sel_row_b||display_rows!=display_rows_b)
    {
        if(first_row==0&&(sel_row_b<first_row||sel_row_b>first_row + display_rows)){
            if(sel_row_b<sel_row){ //issued down
                sel_row=sel_row_b; //just make sel row visible
                first_row = sel_row +bzone+1 - display_rows;
                row_hint=0;
            }

            if(sel_row_b>sel_row){ //issued up
                sel_row=0;
                sel_row_b=sel_row; //go to
                row_hint=0;
            }
        }

        //sel_row is pushing down
        if( sel_row+bzone+1 > first_row + display_rows ){ //beyond zone
            if(sel_row>sel_row_b)
                first_row++;
            if(sel_row+1+1 > first_row + display_rows){ //beyond max
                first_row = sel_row + 1+1 - display_rows;
                row_hint=0;
            }
        }

        //sel_row is pushing up
        if( sel_row-bzone < first_row ){
            if(sel_row<sel_row_b)
            first_row--;
            if( sel_row-1 < first_row ){
                first_row=sel_row-1;
                row_hint=0;
            }
        }

        if (first_row < 0){
            first_row = 0;
        }

        sel_row_b=sel_row; display_rows_b=display_rows;
    }

    if (first_row > units.size()-display_rows+bottom_hint ){
        first_row = units.size()-display_rows+bottom_hint;
    }

    if (first_row < 0)
        first_row = 0;

    int row_width=column_size[COLUMN_LABORS];
    bzone=row_width/8+1;

    //set first column according to if sel is pushing zone
    if( sel_column-bzone < first_column )
        first_column=sel_column-bzone;

    if( sel_column+bzone >= first_column + row_width )
        first_column=sel_column-row_width+bzone+1;

   if(first_column<0)
       first_column=0;

    if(first_column>=NUM_LABORS-row_width)
        first_column=NUM_LABORS-row_width;

}

bool viewscreen_unitlaborsst::scrollknock(int *reg, int stickval, int passval){
    if(!(scroll_knock)){
        scroll_knock=true;
        knock_ts = std::chrono::system_clock::now();
        *reg = stickval;
        return false;
    }else{
        chronow = std::chrono::system_clock::now();
        if ((std::chrono::duration_cast<std::chrono::milliseconds>(chronow-knock_ts)).count()>200){
            *reg = passval;
            scroll_knock=false;
            return true;
        }else{
            knock_ts=chronow;
            *reg = stickval;
            return false;
        }
    }
}

void viewscreen_unitlaborsst::feed(set<df::interface_key> *events)
{
    int8_t modstate = Core::getInstance().getModstate();
    bool leave_all = events->count(interface_key::LEAVESCREEN_ALL);
    if (leave_all || events->count(interface_key::LEAVESCREEN))
    {
        events->clear();
        Screen::dismiss(this);
        if (leave_all)
        {
            //set unitlist cursor pos
            if (VIRTUAL_CAST_VAR(unitlist, df::viewscreen_unitlistst, parent))
            {
                int ULcurpos = findUnitsListPos(sel_row);

                if(ULcurpos!=-1){
                    unitlist->cursor_pos[unitlist->page] = ULcurpos;
                }
            }

            events->insert(interface_key::LEAVESCREEN);
            parent->feed(events);
            events->clear();
        }
        return;
    }

    if (!units.size())
        return;

    if (do_refresh_names)
        refreshNames();

    //mouse stuff before scrollers
    int mouse_row = -1;
    int mouse_column = -1;
    int gpsx=-1;
    int gpsy=-1;

    if (enabler->tracking_on && gps->mouse_x != -1 && gps->mouse_y != -1)
    {
        gpsx=gps->mouse_x;
        gpsy=gps->mouse_y;
        int click_header = COLUMN_MAX; // group ID of the column header clicked
        int click_body = COLUMN_MAX; // group ID of the column body clicked

        int click_unit = -1; // Index into units[] (-1 if out of range)
        int click_labor = -1; // Index into columns[] (-1 if out of range)

        const int coltweaka[] = {0,-1,0,0,0,0,0}; //make selection col
        const int coltweakb[] = {0, 1,0,0,0,0,0}; //include its border

        if (gpsy == Screen::getWindowSize().y-3){
            if( gpsx>39 && gpsx<63 ){ //clicks in sort mode options in footer
                if(enabler->mouse_lbut){
                    events->insert(interface_key::SECONDSCROLL_PAGEUP);
                }else if(enabler->mouse_rbut){
                    events->insert(interface_key::SECONDSCROLL_PAGEDOWN);
                }
            }else if( gpsx>=63 && gpsx<79 ){
                if(enabler->mouse_lbut){
                    events->insert(interface_key::SECONDSCROLL_UP);
                }else if(enabler->mouse_rbut){
                    events->insert(interface_key::SECONDSCROLL_DOWN);
                }
            }
        }

        for (int i = 0; i < COLUMN_MAX; i++)
        {
            if ((gpsx >= column_anchor[i] + coltweaka[i]) &&
                (gpsx < column_anchor[i] + column_size[i] + coltweakb[i]))
            {
                if ((gpsy >= 1) && (gpsy <= 3))
                    click_header = i;
                if ((gpsy >= 4) && (gpsy < 4 + display_rows))
                    click_body = i;
            }
        }

        if ((gpsx >= column_anchor[COLUMN_LABORS]) &&
            (gpsx < column_anchor[COLUMN_LABORS] + column_size[COLUMN_LABORS]))
            click_labor = gpsx - column_anchor[COLUMN_LABORS] + first_column;
        if ((gpsy >= 4) && (gpsy < 4 + display_rows))
            click_unit = gpsy - 4 + first_row;

        switch (click_header)
        {
        case COLUMN_STRESS:
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                if (enabler->mouse_rbut)
                    events->insert(interface_key::SECONDSCROLL_PAGEUP);
                if (enabler->mouse_lbut)
                    events->insert(interface_key::SECONDSCROLL_PAGEDOWN);

                enabler->mouse_lbut = enabler->mouse_rbut =0;
            }
            break;

        case COLUMN_SELECTED:
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                widesort_mode = WIDESORT_SELECTED;
                selection_changed=true;

                if (enabler->mouse_lbut)
                    events->insert(interface_key::SECONDSCROLL_PAGEUP);
                if (enabler->mouse_rbut)
                    events->insert(interface_key::SECONDSCROLL_PAGEDOWN);

                enabler->mouse_lbut = enabler->mouse_rbut =0;
            }
            break;

        case COLUMN_NAME:
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                if (enabler->mouse_lbut)
                    events->insert(interface_key::SECONDSCROLL_UP);
                if (enabler->mouse_rbut)
                    events->insert(interface_key::SECONDSCROLL_DOWN);

                enabler->mouse_lbut = enabler->mouse_rbut =0;
            }
            break;

        case COLUMN_DETAIL:
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                if(enabler->mouse_lbut)
                    detail_mode--;
                else
                    detail_mode++;

                if(detail_mode==DETAIL_MODE_MAX)
                    detail_mode=0;
                if(detail_mode<0)
                    detail_mode=DETAIL_MODE_MAX-1;

                //tab detail mode
                enabler->mouse_lbut = enabler->mouse_rbut =0;
            }
            break;

        case COLUMN_LABORS: //finesort column
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                if(enabler->mouse_rbut){
                    finesort_mode = FINESORT_COLUMN;
                    mouse_column = click_labor;
                    column_sort_column = -1;

                    events->insert(interface_key::SECONDSCROLL_UP);
                }else{

                    finesort_mode = FINESORT_COLUMN;
                    sel_column = click_labor;
                    column_sort_column = -1;
                    row_hint=25;
                    col_hint=25;

                    events->insert(interface_key::SECONDSCROLL_UP);
                }
                enabler->mouse_lbut = enabler->mouse_rbut =0;
            }
            break;
        }

        switch (click_body)
        {
        case COLUMN_STRESS:
            // do nothing
            break;

        case COLUMN_SELECTED:
            // left-click to select, right-click or shift-click to extend selection
            if (enabler->mouse_rbut || (enabler->mouse_lbut && (modstate & DFH_MOD_SHIFT)))
            {
                mouse_row = click_unit;
                events->insert(interface_key::CUSTOM_SHIFT_X); //deselect
            }
            else if (enabler->mouse_lbut)
            {
                mouse_row = click_unit;
                events->insert(interface_key::CUSTOM_X); //select
            }
            enabler->mouse_lbut = enabler->mouse_rbut = 0;
            break;

        case COLUMN_NAME:
        case COLUMN_DETAIL:
            // left-click to view, right-click to zoom
            if (enabler->mouse_lbut)
            {
                sel_row = click_unit;
                events->insert(interface_key::UNITJOB_VIEW_UNIT);
                row_hint=0;
            }
            if (enabler->mouse_rbut)
            {
                sel_row = click_unit;
                events->insert(interface_key::UNITJOB_ZOOM_CRE);
                row_hint=0;
            }
            enabler->mouse_lbut = enabler->mouse_rbut = 0;
            break;

        case COLUMN_LABORS:
            // left-click to toggle, right-click to just highlight
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                if (enabler->mouse_lbut)
                {
                    mouse_row = click_unit;
                    mouse_column = click_labor;
                    events->insert(interface_key::SELECT);
                }
                if (enabler->mouse_rbut)
                {
                    sel_row = click_unit;
                    sel_column = click_labor;
                }
                enabler->mouse_lbut = enabler->mouse_rbut = 0;
            }
            break;
        }
        enabler->mouse_lbut = enabler->mouse_rbut = 0;
    }

    //cursor scrolling
    if (events->count(interface_key::CURSOR_UP) || events->count(interface_key::CURSOR_UPLEFT) || events->count(interface_key::CURSOR_UPRIGHT))
        sel_row--;
    if (events->count(interface_key::CURSOR_UP_FAST) || events->count(interface_key::CURSOR_UPLEFT_FAST) || events->count(interface_key::CURSOR_UPRIGHT_FAST))
        sel_row -= 10;
    if (events->count(interface_key::CURSOR_DOWN) || events->count(interface_key::CURSOR_DOWNLEFT) || events->count(interface_key::CURSOR_DOWNRIGHT))
        sel_row++;
    if (events->count(interface_key::CURSOR_DOWN_FAST) || events->count(interface_key::CURSOR_DOWNLEFT_FAST) || events->count(interface_key::CURSOR_DOWNRIGHT_FAST))
        sel_row += 10;

    if (events->count(interface_key::CURSOR_DOWN_Z_AUX))
        sel_row = units.size()-1;

    if (events->count(interface_key::STRING_A000))
        sel_row = 0;

    if (events->count(interface_key::CURSOR_UP_Z_AUX))
        sel_row = 0;

    if (events->count(interface_key::CURSOR_LEFT) || events->count(interface_key::CURSOR_UPLEFT) || events->count(interface_key::CURSOR_DOWNLEFT))
        sel_column--;
    if (events->count(interface_key::CURSOR_LEFT_FAST) || events->count(interface_key::CURSOR_UPLEFT_FAST) || events->count(interface_key::CURSOR_DOWNLEFT_FAST))
        sel_column -= 10;
    if (events->count(interface_key::CURSOR_RIGHT) || events->count(interface_key::CURSOR_UPRIGHT) || events->count(interface_key::CURSOR_DOWNRIGHT))
        sel_column++;
    if (events->count(interface_key::CURSOR_RIGHT_FAST) || events->count(interface_key::CURSOR_UPRIGHT_FAST) || events->count(interface_key::CURSOR_DOWNRIGHT_FAST))
        sel_column += 10;

    if((!enabler->tracking_on )
      ||(enabler->tracking_on &&(gpsx >= column_anchor[COLUMN_LABORS]))){
        if ((sel_column > -1) && events->count(interface_key::CURSOR_UP_Z))
        {
            // go to beginning of current column group; if already at the beginning, go to the beginning of the previous one
            sel_column--;
            int cur = columns[sel_column].group;
            while ((sel_column > 0) && columns[sel_column - 1].group == cur)
                sel_column--;
        }

        if ((sel_column < NUM_LABORS - 1) && events->count(interface_key::CURSOR_DOWN_Z))
        {
            // go to beginning of next group
            int cur = columns[sel_column].group;
            int next = sel_column+1;
            while ((next < NUM_LABORS) && (columns[next].group == cur))
                next++;
            if ((next < NUM_LABORS) && (columns[next].group != cur))
                sel_column = next;
        }
    }
    else //these keys are set for mouse scroll wheel \:/
    {
        if (events->count(interface_key::CURSOR_DOWN_Z)){
            first_row-=4;
            if(first_row<0) first_row=0;
        }else if(events->count(interface_key::CURSOR_UP_Z)){
            first_row+=4;
        }
    }


    bool row_knocking = true;
    //deal wth column overshots and enable push to other side
    if (sel_row < 0){
        if (scrollknock(&sel_row, 0, (int)units.size()-1))
            row_hint=0;
    }else if (sel_row > units.size()-1){
        if (scrollknock(&sel_row, (int)units.size()-1 , 0))
            row_hint=0;
    }else
        row_knocking = false;

    if (sel_column < 0){
        if (scrollknock(&sel_column, 0, (int)NUM_LABORS - 1))
            col_hint=0;
    }else if (sel_column >= NUM_LABORS){
        if (scrollknock(&sel_column, (int)NUM_LABORS - 1 , 0))
            col_hint=0;
    }else{
        if(!row_knocking)
            scroll_knock=false;
    }

    if (events->count(interface_key::CURSOR_DOWN_Z) || events->count(interface_key::CURSOR_UP_Z))
    {
        // when moving by group, ensure the whole group is shown onscreen
        int endgroup_column = sel_column;
        while ((endgroup_column < NUM_LABORS-1) && columns[endgroup_column+1].group == columns[sel_column].group)
            endgroup_column++;

        if (first_column < endgroup_column - column_size[COLUMN_LABORS] + 1)
            first_column = endgroup_column - column_size[COLUMN_LABORS] + 1;
    }

    if (events->count(interface_key::STRING_A000))
        sel_column = 0;;

    checkScroll();

    //set labor
    int cur_row= (mouse_row!=-1)? mouse_row:sel_row;
    int cur_column= (mouse_column!=-1)? mouse_column:sel_column;

    UnitInfo *cur = units[cur_row];
    df::unit *unit = cur->unit;
    df::unit_labor cur_labor = columns[cur_column].labor;
    if (events->count(interface_key::SELECT) && (cur->allowEdit) && Units::isValidLabor(unit, cur_labor))
    {
        const SkillColumn &col = columns[cur_column];
        bool newstatus = !unit->status.labors[col.labor];
        if (col.special)
        {
            if (newstatus)
            {
                for (int i = 0; i < NUM_LABORS; i++)
                {
                    if ((columns[i].labor != unit_labor::NONE) && columns[i].special)
                        unit->status.labors[columns[i].labor] = false;
                }
            }
            unit->military.pickup_flags.bits.update = true;
        }
        unit->status.labors[col.labor] = newstatus;
    }
    if (events->count(interface_key::SELECT_ALL) && (cur->allowEdit) && Units::isValidLabor(unit, cur_labor))
    {
        selection_changed=true;
        const SkillColumn &col = columns[cur_column];
        bool newstatus = !unit->status.labors[col.labor];
        for (int i = 0; i < NUM_LABORS; i++)
        {
            if (columns[i].group != col.group)
                continue;
            if (!Units::isValidLabor(unit, columns[i].labor))
                continue;
            if (columns[i].special)
            {
                if (newstatus)
                {
                    for (int j = 0; j < NUM_LABORS; j++)
                    {
                        if ((columns[j].labor != unit_labor::NONE) && columns[j].special)
                            unit->status.labors[columns[j].labor] = false;
                    }
                }
                unit->military.pickup_flags.bits.update = true;
            }
            unit->status.labors[columns[i].labor] = newstatus;
        }
    }

    //change sort modes
    if (events->count(interface_key::SECONDSCROLL_DOWN)||events->count(interface_key::SECONDSCROLL_UP))
    {
        if(events->count(interface_key::SECONDSCROLL_DOWN)){
            //reselects column if column mode and differ from last
            if(finesort_mode==FINESORT_COLUMN && (cur_column != column_sort_last)){
                column_sort_column = column_sort_last=cur_column;
            }else { //move mode
                column_sort_column = column_sort_last=-1;
                finesort_mode=static_cast<fine_sorts>(static_cast<int>(finesort_mode)+1);
                if(finesort_mode==FINESORT_OVER) finesort_mode=static_cast<fine_sorts>(0);
                if(finesort_mode==FINESORT_COLUMN){
                    column_sort_column = column_sort_last;
                }
            }
        }else{
             if(finesort_mode==FINESORT_COLUMN && (cur_column != column_sort_last)){
                column_sort_column = column_sort_last=cur_column;
            }else { //move mode
                column_sort_column = column_sort_last = -1;
                finesort_mode=static_cast<fine_sorts>(static_cast<int>(finesort_mode)-1);
                if(finesort_mode==FINESORT_UNDER) finesort_mode=static_cast<fine_sorts>(FINESORT_OVER-1);
                if(finesort_mode==FINESORT_COLUMN){
                    column_sort_column = column_sort_last;
                }
            }
        }

        dualSort();
    }

    if (events->count(interface_key::SECONDSCROLL_PAGEDOWN)||events->count(interface_key::SECONDSCROLL_PAGEUP))
    {
        if(events->count(interface_key::SECONDSCROLL_PAGEDOWN)){
            if(!(widesort_mode==WIDESORT_SELECTED && selection_changed)){
                widesort_mode=static_cast<wide_sorts>(static_cast<int>(widesort_mode)+1);
                if(widesort_mode==WIDESORT_OVER) widesort_mode=static_cast<wide_sorts>(0);
            }else{
                selection_changed=false;
            }
        }else{
            if(!(widesort_mode==WIDESORT_SELECTED && selection_changed)){
                widesort_mode=static_cast<wide_sorts>(static_cast<int>(widesort_mode)-1);
                if(widesort_mode==WIDESORT_UNDER) widesort_mode=static_cast<wide_sorts>(static_cast<int>(WIDESORT_OVER)-1);

            }else{
                selection_changed=false;
            }
        }
        dualSort();
    }

    if (events->count(interface_key::OPTION20)) //toggle apt coloring
    {
        color_mode ++;
        if(color_mode==6)
            color_mode=0;
        save_manipulator_config();
    }

    //!fix toggle view mode, mod shift isnt working here
    if (events->count(interface_key::CHANGETAB)&&(modstate & DFH_MOD_SHIFT))
    {
        int duemode= detail_mode-1;
        if(duemode==-1) duemode+=DETAIL_MODE_MAX;
        detail_mode = duemode;
    }
    else if (events->count(interface_key::CHANGETAB))
    {
        int duemode= detail_mode+1;
        if(duemode==DETAIL_MODE_MAX) duemode=0;
        detail_mode = duemode;
    }

    //unit selection
    if (events->count(interface_key::CUSTOM_SHIFT_X))
    {
        selection_changed=true;
        if (last_selection == -1 || last_selection == cur_row)
            events->insert(interface_key::CUSTOM_X);
        else
        {
            for (int i = std::min(cur_row, last_selection);
                 i <= std::max(cur_row, last_selection);
                 i++)
            {
                if (i == last_selection) continue;
                if (!units[i]->allowEdit) continue;
                units[i]->selected = units[last_selection]->selected;
            }
            stashSelection(units);
        }
    }

    if (events->count(interface_key::CUSTOM_X) && cur->allowEdit)
    {
        selection_changed=true;
        cur->selected = !cur->selected;
        stashSelection(cur);
        last_selection = cur_row;
    }

    if (events->count(interface_key::CUSTOM_A) || events->count(interface_key::CUSTOM_SHIFT_A))
    {
        selection_changed=true;
        for (size_t i = 0; i < units.size(); i++){
            if (units[i]->allowEdit){
                units[i]->selected = (bool)events->count(interface_key::CUSTOM_A);
            }
        }
        stashSelection(units);
    }

    //nick prof editting
    if (events->count(interface_key::CUSTOM_B))
    {
        Screen::show(new viewscreen_unitbatchopst(units, true, &do_refresh_names), plugin_self);
    }

    if (events->count(interface_key::CUSTOM_E))
    {
        vector<UnitInfo*> tmp;
        tmp.push_back(cur);
        Screen::show(new viewscreen_unitbatchopst(tmp, false, &do_refresh_names), plugin_self);
    }

    if (events->count(interface_key::CUSTOM_P))
    {
        bool has_selected = false;
        for (size_t i = 0; i < units.size(); i++)
            if (units[i]->selected)
                has_selected = true;

        if (has_selected) {
            Screen::show(new viewscreen_unitprofessionset(units, true), plugin_self);
        } else {
            vector<UnitInfo*> tmp;
            tmp.push_back(cur);
            Screen::show(new viewscreen_unitprofessionset(tmp, false), plugin_self);
        }
    }

    if (events->count(interface_key::CUSTOM_SHIFT_P))
    {
        manager.save_from_unit(cur);
    }

    if (events->count(interface_key::UNITJOB_VIEW_UNIT) || events->count(interface_key::UNITJOB_ZOOM_CRE))
    {
        if (VIRTUAL_CAST_VAR(unitlist, df::viewscreen_unitlistst, parent))
        {
            int ULcurpos = findUnitsListPos(sel_row);

            if(ULcurpos!=-1){
                unitlist->cursor_pos[unitlist->page] = ULcurpos;
                unitlist->feed(events);
                if (Screen::isDismissed(unitlist))
                    Screen::dismiss(this);
                else
                    do_refresh_names = true;
            }
        }
    }
}

void viewscreen_unitlaborsst::paintAttributeRow(int &row ,UnitInfo *cur, df::unit* unit, bool header=false)
{
    using namespace unit_info_ops;

    int colm=0;
    int dwide = column_size[COLUMN_DETAIL];
    int pad =dwide<19?0:(dwide-19)/2;
    int uavg = (cur->work_aptitude+cur->skill_aptitude)/2;
    int aavg = (work_aptitude_avg+skill_aptitude_avg)/2;
    int vlow=(uavg*3/8+aavg/2-7);// (3/5ths)
    int vhigh=(uavg*5/8+aavg/2+16);

    for (int att = 0; att < 19; att++)
    {
        //skip attribs if too small
        if(dwide<19&&att==15) att++; //musi
        if(dwide<18&&att==13) att++; //ling
        if(dwide<17&&att==4) att++;  //recoup
        if(dwide<16&&att==5) att++;  //disease
        if(dwide<15&&att>dwide-1) continue; //crop rest

        int bg = COLOR_BLACK;
        int fg = COLOR_GREY;

        int val;
        if(att<6){
             val=unit->body.physical_attrs[att].value;
        }else{
             val=unit->status.current_soul->mental_attrs[att-6].value;
        }

        val= ((val+124)*10)/256; // 0 to 5000 > 0 to 200

        if(val<vlow){
            fg = COLOR_LIGHTRED;
        }else if(val>vhigh){
            fg = COLOR_LIGHTCYAN;
        }

        if (header)
        {
            df::job_skill skill;

            if(columns[sel_column].labor != unit_labor::NONE)
            {
                //display attribs excercised by labor:
                skill = labor_skill_map[columns[sel_column].labor];

                if((att<6 && skills_attribs[skill].phys_attr_weights[att]==1)
                  ||(att>5 && skills_attribs[skill].mental_attr_weights[att-6]==1))
                {   fg = COLOR_WHITE; bg = COLOR_DARKGREY; }
            }
            else
            {   skill = columns[sel_column].skill; }

            if(skill != job_skill::NONE)
            {
                //display attrib affecting skill
                if((att<6 && skills_attribs[skill].phys_attr_weights[att]==1)
                  ||(att>5 && skills_attribs[skill].mental_attr_weights[att-6]==1))
                {   fg = COLOR_WHITE; bg = COLOR_DARKGREY; }
                else if((att<6 && skills_attribs[skill].phys_attr_weights[att]==9)
                  ||(att>5 && skills_attribs[skill].mental_attr_weights[att-6]==9))
                {   fg = COLOR_LIGHTGREEN;  }

            }

            if(fg == COLOR_GREY) fg = COLOR_YELLOW;
            const char legenda[] = "SaterdAfwcipmlsmkes"; //attribute
            const char legendb[] = "tgoneinoirnaeipuimo";

            Screen::paintTile(Screen::Pen(legenda[colm], fg, bg), column_anchor[COLUMN_DETAIL] +colm+pad , 1 );
            Screen::paintTile(Screen::Pen(legendb[colm], fg, bg), column_anchor[COLUMN_DETAIL] +colm+pad , 2 );

        }

        if(row>=0&&row<display_rows){
            val/=10;

            Screen::paintTile(Screen::Pen(skill_levels[val].abbrev, fg, bg), column_anchor[COLUMN_DETAIL] +colm+pad , 4 + row);
        }

        colm++;
    }//attribs loop
}

void viewscreen_unitlaborsst::paintLaborRow(int &row,UnitInfo *cur, df::unit* unit)
{
    if(detail_mode==DETAIL_MODE_ATTRIBUTE)
        paintAttributeRow(row,cur,unit);

    const int8_t cltheme[]={
        /*noskill        hint 0           hint 1         hint 2 */
        //BG not set
        COLOR_BLACK,     COLOR_BLACK,     COLOR_BLACK,   COLOR_BLACK,
        //FG not set
        COLOR_DARKGREY,  COLOR_YELLOW,    COLOR_GREY,    COLOR_LIGHTCYAN,
        //BG not set and cursor
        COLOR_WHITE,     COLOR_WHITE,     COLOR_WHITE,   COLOR_WHITE,
        //FG not set and cursor
        COLOR_BLACK,     COLOR_BLACK,     COLOR_BLACK,   COLOR_BLACK,

        //BG set
        COLOR_DARKGREY,  COLOR_YELLOW,    COLOR_GREY,    COLOR_LIGHTCYAN,
        //FG set
        COLOR_GREY,      COLOR_WHITE,     COLOR_WHITE,   COLOR_WHITE,
        //BG set and cursor
        COLOR_WHITE,     COLOR_WHITE,     COLOR_WHITE,   COLOR_WHITE,
        //FG set and cursor
        COLOR_WHITE,     COLOR_WHITE,     COLOR_WHITE,   COLOR_WHITE,

        //32 BG mili      33 FG other
        COLOR_LIGHTMAGENTA,     COLOR_LIGHTMAGENTA
    };

    const int8_t bwtheme[]={
        /*noskill         hint 0      */
        //BG not set
        COLOR_BLACK,      COLOR_BLACK,
        //FG not set FG
        COLOR_DARKGREY,   COLOR_WHITE,
        //BG not set and cursor
        COLOR_WHITE,      COLOR_WHITE,
        //FG not set and cursor
        COLOR_BLACK,      COLOR_BLACK,

        //BG set
        COLOR_DARKGREY,   COLOR_WHITE,
        //FG set
        COLOR_WHITE,      COLOR_WHITE,
        //BG set and curso
        COLOR_WHITE,      COLOR_GREY,
        //FG set and curso
        COLOR_WHITE,      COLOR_WHITE,

        //16 BG mili       BG other
        COLOR_CYAN,       COLOR_LIGHTBLUE

    };

    //first_column can change by hoz scroll
    int col_end = first_column + column_size[COLUMN_LABORS];

    int8_t bg,fg;

    for (int role = first_column; role < col_end; role++)
    {
        bool is_work=(columns[role].labor != unit_labor::NONE);
        bool is_skilled=false;
        bool is_cursor=false;
        bool role_isset=false;

        uint8_t ch = '-';

        if (columns[role].skill == job_skill::NONE)
        {
            ch = 0xF9; // fat dot char for noskill jobs, it has opaque bg
        }
        else
        {
            is_skilled=true;

            df::unit_skill *skill = NULL;

            int level = unitSkillRating(cur,columns[role].skill);
            int exp = unitSkillExperience(cur,columns[role].skill);

            if (level || exp)
            {
                if (level > NUM_SKILL_LEVELS - 1)
                    level = NUM_SKILL_LEVELS - 1;
                ch = skill_levels[level].abbrev;
            }
        }

        if (columns[role].labor != unit_labor::NONE
         && unit->status.labors[columns[role].labor]){
            role_isset=true;
        }

        if ((role == sel_column) && (row+first_row == sel_row)){
            is_cursor=true;
        }

        int crow=0, hint=0;
        if(is_skilled) hint=1;
        if(role_isset) crow=2;
        if(is_cursor)  crow++;

        if( color_mode!=0 )
        {
            if(color_mode==1){
                if(is_skilled) hint+= cur->column_hints[role];
            }else
                hint=color_mode-2;

            bg=cltheme[crow*8+hint];
            fg=cltheme[crow*8+4+hint];

            if(is_cursor && !(is_work && cur->allowEdit)){
               bg=cltheme[32];
               fg=cltheme[33];
               //if(role>103) bg=cltheme[33]; //fighting etc
            }

            if( row_hint<60 && bg==COLOR_BLACK&& row+first_row == sel_row ){
                bg=COLOR_BLUE;
            }
            if( col_hint<60 && bg==COLOR_BLACK && role == sel_column){
                bg=COLOR_BLUE;
            }
        }
        else
        {
            bg=bwtheme[crow*4+hint];
            fg=bwtheme[crow*4+2+hint];

            if(!(is_work||is_cursor)){
               bg=bwtheme[16];
               if(role>103) bg=bwtheme[17]; //fighting etc
            }
        }

        //paints each element in labor grid
        Screen::paintTile(Screen::Pen(ch, fg, bg), column_anchor[COLUMN_LABORS] + role - first_column, 4 + row);

    }//columns
}

void viewscreen_unitlaborsst::render()
{
    if (Screen::isDismissed(this))
        return;

    dfhack_viewscreen::render();

    auto dim = Screen::getWindowSize();

    Screen::clear();
    Screen::drawBorder("  Dwarf Manipulator - Manage Labors  ");

    Screen::paintString(Screen::Pen(' ', 7, 0), column_anchor[COLUMN_STRESS], 2, "Stress");
    Screen::paintTile(Screen::Pen('\373', 7, 0), column_anchor[COLUMN_SELECTED], 2);
    Screen::paintString(Screen::Pen(' ', 7, 0), column_anchor[COLUMN_NAME], 2, "Name");

    string detail_str;
    int8_t cclr= COLOR_GREY;
    int8_t fg = COLOR_WHITE;
    int8_t bg = COLOR_BLACK;

    detail_str = "                  "; //clear the attribute legend
    Screen::paintString(Screen::Pen(' ', 7, 0), column_anchor[COLUMN_DETAIL], 1, detail_str);

    detail_str = detailmode_legend[detail_mode];

    Screen::paintString(Screen::Pen(' ', cclr, 0), column_anchor[COLUMN_DETAIL], 2, detail_str);

    for (int col = 0; col < column_size[COLUMN_LABORS]; col++)
    {
        int col_offset = col + first_column;
        if (col_offset >= NUM_LABORS)
            break;

        fg = columns[col_offset].color;
        bg = COLOR_BLACK;

        if (col_offset == sel_column){
            fg = COLOR_WHITE;
            bg = COLOR_BLUE;
        }

        if (col_offset == column_sort_column && finesort_mode == FINESORT_COLUMN)
        {
            if (col_offset == sel_column){
                fg = COLOR_WHITE;
                bg = COLOR_LIGHTGREEN;
            }else{
                fg = COLOR_WHITE;
                bg = COLOR_YELLOW;
            }
        }

        Screen::paintTile(Screen::Pen(columns[col_offset].label[0], fg, bg), column_anchor[COLUMN_LABORS] + col, 1);
        Screen::paintTile(Screen::Pen(columns[col_offset].label[1], fg, bg), column_anchor[COLUMN_LABORS] + col, 2);
        df::profession profession = columns[col_offset].profession;
        if (false&&(profession != profession::NONE) && (ui->race_id != -1))
        {
            auto graphics = world->raws.creatures.all[ui->race_id]->graphics;
            Screen::paintTile(
                Screen::Pen(' ', fg, 0,
                    graphics.profession_add_color[creature_graphics_role::DEFAULT][profession],
                    graphics.profession_texpos[creature_graphics_role::DEFAULT][profession]),
                column_anchor[COLUMN_LABORS] + col, 1);
        }
    }

    for (int row = 0; row < display_rows; row++)
    {
        int row_offset = row + first_row;
        if (row_offset >= units.size())
            break;

        UnitInfo *cur = units[row_offset];
        df::unit *unit = cur->unit;

        int stress_lvl = unit->status.current_soul ? unit->status.current_soul->personality.stress_level : 0;
        if (stress_lvl < -999999){ stress_lvl = -999999;  }
        if (stress_lvl > 9999999) stress_lvl = 9999999;
        //display the integer counter scaled down to comfortable values
        string stress = stl_sprintf("%6i", stress_lvl/100);

        //these colors were set *really* wonky
        fg = COLOR_LIGHTCYAN;        //royal
        if (stress_lvl <  200000)
            fg = COLOR_WHITE;        //good
         if (stress_lvl < 100000)
            fg = COLOR_YELLOW;       //lower progress
        if (stress_lvl <  10000)
            fg = COLOR_LIGHTGREEN;   //shows lightgreen for while at
        if (stress_lvl < -50000)     //start of play to say ok,
            fg = COLOR_GREEN;        //dark green for common mayhem

        Screen::paintString(Screen::Pen(' ', fg, bg), column_anchor[COLUMN_STRESS], 4 + row, stress);

        Screen::paintTile(
            (cur->selected) ? Screen::Pen('\373', COLOR_LIGHTGREEN, 0) :
                ((cur->allowEdit) ? Screen::Pen('-', COLOR_DARKGREY, 0) : Screen::Pen('-', COLOR_RED, 0)),
            column_anchor[COLUMN_SELECTED], 4 + row);

        fg = COLOR_WHITE;

        if(cur->sort_grouping_hint)
            fg = COLOR_LIGHTCYAN;
        else
            fg = COLOR_WHITE;

        if (row_offset == sel_row)
            bg = COLOR_BLUE;

        string name = cur->name;
        name.resize(column_size[COLUMN_NAME]);
        Screen::paintString(Screen::Pen(' ', fg, bg), column_anchor[COLUMN_NAME], 4 + row, name);

        bg = COLOR_BLACK;
        if (detail_mode == DETAIL_MODE_SQUAD) {
            fg = COLOR_LIGHTCYAN;
            detail_str = cur->squad_info;
        } else if (detail_mode == DETAIL_MODE_NOTICE) {
            fg = COLOR_LIGHTMAGENTA;
            detail_str = cur->notices;
        } else if (detail_mode == DETAIL_MODE_JOB) {
            detail_str = cur->job_desc;
            if (cur->job_mode == UnitInfo::IDLE) {
                fg = COLOR_YELLOW;
            } else if (cur->job_mode == UnitInfo::SOCIAL) {
                fg = COLOR_LIGHTGREEN;
            } else {
                fg = COLOR_LIGHTCYAN;
            }
        } else if (detail_mode == DETAIL_MODE_PROFESSION) {
            fg = cur->color;
            detail_str = cur->profession;
        } else if (detail_mode == DETAIL_MODE_ATTRIBUTE) {
            detail_str = "";
        }

        if(detail_mode != DETAIL_MODE_ATTRIBUTE){
              detail_str.resize(column_size[COLUMN_DETAIL]);
              Screen::paintString(Screen::Pen(' ', fg, bg), column_anchor[COLUMN_DETAIL], 4 + row, detail_str);
        }

        paintLaborRow(row,cur,unit);
    }

    row_hint++;
    col_hint++;

    UnitInfo *cur = units[sel_row];

    bool canToggle = false;
    if (cur != NULL)
    {
        df::unit *unit = cur->unit;

        int c3po=sel_row - first_row; //little kludge
        if(detail_mode==DETAIL_MODE_ATTRIBUTE)
            paintAttributeRow(c3po,cur,unit,true); //paintheader


        int x = 1, y = 3 + display_rows + 2;
        Screen::Pen white_pen(' ', 15, 0);

        Screen::paintString(white_pen, x, y, (cur->unit && cur->unit->sex) ? "\x0b" : "\x0c");
        x += 2;
        Screen::paintString(white_pen, x, y, cur->transname);
        x += cur->transname.length();

        if (cur->transname.length())
        {
            Screen::paintString(white_pen, x, y, ", ");
            x += 2;
        }
        Screen::paintString(white_pen, x, y, cur->profession);
        x += cur->profession.length();
        Screen::paintString(white_pen, x, y, ": ");
        x += 2;

        string str;
        if (columns[sel_column].skill == job_skill::NONE)
        {
            str = ENUM_ATTR_STR(unit_labor, caption, columns[sel_column].labor);
            if (unit->status.labors[columns[sel_column].labor])
                str = "Availed to "+str;
            else
                str = "Forbade to "+str;
        }
        else
        {
            string descq ="";
            string statq ="";
            int apti=(color_mode!=0)?cur->column_aptitudes[sel_column]:0;

            int level = unitSkillRating(cur, columns[sel_column].skill);
            int exp = unitSkillExperience(cur, columns[sel_column].skill);

            if (level || exp)
            {
                if (level > NUM_SKILL_LEVELS - 1)
                    level = NUM_SKILL_LEVELS - 1;

                descq = stl_sprintf("%s %s (%c", skill_levels[level].name, ENUM_ATTR_STR(job_skill, caption_noun, columns[sel_column].skill), skill_levels[level].abbrev);

                if (level != NUM_SKILL_LEVELS - 1)
                    statq = stl_sprintf(" %d/%dxp)", ((exp+5)/10), ((skill_levels[level].points+5)/10));

                if(apti>0) statq = stl_sprintf(" %da", apti )+statq;
            }
            else
            {
                string strb = ENUM_ATTR_STR(job_skill, caption_noun, columns[sel_column].skill);
                string strc = strb.substr(0,1);
                if(strc=="A"||strc=="E"||strc=="I"||strc=="O"){
                    strc="Never an ";
                }else{
                    strc="Never a ";
                }
                if(strb=="Balance"||strb=="Concentration"||strb=="Coordination"||strb=="Discipline"||strb=="Military Tactics"){
                    if(strb=="Military Tactics"){ strc="Never studied ";}
                    else{
                        strc="Never trained ";
                    }//ocd confirmed
                }
                descq = strc+ strb;
                if(apti>0) { statq =stl_sprintf(" (%da)",apti  ); }

            }

            str=descq+statq;
        }

        Screen::paintString(Screen::Pen(' ', COLOR_LIGHTCYAN, 0), x, y, str);

        if (cur->unit->military.squad_id > -1)
        {
            x = 1;
            y++;

            string squadLabel = "Squad: ";
            Screen::paintString(white_pen, x, y, squadLabel);
            x += squadLabel.size();

            string squad = cur->squad_effective_name;
            Screen::paintString(Screen::Pen(' ', 11, 0), x, y, squad);
            x += squad.size();

            string pos = stl_sprintf(" Pos %i", cur->unit->military.squad_position + 1);
            Screen::paintString(Screen::Pen(' ', 9, 0), x, y, pos);

        }

        canToggle = (cur->allowEdit) && Units::isValidLabor(unit, columns[sel_column].labor);
    }

    int x = 2, y = dim.y - 4;
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::SELECT));
    OutputString(canToggle ? 15 : COLOR_GREY, x, y, ": Toggle labor, ");

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::SELECT_ALL));
    OutputString(canToggle ? 15 : COLOR_GREY, x, y, ": Toggle Work, ");

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::OPTION20));
    OutputString(15, x, y, ": Color, ");

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::UNITJOB_VIEW_UNIT));
    OutputString(15, x, y, ": ViewCre, ");

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::UNITJOB_ZOOM_CRE));
    OutputString(15, x, y, ": ZoomCre");

    x = 2; y = dim.y - 3;
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::LEAVESCREEN));
    OutputString(15, x, y, ": Done,  ");

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CHANGETAB));
    OutputString(15, x, y, ": Showing ");
    string cout=detailmode_shortnames[detail_mode];
    OutputString(15, x, y, cout); // n=15?!!

    OutputString(10,x,y,Screen::getKeyDisplay(interface_key::SECONDSCROLL_UP));
    OutputString(10,x,y,Screen::getKeyDisplay(interface_key::SECONDSCROLL_DOWN));
    OutputString(15, x, y, ": Sorting");
    cout=widesort_names[static_cast<int>(widesort_mode)];//+

    if(widesort_mode!=WIDESORT_NONE){
        OutputString(15, x, y, cout);
        //char codesc= (widesort_descend)? 0x19:0x18;
        //cout=stl_sprintf("%c,",codesc);
        cout=stl_sprintf(", "); //disabled direction changing
        OutputString(15, x, y, cout);  //!remove this tweak sometime
        cout=widesort_gaps[static_cast<int>(widesort_mode)];//+
        OutputString(15, x, y, cout);
    }else
        OutputString(15, x, y, cout);

    OutputString(10,x,y,Screen::getKeyDisplay(interface_key::SECONDSCROLL_PAGEUP));
    OutputString(10,x,y,Screen::getKeyDisplay(interface_key::SECONDSCROLL_PAGEDOWN));
    OutputString(15, x, y, ": by ");

    cout=finesort_names[static_cast<int>(finesort_mode)];//"Sort fine "+
    OutputString(15, x, y, cout);

    x = 2; y = dim.y - 2;
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_X));
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_SHIFT_X));
    OutputString(15, x, y, ": Select ");
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_A));
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_SHIFT_A));
    OutputString(15, x, y, ": all/none, ");
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_B));
    OutputString(15, x, y, ": Batch ");
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_E));
    OutputString(15, x, y, ": Edit ");
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_P));
    OutputString(15, x, y, ": Apply Profession ");
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_SHIFT_P));
    OutputString(15, x, y, ": Read Prof. ");
}

df::unit *viewscreen_unitlaborsst::getSelectedUnit()
{
    do_refresh_names = true;
    return units[sel_row]->unit;
}

struct unitlist_hook : df::viewscreen_unitlistst
{
    typedef df::viewscreen_unitlistst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        if (input->count(interface_key::UNITVIEW_PRF_PROF))
        {
            if (units[page].size())
            {
                Screen::show(new viewscreen_unitlaborsst(units[page], cursor_pos[page]), plugin_self);
                return;
            }
        }
        INTERPOSE_NEXT(feed)(input);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();

        if (units[page].size())
        {
            auto dim = Screen::getWindowSize();
            int x = 2, y = dim.y - 2;
            OutputString(12, x, y, Screen::getKeyDisplay(interface_key::UNITVIEW_PRF_PROF));
            OutputString(15, x, y, ": Manage labors (DFHack)");
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(unitlist_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(unitlist_hook, render);

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    if (!gps)
        return CR_FAILURE;

    if (enable != is_enabled)
    {
        if (!INTERPOSE_HOOK(unitlist_hook, feed).apply(enable) ||
            !INTERPOSE_HOOK(unitlist_hook, render).apply(enable))
            return CR_FAILURE;

        is_enabled = enable;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands)
{
    if (!Filesystem::isdir(CONFIG_PATH) && !Filesystem::mkdir(CONFIG_PATH))
    {
        out.printerr("manipulator: Could not create configuration folder: \"%s\"\n", CONFIG_PATH);
        return CR_FAILURE;
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    INTERPOSE_HOOK(unitlist_hook, feed).remove();
    INTERPOSE_HOOK(unitlist_hook, render).remove();
    return CR_OK;
}
