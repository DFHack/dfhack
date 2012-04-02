// Intention: help with activity zone management (auto-pasture animals, auto-pit goblins, ...)
// 
// the following things would be nice:
// - dump info about pastures, pastured animals, count non-pastured tame animals, print gender info
// - help finding caged dwarves? (maybe even allow to build their cages for fast release)
// - dump info about caged goblins, animals, ...
// - full automation of handling mini-pastures over nestboxes:
//   go through all pens, check if they are empty and placed over a nestbox
//   find female tame egg-layer who is not assigned to another pen (allowing to grab them from cages would be nice)
//   maybe check for minimum age? it's not that useful to fill nestboxes with freshly hatched birds
//   assign to that pasture
// - allow to mark old animals for slaughter automatically?
//   that should include a check to ensure that at least one male and one female remain for breeding
//   allow some fine-tuning like how many males/females should per race should be left alive
//   and at what age they are being marked for slaughter. don't slaughter pregnant females?
// - count grass tiles on pastures, move grazers to new pasture if old pasture is empty
//   move hungry unpastured grazers to pasture with grass
// 
// What is working so far:
// - print detailed info about activity zone and units under cursor (mostly for checking refs and stuff)
// - mark a zone which is used for future assignment commands
// - assign single selected creature to a zone
// - unassign single creature under cursor from current zone
// - pitting own dwarves :)

#include <iostream>
#include <iomanip>
#include <climits>
#include <vector>
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

using std::vector;
using std::string;
using namespace DFHack;
using namespace df::enums;
using df::global::world;
using df::global::cursor;
using df::global::ui;

using namespace DFHack::Gui;

struct genref : df::general_ref_building_civzone_assignedst {};

command_result df_zone (color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("zone");

const string zone_help =
    "Allows easier management of pens/pastures and pits.\n"
    "Options:\n"
    "  set          - set zone under cursor as default for future assigns\n"
    "  assign       - assign selected creature to a pen or pit\n"
    "                 (can be followed by valid zone id which will then be set)\n"
    "  unassign     - unassign selected creature from it's zone\n"
    "  uinfo        - print info about selected unit\n"
    "  zinfo        - print info about zone(s) under cursor\n"
	//"  cinfo        - print info about (built) cage under cursor\n"
    //"  rinfo        - print info about restraint under cursor\n"
	"  all          - in combination with 'zinfo' or 'cinfo': prints all zones/units\n"
    "  verbose      - print some more info, mostly useless debug stuff\n"
    "Example:\n"
    "  (ingame) move cursor to a pen/pasture or pit zone\n"
    "  (dfhack) 'zone set' to use this zone for future assignments\n"
    "  (dfhack) map 'zone assign' to a hotkey of your choice\n"
    "  (ingame) select a unit with 'v', 'k' or in the unit list or inside a cage\n"
	"  (ingame) press hotkey to assign unit to it's new home (or pit)\n"
    ;


DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "zone", "manage activity zones.",
        df_zone, false,
        zone_help.c_str()
        ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}


///////////////
// Various small tool functions
// 
int32_t getCreatureAge(df::unit* unit);
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
void unitInfo(color_ostream & out, df::unit* creature, bool list_refs);
void zoneInfo(color_ostream & out, df::building* building, bool list_refs);
void cageInfo(color_ostream & out, df::building* building, bool list_refs);
void chainInfo(color_ostream & out, df::building* building, bool list_refs);


int32_t getUnitAge(df::unit* unit)
{
	// If the birthday this year has not yet passed, subtract one year.
	// ASSUMPTION: birth_time is on the same scale as cur_year_tick
    int32_t yearDifference = *df::global::cur_year - unit->relations.birth_year;
    if (unit->relations.birth_time >= *df::global::cur_year_tick)
        yearDifference--;
    return yearDifference;
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
    if(creature->civ_id == ui->civ_id)
        return true;
    else
        return false;
}

// dump some unit info
void unitInfo(color_ostream & out, df::unit* unit, bool list_refs = false)
{
    out.print("Unit %d", unit->id); //race %d, civ %d,", creature->race, creature->civ_id
    if(unit->name.has_name)
        out << ", name: " << unit->name.first_name << " " << unit->name.nickname;
    df::creature_raw *raw = df::global::world->raws.creatures.all[unit->race];
    out << " " << raw->creature_id << " (";
    switch(unit->sex)
    {
    case -1:
        out << "n/a";
        break;
    case 0:
        out << "female";
        break;
    case 1:
        out << "male";
        break;
    default:
        out << "INVALID!";
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
    
    out << endl;

    if(!list_refs)
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
    if(building->getType() == building_type::Cage)
        return true;
	else
		return false;
}

bool isChain(df::building * building)
{
    if(building->getType() == building_type::Chain)
        return true;
	else
		return false;
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

// check if unit is already assigned to a zone, remove that ref from unit and old zone
// returns false if no pasture information was found
// helps as workaround for http://www.bay12games.com/dwarves/mantisbt/view.php?id=4475 by the way
// (pastured animals assigned to chains will get hauled back and forth because the pasture ref is not deleted)
bool unassignUnitFromZone(df::unit* unit)
{
    bool success = false;
    for (size_t or = 0; or < unit->refs.size(); or++)
    {
        df::general_ref * oldref = unit->refs[or];
        if(oldref->getType() == df::general_ref_type::BUILDING_CIVZONE_ASSIGNED)
        {
            unit->refs.erase(unit->refs.begin() + or);
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
    if(verbose)
    {
        if(unassignUnitFromZone(unit))
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


// dump some zone info
void zoneInfo(color_ostream & out, df::building* building, bool list_refs = false)
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

    // look at zone flags, ignore fishing, water, hospital, ...
    // in fact, only deal with pits and pens
    if(civ->zone_flags.bits.pen_pasture)
        out << ", pen/pasture";
    else if (civ->zone_flags.bits.pit_pond && civ->pit_flags==0)
        out << ", pit";
    else
        return;
    out << endl;

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
                    
            unitInfo(out, creature, false);
        }
    }
}

// dump some cage info
void cageInfo(color_ostream & out, df::building* building, bool list_refs = false)
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
                    
            unitInfo(out, creature, false);
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

    bool zone_assign = false;
    bool zone_unassign = false;
    bool zone_set = false;
    bool verbose = false;
    bool all = false;
    static int target_zone = -1;

    for (size_t i = 0; i < parameters.size(); i++)
    {
        string & p = parameters[i];
        
        if (p == "help" || p == "?")
        {
            out << zone_help << endl;
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
        //else if(p == "cinfo")
        //{
        //    cage_info = true;
        //}
        //else if(p == "rinfo")
        //{
        //    chain_info = true;
        //}
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
            if(i == parameters.size()-1)
            {
                if(target_zone == -1)
                {
                    out.printerr("No zone id specified and current one is invalid!");
                    return CR_WRONG_USAGE;
                }
                else
                {
                    out << "No zone id specified. Will try to use #" << target_zone << endl;
                    zone_assign = true;
                }
            }
            else
            {
                stringstream ss(parameters[i+1]);
                i++;
                ss >> target_zone;
                out << "Assign selected unit to zone #" << target_zone <<std::endl;
                zone_assign = true;
            }
        }
        else if(p == "set")
        {
            zone_set = true;
        }
        else if(p == "all")
        {
            all = true;
        }
        else
            return CR_WRONG_USAGE;
    }

    if((zone_info && !all) || zone_set)
        need_cursor = true;

    if(need_cursor && cursor->x == -30000)
    {
        out.printerr("No cursor; place cursor over activity zone.\n");
        return CR_FAILURE;
    }

    // give info on unit(s)
    if(unit_info)
    {
        if(all)
        {
            // todo: this should really be filtered somehow (prints even dead units etc)
            for(size_t c=0; c<world->units.all.size(); c++)
            {
                df::unit *unit = world->units.all[c];
                unitInfo(out, unit, verbose);
            }
        }
        else
        {
            df::unit *unit = getSelectedUnit(out);
            if (!unit)
                return CR_FAILURE;
            unitInfo(out, unit, verbose);
        }
        return CR_OK;
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

    // de-assign from pen or pit
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
            out << "Unit is not assigned to a zone!" << endl;
        return CR_OK;
    }

    // assign to pen or pit
    if(zone_assign)
    {
        // must have unit selected
        df::unit *unit = getSelectedUnit(out);
        if (!unit)
        {
            out << "No unit selected." << endl;
            return CR_WRONG_USAGE;
        }

		// try to get building index from the id
        int32_t index = findBuildingIndexById(target_zone);
        if(index == -1)
        {
            out << "Invalid building id." << endl;
            target_zone = -1;
            return CR_WRONG_USAGE;
        }

        df::building * building = world->buildings.all.at(index);
		return assignUnitToZone(out, unit, building, verbose);
    }

    return CR_OK;
}
