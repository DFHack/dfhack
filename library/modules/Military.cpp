#include <string>
#include <vector>
#include <array>
#include "MiscUtils.h"
#include "modules/Military.h"
#include "modules/Translation.h"
#include "df/building.h"
#include "df/building_civzonest.h"
#include "df/historical_figure.h"
#include "df/historical_entity.h"
#include "df/entity_position.h"
#include "df/entity_position_assignment.h"
#include "df/plotinfost.h"
#include "df/squad.h"
#include "df/squad_position.h"
#include "df/squad_schedule_order.h"
#include "df/squad_order.h"
#include "df/squad_order_trainst.h"
#include "df/world.h"

using namespace DFHack;
using namespace df::enums;
using df::global::world;
using df::global::plotinfo;

std::string Military::getSquadName(int32_t squad_id)
{
    df::squad *squad = df::squad::find(squad_id);
    if (!squad)
        return "";
    if (squad->alias.size() > 0)
        return squad->alias;
    return Translation::translateName(&squad->name, true);
}

//only works for making squads for fort mode player controlled dwarf squads
//could be extended straightforwardly by passing in entity
df::squad* Military::makeSquad(int32_t assignment_id)
{
    if (df::global::squad_next_id == nullptr || df::global::plotinfo == nullptr)
        return nullptr;

    df::language_name name;
    name.type = df::language_name_type::Squad;

    for (int i=0; i < 7; i++)
    {
        name.words[i] = -1;
        name.parts_of_speech[i] = df::part_of_speech::Noun;
    }

    df::historical_entity* fort = df::historical_entity::find(df::global::plotinfo->group_id);

    if (fort == nullptr)
        return nullptr;

    df::entity_position_assignment* found_assignment = nullptr;

    for (auto* assignment : fort->positions.assignments)
    {
        if (assignment->id == assignment_id)
        {
            found_assignment = assignment;
            break;
        }
    }

    if (found_assignment == nullptr)
        return nullptr;

    //this function does not attempt to delete or replace squads for assignments
    if (found_assignment->squad_id != -1)
        return nullptr;

    df::entity_position* corresponding_position = nullptr;

    for (auto* position : fort->positions.own)
    {
        if (position->id == found_assignment->position_id)
        {
            corresponding_position = position;
            break;
        }
    }

    if (corresponding_position == nullptr)
        return nullptr;

    df::squad* result = new df::squad();
    result->id = *df::global::squad_next_id;
    result->uniform_priority = result->id + 1; //no idea why, but seems to hold
    result->supplies.carry_food = 2;
    result->supplies.carry_water = df::squad::T_supplies::Water;
    result->entity_id = df::global::plotinfo->group_id;
    result->leader_position = corresponding_position->id;
    result->leader_assignment = found_assignment->id;
    result->name = name;

    int16_t squad_size = corresponding_position->squad_size;

    for (int i=0; i < squad_size; i++)
    {
        //construct for squad_position seems to set all the attributes correctly
        df::squad_position* pos = new df::squad_position();

        result->positions.push_back(pos);
    }

    const auto& routines = df::global::plotinfo->alerts.routines;

    for (const auto& routine : routines)
    {
        df::squad_schedule_entry* asched = (df::squad_schedule_entry*)malloc(sizeof(df::squad_schedule_entry) * 12);

        for(int kk=0; kk < 12; kk++)
        {
            new (&asched[kk]) df::squad_schedule_entry;

            for(int jj=0; jj < squad_size; jj++)
            {
                int32_t* order_assignments = new int32_t();
                *order_assignments = -1;

                asched[kk].order_assignments.push_back(order_assignments);
            }
        }

        auto insert_training_order = [asched, squad_size](int month)
        {
            df::squad_schedule_order* order = new df::squad_schedule_order();
            order->min_count = squad_size;
            //assumed
            order->positions.resize(squad_size);

            df::squad_order* s_order = df::allocate<df::squad_order_trainst>();

            s_order->year = *df::global::cur_year;
            s_order->year_tick = *df::global::cur_year_tick;

            order->order = s_order;

            asched[month].orders.push_back(order);
            //wear uniform while training
            asched[month].uniform_mode = df::squad_civilian_uniform_type::Regular;
        };

        //Dwarf fortress does do this via a series of string comparisons
        //Off duty: No orders, Sleep/room at will. Equip/orders only
        if (routine->name == "Off duty")
        {
            for (int i=0; i < 12; i++)
            {
                asched[i].sleep_mode = df::squad_sleep_option_type::AnywhereAtWill;
                asched[i].uniform_mode = df::squad_civilian_uniform_type::Civilian;
            }
        }
        //Staggered Training: Training orders at months 3 4 5 9 10 11, *or* 0 1 2 6 7 8, sleep/room at will. Equip/orders only, except train months which are equip/always
        else if (routine->name == "Staggered training")
        {
            //this is semi randomised for different squads
            //appears to be something like squad.id & 1, it isn't smart
            //if you alternate squad creation, its 'correctly' staggered
            //but it'll also happily not stagger them if you eg delete a squad and make another
            std::array<int, 6> indices;

            if ((*df::global::squad_next_id) & 1)
            {
                indices = {3, 4, 5, 9, 10, 11};
            }
            else
            {
                indices = {0, 1, 2, 6, 7, 8};
            }

            for (int index : indices)
            {
                insert_training_order(index);
                //still sleep in room at will even when training
                asched[index].sleep_mode = df::squad_sleep_option_type::AnywhereAtWill;
            }
        }
        //see above, but with all indices
        else if (routine->name == "Constant training")
        {
            for (int i=0; i < 12; i++)
            {
                insert_training_order(i);
                //still sleep in room at will even when training
                asched[i].sleep_mode = df::squad_sleep_option_type::AnywhereAtWill;
            }
        }
        else if (routine->name == "Ready")
        {
            for (int i=0; i < 12; i++)
            {
                asched[i].sleep_mode = df::squad_sleep_option_type::InBarracksAtNeed;
                asched[i].uniform_mode = df::squad_civilian_uniform_type::Regular;
            }
        }
        else
        {
            for (int i=0; i < 12; i++)
            {
                asched[i].sleep_mode = df::squad_sleep_option_type::AnywhereAtWill;
                asched[i].uniform_mode = df::squad_civilian_uniform_type::Regular;
            }
        }

        result->schedule.push_back(reinterpret_cast<df::squad::T_schedule*>(asched));
    }

    //Modify necessary world state
    (*df::global::squad_next_id)++;
    fort->squads.push_back(result->id);
    df::global::world->squads.all.push_back(result);
    found_assignment->squad_id = result->id;

    return result;
}

void Military::updateRoomAssignments(int32_t squad_id, int32_t civzone_id, df::squad_use_flags flags)
{
    df::squad* squad = df::squad::find(squad_id);
    df::building* bzone = df::building::find(civzone_id);

    df::building_civzonest* zone = strict_virtual_cast<df::building_civzonest>(bzone);

    if (squad == nullptr || zone == nullptr)
        return;

    df::squad::T_rooms* room_from_squad = nullptr;
    df::building_civzonest::T_squad_room_info* room_from_building = nullptr;

    for (auto room : squad->rooms)
    {
        if (room->building_id == civzone_id)
        {
            room_from_squad = room;
            break;
        }
    }

    for (auto room : zone->squad_room_info)
    {
        if (room->squad_id == squad_id)
        {
            room_from_building = room;
            break;
        }
    }

    if (flags.whole == 0 && room_from_squad == nullptr && room_from_building == nullptr)
        return;

    //if we're setting 0 flags, and there's no room already, don't set a room
    bool avoiding_squad_roundtrip = flags.whole == 0 && room_from_squad == nullptr;

    if (!avoiding_squad_roundtrip && room_from_squad == nullptr)
    {
        room_from_squad = new df::squad::T_rooms();
        room_from_squad->building_id = civzone_id;

        insert_into_vector(squad->rooms, &df::squad::T_rooms::building_id, room_from_squad);
    }

    if (room_from_building == nullptr)
    {
        room_from_building = new df::building_civzonest::T_squad_room_info();
        room_from_building->squad_id = squad_id;

        insert_into_vector(zone->squad_room_info, &df::building_civzonest::T_squad_room_info::squad_id, room_from_building);
    }

    if (room_from_squad)
        room_from_squad->mode = flags;

    room_from_building->mode = flags;

    if (flags.whole == 0 && !avoiding_squad_roundtrip)
    {
        for (int i=0; i < (int)squad->rooms.size(); i++)
        {
            if (squad->rooms[i]->building_id == civzone_id)
            {
                delete squad->rooms[i];
                squad->rooms.erase(squad->rooms.begin() + i);
                i--;
            }
        }
    }
}
