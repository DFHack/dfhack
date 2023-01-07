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

#include <functional>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include "df/building_cagest.h"
#include "df/building_chainst.h"
#include "df/building_civzonest.h"
#include "df/general_ref_building_civzone_assignedst.h"
#include "df/plotinfost.h"
#include "df/unit.h"
#include "df/unit_relationship_type.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/world.h"

#include "PluginManager.h"
#include "uicommon.h"
#include "VTableInterpose.h"

#include "modules/Buildings.h"
#include "modules/Gui.h"
#include "modules/Units.h"
#include "modules/Translation.h"

using std::function;
using std::make_pair;
using std::ostringstream;
using std::pair;
using std::runtime_error;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

using namespace DFHack;

DFHACK_PLUGIN("zone");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(cursor);
REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(plotinfo);
REQUIRE_GLOBAL(ui_building_item_cursor);
REQUIRE_GLOBAL(ui_building_assign_type);
REQUIRE_GLOBAL(ui_building_assign_is_marked);
REQUIRE_GLOBAL(ui_building_assign_units);
REQUIRE_GLOBAL(ui_building_assign_items);
REQUIRE_GLOBAL(ui_building_in_assign);
REQUIRE_GLOBAL(ui_menu_width);
REQUIRE_GLOBAL(world);

static void doMarkForSlaughter(df::unit* unit)
{
    unit->flags2.bits.slaughter = 1;
}

// found a unit with weird position values on one of my maps (negative and in the thousands)
// it didn't appear in the animal stocks screen, but looked completely fine otherwise (alive, tame, own, etc)
// maybe a rare bug, but better avoid assigning such units to zones or slaughter etc.
static bool hasValidMapPos(df::unit* unit)
{
    if(    unit->pos.x >=0 && unit->pos.y >= 0 && unit->pos.z >= 0
        && unit->pos.x < world->map.x_count
        && unit->pos.y < world->map.y_count
        && unit->pos.z < world->map.z_count)
        return true;
    else
        return false;
}

static bool isInBuiltCageRoom(df::unit*);

// dump some unit info
static void unitInfo(color_ostream & out, df::unit* unit, bool verbose = false)
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

    if(Units::isAdult(unit))
        out << "adult";
    else if(Units::isBaby(unit))
        out << "baby";
    else if(Units::isChild(unit))
        out << "child";
    out << " ";
    // sometimes empty even in vanilla RAWS, sometimes contains full race name (e.g. baby alpaca)
    // all animals I looked at don't have babies anyways, their offspring tarts as CHILD
    //out << getRaceBabyName(unit);
    //out << getRaceChildName(unit);

    out << Units::getRaceName(unit) << " (";
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
    out << ", age: " << Units::getAge(unit, true);


    if(Units::isTame(unit))
        out << ", tame";
    if(Units::isOwnCiv(unit))
        out << ", owned";
    if(Units::isWar(unit))
        out << ", war";
    if(Units::isHunter(unit))
        out << ", hunter";
    if(Units::isMerchant(unit))
        out << ", merchant";
    if(Units::isForest(unit))
        out << ", forest";
    if(Units::isEggLayer(unit))
        out << ", egglayer";
    if(Units::isGrazer(unit))
        out << ", grazer";
    if(Units::isMilkable(unit))
        out << ", milkable";
    if(unit->flags2.bits.slaughter)
        out << ", slaughter";

    if(verbose)
    {
        out << ". Pos: ("<<unit->pos.x << "/"<< unit->pos.y << "/" << unit->pos.z << ") " << endl;
        out << "index in units vector: " << Units::findIndexById(unit->id) << endl;
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

static bool isCage(df::building * building)
{
    return building && (building->getType() == df::building_type::Cage);
}

static bool isChain(df::building * building)
{
    return building && (building->getType() == df::building_type::Chain);
}

static df::general_ref_building_civzone_assignedst * createCivzoneRef()
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

static bool isInBuiltCage(df::unit* unit);

// check if assigned to pen, pit, (built) cage or chain
// note: BUILDING_CAGED is not set for animals (maybe it's used for dwarves who get caged as sentence)
// animals in cages (no matter if built or on stockpile) get the ref CONTAINED_IN_ITEM instead
// removing them from cages on stockpiles is no problem even without clearing the ref
// and usually it will be desired behavior to do so.
static bool isAssigned(df::unit* unit)
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

static bool isAssignedToZone(df::unit* unit)
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

// check if contained in item (e.g. animals in cages)
static bool isContainedInItem(df::unit* unit)
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

static bool isInBuiltCage(df::unit* unit)
{
    bool caged = false;
    for (size_t b=0; b < world->buildings.all.size(); b++)
    {
        df::building* building = world->buildings.all[b];
        if( building->getType() == df::building_type::Cage)
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
static bool isInBuiltCageRoom(df::unit* unit)
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

        if(building->getType() == df::building_type::Cage)
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

static df::building * getBuiltCageAtPos(df::coord pos)
{
    df::building* cage = NULL;
    for (size_t b=0; b < world->buildings.all.size(); b++)
    {
        df::building* building = world->buildings.all[b];
        if( building->getType() == df::building_type::Cage
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

// check if unit is already assigned to a zone, remove that ref from unit and old zone
// check if unit is already assigned to a cage, remove that ref from the cage
// returns false if no cage or pasture information was found
// helps as workaround for http://www.bay12games.com/dwarves/mantisbt/view.php?id=4475 by the way
// (pastured animals assigned to chains will get hauled back and forth because the pasture ref is not deleted)
static bool unassignUnitFromBuilding(df::unit* unit)
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
static command_result assignUnitToZone(color_ostream& out, df::unit* unit, df::building* building, bool verbose = false)
{
    // building must be a pen/pasture or pit
    if(!Buildings::isPenPasture(building) && !Buildings::isPitPond(building))
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
        << "(" << Units::getRaceName(unit) << ")"
        << " assigned to zone " << building->id;
    if(Buildings::isPitPond(building))
        out << " (pit/pond).";
    if(Buildings::isPenPasture(building))
        out << " (pen/pasture).";
    out << endl;

    return CR_OK;
}

static command_result assignUnitToCage(color_ostream& out, df::unit* unit, df::building* building, bool verbose)
{
    // building must be a pen/pasture or pit
    if(!isCage(building))
    {
        out << "Invalid building type. This is not a cage." << endl;
        return CR_WRONG_USAGE;
    }

    // don't assign owned pets to a cage. the owner will release them, resulting into infinite hauling (df bug)
    if(unit->relationship_ids[df::unit_relationship_type::Pet] != -1)
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
        << "(" << Units::getRaceName(unit) << ")"
        << " assigned to cage " << building->id;
    out << endl;

    return CR_OK;
}

static command_result assignUnitToChain(color_ostream& out, df::unit* unit, df::building* building, bool verbose)
{
    out << "sorry. assigning to chains is not possible yet." << endl;
    return CR_WRONG_USAGE;
}

static command_result assignUnitToBuilding(color_ostream& out, df::unit* unit, df::building* building, bool verbose)
{
    command_result result = CR_WRONG_USAGE;

    if(Buildings::isActivityZone(building))
        result = assignUnitToZone(out, unit, building, verbose);
    else if(isCage(building))
        result = assignUnitToCage(out, unit, building, verbose);
    else if(isChain(building))
        result = assignUnitToChain(out, unit, building, verbose);
    else
        out << "Cannot assign units to this type of building!" << endl;

    return result;
}

static command_result assignUnitsToCagezone(color_ostream& out, vector<df::unit*> units, df::building* building, bool verbose)
{
    if(!Buildings::isPenPasture(building))
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

static command_result nickUnitsInZone(color_ostream& out, df::building* building, string nick)
{
    // building must be a pen/pasture or pit
    if(!Buildings::isPenPasture(building) && !Buildings::isPitPond(building))
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

static command_result nickUnitsInCage(color_ostream& out, df::building* building, string nick)
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

static command_result nickUnitsInChain(color_ostream& out, df::building* building, string nick)
{
    out << "sorry. nicknaming chained units is not possible yet." << endl;
    return CR_WRONG_USAGE;
}

// give all units inside a pasture or cage the same nickname
// (usage example: protect them from being autobutchered)
static command_result nickUnitsInBuilding(color_ostream& out, df::building* building, string nick)
{
    command_result result = CR_WRONG_USAGE;

    if(Buildings::isActivityZone(building))
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
static void zoneInfo(color_ostream & out, df::building* building, bool verbose)
{
    if(!Buildings::isActivityZone(building))
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
    if(Buildings::isActive(civ))
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
static void cageInfo(color_ostream & out, df::building* building, bool verbose)
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
static void chainInfo(color_ostream & out, df::building* building, bool list_refs = false)
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

static df::building* getAssignableBuildingAtCursor(color_ostream& out)
{
    // set building at cursor position to be new target building
    if (cursor->x == -30000)
    {
        out.printerr("No cursor; place cursor over activity zone, pen,"
                " pasture, pit, pond, chain, or cage.\n");
        return NULL;
    }

    auto building_at_tile = Buildings::findAtTile(Gui::getCursorPos());

    // cagezone wants a pen/pit as starting point
    if (isCage(building_at_tile))
    {
        out << "Target building type: cage." << endl;
        return building_at_tile;
    }
    else
    {
        auto zone_at_tile = Buildings::findPenPitAt(Gui::getCursorPos());
        if(!zone_at_tile)
        {
            out << "No pen/pasture, pit, or cage under cursor!" << endl;
            return NULL;
        }
        else
        {
            out << "Target building type: pen/pasture or pit." << endl;
            return zone_at_tile;
        }
    }
}

// ZONE FILTERS (as in, filters used by 'zone')

// Maps parameter names to filters.
static unordered_map<string, function<bool(df::unit*)>> zone_filters;
static struct zone_filters_init { zone_filters_init() {
    zone_filters["caged"] = isContainedInItem;
    zone_filters["egglayer"] = Units::isEggLayer;
    zone_filters["female"] = Units::isFemale;
    zone_filters["grazer"] = Units::isGrazer;
    zone_filters["hunting"] = Units::isHunter;
    zone_filters["male"] = Units::isMale;
    zone_filters["milkable"] = Units::isMilkable;
    zone_filters["naked"] = Units::isNaked;
    zone_filters["own"] = Units::isOwnCiv;
    zone_filters["tamable"] = Units::isTamable;
    zone_filters["tame"] = Units::isTame;
    zone_filters["trainablewar"] = [](df::unit *unit) -> bool
    {
        return !Units::isWar(unit) && !Units::isHunter(unit) && Units::isTrainableWar(unit);
    };
    zone_filters["trainablehunt"] = [](df::unit *unit) -> bool
    {
        return !Units::isWar(unit) && !Units::isHunter(unit) && Units::isTrainableHunting(unit);
    };
    zone_filters["trained"] = Units::isTrained;
    // backwards compatibility
    zone_filters["unassigned"] = [](df::unit *unit) -> bool
    {
        return !isAssigned(unit);
    };
    zone_filters["war"] = Units::isWar;
}} zone_filters_init_;

// Extra annotations / descriptions for parameter names.
static unordered_map<string, string> zone_filter_notes;
static struct zone_filter_notes_init { zone_filter_notes_init() {
    zone_filter_notes["caged"] = "caged (ignores built cages)";
    zone_filter_notes["hunting"] = "trained hunting creature";
    zone_filter_notes["named"] = "has name or nickname";
    zone_filter_notes["own"] = "own civilization";
    zone_filter_notes["trainablehunt"] = "trainable for hunting";
    zone_filter_notes["trainablewar"] = "trainable for war";
    zone_filter_notes["war"] = "trained war creature";
}} zone_filter_notes_init_;

static pair<string, function<bool(df::unit*)>> createRaceFilter(vector<string> &filter_args)
{
    // guaranteed to exist.
    string race = filter_args[0];

    return make_pair(
        "race of " + race,
        [race](df::unit *unit) -> bool {
            return Units::getRaceName(unit) == race;
        }
    );
}

static pair<string, function<bool(df::unit*)>> createAgeFilter(vector<string> &filter_args)
{
    int target_age;
    stringstream ss(filter_args[0]);

    ss >> target_age;

    if (ss.fail()) {
        ostringstream err;
        err <<  "Invalid age: " << filter_args[0] << "; age must be a number!";
        throw runtime_error(err.str());
    }
    if (target_age < 0) {
        ostringstream err;
        err <<  "Invalid age: " << target_age << "; age must be >= 0!";
        throw runtime_error(err.str());
    }

    return make_pair(
        "age of exactly " + int_to_string(target_age),
        [target_age](df::unit *unit) -> bool {
            return Units::getAge(unit, true) == target_age;
        }
    );
}

static pair<string, function<bool(df::unit*)>> createMinAgeFilter(vector<string> &filter_args)
{
    double min_age;
    stringstream ss(filter_args[0]);

    ss >> min_age;

    if (ss.fail()) {
        ostringstream err;
        err <<  "Invalid minimum age: " << filter_args[0] << "; age must be a number!";
        throw runtime_error(err.str());
    }
    if (min_age < 0) {
        ostringstream err;
        err <<  "Invalid minimum age: " << min_age << "; age must be >= 0!";
        throw runtime_error(err.str());
    }

    return make_pair(
        "minimum age of " + int_to_string(min_age),
        [min_age](df::unit *unit) -> bool {
            return Units::getAge(unit, true) >= min_age;
        }
    );
}

static pair<string, function<bool(df::unit*)>> createMaxAgeFilter(vector<string> &filter_args)
{
    double max_age;
    stringstream ss(filter_args[0]);

    ss >> max_age;

    if (ss.fail()) {
        ostringstream err;
        err <<  "Invalid maximum age: " << filter_args[0] << "; age must be a number!";
        throw runtime_error(err.str());
    }
    if (max_age < 0) {
        ostringstream err;
        err <<  "Invalid maximum age: " << max_age << "; age must be >= 0!";
        throw runtime_error(err.str());
    }

    return make_pair(
        "maximum age of " + int_to_string(max_age),
        [max_age](df::unit *unit) -> bool {
            return Units::getAge(unit, true) <= max_age;
        }
    );
}

// Filters that take arguments.
// Wow, what a type signature.
// Applied to their number of arguments in a vector<string>& to create a pair.
// Like:
//     int argcount = zone_param_filters[...].first;
//     function<bool(df::unit*)> filter = zone_param_filters[...].second(filter_args);
// (description, filter).
// Sort of like filter function constructors.
// Constructor functions are permitted to throw strings, which will be caught and printed.
// Result filter functions are not permitted to throw std::exceptions.
// Result filter functions should not store references
static unordered_map<string, pair<int,
           function<pair<string, function<bool(df::unit*)>>(vector<string>&)>>> zone_param_filters;
static struct zone_param_filters_init { zone_param_filters_init() {
    zone_param_filters["race"] = make_pair(1, createRaceFilter);
    zone_param_filters["age"] = make_pair(1, createAgeFilter);
    zone_param_filters["minage"] = make_pair(1, createMinAgeFilter);
    zone_param_filters["maxage"] = make_pair(1, createMaxAgeFilter);
}} zone_param_filters_init_;

static command_result df_zone(color_ostream &out, vector <string> & parameters) {
    CoreSuspender suspend;

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    static df::building* target_building = NULL;

    int target_count = 0;

    bool unit_info = false;
    bool unit_slaughter = false;
    bool building_assign = false;
    bool building_unassign = false;
    bool cagezone_assign = false;
    bool nick_set = false;
    string target_nick;
    bool enum_nick = false;
    string enum_prefix;

    bool verbose = false;

    size_t start_index;

    {
        const string & p0 = parameters[0];

        if (p0 == "help" || p0 == "?")
        {
            return CR_WRONG_USAGE;
        }
        else if(p0 == "zinfo")
        {
            if (cursor->x == -30000) {
                out.color(COLOR_RED);
                out << "No cursor; place cursor over activity zone, chain, or cage." << endl;
                out.reset_color();

                return CR_FAILURE;
            }

            // give info on zone(s), chain or cage under cursor
            // (doesn't use the findXyzAtCursor() methods because zones might
            // overlap and contain a cage or chain)
            vector<df::building_civzonest*> zones;
            Buildings::findCivzonesAt(&zones, Gui::getCursorPos());
            for (auto zone = zones.begin(); zone != zones.end(); ++zone)
                zoneInfo(out, *zone, verbose);
            df::building* building = Buildings::findAtTile(Gui::getCursorPos());
            chainInfo(out, building, verbose);
            cageInfo(out, building, verbose);
            return CR_OK;
        }
        else if(p0 == "set")
        {
            target_building = getAssignableBuildingAtCursor(out);
            if (target_building) {
                out.color(COLOR_BLUE);
                out << "Current building set to #"
                    << target_building->id << endl;
                out.reset_color();

                return CR_OK;
            } else {
                out.color(COLOR_BLUE);
                out << "Current building unset (no building under cursor)." << endl;
                out.reset_color();

                return CR_OK;
            }
        }
        else if(p0 == "unassign")
        {
            // if there's a unit selected...
            df::unit *unit = Gui::getSelectedUnit(out, true);
            if (unit) {
                // remove assignment reference from unit and old zone
                if(unassignUnitFromBuilding(unit))
                {
                    out.color(COLOR_BLUE);
                    out << "Selected unit unassigned." << endl;
                    out.reset_color();

                }
                else
                {
                    out.color(COLOR_RED);
                    out << "Selected unit is not assigned to an activity zone!"
                        << endl;
                    out.reset_color();

                }
            }

            // if followed by another parameter, check if it's numeric
            bool target_building_given = false;
            if(parameters.size() >= 2)
            {
                auto & str = parameters[1];
                if(str.size() > 0 && str[0] >= '0' && str[0] <= '9')
                {
                    stringstream ss(parameters[1]);
                    int new_building = -1;
                    ss >> new_building;
                    if(new_building != -1)
                    {
                        target_building = df::building::find(new_building);
                        target_building_given = true;
                        out << "Using building #" << new_building << endl;
                    }
                    else
                    {
                        out.color(COLOR_RED);
                        out << "Couldn't parse " << parameters[1] << endl;
                        out.reset_color();

                    }
                }
            }
            if (!target_building_given)
            {
                if(target_building) {
                    out << "No building id specified. Will use #"
                        << target_building->id << endl;
                } else {
                    out.color(COLOR_RED);
                    out << "No building id specified and current one is invalid!" << endl;
                    out.reset_color();

                    return CR_WRONG_USAGE;
                }
            }

            out << "Unassigning unit(s) from building..." << endl;
            building_unassign = true;
            start_index = 1;
        }
        else if(p0 == "uinfo")
        {
            out << "Logging unit info..." << endl;
            unit_info = true;
            start_index = 1;
        }
        else if(p0 == "assign" || p0 == "tocages")
        {
            const char* const building_type = p0 == "assign" ? "building" : "cage zone";

            // if followed by another parameter, check if it's numeric

            bool target_building_given = false;
            if(parameters.size() >= 2)
            {
                auto & str = parameters[1];
                if(str.size() > 0 && str[0] >= '0' && str[0] <= '9')
                {
                    stringstream ss(parameters[1]);
                    int new_building = -1;
                    ss >> new_building;
                    if(new_building != -1)
                    {
                        target_building = df::building::find(new_building);
                        target_building_given = true;
                        out << "Assign unit(s) to " << building_type
                            << " #" << new_building << endl;
                        start_index = 2;
                    }
                }
            }
            if(!target_building_given)
            {
                if(target_building) {
                    out << "No " << building_type << " id specified. Will use #"
                        << target_building->id << endl;
                    building_assign = true;
                    start_index = 1;
                } else {
                    out.color(COLOR_RED);
                    out << "No " << building_type
                        << " id specified and current one is invalid!" << endl;
                    out.reset_color();

                    return CR_WRONG_USAGE;
                }
            }

            if (p0 == "assign")
            {
                building_assign = true;
            }
            else
            {
                cagezone_assign = true;
            }
        }
        else if(p0 == "slaughter")
        {
            out << "Assign animals for slaughter." << endl;
            unit_slaughter = true;
            start_index = 1;
        }
        else if(p0 == "nick")
        {
            if(parameters.size() <= 2)
            {
                out.printerr("No nickname specified! Use 'remnick' to remove nicknames!\n");
                return CR_WRONG_USAGE;
            }
            nick_set = true;
            target_nick = parameters[1];
            start_index = 2;
            out << "Set nickname to: " << target_nick << endl;
        }
        else if(p0 == "enumnick")
        {
            if(parameters.size() <= 2)
            {
                out.printerr("No prefix specified! Use 'remnick' to remove nicknames!\n");
                return CR_WRONG_USAGE;
            }
            enum_nick = true;
            enum_prefix = parameters[1];
            start_index = 2;
            out << "Set nickname to: " << enum_prefix <<" <N>" << endl;
        }
        else if(p0 == "remnick")
        {
            nick_set = true;
            target_nick = "";
            start_index = 1;
            out << "Remove nicknames..." << endl;
        }
        else if(p0 == "zone") {
            // when there are no other arguments
            return CR_WRONG_USAGE;
        }
        else
        {
            out.color(COLOR_RED);
            out << "Unknown command: " << p0 << endl;
            out.reset_color();

            return CR_WRONG_USAGE;
        }
    }

    // whether to invert the next filter we read
    bool invert_filter = false;

    // custom handling for things with defaults
    bool merchant_filter_set = false;
    bool named_filter_set = false;
    bool race_filter_set = false;

    // a vector of active filters
    // if all filters return true, we process a unit
    // if any filter returns false, we skip that unit
    vector<function<bool(df::unit*)>> active_filters;

    for (size_t i = start_index; i < parameters.size(); i++)
    {
        string& p = parameters[i];

        if(p == "verbose")
        {
            if(invert_filter)
            {
                out.color(COLOR_RED);
                out << "Note: 'not verbose' unnecessary, verbose mode disabled by default"
                    << endl;
                out.reset_color();

            }
            verbose = !invert_filter;
            invert_filter = false;
        }
        else if(p == "not")
        {
            invert_filter = true;
        }
        else if(p == "count")
        {
            if(invert_filter)
            {
                out.color(COLOR_RED);
                out << "Error: 'not count' doesn't make sense." << endl;
                out.reset_color();

                return CR_WRONG_USAGE;
            }
            if(i == parameters.size()-1)
            {
                out.color(COLOR_RED);
                out << "Error: no count specified." << endl;
                out.reset_color();

                return CR_WRONG_USAGE;
            }
            else
            {
                stringstream ss(parameters[i+1]);
                i++;
                ss >> target_count;
                if(target_count <= 0)
                {
                    out.color(COLOR_RED);
                    out << "Error: invalid count specified (must be > 0)."
                        << endl;
                    out.reset_color();

                }
                out.color(COLOR_GREEN);
                out << "Process up to " << target_count << " units."
                    << endl;
                out.reset_color();

            }
        }
        else if(p == "all")
        {
            if(invert_filter) {
                out.color(COLOR_RED);
                out << "Error: 'not all' doesn't make sense."
                    << endl;
                out.reset_color();

                return CR_WRONG_USAGE;
            }
            out.color(COLOR_GREEN);
            out << "Process all units." << endl;
            out.reset_color();

            target_count = INT_MAX;
        }
        else if (zone_filters.count(p) == 1) {
            string& desc = zone_filter_notes.count(p) == 1 ?
                zone_filter_notes[p] : p;

            if (invert_filter) {
                auto &filter = zone_filters[p];

                auto z = not1(filter);

                // we have to invert the filter, so we use a closure
                active_filters.push_back(not1(filter));

                out.color(COLOR_GREEN);
                out << "Filter: 'not " << desc << "'"
                    << endl;
                out.reset_color();

            } else {
                active_filters.push_back(zone_filters[p]);
                out.color(COLOR_GREEN);
                out << "Filter: '" << desc << "'"
                    << endl;
                out.reset_color();

            }

            invert_filter = false;
        } else if (zone_param_filters.count(p) == 1) {
            // get the constructor
            auto &filter_pair = zone_param_filters[p];
            auto arg_count = filter_pair.first;
            auto &filter_constructor = filter_pair.second;
            vector<string> args;

            // get arguments
            while (arg_count) {
                i++;
                arg_count--;
                args.push_back(parameters[i]);
            }

            // get results
            try {
                auto result_pair = filter_constructor(args);
                string& desc = result_pair.first;
                auto& filter = result_pair.second;

                if (invert_filter) {
                    active_filters.push_back(not1(filter));
                    out.color(COLOR_GREEN);
                    out << "Filter: 'not " << desc << "'"
                        << endl;
                    out.reset_color();

                } else {
                    active_filters.push_back(filter);
                    out.color(COLOR_GREEN);
                    out << "Filter: '" << desc << "'"
                        << endl;
                    out.reset_color();

                }
                invert_filter = false;

                if (p == "race") {
                    race_filter_set = true;
                }
            } catch (const exception&) {
                return CR_FAILURE;
            }
        }
        else if(p == "merchant")
        {
            if (invert_filter) {
                out.color(COLOR_GREEN);
                out << "Filter: 'not merchant'" << endl;
                out.reset_color();

                active_filters.push_back([](df::unit *unit)
                    {
                        return !Units::isMerchant(unit) && !Units::isForest(unit);
                    }
                );
            } else {
                out.color(COLOR_GREEN);
                out << "Filter: 'merchant'" << endl;
                out.reset_color();

                active_filters.push_back([](df::unit *unit)
                    {
                        return Units::isMerchant(unit) || Units::isForest(unit);
                    }
                );
            }
            merchant_filter_set = true;
            invert_filter = false;
        }
        else if(p == "named")
        {
            if (invert_filter) {
                out.color(COLOR_GREEN);
                out << "Filter: 'not named'" << endl;
                out.reset_color();

                if (unit_slaughter) {
                    out.color(COLOR_RED);
                    out << "Note: 'not named' unnecessary when slaughtering, "
                        "named units ignored by default" << endl;
                    out.reset_color();

                }
                active_filters.push_back([](df::unit *unit)
                    {
                        return !unit->name.has_name;
                    }
                );
            }
            else
            {
                out.color(COLOR_GREEN);
                out << "Filter: 'named'" << endl;
                out.reset_color();

                active_filters.push_back([](df::unit *unit)
                    {
                        return unit->name.has_name;
                    }
                );
            }
            named_filter_set = true;
            invert_filter = false;
        } else {
            out.printerr("Unknown command: %s\n", p.c_str());
            return CR_WRONG_USAGE;
        }
    }

    if (building_unassign)
    {
        // filter for units in the building
        unordered_set<int32_t> assigned_unit_ids;
        if(Buildings::isActivityZone(target_building))
        {
            df::building_civzonest *civz = (df::building_civzonest *) target_building;
            auto &assigned_units_vec = civz->assigned_units;

            copy(assigned_units_vec.begin(),
                   assigned_units_vec.end(),
                   std::inserter(assigned_unit_ids, assigned_unit_ids.end()));

        }
        else if(isCage(target_building))
        {
            df::building_cagest *cage = (df::building_cagest *) target_building;
            auto &assigned_units_vec = cage->assigned_units;

            copy(assigned_units_vec.begin(),
                   assigned_units_vec.end(),
                   std::inserter(assigned_unit_ids, assigned_unit_ids.end()));

        }
        else if(isChain(target_building))
        {
            out.color(COLOR_RED);
            out << "Sorry, unassigning units from chains is not possible yet."
                << endl;
            out.reset_color();

            return CR_FAILURE;
        }
        else
        {
            out.color(COLOR_RED);
            out << "Cannot unassign units from this type of building!"
                << endl;
            out.reset_color();

            return CR_FAILURE;
        }

        active_filters.push_back([assigned_unit_ids](df::unit *unit) -> bool
            {
                return assigned_unit_ids.count(unit->id) == 1;
            }
        );
    }

    if (target_count == 0)
    {
        out.color(COLOR_RED);
        out << "No target count! 'zone " << parameters[0]
            << "' must be followed by 'all' or 'count [COUNT]'." << endl;
        out.reset_color();

        return CR_WRONG_USAGE;
    }

    if(!race_filter_set && (building_assign || cagezone_assign || unit_slaughter))
    {
        string own_race_name = Units::getRaceNameById(plotinfo->race_id);
        out.color(COLOR_BROWN);
        out << "Default filter for " << parameters[0]
            << ": 'not (race " << own_race_name << " or own civilization)'; use 'race "
            << own_race_name << "' filter to override."
            << endl;
        out.reset_color();

        active_filters.push_back([](df::unit *unit)
            {
                return !Units::isOwnRace(unit) || !Units::isOwnCiv(unit);
            }
        );
    }
    if(!named_filter_set && unit_slaughter)
    {
        out.color(COLOR_BROWN);
        out << "Default filter for " << parameters[0]
            << ": 'not named'; use 'named' filter to override."
            << endl;
        out.reset_color();

        active_filters.push_back([](df::unit *unit)
            {
                return !unit->name.has_name;
            }
        );
    }
    if(!merchant_filter_set && (building_assign || cagezone_assign || unit_slaughter))
    {
        out.color(COLOR_BROWN);
        out << "Default filter for " << parameters[0]
            << ": 'not merchant'; use 'merchant' filter to override."
            << endl;
        out.reset_color();

        active_filters.push_back([](df::unit *unit)
            {
                return !Units::isMerchant(unit) && !Units::isForest(unit);
            }
        );
    }

    if(building_assign || cagezone_assign || (nick_set && target_count == 0))
    {
        if (!target_building)
        {
            out.color(COLOR_RED);
            out << "Invalid building id." << endl;
            out.reset_color();

            return CR_WRONG_USAGE;
        }
        if(nick_set)
        {
            out.color(COLOR_BLUE);
            out << "Renaming all units in target building."
                << endl;
            out.reset_color();

            return nickUnitsInBuilding(out, target_building, target_nick);
        }
    }

    if(target_count > 0)
    {
        vector <df::unit*> units_for_cagezone;
        int count = 0;
        int matchedCount = 0;
        for(auto unit_it = world->units.all.begin(); unit_it != world->units.all.end(); ++unit_it)
        {
            df::unit *unit = *unit_it;

            // ignore inactive and undead units
            if (!Units::isActive(unit) || Units::isUndead(unit)) {
                continue;
            }

            bool passes_all_filters = true;

            for (auto filter_it = active_filters.begin(); filter_it != active_filters.end(); ++filter_it)
            {
                auto &filter = *filter_it;

                if (!filter(unit)) {
                    passes_all_filters = false;
                    break;
                }
            }

            if (!passes_all_filters) {
                continue;
            }

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

            matchedCount++;

            if(unit_info)
            {
                unitInfo(out, unit, verbose);
                continue;
            }
            else if(nick_set)
            {
                Units::setNickname(unit, target_nick);
            }
            else if(enum_nick)
            {
                std::stringstream ss;
                ss << enum_prefix << " " << matchedCount;
                Units::setNickname(unit, ss.str());
            }
            else if(cagezone_assign)
            {
                units_for_cagezone.push_back(unit);
            }
            else if(building_assign)
            {
                command_result result = assignUnitToBuilding(out, unit, target_building, verbose);
                if(result != CR_OK)
                    return result;
            }
            else if(unit_slaughter)
            {
                doMarkForSlaughter(unit);
            }
            else if(building_unassign)
            {
                bool removed = unassignUnitFromBuilding(unit);

                if (removed)
                {
                    out << "Unit " << unit->id
                        << "(" << Units::getRaceName(unit) << ")"
                        << " unassigned from";

                    if (Buildings::isActivityZone(target_building))
                    {
                        out << " zone ";
                    }
                    else if (isCage(target_building))
                    {
                        out << " cage ";
                    }
                    else if (isChain(target_building))
                    {
                        out << " chain ";
                    }
                    else
                    {
                        out << " ???? ";
                    }

                    out << target_building->id << endl;
                }
                else
                {
                    out.printerr("Failed to remove unit from building:");
                    unitInfo(out, unit, true);
                }

            }

            count++;
            if(count >= target_count)
                break;
        }
        if(cagezone_assign)
        {
            command_result result = assignUnitsToCagezone(out, units_for_cagezone, target_building, verbose);
            if(result != CR_OK)
                return result;
        }

        out.color(COLOR_BLUE);
        out << "Matched creatures: " << matchedCount << endl;
        out << "Processed creatures: " << count << endl;
        out.reset_color();
    }
    else
    {
        // must have unit selected
        df::unit *unit = Gui::getSelectedUnit(out, true);
        if (!unit) {
            out.color(COLOR_RED);
            out << "Error: no unit selected!" << endl;
            out.reset_color();

            return CR_WRONG_USAGE;
        }

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
            doMarkForSlaughter(unit);
            return CR_OK;
        }
    }

    return CR_OK;
}

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

            if (!show_non_grazers && !Units::isGrazer(curr_unit))
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

            if (!show_male && Units::isMale(curr_unit))
                continue;

            if (!show_female && Units::isFemale(curr_unit))
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
        int8_t a = (*ui_menu_width)[0];
        int8_t b = (*ui_menu_width)[1];
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
        if ( ( (plotinfo->main.mode == ui_sidebar_mode::ZonesPenInfo || plotinfo->main.mode == ui_sidebar_mode::ZonesPitInfo) &&
            ui_building_assign_type && ui_building_assign_units &&
            ui_building_assign_is_marked && ui_building_assign_items &&
            ui_building_assign_type->size() == ui_building_assign_units->size() &&
            ui_building_item_cursor)
            // allow mode QueryBuilding, but only for cages (bedrooms will crash DF with this code, chains don't work either etc)
            ||
            (   plotinfo->main.mode == ui_sidebar_mode::QueryBuilding &&
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
                filter.initialize(plotinfo->main.mode);
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

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (enable != is_enabled) {
        if (!INTERPOSE_HOOK(zone_hook, feed).apply(enable) ||
            !INTERPOSE_HOOK(zone_hook, render).apply(enable))
            return CR_FAILURE;

        is_enabled = enable;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    commands.push_back(PluginCommand(
        "zone",
        "Manage activity zones.",
        df_zone));
    return CR_OK;
}
