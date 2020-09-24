#include "buildingplan-rooms.h"
#include "buildingplan-lib.h"

#include <df/entity_position.h>
#include <df/job_type.h>
#include <df/world.h>

#include <modules/World.h>
#include <modules/Units.h>
#include <modules/Buildings.h>

using namespace DFHack;

bool canReserveRoom(df::building *building)
{
    if (!building)
        return false;

    if (building->jobs.size() > 0 && building->jobs[0]->job_type == df::job_type::DestroyBuilding)
        return false;

    return building->is_room;
}

std::vector<Units::NoblePosition> getUniqueNoblePositions(df::unit *unit)
{
    std::vector<Units::NoblePosition> np;
    Units::getNoblePositions(&np, unit);
    for (auto iter = np.begin(); iter != np.end(); iter++)
    {
        if (iter->position->code == "MILITIA_CAPTAIN")
        {
            np.erase(iter);
            break;
        }
    }

    return np;
}

/*
 * ReservedRoom
 */

ReservedRoom::ReservedRoom(df::building *building, std::string noble_code)
{
    this->building = building;
    config = DFHack::World::AddPersistentData("buildingplan/reservedroom");
    config.val() = noble_code;
    config.ival(1) = building->id;
    pos = df::coord(building->centerx, building->centery, building->z);
}

ReservedRoom::ReservedRoom(PersistentDataItem &config, color_ostream &)
{
    this->config = config;

    building = df::building::find(config.ival(1));
    if (!building)
        return;
    pos = df::coord(building->centerx, building->centery, building->z);
}

bool ReservedRoom::checkRoomAssignment()
{
    if (!isValid())
        return false;

    auto np = getOwnersNobleCode();
    bool correctOwner = false;
    for (auto iter = np.begin(); iter != np.end(); iter++)
    {
        if (iter->position->code == getCode())
        {
            correctOwner = true;
            break;
        }
    }

    if (correctOwner)
        return true;

    for (auto iter = df::global::world->units.active.begin(); iter != df::global::world->units.active.end(); iter++)
    {
        df::unit* unit = *iter;
        if (!Units::isCitizen(unit))
            continue;

        if (!Units::isActive(unit))
            continue;

        np = getUniqueNoblePositions(unit);
        for (auto iter = np.begin(); iter != np.end(); iter++)
        {
            if (iter->position->code == getCode())
            {
                Buildings::setOwner(building, unit);
                break;
            }
        }
    }

    return true;
}

void ReservedRoom::remove() { DFHack::World::DeletePersistentData(config); }

bool ReservedRoom::isValid()
{
    if (!building)
        return false;

    if (Buildings::findAtTile(pos) != building)
        return false;

    return canReserveRoom(building);
}

int32_t ReservedRoom::getId()
{
    if (!isValid())
        return 0;

    return building->id;
}

std::string ReservedRoom::getCode() { return config.val(); }

void ReservedRoom::setCode(const std::string &noble_code) { config.val() = noble_code; }

std::vector<Units::NoblePosition> ReservedRoom::getOwnersNobleCode()
{
    if (!building->owner)
        return std::vector<Units::NoblePosition> ();

    return getUniqueNoblePositions(building->owner);
}

/*
 * RoomMonitor
 */

std::string RoomMonitor::getReservedNobleCode(int32_t buildingId)
{
    for (auto iter = reservedRooms.begin(); iter != reservedRooms.end(); iter++)
    {
        if (buildingId == iter->getId())
            return iter->getCode();
    }

    return "";
}

void RoomMonitor::toggleRoomForPosition(int32_t buildingId, std::string noble_code)
{
    bool found = false;
    for (auto iter = reservedRooms.begin(); iter != reservedRooms.end(); iter++)
    {
        if (buildingId != iter->getId())
        {
            continue;
        }
        else
        {
            if (noble_code == iter->getCode())
            {
                iter->remove();
                reservedRooms.erase(iter);
            }
            else
            {
                iter->setCode(noble_code);
            }
            found = true;
            break;
        }
    }

    if (!found)
    {
        ReservedRoom room(df::building::find(buildingId), noble_code);
        reservedRooms.push_back(room);
    }
}

void RoomMonitor::doCycle()
{
    for (auto iter = reservedRooms.begin(); iter != reservedRooms.end();)
    {
        if (iter->checkRoomAssignment())
        {
            ++iter;
        }
        else
        {
            iter->remove();
            iter = reservedRooms.erase(iter);
        }
    }
}

void RoomMonitor::reset(color_ostream &out)
{
    reservedRooms.clear();
    std::vector<PersistentDataItem> items;
    DFHack::World::GetPersistentData(&items, "buildingplan/reservedroom");

    for (auto i = items.begin(); i != items.end(); i++)
    {
        ReservedRoom rr(*i, out);
        if (rr.isValid())
            addRoom(rr);
    }
}

void RoomMonitor::addRoom(ReservedRoom &rr)
{
    for (auto iter = reservedRooms.begin(); iter != reservedRooms.end(); iter++)
    {
        if (iter->getId() == rr.getId())
            return;
    }

    reservedRooms.push_back(rr);
}

RoomMonitor roomMonitor;
