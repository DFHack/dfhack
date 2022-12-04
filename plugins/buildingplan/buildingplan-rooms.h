#pragma once

#include "modules/Persistence.h"
#include "modules/Units.h"

class ReservedRoom
{
public:
    ReservedRoom(df::building *building, std::string noble_code);

    ReservedRoom(DFHack::PersistentDataItem &config, DFHack::color_ostream &out);

    bool checkRoomAssignment();
    void remove();
    bool isValid();

    int32_t getId();
    std::string getCode();
    void setCode(const std::string &noble_code);

private:
    df::building *building;
    DFHack::PersistentDataItem config;
    df::coord pos;

    std::vector<DFHack::Units::NoblePosition> getOwnersNobleCode();
};

class RoomMonitor
{
public:
    RoomMonitor() { }

    std::string getReservedNobleCode(int32_t buildingId);

    void toggleRoomForPosition(int32_t buildingId, std::string noble_code);

    void doCycle();

    void reset(DFHack::color_ostream &out);

private:
    std::vector<ReservedRoom> reservedRooms;

    void addRoom(ReservedRoom &rr);
};

bool canReserveRoom(df::building *building);
std::vector<DFHack::Units::NoblePosition> getUniqueNoblePositions(df::unit *unit);

extern RoomMonitor roomMonitor;
