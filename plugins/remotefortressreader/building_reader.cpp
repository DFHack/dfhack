#include "building_reader.h"
#include "DataDefs.h"

#include "df/building_axle_horizontalst.h"
#include "df/building_bridgest.h"
#include "df/building_def_furnacest.h"
#include "df/building_def_workshopst.h"
#include "df/building_rollersst.h"
#include "df/building_screw_pumpst.h"
#include "df/building_siegeenginest.h"
#include "df/building_water_wheelst.h"
#include "df/building_wellst.h"
#include "df/building_windmillst.h"
#include "df/world.h"


#include "modules/Buildings.h"


using namespace DFHack;
using namespace df::enums;
using namespace RemoteFortressReader;
using namespace std;

DFHack::command_result GetBuildingDefList(DFHack::color_ostream &stream, const DFHack::EmptyMessage *in, RemoteFortressReader::BuildingList *out)
{
    FOR_ENUM_ITEMS(building_type, bt)
    {
        BuildingDefinition * bld = out->add_building_list();
        bld->mutable_building_type()->set_building_type(bt);
        bld->mutable_building_type()->set_building_subtype(-1);
        bld->mutable_building_type()->set_building_custom(-1);
        bld->set_id(ENUM_KEY_STR(building_type, bt));

        switch (bt)
        {
        case df::enums::building_type::NONE:
            break;
        case df::enums::building_type::Chair:
            break;
        case df::enums::building_type::Bed:
            break;
        case df::enums::building_type::Table:
            break;
        case df::enums::building_type::Coffin:
            break;
        case df::enums::building_type::FarmPlot:
            break;
        case df::enums::building_type::Furnace:
            FOR_ENUM_ITEMS(furnace_type, st)
            {
                bld = out->add_building_list();
                bld->mutable_building_type()->set_building_type(bt);
                bld->mutable_building_type()->set_building_subtype(st);
                bld->mutable_building_type()->set_building_custom(-1);
                bld->set_id(ENUM_KEY_STR(building_type, bt) + "_" + ENUM_KEY_STR(furnace_type, st));

                if (st == furnace_type::Custom)
                {
                    for (int i = 0; i < df::global::world->raws.buildings.furnaces.size(); i++)
                    {
                        auto cust = df::global::world->raws.buildings.furnaces[i];

                        bld = out->add_building_list();
                        bld->mutable_building_type()->set_building_type(bt);
                        bld->mutable_building_type()->set_building_subtype(st);
                        bld->mutable_building_type()->set_building_custom(cust->id);
                        bld->set_id(cust->code);
                        bld->set_name(cust->name);
                    }
                }
            }
            break;
        case df::enums::building_type::TradeDepot:
            break;
        case df::enums::building_type::Shop:
            FOR_ENUM_ITEMS(shop_type, st)
            {
                bld = out->add_building_list();
                bld->mutable_building_type()->set_building_type(bt);
                bld->mutable_building_type()->set_building_subtype(st);
                bld->mutable_building_type()->set_building_custom(-1);
                bld->set_id(ENUM_KEY_STR(building_type, bt) + "_" + ENUM_KEY_STR(shop_type, st));

            }
            break;
        case df::enums::building_type::Door:
            break;
        case df::enums::building_type::Floodgate:
            break;
        case df::enums::building_type::Box:
            break;
        case df::enums::building_type::Weaponrack:
            break;
        case df::enums::building_type::Armorstand:
            break;
        case df::enums::building_type::Workshop:
            FOR_ENUM_ITEMS(workshop_type, st)
            {
                bld = out->add_building_list();
                bld->mutable_building_type()->set_building_type(bt);
                bld->mutable_building_type()->set_building_subtype(st);
                bld->mutable_building_type()->set_building_custom(-1);
                bld->set_id(ENUM_KEY_STR(building_type, bt) + "_" + ENUM_KEY_STR(workshop_type, st));

                if (st == workshop_type::Custom)
                {
                    for (int i = 0; i < df::global::world->raws.buildings.workshops.size(); i++)
                    {
                        auto cust = df::global::world->raws.buildings.workshops[i];

                        bld = out->add_building_list();
                        bld->mutable_building_type()->set_building_type(bt);
                        bld->mutable_building_type()->set_building_subtype(st);
                        bld->mutable_building_type()->set_building_custom(cust->id);
                        bld->set_id(cust->code);
                        bld->set_name(cust->name);
                    }
                }
            }
            break;
        case df::enums::building_type::Cabinet:
            break;
        case df::enums::building_type::Statue:
            break;
        case df::enums::building_type::WindowGlass:
            break;
        case df::enums::building_type::WindowGem:
            break;
        case df::enums::building_type::Well:
            break;
        case df::enums::building_type::Bridge:
            break;
        case df::enums::building_type::RoadDirt:
            break;
        case df::enums::building_type::RoadPaved:
            break;
        case df::enums::building_type::SiegeEngine:
            FOR_ENUM_ITEMS(siegeengine_type, st)
            {
                bld = out->add_building_list();
                bld->mutable_building_type()->set_building_type(bt);
                bld->mutable_building_type()->set_building_subtype(st);
                bld->mutable_building_type()->set_building_custom(-1);
                bld->set_id(ENUM_KEY_STR(building_type, bt) + "_" + ENUM_KEY_STR(siegeengine_type, st));

            }
            break;
        case df::enums::building_type::Trap:
            FOR_ENUM_ITEMS(trap_type, st)
            {
                bld = out->add_building_list();
                bld->mutable_building_type()->set_building_type(bt);
                bld->mutable_building_type()->set_building_subtype(st);
                bld->mutable_building_type()->set_building_custom(-1);
                bld->set_id(ENUM_KEY_STR(building_type, bt) + "_" + ENUM_KEY_STR(trap_type, st));

            }
            break;
        case df::enums::building_type::AnimalTrap:
            break;
        case df::enums::building_type::Support:
            break;
        case df::enums::building_type::ArcheryTarget:
            break;
        case df::enums::building_type::Chain:
            break;
        case df::enums::building_type::Cage:
            break;
        case df::enums::building_type::Stockpile:
            break;
        case df::enums::building_type::Civzone:
            FOR_ENUM_ITEMS(civzone_type, st)
            {
                bld = out->add_building_list();
                bld->mutable_building_type()->set_building_type(bt);
                bld->mutable_building_type()->set_building_subtype(st);
                bld->mutable_building_type()->set_building_custom(-1);
                bld->set_id(ENUM_KEY_STR(building_type, bt) + "_" + ENUM_KEY_STR(civzone_type, st));

            }
            break;
        case df::enums::building_type::Weapon:
            break;
        case df::enums::building_type::Wagon:
            break;
        case df::enums::building_type::ScrewPump:
            break;
        case df::enums::building_type::Construction:
            FOR_ENUM_ITEMS(construction_type, st)
            {
                bld = out->add_building_list();
                bld->mutable_building_type()->set_building_type(bt);
                bld->mutable_building_type()->set_building_subtype(st);
                bld->mutable_building_type()->set_building_custom(-1);
                bld->set_id(ENUM_KEY_STR(building_type, bt) + "_" + ENUM_KEY_STR(construction_type, st));

            }
            break;
        case df::enums::building_type::Hatch:
            break;
        case df::enums::building_type::GrateWall:
            break;
        case df::enums::building_type::GrateFloor:
            break;
        case df::enums::building_type::BarsVertical:
            break;
        case df::enums::building_type::BarsFloor:
            break;
        case df::enums::building_type::GearAssembly:
            break;
        case df::enums::building_type::AxleHorizontal:
            break;
        case df::enums::building_type::AxleVertical:
            break;
        case df::enums::building_type::WaterWheel:
            break;
        case df::enums::building_type::Windmill:
            break;
        case df::enums::building_type::TractionBench:
            break;
        case df::enums::building_type::Slab:
            break;
        case df::enums::building_type::Nest:
            break;
        case df::enums::building_type::NestBox:
            break;
        case df::enums::building_type::Hive:
            break;
        case df::enums::building_type::Rollers:
            break;
        default:
            break;
        }
    }
    return CR_OK;
}


void CopyBuilding(int buildingIndex, RemoteFortressReader::BuildingInstance * remote_build)
{
    df::building * local_build = df::global::world->buildings.all[buildingIndex];
    remote_build->set_index(buildingIndex);
    int minZ = local_build->z;
    if (local_build->getType() == df::enums::building_type::Well)
    {
        df::building_wellst * well_building = virtual_cast<df::building_wellst>(local_build);
        if (well_building)
            minZ = well_building->bucket_z;
    }
    remote_build->set_pos_x_min(local_build->x1);
    remote_build->set_pos_y_min(local_build->y1);
    remote_build->set_pos_z_min(minZ);

    remote_build->set_pos_x_max(local_build->x2);
    remote_build->set_pos_y_max(local_build->y2);
    remote_build->set_pos_z_max(local_build->z);

    auto buildingType = remote_build->mutable_building_type();
    auto type = local_build->getType();
    buildingType->set_building_type(type);
    buildingType->set_building_subtype(local_build->getSubtype());
    buildingType->set_building_custom(local_build->getCustomType());

    auto material = remote_build->mutable_material();
    material->set_mat_type(local_build->mat_type);
    material->set_mat_index(local_build->mat_index);

    remote_build->set_building_flags(local_build->flags.whole);
    remote_build->set_is_room(local_build->is_room);
    if (local_build->is_room || local_build->getType() == df::enums::building_type::Civzone || local_build->getType() == df::enums::building_type::Stockpile)
    {
        auto room = remote_build->mutable_room();
        room->set_pos_x(local_build->room.x);
        room->set_pos_y(local_build->room.y);
        room->set_width(local_build->room.width);
        room->set_height(local_build->room.height);
        for (int i = 0; i < (local_build->room.width * local_build->room.height); i++)
        {
            room->add_extents(local_build->room.extents[i]);
        }
    }


    switch (type)
    {
    case df::enums::building_type::NONE:
        break;
    case df::enums::building_type::Chair:
        break;
    case df::enums::building_type::Bed:
        break;
    case df::enums::building_type::Table:
        break;
    case df::enums::building_type::Coffin:
        break;
    case df::enums::building_type::FarmPlot:
        break;
    case df::enums::building_type::Furnace:
        break;
    case df::enums::building_type::TradeDepot:
        break;
    case df::enums::building_type::Shop:
        break;
    case df::enums::building_type::Door:
        break;
    case df::enums::building_type::Floodgate:
        break;
    case df::enums::building_type::Box:
        break;
    case df::enums::building_type::Weaponrack:
        break;
    case df::enums::building_type::Armorstand:
        break;
    case df::enums::building_type::Workshop:
        break;
    case df::enums::building_type::Cabinet:
        break;
    case df::enums::building_type::Statue:
        break;
    case df::enums::building_type::WindowGlass:
        break;
    case df::enums::building_type::WindowGem:
        break;
    case df::enums::building_type::Well:
        break;
    case df::enums::building_type::Bridge:
    {
        auto actual = strict_virtual_cast<df::building_bridgest>(local_build);
        if (actual)
        {
            auto direction = actual->direction;
            switch (direction)
            {
            case df::building_bridgest::Retracting:
                break;
            case df::building_bridgest::Left:
                remote_build->set_direction(WEST);
                break;
            case df::building_bridgest::Right:
                remote_build->set_direction(EAST);
                break;
            case df::building_bridgest::Up:
                remote_build->set_direction(NORTH);
                break;
            case df::building_bridgest::Down:
                remote_build->set_direction(SOUTH);
                break;
            default:
                break;
            }
        }
    }
    break;
    case df::enums::building_type::RoadDirt:
        break;
    case df::enums::building_type::RoadPaved:
        break;
    case df::enums::building_type::SiegeEngine:
    {
        auto actual = strict_virtual_cast<df::building_siegeenginest>(local_build);
        if (actual)
        {
            auto facing = actual->facing;
            switch (facing)
            {
            case df::building_siegeenginest::Left:
                remote_build->set_direction(WEST);
                break;
            case df::building_siegeenginest::Up:
                remote_build->set_direction(NORTH);
                break;
            case df::building_siegeenginest::Right:
                remote_build->set_direction(EAST);
                break;
            case df::building_siegeenginest::Down:
                remote_build->set_direction(SOUTH);
                break;
            default:
                break;
            }
        }
    }
    break;
    case df::enums::building_type::Trap:
        break;
    case df::enums::building_type::AnimalTrap:
        break;
    case df::enums::building_type::Support:
        break;
    case df::enums::building_type::ArcheryTarget:
        break;
    case df::enums::building_type::Chain:
        break;
    case df::enums::building_type::Cage:
        break;
    case df::enums::building_type::Stockpile:
        break;
    case df::enums::building_type::Civzone:
        break;
    case df::enums::building_type::Weapon:
        break;
    case df::enums::building_type::Wagon:
        break;
    case df::enums::building_type::ScrewPump:
    {
        auto actual = strict_virtual_cast<df::building_screw_pumpst>(local_build);
        if (actual)
        {
            auto direction = actual->direction;
            switch (direction)
            {
            case df::enums::screw_pump_direction::FromNorth:
                remote_build->set_direction(NORTH);
                break;
            case df::enums::screw_pump_direction::FromEast:
                remote_build->set_direction(EAST);
                break;
            case df::enums::screw_pump_direction::FromSouth:
                remote_build->set_direction(SOUTH);
                break;
            case df::enums::screw_pump_direction::FromWest:
                remote_build->set_direction(WEST);
                break;
            default:
                break;
            }
        }
    }
    break;
    case df::enums::building_type::Construction:
        break;
    case df::enums::building_type::Hatch:
        break;
    case df::enums::building_type::GrateWall:
        break;
    case df::enums::building_type::GrateFloor:
        break;
    case df::enums::building_type::BarsVertical:
        break;
    case df::enums::building_type::BarsFloor:
        break;
    case df::enums::building_type::GearAssembly:
        break;
    case df::enums::building_type::AxleHorizontal:
    {
        auto actual = strict_virtual_cast<df::building_axle_horizontalst>(local_build);
        if (actual)
        {
            if (actual->is_vertical)
                remote_build->set_direction(NORTH);
            else
                remote_build->set_direction(EAST);
        }
    }
    break;
    case df::enums::building_type::AxleVertical:
        break;
    case df::enums::building_type::WaterWheel:
    {
        auto actual = strict_virtual_cast<df::building_water_wheelst>(local_build);
        if (actual)
        {
            if (actual->is_vertical)
                remote_build->set_direction(NORTH);
            else
                remote_build->set_direction(EAST);
        }
    }
    break;
    case df::enums::building_type::Windmill:
    {
        auto actual = strict_virtual_cast<df::building_windmillst>(local_build);
        if (actual)
        {
            if (actual->orient_x < 0)
                remote_build->set_direction(WEST);
            else if (actual->orient_x > 0)
                remote_build->set_direction(EAST);
            else if (actual->orient_y < 0)
                remote_build->set_direction(NORTH);
            else if (actual->orient_y > 0)
                remote_build->set_direction(SOUTH);
            else
                remote_build->set_direction(WEST);
        }
    }
    break;
    case df::enums::building_type::TractionBench:
        break;
    case df::enums::building_type::Slab:
        break;
    case df::enums::building_type::Nest:
        break;
    case df::enums::building_type::NestBox:
        break;
    case df::enums::building_type::Hive:
        break;
    case df::enums::building_type::Rollers:
    {
        auto actual = strict_virtual_cast<df::building_rollersst>(local_build);
        if (actual)
        {
            auto direction = actual->direction;
            switch (direction)
            {
            case df::enums::screw_pump_direction::FromNorth:
                remote_build->set_direction(NORTH);
                break;
            case df::enums::screw_pump_direction::FromEast:
                remote_build->set_direction(EAST);
                break;
            case df::enums::screw_pump_direction::FromSouth:
                remote_build->set_direction(SOUTH);
                break;
            case df::enums::screw_pump_direction::FromWest:
                remote_build->set_direction(WEST);
                break;
            default:
                break;
            }
        }
    }
    break;
    default:
        break;
    }
}

