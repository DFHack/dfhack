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
#include "df/creature_graphics_role.h"
#include "df/creature_raw.h"
#include "df/caste_raw.h"
#include "df/historical_entity.h"
#include "df/entity_raw.h"

#include "uicommon.h"
#include "listcolumn.h"

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

// The various skill rankings. Zero skill is hardcoded to "Not" and '-'.
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

#define NUM_COLUMNS (sizeof(columns) / sizeof(SkillColumn))

// All of the skill/labor columns we want to display.
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
// Farming/Related
    {5, 6, profession::BUTCHER, unit_labor::BUTCHER, job_skill::BUTCHER, "Bu"},
    {5, 6, profession::TANNER, unit_labor::TANNER, job_skill::TANNER, "Ta"},
    {5, 6, profession::PLANTER, unit_labor::PLANT, job_skill::PLANT, "Gr"},
    {5, 6, profession::DYER, unit_labor::DYER, job_skill::DYER, "Dy"},
    {5, 6, profession::SOAP_MAKER, unit_labor::SOAP_MAKER, job_skill::SOAP_MAKING, "So"},
    {5, 6, profession::WOOD_BURNER, unit_labor::BURN_WOOD, job_skill::WOOD_BURNING, "WB"},
    {5, 6, profession::POTASH_MAKER, unit_labor::POTASH_MAKING, job_skill::POTASH_MAKING, "Po"},
    {5, 6, profession::LYE_MAKER, unit_labor::LYE_MAKING, job_skill::LYE_MAKING, "Ly"},
    {5, 6, profession::MILLER, unit_labor::MILLER, job_skill::MILLING, "Ml"},
    {5, 6, profession::BREWER, unit_labor::BREWER, job_skill::BREWING, "Br"},
    {5, 6, profession::HERBALIST, unit_labor::HERBALIST, job_skill::HERBALISM, "He"},
    {5, 6, profession::THRESHER, unit_labor::PROCESS_PLANT, job_skill::PROCESSPLANTS, "Th"},
    {5, 6, profession::CHEESE_MAKER, unit_labor::MAKE_CHEESE, job_skill::CHEESEMAKING, "Ch"},
    {5, 6, profession::MILKER, unit_labor::MILK, job_skill::MILK, "Mk"},
    {5, 6, profession::SHEARER, unit_labor::SHEARER, job_skill::SHEARING, "Sh"},
    {5, 6, profession::SPINNER, unit_labor::SPINNER, job_skill::SPINNING, "Sp"},
    {5, 6, profession::COOK, unit_labor::COOK, job_skill::COOK, "Co"},
    {5, 6, profession::PRESSER, unit_labor::PRESSING, job_skill::PRESSING, "Pr"},
    {5, 6, profession::BEEKEEPER, unit_labor::BEEKEEPING, job_skill::BEEKEEPING, "Be"},
    {5, 6, profession::GELDER, unit_labor::GELD, job_skill::GELD, "Ge"},
// Fishing/Related
    {6, 1, profession::FISHERMAN, unit_labor::FISH, job_skill::FISH, "Fi"},
    {6, 1, profession::FISH_CLEANER, unit_labor::CLEAN_FISH, job_skill::PROCESSFISH, "Cl"},
    {6, 1, profession::FISH_DISSECTOR, unit_labor::DISSECT_FISH, job_skill::DISSECT_FISH, "Di"},
// Metalsmithing
    {7, 8, profession::FURNACE_OPERATOR, unit_labor::SMELT, job_skill::SMELT, "Fu"},
    {7, 8, profession::WEAPONSMITH, unit_labor::FORGE_WEAPON, job_skill::FORGE_WEAPON, "We"},
    {7, 8, profession::ARMORER, unit_labor::FORGE_ARMOR, job_skill::FORGE_ARMOR, "Ar"},
    {7, 8, profession::BLACKSMITH, unit_labor::FORGE_FURNITURE, job_skill::FORGE_FURNITURE, "Bl"},
    {7, 8, profession::METALCRAFTER, unit_labor::METAL_CRAFT, job_skill::METALCRAFT, "Cr"},
// Jewelry
    {8, 10, profession::GEM_CUTTER, unit_labor::CUT_GEM, job_skill::CUTGEM, "Cu"},
    {8, 10, profession::GEM_SETTER, unit_labor::ENCRUST_GEM, job_skill::ENCRUSTGEM, "Se"},
// Crafts
    {9, 9, profession::LEATHERWORKER, unit_labor::LEATHER, job_skill::LEATHERWORK, "Le"},
    {9, 9, profession::WOODCRAFTER, unit_labor::WOOD_CRAFT, job_skill::WOODCRAFT, "Wo"},
    {9, 9, profession::STONECRAFTER, unit_labor::STONE_CRAFT, job_skill::STONECRAFT, "St"},
    {9, 9, profession::BONE_CARVER, unit_labor::BONE_CARVE, job_skill::BONECARVE, "Bo"},
    {9, 9, profession::GLASSMAKER, unit_labor::GLASSMAKER, job_skill::GLASSMAKER, "Gl"},
    {9, 9, profession::WEAVER, unit_labor::WEAVER, job_skill::WEAVING, "We"},
    {9, 9, profession::CLOTHIER, unit_labor::CLOTHESMAKER, job_skill::CLOTHESMAKING, "Cl"},
    {9, 9, profession::STRAND_EXTRACTOR, unit_labor::EXTRACT_STRAND, job_skill::EXTRACT_STRAND, "Ad"},
    {9, 9, profession::POTTER, unit_labor::POTTERY, job_skill::POTTERY, "Po"},
    {9, 9, profession::GLAZER, unit_labor::GLAZING, job_skill::GLAZING, "Gl"},
    {9, 9, profession::WAX_WORKER, unit_labor::WAX_WORKING, job_skill::WAX_WORKING, "Wx"},
    {9, 9, profession::PAPERMAKER, unit_labor::PAPERMAKING, job_skill::PAPERMAKING, "Pa"},
    {9, 9, profession::BOOKBINDER, unit_labor::BOOKBINDING, job_skill::BOOKBINDING, "Bk"},
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
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::LEADERSHIP, "Ld"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::TEACHING, "Te"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::KNOWLEDGE_ACQUISITION, "St"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::DISCIPLINE, "Di"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::CONCENTRATION, "Co"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::SITUATIONAL_AWARENESS, "Ob"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::COORDINATION, "Cr"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::BALANCE, "Ba"},
    {15, 8, profession::NONE, unit_labor::NONE, job_skill::CLIMBING, "Cl"},
// Social
    {16, 3, profession::NONE, unit_labor::NONE, job_skill::PERSUASION, "Pe"},
    {16, 3, profession::NONE, unit_labor::NONE, job_skill::NEGOTIATION, "Ne"},
    {16, 3, profession::NONE, unit_labor::NONE, job_skill::JUDGING_INTENT, "Ju"},
    {16, 3, profession::NONE, unit_labor::NONE, job_skill::LYING, "Li"},
    {16, 3, profession::NONE, unit_labor::NONE, job_skill::INTIMIDATION, "In"},
    {16, 3, profession::NONE, unit_labor::NONE, job_skill::CONVERSATION, "Cn"},
    {16, 3, profession::NONE, unit_labor::NONE, job_skill::COMEDY, "Cm"},
    {16, 3, profession::NONE, unit_labor::NONE, job_skill::FLATTERY, "Fl"},
    {16, 3, profession::NONE, unit_labor::NONE, job_skill::CONSOLE, "Cs"},
    {16, 3, profession::NONE, unit_labor::NONE, job_skill::PACIFY, "Pc"},
// Noble
    {17, 5, profession::TRADER, unit_labor::NONE, job_skill::APPRAISAL, "Ap"},
    {17, 5, profession::ADMINISTRATOR, unit_labor::NONE, job_skill::ORGANIZATION, "Or"},
    {17, 5, profession::CLERK, unit_labor::NONE, job_skill::RECORD_KEEPING, "RK"},
// Miscellaneous
    {18, 3, profession::NONE, unit_labor::NONE, job_skill::THROW, "Th"},
    {18, 3, profession::NONE, unit_labor::NONE, job_skill::CRUTCH_WALK, "CW"},
    {18, 3, profession::NONE, unit_labor::NONE, job_skill::SWIMMING, "Sw"},
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
    string profession;
    int8_t color;
    int active_index;
    string squad_effective_name;
    string squad_info;
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
};

enum detail_cols {
    DETAIL_MODE_PROFESSION,
    DETAIL_MODE_SQUAD,
    DETAIL_MODE_JOB
};
enum altsort_mode {
    ALTSORT_NAME,
    ALTSORT_SELECTED,
    ALTSORT_DETAIL,
    ALTSORT_STRESS,
    ALTSORT_ARRIVAL,
    ALTSORT_MAX
};

string itos (int n)
{
    stringstream ss;
    ss << n;
    return ss.str();
}

bool descending;
df::job_skill sort_skill;
df::unit_labor sort_labor;

bool sortByName (const UnitInfo *d1, const UnitInfo *d2)
{
    if (descending)
        return (d1->name > d2->name);
    else
        return (d1->name < d2->name);
}

bool sortByProfession (const UnitInfo *d1, const UnitInfo *d2)
{
    if (descending)
        return (d1->profession > d2->profession);
    else
        return (d1->profession < d2->profession);
}

bool sortBySquad (const UnitInfo *d1, const UnitInfo *d2)
{
    bool gt = false;
    if (d1->unit->military.squad_id == -1 && d2->unit->military.squad_id == -1)
        gt = d1->name > d2->name;
    else if (d1->unit->military.squad_id == -1)
        gt = true;
    else if (d2->unit->military.squad_id == -1)
        gt = false;
    else if (d1->unit->military.squad_id != d2->unit->military.squad_id)
        gt = d1->squad_effective_name > d2->squad_effective_name;
    else
        gt = d1->unit->military.squad_position > d2->unit->military.squad_position;
    return descending ? gt : !gt;
}

bool sortByJob (const UnitInfo *d1, const UnitInfo *d2)
{
    if (d1->job_mode != d2->job_mode)
    {
        if (descending)
            return int(d1->job_mode) < int(d2->job_mode);
        else
            return int(d1->job_mode) > int(d2->job_mode);
    }

    if (descending)
        return d1->job_desc > d2->job_desc;
    else
        return d1->job_desc < d2->job_desc;
}

bool sortByStress (const UnitInfo *d1, const UnitInfo *d2)
{
    if (!d1->unit->status.current_soul)
        return !descending;
    if (!d2->unit->status.current_soul)
        return descending;

    if (descending)
        return (d1->unit->status.current_soul->personality.stress_level > d2->unit->status.current_soul->personality.stress_level);
    else
        return (d1->unit->status.current_soul->personality.stress_level < d2->unit->status.current_soul->personality.stress_level);
}

bool sortByArrival (const UnitInfo *d1, const UnitInfo *d2)
{
    if (descending)
        return (d1->active_index > d2->active_index);
    else
        return (d1->active_index < d2->active_index);
}

bool sortBySkill (const UnitInfo *d1, const UnitInfo *d2)
{
    if (sort_skill != job_skill::NONE)
    {
        if (!d1->unit->status.current_soul)
            return !descending;
        if (!d2->unit->status.current_soul)
            return descending;
        df::unit_skill *s1 = binsearch_in_vector<df::unit_skill,df::job_skill>(d1->unit->status.current_soul->skills, &df::unit_skill::id, sort_skill);
        df::unit_skill *s2 = binsearch_in_vector<df::unit_skill,df::job_skill>(d2->unit->status.current_soul->skills, &df::unit_skill::id, sort_skill);
        int l1 = s1 ? s1->rating : 0;
        int l2 = s2 ? s2->rating : 0;
        int e1 = s1 ? s1->experience : 0;
        int e2 = s2 ? s2->experience : 0;
        if (descending)
        {
            if (l1 != l2)
                return l1 > l2;
            if (e1 != e2)
                return e1 > e2;
        }
        else
        {
            if (l1 != l2)
                return l1 < l2;
            if (e1 != e2)
                return e1 < e2;
        }
    }
    if (sort_labor != unit_labor::NONE)
    {
        if (descending)
            return d1->unit->status.labors[sort_labor] > d2->unit->status.labors[sort_labor];
        else
            return d1->unit->status.labors[sort_labor] < d2->unit->status.labors[sort_labor];
    }
    return false;
}

bool sortBySelected (const UnitInfo *d1, const UnitInfo *d2)
{
    return descending ? (d1->selected > d2->selected) : (d1->selected < d2->selected);
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
                ret += world->raws.language.words[name.words[i]]->forms[name.parts_of_speech[i]];
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
        for (size_t i = 0; i < NUM_COLUMNS; i++)
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

            for (size_t i = 0; i < NUM_COLUMNS; i++)
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

        for (size_t i = 0; i < NUM_COLUMNS; i++)
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

        for (size_t i = 0; i < NUM_COLUMNS; i++)
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
        for (size_t i = 0; i < NUM_COLUMNS; i++)
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
        formatter.add_option("i", "Position in list", unit_ops::get_list_id);
        formatter.add_option("pi", "Position in list, among dwarves with same profession", unit_ops::get_list_id_prof);
        formatter.add_option("gi", "Position in list, among dwarves in same profession group", unit_ops::get_list_id_group);
        formatter.add_option("ri", "Raw unit ID", unit_ops::get_unit_id);
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
        size_t max_x = gps->dimx - 2;
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
        size_t max_x = gps->dimx - 2;
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
    DISP_COLUMN_STRESS,
    DISP_COLUMN_SELECTED,
    DISP_COLUMN_NAME,
    DISP_COLUMN_DETAIL,
    DISP_COLUMN_LABORS,
    DISP_COLUMN_MAX,
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
    void resize(int w, int h) { calcSize(); }

    void help() { }

    std::string getFocusString() { return "unitlabors"; }

    df::unit *getSelectedUnit();

    viewscreen_unitlaborsst(vector<df::unit*> &src, int cursor_pos);
    ~viewscreen_unitlaborsst() { };

protected:
    vector<UnitInfo *> units;
    altsort_mode altsort;

    bool do_refresh_names;
    int detail_mode;
    int first_row, sel_row, num_rows;
    int first_column, sel_column;
    int last_selection;

    int col_widths[DISP_COLUMN_MAX];
    int col_offsets[DISP_COLUMN_MAX];

    void refreshNames();
    void calcIDs();
    void calcSize ();
};

viewscreen_unitlaborsst::viewscreen_unitlaborsst(vector<df::unit*> &src, int cursor_pos)
{
    std::map<df::unit*,int> active_idx;
    auto &active = world->units.active;
    for (size_t i = 0; i < active.size(); i++)
        active_idx[active[i]] = i;

    for (size_t i = 0; i < src.size(); i++)
    {
        df::unit *unit = src[i];
        if (!unit)
        {
            if (cursor_pos > int(i))
                cursor_pos--;
            continue;
        }

        UnitInfo *cur = new UnitInfo;

        cur->ids.init();
        cur->unit = unit;
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

        units.push_back(cur);
    }
    altsort = ALTSORT_NAME;
    detail_mode = DETAIL_MODE_PROFESSION;
    first_column = sel_column = 0;

    refreshNames();
    calcIDs();

    first_row = 0;
    sel_row = cursor_pos;
    calcSize();

    // recalculate first_row to roughly match the original layout
    first_row = 0;
    while (first_row < sel_row - num_rows + 1)
        first_row += num_rows + 2;
    // make sure the selection stays visible
    if (first_row > sel_row)
        first_row = sel_row - num_rows + 1;
    // don't scroll beyond the end
    if (first_row > int(units.size()) - num_rows)
        first_row = units.size() - num_rows;

    last_selection = -1;
}

void viewscreen_unitlaborsst::calcIDs()
{
    static int list_prof_ids[NUM_COLUMNS];
    static int list_group_ids[NUM_COLUMNS];
    static map<df::profession, int> group_map;
    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;
        for (size_t i = 0; i < NUM_COLUMNS; i++)
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
            cur->squad_info = stl_sprintf("%i", unit->military.squad_position + 1) + "." + cur->squad_effective_name;
        } else {
            cur->squad_effective_name = "";
            cur->squad_info = "";
        }
    }
    calcSize();
}

void viewscreen_unitlaborsst::calcSize()
{
    auto dim = Screen::getWindowSize();

    num_rows = dim.y - 11;
    if (num_rows > int(units.size()))
        num_rows = units.size();

    int num_columns = dim.x - DISP_COLUMN_MAX - 1;

    // min/max width of columns
    int col_minwidth[DISP_COLUMN_MAX];
    int col_maxwidth[DISP_COLUMN_MAX];
    col_minwidth[DISP_COLUMN_STRESS] = 6;
    col_maxwidth[DISP_COLUMN_STRESS] = 6;
    col_minwidth[DISP_COLUMN_SELECTED] = 1;
    col_maxwidth[DISP_COLUMN_SELECTED] = 1;
    col_minwidth[DISP_COLUMN_NAME] = 16;
    col_maxwidth[DISP_COLUMN_NAME] = 16;        // adjusted in the loop below
    col_minwidth[DISP_COLUMN_DETAIL] = 10;
    col_maxwidth[DISP_COLUMN_DETAIL] = 10;  // adjusted in the loop below
    col_minwidth[DISP_COLUMN_LABORS] = 1;
    col_maxwidth[DISP_COLUMN_LABORS] = NUM_COLUMNS;

    // get max_name/max_prof from strings length
    for (size_t i = 0; i < units.size(); i++)
    {
        if (size_t(col_maxwidth[DISP_COLUMN_NAME]) < units[i]->name.size())
            col_maxwidth[DISP_COLUMN_NAME] = units[i]->name.size();

        size_t detail_cmp;
        if (detail_mode == DETAIL_MODE_SQUAD) {
            detail_cmp = units[i]->squad_info.size();
        } else if (detail_mode == DETAIL_MODE_JOB) {
            detail_cmp = units[i]->job_desc.size();
        } else {
            detail_cmp = units[i]->profession.size();
        }
        if (size_t(col_maxwidth[DISP_COLUMN_DETAIL]) < detail_cmp)
            col_maxwidth[DISP_COLUMN_DETAIL] = detail_cmp;
    }

    // check how much room we have
    int width_min = 0, width_max = 0;
    for (int i = 0; i < DISP_COLUMN_MAX; i++)
    {
        width_min += col_minwidth[i];
        width_max += col_maxwidth[i];
    }

    if (width_max <= num_columns)
    {
        // lots of space, distribute leftover (except last column)
        int col_margin   = (num_columns - width_max) / (DISP_COLUMN_MAX-1);
        int col_margin_r = (num_columns - width_max) % (DISP_COLUMN_MAX-1);
        for (int i = DISP_COLUMN_MAX-1; i>=0; i--)
        {
            col_widths[i] = col_maxwidth[i];

            if (i < DISP_COLUMN_MAX-1)
            {
                col_widths[i] += col_margin;

                if (col_margin_r)
                {
                    col_margin_r--;
                    col_widths[i]++;
                }
            }
        }
    }
    else if (width_min <= num_columns)
    {
        // constrained, give between min and max to every column
        int space = num_columns - width_min;
        // max size columns not yet seen may consume
        int next_consume_max = width_max - width_min;

        for (int i = 0; i < DISP_COLUMN_MAX; i++)
        {
            // divide evenly remaining space
            int col_margin = space / (DISP_COLUMN_MAX-i);

            // take more if the columns after us cannot
            next_consume_max -= (col_maxwidth[i]-col_minwidth[i]);
            if (col_margin < space-next_consume_max)
                col_margin = space - next_consume_max;

            // no more than maxwidth
            if (col_margin > col_maxwidth[i] - col_minwidth[i])
                col_margin = col_maxwidth[i] - col_minwidth[i];

            col_widths[i] = col_minwidth[i] + col_margin;

            space -= col_margin;
        }
    }
    else
    {
        // should not happen, min screen is 80x25
        int space = num_columns;
        for (int i = 0; i < DISP_COLUMN_MAX; i++)
        {
            col_widths[i] = space / (DISP_COLUMN_MAX-i);
            space -= col_widths[i];
        }
    }

    for (int i = 0; i < DISP_COLUMN_MAX; i++)
    {
        if (i == 0)
            col_offsets[i] = 1;
        else
            col_offsets[i] = col_offsets[i - 1] + col_widths[i - 1] + 1;
    }

    // don't adjust scroll position immediately after the window opened
    if (units.size() == 0)
        return;

    // if the window grows vertically, scroll upward to eliminate blank rows from the bottom
    if (first_row > int(units.size()) - num_rows)
        first_row = units.size() - num_rows;

    // if it shrinks vertically, scroll downward to keep the cursor visible
    if (first_row < sel_row - num_rows + 1)
        first_row = sel_row - num_rows + 1;

    // if the window grows horizontally, scroll to the left to eliminate blank columns from the right
    if (first_column > int(NUM_COLUMNS) - col_widths[DISP_COLUMN_LABORS])
        first_column = NUM_COLUMNS - col_widths[DISP_COLUMN_LABORS];

    // if it shrinks horizontally, scroll to the right to keep the cursor visible
    if (first_column < sel_column - col_widths[DISP_COLUMN_LABORS] + 1)
        first_column = sel_column - col_widths[DISP_COLUMN_LABORS] + 1;
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

    int old_sel_row = sel_row;

    if (events->count(interface_key::CURSOR_UP) || events->count(interface_key::CURSOR_UPLEFT) || events->count(interface_key::CURSOR_UPRIGHT))
        sel_row--;
    if (events->count(interface_key::CURSOR_UP_FAST) || events->count(interface_key::CURSOR_UPLEFT_FAST) || events->count(interface_key::CURSOR_UPRIGHT_FAST))
        sel_row -= 10;
    if (events->count(interface_key::CURSOR_DOWN) || events->count(interface_key::CURSOR_DOWNLEFT) || events->count(interface_key::CURSOR_DOWNRIGHT))
        sel_row++;
    if (events->count(interface_key::CURSOR_DOWN_FAST) || events->count(interface_key::CURSOR_DOWNLEFT_FAST) || events->count(interface_key::CURSOR_DOWNRIGHT_FAST))
        sel_row += 10;

    if ((sel_row > 0) && events->count(interface_key::CURSOR_UP_Z_AUX))
    {
        sel_row = 0;
    }
    if ((size_t(sel_row) < units.size()-1) && events->count(interface_key::CURSOR_DOWN_Z_AUX))
    {
        sel_row = units.size()-1;
    }

    if (sel_row < 0)
    {
        if (old_sel_row == 0 && events->count(interface_key::CURSOR_UP))
            sel_row = units.size() - 1;
        else
            sel_row = 0;
    }

    if (size_t(sel_row) > units.size() - 1)
    {
        if (size_t(old_sel_row) == units.size()-1 && events->count(interface_key::CURSOR_DOWN))
            sel_row = 0;
        else
            sel_row = units.size() - 1;
    }

    if (events->count(interface_key::STRING_A000))
        sel_row = 0;

    if (sel_row < first_row)
        first_row = sel_row;
    if (first_row < sel_row - num_rows + 1)
        first_row = sel_row - num_rows + 1;

    if (events->count(interface_key::CURSOR_LEFT) || events->count(interface_key::CURSOR_UPLEFT) || events->count(interface_key::CURSOR_DOWNLEFT))
        sel_column--;
    if (events->count(interface_key::CURSOR_LEFT_FAST) || events->count(interface_key::CURSOR_UPLEFT_FAST) || events->count(interface_key::CURSOR_DOWNLEFT_FAST))
        sel_column -= 10;
    if (events->count(interface_key::CURSOR_RIGHT) || events->count(interface_key::CURSOR_UPRIGHT) || events->count(interface_key::CURSOR_DOWNRIGHT))
        sel_column++;
    if (events->count(interface_key::CURSOR_RIGHT_FAST) || events->count(interface_key::CURSOR_UPRIGHT_FAST) || events->count(interface_key::CURSOR_DOWNRIGHT_FAST))
        sel_column += 10;

    if ((sel_column != 0) && events->count(interface_key::CURSOR_UP_Z))
    {
        // go to beginning of current column group; if already at the beginning, go to the beginning of the previous one
        sel_column--;
        int cur = columns[sel_column].group;
        while ((sel_column > 0) && columns[sel_column - 1].group == cur)
            sel_column--;
    }
    if ((sel_column != NUM_COLUMNS - 1) && events->count(interface_key::CURSOR_DOWN_Z))
    {
        // go to beginning of next group
        int cur = columns[sel_column].group;
        size_t next = sel_column+1;
        while ((next < NUM_COLUMNS) && (columns[next].group == cur))
            next++;
        if ((next < NUM_COLUMNS) && (columns[next].group != cur))
            sel_column = next;
    }

    if (events->count(interface_key::STRING_A000))
        sel_column = 0;

    if (sel_column < 0)
        sel_column = 0;
    if (size_t(sel_column) > NUM_COLUMNS - 1)
        sel_column = NUM_COLUMNS - 1;

    if (events->count(interface_key::CURSOR_DOWN_Z) || events->count(interface_key::CURSOR_UP_Z))
    {
        // when moving by group, ensure the whole group is shown onscreen
        int endgroup_column = sel_column;
        while ((size_t(endgroup_column) < NUM_COLUMNS-1) && columns[endgroup_column+1].group == columns[sel_column].group)
            endgroup_column++;

        if (first_column < endgroup_column - col_widths[DISP_COLUMN_LABORS] + 1)
            first_column = endgroup_column - col_widths[DISP_COLUMN_LABORS] + 1;
    }

    if (sel_column < first_column)
        first_column = sel_column;
    if (first_column < sel_column - col_widths[DISP_COLUMN_LABORS] + 1)
        first_column = sel_column - col_widths[DISP_COLUMN_LABORS] + 1;

    int input_row = sel_row;
    int input_column = sel_column;
    int input_sort = altsort;

    // Translate mouse input to appropriate keyboard input
    if (enabler->tracking_on && gps->mouse_x != -1 && gps->mouse_y != -1)
    {
        int click_header = DISP_COLUMN_MAX; // group ID of the column header clicked
        int click_body = DISP_COLUMN_MAX; // group ID of the column body clicked

        int click_unit = -1; // Index into units[] (-1 if out of range)
        int click_labor = -1; // Index into columns[] (-1 if out of range)

        for (int i = 0; i < DISP_COLUMN_MAX; i++)
        {
            if ((gps->mouse_x >= col_offsets[i]) &&
                (gps->mouse_x < col_offsets[i] + col_widths[i]))
            {
                if ((gps->mouse_y >= 1) && (gps->mouse_y <= 2))
                    click_header = i;
                if ((gps->mouse_y >= 4) && (gps->mouse_y < 4 + num_rows))
                    click_body = i;
            }
        }

        if ((gps->mouse_x >= col_offsets[DISP_COLUMN_LABORS]) &&
            (gps->mouse_x < col_offsets[DISP_COLUMN_LABORS] + col_widths[DISP_COLUMN_LABORS]))
            click_labor = gps->mouse_x - col_offsets[DISP_COLUMN_LABORS] + first_column;
        if ((gps->mouse_y >= 4) && (gps->mouse_y < 4 + num_rows))
            click_unit = gps->mouse_y - 4 + first_row;

        switch (click_header)
        {
        case DISP_COLUMN_STRESS:
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                input_sort = ALTSORT_STRESS;
                if (enabler->mouse_lbut)
                    events->insert(interface_key::SECONDSCROLL_PAGEUP);
                if (enabler->mouse_rbut)
                    events->insert(interface_key::SECONDSCROLL_PAGEDOWN);
            }
            break;

        case DISP_COLUMN_SELECTED:
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                input_sort = ALTSORT_SELECTED;
                if (enabler->mouse_lbut)
                    events->insert(interface_key::SECONDSCROLL_PAGEUP);
                if (enabler->mouse_rbut)
                    events->insert(interface_key::SECONDSCROLL_PAGEDOWN);
            }
            break;

        case DISP_COLUMN_NAME:
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                input_sort = ALTSORT_NAME;
                if (enabler->mouse_lbut)
                    events->insert(interface_key::SECONDSCROLL_PAGEDOWN);
                if (enabler->mouse_rbut)
                    events->insert(interface_key::SECONDSCROLL_PAGEUP);
            }
            break;

        case DISP_COLUMN_DETAIL:
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                input_sort = ALTSORT_DETAIL;
                if (enabler->mouse_lbut)
                    events->insert(interface_key::SECONDSCROLL_PAGEDOWN);
                if (enabler->mouse_rbut)
                    events->insert(interface_key::SECONDSCROLL_PAGEUP);
            }
            break;

        case DISP_COLUMN_LABORS:
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                input_column = click_labor;
                if (enabler->mouse_lbut)
                    events->insert(interface_key::SECONDSCROLL_UP);
                if (enabler->mouse_rbut)
                    events->insert(interface_key::SECONDSCROLL_DOWN);
            }
            break;
        }

        switch (click_body)
        {
        case DISP_COLUMN_STRESS:
            // do nothing
            break;

        case DISP_COLUMN_SELECTED:
            // left-click to select, right-click or shift-click to extend selection
            if (enabler->mouse_rbut || (enabler->mouse_lbut && (modstate & DFH_MOD_SHIFT)))
            {
                input_row = click_unit;
                events->insert(interface_key::CUSTOM_SHIFT_X);
            }
            else if (enabler->mouse_lbut)
            {
                input_row = click_unit;
                events->insert(interface_key::CUSTOM_X);
            }
            break;

        case DISP_COLUMN_NAME:
        case DISP_COLUMN_DETAIL:
            // left-click to view, right-click to zoom
            if (enabler->mouse_lbut)
            {
                input_row = click_unit;
                events->insert(interface_key::UNITJOB_VIEW_UNIT);
            }
            if (enabler->mouse_rbut)
            {
                input_row = click_unit;
                events->insert(interface_key::UNITJOB_ZOOM_CRE);
            }
            break;

        case DISP_COLUMN_LABORS:
            // left-click to toggle, right-click to just highlight
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                if (enabler->mouse_lbut)
                {
                    input_row = click_unit;
                    input_column = click_labor;
                    events->insert(interface_key::SELECT);
                }
                if (enabler->mouse_rbut)
                {
                    sel_row = click_unit;
                    sel_column = click_labor;
                }
            }
            break;
        }
        enabler->mouse_lbut = enabler->mouse_rbut = 0;
    }

    UnitInfo *cur = units[input_row];
    df::unit *unit = cur->unit;
    df::unit_labor cur_labor = columns[input_column].labor;
    if (events->count(interface_key::SELECT) && (cur->allowEdit) && Units::isValidLabor(unit, cur_labor))
    {
        const SkillColumn &col = columns[input_column];
        bool newstatus = !unit->status.labors[col.labor];
        if (col.special)
        {
            if (newstatus)
            {
                for (size_t i = 0; i < NUM_COLUMNS; i++)
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
        const SkillColumn &col = columns[input_column];
        bool newstatus = !unit->status.labors[col.labor];
        for (size_t i = 0; i < NUM_COLUMNS; i++)
        {
            if (columns[i].group != col.group)
                continue;
            if (!Units::isValidLabor(unit, columns[i].labor))
                continue;
            if (columns[i].special)
            {
                if (newstatus)
                {
                    for (size_t j = 0; j < NUM_COLUMNS; j++)
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

    if (events->count(interface_key::SECONDSCROLL_UP) || events->count(interface_key::SECONDSCROLL_DOWN))
    {
        descending = events->count(interface_key::SECONDSCROLL_UP);
        sort_skill = columns[input_column].skill;
        sort_labor = columns[input_column].labor;
        std::stable_sort(units.begin(), units.end(), sortBySkill);
        calcIDs();
    }

    if (events->count(interface_key::SECONDSCROLL_PAGEUP) || events->count(interface_key::SECONDSCROLL_PAGEDOWN))
    {
        descending = events->count(interface_key::SECONDSCROLL_PAGEUP);
        switch (input_sort)
        {
        case ALTSORT_NAME:
            std::stable_sort(units.begin(), units.end(), sortByName);
            break;
        case ALTSORT_SELECTED:
            std::stable_sort(units.begin(), units.end(), sortBySelected);
            break;
        case ALTSORT_DETAIL:
            if (detail_mode == DETAIL_MODE_SQUAD) {
                std::stable_sort(units.begin(), units.end(), sortBySquad);
            } else if (detail_mode == DETAIL_MODE_JOB) {
                std::stable_sort(units.begin(), units.end(), sortByJob);
            } else {
                std::stable_sort(units.begin(), units.end(), sortByProfession);
            }
            break;
        case ALTSORT_STRESS:
            std::stable_sort(units.begin(), units.end(), sortByStress);
            break;
        case ALTSORT_ARRIVAL:
            std::stable_sort(units.begin(), units.end(), sortByArrival);
            break;
        }
        calcIDs();
    }
    if (events->count(interface_key::CHANGETAB))
    {
        switch (altsort)
        {
        case ALTSORT_NAME:
            altsort = ALTSORT_SELECTED;
            break;
        case ALTSORT_SELECTED:
            altsort = ALTSORT_DETAIL;
            break;
        case ALTSORT_DETAIL:
            altsort = ALTSORT_STRESS;
            break;
        case ALTSORT_STRESS:
            altsort = ALTSORT_ARRIVAL;
            break;
        case ALTSORT_ARRIVAL:
            altsort = ALTSORT_NAME;
            break;
        case ALTSORT_MAX:
            break;
        }
    }
    if (events->count(interface_key::OPTION20))
    {
        if (detail_mode == DETAIL_MODE_SQUAD) {
            detail_mode = DETAIL_MODE_JOB;
        } else if (detail_mode == DETAIL_MODE_JOB) {
            detail_mode = DETAIL_MODE_PROFESSION;
        } else {
            detail_mode = DETAIL_MODE_SQUAD;
        }
    }

    if (events->count(interface_key::CUSTOM_SHIFT_X))
    {
        if (last_selection == -1 || last_selection == input_row)
            events->insert(interface_key::CUSTOM_X);
        else
        {
            for (int i = std::min(input_row, last_selection);
                 i <= std::max(input_row, last_selection);
                 i++)
            {
                if (i == last_selection) continue;
                if (!units[i]->allowEdit) continue;
                units[i]->selected = units[last_selection]->selected;
            }
        }
    }

    if (events->count(interface_key::CUSTOM_X) && cur->allowEdit)
    {
        cur->selected = !cur->selected;
        last_selection = input_row;
    }

    if (events->count(interface_key::CUSTOM_A) || events->count(interface_key::CUSTOM_SHIFT_A))
    {
        for (size_t i = 0; i < units.size(); i++)
            if (units[i]->allowEdit)
                units[i]->selected = (bool)events->count(interface_key::CUSTOM_A);
    }

    if (events->count(interface_key::CUSTOM_B))
    {
        Screen::show(dts::make_unique<viewscreen_unitbatchopst>(units, true, &do_refresh_names), plugin_self);
    }

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

    if (VIRTUAL_CAST_VAR(unitlist, df::viewscreen_unitlistst, parent))
    {
        if (events->count(interface_key::UNITJOB_VIEW_UNIT) || events->count(interface_key::UNITJOB_ZOOM_CRE))
        {
            for (size_t i = 0; i < unitlist->units[unitlist->page].size(); i++)
            {
                if (unitlist->units[unitlist->page][i] == units[input_row]->unit)
                {
                    unitlist->cursor_pos[unitlist->page] = i;
                    unitlist->feed(events);
                    if (Screen::isDismissed(unitlist))
                        Screen::dismiss(this);
                    else
                        do_refresh_names = true;
                    break;
                }
            }
        }
    }
}

void viewscreen_unitlaborsst::render()
{
    if (Screen::isDismissed(this))
        return;

    dfhack_viewscreen::render();

    auto dim = Screen::getWindowSize();

    Screen::clear();
    Screen::drawBorder("  Dwarf Manipulator - Manage Labors  ");

    Screen::paintString(Screen::Pen(' ', 7, 0), col_offsets[DISP_COLUMN_STRESS], 2, "Stress");
    Screen::paintTile(Screen::Pen('\373', 7, 0), col_offsets[DISP_COLUMN_SELECTED], 2);
    Screen::paintString(Screen::Pen(' ', 7, 0), col_offsets[DISP_COLUMN_NAME], 2, "Name");

    string detail_str;
    if (detail_mode == DETAIL_MODE_SQUAD) {
        detail_str = "Squad";
    } else if (detail_mode == DETAIL_MODE_JOB) {
        detail_str = "Job";
    } else {
        detail_str = "Profession";
    }
    Screen::paintString(Screen::Pen(' ', 7, 0), col_offsets[DISP_COLUMN_DETAIL], 2, detail_str);

    for (int col = 0; col < col_widths[DISP_COLUMN_LABORS]; col++)
    {
        int col_offset = col + first_column;
        if (size_t(col_offset) >= NUM_COLUMNS)
            break;

        int8_t fg = columns[col_offset].color;
        int8_t bg = 0;

        if (col_offset == sel_column)
        {
            fg = 0;
            bg = 7;
        }

        Screen::paintTile(Screen::Pen(columns[col_offset].label[0], fg, bg), col_offsets[DISP_COLUMN_LABORS] + col, 1);
        Screen::paintTile(Screen::Pen(columns[col_offset].label[1], fg, bg), col_offsets[DISP_COLUMN_LABORS] + col, 2);
        df::profession profession = columns[col_offset].profession;
        if ((profession != profession::NONE) && (ui->race_id != -1))
        {
            auto graphics = world->raws.creatures.all[ui->race_id]->graphics;
            Screen::paintTile(
                Screen::Pen(' ', fg, 0,
                    graphics.profession_add_color[creature_graphics_role::DEFAULT][profession],
                    graphics.profession_texpos[creature_graphics_role::DEFAULT][profession]),
                col_offsets[DISP_COLUMN_LABORS] + col, 3);
        }
    }

    for (int row = 0; row < num_rows; row++)
    {
        int row_offset = row + first_row;
        if (size_t(row_offset) >= units.size())
            break;

        UnitInfo *cur = units[row_offset];
        df::unit *unit = cur->unit;
        int8_t fg = 15, bg = 0;

        int stress_lvl = unit->status.current_soul ? unit->status.current_soul->personality.stress_level : 0;
        const vector<UIColor> stress_colors {
            13, // 5:1
            12, // 4:1
            14, // 6:1
            2,  // 2:0
            10  // 2:1 (default)
        };
        fg = vector_get(stress_colors, Units::getStressCategoryRaw(stress_lvl), stress_colors.back());

        // cap at 6 digits
        if (stress_lvl < -99999) stress_lvl = -99999;
        if (stress_lvl > 999999) stress_lvl = 999999;
        string stress = stl_sprintf("%6i", stress_lvl);

        Screen::paintString(Screen::Pen(' ', fg, bg), col_offsets[DISP_COLUMN_STRESS], 4 + row, stress);

        Screen::paintTile(
            (cur->selected) ? Screen::Pen('\373', COLOR_LIGHTGREEN, 0) :
                ((cur->allowEdit) ? Screen::Pen('-', COLOR_DARKGREY, 0) : Screen::Pen('-', COLOR_RED, 0)),
            col_offsets[DISP_COLUMN_SELECTED], 4 + row);

        fg = 15;
        if (row_offset == sel_row)
        {
            fg = 0;
            bg = 7;
        }

        string name = cur->name;
        name.resize(col_widths[DISP_COLUMN_NAME]);
        Screen::paintString(Screen::Pen(' ', fg, bg), col_offsets[DISP_COLUMN_NAME], 4 + row, name);

        bg = 0;
        if (detail_mode == DETAIL_MODE_SQUAD) {
            fg = 11;
            detail_str = cur->squad_info;
        } else if (detail_mode == DETAIL_MODE_JOB) {
            detail_str = cur->job_desc;
            if (cur->job_mode == UnitInfo::IDLE) {
                fg = COLOR_YELLOW;
            } else if (cur->job_mode == UnitInfo::SOCIAL) {
                fg = COLOR_LIGHTGREEN;
            } else {
                fg = COLOR_LIGHTCYAN;
            }
        } else {
            fg = cur->color;
            detail_str = cur->profession;
        }
        detail_str.resize(col_widths[DISP_COLUMN_DETAIL]);
        Screen::paintString(Screen::Pen(' ', fg, bg), col_offsets[DISP_COLUMN_DETAIL], 4 + row, detail_str);

        // Print unit's skills and labor assignments
        for (int col = 0; col < col_widths[DISP_COLUMN_LABORS]; col++)
        {
            int col_offset = col + first_column;
            fg = 15;
            bg = 0;
            uint8_t c = 0xFA;
            if ((col_offset == sel_column) && (row_offset == sel_row))
                fg = 9;
            if (columns[col_offset].skill != job_skill::NONE)
            {
                df::unit_skill *skill = NULL;
                if (unit->status.current_soul)
                    skill = binsearch_in_vector<df::unit_skill,df::job_skill>(unit->status.current_soul->skills, &df::unit_skill::id, columns[col_offset].skill);
                if ((skill != NULL) && (skill->rating || skill->experience))
                {
                    size_t level = skill->rating;
                    if (level > NUM_SKILL_LEVELS - 1)
                        level = NUM_SKILL_LEVELS - 1;
                    c = skill_levels[level].abbrev;
                }
                else
                    c = '-';
            }
            if (columns[col_offset].labor != unit_labor::NONE)
            {
                if (unit->status.labors[columns[col_offset].labor])
                {
                    bg = 7;
                    if (columns[col_offset].skill == job_skill::NONE)
                        c = 0xF9;
                }
            }
            else
                bg = 3;
            Screen::paintTile(Screen::Pen(c, fg, bg), col_offsets[DISP_COLUMN_LABORS] + col, 4 + row);
        }
    }

    UnitInfo *cur = units[sel_row];
    bool canToggle = false;
    if (cur != NULL)
    {
        df::unit *unit = cur->unit;
        int x = 1, y = 3 + num_rows + 2;
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
                str += " Enabled";
            else
                str += " Not Enabled";
        }
        else
        {
            df::unit_skill *skill = NULL;
            if (unit->status.current_soul)
                skill = binsearch_in_vector<df::unit_skill,df::job_skill>(unit->status.current_soul->skills, &df::unit_skill::id, columns[sel_column].skill);
            if (skill)
            {
                size_t level = skill->rating;
                if (level > NUM_SKILL_LEVELS - 1)
                    level = NUM_SKILL_LEVELS - 1;
                str = stl_sprintf("%s %s", skill_levels[level].name, ENUM_ATTR_STR(job_skill, caption_noun, columns[sel_column].skill));
                if (level != NUM_SKILL_LEVELS - 1)
                    str += stl_sprintf(" (%d/%d)", skill->experience, skill_levels[level].points);
            }
            else
                str = stl_sprintf("Not %s (0/500)", ENUM_ATTR_STR(job_skill, caption_noun, columns[sel_column].skill));
        }
        Screen::paintString(Screen::Pen(' ', 9, 0), x, y, str);

        if (cur->unit->military.squad_id > -1) {

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
    OutputString(canToggle ? 15 : 8, x, y, ": Toggle labor, ");

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::SELECT_ALL));
    OutputString(canToggle ? 15 : 8, x, y, ": Toggle Group, ");

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::UNITJOB_VIEW_UNIT));
    OutputString(15, x, y, ": ViewCre, ");

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::UNITJOB_ZOOM_CRE));
    OutputString(15, x, y, ": Zoom-Cre");

    x = 2; y = dim.y - 3;
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::LEAVESCREEN));
    OutputString(15, x, y, ": Done, ");

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::OPTION20));
    OutputString(15, x, y, ": Toggle View, ");

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::SECONDSCROLL_DOWN));
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::SECONDSCROLL_UP));
    OutputString(15, x, y, ": Sort by Skill, ");

    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::SECONDSCROLL_PAGEDOWN));
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::SECONDSCROLL_PAGEUP));
    OutputString(15, x, y, ": Sort by (");
    OutputString(10, x, y, Screen::getKeyDisplay(interface_key::CHANGETAB));
    OutputString(15, x, y, ") ");
    switch (altsort)
    {
    case ALTSORT_NAME:
        OutputString(15, x, y, "Name");
        break;
    case ALTSORT_SELECTED:
        OutputString(15, x, y, "Selected");
        break;
    case ALTSORT_DETAIL:
        if (detail_mode == DETAIL_MODE_SQUAD) {
            OutputString(15, x, y, "Squad");
        } else if (detail_mode == DETAIL_MODE_JOB) {
            OutputString(15, x, y, "Job");
        } else {
            OutputString(15, x, y, "Profession");
        }
        break;
    case ALTSORT_STRESS:
        OutputString(15, x, y, "Stress Level");
        break;
    case ALTSORT_ARRIVAL:
        OutputString(15, x, y, "Arrival");
        break;
    default:
        OutputString(15, x, y, "Unknown");
        break;
    }

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
    OutputString(15, x, y, ": Save Prof. ");
}

df::unit *viewscreen_unitlaborsst::getSelectedUnit()
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
        if (input->count(interface_key::UNITVIEW_PRF_PROF))
        {
            if (units[page].size())
            {
                Screen::show(dts::make_unique<viewscreen_unitlaborsst>(units[page], cursor_pos[page]), plugin_self);
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
