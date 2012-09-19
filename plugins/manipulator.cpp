// Dwarf Manipulator - a Therapist-style labor editor

#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <MiscUtils.h>
#include <modules/Screen.h>
#include <modules/Translation.h>
#include <modules/Units.h>
#include <vector>
#include <string>
#include <set>
#include <algorithm>

#include <VTableInterpose.h>
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

using std::set;
using std::vector;
using std::string;

using namespace DFHack;
using namespace df::enums;

using df::global::world;
using df::global::ui;
using df::global::gps;
using df::global::enabler;

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
    {11, 3, profession::NONE, unit_labor::HAUL_ANIMAL, job_skill::NONE, "An"},
    {11, 3, profession::NONE, unit_labor::PUSH_HAUL_VEHICLE, job_skill::NONE, "Ve"},
// Other Jobs
    {12, 4, profession::ARCHITECT, unit_labor::ARCHITECT, job_skill::DESIGNBUILDING, "Ar"},
    {12, 4, profession::ALCHEMIST, unit_labor::ALCHEMIST, job_skill::ALCHEMY, "Al"},
    {12, 4, profession::NONE, unit_labor::CLEAN, job_skill::NONE, "Cl"},
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

    {20, 5, profession::NONE, unit_labor::NONE, job_skill::MILITARY_TACTICS, "MT"},
    {20, 5, profession::NONE, unit_labor::NONE, job_skill::TRACKING, "Tr"},
    {20, 5, profession::NONE, unit_labor::NONE, job_skill::MAGIC_NATURE, "Dr"},
};

struct UnitInfo
{
    df::unit *unit;
    bool allowEdit;
    string name;
    string transname;
    string profession;
    int8_t color;
};

enum altsort_mode {
    ALTSORT_NAME,
    ALTSORT_PROFESSION,
    ALTSORT_HAPPINESS,
    ALTSORT_MAX
};

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

bool sortByHappiness (const UnitInfo *d1, const UnitInfo *d2)
{
    if (descending)
        return (d1->unit->status.happiness > d2->unit->status.happiness);
    else
        return (d1->unit->status.happiness < d2->unit->status.happiness);
}

bool sortBySkill (const UnitInfo *d1, const UnitInfo *d2)
{
    if (sort_skill != job_skill::NONE)
    {
        df::unit_skill *s1 = binsearch_in_vector<df::unit_skill,df::enum_field<df::job_skill,int16_t>>(d1->unit->status.current_soul->skills, &df::unit_skill::id, sort_skill);
        df::unit_skill *s2 = binsearch_in_vector<df::unit_skill,df::enum_field<df::job_skill,int16_t>>(d2->unit->status.current_soul->skills, &df::unit_skill::id, sort_skill);
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
    return sortByName(d1, d2);
}

enum display_columns {
    DISP_COLUMN_HAPPINESS,
    DISP_COLUMN_NAME,
    DISP_COLUMN_PROFESSION,
    DISP_COLUMN_LABORS,
    DISP_COLUMN_MAX,
};

class viewscreen_unitlaborsst : public dfhack_viewscreen {
public:
    void feed(set<df::interface_key> *events);

    void render();
    void resize(int w, int h) { calcSize(); }

    void help() { }

    std::string getFocusString() { return "unitlabors"; }

    viewscreen_unitlaborsst(vector<df::unit*> &src);
    ~viewscreen_unitlaborsst() { };

protected:
    vector<UnitInfo *> units;
    altsort_mode altsort;

    int first_row, sel_row, num_rows;
    int first_column, sel_column;

    int col_widths[DISP_COLUMN_MAX];
    int col_offsets[DISP_COLUMN_MAX];

    void calcSize ();
};

viewscreen_unitlaborsst::viewscreen_unitlaborsst(vector<df::unit*> &src)
{
    for (size_t i = 0; i < src.size(); i++)
    {
        UnitInfo *cur = new UnitInfo;
        df::unit *unit = src[i];
        cur->unit = unit;
        cur->allowEdit = true;

        if (unit->race != ui->race_id)
            cur->allowEdit = false;

        if (unit->civ_id != ui->civ_id)
            cur->allowEdit = false;

        if (unit->flags1.bits.dead)
            cur->allowEdit = false;

        if (!ENUM_ATTR(profession, can_assign_labor, unit->profession))
            cur->allowEdit = false;

        cur->name = Translation::TranslateName(&unit->name, false);
        cur->transname = Translation::TranslateName(&unit->name, true);
        cur->profession = Units::getProfessionName(unit);
        cur->color = Units::getProfessionColor(unit);

        units.push_back(cur);
    }
    std::sort(units.begin(), units.end(), sortByName);

    altsort = ALTSORT_NAME;
    first_row = sel_row = 0;
    first_column = sel_column = 0;
    calcSize();
}

void viewscreen_unitlaborsst::calcSize()
{
    num_rows = gps->dimy - 10;
    if (num_rows > units.size())
        num_rows = units.size();

    int num_columns = gps->dimx - DISP_COLUMN_MAX - 1;
    for (int i = 0; i < DISP_COLUMN_MAX; i++)
        col_widths[i] = 0;
    while (num_columns > 0)
    {
        num_columns--;
        // need at least 4 digits for happiness
        if (col_widths[DISP_COLUMN_HAPPINESS] < 4)
        {
            col_widths[DISP_COLUMN_HAPPINESS]++;
            continue;
        }
        // of remaining, 20% for Name, 20% for Profession, 60% for Labors
        switch (num_columns % 5)
        {
        case 0: case 2: case 4:
            col_widths[DISP_COLUMN_LABORS]++;
            break;
        case 1:
            col_widths[DISP_COLUMN_NAME]++;
            break;
        case 3:
            col_widths[DISP_COLUMN_PROFESSION]++;
            break;
        }
    }

    while (col_widths[DISP_COLUMN_LABORS] > NUM_COLUMNS)
    {
        col_widths[DISP_COLUMN_LABORS]--;
        if (col_widths[DISP_COLUMN_LABORS] & 1)
            col_widths[DISP_COLUMN_NAME]++;
        else
            col_widths[DISP_COLUMN_PROFESSION]++;
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
    if (first_row > units.size() - num_rows)
        first_row = units.size() - num_rows;

    // if it shrinks vertically, scroll downward to keep the cursor visible
    if (first_row < sel_row - num_rows + 1)
        first_row = sel_row - num_rows + 1;

    // if the window grows horizontally, scroll to the left to eliminate blank columns from the right
    if (first_column > NUM_COLUMNS - col_widths[DISP_COLUMN_LABORS])
        first_column = NUM_COLUMNS - col_widths[DISP_COLUMN_LABORS];

    // if it shrinks horizontally, scroll to the right to keep the cursor visible
    if (first_column < sel_column - col_widths[DISP_COLUMN_LABORS] + 1)
        first_column = sel_column - col_widths[DISP_COLUMN_LABORS] + 1;
}

void viewscreen_unitlaborsst::feed(set<df::interface_key> *events)
{
    if (events->count(interface_key::LEAVESCREEN))
    {
        events->clear();
        Screen::dismiss(this);
        return;
    }

    if (!units.size())
        return;

    if (events->count(interface_key::CURSOR_UP) || events->count(interface_key::CURSOR_UPLEFT) || events->count(interface_key::CURSOR_UPRIGHT))
        sel_row--;
    if (events->count(interface_key::CURSOR_UP_FAST) || events->count(interface_key::CURSOR_UPLEFT_FAST) || events->count(interface_key::CURSOR_UPRIGHT_FAST))
        sel_row -= 10;
    if (events->count(interface_key::CURSOR_DOWN) || events->count(interface_key::CURSOR_DOWNLEFT) || events->count(interface_key::CURSOR_DOWNRIGHT))
        sel_row++;
    if (events->count(interface_key::CURSOR_DOWN_FAST) || events->count(interface_key::CURSOR_DOWNLEFT_FAST) || events->count(interface_key::CURSOR_DOWNRIGHT_FAST))
        sel_row += 10;

    if (sel_row < 0)
        sel_row = 0;
    if (sel_row > units.size() - 1)
        sel_row = units.size() - 1;

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
        // go to end of current column group; if already at the end, go to the end of the next one
        sel_column++;
        int cur = columns[sel_column].group;
        while ((sel_column < NUM_COLUMNS - 1) && columns[sel_column + 1].group == cur)
            sel_column++;
    }

    if (sel_column < 0)
        sel_column = 0;
    if (sel_column > NUM_COLUMNS - 1)
        sel_column = NUM_COLUMNS - 1;

    if (sel_column < first_column)
        first_column = sel_column;
    if (first_column < sel_column - col_widths[DISP_COLUMN_LABORS] + 1)
        first_column = sel_column - col_widths[DISP_COLUMN_LABORS] + 1;

    // handle mouse input
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
                if ((gps->mouse_y >= 4) && (gps->mouse_y <= 4 + num_rows))
                    click_body = i;
            }
        }

        if ((gps->mouse_x >= col_offsets[DISP_COLUMN_LABORS]) &&
            (gps->mouse_x < col_offsets[DISP_COLUMN_LABORS] + col_widths[DISP_COLUMN_LABORS]))
            click_labor = gps->mouse_x - col_offsets[DISP_COLUMN_LABORS] + first_column;
        if ((gps->mouse_y >= 4) && (gps->mouse_y <= 4 + num_rows))
            click_unit = gps->mouse_y - 4 + first_row;

        switch (click_header)
        {
        case DISP_COLUMN_HAPPINESS:
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                descending = enabler->mouse_lbut;
                std::sort(units.begin(), units.end(), sortByHappiness);
            }
            break;

        case DISP_COLUMN_NAME:
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                descending = enabler->mouse_rbut;
                std::sort(units.begin(), units.end(), sortByName);
            }
            break;

        case DISP_COLUMN_PROFESSION:
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                descending = enabler->mouse_rbut;
                std::sort(units.begin(), units.end(), sortByProfession);
            }
            break;

        case DISP_COLUMN_LABORS:
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                descending = enabler->mouse_lbut;
                sort_skill = columns[click_labor].skill;
                sort_labor = columns[click_labor].labor;
                std::sort(units.begin(), units.end(), sortBySkill);
            }
            break;
        }

        switch (click_body)
        {
        case DISP_COLUMN_HAPPINESS:
            // do nothing
            break;

        case DISP_COLUMN_NAME:
        case DISP_COLUMN_PROFESSION:
            // left-click to view, right-click to zoom
            if (enabler->mouse_lbut)
            {
                sel_row = click_unit;
                events->insert(interface_key::UNITJOB_VIEW);
            }
            if (enabler->mouse_rbut)
            {
                sel_row = click_unit;
                events->insert(interface_key::UNITJOB_ZOOM_CRE);
            }
            break;

        case DISP_COLUMN_LABORS:
            // left-click to toggle, right-click to just highlight
            if (enabler->mouse_lbut || enabler->mouse_rbut)
            {
                sel_row = click_unit;
                sel_column = click_labor;
                if (enabler->mouse_lbut)
                    events->insert(interface_key::SELECT);
            }
            break;
        }
        enabler->mouse_lbut = enabler->mouse_rbut = 0;
    }

    UnitInfo *cur = units[sel_row];
    if (events->count(interface_key::SELECT) && (cur->allowEdit) && (columns[sel_column].labor != unit_labor::NONE))
    {
        df::unit *unit = cur->unit;
        const SkillColumn &col = columns[sel_column];
        bool newstatus = !unit->status.labors[col.labor];
        if (col.special)
        {
            if (newstatus)
            {
                for (int i = 0; i < NUM_COLUMNS; i++)
                {
                    if ((columns[i].labor != unit_labor::NONE) && columns[i].special)
                        unit->status.labors[columns[i].labor] = false;
                }
            }
            unit->military.pickup_flags.bits.update = true;
        }
        unit->status.labors[col.labor] = newstatus;
    }
    if (events->count(interface_key::SELECT_ALL) && (cur->allowEdit))
    {
        df::unit *unit = cur->unit;
        const SkillColumn &col = columns[sel_column];
        bool newstatus = !unit->status.labors[col.labor];
        for (int i = 0; i < NUM_COLUMNS; i++)
        {
            if (columns[i].group != col.group)
                continue;
            if (columns[i].special)
            {
                if (newstatus)
                {
                    for (int j = 0; j < NUM_COLUMNS; j++)
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
        sort_skill = columns[sel_column].skill;
        sort_labor = columns[sel_column].labor;
        std::sort(units.begin(), units.end(), sortBySkill);
    }

    if (events->count(interface_key::SECONDSCROLL_PAGEUP) || events->count(interface_key::SECONDSCROLL_PAGEDOWN))
    {
        descending = events->count(interface_key::SECONDSCROLL_PAGEUP);
        switch (altsort)
        {
        case ALTSORT_NAME:
            std::sort(units.begin(), units.end(), sortByName);
            break;
        case ALTSORT_PROFESSION:
            std::sort(units.begin(), units.end(), sortByProfession);
            break;
        case ALTSORT_HAPPINESS:
            std::sort(units.begin(), units.end(), sortByHappiness);
            break;
        }
    }
    if (events->count(interface_key::CHANGETAB))
    {
        switch (altsort)
        {
        case ALTSORT_NAME:
            altsort = ALTSORT_PROFESSION;
            break;
        case ALTSORT_PROFESSION:
            altsort = ALTSORT_HAPPINESS;
            break;
        case ALTSORT_HAPPINESS:
            altsort = ALTSORT_NAME;
            break;
        }
    }

    if (VIRTUAL_CAST_VAR(unitlist, df::viewscreen_unitlistst, parent))
    {
        if (events->count(interface_key::UNITJOB_VIEW) || events->count(interface_key::UNITJOB_ZOOM_CRE))
        {
            for (int i = 0; i < unitlist->units[unitlist->page].size(); i++)
            {
                if (unitlist->units[unitlist->page][i] == units[sel_row]->unit)
                {
                    unitlist->cursor_pos[unitlist->page] = i;
                    unitlist->feed(events);
                    if (Screen::isDismissed(unitlist))
                        Screen::dismiss(this);
                    break;
                }
            }
        }
    }
}

void OutputString(int8_t color, int &x, int y, const std::string &text)
{
    Screen::paintString(Screen::Pen(' ', color, 0), x, y, text);
    x += text.length();
}
void viewscreen_unitlaborsst::render()
{
    if (Screen::isDismissed(this))
        return;

    dfhack_viewscreen::render();

    Screen::clear();
    Screen::drawBorder("  Dwarf Manipulator - Manage Labors  ");


    Screen::paintString(Screen::Pen(' ', 7, 0), col_offsets[DISP_COLUMN_HAPPINESS], 2, "Hap.");
    Screen::paintString(Screen::Pen(' ', 7, 0), col_offsets[DISP_COLUMN_NAME], 2, "Name");
    Screen::paintString(Screen::Pen(' ', 7, 0), col_offsets[DISP_COLUMN_PROFESSION], 2, "Profession");

    for (int col = 0; col < col_widths[DISP_COLUMN_LABORS]; col++)
    {
        int col_offset = col + first_column;
        if (col_offset >= NUM_COLUMNS)
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
        if (row_offset >= units.size())
            break;

        UnitInfo *cur = units[row_offset];
        df::unit *unit = cur->unit;
        int8_t fg = 15, bg = 0;

        int happy = cur->unit->status.happiness;
        string happiness = stl_sprintf("%4i", happy);
        if (happy == 0)         // miserable
            fg = 13;    // 5:1
        else if (happy <= 25)   // very unhappy
            fg = 12;    // 4:1
        else if (happy <= 50)   // unhappy
            fg = 4;     // 4:0
        else if (happy < 75)    // fine
            fg = 14;    // 6:1
        else if (happy < 125)   // quite content
            fg = 6;     // 6:0
        else if (happy < 150)   // happy
            fg = 2;     // 2:0
        else                    // ecstatic
            fg = 10;    // 2:1
        Screen::paintString(Screen::Pen(' ', fg, bg), col_offsets[DISP_COLUMN_HAPPINESS], 4 + row, happiness);

        fg = 15;
        if (row_offset == sel_row)
        {
            fg = 0;
            bg = 7;
        }

        string name = cur->name;
        name.resize(col_widths[DISP_COLUMN_NAME]);
        Screen::paintString(Screen::Pen(' ', fg, bg), col_offsets[DISP_COLUMN_NAME], 4 + row, name);

        string profession = cur->profession;
        profession.resize(col_widths[DISP_COLUMN_PROFESSION]);
        fg = cur->color;
        bg = 0;

        Screen::paintString(Screen::Pen(' ', fg, bg), col_offsets[DISP_COLUMN_PROFESSION], 4 + row, profession);

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
                df::unit_skill *skill = binsearch_in_vector<df::unit_skill,df::enum_field<df::job_skill,int16_t>>(unit->status.current_soul->skills, &df::unit_skill::id, columns[col_offset].skill);
                if ((skill != NULL) && (skill->rating || skill->experience))
                {
                    int level = skill->rating;
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
                bg = 4;
            Screen::paintTile(Screen::Pen(c, fg, bg), col_offsets[DISP_COLUMN_LABORS] + col, 4 + row);
        }
    }

    UnitInfo *cur = units[sel_row];
    bool canToggle = false;
    if (cur != NULL)
    {
        df::unit *unit = cur->unit;
        int x = 1;
        Screen::paintString(Screen::Pen(' ', 15, 0), x, 3 + num_rows + 2, cur->transname);
        x += cur->transname.length();

        if (cur->transname.length())
        {
            Screen::paintString(Screen::Pen(' ', 15, 0), x, 3 + num_rows + 2, ", ");
            x += 2;
        }
        Screen::paintString(Screen::Pen(' ', 15, 0), x, 3 + num_rows + 2, cur->profession);
        x += cur->profession.length();
        Screen::paintString(Screen::Pen(' ', 15, 0), x, 3 + num_rows + 2, ": ");
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
            df::unit_skill *skill = binsearch_in_vector<df::unit_skill,df::enum_field<df::job_skill,int16_t>>(unit->status.current_soul->skills, &df::unit_skill::id, columns[sel_column].skill);
            if (skill)
            {
                int level = skill->rating;
                if (level > NUM_SKILL_LEVELS - 1)
                    level = NUM_SKILL_LEVELS - 1;
                str = stl_sprintf("%s %s", skill_levels[level].name, ENUM_ATTR_STR(job_skill, caption_noun, columns[sel_column].skill));
                if (level != NUM_SKILL_LEVELS - 1)
                    str += stl_sprintf(" (%d/%d)", skill->experience, skill_levels[level].points);
            }
            else
                str = stl_sprintf("Not %s (0/500)", ENUM_ATTR_STR(job_skill, caption_noun, columns[sel_column].skill));
        }
        Screen::paintString(Screen::Pen(' ', 9, 0), x, 3 + num_rows + 2, str);

        canToggle = (cur->allowEdit) && (columns[sel_column].labor != unit_labor::NONE);
    }

    int x = 2;
    OutputString(10, x, gps->dimy - 3, "Enter"); // SELECT key
    OutputString(canToggle ? 15 : 8, x, gps->dimy - 3, ": Toggle labor, ");

    OutputString(10, x, gps->dimy - 3, "Shift+Enter"); // SELECT_ALL key
    OutputString(canToggle ? 15 : 8, x, gps->dimy - 3, ": Toggle Group, ");

    OutputString(10, x, gps->dimy - 3, "v"); // UNITJOB_VIEW key
    OutputString(15, x, gps->dimy - 3, ": ViewCre, ");

    OutputString(10, x, gps->dimy - 3, "c"); // UNITJOB_ZOOM_CRE key
    OutputString(15, x, gps->dimy - 3, ": Zoom-Cre");

    x = 2;
    OutputString(10, x, gps->dimy - 2, "Esc"); // LEAVESCREEN key
    OutputString(15, x, gps->dimy - 2, ": Done, ");

    OutputString(10, x, gps->dimy - 2, "+"); // SECONDSCROLL_DOWN key
    OutputString(10, x, gps->dimy - 2, "-"); // SECONDSCROLL_UP key
    OutputString(15, x, gps->dimy - 2, ": Sort by Skill, ");

    OutputString(10, x, gps->dimy - 2, "*"); // SECONDSCROLL_PAGEDOWN key
    OutputString(10, x, gps->dimy - 2, "/"); // SECONDSCROLL_PAGEUP key
    OutputString(15, x, gps->dimy - 2, ": Sort by (");
    OutputString(10, x, gps->dimy - 2, "Tab"); // CHANGETAB key
    OutputString(15, x, gps->dimy - 2, ") ");
    switch (altsort)
    {
    case ALTSORT_NAME:
        OutputString(15, x, gps->dimy - 2, "Name");
        break;
    case ALTSORT_PROFESSION:
        OutputString(15, x, gps->dimy - 2, "Profession");
        break;
    case ALTSORT_HAPPINESS:
        OutputString(15, x, gps->dimy - 2, "Happiness");
        break;
    default:
        OutputString(15, x, gps->dimy - 2, "Unknown");
        break;
    }
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
                Screen::show(new viewscreen_unitlaborsst(units[page]));
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
            int x = 2;
            OutputString(12, x, gps->dimy - 2, "l"); // UNITVIEW_PRF_PROF key
            OutputString(15, x, gps->dimy - 2, ": Manage labors (DFHack)");
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(unitlist_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(unitlist_hook, render);

DFHACK_PLUGIN("manipulator");

DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands)
{
    if (!gps || !INTERPOSE_HOOK(unitlist_hook, feed).apply() || !INTERPOSE_HOOK(unitlist_hook, render).apply())
        out.printerr("Could not insert Dwarf Manipulator hooks!\n");
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    INTERPOSE_HOOK(unitlist_hook, feed).remove();
    INTERPOSE_HOOK(unitlist_hook, render).remove();
    return CR_OK;
}
