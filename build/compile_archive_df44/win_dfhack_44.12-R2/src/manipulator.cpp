// Cavern Keeper - an improvement of dfhacks manipulator, same license.
// 1k lines of respectable code from dfhack by manipulators ancestral progenitors.
// 4k lines of malformatted chaos by AndrewInput@gmail.com
// Casual Release feb 2018, homed at github.com/strainer/

#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <MiscUtils.h>
#include <VTableInterpose.h>

#include <modules/Screen.h>
#include <modules/Gui.h>
#include <modules/Translation.h>
#include <modules/Units.h>
#include <modules/Filesystem.h>
#include <modules/Job.h>

#include <array>
#include <algorithm>
#include <chrono>
#include <vector>
#include <string>
#include <set>
#include <tuple>
#include <math.h>

#include "listcolumn.h"
#include "uicommon.h"

#include "df/ui.h"
#include "df/enabler.h"
#include "df/graphic.h"
#include "df/interface_key.h"
#include "df/viewscreen_unitlistst.h"

#include "df/caste_raw.h"
#include "df/creature_raw.h"
#include "df/creature_graphics_role.h"
#include "df/entity_raw.h"

#include "df/world.h"
#include "df/nemesis_record.h"
#include "df/historical_entity.h"
#include "df/historical_figure.h"
#include "df/historical_figure_info.h"
#include "df/histfig_hf_link.h"
#include "df/histfig_hf_link_type.h"
#include "df/activity_event.h"

#include "df/unit.h"
#include "df/unit_soul.h"
#include "df/unit_skill.h"
#include "df/unit_syndrome.h"

#include "df/unit_health_info.h"
#include "df/unit_health_flags.h"
#include "df/unit_wound.h"

#include "df/unit_preference.h"
#include "df/unit_relationship_type.h"
#include "df/unit_misc_trait.h"
#include "df/misc_trait_type.h"

#include "df/item_type.h"
#include "df/itemdef_weaponst.h"
#include "df/itemdef_trapcompst.h"
#include "df/itemdef_toyst.h"
#include "df/itemdef_toolst.h"
#include "df/itemdef_instrumentst.h"
#include "df/itemdef_armorst.h"
#include "df/itemdef_ammost.h"
#include "df/itemdef_siegeammost.h"
#include "df/itemdef_glovesst.h"
#include "df/itemdef_shoesst.h"
#include "df/itemdef_shieldst.h"
#include "df/itemdef_helmst.h"
#include "df/itemdef_pantsst.h"
#include "df/itemdef_foodst.h"

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

#define CONFIG_DIR "dfkeeper"

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

    string likesline;
    string dream;
    string tagline;
    string godline;
    string traits;
    string regards;

    enum { IDLE, SOCIAL, JOB, MILITARY } job_mode;
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
    int active_index;

    int work_aptitude = 0;
    int skill_aptitude = 0;

    int notice_score;
    int demand = 0;
    int scholar = 0;
    int performer = 0;
    int martial = 0;
    int medic = 0;
    int focus = 0;
    int civil = 0;

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
  "n/a"
};
const char * const detailmode_legend[] = {
  "Profession", "Squad", "Action", "Notice", "", "n/a"
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
  " Selection ",
  " Profession",
  " Squads    ",
  " Actions   ",
  " Arrivals  ",
  "n/a",
};

enum fine_sorts {
    FINESORT_UNDER=-1,
    FINESORT_UNFOCUSED=0,
    FINESORT_STRESS,
    FINESORT_SURNAME,
    FINESORT_NAME,
    FINESORT_TOPSKILLED,
    FINESORT_MEDIC,
    FINESORT_MARTIAL,
    FINESORT_PERFORMER,
    FINESORT_SCHOLAR,
    FINESORT_COLUMN,
    FINESORT_DEMAND,
    FINESORT_AGE,
    FINESORT_NOTICES,
    FINESORT_OVER
};

const char * const finesort_names[] = {

  "Unfocus",
  "Unkept",
  "Surname",
  "Name",
  "Civil",
  "Medic",
  "Martial",
  "Perform",
  "Scholar",
  "Column",
  "Availed",
  "Age",
  "Notices",
  "n/a",
};

static string cur_world;

int detail_mode = 0; //mode settings
int color_mode = 2;
int hint_power = 2;
int show_curse = 0;
int notices_countdown = 0;
bool show_sprites = false;
int show_details = 3;
int tran_names = 1;
int theme_color = 0;

int cheat_level = 0; //cheatmode
int spare_skill = 0;
int sel_attrib = 0;

int maxnamesz = 5; //for screen layout
int row_space = 0;
int dimex = 80, dimey = 30;
int xmargin = 0, xfooter = 0;
int xpanend = 0, xpanover = 0;

static int first_row = 0;       //focus position
static int display_rows_b = 0;
static int first_column = 0;
static int sel_column = 0;
static int sel_column_b = 0;

static int sel_row = 0;
static int sel_row_b = 0;
static int sel_unitid = -1;

static wide_sorts widesort_mode = WIDESORT_NONE;
static fine_sorts finesort_mode = FINESORT_NAME;
fine_sorts finesort_mode_b = FINESORT_UNDER;
static wide_sorts widesort_mode_b = WIDESORT_NONE;
static int column_sort_column = -1;
int column_sort_last = 0;
bool cancel_sort = false;

static map<int, bool> selection_stash;
static bool selection_changed = false;

static int tip_show = 0;
static int tip_show_timer = 0;

const char * const tip_settings[] = {
  " [    Color Pallete 1/3     ] ",
  " [    Color Pallete 2/3     ] ",
  " [    Color Pallete 3/3     ] ",
  " [     No Highlighting      ] ",
  " [  20% Aptitudes Highlight ] ",
  " [  40% Aptitudes Highlight ] ",
  " [  60% Aptitudes Highlight ] ",
  " [  75% Aptitudes Highlight ] ",
  " [   Revise Embark Skills   ] ",
  " [   Edit Skills and Stats  ] ",
  " [      Left Cheatmode      ] ",
  " [     No Descriptions      ] ",
  " [ 1/5 Description Setting  ] ",
  " [ 2/5 Description Setting  ] ",
  " [ 3/5 Description Setting  ] ",
  " [ 4/5 Description Setting  ] ",
  " [ 5/5 Description Setting  ] ",
  " [ Curses Hidden From Notes ] ",
  " [   Curses Are Revealed    ] ",
  " [   1/4 Naming Setting     ] ",
  " [   2/4 Naming Setting     ] ",
  " [   4/4 Naming Setting     ] ",
  " [   3/4 Naming Setting     ] ",
  " [ Details Hidden (Legacy)  ] "
};

void stashSelection(UnitInfo* cur){
    selection_stash[cur->unit->id] = cur->selected; //sel;
}

void stashSelection(vector<UnitInfo *> &units){
    for (size_t i = 0; i < units.size(); i++){
        selection_stash[units[i]->unit->id] = units[i]->selected;
    }
}

void unstashSelection(vector<UnitInfo *> &units){
    for (size_t i = 0; i < units.size(); i++){
        if(selection_stash[units[i]->unit->id]==true)
            units[i]->selected = true;
        else
           units[i]->selected = false;
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
    return d1->selected > d2->selected;
}

bool sortByProfession (const UnitInfo *d1, const UnitInfo *d2)
{
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

    return d1->unit->military.squad_id < d2->unit->military.squad_id ;
}

bool sortByArrival (const UnitInfo *d1, const UnitInfo *d2)
{
    return d1->arrival_group < d2->arrival_group;
}

bool sortByEnabled (const UnitInfo *d1, const UnitInfo *d2)
{
    if ( Units::isBaby(d2->unit) && !Units::isBaby(d1->unit) )
        return true;

    if ( Units::isChild(d2->unit) && !(Units::isChild(d1->unit)||Units::isBaby(d1->unit)) )
        return true;

    if ( (!d2->allowEdit) && (d1->allowEdit && !(Units::isChild(d1->unit)||Units::isBaby(d1->unit))))
        return true;

    return false;
}

bool sortByJob (const UnitInfo *d1, const UnitInfo *d2)
{
    if(d1->job_mode == UnitInfo::MILITARY && d2->job_mode != UnitInfo::MILITARY)
        return true;
    if(d2->job_mode == UnitInfo::MILITARY)
        return false;

    if(d1->job_mode == UnitInfo::IDLE && d2->job_mode != UnitInfo::IDLE)
        return true;
    if(d2->job_mode == UnitInfo::IDLE)
        return false;

    if(d1->job_mode == UnitInfo::SOCIAL && d2->job_mode != UnitInfo::SOCIAL)
        return true;
    if(d1->job_mode != UnitInfo::SOCIAL && d2->job_mode == UnitInfo::SOCIAL)
        return false;
    if(d1->job_mode == UnitInfo::SOCIAL && d2->job_mode == UnitInfo::SOCIAL)
        return d1->job_desc > d2->job_desc;

    if (d1->job_mode != d2->job_mode)
        return d1->job_mode < d2->job_mode;

    return d1->job_desc < d2->job_desc;
}

//finesorts (do before widesorts)
bool sortByScholar (const UnitInfo *d1, const UnitInfo *d2){
    return (d1->scholar > d2->scholar);
}
bool sortByMedic (const UnitInfo *d1, const UnitInfo *d2){
    return (d1->medic > d2->medic);
}
bool sortByNotices (const UnitInfo *d1, const UnitInfo *d2){
    return (d1->notice_score > d2->notice_score);
}
bool sortByMartial (const UnitInfo *d1, const UnitInfo *d2){
    return (d1->martial > d2->martial);
}
bool sortByPerformer (const UnitInfo *d1, const UnitInfo *d2){
    return (d1->performer > d2->performer);
}
bool sortByTopskilled (const UnitInfo *d1, const UnitInfo *d2){
    return (d1->civil > d2->civil);
}
bool sortByDemand (const UnitInfo *d1, const UnitInfo *d2){
    return (d1->demand > d2->demand);
}
bool sortByAge (const UnitInfo *d1, const UnitInfo *d2){
    return (d1->age > d2->age);
}
bool sortByName (const UnitInfo *d1, const UnitInfo *d2){
    return (d1->name < d2->name);
}
bool sortByTransName (const UnitInfo *d1, const UnitInfo *d2){
    return (d1->transname < d2->transname);
}
bool sortBySurName (const UnitInfo *d1, const UnitInfo *d2)
{
    return (d1->lastname < d2->lastname)
        || (d1->lastname == d2->lastname && d1->name < d2->name);
}
bool sortByUnfocused (const UnitInfo *d1, const UnitInfo *d2){
    return (d1->focus < d2->focus);
}
bool sortByStress (const UnitInfo *d1, const UnitInfo *d2)
{
    if (!d1->unit->status.current_soul)
        return true;
    if (!d2->unit->status.current_soul)
        return false;

    return (d1->unit->status.current_soul->personality.stress_level > d2->unit->status.current_soul->personality.stress_level);
}

bool sortByColumn (const UnitInfo *d1, const UnitInfo *d2)
{
    if (!d1->unit->status.current_soul)
        return false;
    if (!d2->unit->status.current_soul)
        return true;

    df::job_skill sort_skill = columns[column_sort_column].skill;
    df::unit_labor sort_labor = columns[column_sort_column].labor;

    int l1 = 0;
    int l2 = 0;

    if (sort_skill != job_skill::NONE)
    {
        l1 = unitSkillRating(d1,sort_skill)*50;
        l1 += 25+unitSkillExperience(d1,sort_skill)*40/(skill_levels[l1/50].points+1);
        //eg skill 1 = 50, 5 = 250, 10 = 500
        l1 *= d1->column_aptitudes[column_sort_column]+200;

        l2 = unitSkillRating(d2,sort_skill)*50;
        l2 += 25+unitSkillExperience(d2,sort_skill)*45/(skill_levels[l2/50].points+1);
        l2 *= d2->column_aptitudes[column_sort_column]+200;
    }

    if(sort_labor != unit_labor::NONE){
        if(d1->unit->status.labors[sort_labor])
            l1+= 10+l1/10;
        if(d2->unit->status.labors[sort_labor])
            l2+= 10+l2/10;
    }

    return l1 > l2;

}

bool sortByUnitId (const UnitInfo *d1, const UnitInfo *d2 ){
    return (d1->unit->id > d2->unit->id);
}//used to find arrival_groups

bool sortByActiveIndex (const UnitInfo *d1, const UnitInfo *d2 ){
    return (d1->active_index > d2->active_index);
}//used to find arrival_groups


string itos (int n)
{
    stringstream ss;
    ss << n;
    return ss.str();
}

int8_t cltheme_a[]={ //values for alpha background rendering mode
    //noskill        hint 0           hint 1         hint 2 
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

    //32 BG mili         33 FG other           34 row hint
    COLOR_LIGHTMAGENTA,  COLOR_LIGHTMAGENTA,   COLOR_BLUE
};


int8_t cltheme_b[]={
    /*noskill        hint 0           hint 1         hint 2 */
    //BG not set
    COLOR_BLACK,     COLOR_BLACK,     COLOR_BLACK,   COLOR_BLACK,
    //FG not set
    COLOR_DARKGREY,  COLOR_YELLOW,    COLOR_GREY,    COLOR_LIGHTCYAN,
    //BG not set and cursor
    COLOR_GREY,      COLOR_GREY,      COLOR_GREY,    COLOR_GREY,
    //FG not set and cursor
    COLOR_BLACK,     COLOR_BLACK,     COLOR_BLACK,   COLOR_BLACK,

    //BG set
    COLOR_DARKGREY,  COLOR_BROWN,    COLOR_DARKGREY,    COLOR_CYAN,
    //FG set
    COLOR_GREY,      COLOR_WHITE,    COLOR_WHITE,   COLOR_WHITE,
    //BG set and cursor
    COLOR_GREY,     COLOR_YELLOW,    COLOR_GREY,   COLOR_LIGHTCYAN,
    //FG set and cursor
    COLOR_WHITE,     COLOR_WHITE,     COLOR_WHITE,   COLOR_WHITE,

    //32 BG mili         33 FG other           34 row hint
    COLOR_MAGENTA,  COLOR_LIGHTMAGENTA,   COLOR_BLUE
};


int8_t cltheme_c[]={
    /*noskill        hint 0           hint 1         hint 2 */
    //BG not set
    COLOR_BLACK,     COLOR_BLACK,     COLOR_BLACK,   COLOR_BLACK,
    //FG not set
    COLOR_DARKGREY,  COLOR_LIGHTRED,    COLOR_GREY,  COLOR_LIGHTGREEN,
    //BG not set and cursor
    COLOR_GREY,      COLOR_GREY,      COLOR_GREY,    COLOR_GREY,
    //FG not set and cursor
    COLOR_BLACK,     COLOR_BLACK,     COLOR_BLACK,   COLOR_BLACK,

    //BG set
    COLOR_DARKGREY,  COLOR_RED,       COLOR_DARKGREY, COLOR_GREEN,
    //FG set
    COLOR_GREY,      COLOR_WHITE,     COLOR_WHITE,    COLOR_WHITE,
    //BG set and cursor
    COLOR_GREY,      COLOR_LIGHTRED,  COLOR_GREY,     COLOR_LIGHTGREEN,
    //FG set and cursor
    COLOR_WHITE,     COLOR_WHITE,     COLOR_WHITE,    COLOR_WHITE,

    //32 BG mili         33 FG other           34 row hint
    COLOR_MAGENTA,  COLOR_LIGHTMAGENTA,   COLOR_BLUE
};

int8_t *cltheme = cltheme_a;

PersistentDataItem config_dfkeeper;
void save_dfkeeper_config()
{
    config_dfkeeper = World::GetPersistentData("dfkeeper/config");
    if (!config_dfkeeper.isValid()){
        config_dfkeeper = World::AddPersistentData("dfkeeper/config");
        if (!config_dfkeeper.isValid())
            return;
    }

    config_dfkeeper.ival(0) = color_mode+1;
    config_dfkeeper.ival(1) = hint_power;
    config_dfkeeper.ival(2) = spare_skill;
    config_dfkeeper.ival(3) = show_details+1;
    config_dfkeeper.ival(4) = tran_names;
    config_dfkeeper.ival(5) = theme_color;
    config_dfkeeper.ival(6) = show_curse;
}

void read_dfkeeper_config()
{
    config_dfkeeper = World::GetPersistentData("dfkeeper/config");

    if (!config_dfkeeper.isValid()){
        save_dfkeeper_config();
        return;
    }
    //sel_row=config_manipulator.ival(0);
    color_mode = config_dfkeeper.ival(0)-1;
    hint_power = config_dfkeeper.ival(1);
    spare_skill = config_dfkeeper.ival(2);
    show_details = config_dfkeeper.ival(3)-1;
    if(show_details == -1) show_details = 1;
    tran_names = config_dfkeeper.ival(4);
    theme_color = config_dfkeeper.ival(5);
    show_curse = config_dfkeeper.ival(6);
    if(color_mode==-1){
      color_mode = 1;
      hint_power = 1;
    }

    //~ if(spare_skill>777){
        //~ cltheme[5]=cltheme[17]= COLOR_BROWN;
        //~ cltheme[6]=cltheme[18]= COLOR_DARKGREY;
        //~ cltheme[7]=cltheme[19]= COLOR_LIGHTRED;

        //~ cltheme[4] = COLOR_RED;          //FG for not set:
        //~ cltheme[12]= COLOR_DARKGREY;     //cursor FG not set:
        //~ cltheme[16]= COLOR_DARKGREY;     //BG set
        //~ cltheme[20]= COLOR_LIGHTRED;     //FG set
        //~ cltheme[24]= COLOR_LIGHTMAGENTA; //cursor BG set
    //~ }

    if(theme_color==0){
        cltheme=cltheme_a;
    }
    if(theme_color==1){
        cltheme=cltheme_b;
    }
    if(theme_color==2){
        cltheme=cltheme_c;
    }
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
        while (i < fmt.size()){
            if (in_opt){
                if (fmt[i] == '%'){
                    // escape: %% -> %
                    in_opt = false;
                    dest.push_back('%');
                    i++;
                }else{
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
                    }else{
                        // Unrecognized format option; replace with original text
                        dest.push_back('%');
                        in_opt = false;
                    }
                }
            }else{
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
    //  S A T E R D     A F W C I P M L S M K E S
    //  t g o n e i     n o i r n a e i p u i m o
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
    { { 0,0,0,0,0,0 },{ 0,9,9,0,9,0,0,0,0,0,0,0,9 } } /* MAGIC_NATURE */,
    { { 0,0,0,0,0,0 },{ 0,1,0,0,0,0,0,0,1,0,0,0,0 } } /* SNEAK */,
    { { 0,0,0,0,0,0 },{ 1,0,0,1,0,0,0,0,1,0,0,0,0 } } /* DESIGNBUILDING */,
    { { 0,1,0,0,0,0 },{ 0,0,0,0,0,0,0,0,1,0,0,1,0 } } /* DRESS_WOUNDS */,
    { { 0,0,0,0,0,0 },{ 1,0,0,0,1,0,1,0,0,0,0,0,0 } } /* DIAGNOSE */,
    { { 0,1,0,0,0,9 },{ 0,1,0,0,0,0,0,0,1,0,0,0,0 } } /* SURGERY */,
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
    { { 0,1,0,0,0,0 },{ 0,0,0,0,0,0,0,1,0,0,0,0,0 } } /* INTIMIDATION */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,1,0,0,0,1,1 } } /* CONVERSATION */,
    { { 0,1,0,0,0,0 },{ 0,0,0,1,0,0,0,1,0,0,0,0,0 } } /* COMEDY */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,1,0,0,0,1,1 } } /* FLATTERY */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,1,0,0,0,1,9 } } /* CONSOLE */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,1,0,0,0,1,1 } } /* PACIFY */,
    { { 0,0,0,0,0,0 },{ 0,9,0,0,9,0,0,0,0,0,0,9,0 } } /* TRACKING */,
    { { 0,1,0,0,0,9 },{ 1,1,0,0,0,0,1,0,0,0,0,0,0 } } /* KNOWLEDGE_ACQUISITION*/,
    { { 0,0,0,0,0,0 },{ 0,1,1,0,0,1,0,0,0,0,0,0,0 } } /* CONCENTRATION */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* DISCIPLINE */,
    { { 0,0,0,0,0,0 },{ 0,1,0,0,1,0,0,0,1,0,0,0,9 } } /* SITUATIONAL_AWARENESS*/,
    { { 0,0,0,0,0,0 },{ 0,9,0,0,0,0,0,9,0,0,0,0,0 } } /* WRITING */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,9,0,0,9,0,0,0,0,0 } } /* PROSE */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,9,0,0,0,9,0 } } /* POETRY */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,9,9,0,0,0,0,0 } } /* READING */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,9,0,0,9,0,0,0,0,9 } } /* SPEAKING */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* COORDINATION */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* BALANCE */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,1,0,0,0,1,1 } } /* LEADERSHIP */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,1,0,0,0,1,1 } } /* TEACHING */,
    { { 1,1,1,0,0,0 },{ 0,0,1,0,0,0,0,0,1,0,0,0,0 } } /* MELEE_COMBAT */,
    { { 1,1,0,0,0,0 },{ 0,0,0,0,0,0,0,0,1,0,0,0,0 } } /* RANGED_COMBAT */,
    { { 1,1,1,0,0,9 },{ 0,0,0,0,0,0,0,0,1,0,0,0,0 } } /* WRESTLING */,
    { { 1,0,1,1,0,9 },{ 0,0,0,0,0,0,0,0,1,0,0,0,0 } } /* BITE */,
    { { 1,1,1,0,0,0 },{ 0,0,0,0,0,0,0,0,1,0,0,0,0 } } /* GRASP_STRIKE */,
    { { 1,1,1,0,0,0 },{ 0,0,0,0,0,0,0,0,1,0,0,0,0 } } /* STANCE_STRIKE */,
    { { 0,1,0,1,0,0 },{ 0,0,0,0,0,0,0,0,1,0,0,0,0 } } /* DODGING */,
    { { 1,1,1,0,0,0 },{ 0,0,0,0,0,0,0,0,1,0,0,0,0 } } /* MISC_WEAPON */,
    { { 0,0,0,0,0,0 },{ 1,0,0,0,0,0,0,0,1,0,0,0,0 } } /* KNAPPING */,
    { { 0,0,0,0,0,0 },{ 0,0,9,9,0,0,0,0,9,0,0,0,0 } } /* MILITARY_TACTICS */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* SHEARING */,
    { { 1,1,0,1,0,0 },{ 0,0,0,0,0,0,0,0,1,0,0,0,0 } } /* SPINNING */,
    { { 0,1,0,0,0,0 },{ 0,0,0,1,0,0,0,0,1,0,0,0,0 } } /* POTTERY */,
    { { 0,1,0,0,0,0 },{ 0,0,0,1,0,0,0,0,1,0,0,0,0 } } /* GLAZING */,
    { { 1,1,0,1,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* PRESSING */,
    { { 1,1,0,1,0,0 },{ 1,0,0,0,0,0,0,0,0,0,0,0,0 } } /* BEEKEEPING */,
    { { 0,1,0,0,0,0 },{ 0,0,0,1,0,0,0,0,1,0,0,0,0 } } /* WAX_WORKING */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* CLIMBING */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* GELD */,
    { { 0,9,0,9,0,0 },{ 0,0,0,0,0,0,0,0,0,9,9,0,0 } } /* DANCE */,
    { { 0,0,0,0,0,0 },{ 0,0,0,9,0,0,0,0,0,9,0,0,0 } } /* MAKE_MUSIC */,
    { { 0,0,0,0,0,0 },{ 0,9,0,0,0,0,0,0,0,9,0,0,0 } } /* SING_MUSIC */,
    { { 0,9,0,0,0,0 },{ 0,9,0,0,0,0,0,0,0,9,0,0,0 } } /* PLAY_KEYBOARD_INST*/,
    { { 0,0,0,0,0,0 },{ 0,9,0,0,0,0,0,0,9,9,0,0,0 } } /* PLAY_STRINGED_INST*/,
    { { 0,0,0,0,0,0 },{ 0,9,0,0,9,0,0,0,0,9,0,0,0 } } /* PLAY_WIND_INSTRUMENT*/,
    { { 0,0,0,0,0,0 },{ 0,9,0,0,0,0,0,0,0,9,9,0,0 } } /* PLAY_PERCUSSION_INS*/,
    { { 0,0,9,0,0,0 },{ 9,0,0,0,0,0,0,0,0,0,0,0,0 } } /* CRITICAL_THINKING */,
    { { 0,0,0,0,0,0 },{ 9,0,0,0,0,9,0,0,0,0,0,0,0 } } /* LOGIC */,
    { { 0,0,0,0,0,0 },{ 9,0,0,9,0,0,9,9,9,0,0,0,0 } } /* MATHEMATICS */,
    { { 0,0,0,0,0,0 },{ 0,9,0,9,0,0,9,0,0,0,0,0,0 } } /* ASTRONOMY */,
    { { 0,0,0,0,0,9 },{ 9,0,0,9,0,9,0,0,0,0,0,0,0 } } /* CHEMISTRY */,
    { { 0,0,0,0,0,0 },{ 0,0,9,0,0,0,9,0,9,0,0,0,9 } } /* GEOGRAPHY */,
    { { 0,0,0,0,0,0 },{ 9,0,0,0,9,0,0,0,9,0,0,0,0 } } /* OPTICS_ENGINEER */,
    { { 0,0,0,0,0,0 },{ 9,0,0,9,0,0,0,0,9,0,0,0,0 } } /* FLUID_ENGINEER */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* PAPERMAKING */,
    { { 0,0,0,0,0,0 },{ 0,0,0,0,0,0,0,0,0,0,0,0,0 } } /* BOOKBINDING */
    };//S A T E R D     A F W C I P M L S M K E S

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
        
        if(!d->unit->status.current_soul) return 0;
        
        if (labor != unit_labor::NONE && skill == job_skill::NONE)
            skill = labor_skill_map[labor];

        if( skill == job_skill::NONE ) return 0;

        int weights = 0, wg=0, bitmask = 7;
        for (int pa = 0; pa < 6; pa++)
        {
            wg = bitmask & skills_attribs[skill].phys_attr_weights[pa];
            if( wg != 0 )
            {
                weights+= wg;
                attr_weight+= wg * (d->unit->body.physical_attrs[pa].value );
            }
        }

        for (int ma = 0; ma < 13; ma++)
        {
            wg= bitmask & skills_attribs[skill].mental_attr_weights[ma];
            if( wg != 0 )
            {
                weights+= wg;
                attr_weight+= wg * (d->unit->status.current_soul->mental_attrs[ma].value );
            }
        }

        if( weights > 1 ) attr_weight /= weights;

        if(attr_weight<13&&attr_weight>0)
        {   attr_weight = 1; }
        else
        {   attr_weight = ((attr_weight+12)/25); }

        if(attr_weight>200) attr_weight = 200;

        return attr_weight;
    }

    void allotHintColors(UnitInfo* cur, int column_unit_apt[], int a_col,int e_col, int avg_apt, int unit_apt)
    {
        int cn = e_col - a_col;
        double rank = static_cast<int>(1000*log2(static_cast<double>(unit_apt+avg_apt)/static_cast<double>(avg_apt+2)));
        //0 1000 2000(4)
        rank=(rank*115)/100-150;
        if(rank>1800)
            rank = 1800;
        if(rank<0)
            rank = 0;

        const int pwrs[] = {30000,13500,7500,4500}; //few,,,many
        int highs = (cn*rank)/pwrs[hint_power]+1;
        int lows  = (cn*(1800-rank))/pwrs[hint_power]+1;

        for( int j = a_col; j<e_col; j++){
            cur->column_hints[j] = 1;
        }

        while(highs>0){

          highs--;
          int bval = -100000; //(b)efore value
          int bpos = 0;       //(b)efore position

           for( int j = a_col; j<e_col; j++){
               if(cur->column_hints[j]==1
                 &&column_unit_apt[j]>=bval
               ){
                   bpos = j;
                   bval = column_unit_apt[j];
               }
           }
           cur->column_hints[bpos] = 2;
        }

        while(lows>0){

          lows--;
          int bval = 100000;
          int bpos = 0;

           for( int j=a_col; j<e_col; j++){
               if(cur->column_hints[j]==1
                 &&column_unit_apt[j]<=bval
                 &&column_unit_apt[j]>-111111
               ){
                   bpos = j;
                   bval = column_unit_apt[j];
               }
           }
           cur->column_hints[bpos] = 0;
       }
    }


const int traitscore[] ={
//mil       civ       pfm       aca       med
//hi ln lw
 -1, 0, 0,  0, 1, 0,  0, 1, 0,  0, 0, 0,  0, 0, 0//LOVE_PROPENS
,-2,-1,-2, -1,-1, 0,  0,-2, 0,  0,-2, 0, -2,-2, 0//HATE_PROPENS
,-1,-1, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0, -1, 0, 0//ENVY_PROPENS
, 0, 1,-1,  0, 0, 0,  0, 2, 0,  0, 1, 0,  0, 1, 0//CHEER_PROPEN
,-1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0//DEPRESSION_P
,-1, 0, 0,  0, 0, 0,  0,-1, 0,  0, 0, 0,  0, 0, 0//ANGER_PROPEN
,-1,-2, 1,  1, 1, 0,  0, 0, 0,  0, 1, 0, -1, 0, 0//ANXIETY_PROP
, 0, 0, 0,  0, 0, 0,  0, 1, 0,  0, 1, 0,  0, 0, 0//LUST_PROPENS
, 0,-2, 1,  0, 1, 0,  0, 0, 0,  1, 0, 0, -1, 0, 0//STRESS_VULNE
,-1, 0, 0,  0, 0, 0,  0, 1, 0,  0, 0, 0,  0, 0, 0//GREED
, 0, 0, 0,  0, 0, 0,  0, 1, 0,  0, 1, 0, -1, 0,-1//IMMODERATION
,-2, 2,-2,  0, 0, 0, -1, 0, 0,  0, 0, 0, -1,-1, 1//VIOLENT
, 0, 2,-1,  0, 1, 0,  0, 0, 1,  0, 0, 0,  0, 1,-1//PERSEVERENCE
, 0, 0, 0,  0,-1, 0,  0, 1, 0,  0, 1, 0, -1, 0,-1//WASTEFULNESS
, 0, 0,-1,  0, 1, 0,  0, 1, 0,  0, 1, 0, -1, 1, 0//DISCORD
, 0, 1, 0,  0,-1, 0,  0, 1, 0,  0, 0, 0,  0, 1, 0//FRIENDLINESS
,-1, 0,-1,  0, 0, 0,  0, 0, 0,  0, 1, 0, -1, 1,-1//POLITENESS
,-1, 0,-1,  0, 0, 0,  0,-1, 0,  0,-1, 0,  0, 0, 0//DISDAIN_ADVI
,-1, 3,-1,  0,-1, 0,  0, 0, 0,  0,-1, 0,  1, 0, 0//BRAVERY
, 0, 2,-1,  0,-1, 0,  0, 1, 0,  0, 0, 0,  0, 1,-1//CONFIDENCE
, 0, 0, 0,  0, 0, 0,  0, 1, 0,  0, 0, 0,  0, 0, 0//VANITY
,-1, 0, 0,  0, 1, 0,  0, 1, 0,  0, 1, 0,  0, 0, 0//AMBITION
,-1, 1, 0,  0, 0, 0,  0, 0, 0,  0, 1, 0,  0, 1, 0//GRATITUDE
,-1,-1, 0,  0,-1, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0//IMMODESTY
, 0, 1, 0,  0, 1, 0,  1, 1,-1,  0, 0, 0,  0, 1, 0//HUMOR
,-3,-1, 0,  0, 0, 0,  0,-1, 0,  0, 0, 0, -1,-1, 0//VENGEFUL
, 0,-1, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0//PRIDE
,-3,-2,-1,  0,-1, 0,  0,-2, 1, -1,-1, 0, -5,-2, 2//CRUELTY
, 0, 0,-2,  0, 1, 0,  0, 0, 0,  0, 0, 0, -1, 1,-1//SINGLEMINDED
, 0, 2, 0,  0, 0, 0,  0, 1, 0,  0, 0, 0,  0, 1, 0//HOPEFUL
, 0, 1, 0,  0, 0, 0,  0, 1, 0,  1, 2,-1,  0, 0, 0//CURIOUS
,-1, 0,-1,  0, 1, 0, -1,-1, 1,  0, 1, 0, -1, 1, 0//BASHFUL
, 0,-1, 0,  0, 1, 0,  0,-2, 1,  0, 1, 0,  0, 0, 0//PRIVACY
, 0, 0, 0,  1, 3, 0,  0, 0, 0,  0, 1, 0,  0, 0,-2//PERFECTIONIS
, 0, 0, 0,  0, 0, 0,  0,-2, 0,  0,-1, 0,  0, 0, 0//CLOSEMINDED
, 0, 2,-1,  0, 0, 1,  0, 2, 0,  0, 1, 0,  0, 2,-1//TOLERANT
,-1, 0, 1,  0, 0, 0,  0, 1, 0,  0, 0, 0, -1, 0, 0//EMOTION OBSES_
,-2, 0, 1,  0, 1, 0,  0, 0, 0,  0, 1, 0, -2, 0, 0//SWAYED_BY_EM
,-1, 1, 0,  0, 0, 0,  0, 2, 0,  0, 0, 0,  0, 2,-1//ALTRUISM
, 1, 2,-2,  0, 1, 0,  0,-1, 0,  0,-1, 0,  0, 0, 0//DUTIFULNESS
,-1, 0,-1,  1, 0, 1,  0, 0, 0,  0, 0, 0, -1, 0, 0//THOUGHTLESSN
,-1, 1, 0,  0, 2,-1,  0,-1, 0,  0, 1, 0,  0, 1, 0//ORDERLINESS
,-1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0//TRUST
, 0, 1, 0,  0,-2, 1,  0, 3,-1, -1, 1, 1,  0, 1,-1//GREGARIOUSNE
, 1, 1, 0,  0,-1, 1,  0, 0, 0,  0, 0, 1, -1, 1, 0//ASSERTIVENES
, 0, 2, 0,  2, 3,-1,  0, 0, 0,  0, 0, 0,  0, 1,-1//ACTIVITY_LEV
, 1, 1, 0, -1,-3, 2,  0, 0, 0, -1,-1, 0,  0, 0, 0//EXCITEMENT_S
,-1, 0, 1,  0, 0, 0,  0, 4, 0,  2, 2,-1, -1, 0, 0//IMAGINATION
, 0,-1, 0, -1,-1, 0,  0, 1, 0,  2, 2,-1, -1, 0, 0//ABSTRACT_INC
,-1, 0, 1,  1, 1, 0,  1, 1,-1,  0, 1, 0,  0, 0, 0//ART_INCLINED
};

const int regardscore[] ={
//mil civ pfm aca med
  2,-1,-1, 0, 0  //"Law"
, 2, 0, 1, 0, 0  //"Loyal"
,-1, 1, 1, 0, 0  //"Family"
, 1, 0, 0, 1, 1  //"Friendship"
, 0, 0, 0, 1, 0  //"Power"
, 0, 0, 0, 1, 0  //"Truth"
, 0, 0, 0, 0, 0  //"Cunning"
, 0, 0, 1, 1, 0  //"Eloquence"
, 1, 0, 1, 0, 1  //"Equity"
, 0, 0, 0, 0, 1  //"Decorum"
, 0, 0, 0, 0, 0  //"Tradition"
, 0, 1, 0, 1, 0  //"Art"
, 1, 0, 1, 0, 0  //"Accord"
,-2, 1, 1, 2, 0  //"Freedom"
, 1, 0,-1, 0, 0  //"Stoicism"
,-1, 0, 1, 2, 0  //"SelfExam"
, 1, 0, 1, 0, 1  //"SelfCtrl"
,-2, 1,-1, 2, 0  //"Quiet"
, 1, 0, 1, 0, 1  //"Harmony"
, 1, 0, 1, 0, 0  //"Mirth"
,-1, 3,-2,-1,-1  //"Craftwork"
, 3,-2,-2, 0,-1  //"Combat"
, 2, 1, 0,-1, 0  //"Skill"
, 1, 3,-1,-1, 1  //"Labour"
, 2, 0, 0,-1, 1  //"Sacrifice"
, 2, 0, 0, 0,-1  //"Rivalry"
, 2, 0, 0, 0, 1  //"Grit"
,-1,-2, 1, 0,-1  //"Leisure"
, 0, 1, 1, 1, 0  //"Commerce"
, 0, 1, 1, 1, 0  //"Romance"
,-1, 1, 0, 1, 1  //"Nature"
,-1, 1, 1, 0, 1  //"Peace"
,-1,-1,-1, 4, 1  //"Lore"
};

const int dreamscore[] ={
//mil civ pfm aca med
 10,10,10,10,10  // "Surviving"
,10,10,11,11,10  // "Status"
, 9,10,10,10,10  // "Family"
,11,10,11,11,10  // "Power"
, 9,12,10,10,10  // "Artwork"
, 9,12, 8, 9, 9  // "Craftwork"
,10,10,10,11,10  // "Peace"
,12,10, 9, 9,10  // "Combat"
,11,11,10, 9,11  // "Skill"
,10,11,11,10,10  // "Romance"
,12,10,10,11,10  // "Voyages"
, 7, 8, 8, 9, 8  // "Immortality"
};

static int adjustscores[] = { 0,0,0,0,0 }; //mil civ pfm aca med

void assess_traits(UnitInfo *cur){

    int sn=5; //nb of scores
    int pc=0,x=0;
    adjustscores[0]=adjustscores[1]=adjustscores[2]=
    adjustscores[3]=adjustscores[4]=10;

    if(!&cur->unit->status.current_soul) return;
    auto soul = cur->unit->status.current_soul;
    
    if(&soul->personality.traits){
        auto traits = soul->personality.traits;

        for(int c=0;c<50;c++){
            pc=((int)traits[c])-50;
            for(int q=0; q<sn; q++){
                if(pc>13){ //high adjust
                    adjustscores[q]+=(traitscore[x]  *(pc-13)*3)/2;
                }else if(pc<-13){ //low adjust
                    adjustscores[q]-=(traitscore[x+2]*(pc+13)*3)/2;
                }
                //linear adjust
                adjustscores[q]+=traitscore[x+1]*pc;
                x+=3;
            }
        }
        
        //cerr <<  "traits adjust " << 0 << ":sco:" << adjustscores[0]<<"\n";
        //cerr <<  "traits adjust " << 1 << ":sco:" << adjustscores[1]<<"\n";
        //cerr <<  "traits adjust " << 2 << ":sco:" << adjustscores[2]<<"\n";
        //cerr <<  "traits adjust " << 3 << ":sco:" << adjustscores[3]<<"\n";
        //cerr <<  "traits adjust " << 4 << ":sco:" << adjustscores[4]<<"\n";

    }

    if(&soul->personality.values){
        auto &regards = soul->personality.values;
        int vn = regards.size();
        for(int c=0;c<vn;c++){
            int rc=((int)regards[c]->type);
            int pc=((int)regards[c]->strength);
            pc=(pc>25)? 25:(pc<-25)? -25:pc;
            for(int q=0; q<sn; q++){
                adjustscores[q]+=regardscore[rc*sn+q]*pc*2;
            }
        }
        //cerr <<  "+values adjust " << 0 << ":sco:" << adjustscores[0]<<"\n";
        //cerr <<  "+values adjust " << 1 << ":sco:" << adjustscores[1]<<"\n";
        //cerr <<  "+values adjust " << 2 << ":sco:" << adjustscores[2]<<"\n";
        //cerr <<  "+values adjust " << 3 << ":sco:" << adjustscores[3]<<"\n";
        //cerr <<  "+values adjust " << 4 << ":sco:" << adjustscores[4]<<"\n";
    }

    if(&soul->personality.dreams){
        int cn = soul->personality.dreams.size()-1;
        if(cn>-1){
            int dr = soul->personality.dreams[cn]->type;
            if(dr>-1) for(int q=0; q<sn; q++){
                int adj= dreamscore[dr*sn+q];
                adjustscores[q]
                = (adjustscores[q]*adj)/10+(adj-10)*25;
            }
        }
        //cerr <<  "+dreams adjust " << 0 << ":sco:" << adjustscores[0]<<"\n";
        //cerr <<  "+dreams adjust " << 1 << ":sco:" << adjustscores[1]<<"\n";
        //cerr <<  "+dreams adjust " << 2 << ":sco:" << adjustscores[2]<<"\n";
        //cerr <<  "+dreams adjust " << 3 << ":sco:" << adjustscores[3]<<"\n";
        //cerr <<  "+dreams adjust " << 4 << ":sco:" << adjustscores[4]<<"\n";
    }

}

    void calcAptScores(vector<UnitInfo*> &units)  //!
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

        int column_unit_apt[NUM_LABORS] = {};
        int dither = 123;
        //sweep again to set hints
        for (size_t i = 0; i < unit_count; i++)
        {
            UnitInfo *cur = units[i];

            //make normalised apt vals
            for (size_t j = 0; j <= col_end; j++)
            {
                int col_avg = column_total_apt[j]/unit_count;
                //make adjusted apts
                if(column_total_apt[j]){
                    column_unit_apt[j] = 5*(cur->column_aptitudes[j]-(col_avg*2)/3);
                    column_unit_apt[j] += (j*j*j^dither++)%5; //psuedofundom
                }else{
                    column_unit_apt[j] = -111111; //out of hint range for noskill roles
                }
            }

            int skill_col = 75; //this division doesnt need to be totaly precisely

            allotHintColors(cur,column_unit_apt,0,skill_col,uinfo_avg_work_aptitude,cur->work_aptitude);

            allotHintColors(cur,column_unit_apt,skill_col,NUM_LABORS,uinfo_avg_skill_aptitude,cur->skill_aptitude);

        }//units


        //sweep again to set skill class scores
        for (size_t i = 0; i < unit_count; i++)
        {
            UnitInfo *cur = units[i];
            cur->scholar = 0;
            cur->performer = 0;
            cur->martial = 0;
            cur->civil = 0;

            for (size_t j = 0; j <= col_end; j++)
            {
                int avg_skill= column_total_skill[j]/unit_count;

                df::unit_labor col_labor = columns[j].labor;
                df::job_skill col_skill = columns[j].skill;

                int group = columns[j].group;

                int mskill = 1+unitSkillRating(cur,col_skill);
                int wskill = unitSkillRating(cur,col_skill); //0 to 20

                wskill = wskill*4 + ((wskill+2)*cur->column_aptitudes[j])/11 ;
                wskill = static_cast<int>(std::sqrt( static_cast<double>(wskill) ));
                //approx 5 to 20

                if( group<11 ){
                    if(col_skill==df::job_skill::WOODCUTTING
                     ||col_skill==df::job_skill::DISSECT_VERMIN
                     ||col_skill==df::job_skill::GELD
                     ||col_skill==df::job_skill::MILK
                     ||col_skill==df::job_skill::SHEARING
                     ||col_skill==df::job_skill::WOOD_BURNING){
                        cur->civil+=
                            static_cast<int>(wskill*std::sqrt(static_cast<double>(mskill)/static_cast<double>(10+avg_skill*2)));
                    }else{
                        cur->civil+=
                            static_cast<int>(wskill*std::sqrt(static_cast<double>(mskill)/static_cast<double>(5+avg_skill)));
                    }
                }

                //martial
                if( (group>12 && group < 16)
                  ||col_skill==df::job_skill::MILITARY_TACTICS
                  ||col_skill==job_skill::PACIFY){
                  if(col_skill==df::job_skill::TEACHING){
                      cur->martial+=wskill/4+(cur->martial*wskill)/(300);
                  }else{
                      cur->martial += mskill;
                  }

                  if(group==13) cur->martial += wskill/4;
                }

                if( col_skill==df::job_skill::DISCIPLINE ) cur->martial += wskill;

                if( col_skill==df::job_skill::KNOWLEDGE_ACQUISITION
                  ||col_skill==df::job_skill::DODGING ){
                    cur->martial += wskill/4;
                }

                if( group == 17
                ||col_skill==df::job_skill::LEADERSHIP
                ||col_skill==df::job_skill::ORGANIZATION ){
                    cur->martial += wskill/9;
                }

                //scholar
                if(col_skill==df::job_skill::TEACHING
                  ||col_skill==df::job_skill::CONCENTRATION
                  ||col_skill==df::job_skill::WRITING
                  ||col_skill==df::job_skill::READING
                  ||col_skill==df::job_skill::KNOWLEDGE_ACQUISITION
                  ||col_skill==df::job_skill::CONVERSATION
                  ||group==21){
                    cur->scholar += wskill;
                }

                //performer
                if( group == 19 && col_skill != df::job_skill::WRITING
                ||col_skill==job_skill::COMEDY
                ||col_skill==job_skill::FLATTERY
                ||col_skill==job_skill::CONSOLE
                ||col_skill==job_skill::PACIFY){
                    if(group == 19)
                        cur->performer += wskill;
                    else
                        cur->performer += wskill/2;
                }

                //medic
                if(col_skill==job_skill::DRESS_WOUNDS
                 ||col_skill==job_skill::SUTURE
                 ||col_skill==job_skill::SET_BONE
                 ||col_skill==job_skill::SURGERY
                 ||col_skill==job_skill::DIAGNOSE
                 ){
                    cur->medic += wskill;
                }
            }

            cur->martial += cur->unit->body.physical_attrs[4].value/180; //recoupe
            cur->martial += cur->unit->body.physical_attrs[5].value/320; //disease

            assess_traits(cur);
            //mil civ pfm aca med

            //cerr <<  "martial " << cur->martial <<"\n";
            //cerr <<  "civil   " << cur->civil<<"\n";
            //cerr <<  "perform " << cur->performer<<"\n";
            //cerr <<  "scholar " << cur->scholar<<"\n";
            //cerr <<  "medic   " << cur->medic<<"\n";
            
            cur->martial   = cur->martial*3    + adjustscores[0];
            cur->civil     = cur->civil*4      + adjustscores[1];
            cur->performer = cur->performer*4  + adjustscores[2];
            cur->scholar   = cur->scholar*4    + adjustscores[3];
            cur->medic     = cur->medic*5      + adjustscores[4];
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

        if(!unit->curse.add_tags1.bits.NOT_LIVING
          &&unit->curse.add_tags2.bits.NO_AGING )
            cursetype = "Imortal "; 
        
        if(unit->flags3.bits.ghostly)
            cursetype = "Ghost ";

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
            units[i]->notices = "";
            units[i]->notice_score = 0;

            df::unit *curu = units[i]->unit;

            if (!(curu->status.current_soul)){
                units[i]->notices = "Husk";
                units[i]->notice_score = 0;
                continue;
            }
            string lks = "";
            string iss = "";
            int iscore = 0;
            int newwnd = 0;
            int oldwnd = 0;

            //if(curu->body)
            for ( size_t a = 0; a < curu->body.wounds.size(); a++ ) {
                df::unit_wound* wound = curu->body.wounds[a];
                if ( wound->age!=0 && wound->age < 111 ) {
                    newwnd++;
                }else if ( wound->age >= 500 ) {
                    //oldwnd++;
                }
            }

            if(show_curse) iss+=determineCurse(units[i]->unit);
            //these are so adverb easier on one line, fingersx lint passes...
            if(curu->flags2.bits.underworld&&show_curse){
                iss += "Undrwld "; iscore+=2000;
            }
            if(curu->flags2.bits.killed){
                iss += "Dead "; iscore+=1330;
            }
            if(curu->flags3.bits.available_for_adoption){
                iss += "Orphan "; iscore+=600;
            }
            if(curu->flags1.bits.marauder){
                iss += "Maraudr "; iscore+=900;
            }
            if(curu->flags1.bits.active_invader){
                iss += "Invadr "; iscore+=900;
            }
            else if(curu->flags2.bits.visitor_uninvited&&show_curse){
                iss += "Intrudr "; iscore+=900;
            }
            //~ if(!curu->flags1.bits.important_historical_figure){ iss+="Fauna "; iscore+=1;}

            if(curu->pregnancy_timer > 0){
                iss += "Pregnt "; iscore+=700;
            }

            if(curu->flags1.bits.has_mood){
                iss += "Mood "; iscore+=305;
            }
            if(curu->flags1.bits.drowning){
                iss += "Drowning "; iscore+=2000;
            }
            if(curu->flags1.bits.projectile){
                iss += "Falling "; iscore+=945;
            }
            if(curu->flags3.bits.dangerous_terrain){
                iss += "DngrTrrn "; iscore+=600;
            }
            if(curu->flags3.bits.floundering){
                iss += "Flundrg "; iscore+=600;
            }

            if(curu->counters.suffocation){
                iss += "Suffcatn "; iscore+=2000;
            }
            if(curu->counters.pain>10){
                iss += "Agony "; iscore+=700;
            }

            if(curu->health){
                if (curu->health->flags.bits.needs_recovery){
                    iss += "RqRescue "; iscore += 1600;
                }else if(curu->health->flags.bits.rq_diagnosis){
                    iss += "RqDoctor "; iscore += 1200;
                }else if (curu->health->flags.bits.rq_immobilize
                   ||curu->health->flags.bits.rq_surgery
                   ||curu->health->flags.bits.rq_traction
                   ||curu->health->flags.bits.rq_immobilize)
                {
                    iss += "NdSurgry "; iscore += 1300;
                }else if(curu->health->flags.bits.rq_dressing
                    ||curu->health->flags.bits.rq_cleaning
                    ||curu->health->flags.bits.rq_suture
                    ||curu->health->flags.bits.rq_setting
                    ||curu->health->flags.bits.rq_crutch)
                {
                    iss += "NdNurse "; iscore += 700;
                }else if(newwnd&&iscore<1308){
                    iss += "Ruffdup "; iscore += 403;
                }
            }

            if(curu->flags1.bits.chained){
                iss += "Chaind "; iscore += 500;
            }else if(curu->flags1.bits.caged){
                iss += "Trappd "; iscore += 500;
            }

            int exh=curu->counters2.exhaustion;
            if(exh>5000){
                iss += "Xhaustd "; iscore += 601;
            }

            if(curu->flags2.bits.vision_damaged){
                iss += "VisnDmg "; iscore += 809;
            }
            if(curu->flags2.bits.breathing_problem){
                iss += "LungDmg "; iscore += 608;
            }

            //name=SYndrometype->syn_name ?
            int unknowns=0;

            string addlater;
            //if(curu->syndromes)
            for ( size_t b = 0; b < curu->syndromes.active.size(); b++ ) {
                int q = curu->syndromes.active[b]->type;
                if(q == 63||q == 60){
                   addlater="Drunk "; iscore += 103;
                }else{
                   unknowns++; iscore += 303;
                }
            }
            if(unknowns>0){
              if(unknowns>1) iss += "Sick ";
              else           iss += "Intxctd ";
            }

            if(curu->counters2.stored_fat<3000&&curu->counters2.hunger_timer>3000){
                iss += "XStarvtn "; iscore += 2000;
            } else if(curu->counters2.stored_fat<9000){
                iss += "Emaciatd "; iscore += 905;
            }
            if(curu->flags3.bits.injury_thought){
                iss += "Pained "; iscore += 402;
            }

            if ( curu->status2.limbs_grasp_max == 2){
                if ( curu->status2.limbs_grasp_count == 1){
                    iss += "OneHndd "; iscore += 107;
                }else if ( curu->status2.limbs_grasp_count == 0){
                    iss += "NoGrasp "; iscore += 308;
                }
            }

            if ( curu->status2.limbs_stand_max == 2){
                if ( curu->status2.limbs_stand_count == 1){
                    iss += "OneLegd "; iscore += 103;
                }else if ( curu->status2.limbs_stand_count == 0){
                    iss += "BadLegs "; iscore += 303;
                }
            }

            if(curu->flags3.bits.emotionally_overloaded){
                iss += "Despair "; iscore += 1013;
            }

            //if(curu->counters.unconscious){ iss += "Unconsc. "; iscore += 800;}
            if(curu->counters2.numbness){
                iss += "Numb "; iscore += 1055;
            }
            if(curu->counters.stunned){
                iss += "Stund "; iscore += 633;
            }
            if(curu->counters.nausea>1000){
                iss += "Naus "; iscore += 210;
            }
            if(curu->counters.dizziness>1000){
                iss += "Dzzy "; iscore += 500;
            }
            if(curu->counters2.paralysis){
                iss += "Parlysd "; iscore += 3005;
            }
            if(curu->counters2.fever>10){
                iss += "Fever "; iscore += 1002;
            }
            if(curu->counters2.thirst_timer>30000){
                iss += "Dehydt "; iscore += 1005;
            }
            if(curu->counters2.sleepiness_timer>55000){
                iss += "vSleepy "; iscore += 609;
            }
            if(exh<=5000&&exh>2500){
                iss += "Tired "; iscore += 202;
            }

            //if(curu->status)
            for (int r = 0; r < curu->status.misc_traits.size(); r++)
            {

                auto tr = curu->status.misc_traits[r];
                //up to 403200 ?
                if (tr->id == df::misc_trait_type::WantsDrink){
                    if(tr->value>200000){ iss +="NdBooz "; }
                    else if(tr->value>100000){ iss +="vDry "; }
                    else if(tr->value>40000){ iss +="Dry ";}

                    if(tr->value>2000) iscore += tr->value/666;
                }

                if (tr->id == df::misc_trait_type::NoPantsAnger){
                    if(tr->value>20){
                      iss +="nPants ";
                      iscore +=tr->value/100;
                    }
                }
                if (tr->id == df::misc_trait_type::NoShirtAnger){
                    if(tr->value>20){
                        iss +="nShirt ";
                        iscore +=tr->value/100;
                    }
                }
                if (tr->id == df::misc_trait_type::NoShoesAnger){
                    if(tr->value>20){
                        iss +="nShoes ";
                        iscore +=tr->value/100;
                    }
                }
            }

            iss += addlater;

            //units[i]->likes = "na"; //not done yet
            units[i]->notices = iss;
            units[i]->notice_score = iscore;
        }
    }

//pinched from dwarfmonitor
df::world_raws::T_itemdefs &defs = world->raws.itemdefs;

// COIN ROCK SLAB PET GEM CABINET BIN BOX TABLE BARREL CHAIN CHAIR BED DOOR ROUGH
string getItemLabel(int &item_type,int &subtype)
{
    string label;
    if(subtype!=-1){

        switch (item_type)
        {
        case (df::item_type::WEAPON):
            if(defs.weapons.size()>subtype && defs.weapons[subtype])
            label = defs.weapons[subtype]->name_plural;
            break;
        case (df::item_type::TRAPCOMP):
            if(defs.trapcomps.size()>subtype && defs.trapcomps[subtype])
            label = defs.trapcomps[subtype]->name_plural;
            break;
        case (df::item_type::PET):
            label = "pet";
            break;
        case (df::item_type::TOOL):
            if(defs.tools.size()>subtype && defs.tools[subtype])
            label = defs.tools[subtype]->name_plural;
            break;
        //~ case (df::item_type::INSTRUMENT):
            //~ if(defs.instruments[subtype])
            //~ label = defs.instruments[subtype]->name_plural;
            //~ break;
        case (df::item_type::ARMOR):
            if(defs.armor.size()>subtype && defs.armor[subtype])
            label = defs.armor[subtype]->name_plural;
            break;
        case (df::item_type::AMMO):
            if(defs.ammo.size()>subtype && defs.ammo[subtype])
            label = defs.ammo[subtype]->name_plural;
            break;
        //~ case (df::item_type::SIEGEAMMO):
            //~ if(defs.siege_ammo[subtype])
            //~ label = defs.siege_ammo[subtype]->name_plural;
            //~ break;
        case (df::item_type::GLOVES):
            if(defs.gloves.size()>subtype && defs.gloves[subtype])
            label = defs.gloves[subtype]->name_plural;
            break;
        case (df::item_type::SHOES):
            if(defs.shoes.size()>subtype && defs.shoes[subtype])
            label = defs.shoes[subtype]->name_plural;
            break;
        case (df::item_type::SHIELD):
            if(defs.shields.size()>subtype && defs.shields[subtype])
            label = defs.shields[subtype]->name_plural;
            break;
        case (df::item_type::HELM):
            if(defs.helms.size()>subtype && defs.helms[subtype])
            label = defs.helms[subtype]->name_plural;
            break;
        case (df::item_type::PANTS):
            if(defs.pants.size()>subtype && defs.pants[subtype])
            label = defs.pants[subtype]->name_plural;
            break;

        default:
            break;
        }
    }else{
        switch (item_type)
        {
            case (df::item_type::DOOR):
            label = "doors";
            break;
        case (df::item_type::CHAIR):
            label = "thrones";
            break;
        case (df::item_type::TABLE):
            label = "tables";
            break;
        case (df::item_type::BED):
            label = "beds";
            break;
        case (df::item_type::CHAIN):
            label = "chains";
            break;
        case (df::item_type::WINDOW):
            label = "windows";
            break;
        case (df::item_type::CAGE):
            label = "cages";
            break;
        case (df::item_type::BARREL):
            label = "barrels";
            break;
        case (df::item_type::BUCKET):
            label = "buckets";
            break;
        case (df::item_type::COFFIN):
            label = "coffins";
            break;
        case (df::item_type::STATUE):
            label = "statues";
            break;
        case (df::item_type::ARMORSTAND):
            label = "armor stands";
            break;
        case (df::item_type::WEAPONRACK):
            label = "weapon racks";
            break;
        case (df::item_type::COIN):
            label = "coins";
            break;
        case (df::item_type::SLAB):
            label = "slabs";
            break;
        case (df::item_type::BOOK):
            label = "book";
            break;
        case (df::item_type::CABINET):
            label = "cabinets";
            break;
        case (df::item_type::BIN):
            label = "bins";
            break;
        case (df::item_type::ANVIL):
            label = "anvils";
            break;
        case (df::item_type::TRAPPARTS):
            label = "mechanisms";
            break;
        case (df::item_type::ROUGH):
            label = "gems";
            break;
        case (df::item_type::SMALLGEM):
             label = "small gems";
             break;
        case (df::item_type::GEM):
             label = "large gems";
             break;
        case (df::item_type::CROWN):
             label = "crowns";
             break;
        case (df::item_type::RING):
             label = "rings";
             break;
        case (df::item_type::EARRING):
             label = "earrings";
             break;
        case (df::item_type::BRACELET):
             label = "bracelets";
             break;
        case (df::item_type::PET):
            label = "pets";
            break;
        default:
            break;
        }
    }
    return label;
}

const char * const adverb[] = {
"Incredibly ","Extremely ","Really ","Rather ","A bit "
};

const char * const Regardnom[] = {
 "Law"
,"Loyal"
,"Family"
,"Friendship"
,"Power"
,"Truth"
,"Cunning"
,"Eloquence"
,"Equity"
,"Decorum"
,"Tradition"
,"Art"
,"Accord"
,"Freedom"
,"Stoicism"
,"SelfExam"
,"SelfCtrl"
,"Quiet"
,"Harmony"
,"Mirth"
,"Craftwork"
,"Combat"
,"Skill"
,"Labour"
,"Sacrifice"
,"Rivalry"
,"Grit"
,"Leisure"
,"Commerce"
,"Romance"
,"Nature"
,"Peace"
,"Lore"
};

const char * const dreamnom[] = {

 "Surviving"
,"Status"
,"Family"
,"Power"
,"Artwork"
,"Craftwork"
,"Peace"
,"Combat"
,"Skill"
,"Romance"
,"Voyages"
,"Immortality"
};

const char * const traitnom[] = {
     "adoring"   ,"aloof"      //LOVE_PROPENSITY
    ,"hostile"   ,"cool"       //HATE_PROPENSITY
    ,"envious"   ,"unenvious"  //ENVY_PROPENSITY
    ,"cheerful"  ,"grumpy"     //CHEER_PROPENSIT
    ,"depressive","resilient"  //DEPRESSION_PROP
    ,"irritable" ,"calm"       //ANGER_PROPENSIT
    ,"anxious"   ,"assured"    //ANXIETY_PROPENS
    ,"salacious" ,"chaste"     //LUST_PROPENSITY
    ,"brittle"   ,"robust"     //STRESS_VULNERAB
    ,"greedy"    ,"generous"   //GREED
    ,"impetuous" ,"measured"   //IMMODERATION
    ,"fierce"    ,"peaceful"   //VIOLENT
    ,"resolute"  ,"yielding"   //PERSEVERENCE
    ,"wasteful"  ,"frugal"     //WASTEFULNESS
    ,"boisterous","cordial"    //DISCORD
    ,"friendly"  ,"quarrelsome"//FRIENDLINESS
    ,"polite"    ,"rude"       //POLITENESS
    ,"obstinate" ,"receptive"  //DISDAIN_ADVICE
    ,"brave"     ,"cowardly"   //BRAVERY
    ,"confident" ,"unsure"     //CONFIDENCE
    ,"vain"      ,"humble"     //VANITY
    ,"driven"    ,"recumbent"  //AMBITION
    ,"grateful"  ,"ungrateful" //GRATITUDE
    ,"pompous"   ,"austere"    //IMMODESTY
    ,"funny"     ,"dour"       //HUMOR
    ,"vengeful"  ,"forgiving"  //VENGEFUL
    ,"proud"     ,"modest"     //PRIDE
    ,"cruel"     ,"kind"       //CRUELTY
    ,"adamant"   ,"inattentive"//SINGLEMINDED
    ,"optimistic","pessimistic"//HOPEFUL
    ,"inquisitive","incurious" //CURIOUS
    ,"bashful"   ,"brazen"     //BASHFUL
    ,"indiscreet","secretive"  //PRIVACY
    ,"fastidious","sloppy"     //PERFECTIONIST
    ,"stubborn"  ,"fickle"     //CLOSEMINDED
    ,"inclusive" ,"insular"    //TOLERANT
    ,"doting"    ,"independent"//EMOTIONALLY_OBS
    ,"impulsive" ,"impassive"  //SWAYED_BY_EMOTI
    ,"helpful"   ,"selfish"    //ALTRUISM
    ,"dutiful"   ,"unruly"     //DUTIFULNESS
    ,"rash"      ,"tentative"  //THOUGHTLESSNESS
    ,"tidy"      ,"messy"      //ORDERLINESS
    ,"trusting"  ,"cynical"    //TRUST
    ,"gregarious","solitary"   //GREGARIOUSNESS
    ,"assertive" ,"passive"    //ASSERTIVENESS
    ,"active"    ,"leisurely"  //ACTIVITY_LEVEL
    ,"intrepid"  ,"reticent"   //EXCITEMENT_SEEK
    ,"fanciful"  ,"serious"    //IMAGINATION
    ,"contemplative","practical"//ABSTRACT_INCLIN
    ,"artistic"  ,"inartistic" //ART_INCLINED
};

string get_name_string(df::language_name &name){
    string ret = Translation::capitalize(name.first_name)+" ";
    for (int i = 0; i < 2; i++)
    {
        if (name.words[i] >= 0)
            ret += *world->raws.language.translations[name.language]->words[name.words[i]];
    }
    return Translation::capitalize(ret);
}

void setDescriptions(UnitInfo * uin)
{

uin->dream="";
uin->traits="";
uin->likesline="";
uin->regards="";
uin->tagline=" ";
uin->godline="";

auto unit = uin->unit;
if(!&unit->status){ return; }

int uid=uin->unit->id;
int figid = unit->hist_figure_id;

int mother=0,father=0,spouse=0,sibling=0,child=0,lover=0,prisoner=0,inprison=0,master=0,apprentice=0,pets=0
,companion=0,fmaster=0,fapp=0,petown=0,heros=0, stars=0, friends=0, aquaints=0, comrades=0, lsoldiers=0, grudges=0,bullies=0,foes=0;

string gods;

int kids=0, rels=0, kin=0;
int momfid=0, dadfid=0;
int momuid=-2,daduid=-2;

string kills, books, momname;

if(figid!=-1){
    
    df::historical_figure *figure;
    if(Units::getNemesis(unit)) figure = Units::getNemesis(unit)->figure;
    
    //if(figure->info->kills.size()) kills =" kll"+to_string(figure->info->kills);
    //if(figure->info->books) books =" bks"+to_string(figure->info->books);
    
    //these are histfig relation enum now.. rewrite later..
    enum class rattitude {
       aqua=1
      ,frie=2
      ,star=4
      ,loya=8
      ,comr=16
      ,hero=32
      ,grud=64
      ,bull=128
      ,foe =256
    };
    
    rels=(figure->info->relationships->list).size();
    
    if(figure->info->relationships)
    for (int nk = 0; nk < (figure->info->relationships->list).size(); nk++)
    {
        int relatq=0;
        if((figure->info->relationships->list[nk]->attitude).size()==0)
            relatq |= (int)rattitude::aqua;
    
        for(int x=0;x<(figure->info->relationships->list[nk]->attitude).size();x++)
        {
            switch(figure->info->relationships->list[nk]->attitude[x]) //(attitude was anon_3)
            {
            case  0:relatq |= (int)rattitude::hero;  break;
            case  1:relatq |= (int)rattitude::frie;  break;
            case  2:relatq |= (int)rattitude::grud;  break;
            //case  3:relatq |= (int)rattitude::hero; break;//god?
            case  4:relatq |= (int)rattitude::grud; break;
            case  5:relatq |= (int)rattitude::foe;  break;
            case  6:relatq |= (int)rattitude::frie; break;
            case  7:relatq |= (int)rattitude::aqua; break;
            case  8:relatq |= (int)rattitude::grud; break;
            case  9:relatq |= (int)rattitude::foe;  break;
            case 10:relatq |= (int)rattitude::comr; break;
    
            case 14:relatq |= (int)rattitude::aqua; break;
            case 15:relatq |= (int)rattitude::bull; break;
            case 16:relatq |= (int)rattitude::foe;  break;
            case 17:relatq |= (int)rattitude::loya; break;
            case 18:relatq |= (int)rattitude::foe;  break;
            case 19:relatq |= (int)rattitude::star; break;
            case 20:relatq |= (int)rattitude::star; break;
            case 21:relatq |= (int)rattitude::star; break;
            case 22:relatq |= (int)rattitude::star; break;
            case 23:relatq |= (int)rattitude::hero; break;
    
            default: break;
            }
        }
    
        //mil0:7 (sub:com) gru0:0 (grudge bully) sup0:0 (fan:hero)
    
        if(relatq&4 && relatq<128){ stars++; }
    
        if( relatq&256 ){ foes++;}
        else if( relatq &128) { bullies++;  }
        else if( relatq & 64) { grudges++;  }
        else if( relatq & 32) { heros++;    }
        else if( relatq & 16) { comrades++; }
        else if( relatq &  8) { lsoldiers++;}
        else if( relatq &  2) { friends++;  }
        else if( relatq &  1) { aquaints++; }
    }
    
    for (int k = 0; k < figure->histfig_links.size(); k++){
    
        auto &nk=figure->histfig_links[k];
        switch(nk->getType())
        {
        case df::histfig_hf_link_type::MOTHER:
            momfid=nk->target_hf; break ;
        case df::histfig_hf_link_type::FATHER:
            dadfid=nk->target_hf; break ;
        case df::histfig_hf_link_type::SPOUSE: spouse++; break ;
        case df::histfig_hf_link_type::CHILD:  child++; break ;
        case df::histfig_hf_link_type::DEITY:
        {
            auto deity = df::historical_figure::find(nk->target_hf);
            if(gods.size()) gods+=",";
            gods+=deity->name.first_name;
            if(nk->link_strength>5000) gods+="+";  //!tune these
            if(nk->link_strength>10000) gods+="+";
            if(nk->link_strength>20000) gods+="+";
        }
            break;
    
        case df::histfig_hf_link_type::LOVER:     lover++; break ;
        case df::histfig_hf_link_type::PRISONER:  prisoner++; break ;
        case df::histfig_hf_link_type::IMPRISONER:inprison++; break ;
        case df::histfig_hf_link_type::MASTER:    master++; break ;
        case df::histfig_hf_link_type::APPRENTICE:apprentice++; break ;
        case df::histfig_hf_link_type::COMPANION: companion++; break ;
        case df::histfig_hf_link_type::FORMER_MASTER:fmaster++; break ;
        case df::histfig_hf_link_type::FORMER_APPRENTICE:fapp++; break ;
        case df::histfig_hf_link_type::PET_OWNER: petown=nk->target_hf; break ;
        }//switch
        
    }//for it had relations
    
    df::historical_figure *momfigure = df::historical_figure::find(momfid);
    
    if(momfigure){ 
        for (int k = 0; k < momfigure->histfig_links.size(); k++){
    
            auto &nk=momfigure->histfig_links[k];
            switch(nk->getType())
            {
            case df::histfig_hf_link_type::CHILD:  kin++; break ;
            }
        }
    }else{
        kin=-1; //unit is motherless ! 
    }

}///if had figure id

// count minor children, lover and sibling

momuid = unit->relationship_ids[df::unit_relationship_type::Mother];
daduid = unit->relationship_ids[df::unit_relationship_type::Father];

if(momuid<1) momuid=-2;
if(daduid<1) daduid=-2;

for (auto cu = world->units.all.begin(); cu != world->units.all.end(); cu++)
{
    if (Units::isKilled(*cu)){ continue; }

    //this unit is mom or dad of world unit
    if(uid==(*cu)->relationship_ids[df::unit_relationship_type::Pet]) pets++;

    if(uid==(*cu)->relationship_ids[df::unit_relationship_type::Mother]
    || uid==(*cu)->relationship_ids[df::unit_relationship_type::Father])
    {
        if((*cu)->profession == df::profession::CHILD 
        || (*cu)->profession == df::profession::BABY)
        {
            kids++;
        }
    }
}

//for (auto hf = world->history.figures.begin(); hf !=  world->history.figures.end(); hf++){}

if(kin>99) kin=99;

string hard,cave,outdoors;

if(&uin->unit->status){
if(&uin->unit->status.misc_traits)
for (int r = 0; r < unit->status.misc_traits.size(); r++)
{
    auto *tr = unit->status.misc_traits[r];

    if (tr->id == misc_trait_type::anon_2){ //Hardened
        if(tr->value>200000){ hard=" Hrd++"; }
        else if(tr->value>125000){ hard=" Hrd+"; }
        else if(tr->value>75000){  hard=" Hrd"; }
    }
    else if (tr->id == df::misc_trait_type::CaveAdapt){
        if(tr->value>750000){ cave =" Cav++"; }
        else if(tr->value>500000){ cave =" Cav+"; }
        else if(tr->value>250000){ cave =" Cav"; }
    }
}
}


int outy =0;
if(&uin->unit->status.current_soul->personality)
    outy =uin->unit->status.current_soul->personality.unk_v4019_1; 
//outy =uin->unit->status.current_soul->personality.likes_outdoors;

if(outy==1){ outdoors=" Rgh"; }
else if(outy==2){ outdoors=" Rgh+"; }

//Fam 0:0 Frn 0:0 Foe Fan wif lvr pet cav++ hrd++ kil1 loy1 boo1 mas1 app1 gru1 pri-1

string cstr="";

int dds=0;

if(spouse){ dds++; }
if(companion){ dds++; }
if(pets){ dds++; }
if(grudges+bullies+foes){ dds+=2; }
if(heros+stars){ dds+=2; }
if(master+apprentice){ dds+=2; }
if(unit->military.squad_id > -1){ dds+=8; }
if(hard.size()){ dds+=1; }
if(cave.size()){ dds+=1; }
if(outdoors.size()){ dds+=1; }

if(petown){
   dds+=6;
    auto pto = df::historical_figure::find(petown);
    if(pto) cstr+="Pet of "+get_name_string(pto->name)+" ";
}

string siblings=to_string(kin);
if(kin<0){ siblings="-"; }

if(dds>5){
cstr+="Fam"+to_string(kids)+":"+siblings
    +",Frd"+to_string(friends)+":"+to_string(aquaints);
}else{
cstr+="Family"+to_string(kids)+":"+to_string(kin)
    +",Friends"+to_string(friends)+":"+to_string(aquaints); dds+=6;
}
if(spouse){
  if(uin->unit && uin->unit->sex){ cstr+=",wif"; }
  else{ cstr+=",hus"; }
}

if(companion) { cstr+=",cmp"; }
if(lover) { cstr+=",lvr"; }
if(pets) { cstr+=",pet"; }

cstr+=cave;
cstr+=outdoors;
cstr+=hard;

if(unit->military.squad_id > -1){

  string sqd=uin->squad_effective_name;
  int mxln=30-dds; if(mxln<6) mxln=6;
  if(sqd.length()>mxln) sqd.resize(mxln);
  cstr+="  "+sqd+":";
  if(unit->military.squad_position==0){
    cstr+="cpt";
  }else{
    cstr+=to_string(unit->military.squad_position);
  }
  if((lsoldiers+comrades)>0){
     cstr+=":s"+to_string(lsoldiers)+":c"+to_string(comrades);
  }
  cstr+=" ";
}

//cstr+=kills;

if(grudges+bullies+foes){
  cstr+=" Foe"+to_string(bullies)
      + ":"+to_string(foes)
      + ":"+to_string(grudges);
}

if(heros+stars){
  cstr+=" Fan"+to_string(heros)
      + ":"+to_string(stars);
}

if(master+apprentice){
  cstr+=" Civ"+to_string(master)
      + ":"+to_string(apprentice);
}

cstr+=books+" ";

int max_len    = dimex-4;
if(max_len>86) max_len=(85+max_len)/2-1;
//cludgy max sizing here

uin->tagline=cstr;
uin->godline=gods;
cstr="";

if(!&unit->status){ return; }
if(!&unit->status.current_soul){ return; }
if(!&unit->status.current_soul->personality){ return; }
if(!&unit->status.current_soul->personality.dreams){ return; }

auto *personality = &uin->unit->status.current_soul->personality;
//Dreams of..
//Likes Dreams (outwork) and objects
cstr="";

if(&personality->dreams){
   int cn = personality->dreams.size()-1;
   int dr = -1;
   if(cn>-1) dr=personality->dreams[cn]->type; //?? will work?
   if(dr>-1&&dr<11)
       cstr+=dreamnom[dr];
   else
       cstr+="a mystery";
}else{
   cstr+="a mystery";
}

uin->dream=cstr;

cstr="";
string dstr="";
string estr="";

int pw,e,c,n;
bool firstgo= true;

if(&unit->status.current_soul->preferences){
auto prefs = uin->unit->status.current_soul->preferences;

n=prefs.size();

for (c=0;c<n;c++)
{
    if(!(prefs[c]->active)) continue;
    int t=prefs[c]->type;
    int it=prefs[c]->item_type;
    int sit=prefs[c]->item_subtype;
    int mit=prefs[c]->mattype;
    int mix=prefs[c]->matindex;

    if(t==1&&it==170){
        dstr="dogs";
    }else if((t==4||t==8)&&it>-1){
        if(t==8){
            dstr="gem";
            if(it==33||it==18||it==22) dstr+="s";
        }else dstr = getItemLabel(it,sit); //subtype is optional

    }else if(t==0&&mit==0&&it==-1&&sit==-1){
        //is a raw mat like metal or rock
            MaterialInfo mi(mit, mix);
        string ds = mi.toString();

        if(ds =="platinum" ||ds =="gold" ||ds =="silver" ||ds =="steel"
         ||ds =="billon" ||ds =="electrum" ||ds =="bronze"
         ||ds =="iron" ||ds =="copper" ||ds =="aluminum" ||ds=="pig iron"
         ||ds =="brass" ||ds =="tin" ||ds =="rose gold"
         ||ds =="zinc" ||ds =="nickel" ||ds =="lead"
         ||ds =="obsidian" ||ds =="adamantine"
         ||ds =="limestone" ||ds =="bismuth bronze"
         ||ds =="dolomite" ||ds =="bone"
         ||ds =="chalk" ||ds =="marble" ||ds =="wood"
         ||ds =="leather" ||ds =="gems"
        ){ dstr=ds; }
        else{
            if(estr.size()+ds.size()<15) estr+=ds+",";
        }
    }
    if(dstr.size()){ cstr+=dstr+","; dstr="";}
}

if (cstr.size()+estr.size()<20){
  cstr+=estr;
}
if (cstr.size()>1){
  cstr.resize(cstr.size()-1);
}

uin->likesline = cstr+".";

}///preferences

int regard_len = 75; //20+max_len/2 ;
int traits_len  = 110; //- 9-gods.size(); //
if(traits_len>85) traits_len=85+((traits_len-85)*2)/3;
cstr="";

//Regarded stuff..

if(&unit->status.current_soul->personality.values){

std::array<uint8_t,250> cachptr;
std::array<int,250> cachval;

auto &values = personality->values;

n=values.size();

for(c=0;c<n;c++){
    cachptr[c]=c;
    cachval[c]=(int)(values[c]->strength);
}

std::sort(cachptr.begin(), cachptr.begin()+n, [&cachval](int a, int  b){
    return abs(cachval[a]) > abs(cachval[b]);
});

for(e=0;e<n;e++) if(abs(cachval[cachptr[e]])<16) break;

const char * const psve[] = {"++++ ","+++ ","++ ","+ "};
const char * const ngve[] = {"---- ","--- ","-- ","- "};

for (c=0;c<n;c++)
{
    int x=cachptr[c];
    pw=abs(cachval[x]);

    if(pw<14) break;
    //    ++++     +++      ++    +
    pw=pw>46?0: pw>35?1: pw>24?2: 3; //++++>+++>++>+

    dstr=Regardnom[values[x]->type];

    if(cachval[x]>0){
        dstr+=psve[pw];
    }else{
        dstr+=ngve[pw];
    }

    if(cstr.size()+dstr.size()<regard_len){
        cstr+=dstr;
    }else{
        if(c<e) cstr+=".. ";
    }
    if(cstr.size()+6>regard_len){
        break;
    }
}

uin->regards = cstr;
}///rwgards

cstr="";
dstr="";
if(&unit->status.current_soul->personality.traits){
//Traits..
std::array<uint8_t,50> cachptr;
std::array<int,50> cachval;

for(c=0;c<50;c++){
    cachval[c]=(int)personality->traits[c];
    cachptr[c]=c;
}

std::sort(cachptr.begin(), cachptr.end(), [&cachval](int a, int  b){
    return abs(cachval[a]-50) > abs(cachval[b]-50);
});

//find end of must note
for(e=0;e<50;e++) if(abs(cachval[cachptr[e]]-50)<6) break;

bool punc = false;
bool tinu = true;
int adv = -1;
int firstgo=true;
c=0; cstr=""; dstr="";
int abit=0;
int wrds=0;

while(tinu){
    dstr="";
    int tx=cachptr[c++];
    pw=abs(cachval[tx]-50);

    int lowest=12;
    if(pw<=lowest) break; //crash without this
    //incredibly extremely really  rather : a bit
    pw= pw>45?0: pw>36?1: pw>24?2: pw>18?3: 4; 
    //pw=pw>lowest?4:pw; //a bit

    //game cat is
    //most  41 - 50
    //much  24 - 40
    //often 10 - 24

    if(pw==4){
      if(abit++==0) adv=3;
      if(abit>1&&(wrds>4||abs(cachval[tx]-50)<8)) break;
    }

    if(pw>adv){
      if(punc) dstr=". ";
      adv=pw;
      dstr+=adverb[adv];
      //if(firstgo&&pw==2) dstr+="Quite ";
      punc=false;
      firstgo=false;
    }

    if(punc) dstr+=",";
    punc=true;

    dstr+=traitnom[ tx*2 + ((cachval[tx]>50)?0:1) ];

    if(pw==4&&wrds>3&&dstr.size()>13){ //cancel a bit "bigword"
        abit--; //get small final word.
        dstr="";
    }

    if(cstr.size()+dstr.size()<traits_len){
        cstr+=dstr; wrds++;
    }

    if((pw>2&&cstr.size()>(traits_len*2+76)/3)||cstr.size()>traits_len){ //wont fit another in
        //if(c<e) cstr+="..";
        tinu=false;
    }

    if(c>=e)tinu=false;
}

uin->traits=cstr+". ";
}/// Extremely gregarious...

}

void setDistraction(UnitInfo * uin){
    int fo = uin->unit->status.current_soul->personality.current_focus - uin->unit->status.current_soul->personality.undistracted_focus;
    int pol=fo<0?-1:1;

    fo=static_cast<int>(std::sqrt((double)(fo*pol*25))/4+0.5);
    fo=fo>9?9:fo;
    uin->focus=fo*pol;
}
/*
void setDistraction(UnitInfo * uin){
    
    if(!&uin->unit->status.current_soul){ uin->focus=0; return;}
    auto needs = uin->unit->status.current_soul->personality.needs;
    
    int fo=0;
    for (int c=0;c<needs.size();c++)
    { fo+=needs[c]->focus_level*needs[c]->need_level; }
    fo+=33333;
    int pol=fo<0?-1:1;

    fo=static_cast<int>(std::sqrt((double)(fo*pol/333))/9);
    fo=fo>9?9:fo;
    uin->focus=fo*pol;
}
*/

}//namespace unit_info_ops


static bool theme_reload = true;

bool loadPallete()
{
    theme_reload = false;

    /*cerr << "Attempt to load dfkeeper pallete" << file << endl; */
    std::ifstream infile(Filesystem::getcwd() + "/" + CONFIG_DIR + "/dfkeeper_pallete.txt" );
    if (infile.bad()) {
        return false;
    }

    show_sprites = false;
    string line;
    int clr = 0;
    int k = 0;

    while (std::getline(infile, line)) {

        if (strcmp(line.substr(0,7).c_str(),"RELOADS")==0)
            theme_reload = true;

        if (strcmp(line.substr(0,7).c_str(),"SHOWSPR")==0)
            show_sprites = true;

        if (strcmp(line.substr(0,1).c_str(),"/")==0)
            continue;

        size_t pos = line.find(',');

        while( pos != string::npos ){
            clr = std::stoi( line.substr(pos + 1, 2).c_str() );
            pos = line.find(',',pos+1);
            if(k<35)
                cltheme[k++] = clr;//&15;
        }
    }

    return true;
}

/*
// File for custom dfkeeper highlights
// save in [df_root]/dfkeeper/dfkeeper_pallete.txt

// numbers for colors:
// BLACK  = 0  BLUE      = 1  GREEN   = 2  CYAN   = 3
// RED    = 4  MAGENTA   = 5  BROWN   = 6  GREY   = 7
// DKGREY = 8  LTBLUE    = 9  LTGREEN =10  LTCYAN =11
// LTRED  =12  LTMAGENTA =13  YELLOW  =14  WHITE  =15

// following colors prefixed with commas are read in order

//               noskill  low   med  hai
BG for not set:     ,0    ,0    ,0   ,0
FG for not set:     ,8    ,14   ,7   ,11
cursor BG not set   ,15   ,15   ,15  ,15
cursor FG not set   ,0    ,0    ,0   ,0
BG set              ,8    ,14   ,7   ,11
FG set              ,7    ,15   ,15  ,15
cursor BG set       ,15   ,15   ,15  ,15
cursor FG set       ,15   ,15   ,15  ,15

BG military ,13
FG other    ,13
BG posflash ,1

//RELOADS - uncomment this line to
//have theme reload on every relist

//SHOWSPRITES - uncomment to clutter X axis with sprites
*/

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
    string get_unit_ax(UnitInfo *u)
    { return itos(u->active_index); }
    string get_unit_xx(UnitInfo *u)
    { return to_string(u->medic)+","
            +to_string(u->civil)+","
            +to_string(u->scholar)+","
            +to_string(u->performer)+","
            +to_string(u->martial)+","; }
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



class viewscreen_popHelpst : public dfhack_viewscreen {
public:

    viewscreen_popHelpst(){ }

    std::string getFocusString() { return "unitkeeper/helpscreen"; }

    void feed(set<df::interface_key> *events)
    {
        if (events->count(interface_key::LEAVESCREEN))
        {
            Screen::dismiss(this);
            return;
        }
    }
    void render()
    {
        dfhack_viewscreen::render();
        Screen::clear();
        int x = 2, y = 2;

x=2;y=2;
const int Qa[]={1,4,1,4,3,2,4,1,1,4,2,0,1,1,0,1,0,3,4};
const int Qc[]={COLOR_LIGHTRED,COLOR_YELLOW,COLOR_GREEN,COLOR_LIGHTBLUE,COLOR_WHITE};
string Wa="SaterdAfwcipmlsmkes";
string Wb="tgoneinoirnaeipuimo";
string Wc="riuecsaclettmnasnpc";
string Wd="elgroelulauiogtieai";
string We="nihguaysptieruicsta";
string Wf="gtnypss oitnyiaathl";
string Wg="tye eei wvic sllhyA";
string Wh="  s rRs eioe tS e w";
string Wi="  s ae  rtn  ie t a";
string Wj="    ts   y   cn i r";
string Wk="    ii        s c e";
string Wl="    os        e    ";
string Wm="    mt             ";
string Wn="                   ";

int len=19;

OutputString(COLOR_WHITE,x,y,"[]");
OutputString(COLOR_LIGHTGREEN,x,y,":zoom text");
OutputString(COLOR_LIGHTMAGENTA,x,y,"               Cavern Keeper Help               ");
OutputString(COLOR_WHITE,x,y,Screen::getKeyDisplay(interface_key::LEAVESCREEN));
OutputString(COLOR_LIGHTGREEN,x,y,":return"); x=2;y+=2;
OutputString(COLOR_YELLOW,x,y,"Attributes Legend  ");

OutputString(COLOR_GREY,x,y," Most Labors employ and improve certain Attributes."); x=2;y++;
OutputString(COLOR_YELLOW,x,y,"                   ");
OutputString(COLOR_GREY,x,y,"  Attribute mode prints them in a grid."); x=2;y++;

for(int c=0;c<len;c++){ OutputString(Qc[Qa[c]],x,y,Wa.substr(c,1)); };
OutputString(COLOR_RED,x,y," <-");
OutputString(COLOR_GREY,x,y,"First two letters make the grids header."); x=2;y++;

for(int c=0;c<len;c++){ OutputString(Qc[Qa[c]],x,y,Wb.substr(c,1)); };x+=2;
OutputString(COLOR_LIGHTRED,x,y,"  Units weak attributes are highlighted Red."); x=2;y++;

for(int c=0;c<len;c++){ OutputString(Qc[Qa[c]],x,y,Wc.substr(c,1)); };x+=2;
OutputString(COLOR_LIGHTBLUE,x,y,"  Units strong attributes are highlighted Blue."); x=2;y++;

for(int c=0;c<len;c++){ OutputString(Qc[Qa[c]],x,y,Wd.substr(c,1)); };x+=2;
OutputString(COLOR_WHITE,x,y,"  White attribs are exercised by the labor."); x=2;y++;

for(int c=0;c<len;c++){ OutputString(Qc[Qa[c]],x,y,We.substr(c,1)); };x+=2;
OutputString(COLOR_LIGHTGREEN,x,y,"  Green attribs may contribute to the labor."); x=2;y++;

for(int c=0;c<len;c++){ OutputString(Qc[Qa[c]],x,y,Wf.substr(c,1)); };y++;x=2;
for(int c=0;c<len;c++){ OutputString(Qc[Qa[c]],x,y,Wg.substr(c,1)); };x+=2;
OutputString(COLOR_GREY,x,y,"Since there is so much Attribute detail,"); x=2;y++;

for(int c=0;c<len;c++){ OutputString(Qc[Qa[c]],x,y,Wh.substr(c,1)); };x+=2;
OutputString(COLOR_GREY,x,y,"Keeper calculates dwarves aptitude for each labor."); x=2;y++;

for(int c=0;c<len;c++){ OutputString(Qc[Qa[c]],x,y,Wi.substr(c,1)); };x+=2;
OutputString(COLOR_GREY,x,y,"Labors are colored to show good & bad matches"); x=2;y++;

for(int c=0;c<len;c++){ OutputString(Qc[Qa[c]],x,y,Wj.substr(c,1)); };x+=2;
OutputString(COLOR_GREY,x,y,"(to help while availing dwarves to work)."); x=2;y++;

for(int c=0;c<len;c++){ OutputString(Qc[Qa[c]],x,y,Wk.substr(c,1)); };x+=2;
OutputString(COLOR_GREY,x,y,"Labor grid highlight colors are:"); x=2;y++;
for(int c=0;c<len;c++){ OutputString(Qc[Qa[c]],x,y,Wl.substr(c,1)); };x+=2;
OutputString(COLOR_YELLOW,x,y,"  Brass (poor)");
OutputString(COLOR_GREY,x,y,"  Iron (medium)  ");
OutputString(COLOR_LIGHTCYAN,x,y,"Adamantine (good)"); x=2;y++;


for(int c=0;c<len;c++){ OutputString(Qc[Qa[c]],x,y,Wm.substr(c,1)); };x+=2;
OutputString(COLOR_GREY,x,y,"or set to red,grey,green by press 'c'"); x=2;y+=2;

OutputString(COLOR_GREY,x,y,"[ Aptitude score is printed next to skill and experience scores,"); x=2;y++;

OutputString(COLOR_GREY,x,y,"  at the end of the current units line ->... xp");
OutputString(COLOR_LIGHTCYAN,x,y,"50 ");
OutputString(COLOR_GREY,x,y,"lv");
OutputString(COLOR_LIGHTCYAN,x,y,"5 ");
OutputString(COLOR_GREY,x,y,"ap");
OutputString(COLOR_LIGHTCYAN,x,y,"50");
OutputString(COLOR_GREY,x,y,"       ]"); x=2; y++;y++;

OutputString(COLOR_YELLOW,x,y,"Labor selection grid:"); x=2;y++;
OutputString(COLOR_GREY,x,y,"  Units can be sorted by skill for a certain job by clicking"); x=2;y++;

OutputString(COLOR_GREY,x,y,"  on the job in the axis, or by using the sort mode selection keys."); x=2;y++;

OutputString(COLOR_YELLOW,x,y,"Modes:"); x=2;y++;
OutputString(COLOR_GREY,x,y,"  Highlighting and detail level can be set with the mode keys."); x=2;y++;

OutputString(COLOR_YELLOW,x,y,"Summary Lines:"); x=2;y++;

OutputString(COLOR_LIGHTBLUE,x,y,"  The info in DFs creature view page is packed into three lines:"); x=2;y++;
OutputString(COLOR_LIGHTBLUE,x,y,"  1. main character traits are summarized in the top summary line."); x=2;y++;
OutputString(COLOR_LIGHTBLUE,x,y,"  2. Liked items and private concerns (may conflict with traits)."); x=2;y++;
OutputString(COLOR_LIGHTBLUE,x,y,"  3. Goals and some tags. -see the creature view page for meanings."); x=2;y++;

OutputString(COLOR_LIGHTBLUE,x,y,
"  Some sorts (martial, medic etc) analyze the main character traits."); x=2;y++;

OutputString(COLOR_YELLOW,x,y,"Keep column:"); x=2; y++;
OutputString(COLOR_LIGHTBLUE,x,y,
"  A reserve of good memories, negative values risk madness."); x=2;y++;
OutputString(COLOR_LIGHTBLUE,x,y,
"  Keep is measured in milliboons (stress_counter/-1000)"); x=2;y++;

OutputString(COLOR_YELLOW,x,y,"Focus: ");
OutputString(COLOR_GREEN,x,y,"fcs+3"); x=2;y++;
OutputString(COLOR_GREY,x,y,"  This is how well the unit is focusing on tasks."); x=2;y++;

OutputString(COLOR_YELLOW,x,y,"Notices display:"); x=2;y++;
OutputString(COLOR_GREY,x,y,"  A list of health notes like 'is drunk' or 'is werebeast'."); x=2;y++;

OutputString(COLOR_YELLOW,x,y,"Nicknames & professions:"); x=2;y++;
OutputString(COLOR_GREY,x,y,"  Nicknaming is easy and useful to put dwarves into roles and squads."); x=2; y++;
OutputString(COLOR_GREY,x,y,"  Custom professions can also be saved and applied."); x=2;y++;

Screen::drawBorder(" Cavern Keeper ");

    }
protected:

private:
    void resize(int32_t x, int32_t y)
    {
        dfhack_viewscreen::resize(x, y);
    }
};




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
        formatter.add_option("i", "Position in list", unit_ops::get_list_id);
        formatter.add_option("pi", "Position in list, among dwarves with same profession", unit_ops::get_list_id_prof);
        formatter.add_option("gi", "Position in list, among dwarves in same profession group", unit_ops::get_list_id_group);
        formatter.add_option("ag", "Arrival Group", unit_ops::get_arrival);
        formatter.add_option("ri", "Raw unit ID", unit_ops::get_unit_id);
        formatter.add_option("xx", "Raw unit ID", unit_ops::get_unit_xx);
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
    std::string getFocusString() { return "unitkeeper/batch"; }
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
            if (!u || !u->unit) continue;
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
                Screen::dismiss(this);
                return;
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
            Screen::drawBorder("  Dfhack - Batch Operations  ");
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
    std::string getFocusString() { return "unitkeeper/profession"; }
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
        Screen::drawBorder("  Dfhacks Custom Professions ");
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

class viewscreen_unitkeeperst : public dfhack_viewscreen {
public:
    void feed(set<df::interface_key> *events);

    void logic() {
        dfhack_viewscreen::logic();
        if (do_refresh_names)
            refreshNames();
    }

    void sizeDisplay();
    void render();

    void help() { }

    std::string getFocusString() { return "unitkeeper"; }

    df::unit *getSelectedUnit();

    viewscreen_unitkeeperst(vector<df::unit*> &src, int cursor_pos);
    ~viewscreen_unitkeeperst() { };

private:
    vector<UnitInfo *> units;

    void paintFooter(bool canToggle);
    void paintExtraDetail(UnitInfo *cur,string &excess_field_str,int8_t &excess_field_clr);
    void printScripts(UnitInfo *cur);
    void OutputStrings( int c , int& x, int& y, string s);

    bool do_refresh_names;
    int row_hint;
    int col_hint;
    int display_rows;
    int last_selection;

    void resize(int w, int h) { sizeDisplay(); }

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

    void checkScroll();

    bool scrollknock(int * reg, int stickval, int passval);
    bool scroll_knock=false;

    void paintAttributeRow(int row ,UnitInfo *cur, bool header);
    void paintLaborRow(int &row,UnitInfo *cur, df::unit* unit);

    df::unit* findCPsActiveUnit(int cursor_pos);
    int findUnitsListPos(int unit_row);
};

void viewscreen_unitkeeperst::OutputStrings( int c , int& x, int& y, string s){
    if(x>=dimex) return;
    if(x+s.size()>dimex-1){
        s.resize(dimex-x-1);
    }
   OutputString( c, x, y, s);
}

df::unit* viewscreen_unitkeeperst::findCPsActiveUnit(int cursor_pos){

    df::unit *unit;

    if (VIRTUAL_CAST_VAR(unitlist, df::viewscreen_unitlistst, parent)){
        unit = unitlist->units[unitlist->page][cursor_pos];
    }

    return unit;
}

int viewscreen_unitkeeperst::findUnitsListPos(int unit_row){

    int ULcurpos = -1;
    if (VIRTUAL_CAST_VAR(unitlist, df::viewscreen_unitlistst, parent)){
        for (int i = 0; i < unitlist->units[unitlist->page].size(); i++)
        {
            if (unitlist->units[unitlist->page][i] == units[unit_row]->unit){
                ULcurpos=i;
                break;
            }
        }
    }
    return ULcurpos;
}

viewscreen_unitkeeperst::viewscreen_unitkeeperst(vector<df::unit*> &src, int cursor_pos)
{
    if(cursor_pos>0||sel_unitid<0) sel_unitid = src[cursor_pos]->id;

    std::map<df::unit*,int> active_idx;
    auto &active = world->units.active;
    for (size_t i = 0; i < active.size(); i++)
        active_idx[active[i]] = i;

    for (size_t i = 0; i < src.size(); i++)
    {
        df::unit *unit = src[i];

        if (!(unit->status.current_soul))//
        {
            continue;
        }

        UnitInfo *cur = new UnitInfo;

        cur->ids.init();
        cur->unit = unit;
        cur->age = (int)Units::getAge(unit);
        cur->allowEdit = true;
        cur->selected = false;
        cur->active_index = active_idx[unit];

        if (!Units::isOwnCiv(unit))
            cur->allowEdit = false;

        if (!Units::isOwnGroup(unit))
            cur->allowEdit = false;

        if (!Units::isActive(unit))
            cur->allowEdit = false;

        if (unit->flags2.bits.visitor)
            cur->allowEdit = false;

        if (unit->flags3.bits.ghostly)
            cur->allowEdit = false;

        if (!ENUM_ATTR(profession, can_assign_labor, unit->profession))
            cur->allowEdit = false;

        cur->color = Units::getProfessionColor(unit);

        unit_info_ops::setDistraction(cur);

        units.push_back(cur);
    }

    if(cur_world!=World::ReadWorldFolder()){
        cur_world = World::ReadWorldFolder();
        resetModes();
    }

    if(theme_reload)
        loadPallete();

    cheat_level = 0;
    row_hint = 0;
    col_hint = 0;
    refreshNames();
    calcArrivals();

    if(sel_unitid ==-1){
        sel_row_b = sel_row = 0;
        sel_unitid = units[0]->unit->id;
    }else{
        sel_row_b = sel_row = 0;
        for (size_t i = 0; i < units.size(); i++){
            if(sel_unitid == units[i]->unit->id)
                sel_row_b = sel_row = i;
        }
    }

    calcIDs();
    unit_info_ops::calcAptScores(units);
    unit_info_ops::calcNotices(units);

    unstashSelection(units);
    dualSort();

    read_dfkeeper_config(); sizeDisplay();

    while (first_row < sel_row - display_rows + 1)
        first_row += display_rows + 1;
    // make sure the selection stays visible
    if (first_row > sel_row)
        first_row = sel_row - display_rows + 1;
    // don't scroll beyond the end
    if (first_row > units.size() - display_rows)
        first_row = units.size() - display_rows;

    last_selection = -1;

}

void viewscreen_unitkeeperst::calcArrivals()
{
    int bi = -100;  //(b)efore id
    int bai = -100; //(b)efore active_index
    int ci = 0;     //(c)urrent
    int cai = 0;
    int guessed_group = 0;

    std::stable_sort(units.begin(), units.end(), sortByActiveIndex);

    for (size_t i = 0; i < units.size(); i++)
    {
        ci = units[i]->unit->id;
        cai = units[i]->active_index;
        if((abs(ci-bi)>30 && abs(cai-bai)>4)||abs(cai-bai)>10)
            guessed_group++;
        units[i]->arrival_group = guessed_group;
        bi = ci;
        bai = cai;
    }
}

void viewscreen_unitkeeperst::resetModes()
{
    detail_mode = 0;
    first_row = 0;
    sel_row = 0;
    sel_row_b = 0; //selection row (b)efore
    sel_unitid = -1;
    display_rows_b = 0;
    first_column = 0;
    sel_column = sel_column_b = 0;
    sel_attrib = 0;
    column_sort_column = -1;
    widesort_mode = WIDESORT_NONE;
    finesort_mode = FINESORT_NAME;
    selection_changed = false;
    selection_stash.clear();
}

void viewscreen_unitkeeperst::calcIDs()
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

void viewscreen_unitkeeperst::refreshNames()
{
    do_refresh_names = false;
    maxnamesz = 5;

    for (size_t i = 0; i < units.size(); i++)
    {
        UnitInfo *cur = units[i];
        df::unit *unit = cur->unit;

        cur->name = Translation::TranslateName(Units::getVisibleName(unit), false);
        cur->transname = Translation::TranslateName(Units::getVisibleName(unit), true);
        cur->profession = Units::getProfessionName(unit);

        int lz=tran_names<2?cur->name.size():cur->transname.size();
        if(lz==0) lz=cur->profession.size();

        if (maxnamesz < lz) maxnamesz = lz;

        if(cur->name == ""){ //to list animals with no name
            cur->name = cur->profession;
            cur->transname = cur->profession;
            cur->profession = "";
        }

        if(tran_names<2)
            cur->lastname = cur->name.substr(cur->name.find(" ")+1);
        else
            cur->lastname = cur->transname.substr(cur->transname.find(" ")+1);

        if (unit->job.current_job == NULL) {
            df::activity_event *event = Units::getMainSocialEvent(unit);
            if (event) {
                event->getName(unit->id, &cur->job_desc);
                cur->job_mode = UnitInfo::SOCIAL;
            }
            else {
                bool is_on_break = false;
                for (int p = 0; p < unit->status.misc_traits.size(); p++)
                {
                    if (unit->status.misc_traits[p]->id == misc_trait_type::anon_4){
                        is_on_break = true;
                        break;
                    }
                }
                if(is_on_break){
                    cur->job_desc = "On Break";
                }else{
                    cur->job_desc = "Idle";
                }
                cur->job_mode = UnitInfo::IDLE;
            }
        } else {
            cur->job_desc = DFHack::Job::getName(unit->job.current_job);
            cur->job_mode = UnitInfo::JOB;
        }

        if (unit->military.squad_id > -1) {
            if(cur->job_desc == "Idle"){
                if (ENUM_ATTR(profession, military, unit->profession)){
                    cur->job_mode = UnitInfo::MILITARY;
                    cur->job_desc = "On duty";
                }
            }
            cur->squad_effective_name = Units::getSquadName(unit);
            string detail_str = cur->squad_effective_name;
            if(detail_str.substr(0,4)=="The ")
                detail_str=detail_str.substr(4);

            cur->squad_info = stl_sprintf("%i", unit->military.squad_position + 1) + "." + detail_str;
        } else {
            cur->squad_effective_name = "";
            cur->squad_info = "";
        }
    }

    sizeDisplay();
}

void viewscreen_unitkeeperst::calcUnitinfoDemands(){
    for (size_t i = 0; i < units.size(); i++)
    {
        UnitInfo *cur = units[i];
        cur->demand = 0;

        for (size_t j = 0; j < NUM_LABORS; j++)
        {
            df::unit_labor cur_labor = columns[j].labor;
            df::job_skill cur_skill = columns[j].skill;

            int demand = 0;
            if(cur->unit->status.labors[cur_labor]){
                if(cur_skill==df::job_skill::NONE){
                    if(cur_labor==unit_labor::HAUL_STONE
                     ||cur_labor==unit_labor::HAUL_WOOD
                     ||cur_labor==unit_labor::HAUL_ITEM
                     ||cur_labor==unit_labor::HAUL_STONE
                     ||cur_labor==unit_labor::BUILD_CONSTRUCTION){
                        demand = 12;
                    }else{
                        demand = 4;
                    }
                }else{
                    demand = unitSkillRating(cur,cur_skill)*2+1;
                    if(unitSkillExperience(cur,cur_skill) > 0){
                        demand += 2;
                    }
                    demand = static_cast<int>(std::sqrt((demand)*(350+cur->column_aptitudes[j]))/8);
                }
            }
            cur->demand += demand;
        }
    }
}

void viewscreen_unitkeeperst::dualSort()
{
    if(cancel_sort){
        cancel_sort = false;
        return;
    }

    switch (finesort_mode) {
    case FINESORT_COLUMN:
        if(column_sort_column==-1)
            column_sort_column = sel_column;

        column_sort_last = column_sort_column;
        std::stable_sort(units.begin(), units.end(), sortByColumn);

        col_hint = 0;
        break;
    case FINESORT_NOTICES:
        std::stable_sort(units.begin(), units.end(), sortByNotices);
        break;
    case FINESORT_STRESS:
        std::stable_sort(units.begin(), units.end(), sortByStress);
        break;
    case FINESORT_UNFOCUSED:
        std::stable_sort(units.begin(), units.end(), sortByUnfocused);
        break;
    case FINESORT_AGE:
        std::stable_sort(units.begin(), units.end(), sortByAge);
        break;
    case FINESORT_NAME:
        if(tran_names<2)
            std::stable_sort(units.begin(), units.end(), sortByName);
        else
            std::stable_sort(units.begin(), units.end(), sortByTransName);
        break;
    case FINESORT_SURNAME:
        std::stable_sort(units.begin(), units.end(), sortBySurName);
        break;
    case FINESORT_MARTIAL:
        std::stable_sort(units.begin(), units.end(), sortByMartial);
        break;
    case FINESORT_PERFORMER:
        std::stable_sort(units.begin(), units.end(), sortByPerformer);
        break;
    case FINESORT_SCHOLAR:
        std::stable_sort(units.begin(), units.end(), sortByScholar);
        break;
    case FINESORT_MEDIC:
        std::stable_sort(units.begin(), units.end(), sortByMedic);
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
        std::stable_sort(units.begin(), units.end(), sortByEnabled);
        std::stable_sort(units.begin(), units.end(), sortBySquad);
        break;
    case WIDESORT_JOB:
        std::stable_sort(units.begin(), units.end(), sortByJob);
        break;
    case WIDESORT_ARRIVAL:
        std::stable_sort(units.begin(), units.end(), sortByEnabled);
        std::stable_sort(units.begin(), units.end(), sortByArrival);
        break;
    case WIDESORT_OVER:
        break;
    }

    if(finesort_mode != FINESORT_AGE
    && finesort_mode != FINESORT_NOTICES
    && finesort_mode != FINESORT_STRESS
    && finesort_mode != FINESORT_UNFOCUSED
    && widesort_mode != WIDESORT_ARRIVAL
    && widesort_mode != WIDESORT_SQUAD)
    {
      std::stable_sort(units.begin(), units.end(), sortByEnabled);
    }


    //sel_unitid and sel_row and b are yet unchanged after sorting

    //if no sel_unitid just set sel_rows and uid to 0
    if(sel_unitid ==-1){
        sel_row = 0; sel_row_b = -1;
        sel_unitid = units[0]->unit->id;
    }else{
        for (size_t i = 0; i < units.size(); i++){
            if(sel_unitid == units[i]->unit->id)
                sel_row = i;
        }
    }
    //if there was unitid set sel_row_b to its position in new order

    //if widesort changed or sel unit moved and widesort none
    //show top of list
    if( ( sel_row_b!=sel_row && widesort_mode==WIDESORT_NONE) //
        || widesort_mode!=widesort_mode_b){
        widesort_mode_b = widesort_mode;
        first_row = 0;
        //row_hint = 0;
    }

    sel_row_b = sel_row;

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
                toggle_sort_bg = !toggle_sort_bg;
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

void viewscreen_unitkeeperst::sizeDisplay()
{
    auto dim = Screen::getWindowSize();

    dimex = dim.x;
    dimey = dim.y;

    xmargin = 2;
    int xpanwide = dimex-xmargin*2;
    if(xpanwide>86) xpanwide-=2;
    if(xpanwide>86) xpanwide=86;
    xmargin = (dimex-xpanwide)/2;
    xfooter = xpanwide-76;
    xpanend = xmargin+xpanwide;
    xpanover=(dimex*2+xpanend-3)/3;

    display_rows = row_space = dimey-11-(show_details>3?6-show_details:show_details);

    if (display_rows > units.size()) display_rows = units.size();

    int cn_stress   = (dimex<90)? 4 : 5; //budge stress against border if screen small
    int cn_selected = 1;                 //the selection indicators
    int cn_name     = 5;                 //prelim name col width

    int cn_detail   = (dimex/13)*2+7;   //tab detail col eg. professions, attrib grid
    if(cn_detail>23) cn_detail=23;

    int cn_labor    = 25;  //preliminary size of labor grid
    int cn_borders  =  2;  //far left and right border of screen
    int cn_dividers =  4;  //sum of the 4 singal char minimal separation of cols

    int cn_tally = cn_stress+cn_selected+cn_name+cn_detail+cn_labor+cn_borders+cn_dividers;

    int maxname = maxnamesz<30?maxnamesz:31+(maxnamesz-30)/2;

    int maxpart=(21 + (dimex/2-19)) /2;
    int left = dimex-cn_tally;
    if( left>0){
        maxname = maxname>maxpart?maxpart:maxname;
        int increase = maxname-cn_name;
        if(increase>left){
            cn_name+=left;
        }else{
            cn_name+= increase;
            left-= increase;
        }
    }

    int mk = 1; //start after left border
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
    column_size[COLUMN_LABORS]    = dimex - mk - 1;

    if(column_size[COLUMN_LABORS]>NUM_LABORS)
        column_size[COLUMN_LABORS] = NUM_LABORS;

    if (units.size() != 0) checkScroll();
}

void viewscreen_unitkeeperst::checkScroll(){

    int bzone=display_rows/7;
    int bottom_hint = (display_rows<units.size()&&display_rows>16)?1:0;

    if (first_row > units.size()-display_rows+bottom_hint ){
        first_row = units.size()-display_rows+bottom_hint;
    }

    if (first_row < 0)
        first_row = 0;

    if(sel_column!=sel_column_b||sel_row!=sel_row_b||display_rows!=display_rows_b)
    {
        if(first_row==0&&(sel_row_b<first_row||sel_row_b>first_row + display_rows)){
            if(sel_row_b<sel_row){   //issued down
                sel_row = sel_row_b; //just make sel row visible
                first_row = sel_row+bzone+1-display_rows;
                row_hint = 0;
            }

            if(sel_row_b>sel_row){ //issued up
                sel_row = 0;
                sel_row_b = sel_row; //go to
                row_hint = 0;
            }
        }

        //sel_row is pushing down
        if( sel_row+bzone+1 > first_row + display_rows ){ //beyond zone
            if(sel_row>sel_row_b)
                first_row++;
            if(sel_row+1+1 > first_row + display_rows){ //beyond max
                first_row = sel_row + 1+1 - display_rows;
                //row_hint = 0;
            }
        }

        //sel_row is pushing up
        if( sel_row-bzone < first_row ){
            if(sel_row < sel_row_b)
            first_row--;
            if( sel_row-1 < first_row ){
                first_row = sel_row-1;
                //row_hint = 0;
            }
        }

        if (first_row < 0){ first_row = 0; }

        sel_row_b = sel_row;
        display_rows_b = display_rows;
    }

    if (first_row > units.size()-display_rows+bottom_hint ){
        first_row = units.size()-display_rows+bottom_hint;
    }

    if (first_row < 0){ first_row = 0; }

    int row_width = column_size[COLUMN_LABORS];
    bzone = row_width/8 + 1;

    //set first column according to if sel is pushing zone
    if( sel_column-bzone < first_column )
        first_column = sel_column-bzone;

    if( sel_column+bzone >= first_column + row_width )
        first_column = sel_column-row_width+bzone+1;

   if(first_column<0)
       first_column = 0;

    if(first_column>=NUM_LABORS-row_width)
        first_column = NUM_LABORS-row_width;

    sel_column_b = sel_column_b;
}

bool viewscreen_unitkeeperst::scrollknock(int *reg, int stickval, int passval){
    if(!(scroll_knock)){
        scroll_knock = true;
        knock_ts = std::chrono::system_clock::now();
        *reg = stickval;
        return false;
    }else{
        chronow = std::chrono::system_clock::now();
        if ((std::chrono::duration_cast<std::chrono::milliseconds>(chronow-knock_ts)).count()>200){
            *reg = passval;
            scroll_knock = false;
            return true;
        }else{
            knock_ts = chronow;
            *reg = stickval;
            return false;
        }
    }
}

void viewscreen_unitkeeperst::feed(set<df::interface_key> *events)
{
    int8_t modstate = Core::getInstance().getModstate();
    bool leave_all = events->count(interface_key::LEAVESCREEN_ALL);
    if (leave_all || events->count(interface_key::LEAVESCREEN))
    {
        events->clear();
        save_dfkeeper_config();
        Screen::dismiss(this);

        //set unitlist cursor pos
        if (VIRTUAL_CAST_VAR(unitlist, df::viewscreen_unitlistst, parent))
        {
            int ULcurpos = findUnitsListPos(sel_row);

            if(ULcurpos!=-1){
                unitlist->cursor_pos[unitlist->page] = ULcurpos;
            }
        }

        if (leave_all)
        {
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
    int gpsx = -1;
    int gpsy = -1;

    if (enabler->tracking_on && gps->mouse_x != -1 && gps->mouse_y != -1)
    {
        gpsx = gps->mouse_x;
        gpsy = gps->mouse_y;
        int click_header = COLUMN_MAX; // group ID of the column header clicked
        int click_body = COLUMN_MAX; // group ID of the column body clicked

        int click_unit = -1; // Index into units[] (-1 if out of range)
        int click_labor = -1; // Index into columns[] (-1 if out of range)

        const int coltweaka[] = {0,-1,0,0,0,0,0}; //make selection col
        const int coltweakb[] = {0, 1,0,0,0,0,0}; //include its border

        if (gpsy == dimey-3){
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
                //if(finesort_mode_b==-1) 
                
                finesort_mode_b = finesort_mode;
                finesort_mode = FINESORT_COLUMN;
                sel_column = click_labor;
                column_sort_column = -1;
                column_sort_last = -2;
                row_hint = 25;
                col_hint = 25;

                events->insert(interface_key::SECONDSCROLL_UP);
             
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
                row_hint = 0;
            }
            if (enabler->mouse_rbut)
            {
                sel_row = click_unit;
                events->insert(interface_key::UNITJOB_ZOOM_CRE);
                row_hint = 0;
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
    {   sel_row--; row_hint+=15; col_hint+=15; }
    if (events->count(interface_key::CURSOR_UP_FAST) || events->count(interface_key::CURSOR_UPLEFT_FAST) || events->count(interface_key::CURSOR_UPRIGHT_FAST))
        sel_row -= 10;
    if (events->count(interface_key::CURSOR_DOWN) || events->count(interface_key::CURSOR_DOWNLEFT) || events->count(interface_key::CURSOR_DOWNRIGHT))
    {   sel_row++; row_hint+=15; col_hint+=15; }
    if (events->count(interface_key::CURSOR_DOWN_FAST) || events->count(interface_key::CURSOR_DOWNLEFT_FAST) || events->count(interface_key::CURSOR_DOWNRIGHT_FAST))
        sel_row += 10;

    if (events->count(interface_key::CURSOR_DOWN_Z_AUX))
    {   sel_row = units.size()-1; row_hint=0; }

    if (events->count(interface_key::STRING_A000)||events->count(interface_key::CURSOR_UP_Z_AUX))
    {   sel_row = 0; row_hint=0; }

    if (events->count(interface_key::CURSOR_LEFT) || events->count(interface_key::CURSOR_UPLEFT) || events->count(interface_key::CURSOR_DOWNLEFT)){
        sel_column--; row_hint+=15; col_hint+=15;
        sel_attrib--;
    }
    if (events->count(interface_key::CURSOR_LEFT_FAST) || events->count(interface_key::CURSOR_UPLEFT_FAST) || events->count(interface_key::CURSOR_DOWNLEFT_FAST))
        sel_column -= 10;
    if (events->count(interface_key::CURSOR_RIGHT) || events->count(interface_key::CURSOR_UPRIGHT) || events->count(interface_key::CURSOR_DOWNRIGHT)){
        sel_column++; row_hint+=15; col_hint+=15;
        sel_attrib++;
    }
    if (events->count(interface_key::CURSOR_RIGHT_FAST) || events->count(interface_key::CURSOR_UPRIGHT_FAST) || events->count(interface_key::CURSOR_DOWNRIGHT_FAST))
        sel_column += 10;

    if((!enabler->tracking_on )
      ||(enabler->tracking_on &&(gpsx >= column_anchor[COLUMN_LABORS]))){
        if ((sel_column > -1) && events->count(interface_key::CURSOR_UP_Z))
        {
            // go to beginning of current column group; if already at the beginning, go to the beginning of the previous one
            sel_column--; row_hint+=15; col_hint+=15;
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
            first_row-=4; row_hint+=15; col_hint+=15;
            if(first_row<0) first_row=0;
        }else if(events->count(interface_key::CURSOR_UP_Z)){
            first_row+=4; row_hint+=15; col_hint+=15;
        }
    }

    sel_attrib=sel_attrib<0? 0: sel_attrib>18? 18:sel_attrib;

    bool row_knocking = true;
    //deal wth column overshots and enable push to other side
    if (sel_row < 0){
        if (scrollknock(&sel_row, 0, (int)units.size()-1))
            row_hint = 0;
    }else if (sel_row > units.size()-1){
        if (scrollknock(&sel_row, (int)units.size()-1 , 0))
            row_hint = 0;
    }else
        row_knocking = false;

    if (sel_column < 0){
        if (scrollknock(&sel_column, 0, (int)NUM_LABORS - 1))
            col_hint = 0;
    }else if (sel_column >= NUM_LABORS){
        if (scrollknock(&sel_column, (int)NUM_LABORS - 1 , 0))
            col_hint = 0;
    }else{
        if(!row_knocking)
            scroll_knock = false;
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
    if (events->count(interface_key::CUSTOM_J) && (cur->allowEdit) && Units::isValidLabor(unit, cur_labor))
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
            
            if(finesort_mode_b!=-1){ //going back to stashed mode
                finesort_mode=finesort_mode_b;
                finesort_mode_b=FINESORT_UNDER;
            }else if(finesort_mode==FINESORT_COLUMN 
              && (cur_column != column_sort_last)){
            //reselects column if column mode and differ from last
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
                selection_changed = false;
            }
        }else{
            if(!(widesort_mode==WIDESORT_SELECTED && selection_changed)){
                widesort_mode=static_cast<wide_sorts>((int)widesort_mode-1);
                if(widesort_mode==WIDESORT_UNDER) widesort_mode=static_cast<wide_sorts>((int)WIDESORT_OVER-1);

            }else{
                selection_changed = false;
            }
        }
        dualSort();
    }

    if (events->count(interface_key::CUSTOM_D)){
        show_details = (show_details+1)%6;
        sizeDisplay();
        
        tip_show_timer=88;
        tip_show=11+show_details;
    }

    if (events->count(interface_key::CUSTOM_N)){
        notices_countdown=66;
        show_curse = (show_curse+1)%2;
        unit_info_ops::calcNotices(units);
        detail_mode=DETAIL_MODE_NOTICE;
        sizeDisplay();
        
        tip_show_timer=88;
        tip_show=17+show_curse;
    }

    if (events->count(interface_key::CUSTOM_SHIFT_N)){
        if(tran_names==0) tran_names=1; //flipping seq riddle
        else if(tran_names==1) tran_names=3;
        else if(tran_names==2) tran_names=0;
        else if(tran_names==3) tran_names=2;
        
        tip_show_timer=88;
        tip_show=19+tran_names;
        
        refreshNames();
        dualSort();
    }

    if (events->count(interface_key::CUSTOM_SHIFT_C)){

        tip_show_timer=100;
        theme_color++;
        
        /*
        if(cheat_level>2&&spare_skill>777){ //leave cheatmode
            theme_color= spare_skill=cheat_level=0;
            color_mode=2; hint_power = 1;
            //~ cltheme[4] = COLOR_DARKGREY; //FG for not set:
            //~ cltheme[12]= COLOR_BLACK;    //cursor FG not set:
            //~ cltheme[20]= COLOR_GREY;     //FG set
            //~ cltheme[24]= COLOR_WHITE;    //cursor BG set
            tip_show=10; //(left cheatmode) 
        }else 
        */
        
        if(theme_color==1){
            cltheme=cltheme_b;
            tip_show=theme_color;
        }else if(theme_color==2){
            cltheme=cltheme_c;
            tip_show=theme_color;
        }else{ //color ==3
            cltheme=cltheme_a;
            theme_color=0; 
            tip_show=theme_color;
        }
    }

    if (events->count(interface_key::CUSTOM_T)
      ||events->count(interface_key::CUSTOM_SHIFT_T)) //toggle apt coloring
    {
        tip_show_timer=100;

        //6 color_modes 0-5
        //0 is legacy no details
        //1 is ?
        //2 is standard with apt hint level
        //3-5 is plain colors
        
        if (events->count(interface_key::CUSTOM_T)){
         
            //going forward but stick on mode 2
            
            if(color_mode==5){
                cheat_level=0;
                color_mode=0;
                tip_show=23; //Legacy view (no details)
            }else if(color_mode>2){
                color_mode++;
                tip_show=3; //plain view
            }else if(color_mode==0){
                color_mode=1;
                tip_show=3; //monochrome
            }else if(color_mode==1){
                color_mode=2;
                hint_power=0;
                tip_show=4; //lowest apt hint
            }else if(color_mode==2){
                if(hint_power<3){
                    hint_power++;
                    tip_show=4+hint_power;
                }else{
                    color_mode=3;
                    tip_show=3; //plain color
                }
            } 

        }else{
            //going backward but stick on mode 2
            
            if(color_mode==1){ //is disabled
                color_mode=0;
                tip_show=23; //Legacy view (no details)
            }else if(color_mode>3){
                color_mode--;
                tip_show=3; //plain view
            }else if(color_mode==0){
                color_mode=5;
                tip_show=3; //monochrome
              
                //adjust cheat when cycling back
                cheat_level++;
                if(cheat_level>2){ //lavens cheat 
                    tip_show_timer=500;
                    tip_show=8; color_mode=2; hint_power = 3;
                } 
                if(cheat_level>4||(cheat_level>2&&spare_skill>777)){
                    tip_show=9; //armoks cheat
                    spare_skill = 65810; cheat_level=5;
          
                    //~ cltheme[5]=cltheme[17]= COLOR_BROWN;
                    //~ cltheme[6]=cltheme[18]= COLOR_DARKGREY;
                    //~ cltheme[7]=cltheme[19]= COLOR_LIGHTRED;

                    //~ cltheme[4] = COLOR_RED;       //FG of not set:
                    //~ cltheme[12]= COLOR_DARKGREY;   //cursor FG not set:
                    //~ cltheme[20]= COLOR_LIGHTRED;    //FG of set
                    //~ cltheme[24]= COLOR_LIGHTMAGENTA; //cursor BG set
                }
            }else if(color_mode==3){
                color_mode=2;
                hint_power=3;
                tip_show=7; //highest apt hint
            }else if(color_mode==2){
                if(hint_power>0){
                    hint_power--;
                    tip_show=4+hint_power;
                }else{
                    color_mode=1;
                    tip_show=3; //plain color
                }
            } 
        }
        
        if(color_mode==2)
            unit_info_ops::calcAptScores(units);

        //~ color_mode = (color_mode+6)%6; //safety
    }

    if (
        (events->count(interface_key::CUSTOM_Q)
        ||events->count(interface_key::CUSTOM_W))
        && (cheat_level>2)
        && columns[sel_column].skill != job_skill::NONE
        && cur->unit->status.current_soul
    ){

        df::unit_soul *soul = cur->unit->status.current_soul;
        df::unit_skill *s = binsearch_in_vector<df::unit_skill,df::job_skill>(soul->skills, &df::unit_skill::id, columns[sel_column].skill);

        if(detail_mode==DETAIL_MODE_ATTRIBUTE
          && spare_skill>777
        ){  //edit attribs..
            int inc=(events->count(interface_key::CUSTOM_Q))?-256:256;
            int d= sel_attrib;
            if(d<6){
                int v=unit->body.physical_attrs[d].value+inc;
                if(v<1) v=1;
                if(v>4990) v=4990;
                unit->body.physical_attrs[d].value=v;
            }else{
                int v=unit->status.current_soul->mental_attrs[d-6].value+inc;
                if(v<1) v=1;
                if(v>4990) v=4990;
                unit->status.current_soul->mental_attrs[d-6].value=v;
            }
        }else if(s==NULL){ //set skill to 0
            auto uskill = df::allocate<df::unit_skill>();
            int c = 0;
            if(spare_skill>0){
                c = 1;
                spare_skill--;
                if(Units::isValidLabor(cur->unit , columns[sel_column].labor))
                    cur->unit->status.labors[columns[sel_column].labor] = true;
            }
            uskill->rating = static_cast<df::enums::skill_rating::skill_rating>(c);
            uskill->experience = 0;
            uskill->id = columns[sel_column].skill;
            uskill->unused_counter = 0;
            uskill->rusty = 0;
            uskill->rust_counter = 0;
            uskill->demotion_counter = 0;
            uskill->natural_skill_lvl = 0;
            uskill->_identity = soul->_identity;

            int ds = (int)uskill->id;
            size_t inb = soul->skills.size();
            size_t mn = soul->skills.size();

            for( size_t p = 0; p< soul->skills.size(); p++ ){
              if(ds < (int)(soul->skills[p]->id)){
                  inb = p;
                  break;
              }
            }

            soul->skills.push_back( uskill );

            for( size_t p = mn; p>inb; p-- ){
              soul->skills[p] = soul->skills[p-1];
            }

            soul->skills[inb] = uskill;

        }else{ //change skill

            int c = (int)(s->rating);
            if(events->count(interface_key::CUSTOM_Q)){
                if(c==0){
                    s->experience = 0;
                }
                c--;
                c = c<0 ? 0:c;
            }else if(spare_skill>c){
               c++;
               c = c>NUM_SKILL_LEVELS-1 ? NUM_SKILL_LEVELS-1:c;
            }

            int skdiff= (c-(int)(s->rating));
            if(skdiff>0) skdiff*=((int)(s->rating)+1);
            else skdiff*=((int)(s->rating));

            if(skdiff && spare_skill>=skdiff){
                spare_skill -= skdiff;
                s->rating = static_cast<df::enums::skill_rating::skill_rating>(c);
            }
            if(Units::isValidLabor(cur->unit , columns[sel_column].labor)){
                cur->unit->status.labors[columns[sel_column].labor] = c>0 ;
            }
        }
    }

    //!fix need shift tab code here
    if (events->count(interface_key::SEC_CHANGETAB))
    {
        int duemode= detail_mode-1;
        if(duemode==-1) duemode+=DETAIL_MODE_MAX;
        detail_mode = duemode;
    }
    else if (events->count(interface_key::CHANGETAB))
    {
        int duemode = detail_mode+1;
        if(duemode==DETAIL_MODE_MAX) duemode=0;
        detail_mode = duemode;
    }

    //unit selection
    if (events->count(interface_key::CUSTOM_SHIFT_X))
    {
        selection_changed = true;
        if (last_selection == -1 || last_selection == cur_row)
            events->insert(interface_key::CUSTOM_X);
        else
        {
            for (int i = std::min(cur_row, last_selection);
                 i <= std::max(cur_row, last_selection);
                 i++)
            {
                if (i == last_selection) continue;
                //if (!units[i]->allowEdit) continue;
                units[i]->selected = units[last_selection]->selected;
            }
            stashSelection(units);
        }
    }

    if (events->count(interface_key::CUSTOM_X))//&& cur->allowEdit
    {
        selection_changed = true;
        cur->selected = !cur->selected;
        stashSelection(cur);
        last_selection = cur_row;
    }

    if (events->count(interface_key::CUSTOM_A) || events->count(interface_key::CUSTOM_SHIFT_A))
    {
        selection_changed = true;
        for (size_t i = 0; i < units.size(); i++){
            if (units[i]->selected||units[i]->allowEdit){
                units[i]->selected = (bool)events->count(interface_key::CUSTOM_A);
            } //unedittable units need selected individually
        }
        stashSelection(units);
    }

    //nick prof editting
    if (events->count(interface_key::CUSTOM_B))
    {
        Screen::show(dts::make_unique<viewscreen_unitbatchopst>(units, true, &do_refresh_names), plugin_self);
    }

    //nick prof editting
    if (events->count(interface_key::CUSTOM_E))
    {
        vector<UnitInfo*> tmp;
        tmp.push_back(cur);
        Screen::show(dts::make_unique<viewscreen_unitbatchopst>(tmp, false, &do_refresh_names), plugin_self);
    }

    if (events->count(interface_key::CUSTOM_P))
    {
        bool has_selected = false;
        for (size_t i = 0; i < units.size(); i++)
            if (units[i]->selected)
                has_selected = true;

        if (has_selected) {
            Screen::show(dts::make_unique<viewscreen_unitprofessionset>(units, true), plugin_self);
        } else {
            vector<UnitInfo*> tmp;
            tmp.push_back(cur);
            Screen::show(dts::make_unique<viewscreen_unitprofessionset>(tmp, false), plugin_self);
        }
    }

    if (events->count(interface_key::CUSTOM_SHIFT_P))
    {
        manager.save_from_unit(cur);
    }

    if (events->count(interface_key::CUSTOM_H))
    {
        Screen::show(dts::make_unique<viewscreen_popHelpst>());
    }

    if (events->count(interface_key::UNITJOB_VIEW_UNIT) || events->count(interface_key::UNITJOB_ZOOM_CRE))
    {
        save_dfkeeper_config();
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


void viewscreen_unitkeeperst::paintAttributeRow(int row ,UnitInfo *cur, bool header=false)
{
    using namespace unit_info_ops;
    df::unit* unit = cur->unit;

    int colm = 0;
    int dwide = column_size[COLUMN_DETAIL];
    int pad = dwide<19?0:(dwide-19)/2;
    int uavg = (cur->work_aptitude+cur->skill_aptitude)/2;
    int aavg = (work_aptitude_avg+skill_aptitude_avg)/2;
    int vlow = (uavg*3/8+aavg/2-7);// (3/5ths)
    int vhigh = (uavg*5/8+aavg/2+16);

    for (int att = 0; att < 19; att++)
    {
        //skip attribs if too small
        if(dwide<19 && att==15) att++; //musi
        if(dwide<18 && att==13) att++; //ling
        if(dwide<17 && att==4) att++;  //recoup
        if(dwide<16 && att==5) att++;  //disease
        if(dwide<15 && att>dwide-1) continue; //crop rest

        int bg = COLOR_BLACK;
        int fg = COLOR_GREY;

        if(sel_attrib==att && (cheat_level>2 && spare_skill>777)){
            bg = COLOR_RED;
        }

        int val;
        if(att<6){
             val = unit->body.physical_attrs[att].value;
        }else{
             val = unit->status.current_soul->mental_attrs[att-6].value;
        }

        val = ((val+124)*10)/256; // 0 to 5000 > 0 to 200

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

            if(fg == COLOR_GREY)
                fg = COLOR_YELLOW;
            const char legenda[] = "SaterdAfwcipmlsmkes"; //attribute
            const char legendb[] = "tgoneinoirnaeipuimo";

            Screen::paintTile(Screen::Pen(legenda[colm], fg, bg), column_anchor[COLUMN_DETAIL] +colm+pad , 1 );
            Screen::paintTile(Screen::Pen(legendb[colm], fg, bg), column_anchor[COLUMN_DETAIL] +colm+pad , 2 );
        }

        if(row>=0&&row<display_rows){
            val /= 10;

            Screen::paintTile(Screen::Pen(skill_levels[val].abbrev, fg, bg), column_anchor[COLUMN_DETAIL] +colm+pad , 4 + row);
        }

        colm++;
    }//attribs loop
}

void viewscreen_unitkeeperst::paintLaborRow(int &row,UnitInfo *cur, df::unit* unit)
{
    if(detail_mode==DETAIL_MODE_ATTRIBUTE)
        paintAttributeRow(row,cur);

    const int8_t bwtheme[]={
        /*noskill         hint 0   */
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
        //BG set and cursor
        COLOR_WHITE,      COLOR_GREY,
        //FG set and cursor
        COLOR_WHITE,      COLOR_WHITE,

        //16 BG mili      BG other
        COLOR_CYAN,       COLOR_LIGHTBLUE
    };

    //first_column can change by hoz scroll
    int col_end = first_column + column_size[COLUMN_LABORS];

    int8_t bg,fg;

    for (int role = first_column; role < col_end; role++)
    {
        bool is_work=(columns[role].labor != unit_labor::NONE);
        bool is_skilled = false;
        bool is_cursor = false;
        bool role_isset = false;

        uint8_t ch = '-';

        if (columns[role].skill == job_skill::NONE)
        {
            ch = 0xF9; // fat dot char for noskill jobs, it has opaque bg
        }else{
            is_skilled = true;

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
            role_isset = true;
        }

        if ((role == sel_column) && (row+first_row == sel_row)){
            is_cursor = true;
        }

        int crow = 0, hint = 0;
        if(is_skilled) hint = 1;
        if(role_isset) crow = 2;
        if(is_cursor)  crow++;
        //1345  0123
        if( color_mode!=0 )
        {
            if(color_mode==2){
                if(is_skilled)
                    hint += cur->column_hints[role];
            }else if(color_mode!=1){
                hint = color_mode-3;
                const int swip[] ={2,3,1};
                hint = swip[hint];
            }else{
                hint+=1;
                if(hint==1) hint=0;
            }

            bg = cltheme[crow*8+hint];
            fg = cltheme[crow*8+4+hint];

            if(is_cursor && !(is_work && cur->allowEdit)){
               bg = cltheme[32];
               fg = cltheme[33];
               //if(role>103) bg=cltheme[33]; //fighting etc
            }

            if( row_hint<60 && bg==cltheme[0]&& row+first_row == sel_row ){
                bg = cltheme[34]; //flash blue bg to find cursor
            }

            if( col_hint<60 && bg==cltheme[0] && role == sel_column){
                bg = cltheme[34];
            }
        }else{
            bg = bwtheme[crow*4+hint];
            fg = bwtheme[crow*4+2+hint];

            if(!(is_work||is_cursor)){
               bg = bwtheme[16];
               if(role>103)
                   bg = bwtheme[17]; //fighting etc
            }
        }

        //paints each element in labor grid
        Screen::paintTile(Screen::Pen(ch, fg, bg), column_anchor[COLUMN_LABORS] + role - first_column, 4 + row);

    }//columns
}


void viewscreen_unitkeeperst::render()
{
    if (Screen::isDismissed(this))
        return;

    dfhack_viewscreen::render();

    Screen::clear();

    Screen::paintString(Screen::Pen(' ', 7, 0), column_anchor[COLUMN_SELECTED]-5, 2, "Keep");
    Screen::paintTile(Screen::Pen('\373', 7, 0), column_anchor[COLUMN_SELECTED], 2);
    Screen::paintString(Screen::Pen(' ', 7, 0), column_anchor[COLUMN_NAME], 2, "Name");

    string detail_str;
    string excess_field_str = "";
    int8_t excess_field_colr = 0;
    int8_t cclr = COLOR_GREY;
    int8_t fg = COLOR_WHITE;
    int8_t bg = COLOR_BLACK;

    detail_str = "                  "; //clear the attribute legend
    Screen::paintString(Screen::Pen(' ', 7, 0), column_anchor[COLUMN_DETAIL], 1, detail_str);

    detail_str = detailmode_legend[detail_mode];

    if(detail_mode==DETAIL_MODE_NOTICE){
        if(show_curse) detail_str +="s";
        if(notices_countdown>0){
            notices_countdown--;
            if(show_curse) detail_str +=" (curses)";
            else detail_str +=" (no curses)";
        }
    }
    Screen::paintString(Screen::Pen(' ', cclr, 0), column_anchor[COLUMN_DETAIL], 2, detail_str);

    sel_unitid = units[sel_row]->unit->id;

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

        if(ui->race_id!=-1 &&(show_sprites||col_offset>80) && profession!=profession::NONE){
            auto graphics = world->raws.creatures.all[ui->race_id]->graphics;
            Screen::paintTile(
                Screen::Pen(' ', fg, 0,
                    graphics.profession_add_color[creature_graphics_role::DEFAULT][profession],
                    graphics.profession_texpos[creature_graphics_role::DEFAULT][profession]),
                column_anchor[COLUMN_LABORS] + col, 3);
        }
    }

    bg = COLOR_BLACK;

    for (int row = 0; row < display_rows; row++)
    {
        int row_offset = row + first_row;

        if (row_offset >= units.size()) break;

        UnitInfo *cur = units[row_offset];
        df::unit *unit = cur->unit;

        int glad_lvl = unit->status.current_soul ? unit->status.current_soul->personality.stress_level/-1000 : 0;

        if (glad_lvl > 100) glad_lvl = 100;
        if (glad_lvl < -100) glad_lvl = -100;
        //display the integer counter scaled down to comfortable values
        string stress = stl_sprintf("%4i", glad_lvl);

        fg = COLOR_BLUE;
        if (glad_lvl < 100) fg = COLOR_LIGHTBLUE;
        if (glad_lvl < 50)  fg = COLOR_LIGHTCYAN;
        if (glad_lvl < 25)  fg = COLOR_LIGHTGREEN;
        if (glad_lvl < 10)  fg = COLOR_GREEN;
        if (glad_lvl < 1)   fg = COLOR_YELLOW;
        if (glad_lvl < -9)  fg = COLOR_BROWN;
        if (glad_lvl < -24) fg = COLOR_RED;
        if (glad_lvl < -49) fg = COLOR_LIGHTMAGENTA;
        if (glad_lvl < -99) fg = COLOR_MAGENTA;
    

        Screen::paintString(Screen::Pen(' ', fg, bg), column_anchor[COLUMN_SELECTED]-5, 4 + row, stress);

        Screen::paintTile(
            (cur->selected) ? Screen::Pen('\373', COLOR_LIGHTGREEN, 0):
                ((cur->allowEdit) ? Screen::Pen('-', COLOR_GREY, 0) : Screen::Pen('-', COLOR_RED, 0)),
            column_anchor[COLUMN_SELECTED], 4 + row);

        fg = COLOR_WHITE;

        if(cur->sort_grouping_hint) fg = COLOR_LIGHTCYAN;
        else fg = COLOR_WHITE;

        if (row_offset == sel_row) bg = COLOR_BLUE;

        string name;
        if(tran_names<2)
            name = cur->name;
        else
            name = cur->transname;

        name.resize(column_size[COLUMN_NAME]);
        Screen::paintString(Screen::Pen(' ', fg, bg), column_anchor[COLUMN_NAME], 4 + row, name);

        bg = COLOR_BLACK;
        if (detail_mode == DETAIL_MODE_SQUAD) {
            fg = COLOR_LIGHTCYAN;
            detail_str = cur->squad_info;
        } else if (detail_mode == DETAIL_MODE_NOTICE) {
            if(cur->notice_score > 1000){
                fg = COLOR_LIGHTMAGENTA;
            }else if(cur->notice_score>300){
                fg = COLOR_YELLOW;
            }else{
                fg = COLOR_GREY;
            }
            detail_str = cur->notices;
        } else if (detail_mode == DETAIL_MODE_JOB) {
            detail_str = cur->job_desc;
            if (cur->job_mode == UnitInfo::IDLE) {
                fg = COLOR_YELLOW;
            } else if (cur->job_mode == UnitInfo::SOCIAL) {
                fg = COLOR_LIGHTGREEN;
            } else if (cur->job_mode == UnitInfo::MILITARY) {
                fg = COLOR_GREY;
            } else {
                fg = COLOR_LIGHTCYAN;
            }
        } else if (detail_mode == DETAIL_MODE_PROFESSION) {
            fg = cur->color;
            detail_str = cur->profession;
            if(false && Units::isChild(cur->unit)
                //&&Units::getRaceName(cur->unit)=="Dwarf"
            ){
                detail_str +=" "+cur->age;
            }
        } else if (detail_mode == DETAIL_MODE_ATTRIBUTE) {
            detail_str = "";
        }

        if(detail_mode != DETAIL_MODE_ATTRIBUTE){
            if (row_offset == sel_row && detail_str.size()>column_size[COLUMN_DETAIL]+2){
                excess_field_str = detail_str;
                excess_field_colr = fg;
            }
            detail_str.resize(column_size[COLUMN_DETAIL]);
            Screen::paintString(Screen::Pen(' ', fg, bg), column_anchor[COLUMN_DETAIL], 4 + row, detail_str);
        }

        paintLaborRow(row,cur,unit);
    }

    if(detail_mode==DETAIL_MODE_ATTRIBUTE){
        paintAttributeRow( sel_row-first_row, units[sel_row], true);
    }

    row_hint++;
    col_hint++;
    // all rows finished

    UnitInfo *cur = units[sel_row];
    bool canEdit = false;

    if (cur != NULL){
        paintExtraDetail(cur, excess_field_str, excess_field_colr);
        printScripts(cur);
        canEdit = (cur->allowEdit) &&
        Units::isValidLabor(cur->unit, columns[sel_column].labor);
    }
    paintFooter(canEdit);
}


void viewscreen_unitkeeperst::printScripts(UnitInfo *cur)
{
    df::unit *unit = cur->unit;
    int x = xmargin, y = 5 + display_rows;

    Screen::Pen male_pen(' ', COLOR_LIGHTCYAN, 0);
    Screen::Pen fema_pen(' ', COLOR_LIGHTGREEN, 0);
    if(cur->unit && cur->unit->sex){
        Screen::paintString(male_pen, x, y, "\x0b");
    }else{
        Screen::paintString(fema_pen, x, y, "\x0c");
    }
    x++;

    string syear = to_string(cur->age)+"yr";
    syear.resize(4);

    string sfocus=to_string(cur->focus);
      
    if(cur->focus> -1)
        sfocus="fcs+"+sfocus;
    else
        sfocus="fcs"+sfocus;

    string snom;
    if(tran_names&1) snom = cur->transname;
    else snom = cur->name;

    string sjob,aps,lvs,xps;

    if (columns[sel_column].skill == job_skill::NONE)
    {
        sjob = ENUM_ATTR_STR(unit_labor, caption, columns[sel_column].labor);
        if (unit->status.labors[columns[sel_column].labor])
            sjob = "Availed to "+sjob;
        else
            sjob = "Forbade to "+sjob;
    }
    else
    {
        string descq = "";
        string statq = "";

        int apti = (color_mode!=0)?cur->column_aptitudes[sel_column]:0;
        int level = unitSkillRating(cur, columns[sel_column].skill);
        int exp = unitSkillExperience(cur, columns[sel_column].skill);

        if(apti>0) aps= stl_sprintf("%d",apti);

        if (level || exp)
        {
            if (level > NUM_SKILL_LEVELS - 1){
                level = NUM_SKILL_LEVELS- 1;
            }
            lvs = stl_sprintf("%c", skill_levels[level].abbrev);

            sjob = stl_sprintf("%s %s", skill_levels[level].name, ENUM_ATTR_STR(job_skill, caption_noun, columns[sel_column].skill));

            if (level != NUM_SKILL_LEVELS - 1){
                xps = stl_sprintf("%d", ((exp+5)*100)/ (skill_levels[level].points+5));
            }
        }else{
            string strb = ENUM_ATTR_STR(job_skill, caption_noun, columns[sel_column].skill);
            string strc = strb.substr(0,1);
            if(strc=="A"||strc=="E"||strc=="I"||strc=="O"){
                strc = "Never an ";
            }else{
                strc = "Never a ";
            }
            if(strb=="Balance"||strb=="Concentration"||strb=="Coordination"||strb=="Discipline"||strb=="Military Tactics"){
                if(strb=="Military Tactics"){ strc = "Never studied ";}
                else{
                    strc = "Never trained ";
                }//ocd confirmed
            }
            sjob = strc+strb;
        }
    }
    if(aps.size()==0&&(lvs.size()||xps.size())){ aps="  "; }

    string sprof = cur->profession;

    int spill= xpanover;

    int cdsize=0;

    if(aps.size()){ cdsize+=aps.size()+3; }
    if(xps.size()){ cdsize+=xps.size()+3; }
    if(lvs.size()){ cdsize+=lvs.size()+3; }

    spill-=xmargin;
    spill-=cdsize;
    spill-=sjob.size();
    spill-=syear.size();
    if(color_mode!=0) spill-=sfocus.size();
    spill-=snom.size();
    spill-=sprof.size();

    string blank=" ";
    string blanks=" ";

    int skipb=0;
    bool sqz=false;
    if(spill<0){
      skipb= -spill;
      if(skipb>3){ skipb=3; }
      spill+=skipb;
    }
    if(spill<0){ spill+=sfocus.size(); sfocus=""; }
    if(spill<0){ skipb++; spill++; }
    if(spill<0){ skipb++; spill++; }
    if(spill<0){
        int m =sprof.size()+spill;
        sprof.resize(m<3?3:m);
        sprof+="..";
    }else{
        sprof+=":";
    }

    int fobg=COLOR_WHITE;
    if(cur->focus< 9) fobg=COLOR_LIGHTCYAN;
    if(cur->focus< 6) fobg=COLOR_LIGHTGREEN;
    if(cur->focus< 3) fobg=COLOR_GREEN;
    if(cur->focus< 0) fobg=COLOR_YELLOW;
    if(cur->focus<-3) fobg=COLOR_BROWN;
    if(cur->focus<-5) fobg=COLOR_RED;
    if(cur->focus<-7) fobg=COLOR_MAGENTA;
    if(cur->focus<-8) fobg=COLOR_DARKGREY;

    OutputString( COLOR_LIGHTBLUE , x, y, syear);
    if(color_mode!=0){
        if(skipb<1) OutputString( COLOR_GREY, x, y, blank);
        OutputString( fobg , x, y, sfocus);
    }
    OutputString( COLOR_GREY , x, y, ",");
    if(skipb<3) OutputString( COLOR_GREY, x, y, blank);
    OutputStrings( COLOR_WHITE, x, y, snom);
    OutputStrings( COLOR_GREY, x, y, blank);
    if(skipb<2) OutputStrings( COLOR_GREY, x, y, blank);
    OutputStrings( COLOR_LIGHTBLUE, x, y, sprof);
    if(skipb<5) OutputStrings( COLOR_GREY, x, y, blank);
    OutputStrings( COLOR_LIGHTCYAN, x, y, sjob);
    //OutputString( COLOR_GREY, x, y, blank);
    if(skipb<4) OutputStrings( COLOR_GREY, x, y, blank);

    int lit=COLOR_LIGHTCYAN;
    int drk=COLOR_GREY;

    int defx = xpanend-cdsize;
    int ovex = xpanover-cdsize;
    int bx=x;

    blanks.resize( dimex-1-ovex);
    OutputStrings( COLOR_BLACK, bx, y, blanks);

    if(x<defx&&color_mode!=0){ x=defx; }

    if(x>ovex){ x=ovex; }

    if(xps.size()){
        x--;
        OutputStrings( drk, x, y, " xp");
        OutputStrings( lit, x, y, xps);
        OutputStrings( drk, x, y, blank);
    }

    if(lvs.size()){
        OutputStrings( drk, x, y, "lv");
        OutputStrings( lit, x, y, lvs);
        OutputStrings( drk, x, y, blank);
    }

    if(color_mode!=0){
        int litd=lit;
        int apti = (color_mode!=0)?cur->column_hints[sel_column]:0;

        if((lvs.size()||xps.size()||aps.size())){
          x--; OutputStrings( drk, x, y, " ap");
        }

        if(aps.size()){ OutputStrings( litd, x, y, aps);}
    }

    blanks.resize( dimex-1-x);
    OutputStrings( COLOR_BLACK, x, y, blanks);

    Screen::drawBorder(" Cavern Keeper ");
    if(color_mode==0) return;

    x=xmargin; y++;

    int to_def=xpanend-xmargin;
    int to_lim=xpanover-xmargin;
    int to_max=dimex-xmargin-1;

    if(show_details!=0&&cur->tagline.size()==0){
        unit_info_ops::setDescriptions(cur);
    }

    string ds=cur->traits;

    if(show_details!=0&&show_details<4){

        if(ds.size()>to_lim){
            ds.resize(to_lim-3); ds+="...";
        }
        ds.resize(to_max);
        Screen::paintString(Screen::Pen(' ', COLOR_GREY, 0), x, y, ds);
        x=xmargin; y++;
    }

    if(show_details>1&&show_details<5){

        ds="";
        if(cur->likesline.size()>1)
            ds+="Likes "+cur->likesline;

        string rg = "";
        if(cur->regards.size())
            rg+=" Regards "+cur->regards;
        else
            rg+=" Regards nothing much ";

        int rn=rg.size(),dn=ds.size();

        if(rn+dn>to_lim){
            rg = " Rgds "+cur->regards;
            rn=rg.size();
            if(rn+dn>to_lim){
                ds.resize(to_lim-rn-3);
                ds+=".. ";
                dn=ds.size();
            }
        }

        if(dn+rn<to_def) ds.resize(to_def-rn);

        OutputString( COLOR_GREY, x, y, ds);

        rg.resize(to_max-ds.size());

        OutputString( COLOR_WHITE, x, y, rg);
        x=xmargin; y++;
    }

    if(show_details>2){
        ds=cur->tagline;
        string rg;
        if(cur->dream.size())
            rg+=" Dreams "+cur->dream;
        if(cur->godline.size())
            rg += ",Gods "+cur->godline;
        int rn=rg.size(),dn=ds.size();

        if(rn+dn>to_lim){
            ds.resize(to_lim-rn-3);
            ds+=".. ";
            dn=ds.size();
        }

        if(dn+rn<=to_def) ds.resize(to_def-rn-1); //-1 tweak

        OutputString( COLOR_BROWN, x, y, ds);

        rg.resize(to_max-ds.size());

        OutputString( COLOR_YELLOW, x, y, rg);
    }
}


void viewscreen_unitkeeperst::paintExtraDetail(UnitInfo *cur,string &excess_field_str,int8_t &excess_field_colr)
{
    int x=xmargin,y=display_rows+6+(show_details>3?6-show_details:show_details); //first line below list

    if(cheat_level>2){ //Declare cheat mode
        x = dimex/2 - 13;
        y = ((y*3) + 2*(dimey-5))/5;

        int clr = COLOR_YELLOW;
        OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_Q));
        OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_W));

        string cheat;
        if(spare_skill<778){
             cheat = ": Whims of Laven ~>";
             OutputString(clr, x, y, cheat);
             OutputString(15, x, y, stl_sprintf(" %i pts", spare_skill));
        }
        else
        {
             spare_skill = 65810;
             cltheme[5]=cltheme[17]= COLOR_BROWN;
             cltheme[6]=cltheme[18]= COLOR_DARKGREY;
             cltheme[7]=cltheme[19]= COLOR_LIGHTRED;
             cltheme[20]= COLOR_MAGENTA;
             cheat = ": Armok's Thirst !!";
             OutputString(COLOR_LIGHTMAGENTA, x, y, cheat);
        }
    }
    else if (show_details==0 && cur->unit->military.squad_id > -1)
    {
        string squadLabel = "Squad: ";
        Screen::paintString(Screen::Pen(' ', COLOR_GREY, 0), x, y, squadLabel);
        x += squadLabel.size();

        string squad = cur->squad_effective_name;
        Screen::paintString(Screen::Pen(' ', 11, 0), x, y, squad);
        x += squad.size();

        string pos = stl_sprintf(" Pos %i", cur->unit->military.squad_position + 1);
        Screen::paintString(Screen::Pen(' ', 9, 0), x, y, pos);
        x += pos.size()+1;
    }
    else if(excess_field_str.size())
    {   excess_field_str.resize(dimex-1-xmargin);
        Screen::paintString(Screen::Pen(' ', excess_field_colr, 0), x, y, excess_field_str);
    }
}


void viewscreen_unitkeeperst::paintFooter(bool canToggle){

    int gry=COLOR_WHITE;
    int blk=COLOR_BLACK;
    int drk=COLOR_DARKGREY;
    int lit=COLOR_WHITE;

    string hblank="",iblank="",jblank="",kblank="",skeys="";

    int z,h,i,j,k;

    skeys =Screen::getKeyDisplay(interface_key::UNITJOB_VIEW_UNIT);
    skeys+=Screen::getKeyDisplay(interface_key::UNITJOB_ZOOM_CRE);
    skeys+=Screen::getKeyDisplay(interface_key::SELECT);
    skeys+=Screen::getKeyDisplay(interface_key::CUSTOM_J);
    skeys+=Screen::getKeyDisplay(interface_key::CUSTOM_X);
    skeys+=Screen::getKeyDisplay(interface_key::CUSTOM_A);
    skeys+=Screen::getKeyDisplay(interface_key::CUSTOM_SHIFT_A);

    z=xfooter+11-skeys.size(); //z=extraspace+difference in shortcut size
    h=0,i=0,j=0,k=0;
    while(z>0){ if(z>1){h++;z-=2;} if(z>0){i++;z-=1;} if(z>0){j++;z-=1;} if(z>0){k++;z-=1;} }
    hblank.resize(h); iblank.resize(i); jblank.resize(j); kblank.resize(k);

    int x = xmargin, y = dimey - 4;

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::UNITJOB_VIEW_UNIT));
    OutputString(gry, x, y, ": View Unit, ");

    OutputString(blk, x, y, iblank);

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::UNITJOB_ZOOM_CRE));
    OutputString(gry, x, y, ": Go Unit, ");

    OutputString(blk, x, y, hblank);

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::SELECT));
    OutputString(canToggle ? gry : drk, x, y, ": Pick Labor ");

    OutputString(blk, x, y, jblank);

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_J));
    OutputString(canToggle ? gry : drk, x, y, ": Work, ");

    OutputString(blk, x, y, hblank);

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_X));
    OutputString(gry, x, y, ": Select ");

    OutputString(blk, x, y, kblank);

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_A));
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_SHIFT_A));
    OutputString(gry, x, y, ": all/none ");


    skeys =Screen::getKeyDisplay(interface_key::LEAVESCREEN);
    skeys+=Screen::getKeyDisplay(interface_key::CHANGETAB);
    skeys+=Screen::getKeyDisplay(interface_key::SECONDSCROLL_UP);
    skeys+=Screen::getKeyDisplay(interface_key::SECONDSCROLL_DOWN);
    skeys+=Screen::getKeyDisplay(interface_key::SECONDSCROLL_PAGEUP);
    skeys+=Screen::getKeyDisplay(interface_key::SECONDSCROLL_PAGEDOWN);

    z=xfooter+10-skeys.size();
    h=0,i=0,j=0,k=0;
    while(z>0){ if(z>1){h++;z-=2;} if(z>0){i++;z-=1;} if(z>0){j++;z-=1;} if(z>0){k++;z-=1;} }
    hblank.resize(h); iblank.resize(i); jblank.resize(j); kblank.resize(k);

    x = xmargin; y = dimey - 3;

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::LEAVESCREEN));
    OutputString(gry, x, y, ": Done.  ");

    OutputString(blk, x, y, hblank); OutputString(blk, x, y, kblank);

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CHANGETAB));
    OutputString(gry, x, y, ": Showing ");
    string cout=detailmode_shortnames[detail_mode];
    OutputString(lit, x, y, cout); // n=15?!!

    OutputString(blk, x, y, hblank); OutputString(blk, x, y, jblank);

    OutputString(10,x,y,Screen::getKeyDisplay(interface_key::SECONDSCROLL_PAGEUP));
    OutputString(10,x,y,Screen::getKeyDisplay(interface_key::SECONDSCROLL_PAGEDOWN));

    OutputString(lit, x, y, ": Sorting");
    cout=widesort_names[(int)widesort_mode];

    if(widesort_mode!=WIDESORT_NONE){
        OutputString(lit, x, y, cout);
        cout=stl_sprintf(", ");
        OutputString(lit, x, y, cout);
    }else
        OutputString(lit, x, y, cout);

    OutputString(blk, x, y, iblank);

    OutputString(10,x,y,Screen::getKeyDisplay(interface_key::SECONDSCROLL_UP));
    OutputString(10,x,y,Screen::getKeyDisplay(interface_key::SECONDSCROLL_DOWN));
    OutputString(lit, x, y, ": by ");

    cout=finesort_names[(int)finesort_mode];//"Sort fine "+
    OutputString(lit, x, y, cout);


    skeys =Screen::getKeyDisplay(interface_key::CUSTOM_SHIFT_C);
    skeys+=Screen::getKeyDisplay(interface_key::CUSTOM_D);
    skeys+=Screen::getKeyDisplay(interface_key::CUSTOM_N);
    skeys+=Screen::getKeyDisplay(interface_key::CUSTOM_SHIFT_N);
    skeys+=Screen::getKeyDisplay(interface_key::CUSTOM_T);
    skeys+=Screen::getKeyDisplay(interface_key::CUSTOM_SHIFT_T);
    skeys+=Screen::getKeyDisplay(interface_key::CUSTOM_E);
    skeys+=Screen::getKeyDisplay(interface_key::CUSTOM_B);
    skeys+=Screen::getKeyDisplay(interface_key::CUSTOM_P);
    skeys+=Screen::getKeyDisplay(interface_key::CUSTOM_SHIFT_P);
    skeys+=Screen::getKeyDisplay(interface_key::CUSTOM_H);

    z=xfooter+11-skeys.size();
    h=0,i=0,j=0,k=0;
    while(z>0){ if(z>1){h++;z-=2;} if(z>0){i++;z-=1;} if(z>0){j++;z-=1;} if(z>0){k++;z-=1;} }
    hblank.resize(h); iblank.resize(i); jblank.resize(j); kblank.resize(k);

    x = xmargin; y = dimey - 2;

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_SHIFT_C));
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_D));
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_N));
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_SHIFT_N));
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_T));
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_SHIFT_T));

    OutputString(gry, x, y, ": Mode, ");

    int bx=x;
    
    OutputString(blk, x, y, hblank); OutputString(blk, x, y, kblank);
      
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_E));
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_B));
    OutputString(gry, x, y, ": Nickname Unit/Batch,  ");

    OutputString(blk, x, y, iblank);

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_P));
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_SHIFT_P));
    OutputString(gry, x, y, ": Save/Apply Profession,  ");

    OutputString(blk, x, y, hblank); OutputString(blk, x, y, jblank);

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_H));
    OutputString(gry, x, y, ": Help");
    
    if(tip_show_timer>0){
      tip_show_timer--;
      cout=tip_settings[tip_show];
      OutputString(COLOR_YELLOW, bx, y, cout );
    }
}

df::unit *viewscreen_unitkeeperst::getSelectedUnit()
{
    // This query might be from the rename plugin
    do_refresh_names = true;
    return units[sel_row]->unit;
}

struct unitlist_hook : df::viewscreen_unitlistst
{
    typedef df::viewscreen_unitlistst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        if (input->count(interface_key::CUSTOM_K))
        {
            if (units[page].size())
            {
                Screen::show(dts::make_unique<viewscreen_unitkeeperst>(units[page], cursor_pos[page]), plugin_self);
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
            OutputString(12, x, y, Screen::getKeyDisplay(interface_key::CUSTOM_K));
            OutputString(15, x, y, ": Keeper  ");
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
    if (!Filesystem::isdir(CONFIG_DIR) && !Filesystem::mkdir(CONFIG_DIR))
    {
        out.printerr("dfkeeper: Could not create configuration folder: \"%s\"\n", CONFIG_DIR);
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
