// Intention: help with activity zone management (auto-pasture animals, auto-pit goblins, ...)
// 
// the following things would be nice:
// - dump info about pastures, pastured animals, count non-pastured tame animals, print gender info
// - help finding caged dwarves? (maybe even allow to build their cages for fast release)
// - dump info about caged goblins, animals, ...
// - count grass tiles on pastures, move grazers to new pasture if old pasture is empty
//   move hungry unpastured grazers to pasture with grass
// 
// What is working so far:
// - print detailed info about activity zone and units under cursor (mostly for checking refs and stuff)
// - mark a zone which is used for future assignment commands
// - assign single selected creature to a zone
// - unassign single creature under cursor from current zone
// - pitting own dwarves :)
// - full automation of handling mini-pastures over nestboxes:
//   go through all pens, check if they are empty and placed over a nestbox
//   find female tame egg-layer who is not assigned to another pen and assign it to nestbox pasture
//   maybe check for minimum age? it's not that useful to fill nestboxes with freshly hatched birds
// - full automation of marking live-stock for slaughtering
//   races can be added to a watchlist and it can be set how many male/female kids/adults are left alive
//   TODO: - parse more than one race in one command
//         - support keywords 'all' and 'new'
//         - autowatch
//         - save config

#include <iostream>
#include <iomanip>
#include <climits>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <ctime>
#include <cstdio>
using namespace std;

#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Units.h"
#include "modules/Maps.h"
#include "modules/Gui.h"
#include "modules/Materials.h"
#include "modules/MapCache.h"
#include "modules/Buildings.h"
#include "MiscUtils.h"

#include <df/ui.h>
#include "df/world.h"
#include "df/world_raws.h"
#include "df/building_def.h"
#include "df/building_civzonest.h"
#include "df/building_cagest.h"
#include "df/building_chainst.h"
#include "df/general_ref_building_civzone_assignedst.h"
#include <df/creature_raw.h>
#include <df/caste_raw.h>

using std::vector;
using std::string;
using namespace DFHack;
using namespace df::enums;
using df::global::world;
using df::global::cursor;
using df::global::ui;

using namespace DFHack::Gui;

command_result df_zone (color_ostream &out, vector <string> & parameters);
command_result df_autonestbox (color_ostream &out, vector <string> & parameters);
command_result df_autobutcher(color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("zone");

const string zone_help =
    "Allows easier management of pens/pastures and pits.\n"
    "Options:\n"
    "  set          - set zone under cursor as default for future assigns\n"
    "  assign       - assign creature(s) to a pen or pit\n"
    "                 if no filters are used, a single unit must be selected.\n"
    "                 can be followed by valid zone id which will then be set.\n"
    "  slaughter    - mark creature(s) for slaughter\n"
    "                 if no filters are used, a single unit must be selected.\n"
    "                 with filters named units are ignored unless specified.\n"
    "  unassign     - unassign selected creature from it's zone\n"
    "  uinfo        - print info about selected unit\n"
    "  zinfo        - print info about zone(s) under cursor\n"
    "  verbose      - print some more info, mostly useless debug stuff\n"
    "  filters      - print list of supported filters\n"
    "  examples     - print some usage examples\n";

const string zone_help_filters =
    "Filters (to be used in combination with 'all' or 'count'):\n"
    "  all          - in combinations with zinfo/uinfo: print all zones/units\n"
    "                 in combination with assign: process all units\n"
    "                 should be used in combination with further filters\n"
    "  count        - must be followed by number. process X units\n"
    "                 should be used in combination with further filters\n"
    "  race         - must be followed by a race raw id (e.g. BIRD_TURKEY)\n"
    "  unassigned   - not assigned to zone, chain or built cage\n"
    "  caged        - in a built cage\n"
    "  uncaged      - not in cage\n"
    "  foreign      - not of own civilization (i.e. own fortress)\n"
    "  own          - from own civilization (fortress)\n"
    "  war          - trained war creature\n"
    "  tamed        - tamed\n"
    "  named        - has name or nickname\n"
    "                 can be used to mark named units for slaughter\n"
    "  trained      - obvious\n"
    "  untrained    - obvious\n"
    "  male         - obvious\n"
    "  female       - obvious\n"
    "  egglayer     - race lays eggs (use together with 'female')\n"
    "  grazer       - obvious\n"
    "  milkable     - race is milkable (use together with 'female')\n"
    "  minage       - minimum age. must be followed by number\n"
    "  maxage       - maximum age. must be followed by number\n";

const string zone_help_examples =
    "Example for assigning single units:\n"
    "  (ingame) move cursor to a pen/pasture or pit zone\n"
    "  (dfhack) 'zone set' to use this zone for future assignments\n"
    "  (dfhack) map 'zone assign' to a hotkey of your choice\n"
    "  (ingame) select unit with 'v', 'k' or from unit list or inside a cage\n"
    "  (ingame) press hotkey to assign unit to it's new home (or pit)\n"
    "Examples for assigning with filters:\n"
    "  (this assumes you have already set up a target zone)\n"
    "  zone assign all own grazer maxage 10\n"
    "  zone assign count 5 own female milkable\n"
    "  zone assign all own race DWARF maxage 2\n"
    "    throw all useless kids into a pit :)\n"
    "Notes:\n"
    "  Assigning per filters ignores built cages and chains currently.\n"
    "  Usually you should always use the filter 'own' (which implies tame)\n"
    "  unless you want to use the zone tool for pitting hostiles.\n"
    "  'own' ignores own dwarves unless you specify 'race DWARF'\n"
    "  (so it's safe to use 'assign all own' to one big pasture\n"
    "  if you want to have all your animals at the same place).\n"
    "  'egglayer' and 'milkable' should be used together with 'female'\n"
    "  unless you have a mod with egg-laying male elves who give milk.\n";


const string autonestbox_help =
    "Assigns unpastured female egg-layers to nestbox zones.\n"
    "Requires that you create pen/pasture zones above nestboxes.\n"
    "If the pen is bigger than 1x1 the nestbox must be in the top left corner.\n"
    "Only 1 unit will be assigned per pen, regardless of the size.\n"
    "The age of the units is currently not checked, most birds grow up quite fast.\n"
    "When called without options autonestbox will instantly run once.\n"
    "Options:\n"
    "  start        - run every X frames (df simulation ticks)\n"
    "                 default: X=6000  (~60 seconds at 100fps)\n"
    "  stop         - stop running automatically\n"
    "  sleep X      - change timer to sleep X frames between runs.\n";

const string autobutcher_help =
    "Assigns your lifestock for slaughter once it reaches a specific count. Requires\n"
    "that you add the target race(s) to a watch list. Only tame units of your own\n"
    "civilization will be processed. Named units will be completely ignored (you can\n"
    "give animals nicknames with the tool 'rename unit' to protect them from\n"
    "getting slaughtered automatically.\n"
    "Once you have too much adults, the oldest will be butchered first.\n"
    "Once you have too much kids, the youngest will be butchered first.\n"
    "If you don't set a target count the following default will be used:\n"
    "1 male kid, 5 female kids, 1 male adult, 5 female adults.\n"
    "Options:\n"
    "  start        - run every X frames (df simulation ticks)\n"
    "                 default: X=6000  (~60 seconds at 100fps)\n"
    "  stop         - stop running automatically\n"
    "  sleep X      - change timer to sleep X frames between runs.\n"
    "  watch R      - start watching race(s)\n"
    "                 R = valid race RAW id (ALPACA, BIRD_TURKEY, etc)\n"
    //"                 or a list of RAW ids seperated by spaces\n"
    //"                 or the keyword 'all' which adds all races with\n"
    //"                 at least one owned tame unit in your fortress\n"
    "  unwatch R    - stop watching race\n"
    "                 the current target settings will be remembered\n"
    "  forget R     - unwatch race and forget target settings for it\n"
    //"  autowatch    - automatically adds all new races (animals you buy\n"
    //"                 from merchants, tame yourself or get from migrants)\n"
    //"                 to the watch list using default target count\n"
    "  list         - print a list of watched races\n"
    "  target fk mk fa ma R\n"
    "               - set target count for specified race:\n"
    "                 fk = number of female kids\n"
    "                 mk = number of male kids\n"
    "                 fa = number of female adults\n"
    "                 ma = number of female adults\n"
    //"                 R = 'all' sets count for all races on the current watchlist\n"
    //"                 including the races which are currenly set to 'unwatched'\n"
    //"                 and sets the new default for future watch commands\n"
    //"                 R = 'new' sets the new default for future watch commands\n"
    //"                 without changing your current watchlist\n"
    "  example      - print some usage examples\n";

const string autobutcher_help_example =
    "Examples:\n"
    "  autobutcher target 4 3 2 1 ALPACA\n"
    "  autobutcher watch ALPACA\n"
    "  autobutcher start\n"
    "    This means you want to have max 7 kids (4 female, 3 male) and max 3 adults\n"
    "    (2 female, 1 male) of the race alpaca. Once the kids grow up the oldest\n"
    "    adults will get slaughtered. Excess kids will get slaughtered starting with\n"
    "    the youngest to allow that the older ones grow into adults.\n"
    //"  autobutcher target 0 0 0 0 all\n"
    //"  autobutcher autowatch\n"
    //"  autobutcher start\n"
    //"    This tells autobutcher to automatically put all new races onto the watchlist\n"
    //"    and mark unnamed tame units for slaughter as soon as they arrive in your\n"
    //"    fortress. Settings already made for some races will be left untouched.\n"
    ;


DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "zone", "manage activity zones.",
        df_zone, false,
        zone_help.c_str()
        ));
    commands.push_back(PluginCommand(
        "autonestbox", "auto-assign nestbox zones.",
        df_autonestbox, false,
        autonestbox_help.c_str()
        ));
    commands.push_back(PluginCommand(
        "autobutcher", "auto-assign lifestock for butchering.",
        df_autobutcher, false,
        autobutcher_help.c_str()
        ));
    return CR_OK;
}

command_result autobutcher_cleanup(color_ostream &out);
DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    autobutcher_cleanup(out);
    return CR_OK;
}

///////////////
// stuff for autonestbox and autobutcher
// should be moved to own plugin once the tool methods it shares with the zone plugin are moved to Unit.h / Building.h

command_result autoNestbox( color_ostream &out, bool verbose );
command_result autoButcher( color_ostream &out, bool verbose );

static bool enable_autonestbox = false;
static bool enable_autobutcher = false;
static size_t sleep_autonestbox = 6000;
static size_t sleep_autobutcher = 6000;
static bool autonestbox_did_complain = false; // avoids message spam

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) 
    {
    case DFHack::SC_MAP_LOADED:
        // initialize from the world just loaded
        break;
    case DFHack::SC_MAP_UNLOADED:
        enable_autonestbox = false;
        enable_autobutcher = false;
        // cleanup
        autobutcher_cleanup(out);
        break;
    default:
        break;
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    static size_t ticks_autonestbox = 0;
    static size_t ticks_autobutcher = 0;

    if(enable_autonestbox)
    {
        if(++ticks_autonestbox >= sleep_autonestbox)
        {
            ticks_autonestbox = 0;
            autoNestbox(out, false);
        }
    }
    
    if(enable_autobutcher)
    {
        if(++ticks_autobutcher >= sleep_autobutcher)
        {
            ticks_autobutcher= 0;
            autoButcher(out, false);
        }
    }

    return CR_OK;
}


///////////////
// Various small tool functions
// probably many of these should be moved to Unit.h and Building.h sometime later...

int32_t getUnitAge(df::unit* unit);
bool isTame(df::unit* unit);
bool isTrained(df::unit* unit);
bool isTrained(df::unit* creature);
bool isWar(df::unit* creature);
bool isHunter(df::unit* creature);
bool isOwnCiv(df::unit* creature);

bool isActivityZone(df::building * building);
bool isPenPasture(df::building * building);
bool isPit(df::building * building);
bool isActive(df::building * building);

int32_t findBuildingIndexById(int32_t id);
int32_t findPenPitAtCursor();
int32_t findCageAtCursor();
int32_t findChainAtCursor();

df::general_ref_building_civzone_assignedst * createCivzoneRef();
bool unassignUnitFromZone(df::unit* unit);
command_result assignUnitToZone(color_ostream& out, df::unit* unit, df::building* building, bool verbose);
void unitInfo(color_ostream & out, df::unit* creature, bool verbose);
void zoneInfo(color_ostream & out, df::building* building, bool verbose);
void cageInfo(color_ostream & out, df::building* building, bool verbose);
void chainInfo(color_ostream & out, df::building* building, bool verbose);
bool isBuiltCageAtPos(df::coord pos);

int32_t getUnitAge(df::unit* unit)
{
    // If the birthday this year has not yet passed, subtract one year.
    // ASSUMPTION: birth_time is on the same scale as cur_year_tick
    int32_t yearDifference = *df::global::cur_year - unit->relations.birth_year;
    if (unit->relations.birth_time >= *df::global::cur_year_tick)
        yearDifference--;
    return yearDifference;
}

bool isDead(df::unit* unit)
{
    return unit->flags1.bits.dead;
}

bool isMarkedForSlaughter(df::unit* unit)
{
    return unit->flags2.bits.slaughter;
}

void doMarkForSlaughter(df::unit* unit)
{
    unit->flags2.bits.slaughter = 1;
}

bool isTame(df::unit* creature)
{
    bool tame = false;
    if(creature->flags1.bits.tame)
    {
        switch (creature->training_level)
        {
        case df::animal_training_level::Trained:
        case df::animal_training_level::WellTrained:
        case df::animal_training_level::SkilfullyTrained:
        case df::animal_training_level::ExpertlyTrained:
        case df::animal_training_level::ExceptionallyTrained:
        case df::animal_training_level::MasterfullyTrained:
        case df::animal_training_level::Domesticated:
            tame=true;
            break;
        case df::animal_training_level::SemiWild: //??
        case df::animal_training_level::Unk8:     //??
        case df::animal_training_level::WildUntamed:
        default:
            tame=false;
            break;
        }
    }
    return tame;
}

// check if trained (might be useful if pasturing war dogs etc)
bool isTrained(df::unit* creature)
{
    bool trained = false;
    if(creature->flags1.bits.tame)
    {
        switch (creature->training_level)
        {
        case df::animal_training_level::Trained:
        case df::animal_training_level::WellTrained:
        case df::animal_training_level::SkilfullyTrained:
        case df::animal_training_level::ExpertlyTrained:
        case df::animal_training_level::ExceptionallyTrained:
        case df::animal_training_level::MasterfullyTrained:
        //case df::animal_training_level::Domesticated:
            trained = true;
            break;
        default:
            break;
        }
    }
    return trained;
}

// check for profession "war creature"
bool isWar(df::unit* creature)
{
    if(   creature->profession  == df::profession::TRAINED_WAR
       || creature->profession2 == df::profession::TRAINED_WAR)
        return true;
    else
        return false;
}

// check for profession "hunting creature"
bool isHunter(df::unit* creature)
{
    if(   creature->profession  == df::profession::TRAINED_HUNTER
       || creature->profession2 == df::profession::TRAINED_HUNTER)
        return true;
    else
        return false;
}


// check if creature belongs to the player's civilization
// (don't try to pasture/slaughter random untame animals)
bool isOwnCiv(df::unit* creature)
{
    return creature->civ_id == ui->civ_id;
}

// check if creature belongs to the player's race
// (in combination with check for civ helps to filter out own dwarves)
bool isOwnRace(df::unit* creature)
{
    return creature->race == ui->race_id;
}

string getRaceName(df::unit* unit)
{
    df::creature_raw *raw = df::global::world->raws.creatures.all[unit->race];
    return raw->creature_id;
}
string getRaceBabyName(df::unit* unit)
{
    df::creature_raw *raw = df::global::world->raws.creatures.all[unit->race];
    return raw->general_baby_name[0];
}
string getRaceChildName(df::unit* unit)
{
    df::creature_raw *raw = df::global::world->raws.creatures.all[unit->race];
    return raw->general_child_name[0];
}

bool isBaby(df::unit* unit)
{
    return unit->profession == df::profession::BABY;
}

bool isChild(df::unit* unit)
{
    return unit->profession == df::profession::CHILD;
}

bool isAdult(df::unit* unit)
{
    return !isBaby(unit) && !isChild(unit);
}

bool isEggLayer(df::unit* unit)
{
    df::creature_raw *raw = df::global::world->raws.creatures.all[unit->race];
    size_t sizecas = raw->caste.size();
    for (size_t j = 0; j < sizecas;j++)
    {
        df::caste_raw *caste = raw->caste[j];
        if(   caste->flags.is_set(caste_raw_flags::LAYS_EGGS)
            || caste->flags.is_set(caste_raw_flags::LAYS_UNUSUAL_EGGS))
            return true;
    }
    return false;
}

bool isGrazer(df::unit* unit)
{
    df::creature_raw *raw = df::global::world->raws.creatures.all[unit->race];
    size_t sizecas = raw->caste.size();
    for (size_t j = 0; j < sizecas;j++)
    {
        df::caste_raw *caste = raw->caste[j];
        if(caste->flags.is_set(caste_raw_flags::GRAZER))
            return true;
    }
    return false;
}

bool isMilkable(df::unit* unit)
{
    df::creature_raw *raw = df::global::world->raws.creatures.all[unit->race];
    size_t sizecas = raw->caste.size();
    for (size_t j = 0; j < sizecas;j++)
    {
        df::caste_raw *caste = raw->caste[j];
        if(caste->flags.is_set(caste_raw_flags::MILKABLE))
            return true;
    }
    return false;
}

bool isMale(df::unit* unit)
{
    return unit->sex == 1;
}

bool isFemale(df::unit* unit)
{
    return unit->sex == 0;
}

// found a unit with weird position values on one of my maps (negative and in the thousands)
// it didn't appear in the animal stocks screen, but looked completely fine otherwise (alive, tame, own, etc)
// maybe a rare but, but better avoid assigning such units to zones or slaughter etc.
bool hasValidMapPos(df::unit* unit)
{
    if(    unit->pos.x >=0 && unit->pos.y >= 0 && unit->pos.z >= 0
        && unit->pos.x < world->map.x_count 
        && unit->pos.y < world->map.y_count 
        && unit->pos.z < world->map.z_count)
        return true;
    else
        return false;
}

// dump some unit info
void unitInfo(color_ostream & out, df::unit* unit, bool verbose = false)
{
    if(isDead(unit))
        return;

    out.print("Unit %d ", unit->id); //race %d, civ %d,", creature->race, creature->civ_id
    if(unit->name.has_name)
    {
        // units given a nick with the rename tool might not have a first name (animals etc)
        string firstname = unit->name.first_name;
        if(firstname.size() > 0)
        {
            firstname[0] = toupper(firstname[0]);
            out << "Name: " << firstname;
        }
        if(unit->name.nickname.size() > 0)
            out << " '" << unit->name.nickname << "'";
        out << ", ";
    }
    
    if(isAdult(unit))
        out << "adult";
    else if(isBaby(unit))
        out << "baby";
    else if(isChild(unit))
        out << "child";
    out << " ";
    // sometimes empty even in vanilla RAWS, sometimes contains full race name (e.g. baby alpaca) 
    // all animals I looked at don't have babies anyways, their offspring starts as CHILD
    //out << getRaceBabyName(unit);
    //out << getRaceChildName(unit);

    out << getRaceName(unit) << " (";
    switch(unit->sex)
    {
    case 0:
        out << "female";
        break;
    case 1:
        out << "male";
        break;
    case -1:
    default:
        out << "n/a";
        break;
    }
    out << ")";
    out << ", age: " << getUnitAge(unit);
    
    if(isTame(unit))
        out << ", tame";    
    if(isOwnCiv(unit))
        out << ", owned";
    if(isWar(unit))
        out << ", war";
    if(isHunter(unit))
        out << ", hunter";
    
    if(verbose)
    {
        //out << ". Pos: ("<<unit->pos.x << "/"<< unit->pos.y << "/" << unit->pos.z << ")" << endl;
        if(isEggLayer(unit))
            out << ", egglayer";
        if(isGrazer(unit))
            out << ", grazer";
        if(isMilkable(unit))
            out << ", milkable";
    }
    out << endl;

    if(!verbose)
        return;

    //out << "number of refs: " << creature->refs.size() << endl;
    for(size_t r = 0; r<unit->refs.size(); r++)
    {
        df::general_ref* ref = unit->refs.at(r);
        df::general_ref_type refType = ref->getType();
        out << "  ref#" << r <<" refType#" << refType << " "; //endl;
        switch(refType)
        {
        case df::general_ref_type::BUILDING_CIVZONE_ASSIGNED:
            {
            out << "assigned to zone";
            df::building_civzonest * civAss = (df::building_civzonest *) ref->getBuilding();
            out << " #" << civAss->id;
            }
            break;
        case df::general_ref_type::CONTAINED_IN_ITEM:
            out << "contained in item";
            break;
        case df::general_ref_type::BUILDING_CAGED:
            out << "caged";
            break;
        case df::general_ref_type::BUILDING_CHAIN:
            out << "chained";
            break;
        default:
            //out << "unhandled reftype";
            break;
        }
        out << endl;
    }
}

bool isActivityZone(df::building * building)
{
    if(    building->getType() == building_type::Civzone
        && building->getSubtype() == civzone_type::ActivityZone)
        return true;
    else
        return false;
}

bool isPenPasture(df::building * building)
{
    if(!isActivityZone(building))
        return false;

    df::building_civzonest * civ = (df::building_civzonest *) building;

    if(civ->zone_flags.bits.pen_pasture)
        return true;
    else
        return false;
}

bool isPit(df::building * building)
{
    if(!isActivityZone(building))
        return false;

    df::building_civzonest * civ = (df::building_civzonest *) building;

    if(civ->zone_flags.bits.pit_pond && civ->pit_flags==0)
        return true;
    else 
        return false;
}

bool isCage(df::building * building)
{
    return building->getType() == building_type::Cage;
}

bool isChain(df::building * building)
{
    return building->getType() == building_type::Chain;
}

bool isActive(df::building * building)
{
    if(!isActivityZone(building))
        return false;

    df::building_civzonest * civ = (df::building_civzonest *) building;
    if(civ->zone_flags.bits.active)
        return true;
    else
        return false;
}

int32_t findBuildingIndexById(int32_t id)
{
    for (size_t b = 0; b < world->buildings.all.size(); b++)
    {
        if(world->buildings.all.at(b)->id == id)
            return b;
    }
    return -1;
}

// returns id of pen/pit at cursor position (-1 if nothing found)
int32_t findPenPitAtCursor()
{
    int32_t foundID = -1;
    
    if(cursor->x == -30000)
        return -1;

    for (size_t b = 0; b < world->buildings.all.size(); b++)
    {
        df::building* building = world->buildings.all[b];

        // find zone under cursor
        if (!(building->x1 <= cursor->x && cursor->x <= building->x2 &&
            building->y1 <= cursor->y && cursor->y <= building->y2 &&
            building->z == cursor->z))
            continue;

        if(isPenPasture(building) || isPit(building))
        {
            foundID = building->id;
            break;
        }
    }
    return foundID;
}

// returns id of cage at cursor position (-1 if nothing found)
int32_t findCageAtCursor()
{
    int32_t foundID = -1;
    
    if(cursor->x == -30000)
        return -1;

    for (size_t b = 0; b < world->buildings.all.size(); b++)
    {
        df::building* building = world->buildings.all[b];

        if (!(building->x1 <= cursor->x && cursor->x <= building->x2 &&
            building->y1 <= cursor->y && cursor->y <= building->y2 &&
            building->z == cursor->z))
            continue;

        if(isCage(building))
        {
            foundID = building->id;
            break;
        }
    }
    return foundID;
}

int32_t findChainAtCursor()
{
    int32_t foundID = -1;
    
    if(cursor->x == -30000)
        return -1;

    for (size_t b = 0; b < world->buildings.all.size(); b++)
    {
        df::building* building = world->buildings.all[b];

        // find zone under cursor
        if (!(building->x1 <= cursor->x && cursor->x <= building->x2 &&
            building->y1 <= cursor->y && cursor->y <= building->y2 &&
            building->z == cursor->z))
            continue;

        if(isChain(building))
        {
            foundID = building->id;
            break;
        }
    }
    return foundID;
}

df::general_ref_building_civzone_assignedst * createCivzoneRef()
{
    static bool vt_initialized = false;
    df::general_ref_building_civzone_assignedst* newref = NULL;

    // after having run successfully for the first time it's safe to simply create the object
    if(vt_initialized)
    {
        newref = (df::general_ref_building_civzone_assignedst*)
            df::general_ref_building_civzone_assignedst::_identity.instantiate();
        return newref;
    }

    // being called for the first time, need to initialize the vtable
    for(size_t i = 0; i < world->units.all.size(); i++)
    {
        df::unit * creature = world->units.all[i];                    
        for(size_t r = 0; r<creature->refs.size(); r++)
        {
            df::general_ref* ref;
            ref = creature->refs.at(r);
            if(ref->getType() == df::general_ref_type::BUILDING_CIVZONE_ASSIGNED)
            {
                if (strict_virtual_cast<df::general_ref_building_civzone_assignedst>(ref))
                {
                    // !! calling new() doesn't work, need _identity.instantiate() instead !!
                    newref = (df::general_ref_building_civzone_assignedst*) 
                        df::general_ref_building_civzone_assignedst::_identity.instantiate();
                    vt_initialized = true;
                    break;
                }
            }
        }
        if(vt_initialized)
            break;
    }
    return newref;
}

// check if assigned to pen, pit, (built) cage or chain
// note: BUILDING_CAGED is not set for animals (maybe it's used for dwarves who get caged as sentence)
// animals in cages (no matter if built or on stockpile) get the ref CONTAINED_IN_ITEM instead
// removing them from cages on stockpiles is no problem even without clearing the ref
// and usually it will be desired behavior to do so.
bool isAssigned(df::unit* unit)
{
    bool assigned = false;
    for (size_t r=0; r < unit->refs.size(); r++)
    {
        df::general_ref * ref = unit->refs[r];
        auto rtype = ref->getType();
        if(    rtype == df::general_ref_type::BUILDING_CIVZONE_ASSIGNED
            || rtype == df::general_ref_type::BUILDING_CAGED
            || rtype == df::general_ref_type::BUILDING_CHAIN
            || (rtype == df::general_ref_type::CONTAINED_IN_ITEM && isBuiltCageAtPos(unit->pos))
            )
        {
            assigned = true;
            break;
        }
    }
    return assigned;
}

// check if assigned to a chain or built cage
// (need to check if the ref needs to be removed, until then touching them is forbidden)
bool isChained(df::unit* unit)
{
    bool contained = false;
    for (size_t r=0; r < unit->refs.size(); r++)
    {
        df::general_ref * ref = unit->refs[r];
        auto rtype = ref->getType();
        if(rtype == df::general_ref_type::BUILDING_CHAIN)
        {
            contained = true;
            break;
        }
    }
    return contained;
}

// check if contained in item (e.g. animals in cages)
bool isContainedInItem(df::unit* unit)
{
    bool contained = false;
    for (size_t r=0; r < unit->refs.size(); r++)
    {
        df::general_ref * ref = unit->refs[r];
        auto rtype = ref->getType();
        if(rtype == df::general_ref_type::CONTAINED_IN_ITEM)
        {
            contained = true;
            break;
        }
    }
    return contained;
}

// check a map position for a built cage
// animals in cages are CONTAINED_IN_ITEM, no matter if they are on a stockpile or inside a built cage
// if they are on animal stockpiles they should count as unassigned to allow pasturing them
// if they are inside built cages they should be ignored in case the cage is a zoo or linked to a lever or whatever 
bool isBuiltCageAtPos(df::coord pos)
{
    bool cage = false;
    for (size_t b=0; b < world->buildings.all.size(); b++)
    {
        df::building* building = world->buildings.all[b];
        if( building->getType() == building_type::Cage
            && building->x1 == pos.x
            && building->y1 == pos.y
            && building->z  == pos.z )
        {
            cage = true;
            break;
        }
    }
    return cage;
}

bool isNestboxAtPos(int32_t x, int32_t y, int32_t z)
{
    bool found = false;
    for (size_t b=0; b < world->buildings.all.size(); b++)
    {
        df::building* building = world->buildings.all[b];
        if( building->getType() == building_type::NestBox
            && building->x1 == x
            && building->y1 == y
            && building->z  == z )
        {
            found = true;
            break;
        }
    }
    return found;
}

bool isEmptyPasture(df::building* building)
{
    if(!isPenPasture(building))
        return false;
    df::building_civzonest * civ = (df::building_civzonest *) building;
    if(civ->assigned_creature.size() == 0)
        return true;
    else
        return false;
}

df::building* findFreeNestboxZone()
{
    df::building * free_building = NULL;
    bool cage = false;
    for (size_t b=0; b < world->buildings.all.size(); b++)
    {
        df::building* building = world->buildings.all[b];
        if( isEmptyPasture(building) &&
            isActive(building) &&
            isNestboxAtPos(building->x1, building->y1, building->z))
        {
            free_building = building;
            break;
        }
    }
    return free_building;
}

bool isFreeEgglayer(df::unit * unit)
{
    if( !isDead(unit)
        && isFemale(unit)
        && isTame(unit)
        && isOwnCiv(unit)
        && isEggLayer(unit)
        && !isAssigned(unit)
        )
        return true;
    else
        return false;
}

df::unit * findFreeEgglayer()
{
    df::unit* free_unit = NULL;
    for (size_t i=0; i < world->units.all.size(); i++)
    {
        df::unit* unit = world->units.all[i];
        if(isFreeEgglayer(unit))
        {
            free_unit = unit;
            break;
        }
    }
    return free_unit;
}

size_t countFreeEgglayers()
{
    size_t count = 0;
    for (size_t i=0; i < world->units.all.size(); i++)
    {
        df::unit* unit = world->units.all[i];
        if(isFreeEgglayer(unit))
            count ++;
    }
    return count;
}

// check if unit is already assigned to a zone, remove that ref from unit and old zone
// returns false if no pasture information was found
// helps as workaround for http://www.bay12games.com/dwarves/mantisbt/view.php?id=4475 by the way
// (pastured animals assigned to chains will get hauled back and forth because the pasture ref is not deleted)
bool unassignUnitFromZone(df::unit* unit)
{
    bool success = false;
    for (std::size_t idx = 0; idx < unit->refs.size(); idx++)
    {
        df::general_ref * oldref = unit->refs[idx];
        if(oldref->getType() == df::general_ref_type::BUILDING_CIVZONE_ASSIGNED)
        {
            unit->refs.erase(unit->refs.begin() + idx);
            df::building_civzonest * oldciv = (df::building_civzonest *) oldref->getBuilding();
            for(size_t oc=0; oc<oldciv->assigned_creature.size(); oc++)
            {
                if(oldciv->assigned_creature[oc] == unit->id)
                {
                    oldciv->assigned_creature.erase(oldciv->assigned_creature.begin() + oc);
                    break;
                }
            }
            delete oldref;
            success = true;
            break;
        }
    }
    return success;
}

// assign to pen or pit
command_result assignUnitToZone(color_ostream& out, df::unit* unit, df::building* building, bool verbose = false)
{
    //if(!isOwnCiv(unit) || !isTame(unit))
    //{
    //    out << "Creature must be tame and your own." << endl;
    //    return CR_WRONG_USAGE;
    //}

    // building must be a pen/pasture or pit
    //df::building * building = world->buildings.all.at(index);
    if(!isPenPasture(building) && !isPit(building))
    {
        out << "Invalid building type. This is not a pen/pasture or pit." << endl;
        //target_zone = -1;
        return CR_WRONG_USAGE;
    }

    // try to get a fresh civzone ref
    df::general_ref_building_civzone_assignedst * ref = createCivzoneRef();
    if(!ref)
    {
        out << "Could not find a clonable activity zone reference" << endl
            << "You need to pen/pasture/pit at least one creature" << endl
            << "before using 'assign' for the first time." << endl;
        return CR_WRONG_USAGE;
    }
        
    // check if unit is already pastured, remove that ref from unit and old pasture
    //    testing showed that this only seems to be necessary for pastured creatures
    //    if they are in cages on stockpiles the game unassigns them automatically
    //    (need to check if that is also true for chains and built cages)
    bool cleared_old = unassignUnitFromZone(unit);

    if(verbose)
    {
        if(cleared_old)
            out << "old zone info cleared.";
        else
            out << "no old zone info found.";
    }

    ref->building_id = building->id;
    unit->refs.push_back(ref);

    df::building_civzonest * civz = (df::building_civzonest *) building;
    civz->assigned_creature.push_back(unit->id);

    df::creature_raw *raw = df::global::world->raws.creatures.all[unit->race];
    out << "Unit " << unit->id 
        << "(" << raw->creature_id << ")" 
        << " assigned to zone " << building->id;
    if(isPit(building))
        out << " (pit).";
    if(isPenPasture(building))
        out << " (pen/pasture).";
    out << endl;

    return CR_OK;
}

command_result assignUnitToCage(color_ostream& out, df::unit* unit, df::building* building, bool verbose = false)
{
    out << "sorry. assigning to cages is not possible yet." << endl;
    return CR_WRONG_USAGE;
}

command_result assignUnitToChain(color_ostream& out, df::unit* unit, df::building* building, bool verbose = false)
{
    out << "sorry. assigning to chains is not possible yet." << endl;
    return CR_WRONG_USAGE;
}

command_result assignUnitToBuilding(color_ostream& out, df::unit* unit, df::building* building, bool verbose = false)
{
    command_result result = CR_WRONG_USAGE;

    if(isActivityZone(building))
        result = assignUnitToZone(out, unit, building, verbose);
    else if(isCage(building))
        result = assignUnitToCage(out, unit, building, verbose);
    else if(isChain(building))
        result = assignUnitToChain(out, unit, building, verbose);
    else
        out << "Cannot assign units to this type of building!" << endl;

    return result;
}

// dump some zone info
void zoneInfo(color_ostream & out, df::building* building, bool verbose = false)
{
    if(building->getType()!= building_type::Civzone)
        return;

    if(building->getSubtype() != civzone_type::ActivityZone)
        return;

    string name;
    building->getName(&name);
    out.print("Building %i - \"%s\" - type %s (%i)",
                building->id,
                name.c_str(),
                ENUM_KEY_STR(building_type, building->getType()).c_str(),
                building->getType());
    out.print(", subtype %s (%i)",
                ENUM_KEY_STR(civzone_type, (df::civzone_type)building->getSubtype()).c_str(),
                building->getSubtype());
    out.print("\n");

    df::building_civzonest * civ = (df::building_civzonest *) building;
    if(civ->zone_flags.bits.active)
        out << "active";
    else
        out << "not active";

    if(civ->zone_flags.bits.pen_pasture)
        out << ", pen/pasture";
    else if (civ->zone_flags.bits.pit_pond && civ->pit_flags==0)
        out << ", pit";
    else
        return;
    out << endl;
    out << "x1:" <<building->x1
        << " x2:" <<building->x2
        << " y1:" <<building->y1
        << " y2:" <<building->y2
        << " z:" <<building->z
        << endl;

    int32_t creaturecount = civ->assigned_creature.size();
    out << "Creatures in this zone: " << creaturecount << endl;
    for(size_t c = 0; c < creaturecount; c++)
    {
        int32_t cindex = civ->assigned_creature.at(c);

        // print list of all units assigned to that zone
        for(size_t i = 0; i < world->units.all.size(); i++)
        {
            df::unit * creature = world->units.all[i];
            if(creature->id != cindex)
                continue;
                    
            unitInfo(out, creature, verbose);
        }
    }
}

// dump some cage info
void cageInfo(color_ostream & out, df::building* building, bool verbose = false)
{
    if(!isCage(building))
        return;

    string name;
    building->getName(&name);
    out.print("Building %i - \"%s\" - type %s (%i)",
                building->id,
                name.c_str(),
                ENUM_KEY_STR(building_type, building->getType()).c_str(),
                building->getType());
    out.print("\n");

    out << "x:"  << building->x1
        << " y:" << building->y1
        << " z:" << building->z
        << endl;

    df::building_cagest * cage = (df::building_cagest*) building;
    int32_t creaturecount = cage->assigned_creature.size();
    out << "Creatures in this cage: " << creaturecount << endl;
    for(size_t c = 0; c < creaturecount; c++)
    {
        int32_t cindex = cage->assigned_creature.at(c);

        // print list of all units assigned to that cage
        for(size_t i = 0; i < world->units.all.size(); i++)
        {
            df::unit * creature = world->units.all[i];
            if(creature->id != cindex)
                continue;
                    
            unitInfo(out, creature, verbose);
        }
    }
}

// dump some chain/restraint info
void chainInfo(color_ostream & out, df::building* building, bool list_refs = false)
{
    if(!isChain(building))
        return;

    string name;
    building->getName(&name);
    out.print("Building %i - \"%s\" - type %s (%i)",
                building->id,
                name.c_str(),
                ENUM_KEY_STR(building_type, building->getType()).c_str(),
                building->getType());
    out.print("\n");

    df::building_chainst* chain = (df::building_chainst*) building;
    if(chain->assigned)
    {
        out << "assigned: ";
        unitInfo(out, chain->assigned, true);
    }
    if(chain->chained)
    {
        out << "chained: ";
        unitInfo(out, chain->chained, true);
    }
}

command_result df_zone (color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;

    bool need_cursor = false; // for zone_info, zone_assign, ...
    bool unit_info = false;
    bool zone_info = false;
    //bool cage_info = false;
    //bool chain_info = false;
    
    bool find_unassigned = false;
    bool find_caged = false;
    bool find_uncaged = false;
    bool find_foreign = false;
    bool find_untrained = false;
    //bool find_trained = false;
    bool find_war = false;
    bool find_own = false;
    bool find_tame = false;
    bool find_male = false;
    bool find_female = false;
    bool find_egglayer = false;
    bool find_grazer = false;
    bool find_milkable = false;
    bool find_named = false;

    bool find_agemin = false;
    bool find_agemax = false;
    int target_agemin = 0;
    int target_agemax = 0;

    bool find_count = false;
    size_t target_count = 0;

    bool find_race = false;
    string target_race = "";

    bool zone_assign = false;
    bool zone_unassign = false;
    bool zone_set = false;
    bool verbose = false;
    bool all = false;
    bool unit_slaughter = false;
    static int target_zone = -1;

    for (size_t i = 0; i < parameters.size(); i++)
    {
        string & p = parameters[i];
        
        if (p == "help" || p == "?")
        {
            out << zone_help << endl;
            return CR_OK;
        }
        if (p == "filters")
        {
            out << zone_help_filters << endl;
            return CR_OK;
        }
        if (p == "examples")
        {
            out << zone_help_examples << endl;
            return CR_OK;
        }
        else if(p == "zinfo")
        {
            zone_info = true;
        }
        else if(p == "uinfo")
        {
            unit_info = true;
        }
        else if(p == "verbose")
        {
            verbose = true;
        }
        else if(p == "unassign")
        {
            zone_unassign = true;
        }
        else if(p == "assign")
        {
            // if followed by another parameter, check if it's numeric
            if(i < parameters.size()-1)
            {
                stringstream ss(parameters[i+1]);
                int new_zone = -1;
                ss >> new_zone;
                if(new_zone != -1)
                {
                    i++;
                    target_zone = new_zone;
                    out << "Assign selected unit(s) to zone #" << target_zone <<std::endl;
                }
            }
            if(target_zone == -1)
            {
                out.printerr("No zone id specified and current one is invalid!\n");
                return CR_WRONG_USAGE;
            }
            else
            {
                out << "No zone id specified. Will try to use #" << target_zone << endl;
                zone_assign = true;
            }
        }
        else if(p == "race")
        {
            if(i == parameters.size()-1)
            {
                out.printerr("No race id specified!");
                return CR_WRONG_USAGE;
            }
            else
            {
                target_race = parameters[i+1];
                i++;
                out << "Filter by race " << target_race << endl;
                find_race = true;
            }
        }
        else if(p == "foreign")
        {
            out << "Filter by 'foreign' (i.e. not from the fortress civ, can be a dwarf)." << endl;
            find_foreign = true;
        }
        else if(p == "unassigned")
        {
            out << "Filter by 'unassigned'." << endl;
            find_unassigned = true;
        }
        else if(p == "caged")
        {
            out << "Filter by 'caged' (ignores built cages)." << endl;
            find_caged = true;
        }
        else if(p == "uncaged")
        {
            out << "Filter by 'uncaged'." << endl;
            find_uncaged = true;
        }
        else if(p == "untrained")
        {
            out << "Filter by 'untrained'." << endl;
            find_untrained = true;
        }
        else if(p == "war")
        {
            out << "Filter by 'trained war creature'." << endl;
            find_war = true;
        }
        else if(p == "own")
        {
            out << "Filter by 'own civilization'." << endl;
            find_own = true;
        }
        else if(p == "tame")
        {
            out << "Filter by 'tame'." << endl;
            find_tame = true;
        }
        else if(p == "named")
        {
            out << "Filter by 'has name or nickname'." << endl;
            find_named = true;
        }
        else if(p == "slaughter")
        {
            out << "Assign animals for slaughter." << endl;
            unit_slaughter = true;
        }
        else if(p == "count")
        {
            if(i == parameters.size()-1)
            {
                out.printerr("No count specified!");
                return CR_WRONG_USAGE;
            }
            else
            {
                stringstream ss(parameters[i+1]);
                i++;
                ss >> target_count;
                if(target_count <= 0)
                {
                    out.printerr("Invalid count specified (must be > 0)!");
                    return CR_WRONG_USAGE;
                }
                find_count = true;
                out << "Process up to " << target_count << " units." << endl;
            }
        }
        else if(p == "age")
        {
            if(i == parameters.size()-1)
            {
                out.printerr("No age specified!");
                return CR_WRONG_USAGE;
            }
            else
            {
                stringstream ss(parameters[i+1]);
                i++;
                ss >> target_agemin;
                ss >> target_agemax;
                find_agemin = true;
                find_agemax = true;
                out << "Filter by exact age of " << target_agemin << endl;
            }
        }
        else if(p == "minage")
        {
            if(i == parameters.size()-1)
            {
                out.printerr("No age specified!");
                return CR_WRONG_USAGE;
            }
            else
            {
                stringstream ss(parameters[i+1]);
                i++;
                ss >> target_agemin;
                find_agemin = true;
                out << "Filter by minimum age of " << target_agemin << endl;
            }
        }
        else if(p == "maxage")
        {
            if(i == parameters.size()-1)
            {
                out.printerr("No age specified!");
                return CR_WRONG_USAGE;
            }
            else
            {
                stringstream ss(parameters[i+1]);
                i++;
                ss >> target_agemax;
                find_agemax = true;
                out << "Filter by maximum age of " << target_agemax << endl;
            }
        }
        else if(p == "male")
        {
            find_male = true;
        }
        else if(p == "female")
        {
            find_female = true;
        }
        else if(p == "egglayer")
        {
            find_egglayer = true;
        }
        else if(p == "grazer")
        {
            find_grazer = true;
        }
        else if(p == "milkable")
        {
            find_milkable = true;
        }
        else if(p == "set")
        {
            zone_set = true;
        }
        else if(p == "all")
        {
            out << "Filter: all" << endl;
            all = true;
        }
        else
        {
            out << "Unknown command: " << p << endl;
            return CR_WRONG_USAGE;
        }
    }

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    if((zone_info && !all) || zone_set)
        need_cursor = true;

    if(need_cursor && cursor->x == -30000)
    {
        out.printerr("No cursor; place cursor over activity zone.\n");
        return CR_FAILURE;
    }

    // search for male and female is exclusive, so drop the flags if both are specified
    if(find_male && find_female)
    {
        find_male=false;
        find_female=false;
    }

    // try to cope with user dumbness
    if(target_agemin > target_agemax)
    {
        out << "Invalid values for minage/maxage specified! I'll swap them." << endl;
        int oldmin = target_agemin;
        target_agemin = target_agemax;
        target_agemax = oldmin;
    }

    // give info on zone(s), chain or cage under cursor
    // (doesn't use the findXyzAtCursor() methods because zones might overlap and contain a cage or chain)
    if(zone_info) // || chain_info || cage_info)
    {
        for (size_t b = 0; b < world->buildings.all.size(); b++)
        {
            df::building * building = world->buildings.all[b];

            // find building under cursor
            if (!all &&
                !(building->x1 <= cursor->x && cursor->x <= building->x2 &&
                building->y1 <= cursor->y && cursor->y <= building->y2 &&
                building->z == cursor->z))
                continue;

            zoneInfo(out, building, verbose);
            chainInfo(out, building, verbose);
            cageInfo(out, building, verbose);
        }
        return CR_OK;
    }

    // set building at cursor position to be new target zone
    if(zone_set)
    {
        target_zone = findPenPitAtCursor();
        if(target_zone==-1)
        {
            out << "No pen/pasture or pit under cursor!" << endl;
            return CR_WRONG_USAGE;
        }
        out << "Current zone set to #" << target_zone << endl;
        return CR_OK;
    }

    // assign to pen or pit
    if(zone_assign || unit_info || unit_slaughter)
    {
        df::building * building;
        if(zone_assign)
        {
            // try to get building index from the id
            int32_t index = findBuildingIndexById(target_zone);
            if(index == -1)
            {
                out << "Invalid building id." << endl;
                target_zone = -1;
                return CR_WRONG_USAGE;
            }
            building = world->buildings.all.at(index);
        }

        if(all || find_count)
        {
            size_t count = 0;
            for(size_t c = 0; c < world->units.all.size(); c++)
            {
                df::unit *unit = world->units.all[c];
                if(find_race && getRaceName(unit) != target_race)
                    continue;
                // ignore own dwarves by default
                if(isOwnCiv(unit) && isOwnRace(unit))
                {
                    if(!find_race)
                    {
                        continue;
                    }
                }

                if(    (find_unassigned && isAssigned(unit))
                    || (isContainedInItem(unit) && (find_uncaged || isBuiltCageAtPos(unit->pos)))
                    || (isChained(unit))
                    || (find_caged && !isContainedInItem(unit))
                    || (find_own && !isOwnCiv(unit))
                    || (find_foreign && isOwnCiv(unit))
                    || (find_tame && !isTame(unit))
                    || (find_untrained && isTrained(unit))
                    || (find_war && !isWar(unit))
                    || (find_agemin && getUnitAge(unit)<target_agemin)
                    || (find_agemax && getUnitAge(unit)>target_agemax)
                    || (find_grazer && !isGrazer(unit))
                    || (find_egglayer && !isEggLayer(unit))
                    || (find_milkable && !isMilkable(unit))
                    || (find_male && !isMale(unit))
                    || (find_female && !isFemale(unit))
                    || (find_named && !unit->name.has_name)
                    )
                    continue;

                if(!hasValidMapPos(unit))
                {
                    uint32_t max_x, max_y, max_z;
                    Maps::getSize(max_x, max_y, max_z);
                    out << "("<<max_x << "/"<< max_y << "/" << max_z << "). map max" << endl;
                    out << "invalid unit pos: " ;
                    out << "("<<unit->pos.x << "/"<< unit->pos.y << "/" << unit->pos.z << "). will skip this unit" << endl;
                    continue;
                }

                if(unit_info)
                {
                    unitInfo(out, unit, verbose);
                }
                else if(zone_assign)
                {            
                    command_result result = assignUnitToBuilding(out, unit, building, verbose);
                    if(result != CR_OK)
                        return result;
                }
                else if(unit_slaughter)
                {
                    // don't slaughter named creatures unless told to do so
                    if(!find_named && unit->name.has_name)
                        continue;
                    doMarkForSlaughter(unit);
                }

                count++;
                if(find_count && count >= target_count)
                    break;
            }
            out << "Processed creatures: " << count << endl;
        }
        else
        {
            // must have unit selected
            df::unit *unit = getSelectedUnit(out);
            if (!unit)
            {
                out << "No unit selected." << endl;
                return CR_WRONG_USAGE;
            }

            if(unit_info)
            {
                unitInfo(out, unit, verbose);
                return CR_OK;
            }
            else if(zone_assign)
            {
                return assignUnitToBuilding(out, unit, building, verbose);
            }
            else if(unit_slaughter)
            {
                // by default behave like the game: only allow slaughtering of named war/hunting pets
                //if(unit->name.has_name && !find_named && !(isWar(unit)||isHunter(unit)))
                //{
                //    out << "Slaughter of named unit denied. Use the filter 'named' to override this check." << endl;
                //    return CR_OK;
                //}

                doMarkForSlaughter(unit);
                return CR_OK;
            }
            
        }
    }

    // de-assign from pen or pit
    // using the zone tool to free creatures from cages or chains 
    // is pointless imo since that is already quite easy using the ingame UI. 
    // but it's easy to implement so I might as well add it later
    if(zone_unassign)
    {
        // must have unit selected
        df::unit *unit = getSelectedUnit(out);
        if (!unit)
        {
            out << "No unit selected." << endl;
            return CR_WRONG_USAGE;
        }

        // remove assignment reference from unit and old zone
        if(unassignUnitFromZone(unit))
            out << "Unit unassigned." << endl;
        else
            out << "Unit is not assigned to an activity zone!" << endl;
        return CR_OK;
    }

    return CR_OK;
}

command_result df_autonestbox(color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;

    bool verbose = false;

    for (size_t i = 0; i < parameters.size(); i++)
    {
        string & p = parameters[i];
        
        if (p == "help" || p == "?")
        {
            out << autonestbox_help << endl;
            return CR_OK;
        }
        if (p == "start")
        {
            enable_autonestbox = true;
            autonestbox_did_complain = false;
            out << "Autonestbox started.";
            return autoNestbox(out, verbose);
            //return CR_OK;
        }
        if (p == "stop")
        {
            enable_autonestbox = false;
            out << "Autonestbox stopped.";
            return CR_OK;
        }
        else if(p == "verbose")
        {
            verbose = true;
        }
        else if(p == "sleep")
        {
            if(i == parameters.size()-1)
            {
                out.printerr("No duration specified!");
                return CR_WRONG_USAGE;
            }
            else
            {
                size_t ticks = 0;
                stringstream ss(parameters[i+1]);
                i++;
                ss >> ticks;
                if(ticks <= 0)
                {
                    out.printerr("Invalid duration specified (must be > 0)!");
                    return CR_WRONG_USAGE;
                }
                sleep_autonestbox = ticks;
                out << "New sleep timer for autonestbox: " << ticks << " ticks." << endl;
                return CR_OK;
            }
        }
        else
        {
            out << "Unknown command: " << p << endl;
            return CR_WRONG_USAGE;
        }
    }
    return autoNestbox(out, verbose);
    //return CR_OK;
}

command_result autoNestbox( color_ostream &out, bool verbose = false )
{
    bool stop = false;
    size_t processed = 0;

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        enable_autonestbox = false;
        return CR_FAILURE;
    }

    do
    {
        df::building * free_building = findFreeNestboxZone();
        df::unit * free_unit = findFreeEgglayer();
        if(free_building && free_unit)
        {
            command_result result = assignUnitToBuilding(out, free_unit, free_building, verbose);
            if(result != CR_OK)
                return result;
            processed ++;
            //if(find_count && processed >= target_count)
            //    stop = true;
        }
        else
        {
            stop = true;
            if(free_unit && !free_building)
            {
                static size_t old_count = 0;
                size_t freeEgglayers = countFreeEgglayers();
                // avoid spamming the same message
                if(old_count != freeEgglayers)
                    autonestbox_did_complain = false;
                old_count = freeEgglayers;
                if(!autonestbox_did_complain)
                {
                    stringstream ss;
                    ss << freeEgglayers;
                    string announce = "Not enough free nestbox zones found! You need " + ss.str() + " more.";
                    Gui::showAnnouncement(announce, 6, true);
                    out << announce << endl;
                    autonestbox_did_complain = true;
                }
            }
        }
    } while (!stop);
    if(processed > 0)
    {
        stringstream ss;
        ss << processed;
        string announce;
        announce = ss.str() + " nestboxes were assigned.";
        Gui::showAnnouncement(announce, 2, false);
        out << announce << endl;
        // can complain again
        // (might lead to spamming the same message twice, but catches the case 
        // where for example 2 new egglayers hatched right after 2 zones were created and assigned)
        autonestbox_did_complain = false;
    }
    return CR_OK;
}


// getUnitAge() returns 0 if born in current year, therefore the look at birth_time in that case
// (assuming that the value from there indicates in which tick of the current year the unit was born)
bool compareUnitAgesYounger(df::unit* i, df::unit* j) 
{
    int32_t age_i = getUnitAge(i);
    int32_t age_j = getUnitAge(j);
    if(age_i == 0 && age_j == 0)
    {
        age_i = i->relations.birth_time;
        age_j = j->relations.birth_time;
    }
    return (age_i < age_j); 
}
bool compareUnitAgesOlder(df::unit* i, df::unit* j) 
{ 
    int32_t age_i = getUnitAge(i);
    int32_t age_j = getUnitAge(j);
    if(age_i == 0 && age_j == 0)
    {
        age_i = i->relations.birth_time;
        age_j = j->relations.birth_time;
    }
    return (age_i > age_j); 
}

//enum WatchedRaceSubtypes
//{
//    femaleKid=0,
//    maleKid,
//    femaleAdult,
//    maleAdult
//};

struct WatchedRace
{
public:

    bool isWatched; // if true, autobutcher will process this race
    int raceId;
    int fk; // max female kids
    int mk; // max male kids
    int fa; // max female adults
    int ma; // max male adults

    // bah, this should better be an array of 4 vectors
    // that way there's no need for the 4 ugly process methods
    vector <df::unit*> fk_ptr;
    vector <df::unit*> mk_ptr;
    vector <df::unit*> fa_ptr;
    vector <df::unit*> ma_ptr;
    
    WatchedRace(bool watch, int id, int _fk, int _mk, int _fa, int _ma)
    {
        isWatched = watch;
        raceId = id;
        fk = _fk;
        mk = _mk;
        fa = _fa;
        ma = _ma;
    }

    ~WatchedRace()
    {
        ClearUnits();
    }

    void SortUnitsByAge()
    {
        sort(fk_ptr.begin(), fk_ptr.end(), compareUnitAgesOlder);
        sort(mk_ptr.begin(), mk_ptr.end(), compareUnitAgesOlder);
        sort(fa_ptr.begin(), fa_ptr.end(), compareUnitAgesYounger);
        sort(ma_ptr.begin(), ma_ptr.end(), compareUnitAgesYounger);
    }
    
    void PushUnit(df::unit * unit)
    {
        if(isFemale(unit))
        {
            if(isBaby(unit) || isChild(unit))
                fk_ptr.push_back(unit);
            else
                fa_ptr.push_back(unit);
        }
        else //treat sex n/a like it was male
        {
            if(isBaby(unit) || isChild(unit))
                mk_ptr.push_back(unit);
            else
                ma_ptr.push_back(unit);
        }
    }

    void ClearUnits()
    {
        fk_ptr.clear();
        mk_ptr.clear();
        fa_ptr.clear();
        ma_ptr.clear();
    }

    int ProcessUnits_fk()
    {
        int subcount = 0;
        while(fk_ptr.size() > fk)
        {
            df::unit* unit = fk_ptr.back();
            doMarkForSlaughter(unit);
            fk_ptr.pop_back();
            subcount++;
        }
        return subcount;
    }
    int ProcessUnits_mk()
    {
        int subcount = 0;
        while(mk_ptr.size() > mk)
        {
            df::unit* unit = mk_ptr.back();
            doMarkForSlaughter(unit);
            mk_ptr.pop_back();
            subcount++;
        }
        return subcount;
    }
    int ProcessUnits_fa()
    {
        int subcount = 0;
        while(fa_ptr.size() > fa)
        {
            df::unit* unit = fa_ptr.back();
            doMarkForSlaughter(unit);
            fa_ptr.pop_back();
            subcount++;
        }
        return subcount;
    }
    int ProcessUnits_ma()
    {
        int subcount = 0;
        while(ma_ptr.size() > ma)
        {
            df::unit* unit = ma_ptr.back();
            doMarkForSlaughter(unit);
            ma_ptr.pop_back();
            subcount++;
        }
        return subcount;
    }

    int ProcessUnits()
    {
        SortUnitsByAge();
        int slaughter_count = 0;
        slaughter_count += ProcessUnits_fk();
        slaughter_count += ProcessUnits_mk();
        slaughter_count += ProcessUnits_fa();
        slaughter_count += ProcessUnits_ma();
        ClearUnits();
        return slaughter_count;
    }
};
// vector of races handled by autobutcher
// the name is a bit misleading since entries can be set to 'unwatched' 
// to ignore them for a while but still keep the target count settings
std::vector<WatchedRace*> watched_races;

command_result autobutcher_cleanup(color_ostream &out)
{
    for(size_t i=0; i<watched_races.size(); i++)
    {
        //watched_races.ClearUnits();
        delete watched_races[i];
    }
    watched_races.clear();
    return CR_OK;
}

command_result df_autobutcher(color_ostream &out, vector <string> & parameters)
{
    // move those somewhere else for persistency
    static int default_fk = 5;
    static int default_mk = 1;
    static int default_fa = 5;
    static int default_ma = 1;

    CoreSuspender suspend;

    bool verbose = false;
    bool watch_race = false;
    bool unwatch_race = false;
    bool forget_race = false;
    bool list_watched = false;
    bool change_target = false;
    string target_racename = "";
    int target_fk = default_fk;
    int target_mk = default_mk;
    int target_fa = default_fa;
    int target_ma = default_ma;

    int32_t target_raceid = -1;

    for (size_t i = 0; i < parameters.size(); i++)
    {
        string & p = parameters[i];
        
        if (p == "help" || p == "?")
        {
            out << autobutcher_help << endl;
            return CR_OK;
        }
        if (p == "example")
        {
            out << autobutcher_help_example << endl;
            return CR_OK;
        }
        else if (p == "start")
        {
            enable_autobutcher = true;
            out << "Autobutcher started.";
            return autoButcher(out, verbose);
        }
        else if (p == "stop")
        {
            enable_autobutcher = false;
            out << "Autobutcher stopped.";
            return CR_OK;
        }
        else if(p == "verbose")
        {
            verbose = true;
        }
        else if(p == "sleep")
        {
            if(i == parameters.size()-1)
            {
                out.printerr("No duration specified!");
                return CR_WRONG_USAGE;
            }
            else
            {
                size_t ticks = 0;
                stringstream ss(parameters[i+1]);
                i++;
                ss >> ticks;
                if(ticks <= 0)
                {
                    out.printerr("Invalid duration specified (must be > 0)!");
                    return CR_WRONG_USAGE;
                }
                sleep_autobutcher = ticks;
                out << "New sleep timer for autobutcher: " << ticks << " ticks." << endl;
                return CR_OK;
            }
        }
        else if(p == "watch")
        {
            if(i == parameters.size()-1)
            {
                out.printerr("No race specified!");
                return CR_WRONG_USAGE;
            }
            else
            {
                // todo: parse more than one race
                target_racename = parameters[i+1];
                i++;
                out << "Start watching race " << target_racename << endl;
                watch_race = true;
            }
        }
        else if(p == "unwatch")
        {
            if(i == parameters.size()-1)
            {
                out.printerr("No race specified!");
                return CR_WRONG_USAGE;
            }
            else
            {
                // todo: parse more than one race
                target_racename = parameters[i+1];
                i++;
                out << "Stop watching race " << target_racename << endl;
                unwatch_race = true;
            }
        }
        else if(p == "forget")
        {
            if(i == parameters.size()-1)
            {
                out.printerr("No race specified!");
                return CR_WRONG_USAGE;
            }
            else
            {
                // todo: parse more than one race
                target_racename = parameters[i+1];
                i++;
                out << "Forget settings for race " << target_racename << endl;
                forget_race = true;
            }
        }
        else if(p == "target")
        {
            // needs at least 5 more parameters:
            // fk mk fa ma R (can have more than 1 R)
            if(parameters.size() < 6)
            {
                out.printerr("Not enough parameters!");
                return CR_WRONG_USAGE;
            }
            else
            {
                stringstream fk(parameters[i+1]);
                stringstream mk(parameters[i+2]);
                stringstream fa(parameters[i+3]);
                stringstream ma(parameters[i+4]);
                fk >> target_fk;
                mk >> target_mk;
                fa >> target_fa;
                ma >> target_ma;
                
                // todo: parse more than one race, handle 'all' and 'new'
                target_racename = parameters[i+5];
                i+=5;
                out << "Target count for " << target_racename << ":"
                    << " fk=" << target_fk
                    << " mk=" << target_mk
                    << " fa=" << target_fa
                    << " ma=" << target_ma
                    << endl;
                change_target = true;
            }
        }
        else if(p == "autowatch")
        {
            out << "not supported yet, sorry" << endl;
            return CR_OK;
        }
        else if(p == "noautowatch")
        {
            out << "not supported yet, sorry" << endl;
            return CR_OK;
        }
        else if(p == "list")
        {
            list_watched = true;
        }
        else
        {
            out << "Unknown command: " << p << endl;
            return CR_WRONG_USAGE;
        }
    }

    if( target_racename == "all" ||
        target_racename == "new" )
    {
        out << "'all' and 'new' are not supported yet, sorry." << endl;
        return CR_OK;
    }

    if(list_watched)
    {
        for(size_t i=0; i<watched_races.size(); i++)
        {
            WatchedRace * w = watched_races[i];
            df::creature_raw * raw = df::global::world->raws.creatures.all[w->raceId];
            string name = raw->creature_id;
            if(w->isWatched)
                out << "watched: ";
            else
                out << "not watched: ";
            out << name 
                << " fk=" << w->fk
                << " mk=" << w->mk
                << " fa=" << w->fa
                << " ma=" << w->ma
                << endl;
        }
        return CR_OK;
    }

    size_t num_races = df::global::world->raws.creatures.all.size(); 
    bool found_race = false;
    for(size_t i=0; i<num_races; i++)
    {
        df::creature_raw *raw = df::global::world->raws.creatures.all[i];
        if(raw->creature_id == target_racename)
        {
            target_raceid = i;
            found_race = true;
            break;
        }
    }
    if(!found_race)
    {
        out << "Race not found!" << endl;
        return CR_OK;
    }

    if(unwatch_race)
    {
        bool found = false;
        for(size_t i=0; i<watched_races.size(); i++)
        {
            WatchedRace * w = watched_races[i];
            if(w->raceId == target_raceid)
            {
                found=true;
                w->isWatched=false;
                break;
            }
        }
        if(found)
            out << target_racename << " is not watched anymore." << endl;
        else
            out << target_racename << " was not being watched!" << endl;
        return CR_OK;
    }

    if(watch_race || change_target)
    {
        bool watching = false;
        for(size_t i=0; i<watched_races.size(); i++)
        {
            WatchedRace * w = watched_races[i];
            if(w->raceId == target_raceid)
            {
                if(watch_race)
                {
                    if(w->isWatched)
                        out << target_racename << " is already being watched." << endl;
                    w->isWatched = true;
                    watching = true;
                }

                if(change_target)
                {
                    w->fk = target_fk;
                    w->mk = target_mk;
                    w->fa = target_fa;
                    w->ma = target_ma;
                }
                break;
            }
        }
        if(!watching)
        {
            WatchedRace * w = new WatchedRace(watch_race, target_raceid, target_fk, target_mk, target_fa, target_ma);
            watched_races.push_back(w);
        }
        out << target_racename << " is now being watched." << endl;
        return CR_OK;
    }

    if(forget_race)
    {
        bool watched = false;
        for(size_t i=0; i<watched_races.size(); i++)
        {
            WatchedRace * w = watched_races[i];
            if(w->raceId == target_raceid)
            {
                watched_races.erase(watched_races.begin()+i);

                if(w->isWatched)
                    out << target_racename << " is already being watched." << endl;
                w->isWatched = true;
                watched = true;
                break;
            }
        }
        if(!watched)
        {
            out << target_racename << " was not on the watchlist." << endl;
            return CR_OK;
        }
        out << target_racename << " was forgotten." << endl;
        return CR_OK;
    }

    return CR_OK;
}

int getWatchedIndex(int id)
{
    for(size_t i=0; i<watched_races.size(); i++)
    {
        WatchedRace * w = watched_races[i];
        if(w->raceId == id && w->isWatched)
            return i;
    }
    return -1;
}

command_result autoButcher( color_ostream &out, bool verbose = false )
{
    // don't run if not supposed to
    if(!Maps::IsValid())
        return CR_OK;

    // check if there is anything to watch before walking through units vector
    bool watching = false;
    for(size_t i=0; i<watched_races.size(); i++)
    {
        WatchedRace * w = watched_races[i];
        if(w->isWatched)
        {
            watching = true;
            break;
        }
    }
    if(!watching)
        return CR_OK;

    for(size_t i=0; i<world->units.all.size(); i++)
    {
        df::unit * unit = world->units.all[i];
        if(    isDead(unit)
            || isMarkedForSlaughter(unit)
            || !isOwnCiv(unit)
            || !isTame(unit)
            || unit->name.has_name
            )
            continue;

        // found a bugged unit which had invalid coordinates.
        // marking it for slaughter didn't seem to have negative effects, but you never know...
        if(!hasValidMapPos(unit))
            continue;

        int watched_index = getWatchedIndex(unit->race);
        if(watched_index != -1)
        {
            WatchedRace * w = watched_races[watched_index];
            w->PushUnit(unit);
        }
    }

    int slaughter_count = 0;
    for(size_t i=0; i<watched_races.size(); i++)
    {
        WatchedRace * w = watched_races[i];
        int slaughter_subcount = w->ProcessUnits();
        slaughter_count += slaughter_subcount;
        df::creature_raw *raw = df::global::world->raws.creatures.all[w->raceId];
        out << raw->creature_id << " marked for slaughter: " << slaughter_subcount << endl;
    }
    out << slaughter_count << " units total marked for slaughter." << endl;

    return CR_OK;
}