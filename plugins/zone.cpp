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
// - mass-assign creatures using filters
// - unassign single creature under cursor from current zone
// - pitting own dwarves :)
// - full automation of handling mini-pastures over nestboxes:
//   go through all pens, check if they are empty and placed over a nestbox
//   find female tame egg-layer who is not assigned to another pen and assign it to nestbox pasture
//   maybe check for minimum age? it's not that useful to fill nestboxes with freshly hatched birds
//   state and sleep setting is saved the first time autonestbox is started (to avoid writing stuff if the plugin is never used)
// - full automation of marking live-stock for slaughtering
//   races can be added to a watchlist and it can be set how many male/female kids/adults are left alive
//   adding to the watchlist can be automated as well.
//   config for autobutcher (state and sleep setting) is saved the first time autobutcher is started
//   config for watchlist entries is saved when they are created or modified

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
#include "MiscUtils.h"
#include "uicommon.h"

#include "LuaTools.h"
#include "DataFuncs.h"

#include "modules/Units.h"
#include "modules/Maps.h"
#include "modules/Gui.h"
#include "modules/Materials.h"
#include "modules/MapCache.h"
#include "modules/Buildings.h"
#include "modules/World.h"
#include "modules/Screen.h"
#include "MiscUtils.h"
#include <VTableInterpose.h>

#include "df/ui.h"
#include "df/world.h"
#include "df/world_raws.h"
#include "df/building_def.h"
#include "df/building_civzonest.h"
#include "df/building_cagest.h"
#include "df/building_chainst.h"
#include "df/building_nest_boxst.h"
#include "df/general_ref_building_civzone_assignedst.h"
#include <df/creature_raw.h>
#include <df/caste_raw.h>
#include "df/unit_soul.h"
#include "df/unit_wound.h"
#include "df/viewscreen_dwarfmodest.h"
#include "modules/Translation.h"

using std::vector;
using std::string;
using namespace DFHack;
using namespace DFHack::Units;
using namespace DFHack::Buildings;
using namespace df::enums;

DFHACK_PLUGIN("zone");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(cursor);
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(ui_build_selector);
REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(cur_year);
REQUIRE_GLOBAL(cur_year_tick);

REQUIRE_GLOBAL(ui_building_item_cursor);
REQUIRE_GLOBAL(ui_building_assign_type);
REQUIRE_GLOBAL(ui_building_assign_is_marked);
REQUIRE_GLOBAL(ui_building_assign_units);
REQUIRE_GLOBAL(ui_building_assign_items);
REQUIRE_GLOBAL(ui_building_in_assign);

REQUIRE_GLOBAL(ui_menu_width);
REQUIRE_GLOBAL(ui_area_map_width);

using namespace DFHack::Gui;

command_result df_zone (color_ostream &out, vector <string> & parameters);
command_result df_autonestbox (color_ostream &out, vector <string> & parameters);
command_result df_autobutcher(color_ostream &out, vector <string> & parameters);

DFhackCExport command_result plugin_enable ( color_ostream &out, bool enable);

const string zone_help =
    "Allows easier management of pens/pastures, pits and cages.\n"
    "Options:\n"
    "  set          - set zone under cursor as default for future assigns\n"
    "  assign       - assign creature(s) to a pen or pit\n"
    "                 if no filters are used, a single unit must be selected.\n"
    "                 can be followed by valid building id which will then be set.\n"
    "                 building must be a pen/pasture, pit or cage.\n"
    "  slaughter    - mark creature(s) for slaughter\n"
    "                 if no filters are used, a single unit must be selected.\n"
    "                 with filters named units are ignored unless specified.\n"
    "  unassign     - unassign selected creature(s) from it's zone or cage\n"
    "  nick         - give unit(s) nicknames (e.g. all units in a cage)\n"
    "  remnick      - remove nicknames\n"
    "  tocages      - assign to (multiple) built cages inside a pen/pasture\n"
    "                 spreads creatures evenly among cages for faster hauling.\n"
    "  uinfo        - print info about selected unit\n"
    "  zinfo        - print info about zone(s) under cursor\n"
    "  verbose      - print some more info, mostly useless debug stuff\n"
    "  filters      - print list of supported filters\n"
    "  examples     - print some usage examples\n"
    ;

const string zone_help_filters =
    "Filters (to be used in combination with 'all' or 'count'):\n"
    "These filters can not be used with the prefix 'not':"
    "  all          - in combinations with zinfo/uinfo: print all zones/units\n"
    "                 in combination with assign: process all units\n"
    "                 should be used in combination with further filters\n"
    "  count        - must be followed by number. process X units\n"
    "                 should be used in combination with further filters\n"
    "  unassigned   - not assigned to zone, chain or built cage\n"
    "  age          - exact age. must be followed by number\n"
    "  minage       - minimum age. must be followed by number\n"
    "  maxage       - maximum age. must be followed by number\n"
    "These filters can be used with the prefix 'not' (e.g. 'not own'):"
    "  race         - must be followed by a race raw id (e.g. BIRD_TURKEY)\n"
    "  caged        - in a built cage\n"
    "  own          - from own civilization\n"
    "  war          - trained war creature\n"
    "  tame         - tamed\n"
    "  named        - has name or nickname\n"
    "                 can be used to mark named units for slaughter\n"
    "  merchant     - is a merchant / belongs to a merchant\n"
    "                 can be used to pit merchants and slaughter their animals\n"
    "                 (could have weird effects during trading, be careful)\n"
    "  trained      - obvious\n"
    "  trainablewar - can be trained for war (and is not already trained)\n"
    "  trainablehunt- can be trained for hunting (and is not already trained)\n"
    "  male         - obvious\n"
    "  female       - obvious\n"
    "  egglayer     - race lays eggs (use together with 'female')\n"
    "  grazer       - is a grazer\n"
    "  milkable     - race is milkable (use together with 'female')\n"
    ;

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
    "  zone assign all own milkable not grazer\n"
    "  zone assign count 5 own female milkable\n"
    "  zone assign all own race DWARF maxage 2\n"
    "    throw all useless kids into a pit :)\n"
    "Notes:\n"
    "  Unassigning per filters ignores built cages and chains currently. Usually you\n"
    "  should always use the filter 'own' (which implies tame) unless you want to\n"
    "  use the zone tool for pitting hostiles. 'own' ignores own dwarves unless you\n"
    "  specify 'race DWARF' and it ignores merchants and their animals unless you\n"
    "  specify 'merchant' (so it's safe to use 'assign all own' to one big pasture\n"
    "  if you want to have all your animals at the same place).\n"
    "  'egglayer' and 'milkable' should be used together with 'female'\n"
    "  well, unless you have a mod with egg-laying male elves who give milk...\n";


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
    "that you add the target race(s) to a watch list. Only tame units will be\n"
    "processed. Named units will be completely ignored (you can give animals\n"
    "nicknames with the tool 'rename unit' to protect them from getting slaughtered\n"
    "automatically. Trained war or hunting pets will be ignored.\n"
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
    "                 or a list of RAW ids seperated by spaces\n"
    "                 or the keyword 'all' which affects your whole current watchlist.\n"
    "  unwatch R    - stop watching race(s)\n"
    "                 the current target settings will be remembered\n"
    "  forget R     - unwatch race(s) and forget target settings for it/them\n"
    "  autowatch    - automatically adds all new races (animals you buy\n"
    "                 from merchants, tame yourself or get from migrants)\n"
    "                 to the watch list using default target count\n"
    "  noautowatch  - stop auto-adding new races to the watch list\n"
    "  list         - print status and watchlist\n"
    "  list_export  - print status and watchlist in batchfile format\n"
    "                 can be used to copy settings into another savegame\n"
    "                 usage: 'dfhack-run autobutcher list_export > xyz.bat' \n"
    "  target fk mk fa ma R\n"
    "               - set target count for specified race:\n"
    "                 fk = number of female kids\n"
    "                 mk = number of male kids\n"
    "                 fa = number of female adults\n"
    "                 ma = number of female adults\n"
    "                 R = 'all' sets count for all races on the current watchlist\n"
    "                 including the races which are currenly set to 'unwatched'\n"
    "                 and sets the new default for future watch commands\n"
    "                 R = 'new' sets the new default for future watch commands\n"
    "                 without changing your current watchlist\n"
    "  example      - print some usage examples\n";

const string autobutcher_help_example =
    "Examples:\n"
    "  autobutcher target 4 3 2 1 ALPACA BIRD_TURKEY\n"
    "  autobutcher watch ALPACA BIRD_TURKEY\n"
    "  autobutcher start\n"
    "    This means you want to have max 7 kids (4 female, 3 male) and max 3 adults\n"
    "    (2 female, 1 male) of the races alpaca and turkey. Once the kids grow up the\n"
    "    oldest adults will get slaughtered. Excess kids will get slaughtered starting\n"
    "    the the youngest to allow that the older ones grow into adults.\n"
    "  autobutcher target 0 0 0 0 new\n"
    "  autobutcher autowatch\n"
    "  autobutcher start\n"
    "    This tells autobutcher to automatically put all new races onto the watchlist\n"
    "    and mark unnamed tame units for slaughter as soon as they arrive in your\n"
    "    fortress. Settings already made for some races will be left untouched.\n";

command_result init_autobutcher(color_ostream &out);
command_result cleanup_autobutcher(color_ostream &out);
command_result start_autobutcher(color_ostream &out);

command_result init_autonestbox(color_ostream &out);
command_result cleanup_autonestbox(color_ostream &out);
command_result start_autonestbox(color_ostream &out);


///////////////
// stuff for autonestbox and autobutcher
// should be moved to own plugin once the tool methods it shares with the zone plugin are moved to Unit.h / Building.h

command_result autoNestbox( color_ostream &out, bool verbose );
command_result autoButcher( color_ostream &out, bool verbose );

static bool enable_autonestbox = false;
static bool enable_autobutcher = false;
static bool enable_autobutcher_autowatch = false;
static size_t sleep_autonestbox = 6000;
static size_t sleep_autobutcher = 6000;
static bool autonestbox_did_complain = false; // avoids message spam

static PersistentDataItem config_autobutcher;
static PersistentDataItem config_autonestbox;


DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event)
    {
    case DFHack::SC_MAP_LOADED:
        // initialize from the world just loaded
        init_autobutcher(out);
        init_autonestbox(out);
        break;
    case DFHack::SC_MAP_UNLOADED:
        enable_autonestbox = false;
        enable_autobutcher = false;
        // cleanup
        cleanup_autobutcher(out);
        cleanup_autonestbox(out);
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

bool isTame(df::unit* unit);
bool isTrained(df::unit* unit);
bool isGay(df::unit* unit);

df::building* findCageAtCursor();
df::building* findChainAtCursor();

df::general_ref_building_civzone_assignedst * createCivzoneRef();
bool unassignUnitFromBuilding(df::unit* unit);
command_result assignUnitToZone(color_ostream& out, df::unit* unit, df::building* building, bool verbose);
void unitInfo(color_ostream & out, df::unit* creature, bool verbose);
void zoneInfo(color_ostream & out, df::building* building, bool verbose);
void cageInfo(color_ostream & out, df::building* building, bool verbose);
void chainInfo(color_ostream & out, df::building* building, bool verbose);
bool isBuiltCageAtPos(df::coord pos);
bool isInBuiltCageRoom(df::unit*);
bool isNaked(df::unit *);

// ignore vampires, they should be treated like normal dwarves
bool isUndead(df::unit* unit)
{
    return (unit->flags3.bits.ghostly ||
            ( (unit->curse.add_tags1.bits.OPPOSED_TO_LIFE || unit->curse.add_tags1.bits.NOT_LIVING)
             && !unit->curse.add_tags1.bits.BLOODSUCKER ));
}

void doMarkForSlaughter(df::unit* unit)
{
    unit->flags2.bits.slaughter = 1;
}

// check if creature is tame
bool isTame(df::unit* creature)
{
    bool tame = false;
    if(creature->flags1.bits.tame)
    {
        switch (creature->training_level)
        {
        case df::animal_training_level::SemiWild: //??
        case df::animal_training_level::Trained:
        case df::animal_training_level::WellTrained:
        case df::animal_training_level::SkilfullyTrained:
        case df::animal_training_level::ExpertlyTrained:
        case df::animal_training_level::ExceptionallyTrained:
        case df::animal_training_level::MasterfullyTrained:
        case df::animal_training_level::Domesticated:
            tame=true;
            break;
        case df::animal_training_level::Unk8:     //??
        case df::animal_training_level::WildUntamed:
        default:
            tame=false;
            break;
        }
    }
    return tame;
}

// check if creature is domesticated
// seems to be the only way to really tell if it's completely safe to autonestbox it (training can revert)
bool isDomesticated(df::unit* creature)
{
    bool tame = false;
    if(creature->flags1.bits.tame)
    {
        switch (creature->training_level)
        {
        case df::animal_training_level::Domesticated:
            tame=true;
            break;
        default:
            tame=false;
            break;
        }
    }
    return tame;
}

// check if trained (might be useful if pasturing war dogs etc)
bool isTrained(df::unit* unit)
{
    // case a: trained for war/hunting (those don't have a training level, strangely)
    if(isWar(unit) || isHunter(unit))
        return true;

    // case b: tamed and trained wild creature, gets a training level
    bool trained = false;
    switch (unit->training_level)
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
    return trained;
}

// found a unit with weird position values on one of my maps (negative and in the thousands)
// it didn't appear in the animal stocks screen, but looked completely fine otherwise (alive, tame, own, etc)
// maybe a rare bug, but better avoid assigning such units to zones or slaughter etc.
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

bool isNaked(df::unit* unit)
{
    return (unit->inventory.empty());
}

bool isGay(df::unit* unit)
{
    df::orientation_flags orientation = unit->status.current_soul->orientation_flags;
    return (isFemale(unit) && ! (orientation.whole & (orientation.mask_marry_male | orientation.mask_romance_male)))
        || (! isFemale(unit) && ! (orientation.whole & (orientation.mask_marry_female | orientation.mask_romance_female)));
}

bool isGelded(df::unit* unit)
{
    auto wounds = unit->body.wounds;
    for(auto wound = wounds.begin(); wound != wounds.end(); ++wound)
    {
        auto parts = (*wound)->parts;
        for (auto part = parts.begin(); part != parts.end(); ++part)
        {
            if ((*part)->flags2.bits.gelded)
                return true;
        }
    }
    return false;
}

// dump some unit info
void unitInfo(color_ostream & out, df::unit* unit, bool verbose = false)
{
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
    out << ", age: " << getAge(unit, true);

    if(isTame(unit))
        out << ", tame";
    if(isOwnCiv(unit))
        out << ", owned";
    if(isWar(unit))
        out << ", war";
    if(isHunter(unit))
        out << ", hunter";
    if(isMerchant(unit))
        out << ", merchant";
    if(isForest(unit))
        out << ", forest";
    if(isEggLayer(unit))
        out << ", egglayer";
    if(isGrazer(unit))
        out << ", grazer";
    if(isMilkable(unit))
        out << ", milkable";

    if(verbose)
    {
        out << ". Pos: ("<<unit->pos.x << "/"<< unit->pos.y << "/" << unit->pos.z << ") " << endl;
        out << "index in units vector: " << FindIndexById(unit->id) << endl;
    }
    out << endl;

    if(!verbose)
        return;

    //out << "number of refs: " << creature->general_refs.size() << endl;
    for(size_t r = 0; r<unit->general_refs.size(); r++)
    {
        df::general_ref* ref = unit->general_refs.at(r);
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
    if(isInBuiltCageRoom(unit))
    {
        out << "in a room." << endl;
    }
}

bool isCage(df::building * building)
{
    return building && (building->getType() == building_type::Cage);
}

bool isChain(df::building * building)
{
    return building && (building->getType() == building_type::Chain);
}

// returns building of cage at cursor position (NULL if nothing found)
df::building* findCageAtCursor()
{
    df::building* building = Buildings::findAtTile(Gui::getCursorPos());
    if (isCage(building))
        return building;
    return NULL;
}

df::building* findChainAtCursor()
{
    df::building* building = Buildings::findAtTile(Gui::getCursorPos());
    if (isChain(building))
        return building;
    return NULL;
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
        for(size_t r = 0; r<creature->general_refs.size(); r++)
        {
            df::general_ref* ref;
            ref = creature->general_refs.at(r);
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

bool isInBuiltCage(df::unit* unit);

// check if assigned to pen, pit, (built) cage or chain
// note: BUILDING_CAGED is not set for animals (maybe it's used for dwarves who get caged as sentence)
// animals in cages (no matter if built or on stockpile) get the ref CONTAINED_IN_ITEM instead
// removing them from cages on stockpiles is no problem even without clearing the ref
// and usually it will be desired behavior to do so.
bool isAssigned(df::unit* unit)
{
    bool assigned = false;
    for (size_t r=0; r < unit->general_refs.size(); r++)
    {
        df::general_ref * ref = unit->general_refs[r];
        auto rtype = ref->getType();
        if(    rtype == df::general_ref_type::BUILDING_CIVZONE_ASSIGNED
            || rtype == df::general_ref_type::BUILDING_CAGED
            || rtype == df::general_ref_type::BUILDING_CHAIN
            || (rtype == df::general_ref_type::CONTAINED_IN_ITEM && isInBuiltCage(unit))
            )
        {
            assigned = true;
            break;
        }
    }
    return assigned;
}

bool isAssignedToZone(df::unit* unit)
{
    bool assigned = false;
    for (size_t r=0; r < unit->general_refs.size(); r++)
    {
        df::general_ref * ref = unit->general_refs[r];
        auto rtype = ref->getType();
        if(rtype == df::general_ref_type::BUILDING_CIVZONE_ASSIGNED)
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
    for (size_t r=0; r < unit->general_refs.size(); r++)
    {
        df::general_ref * ref = unit->general_refs[r];
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
    for (size_t r=0; r < unit->general_refs.size(); r++)
    {
        df::general_ref * ref = unit->general_refs[r];
        auto rtype = ref->getType();
        if(rtype == df::general_ref_type::CONTAINED_IN_ITEM)
        {
            contained = true;
            break;
        }
    }
    return contained;
}

bool isInBuiltCage(df::unit* unit)
{
    bool caged = false;
    for (size_t b=0; b < world->buildings.all.size(); b++)
    {
        df::building* building = world->buildings.all[b];
        if( building->getType() == building_type::Cage)
        {
            df::building_cagest* cage = (df::building_cagest*) building;
            for(size_t c=0; c<cage->assigned_units.size(); c++)
            {
                if(cage->assigned_units[c] == unit->id)
                {
                    caged = true;
                    break;
                }
            }
        }
        if(caged)
            break;
    }
    return caged;
}

// built cage defined as room (supposed to detect zoo cages)
bool isInBuiltCageRoom(df::unit* unit)
{
    bool caged_room = false;
    for (size_t b=0; b < world->buildings.all.size(); b++)
    {
        df::building* building = world->buildings.all[b];

        // !!! building->isRoom() returns true if the building can be made a room but currently isn't
        // !!! except for coffins/tombs which always return false
        // !!! using the bool is_room however gives the correct state/value
        if(!building->is_room)
            continue;

        if(building->getType() == building_type::Cage)
        {
            df::building_cagest* cage = (df::building_cagest*) building;
            for(size_t c=0; c<cage->assigned_units.size(); c++)
            {
                if(cage->assigned_units[c] == unit->id)
                {
                    caged_room = true;
                    break;
                }
            }
        }
        if(caged_room)
            break;
    }
    return caged_room;
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

df::building * getBuiltCageAtPos(df::coord pos)
{
    df::building* cage = NULL;
    for (size_t b=0; b < world->buildings.all.size(); b++)
    {
        df::building* building = world->buildings.all[b];
        if( building->getType() == building_type::Cage
            && building->x1 == pos.x
            && building->y1 == pos.y
            && building->z  == pos.z )
        {
            // don't set pointer if not constructed yet
            if(building->getBuildStage()!=building->getMaxBuildStage())
                break;

            cage = building;
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

bool isFreeNestboxAtPos(int32_t x, int32_t y, int32_t z)
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
            df::building_nest_boxst* nestbox = (df::building_nest_boxst*) building;
            if(nestbox->claimed_by == -1 && nestbox->contained_items.size() == 1)
            {
                found = true;
                break;
            }
        }
    }
    return found;
}

bool isEmptyPasture(df::building* building)
{
    if(!isPenPasture(building))
        return false;
    df::building_civzonest * civ = (df::building_civzonest *) building;
    if(civ->assigned_units.size() == 0)
        return true;
    else
        return false;
}

df::building* findFreeNestboxZone()
{
    df::building * free_building = NULL;
    for (size_t b=0; b < world->buildings.all.size(); b++)
    {
        df::building* building = world->buildings.all[b];
        if( isEmptyPasture(building) &&
            isActive(building) &&
            isFreeNestboxAtPos(building->x1, building->y1, building->z))
        {
            free_building = building;
            break;
        }
    }
    return free_building;
}

bool isFreeEgglayer(df::unit * unit)
{
    if( !isDead(unit) && !isUndead(unit)
        && isFemale(unit)
        && isTame(unit)
        && isOwnCiv(unit)
        && isEggLayer(unit)
        && !isAssigned(unit)
        && !isGrazer(unit) // exclude grazing birds because they're messy
        && !isMerchant(unit) // don't steal merchant mounts
        && !isForest(unit)  // don't steal birds from traders, they hate that
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
// check if unit is already assigned to a cage, remove that ref from the cage
// returns false if no cage or pasture information was found
// helps as workaround for http://www.bay12games.com/dwarves/mantisbt/view.php?id=4475 by the way
// (pastured animals assigned to chains will get hauled back and forth because the pasture ref is not deleted)
bool unassignUnitFromBuilding(df::unit* unit)
{
    bool success = false;
    for (std::size_t idx = 0; idx < unit->general_refs.size(); idx++)
    {
        df::general_ref * oldref = unit->general_refs[idx];
        switch(oldref->getType())
        {
        case df::general_ref_type::BUILDING_CIVZONE_ASSIGNED:
            {
                unit->general_refs.erase(unit->general_refs.begin() + idx);
                df::building_civzonest * oldciv = (df::building_civzonest *) oldref->getBuilding();
                for(size_t oc=0; oc<oldciv->assigned_units.size(); oc++)
                {
                    if(oldciv->assigned_units[oc] == unit->id)
                    {
                        oldciv->assigned_units.erase(oldciv->assigned_units.begin() + oc);
                        break;
                    }
                }
                delete oldref;
                success = true;
                break;
            }

        case df::general_ref_type::CONTAINED_IN_ITEM:
            {
                // game does not erase the ref until creature gets removed from cage
                //unit->general_refs.erase(unit->general_refs.begin() + idx);

                // walk through buildings, check cages for inhabitants, compare ids
                for (size_t b=0; b < world->buildings.all.size(); b++)
                {
                    bool found = false;
                    df::building* building = world->buildings.all[b];
                    if(isCage(building))
                    {
                        df::building_cagest* oldcage = (df::building_cagest*) building;
                        for(size_t oc=0; oc<oldcage->assigned_units.size(); oc++)
                        {
                            if(oldcage->assigned_units[oc] == unit->id)
                            {
                                oldcage->assigned_units.erase(oldcage->assigned_units.begin() + oc);
                                found = true;
                                break;
                            }
                        }
                    }
                    if(found)
                        break;
                }
                success = true;
                break;
            }

        case df::general_ref_type::BUILDING_CHAIN:
            {
                // try not erasing the ref and see what happens
                //unit->general_refs.erase(unit->general_refs.begin() + idx);
                // probably need to delete chain reference here
                //success = true;
                break;
            }

        case df::general_ref_type::BUILDING_CAGED:
            {
                // not sure what to do here, doesn't seem to get used by the game
                //unit->general_refs.erase(unit->general_refs.begin() + idx);
                //success = true;
                break;
            }

        default:
            {
                // some reference which probably shouldn't get deleted
                // (animals who are historical figures and have a NEMESIS reference or whatever)
                break;
            }
        }
    }
    return success;
}

// assign to pen or pit
command_result assignUnitToZone(color_ostream& out, df::unit* unit, df::building* building, bool verbose = false)
{
    // building must be a pen/pasture or pit
    if(!isPenPasture(building) && !isPitPond(building))
    {
        out << "Invalid building type. This is not a pen/pasture or pit/pond." << endl;
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
    //    testing showed that removing the ref from the unit only seems to be necessary for pastured creatures
    //    if they are in cages on stockpiles the game unassigns them automatically
    //    if they are in built cages the pointer to the creature needs to be removed from the cage
    //    TODO: check what needs to be done for chains
    bool cleared_old = unassignUnitFromBuilding(unit);

    if(verbose)
    {
        if(cleared_old)
            out << "old zone info cleared.";
        else
            out << "no old zone info found.";
    }

    ref->building_id = building->id;
    unit->general_refs.push_back(ref);

    df::building_civzonest * civz = (df::building_civzonest *) building;
    civz->assigned_units.push_back(unit->id);

    out << "Unit " << unit->id
        << "(" << getRaceName(unit) << ")"
        << " assigned to zone " << building->id;
    if(isPitPond(building))
        out << " (pit/pond).";
    if(isPenPasture(building))
        out << " (pen/pasture).";
    out << endl;

    return CR_OK;
}

command_result assignUnitToCage(color_ostream& out, df::unit* unit, df::building* building, bool verbose)
{
    // building must be a pen/pasture or pit
    if(!isCage(building))
    {
        out << "Invalid building type. This is not a cage." << endl;
        return CR_WRONG_USAGE;
    }

    // don't assign owned pets to a cage. the owner will release them, resulting into infinite hauling (df bug)
    if(unit->relations.pet_owner_id != -1)
        return CR_OK;

    // check if unit is already pastured or caged, remove refs where necessary
    bool cleared_old = unassignUnitFromBuilding(unit);
    if(verbose)
    {
        if(cleared_old)
            out << "old zone info cleared.";
        else
            out << "no old zone info found.";
    }

    //ref->building_id = building->id;
    //unit->general_refs.push_back(ref);

    df::building_cagest* civz = (df::building_cagest*) building;
    civz->assigned_units.push_back(unit->id);

    out << "Unit " << unit->id
        << "(" << getRaceName(unit) << ")"
        << " assigned to cage " << building->id;
    out << endl;

    return CR_OK;
}

command_result assignUnitToChain(color_ostream& out, df::unit* unit, df::building* building, bool verbose)
{
    out << "sorry. assigning to chains is not possible yet." << endl;
    return CR_WRONG_USAGE;
}

command_result assignUnitToBuilding(color_ostream& out, df::unit* unit, df::building* building, bool verbose)
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

command_result assignUnitsToCagezone(color_ostream& out, vector<df::unit*> units, df::building* building, bool verbose)
{
    if(!isPenPasture(building))
    {
        out << "A cage zone needs to be a pen/pasture containing at least one cage!" << endl;
        return CR_WRONG_USAGE;
    }

    int32_t x1 = building->x1;
    int32_t x2 = building->x2;
    int32_t y1 = building->y1;
    int32_t y2 = building->y2;
    int32_t z  = building->z;
    vector <df::building_cagest*> cages;
    for (int32_t x = x1; x<=x2; x++)
    {
        for (int32_t y = y1; y<=y2; y++)
        {
            df::building* cage = getBuiltCageAtPos(df::coord(x,y,z));
            if(cage)
            {
                df::building_cagest* cagest = (df::building_cagest*) cage;
                cages.push_back(cagest);
            }
        }
    }
    if(!cages.size())
    {
        out << "No cages found in this zone!" << endl;
        return CR_WRONG_USAGE;
    }
    else
    {
        out << "Number of cages: " << cages.size() << endl;
    }

    while(units.size())
    {
        // hrm, better use sort() instead?
        df::building_cagest * bestcage = cages[0];
        size_t lowest = cages[0]->assigned_units.size();
        for(size_t i=1; i<cages.size(); i++)
        {
            if(cages[i]->assigned_units.size()<lowest)
            {
                lowest = cages[i]->assigned_units.size();
                bestcage = cages[i];
            }
        }
        df::unit* unit = units.back();
        units.pop_back();
        command_result result = assignUnitToCage(out, unit, (df::building*) bestcage, verbose);
        if(result!=CR_OK)
            return result;
    }

    return CR_OK;
}

command_result nickUnitsInZone(color_ostream& out, df::building* building, string nick)
{
    // building must be a pen/pasture or pit
    if(!isPenPasture(building) && !isPitPond(building))
    {
        out << "Invalid building type. This is not a pen/pasture or pit/pond." << endl;
        return CR_WRONG_USAGE;
    }

    df::building_civzonest * civz = (df::building_civzonest *) building;
    for(size_t i = 0; i < civz->assigned_units.size(); i++)
    {
        df::unit* unit = df::unit::find(civz->assigned_units[i]);
        if(unit)
            Units::setNickname(unit, nick);
    }

    return CR_OK;
}

command_result nickUnitsInCage(color_ostream& out, df::building* building, string nick)
{
    // building must be a pen/pasture or pit
    if(!isCage(building))
    {
        out << "Invalid building type. This is not a cage." << endl;
        return CR_WRONG_USAGE;
    }

    df::building_cagest* cage = (df::building_cagest*) building;
    for(size_t i=0; i<cage->assigned_units.size(); i++)
    {
        df::unit* unit = df::unit::find(cage->assigned_units[i]);
        if(unit)
            Units::setNickname(unit, nick);
    }

    return CR_OK;
}

command_result nickUnitsInChain(color_ostream& out, df::building* building, string nick)
{
    out << "sorry. nicknaming chained units is not possible yet." << endl;
    return CR_WRONG_USAGE;
}

// give all units inside a pasture or cage the same nickname
// (usage example: protect them from being autobutchered)
command_result nickUnitsInBuilding(color_ostream& out, df::building* building, string nick)
{
    command_result result = CR_WRONG_USAGE;

    if(isActivityZone(building))
        result = nickUnitsInZone(out, building, nick);
    else if(isCage(building))
        result = nickUnitsInCage(out, building, nick);
    else if(isChain(building))
        result = nickUnitsInChain(out, building, nick);
    else
        out << "Cannot nickname units in this type of building!" << endl;

    return result;
}

// dump some zone info
void zoneInfo(color_ostream & out, df::building* building, bool verbose)
{
    if(!isActivityZone(building))
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
    if(isActive(civ))
        out << "active";
    else
        out << "not active";

    if(civ->zone_flags.bits.pen_pasture)
        out << ", pen/pasture";
    else if (civ->zone_flags.bits.pit_pond)
    {
        out << " (pit flags:" << civ->pit_flags.whole << ")";
        if(civ->pit_flags.bits.is_pond)
            out << ", pond";
        else
            out << ", pit";
    }
    out << endl;
    out << "x1:" <<building->x1
        << " x2:" <<building->x2
        << " y1:" <<building->y1
        << " y2:" <<building->y2
        << " z:" <<building->z
        << endl;

    size_t creaturecount = civ->assigned_units.size();
    out << "Creatures in this zone: " << creaturecount << endl;
    for(size_t c = 0; c < creaturecount; c++)
    {
        int32_t cindex = civ->assigned_units.at(c);

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
void cageInfo(color_ostream & out, df::building* building, bool verbose)
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

    size_t creaturecount = cage->assigned_units.size();
    out << "Creatures in this cage: " << creaturecount << endl;
    for(size_t c = 0; c < creaturecount; c++)
    {
        int32_t cindex = cage->assigned_units.at(c);

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

    bool invert_filter = false;
    bool find_unassigned = false;
    bool find_caged = false;
    bool find_not_caged = false;
    bool find_trainable_war = false;
    bool find_not_trainable_war = false;
    bool find_trainable_hunting = false;
    bool find_not_trainable_hunting = false;
    bool find_trained = false;
    bool find_not_trained = false;
    bool find_war = false;
    bool find_not_war = false;
    bool find_hunter = false;
    bool find_not_hunter = false;
    bool find_own = false;
    bool find_not_own = false;
    bool find_tame = false;
    bool find_not_tame = false;
    bool find_merchant = false;
    bool find_male = false;
    bool find_not_male = false;
    bool find_female = false;
    bool find_not_female = false;
    bool find_egglayer = false;
    bool find_not_egglayer = false;
    bool find_grazer = false;
    bool find_not_grazer = false;
    bool find_milkable = false;
    bool find_not_milkable = false;
    bool find_named = false;
    bool find_not_named = false;
    bool find_naked = false;
    bool find_not_naked = false;
    bool find_tamable = false;
    bool find_not_tamable = false;

    bool find_agemin = false;
    bool find_agemax = false;
    int target_agemin = 0;
    int target_agemax = 0;

    bool find_count = false;
    size_t target_count = 0;

    bool find_race = false;
    bool find_not_race = false;
    string target_race = "";

    bool building_assign = false;
    bool building_unassign = false;
    bool building_set = false;
    bool cagezone_assign = false;
    bool verbose = false;
    bool all = false;
    bool unit_slaughter = false;
    static df::building* target_building = NULL;
    bool nick_set = false;
    string target_nick = "";

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
            invert_filter=false;
        }
        else if(p == "uinfo")
        {
            unit_info = true;
            invert_filter=false;
        }
        else if(p == "verbose")
        {
            verbose = true;
            if(invert_filter)
            {
                verbose = false;
                invert_filter=false;
            }
        }
        else if(p == "unassign")
        {
            if(invert_filter)
            {
                out << "'not unassign' makes no sense." << endl;
                return CR_WRONG_USAGE;
            }
            building_unassign = true;
        }
        else if(p == "assign")
        {
            if(invert_filter)
            {
                out << "'not assign' makes no sense. (did you want to use unassign?)" << endl;
                return CR_WRONG_USAGE;
            }

            // if followed by another parameter, check if it's numeric
            if(i < parameters.size()-1)
            {
                auto & str = parameters[i+1];
                if(str.size() > 0 && str[0] >= '0' && str[0] <= '9')
                {
                    stringstream ss(parameters[i+1]);
                    int new_building = -1;
                    ss >> new_building;
                    if(new_building != -1)
                    {
                        i++;
                        target_building = df::building::find(new_building);
                        out << "Assign selected unit(s) to building #" << new_building <<std::endl;
                    }
                }
            }
            if(!target_building)
            {
                out.printerr("No building id specified and current one is invalid!\n");
                return CR_WRONG_USAGE;
            }
            else
            {
                out << "No buiding id specified. Will try to use #" << target_building->id << endl;
                building_assign = true;
            }
        }
        else if(p == "tocages")
        {
            if(invert_filter)
            {
                out << "'not tocages' makes no sense." << endl;
                return CR_WRONG_USAGE;
            }

            // if followed by another parameter, check if it's numeric
            if(i < parameters.size()-1)
            {
                stringstream ss(parameters[i+1]);
                int new_building = -1;
                ss >> new_building;
                if(new_building != -1)
                {
                    i++;
                    target_building = df::building::find(new_building);
                    out << "Assign selected unit(s) to cagezone #" << new_building <<std::endl;
                }
            }
            if(!target_building)
            {
                out.printerr("No building id specified and current one is invalid!\n");
                return CR_WRONG_USAGE;
            }
            else
            {
                out << "No buiding id specified. Will try to use #" << target_building->id << endl;
                cagezone_assign = true;
            }
        }
        else if(p == "race" && !invert_filter)
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
        else if(p == "race" && invert_filter)
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
                out << "Excluding race: " << target_race << endl;
                find_not_race = true;
            }
            invert_filter = false;
        }
        else if(p == "not")
        {
            invert_filter = true;
        }
        else if(p == "unassigned")
        {
            if(invert_filter)
                return CR_WRONG_USAGE;
            out << "Filter by 'unassigned'." << endl;
            find_unassigned = true;
        }
        else if(p == "caged" && !invert_filter)
        {
            out << "Filter by 'caged' (ignores built cages)." << endl;
            find_caged = true;
        }
        else if(p == "caged" && invert_filter)
        {
            out << "Filter by 'not caged'." << endl;
            find_not_caged = true;
            invert_filter = false;
        }
        else if(p == "trained" && !invert_filter)
        {
            out << "Filter by 'trained'." << endl;
            find_trained = true;
        }
        else if(p == "trained" && invert_filter)
        {
            out << "Filter by 'untrained'." << endl;
            find_not_trained = true;
            invert_filter = false;
        }
        else if(p == "trainablewar" && !invert_filter)
        {
            out << "Filter by 'trainable for war'." << endl;
            find_trainable_war = true;
        }
        else if(p == "trainablewar" && invert_filter)
        {
            out << "Filter by 'not trainable for war'." << endl;
            find_not_trainable_war = true;
            invert_filter = false;
        }
        else if(p == "trainablehunt"&& !invert_filter)
        {
            out << "Filter by 'trainable for hunting'." << endl;
            find_trainable_hunting = true;
        }
        else if(p == "trainablehunt"&& invert_filter)
        {
            out << "Filter by 'not trainable for hunting'." << endl;
            find_not_trainable_hunting = true;
            invert_filter = false;
        }
        else if(p == "war" && !invert_filter)
        {
            out << "Filter by 'trained war creature'." << endl;
            find_war = true;
        }
        else if(p == "war" && invert_filter)
        {
            out << "Filter by 'not a trained war creature'." << endl;
            find_not_war = true;
            invert_filter = false;
        }
        else if(p == "hunting" && !invert_filter)
        {
            out << "Filter by 'trained hunting creature'." << endl;
            find_hunter = true;
        }
        else if(p == "hunting" && invert_filter)
        {
            out << "Filter by 'not a trained hunting creature'." << endl;
            find_not_hunter = true;
            invert_filter = false;
        }else if(p == "own"&& !invert_filter)
        {
            out << "Filter by 'own civilization'." << endl;
            find_own = true;
        }
        else if(p == "own" && invert_filter)
        {
            out << "Filter by 'not own' (i.e. not from the fortress civ, can be a dwarf)." << endl;
            find_not_own = true;
            invert_filter = false;
        }
        else if(p == "tame" && !invert_filter)
        {
            out << "Filter by 'tame'." << endl;
            find_tame = true;
        }
        else if(p == "tame" && invert_filter)
        {
            out << "Filter by 'not tame'." << endl;
            find_not_tame = true;
            invert_filter=false;
        }
        else if(p == "named" && !invert_filter)
        {
            out << "Filter by 'has name or nickname'." << endl;
            find_named = true;
        }
        else if(p == "named" && invert_filter)
        {
            out << "Filter by 'has no name or nickname'." << endl;
            find_not_named = true;
            invert_filter=false;
        }
        else if(p == "slaughter")
        {
            if(invert_filter)
                return CR_WRONG_USAGE;
            out << "Assign animals for slaughter." << endl;
            unit_slaughter = true;
        }
        else if(p == "count")
        {
            if(invert_filter)
                return CR_WRONG_USAGE;
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
            if(invert_filter)
                return CR_WRONG_USAGE;
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
            if(invert_filter)
                return CR_WRONG_USAGE;
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
            if(invert_filter)
                return CR_WRONG_USAGE;
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
        else if(p == "male" && !invert_filter)
        {
            find_male = true;
        }
        else if(p == "female" && !invert_filter)
        {
            find_female = true;
        }
        else if(p == "egglayer" && !invert_filter)
        {
            find_egglayer = true;
        }
        else if(p == "egglayer" && invert_filter)
        {
            find_not_egglayer = true;
            invert_filter=false;
        }
        else if(p == "naked" && !invert_filter)
        {
            find_naked = true;
        }
        else if(p == "naked" && invert_filter)
        {
            find_not_naked = true;
            invert_filter=false;
        }
        else if(p == "tamable" && !invert_filter)
        {
            find_tamable = true;
        }
        else if(p == "tamable" && invert_filter)
        {
            find_not_tamable = true;
            invert_filter=false;
        }
        else if(p == "grazer" && !invert_filter)
        {
            find_grazer = true;
        }
        else if(p == "grazer" && invert_filter)
        {
            find_not_grazer = true;
            invert_filter=false;
        }
        else if(p == "merchant" && !invert_filter)
        {
            find_merchant = true;
        }
        else if(p == "merchant" && invert_filter)
        {
            // actually 'not merchant' is pointless since merchant units are ignored by default
            invert_filter=false;
        }
        else if(p == "milkable" && !invert_filter)
        {
            find_milkable = true;
        }
        else if(p == "milkable" && invert_filter)
        {
            find_not_milkable = true;
            invert_filter=false;
        }
        else if(p == "set")
        {
            if(invert_filter)
                return CR_WRONG_USAGE;
            building_set = true;
        }
        else if(p == "nick")
        {
            if(invert_filter)
                return CR_WRONG_USAGE;

            if(i == parameters.size()-1)
            {
                out.printerr("No nickname specified! Use 'remnick' to remove nicknames!");
                return CR_WRONG_USAGE;
            }
            nick_set = true;
            target_nick = parameters[i+1];
            i++;
            out << "Set nickname to: " << target_nick << endl;
        }
        else if(p == "remnick")
        {
            if(invert_filter)
                return CR_WRONG_USAGE;

            nick_set = true;
            target_nick = "";
            i++;
            out << "Remove nickname." << endl;
        }
        else if(p == "all")
        {
            if(invert_filter)
                return CR_WRONG_USAGE;
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

    if((zone_info && !all) || building_set)
        need_cursor = true;

    if(need_cursor && cursor->x == -30000)
    {
        out.printerr("No cursor; place cursor over activity zone.\n");
        return CR_FAILURE;
    }

    // search for male and not male is exclusive, so drop the flags if both are specified
    if(find_male && find_not_male)
    {
        find_male=false;
        find_not_male=false;
    }

    // search for female and not female is exclusive, so drop the flags if both are specified
    if(find_female && find_not_female)
    {
        find_female=false;
        find_not_female=false;
    }

    // search for male and female is exclusive, so drop the flags if both are specified
    if(find_male && find_female)
    {
        find_male=false;
        find_female=false;
    }

    // search for trained and untrained is exclusive, so drop the flags if both are specified
    if(find_trained && find_not_trained)
    {
        find_trained=false;
        find_not_trained=false;
    }

    // search for grazer and nograzer is exclusive, so drop the flags if both are specified
    if(find_grazer && find_not_grazer)
    {
        find_grazer=false;
        find_not_grazer=false;
    }

    // todo: maybe add this type of sanity check for all remaining bools, maybe not (lots of code just to avoid parsing dumb input)

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
        vector<df::building_civzonest*> zones;
        Buildings::findCivzonesAt(&zones, Gui::getCursorPos());
        for (auto zone = zones.begin(); zone != zones.end(); ++zone)
            zoneInfo(out, *zone, verbose);
        df::building* building = Buildings::findAtTile(Gui::getCursorPos());
        chainInfo(out, building, verbose);
        cageInfo(out, building, verbose);
        return CR_OK;
    }

    // set building at cursor position to be new target building
    if(building_set)
    {
        // cagezone wants a pen/pit as starting point
        if(!cagezone_assign)
            target_building = findCageAtCursor();
        if(target_building)
        {
            out << "Target building type: cage." << endl;
        }
        else
        {
            target_building = findPenPitAt(Gui::getCursorPos());
            if(!target_building)
            {
                out << "No pen/pasture or pit under cursor!" << endl;
                return CR_WRONG_USAGE;
            }
            else
            {
                out << "Target building type: pen/pasture or pit." << endl;
            }
        }
        out << "Current building set to #" << target_building->id << endl;
        return CR_OK;
    }

    if(building_assign || cagezone_assign || unit_info || unit_slaughter || nick_set)
    {
        if(building_assign || cagezone_assign || (nick_set && !all && !find_count))
        {
            if (!target_building)
            {
                out << "Invalid building id." << endl;
                return CR_WRONG_USAGE;
            }
            if(nick_set && !building_assign)
            {
                out << "Renaming all units in target building." << endl;
                return nickUnitsInBuilding(out, target_building, target_nick);
            }
        }

        if(all || find_count)
        {
            vector <df::unit*> units_for_cagezone;
            size_t count = 0;
            for(size_t c = 0; c < world->units.all.size(); c++)
            {
                df::unit *unit = world->units.all[c];

                // ignore dead and undead units
                if (isDead(unit) || isUndead(unit))
                    continue;

                // ignore merchant units by default
                if (!find_merchant && (isMerchant(unit) || isForest(unit)))
                    continue;
                // but allow pitting them and stealing from them if specified :)
                if (find_merchant && !isMerchant(unit) && !isForest(unit))
                    continue;

                if(find_race && getRaceName(unit) != target_race)
                    continue;
                if(find_not_race && getRaceName(unit) == target_race)
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
                    // avoid tampering with creatures who are currently being hauled to a built cage
                    || (isContainedInItem(unit) && (find_not_caged || isInBuiltCage(unit)))
                    || (isChained(unit))
                    || (find_caged && !isContainedInItem(unit))
                    || (find_not_caged && isContainedInItem(unit))
                    || (find_own && !isOwnCiv(unit))
                    || (find_not_own && isOwnCiv(unit))
                    || (find_tame && !isTame(unit))
                    || (find_not_tame && isTame(unit))
                    || (find_trained && !isTrained(unit))
                    || (find_not_trained && isTrained(unit))
                    || (find_war && !isWar(unit))
                    || (find_not_war && isWar(unit))
                    || (find_hunter && !isHunter(unit))
                    || (find_not_hunter && isHunter(unit))
                    || (find_agemin && (int) getAge(unit, true)<target_agemin)
                    || (find_agemax && (int) getAge(unit, true)>target_agemax)
                    || (find_grazer && !isGrazer(unit))
                    || (find_not_grazer && isGrazer(unit))
                    || (find_egglayer && !isEggLayer(unit))
                    || (find_not_egglayer && isEggLayer(unit))
                    || (find_milkable && !isMilkable(unit))
                    || (find_not_milkable && isMilkable(unit))
                    || (find_male && !isMale(unit))
                    || (find_not_male && isMale(unit))
                    || (find_female && !isFemale(unit))
                    || (find_not_female && isFemale(unit))
                    || (find_named && !unit->name.has_name)
                    || (find_not_named && unit->name.has_name)
                    || (find_naked && !isNaked(unit))
                    || (find_not_naked && isNaked(unit))
                    || (find_tamable && !isTamable(unit))
                    || (find_not_tamable && isTamable(unit))
                    || (find_trainable_war && (isWar(unit) || isHunter(unit) || !isTrainableWar(unit)))
                    || (find_not_trainable_war && isTrainableWar(unit)) // hm, is this check enough?
                    || (find_trainable_hunting && (isWar(unit) || isHunter(unit) || !isTrainableHunting(unit)))
                    || (find_not_trainable_hunting && isTrainableHunting(unit)) // hm, is this check enough?
                    )
                    continue;

                // animals bought in cages have an invalid map pos until they are freed for the first time
                // but if they are not in a cage and have an invalid pos it's better not to touch them
                if(!isContainedInItem(unit) && !hasValidMapPos(unit))
                {
                    if(verbose)
                    {
                        out << "----------"<<endl;
                        out << "invalid unit pos but not in cage either. will skip this unit." << endl;
                        unitInfo(out, unit, verbose);
                        out << "----------"<<endl;
                    }
                    continue;
                }

                if(unit_info)
                {
                    unitInfo(out, unit, verbose);
                    continue;
                }

                if(nick_set)
                {
                    Units::setNickname(unit, target_nick);
                }

                if(cagezone_assign)
                {
                    units_for_cagezone.push_back(unit);
                }
                else if(building_assign)
                {
                    command_result result = assignUnitToBuilding(out, unit, target_building, verbose);
                    if(result != CR_OK)
                        return result;
                }

                if(unit_slaughter)
                {
                    // don't slaughter named creatures unless told to do so
                    if(!unit->name.has_name || find_named)
                        doMarkForSlaughter(unit);
                }

                count++;
                if(find_count && count >= target_count)
                    break;
            }
            if(cagezone_assign)
            {
                command_result result = assignUnitsToCagezone(out, units_for_cagezone, target_building, verbose);
                if(result != CR_OK)
                    return result;
            }

            out << "Processed creatures: " << count << endl;
        }
        else
        {
            // must have unit selected
            df::unit *unit = getSelectedUnit(out, true);
            if (!unit)
                return CR_WRONG_USAGE;

            if(unit_info)
            {
                unitInfo(out, unit, verbose);
                return CR_OK;
            }
            else if(building_assign)
            {
                return assignUnitToBuilding(out, unit, target_building, verbose);
            }
            else if(unit_slaughter)
            {
                // by default behave like the game? only allow slaughtering of named war/hunting pets
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
    if(building_unassign)
    {
        // must have unit selected
        df::unit *unit = getSelectedUnit(out, true);
        if (!unit)
            return CR_WRONG_USAGE;

        // remove assignment reference from unit and old zone
        if(unassignUnitFromBuilding(unit))
            out << "Unit unassigned." << endl;
        else
            out << "Unit is not assigned to an activity zone!" << endl;

        return CR_OK;
    }

    return CR_OK;
}

////////////////////
// autonestbox stuff

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
            autonestbox_did_complain = false;
            start_autonestbox(out);
            return autoNestbox(out, verbose);
        }
        if (p == "stop")
        {
            enable_autonestbox = false;
            if(config_autonestbox.isValid())
                config_autonestbox.ival(0) = 0;
            out << "Autonestbox stopped." << endl;
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
                if(config_autonestbox.isValid())
                    config_autonestbox.ival(1) = sleep_autonestbox;
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

////////////////////
// autobutcher stuff

// getUnitAge() returns 0 if born in current year, therefore the look at birth_time in that case
// (assuming that the value from there indicates in which tick of the current year the unit was born)
bool compareUnitAgesYounger(df::unit* i, df::unit* j)
{
    int32_t age_i = (int32_t) getAge(i, true);
    int32_t age_j = (int32_t) getAge(j, true);
    if(age_i == 0 && age_j == 0)
    {
        age_i = i->relations.birth_time;
        age_j = j->relations.birth_time;
    }
    return (age_i < age_j);
}
bool compareUnitAgesOlder(df::unit* i, df::unit* j)
{
    int32_t age_i = (int32_t) getAge(i, true);
    int32_t age_j = (int32_t) getAge(j, true);
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

enum unit_ptr_index
{
 fk_index = 0,
 mk_index = 1,
 fa_index = 2,
 ma_index = 3
};

struct WatchedRace
{
public:
    PersistentDataItem rconfig;

    bool isWatched; // if true, autobutcher will process this race
    int raceId;

    // target amounts
    unsigned fk; // max female kids
    unsigned mk; // max male kids
    unsigned fa; // max female adults
    unsigned ma; // max male adults

    // amounts of protected (not butcherable) units
    unsigned fk_prot;
    unsigned fa_prot;
    unsigned mk_prot;
    unsigned ma_prot;

    // butcherable units
    vector <df::unit*> unit_ptr[4];

    // priority butcherable units
    vector <df::unit*> prot_ptr[4];

    WatchedRace(bool watch, int id, unsigned _fk, unsigned _mk, unsigned _fa, unsigned _ma)
    {
        isWatched = watch;
        raceId = id;
        fk = _fk;
        mk = _mk;
        fa = _fa;
        ma = _ma;
        fk_prot = fa_prot = mk_prot = ma_prot = 0;
    }

    ~WatchedRace()
    {
        ClearUnits();
    }

    void UpdateConfig(color_ostream & out)
    {
        if(!rconfig.isValid())
        {
            string keyname = "autobutcher/watchlist/" + getRaceNameById(raceId);
            rconfig = World::GetPersistentData(keyname, NULL);
        }
        if(rconfig.isValid())
        {
            rconfig.ival(0) = raceId;
            rconfig.ival(1) = isWatched;
            rconfig.ival(2) = fk;
            rconfig.ival(3) = mk;
            rconfig.ival(4) = fa;
            rconfig.ival(5) = ma;
        }
        else
        {
            // this should never happen
            string keyname = "autobutcher/watchlist/" + getRaceNameById(raceId);
            out << "Something failed, could not find/create config key " << keyname << "!" << endl;
        }
    }

    void RemoveConfig(color_ostream & out)
    {
        if(!rconfig.isValid())
            return;
        World::DeletePersistentData(rconfig);
    }

    void SortUnitsByAge()
    {
        sort(unit_ptr[fk_index].begin(), unit_ptr[fk_index].end(), compareUnitAgesOlder);
        sort(unit_ptr[mk_index].begin(), unit_ptr[mk_index].end(), compareUnitAgesOlder);
        sort(unit_ptr[fa_index].begin(), unit_ptr[fa_index].end(), compareUnitAgesYounger);
        sort(unit_ptr[ma_index].begin(), unit_ptr[ma_index].end(), compareUnitAgesYounger);
        sort(prot_ptr[fk_index].begin(), prot_ptr[fk_index].end(), compareUnitAgesOlder);
        sort(prot_ptr[mk_index].begin(), prot_ptr[mk_index].end(), compareUnitAgesOlder);
        sort(prot_ptr[fa_index].begin(), prot_ptr[fa_index].end(), compareUnitAgesYounger);
        sort(prot_ptr[ma_index].begin(), prot_ptr[ma_index].end(), compareUnitAgesYounger);
    }

    void PushUnit(df::unit * unit)
    {
        if(isFemale(unit))
        {
            if(isBaby(unit) || isChild(unit))
                unit_ptr[fk_index].push_back(unit);
            else
                unit_ptr[fa_index].push_back(unit);
        }
        else //treat sex n/a like it was male
        {
            if(isBaby(unit) || isChild(unit))
                unit_ptr[mk_index].push_back(unit);
            else
                unit_ptr[ma_index].push_back(unit);
        }
    }

    void PushPriorityUnit(df::unit * unit)
    {
        if(isFemale(unit))
        {
            if(isBaby(unit) || isChild(unit))
                prot_ptr[fk_index].push_back(unit);
            else
                prot_ptr[fa_index].push_back(unit);
        }
        else
        {
            if(isBaby(unit) || isChild(unit))
                prot_ptr[mk_index].push_back(unit);
            else
                prot_ptr[ma_index].push_back(unit);
        }
    }

    void PushProtectedUnit(df::unit * unit)
    {
        if(isFemale(unit))
        {
            if(isBaby(unit) || isChild(unit))
                fk_prot++;
            else
                fa_prot++;
        }
        else //treat sex n/a like it was male
        {
            if(isBaby(unit) || isChild(unit))
                mk_prot++;
            else
                ma_prot++;
        }
    }

    void ClearUnits()
    {
        fk_prot = fa_prot = mk_prot = ma_prot = 0;
        for (size_t i = 0; i < 4; i++)
        {
            unit_ptr[i].clear();
            prot_ptr[i].clear();
        }
    }

    int ProcessUnits(vector<df::unit*>& unit_ptr, vector<df::unit*>& unit_pri_ptr, unsigned prot, unsigned goal)
    {
        int subcount = 0;
        while(unit_pri_ptr.size() && (unit_ptr.size() + unit_pri_ptr.size() + prot > goal) )
        {
            df::unit* unit = unit_pri_ptr.back();
            doMarkForSlaughter(unit);
            unit_pri_ptr.pop_back();
            subcount++;
        }
        while(unit_ptr.size() && (unit_ptr.size() + prot > goal) )
        {
            df::unit* unit = unit_ptr.back();
            doMarkForSlaughter(unit);
            unit_ptr.pop_back();
            subcount++;
        }
        return subcount;
    }

    int ProcessUnits()
    {
        SortUnitsByAge();
        int slaughter_count = 0;
        slaughter_count += ProcessUnits(unit_ptr[fk_index], prot_ptr[fk_index], fk_prot, fk);
        slaughter_count += ProcessUnits(unit_ptr[mk_index], prot_ptr[mk_index], mk_prot, mk);
        slaughter_count += ProcessUnits(unit_ptr[fa_index], prot_ptr[fa_index], fa_prot, fa);
        slaughter_count += ProcessUnits(unit_ptr[ma_index], prot_ptr[ma_index], ma_prot, ma);
        ClearUnits();
        return slaughter_count;
    }
};
// vector of races handled by autobutcher
// the name is a bit misleading since entries can be set to 'unwatched'
// to ignore them for a while but still keep the target count settings
std::vector<WatchedRace*> watched_races;

// helper for sorting the watchlist alphabetically
bool compareRaceNames(WatchedRace* i, WatchedRace* j)
{
    string name_i = getRaceNamePluralById(i->raceId);
    string name_j = getRaceNamePluralById(j->raceId);

    return (name_i < name_j);
}

static void autobutcher_sortWatchList(color_ostream &out);

// default target values for autobutcher
static unsigned default_fk = 5;
static unsigned default_mk = 1;
static unsigned default_fa = 5;
static unsigned default_ma = 1;

command_result df_autobutcher(color_ostream &out, vector <string> & parameters)
{
    CoreSuspender suspend;

    bool verbose = false;
    bool watch_race = false;
    bool unwatch_race = false;
    bool forget_race = false;
    bool list_watched = false;
    bool list_export = false;
    bool change_target = false;
    vector <string> target_racenames;
    vector <int> target_raceids;

    unsigned target_fk = default_fk;
    unsigned target_mk = default_mk;
    unsigned target_fa = default_fa;
    unsigned target_ma = default_ma;

    if(!parameters.size())
    {
        out << "You must specify a command!" << endl;
        out << autobutcher_help << endl;
        return CR_OK;
    }

    // parse main command
    string & p = parameters[0];
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
        plugin_enable(out, true);
        enable_autobutcher = true;
        start_autobutcher(out);
        return autoButcher(out, verbose);
    }
    else if (p == "stop")
    {
        enable_autobutcher = false;
        if(config_autobutcher.isValid())
            config_autobutcher.ival(0) = enable_autobutcher;
        out << "Autobutcher stopped." << endl;
        return CR_OK;
    }
    else if(p == "sleep")
    {
        parameters.erase(parameters.begin());
        if(!parameters.size())
        {
            out.printerr("No duration specified!\n");
            return CR_WRONG_USAGE;
        }
        else
        {
            size_t ticks = 0;
            stringstream ss(parameters.back());
            ss >> ticks;
            if(ticks <= 0)
            {
                out.printerr("Invalid duration specified (must be > 0)!\n");
                return CR_WRONG_USAGE;
            }
            sleep_autobutcher = ticks;
            if(config_autobutcher.isValid())
                config_autobutcher.ival(1) = sleep_autobutcher;
            out << "New sleep timer for autobutcher: " << ticks << " ticks." << endl;
            return CR_OK;
        }
    }
    else if(p == "watch")
    {
        parameters.erase(parameters.begin());
        watch_race = true;
        out << "Start watching race(s): "; // << endl;
    }
    else if(p == "unwatch")
    {
        parameters.erase(parameters.begin());
        unwatch_race = true;
        out << "Stop watching race(s): "; // << endl;
    }
    else if(p == "forget")
    {
        parameters.erase(parameters.begin());
        forget_race = true;
        out << "Removing race(s) from watchlist: "; // << endl;
    }
    else if(p == "target")
    {
        // needs at least 5 more parameters:
        // fk mk fa ma R (can have more than 1 R)
        if(parameters.size() < 6)
        {
            out.printerr("Not enough parameters!\n");
            return CR_WRONG_USAGE;
        }
        else
        {
            stringstream fk(parameters[1]);
            stringstream mk(parameters[2]);
            stringstream fa(parameters[3]);
            stringstream ma(parameters[4]);
            fk >> target_fk;
            mk >> target_mk;
            fa >> target_fa;
            ma >> target_ma;
            parameters.erase(parameters.begin(), parameters.begin()+5);
            change_target = true;
            out << "Setting new target count for race(s): "; // << endl;
        }
    }
    else if(p == "autowatch")
    {
        out << "Auto-adding to watchlist started." << endl;
        enable_autobutcher_autowatch = true;
        if(config_autobutcher.isValid())
            config_autobutcher.ival(2) = enable_autobutcher_autowatch;
        return CR_OK;
    }
    else if(p == "noautowatch")
    {
        out << "Auto-adding to watchlist stopped." << endl;
        enable_autobutcher_autowatch = false;
        if(config_autobutcher.isValid())
            config_autobutcher.ival(2) = enable_autobutcher_autowatch;
        return CR_OK;
    }
    else if(p == "list")
    {
        list_watched = true;
    }
    else if(p == "list_export")
    {
        list_export = true;
    }
    else
    {
        out << "Unknown command: " << p << endl;
        return CR_WRONG_USAGE;
    }

    if(list_watched)
    {
        out << "Autobutcher status: ";

        if(enable_autobutcher)
            out << "enabled,";
        else
            out << "not enabled,";

        if (enable_autobutcher_autowatch)
            out << " autowatch,";
        else
            out << " noautowatch,";

        out << " sleep: " << sleep_autobutcher << endl;

        out << "Default setting for new races:"
            << " fk=" << default_fk
            << " mk=" << default_mk
            << " fa=" << default_fa
            << " ma=" << default_ma
            << endl;

        if(!watched_races.size())
        {
            out << "The autobutcher race list is empty." << endl;
            return CR_OK;
        }

        out << "Races on autobutcher list: " << endl;
        for(size_t i=0; i<watched_races.size(); i++)
        {
            WatchedRace * w = watched_races[i];
            df::creature_raw * raw = world->raws.creatures.all[w->raceId];
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

    if(list_export)
    {
        // force creation of config
        out << "autobutcher start" << endl;

        if(!enable_autobutcher)
            out << "autobutcher stop" << endl;

        if (enable_autobutcher_autowatch)
            out << "autobutcher autowatch" << endl;

        out << "autobutcher sleep " << sleep_autobutcher << endl;
        out << "autobutcher target"
            << " " << default_fk
            << " " << default_mk
            << " " << default_fa
            << " " << default_ma
            << " new" << endl;

        for(size_t i=0; i<watched_races.size(); i++)
        {
            WatchedRace * w = watched_races[i];
            df::creature_raw * raw = world->raws.creatures.all[w->raceId];
            string name = raw->creature_id;

            out << "autobutcher target"
                << " " << w->fk
                << " " << w->mk
                << " " << w->fa
                << " " << w->ma
                << " " << name << endl;

            if(w->isWatched)
                out << "autobutcher watch " << name << endl;
        }
        return CR_OK;
    }

    // parse rest of parameters for commands followed by a list of races
    if(    watch_race
        || unwatch_race
        || forget_race
        || change_target )
    {
        if(!parameters.size())
        {
            out.printerr("No race(s) specified!\n");
            return CR_WRONG_USAGE;
        }
        while(parameters.size())
        {
            string tr = parameters.back();
            target_racenames.push_back(tr);
            parameters.pop_back();
            out << tr << " ";
        }
        out << endl;
    }

    if(change_target && target_racenames.size() && target_racenames[0] == "all")
    {
        out << "Setting target count for all races on watchlist." << endl;
        for(size_t i=0; i<watched_races.size(); i++)
        {
            WatchedRace * w = watched_races[i];
            w->fk = target_fk;
            w->mk = target_mk;
            w->fa = target_fa;
            w->ma = target_ma;
            w->UpdateConfig(out);
        }
    }

    if(target_racenames.size() && (target_racenames[0] == "all" || target_racenames[0] == "new"))
    {
        if(change_target)
        {
            out << "Setting target count for the future." << endl;
            default_fk = target_fk;
            default_mk = target_mk;
            default_fa = target_fa;
            default_ma = target_ma;
            if(config_autobutcher.isValid())
            {
                config_autobutcher.ival(3) = default_fk;
                config_autobutcher.ival(4) = default_mk;
                config_autobutcher.ival(5) = default_fa;
                config_autobutcher.ival(6) = default_ma;
            }
            return CR_OK;
        }
        else if(target_racenames[0] == "new")
        {
            out << "The only valid usage of 'new' is in combination when setting a target count!" << endl;

            // hm, maybe instead of complaining start/stop autowatch instead? and get rid of the autowatch option?
            if(unwatch_race)
                out << "'unwatch new' makes no sense! Use 'noautowatch' instead." << endl;
            else if(forget_race)
                out << "'forget new' makes no sense, 'forget' is only for existing watchlist entries! Use 'noautowatch' instead." << endl;
            else if(watch_race)
                out << "'watch new' makes no sense! Use 'autowatch' instead." << endl;
            return CR_WRONG_USAGE;
        }
    }

    if(target_racenames.size() && target_racenames[0] == "all")
    {
        // fill with race ids from watchlist
        for(size_t i=0; i<watched_races.size(); i++)
        {
            WatchedRace * w = watched_races[i];
            target_raceids.push_back(w->raceId);
        }
    }
    else
    {
        // map race names from parameter list to ids
        size_t num_races = world->raws.creatures.all.size();
        while(target_racenames.size())
        {
            bool found_race = false;
            for(size_t i=0; i<num_races; i++)
            {
                if(getRaceNameById(i) == target_racenames.back())
                {
                    target_raceids.push_back(i);
                    target_racenames.pop_back();
                    found_race = true;
                    break;
                }
            }
            if(!found_race)
            {
                out << "Race not found: " << target_racenames.back() << endl;
                return CR_OK;
            }
        }
    }

    while(target_raceids.size())
    {
        bool entry_found = false;
        for(size_t i=0; i<watched_races.size(); i++)
        {
            WatchedRace * w = watched_races[i];
            if(w->raceId == target_raceids.back())
            {
                if(unwatch_race)
                {
                    w->isWatched=false;
                    w->UpdateConfig(out);
                }
                else if(forget_race)
                {
                    w->RemoveConfig(out);
                    watched_races.erase(watched_races.begin()+i);
                }
                else if(watch_race)
                {
                    w->isWatched = true;
                    w->UpdateConfig(out);
                }
                else if(change_target)
                {
                    w->fk = target_fk;
                    w->mk = target_mk;
                    w->fa = target_fa;
                    w->ma = target_ma;
                    w->UpdateConfig(out);
                }
                entry_found = true;
                break;
            }
        }
        if(!entry_found && (watch_race||change_target))
        {
            WatchedRace * w = new WatchedRace(watch_race, target_raceids.back(), target_fk, target_mk, target_fa, target_ma);
            w->UpdateConfig(out);
            watched_races.push_back(w);
            autobutcher_sortWatchList(out);
        }
        target_raceids.pop_back();
    }

    return CR_OK;
}

// check watched_races vector for a race id, return -1 if nothing found
// calling method needs to check itself if the race is currently being watched or ignored
int getWatchedIndex(int id)
{
    for(size_t i=0; i<watched_races.size(); i++)
    {
        WatchedRace * w = watched_races[i];
        if(w->raceId == id) // && w->isWatched)
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
    if(!enable_autobutcher_autowatch)
    {
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
    }

    for(size_t i=0; i<world->units.all.size(); i++)
    {
        df::unit * unit = world->units.all[i];

        // this check is now divided into two steps, squeezed autowatch into the middle
        // first one ignores completely inappropriate units (dead, undead, not belonging to the fort, ...)
        // then let autowatch add units to the watchlist which will probably start breeding (owned pets, war animals, ...)
        // then process units counting those which can't be butchered (war animals, named pets, ...)
        // so that they are treated as "own stock" as well and count towards the target quota
        if(    isDead(unit)
            || isUndead(unit)
            || isMarkedForSlaughter(unit)
            || isMerchant(unit) // ignore merchants' draught animals
            || isForest(unit) // ignore merchants' caged animals
            || !isOwnCiv(unit)
            || !isTame(unit)
            )
            continue;

        // found a bugged unit which had invalid coordinates but was not in a cage.
        // marking it for slaughter didn't seem to have negative effects, but you never know...
        if(!isContainedInItem(unit) && !hasValidMapPos(unit))
            continue;

        WatchedRace * w = NULL;
        int watched_index = getWatchedIndex(unit->race);
        if(watched_index != -1)
        {
            w = watched_races[watched_index];
        }
        else if(enable_autobutcher_autowatch)
        {
            w = new WatchedRace(true, unit->race, default_fk, default_mk, default_fa, default_ma);
            w->UpdateConfig(out);
            watched_races.push_back(w);

            string announce;
            announce = "New race added to autobutcher watchlist: " + getRaceNamePluralById(w->raceId);
            Gui::showAnnouncement(announce, 2, false);
            autobutcher_sortWatchList(out);
        }

        if(w && w->isWatched)
        {
            // don't butcher protected units, but count them as stock as well
            // this way they count towards target quota, so if you order that you want 1 female adult cat
            // and have 2 cats, one of them being a pet, the other gets butchered
            if(    isWar(unit)    // ignore war dogs etc
                || isHunter(unit) // ignore hunting dogs etc
                // ignore creatures in built cages which are defined as rooms to leave zoos alone
                // (TODO: better solution would be to allow some kind of slaughter cages which you can place near the butcher)
                || (isContainedInItem(unit) && isInBuiltCageRoom(unit))  // !!! see comments in isBuiltCageRoom()
                || isAvailableForAdoption(unit)
                || unit->name.has_name )
                w->PushProtectedUnit(unit);
            else if (   isGay(unit)
                     || isGelded(unit))
                w->PushPriorityUnit(unit);
            else
                w->PushUnit(unit);
        }
    }

    int slaughter_count = 0;
    for(size_t i=0; i<watched_races.size(); i++)
    {
        WatchedRace * w = watched_races[i];
        int slaughter_subcount = w->ProcessUnits();
        slaughter_count += slaughter_subcount;
        if(slaughter_subcount)
        {
            stringstream ss;
            ss << slaughter_subcount;
            string announce;
            announce = getRaceNamePluralById(w->raceId) + " marked for slaughter: " + ss.str();
            Gui::showAnnouncement(announce, 2, false);
        }
    }

    return CR_OK;
}

////////////////////////////////////////////////////
// autobutcher and autonestbox start/init/cleanup

command_result start_autobutcher(color_ostream &out)
{
    plugin_enable(out, true);
    enable_autobutcher = true;

    if (!config_autobutcher.isValid())
    {
        config_autobutcher = World::AddPersistentData("autobutcher/config");

        if (!config_autobutcher.isValid())
        {
            out << "Cannot enable autobutcher without a world!" << endl;
            return CR_OK;
        }

        config_autobutcher.ival(1) = sleep_autobutcher;
        config_autobutcher.ival(2) = enable_autobutcher_autowatch;
        config_autobutcher.ival(3) = default_fk;
        config_autobutcher.ival(4) = default_mk;
        config_autobutcher.ival(5) = default_fa;
        config_autobutcher.ival(6) = default_ma;
    }

    config_autobutcher.ival(0) = enable_autobutcher;

    out << "Starting autobutcher." << endl;
    init_autobutcher(out);
    return CR_OK;
}

command_result init_autobutcher(color_ostream &out)
{
    cleanup_autobutcher(out);

    config_autobutcher = World::GetPersistentData("autobutcher/config");
    if(config_autobutcher.isValid())
    {
        if (config_autobutcher.ival(0) == -1)
        {
            config_autobutcher.ival(0) = enable_autobutcher;
            config_autobutcher.ival(1) = sleep_autobutcher;
            config_autobutcher.ival(2) = enable_autobutcher_autowatch;
            config_autobutcher.ival(3) = default_fk;
            config_autobutcher.ival(4) = default_mk;
            config_autobutcher.ival(5) = default_fa;
            config_autobutcher.ival(6) = default_ma;
            out << "Autobutcher's persistent config object was invalid!" << endl;
        }
        else
        {
            enable_autobutcher = config_autobutcher.ival(0);
            sleep_autobutcher = config_autobutcher.ival(1);
            enable_autobutcher_autowatch = config_autobutcher.ival(2);
            default_fk = config_autobutcher.ival(3);
            default_mk = config_autobutcher.ival(4);
            default_fa = config_autobutcher.ival(5);
            default_ma = config_autobutcher.ival(6);
        }
    }

    if(!enable_autobutcher)
        return CR_OK;

    plugin_enable(out, true);
    // read watchlist from save

    std::vector<PersistentDataItem> items;
    World::GetPersistentData(&items, "autobutcher/watchlist/", true);
    for (auto p = items.begin(); p != items.end(); p++)
    {
        string key = p->key();
        out << "Reading from save: " << key << endl;
        //out << "  raceid: "   << p->ival(0) << endl;
        //out << "  watched: "  << p->ival(1) << endl;
        //out << "  fk: "       << p->ival(2) << endl;
        //out << "  mk: "       << p->ival(3) << endl;
        //out << "  fa: "       << p->ival(4) << endl;
        //out << "  ma: "       << p->ival(5) << endl;
        WatchedRace * w = new WatchedRace(p->ival(1), p->ival(0), p->ival(2), p->ival(3),p->ival(4),p->ival(5));
        w->rconfig = *p;
        watched_races.push_back(w);
    }
    autobutcher_sortWatchList(out);
    return CR_OK;
}

command_result cleanup_autobutcher(color_ostream &out)
{
    for(size_t i=0; i<watched_races.size(); i++)
    {
        delete watched_races[i];
    }
    watched_races.clear();
    return CR_OK;
}

command_result start_autonestbox(color_ostream &out)
{
    plugin_enable(out, true);
    enable_autonestbox = true;

    if (!config_autonestbox.isValid())
    {
        config_autonestbox = World::AddPersistentData("autonestbox/config");

        if (!config_autonestbox.isValid())
        {
            out << "Cannot enable autonestbox without a world!" << endl;
            return CR_OK;
        }

        config_autonestbox.ival(1) = sleep_autonestbox;
    }

    config_autonestbox.ival(0) = enable_autonestbox;

    out << "Starting autonestbox." << endl;
    init_autonestbox(out);
    return CR_OK;
}

command_result init_autonestbox(color_ostream &out)
{
    cleanup_autonestbox(out);

    config_autonestbox = World::GetPersistentData("autonestbox/config");
    if(config_autonestbox.isValid())
    {
        if (config_autonestbox.ival(0) == -1)
        {
            config_autonestbox.ival(0) = enable_autonestbox;
            config_autonestbox.ival(1) = sleep_autonestbox;
            out << "Autonestbox's persistent config object was invalid!" << endl;
        }
        else
        {
            enable_autonestbox = config_autonestbox.ival(0);
            sleep_autonestbox = config_autonestbox.ival(1);
        }
    }
    if (enable_autonestbox)
        plugin_enable(out, true);
    return CR_OK;
}

command_result cleanup_autonestbox(color_ostream &out)
{
    // nothing to cleanup currently
    // (future version of autonestbox could store info about cages for useless male kids)
    return CR_OK;
}

// abuse WatchedRace struct for counting stocks (since it sorts by gender and age)
// calling method must delete pointer!
WatchedRace * checkRaceStocksTotal(int race)
{
    WatchedRace * w = new WatchedRace(true, race, default_fk, default_mk, default_fa, default_ma);

    for(size_t i=0; i<world->units.all.size(); i++)
    {
        df::unit * unit = world->units.all[i];

        if(unit->race != race)
            continue;

        if(    isDead(unit)
            || isUndead(unit)
            || isMerchant(unit) // ignore merchants' draught animals
            || isForest(unit) // ignore merchants' caged animals
            || !isOwnCiv(unit)
            )
            continue;

        // found a bugged unit which had invalid coordinates but was not in a cage.
        // marking it for slaughter didn't seem to have negative effects, but you never know...
        if(!isContainedInItem(unit) && !hasValidMapPos(unit))
            continue;

        w->PushUnit(unit);
    }
    return w;
}

WatchedRace * checkRaceStocksProtected(int race)
{
    WatchedRace * w = new WatchedRace(true, race, default_fk, default_mk, default_fa, default_ma);

    for(size_t i=0; i<world->units.all.size(); i++)
    {
        df::unit * unit = world->units.all[i];

        if(unit->race != race)
            continue;

        if(    isDead(unit)
            || isUndead(unit)
            || isMerchant(unit) // ignore merchants' draught animals
            || isForest(unit) // ignore merchants' caged animals
            || !isOwnCiv(unit)
            )
            continue;

        // found a bugged unit which had invalid coordinates but was not in a cage.
        // marking it for slaughter didn't seem to have negative effects, but you never know...
        if(!isContainedInItem(unit) && !hasValidMapPos(unit))
            continue;

        if(   !isTame(unit)
           || isWar(unit) // ignore war dogs etc
           || isHunter(unit) // ignore hunting dogs etc
            // ignore creatures in built cages which are defined as rooms to leave zoos alone
            // (TODO: better solution would be to allow some kind of slaughter cages which you can place near the butcher)
            || (isContainedInItem(unit) && isInBuiltCageRoom(unit))  // !!! see comments in isBuiltCageRoom()
            || isAvailableForAdoption(unit)
            || unit->name.has_name )
            w->PushUnit(unit);
    }
    return w;
}

WatchedRace * checkRaceStocksButcherable(int race)
{
    WatchedRace * w = new WatchedRace(true, race, default_fk, default_mk, default_fa, default_ma);

    for(size_t i=0; i<world->units.all.size(); i++)
    {
        df::unit * unit = world->units.all[i];

        if(unit->race != race)
            continue;

        if(    isDead(unit)
            || isUndead(unit)
            || isMerchant(unit) // ignore merchants' draught animals
            || isForest(unit) // ignore merchants' caged animals
            || !isOwnCiv(unit)
            || !isTame(unit)
            || isWar(unit) // ignore war dogs etc
            || isHunter(unit) // ignore hunting dogs etc
            // ignore creatures in built cages which are defined as rooms to leave zoos alone
            // (TODO: better solution would be to allow some kind of slaughter cages which you can place near the butcher)
            || (isContainedInItem(unit) && isInBuiltCageRoom(unit))  // !!! see comments in isBuiltCageRoom()
            || isAvailableForAdoption(unit)
            || unit->name.has_name
            )
            continue;

        // found a bugged unit which had invalid coordinates but was not in a cage.
        // marking it for slaughter didn't seem to have negative effects, but you never know...
        if(!isContainedInItem(unit) && !hasValidMapPos(unit))
            continue;

        w->PushUnit(unit);
    }
    return w;
}

WatchedRace * checkRaceStocksButcherFlag(int race)
{
    WatchedRace * w = new WatchedRace(true, race, default_fk, default_mk, default_fa, default_ma);

    for(size_t i=0; i<world->units.all.size(); i++)
    {
        df::unit * unit = world->units.all[i];

        if(unit->race != race)
            continue;

        if(    isDead(unit)
            || isUndead(unit)
            || isMerchant(unit) // ignore merchants' draught animals
            || isForest(unit) // ignore merchants' caged animals
            || !isOwnCiv(unit)
            )
            continue;

        // found a bugged unit which had invalid coordinates but was not in a cage.
        // marking it for slaughter didn't seem to have negative effects, but you never know...
        if(!isContainedInItem(unit) && !hasValidMapPos(unit))
            continue;

        if(isMarkedForSlaughter(unit))
            w->PushUnit(unit);
    }
    return w;
}

void butcherRace(int race)
{
    for(size_t i=0; i<world->units.all.size(); i++)
    {
        df::unit * unit = world->units.all[i];

        if(unit->race != race)
            continue;

        if(    isDead(unit)
            || isUndead(unit)
            || isMerchant(unit) // ignore merchants' draught animals
            || isForest(unit) // ignore merchants' caged animals
            || !isOwnCiv(unit)
            || !isTame(unit)
            || isWar(unit) // ignore war dogs etc
            || isHunter(unit) // ignore hunting dogs etc
            // ignore creatures in built cages which are defined as rooms to leave zoos alone
            // (TODO: better solution would be to allow some kind of slaughter cages which you can place near the butcher)
            || (isContainedInItem(unit) && isInBuiltCageRoom(unit))  // !!! see comments in isBuiltCageRoom()
            || isAvailableForAdoption(unit)
            || unit->name.has_name
            )
            continue;

        // found a bugged unit which had invalid coordinates but was not in a cage.
        // marking it for slaughter didn't seem to have negative effects, but you never know...
        if(!isContainedInItem(unit) && !hasValidMapPos(unit))
            continue;

        doMarkForSlaughter(unit);
    }
}

// remove butcher flag for all units of a given race
void unbutcherRace(int race)
{
    for(size_t i=0; i<world->units.all.size(); i++)
    {
        df::unit * unit = world->units.all[i];

        if(unit->race != race)
            continue;

        if(    isDead(unit)
            || isUndead(unit)
            || !isMarkedForSlaughter(unit)
            )
            continue;

        if(!isContainedInItem(unit) && !hasValidMapPos(unit))
            continue;

        unit->flags2.bits.slaughter = 0;
    }
}


/////////////////////////////////////
// API functions to control autobutcher with a lua script

static bool autobutcher_isEnabled() { return enable_autobutcher; }
static bool autowatch_isEnabled() { return enable_autobutcher_autowatch; }

static unsigned autobutcher_getSleep(color_ostream &out)
{
    return sleep_autobutcher;
}

static void autobutcher_setSleep(color_ostream &out, unsigned ticks)
{
    sleep_autobutcher = ticks;
    if(config_autobutcher.isValid())
        config_autobutcher.ival(1) = sleep_autobutcher;
}

static void autobutcher_setEnabled(color_ostream &out, bool enable)
{
    if(enable)
    {
        enable_autobutcher = true;
        start_autobutcher(out);
        autoButcher(out, false);
    }
    else
    {
        enable_autobutcher = false;
        if(config_autobutcher.isValid())
            config_autobutcher.ival(0) = enable_autobutcher;
        out << "Autobutcher stopped." << endl;
    }

    if (enable)
        plugin_enable(out, true);
}

static void autowatch_setEnabled(color_ostream &out, bool enable)
{
    if(enable)
    {
        out << "Auto-adding to watchlist started." << endl;
        enable_autobutcher_autowatch = true;
        if(config_autobutcher.isValid())
            config_autobutcher.ival(2) = enable_autobutcher_autowatch;
    }
    else
    {
        out << "Auto-adding to watchlist stopped." << endl;
        enable_autobutcher_autowatch = false;
        if(config_autobutcher.isValid())
            config_autobutcher.ival(2) = enable_autobutcher_autowatch;
    }
}

// set all data for a watchlist race in one go
// if race is not already on watchlist it will be added
// params: (id, fk, mk, fa, ma, watched)
static void autobutcher_setWatchListRace(color_ostream &out, unsigned id, unsigned fk, unsigned mk, unsigned fa, unsigned ma, bool watched)
{
    int watched_index = getWatchedIndex(id);
    if(watched_index != -1)
    {
        out << "updating watchlist entry" << endl;
        WatchedRace * w = watched_races[watched_index];
        w->fk = fk;
        w->mk = mk;
        w->fa = fa;
        w->ma = ma;
        w->isWatched = watched;
        w->UpdateConfig(out);
    }
    else
    {
        out << "creating new watchlist entry" << endl;
        WatchedRace * w = new WatchedRace(watched, id, fk, mk, fa, ma); //default_fk, default_mk, default_fa, default_ma);
        w->UpdateConfig(out);
        watched_races.push_back(w);

        string announce;
        announce = "New race added to autobutcher watchlist: " + getRaceNamePluralById(w->raceId);
        Gui::showAnnouncement(announce, 2, false);
        autobutcher_sortWatchList(out);
    }
}

// remove entry from watchlist
static void autobutcher_removeFromWatchList(color_ostream &out, unsigned id)
{
    int watched_index = getWatchedIndex(id);
    if(watched_index != -1)
    {
        out << "updating watchlist entry" << endl;
        WatchedRace * w = watched_races[watched_index];
        w->RemoveConfig(out);
        watched_races.erase(watched_races.begin() + watched_index);
    }
}

// sort watchlist alphabetically
static void autobutcher_sortWatchList(color_ostream &out)
{
    sort(watched_races.begin(), watched_races.end(), compareRaceNames);
}

// set default target values for new races
static void autobutcher_setDefaultTargetNew(color_ostream &out, unsigned fk, unsigned mk, unsigned fa, unsigned ma)
{
    default_fk = fk;
    default_mk = mk;
    default_fa = fa;
    default_ma = ma;
    if(config_autobutcher.isValid())
    {
        config_autobutcher.ival(3) = default_fk;
        config_autobutcher.ival(4) = default_mk;
        config_autobutcher.ival(5) = default_fa;
        config_autobutcher.ival(6) = default_ma;
    }
}

// set default target values for ALL races (update watchlist and set new default)
static void autobutcher_setDefaultTargetAll(color_ostream &out, unsigned fk, unsigned mk, unsigned fa, unsigned ma)
{
    for(unsigned i=0; i<watched_races.size(); i++)
    {
        WatchedRace * w = watched_races[i];
        w->fk = fk;
        w->mk = mk;
        w->fa = fa;
        w->ma = ma;
        w->UpdateConfig(out);
    }
    autobutcher_setDefaultTargetNew(out, fk, mk, fa, ma);
}

static void autobutcher_butcherRace(color_ostream &out, unsigned id)
{
    butcherRace(id);
}

static void autobutcher_unbutcherRace(color_ostream &out, unsigned id)
{
    unbutcherRace(id);
}

// push autobutcher settings on lua stack
static int autobutcher_getSettings(lua_State *L)
{
    color_ostream &out = *Lua::GetOutput(L);
    lua_newtable(L);
    int ctable = lua_gettop(L);
    Lua::SetField(L, enable_autobutcher, ctable, "enable_autobutcher");
    Lua::SetField(L, enable_autobutcher_autowatch, ctable, "enable_autowatch");
    Lua::SetField(L, default_fk, ctable, "fk");
    Lua::SetField(L, default_mk, ctable, "mk");
    Lua::SetField(L, default_fa, ctable, "fa");
    Lua::SetField(L, default_ma, ctable, "ma");
    Lua::SetField(L, sleep_autobutcher, ctable, "sleep");
    return 1;
}

// push the watchlist vector as nested table on the lua stack
static int autobutcher_getWatchList(lua_State *L)
{
    color_ostream &out = *Lua::GetOutput(L);
    lua_newtable(L);

    for(size_t i=0; i<watched_races.size(); i++)
    {
        lua_newtable(L);
        int ctable = lua_gettop(L);
        WatchedRace * w = watched_races[i];
        Lua::SetField(L, w->raceId, ctable, "id");
        Lua::SetField(L, w->isWatched, ctable, "watched");
        Lua::SetField(L, getRaceNamePluralById(w->raceId), ctable, "name");
        Lua::SetField(L, w->fk, ctable, "fk");
        Lua::SetField(L, w->mk, ctable, "mk");
        Lua::SetField(L, w->fa, ctable, "fa");
        Lua::SetField(L, w->ma, ctable, "ma");

        int id = w->raceId;

        w = checkRaceStocksTotal(id);
        Lua::SetField(L, w->unit_ptr[fk_index].size(), ctable, "fk_total");
        Lua::SetField(L, w->unit_ptr[mk_index].size(), ctable, "mk_total");
        Lua::SetField(L, w->unit_ptr[fa_index].size(), ctable, "fa_total");
        Lua::SetField(L, w->unit_ptr[ma_index].size(), ctable, "ma_total");
        delete w;

        w = checkRaceStocksProtected(id);
        Lua::SetField(L, w->unit_ptr[fk_index].size(), ctable, "fk_protected");
        Lua::SetField(L, w->unit_ptr[mk_index].size(), ctable, "mk_protected");
        Lua::SetField(L, w->unit_ptr[fa_index].size(), ctable, "fa_protected");
        Lua::SetField(L, w->unit_ptr[ma_index].size(), ctable, "ma_protected");
        delete w;

        w = checkRaceStocksButcherable(id);
        Lua::SetField(L, w->unit_ptr[fk_index].size(), ctable, "fk_butcherable");
        Lua::SetField(L, w->unit_ptr[mk_index].size(), ctable, "mk_butcherable");
        Lua::SetField(L, w->unit_ptr[fa_index].size(), ctable, "fa_butcherable");
        Lua::SetField(L, w->unit_ptr[ma_index].size(), ctable, "ma_butcherable");
        delete w;

        w = checkRaceStocksButcherFlag(id);
        Lua::SetField(L, w->unit_ptr[fk_index].size(), ctable, "fk_butcherflag");
        Lua::SetField(L, w->unit_ptr[mk_index].size(), ctable, "mk_butcherflag");
        Lua::SetField(L, w->unit_ptr[fa_index].size(), ctable, "fa_butcherflag");
        Lua::SetField(L, w->unit_ptr[ma_index].size(), ctable, "ma_butcherflag");
        delete w;

        lua_rawseti(L, -2, i+1);
    }

    return 1;
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(autobutcher_isEnabled),
    DFHACK_LUA_FUNCTION(autowatch_isEnabled),
    DFHACK_LUA_FUNCTION(autobutcher_setEnabled),
    DFHACK_LUA_FUNCTION(autowatch_setEnabled),
    DFHACK_LUA_FUNCTION(autobutcher_getSleep),
    DFHACK_LUA_FUNCTION(autobutcher_setSleep),
    DFHACK_LUA_FUNCTION(autobutcher_setWatchListRace),
    DFHACK_LUA_FUNCTION(autobutcher_setDefaultTargetNew),
    DFHACK_LUA_FUNCTION(autobutcher_setDefaultTargetAll),
    DFHACK_LUA_FUNCTION(autobutcher_butcherRace),
    DFHACK_LUA_FUNCTION(autobutcher_unbutcherRace),
    DFHACK_LUA_FUNCTION(autobutcher_removeFromWatchList),
    DFHACK_LUA_FUNCTION(autobutcher_sortWatchList),
    DFHACK_LUA_END
};

DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(autobutcher_getSettings),
    DFHACK_LUA_COMMAND(autobutcher_getWatchList),
    DFHACK_LUA_END
};

// end lua API



//START zone filters

class zone_filter
{
public:
    zone_filter()
    {
        initialized = false;
    }

    void initialize(const df::ui_sidebar_mode &mode)
    {
        if (!initialized)
        {
            this->mode = mode;
            saved_ui_building_assign_type.clear();
            saved_ui_building_assign_units.clear();
            saved_ui_building_assign_items.clear();
            saved_ui_building_assign_is_marked.clear();
            saved_indexes.clear();

            for (size_t i = 0; i < ui_building_assign_units->size(); i++)
            {
                saved_ui_building_assign_type.push_back(ui_building_assign_type->at(i));
                saved_ui_building_assign_units.push_back(ui_building_assign_units->at(i));
                saved_ui_building_assign_items.push_back(ui_building_assign_items->at(i));
                saved_ui_building_assign_is_marked.push_back(ui_building_assign_is_marked->at(i));
            }

            search_string.clear();
            show_non_grazers = show_pastured = show_noncaged = show_male = show_female = show_other_zones = true;
            entry_mode = false;

            initialized = true;
        }
    }

    void deinitialize()
    {
        initialized = false;
    }

    void apply_filters()
    {
        if (saved_indexes.size() > 0)
        {
            bool list_has_been_sorted = (ui_building_assign_units->size() == reference_list.size()
                && *ui_building_assign_units != reference_list);

            for (size_t i = 0; i < saved_indexes.size(); i++)
            {
                int adjusted_item_index = i;
                if (list_has_been_sorted)
                {
                    for (size_t j = 0; j < ui_building_assign_units->size(); j++)
                    {
                        if (ui_building_assign_units->at(j) == reference_list[i])
                        {
                            adjusted_item_index = j;
                            break;
                        }
                    }
                }

                saved_ui_building_assign_is_marked[saved_indexes[i]] = ui_building_assign_is_marked->at(adjusted_item_index);
            }
        }

        string search_string_l = toLower(search_string);
        saved_indexes.clear();
        ui_building_assign_type->clear();
        ui_building_assign_is_marked->clear();
        ui_building_assign_units->clear();
        ui_building_assign_items->clear();

        for (size_t i = 0; i < saved_ui_building_assign_units.size(); i++)
        {
            df::unit *curr_unit = saved_ui_building_assign_units[i];

            if (!curr_unit)
                continue;

            if (!show_non_grazers && !isGrazer(curr_unit))
                continue;

            if (!show_pastured && isAssignedToZone(curr_unit))
                continue;

            if (!show_noncaged)
            {
                // must be in a container
                if(!isContainedInItem(curr_unit))
                    continue;
                // but exclude built cages (zoos, traps, ...) to avoid "accidental" pitting of creatures you'd prefer to keep
                if (isInBuiltCage(curr_unit))
                    continue;
            }

            if (!show_male && isMale(curr_unit))
                continue;

            if (!show_female && isFemale(curr_unit))
                continue;

            if (!search_string_l.empty())
            {
                string desc = Translation::TranslateName(
                    Units::getVisibleName(curr_unit), false);

                desc += Units::getProfessionName(curr_unit);
                desc = toLower(desc);

                if (desc.find(search_string_l) == string::npos)
                    continue;
            }

            ui_building_assign_type->push_back(saved_ui_building_assign_type[i]);
            ui_building_assign_units->push_back(curr_unit);
            ui_building_assign_items->push_back(saved_ui_building_assign_items[i]);
            ui_building_assign_is_marked->push_back(saved_ui_building_assign_is_marked[i]);

            saved_indexes.push_back(i); // Used to map filtered indexes back to original, if needed
        }

        reference_list = *ui_building_assign_units;
        *ui_building_item_cursor = 0;
    }

    bool handle_input(const set<df::interface_key> *input)
    {
        if (!initialized)
            return false;

        bool key_processed = true;

        if (entry_mode)
        {
            // Query typing mode

            if  (input->count(interface_key::SECONDSCROLL_UP) || input->count(interface_key::SECONDSCROLL_DOWN) ||
                input->count(interface_key::SECONDSCROLL_PAGEUP) || input->count(interface_key::SECONDSCROLL_PAGEDOWN))
            {
                // Arrow key pressed. Leave entry mode and allow screen to process key
                entry_mode = false;
                return false;
            }

            df::interface_key last_token = get_string_key(input);
            int charcode = Screen::keyToChar(last_token);
            if (charcode >= 32 && charcode <= 126)
            {
                // Standard character
                search_string += char(charcode);
                apply_filters();
            }
            else if (last_token == interface_key::STRING_A000)
            {
                // Backspace
                if (search_string.length() > 0)
                {
                    search_string.erase(search_string.length()-1);
                    apply_filters();
                }
            }
            else if (input->count(interface_key::SELECT) || input->count(interface_key::LEAVESCREEN))
            {
                // ENTER or ESC: leave typing mode
                entry_mode = false;
            }
        }
        // Not in query typing mode
        else if (input->count(interface_key::CUSTOM_SHIFT_G) &&
            (mode == ui_sidebar_mode::ZonesPenInfo || mode == ui_sidebar_mode::QueryBuilding))
        {
            show_non_grazers = !show_non_grazers;
            apply_filters();
        }
        else if (input->count(interface_key::CUSTOM_SHIFT_C) &&
            (mode == ui_sidebar_mode::ZonesPenInfo || mode == ui_sidebar_mode::ZonesPitInfo || mode == ui_sidebar_mode::QueryBuilding))
        {
            show_noncaged = !show_noncaged;
            apply_filters();
        }
        else if (input->count(interface_key::CUSTOM_SHIFT_P) &&
            (mode == ui_sidebar_mode::ZonesPenInfo || mode == ui_sidebar_mode::ZonesPitInfo || mode == ui_sidebar_mode::QueryBuilding))
        {
            show_pastured = !show_pastured;
            apply_filters();
        }
        else if (input->count(interface_key::CUSTOM_SHIFT_M) &&
            (mode == ui_sidebar_mode::ZonesPenInfo || mode == ui_sidebar_mode::ZonesPitInfo || mode == ui_sidebar_mode::QueryBuilding))
        {
            show_male = !show_male;
            apply_filters();
        }
        else if (input->count(interface_key::CUSTOM_SHIFT_F) &&
            (mode == ui_sidebar_mode::ZonesPenInfo || mode == ui_sidebar_mode::ZonesPitInfo || mode == ui_sidebar_mode::QueryBuilding))
        {
            show_female = !show_female;
            apply_filters();
        }
        else if (input->count(interface_key::CUSTOM_S))
        {
            // Hotkey pressed, enter typing mode
            entry_mode = true;
        }
        else if (input->count(interface_key::CUSTOM_SHIFT_S))
        {
            // Shift + Hotkey pressed, clear query
            search_string.clear();
            apply_filters();
        }
        else
        {
            // Not a key for us, pass it on to the screen
            key_processed = false;
        }

        return key_processed || entry_mode; // Only pass unrecognized keys down if not in typing mode
    }

    void do_render()
    {
        if (!initialized)
            return;

        int left_margin = gps->dimx - 30;
        int8_t a = *ui_menu_width;
        int8_t b = *ui_area_map_width;
        if ((a == 1 && b > 1) || (a == 2 && b == 2))
            left_margin -= 24;

        int x = left_margin;
        int y = 24;

        OutputString(COLOR_BROWN, x, y, "DFHack Filtering");
        x = left_margin;
        ++y;
        OutputString(COLOR_LIGHTGREEN, x, y, "s");
        OutputString(COLOR_WHITE, x, y, ": Search");
        if (!search_string.empty() || entry_mode)
        {
            OutputString(COLOR_WHITE, x, y, ": ");
            if (!search_string.empty())
                OutputString(COLOR_WHITE, x, y, search_string);
            if (entry_mode)
                OutputString(COLOR_LIGHTGREEN, x, y, "_");
        }

        if (mode == ui_sidebar_mode::ZonesPenInfo || mode == ui_sidebar_mode::QueryBuilding)
        {
            x = left_margin;
            y += 2;
            OutputString(COLOR_LIGHTGREEN, x, y, "G");
            OutputString(COLOR_WHITE, x, y, ": ");
            OutputString((show_non_grazers) ? COLOR_WHITE : COLOR_GREY, x, y, "Non-Grazing");

            x = left_margin;
            ++y;
            OutputString(COLOR_LIGHTGREEN, x, y, "C");
            OutputString(COLOR_WHITE, x, y, ": ");
            OutputString((show_noncaged) ? COLOR_WHITE : COLOR_GREY, x, y, "Not Caged");

            x = left_margin;
            ++y;
            OutputString(COLOR_LIGHTGREEN, x, y, "P");
            OutputString(COLOR_WHITE, x, y, ": ");
            OutputString((show_pastured) ? COLOR_WHITE : COLOR_GREY, x, y, "Currently Pastured");

            x = left_margin;
            ++y;
            OutputString(COLOR_LIGHTGREEN, x, y, "F");
            OutputString(COLOR_WHITE, x, y, ": ");
            OutputString((show_female) ? COLOR_WHITE : COLOR_GREY, x, y, "Female");

            x = left_margin;
            ++y;
            OutputString(COLOR_LIGHTGREEN, x, y, "M");
            OutputString(COLOR_WHITE, x, y, ": ");
            OutputString((show_male) ? COLOR_WHITE : COLOR_GREY, x, y, "Male");
        }

        // pits don't have grazer filter because it seems pointless
        if (mode == ui_sidebar_mode::ZonesPitInfo)
        {
            x = left_margin;
            y += 2;
            OutputString(COLOR_LIGHTGREEN, x, y, "C");
            OutputString(COLOR_WHITE, x, y, ": ");
            OutputString((show_noncaged) ? COLOR_WHITE : COLOR_GREY, x, y, "Not Caged");

            x = left_margin;
            ++y;
            OutputString(COLOR_LIGHTGREEN, x, y, "P");
            OutputString(COLOR_WHITE, x, y, ": ");
            OutputString((show_pastured) ? COLOR_WHITE : COLOR_GREY, x, y, "Currently Pastured");

            x = left_margin;
            ++y;
            OutputString(COLOR_LIGHTGREEN, x, y, "F");
            OutputString(COLOR_WHITE, x, y, ": ");
            OutputString((show_female) ? COLOR_WHITE : COLOR_GREY, x, y, "Female");

            x = left_margin;
            ++y;
            OutputString(COLOR_LIGHTGREEN, x, y, "M");
            OutputString(COLOR_WHITE, x, y, ": ");
            OutputString((show_male) ? COLOR_WHITE : COLOR_GREY, x, y, "Male");
        }
    }

private:
    df::ui_sidebar_mode mode;
    string search_string;
    bool initialized;
    bool entry_mode;
    bool show_non_grazers, show_pastured, show_noncaged, show_male, show_female, show_other_zones;

    std::vector<int8_t> saved_ui_building_assign_type;
    std::vector<df::unit*> saved_ui_building_assign_units, reference_list;
    std::vector<df::item*> saved_ui_building_assign_items;
    std::vector<char> saved_ui_building_assign_is_marked;

    vector <int> saved_indexes;

};

struct zone_hook : public df::viewscreen_dwarfmodest
{
    typedef df::viewscreen_dwarfmodest interpose_base;
    static zone_filter filter;

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input))
    {
        if (!filter.handle_input(input))
            INTERPOSE_NEXT(feed)(input);
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        if ( ( (ui->main.mode == ui_sidebar_mode::ZonesPenInfo || ui->main.mode == ui_sidebar_mode::ZonesPitInfo) &&
            ui_building_assign_type && ui_building_assign_units &&
            ui_building_assign_is_marked && ui_building_assign_items &&
            ui_building_assign_type->size() == ui_building_assign_units->size() &&
            ui_building_item_cursor)
            // allow mode QueryBuilding, but only for cages (bedrooms will crash DF with this code, chains don't work either etc)
            ||
            (   ui->main.mode == ui_sidebar_mode::QueryBuilding &&
                ui_building_in_assign && *ui_building_in_assign &&
                ui_building_assign_type && ui_building_assign_units &&
                ui_building_assign_type->size() == ui_building_assign_units->size() &&
                ui_building_assign_type->size() == ui_building_assign_items->size() &&
                ui_building_assign_type->size() == ui_building_assign_is_marked->size() &&
                ui_building_item_cursor &&
                world->selected_building && isCage(world->selected_building) )
            )
        {
            if (vector_get(*ui_building_assign_units, *ui_building_item_cursor))
                filter.initialize(ui->main.mode);
        }
        else
        {
            filter.deinitialize();
        }

        INTERPOSE_NEXT(render)();

        filter.do_render();

    }
};

zone_filter zone_hook::filter;

IMPLEMENT_VMETHOD_INTERPOSE(zone_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(zone_hook, render);
//END zone filters

DFhackCExport command_result plugin_enable ( color_ostream &out, bool enable)
{
    if (!gps)
        return CR_FAILURE;

    if (enable != is_enabled)
    {
        if (!INTERPOSE_HOOK(zone_hook, feed).apply(enable) ||
            !INTERPOSE_HOOK(zone_hook, render).apply(enable))
            return CR_FAILURE;

        is_enabled = enable;
    }

    return CR_OK;
}

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
    init_autobutcher(out);
    init_autonestbox(out);
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    cleanup_autobutcher(out);
    cleanup_autonestbox(out);
    return CR_OK;
}
