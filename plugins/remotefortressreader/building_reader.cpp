#include "building_reader.h"
#include "DataDefs.h"

//building types
#include "df/building_actual.h"
#include "df/building_chairst.h"
#include "df/building_bedst.h"
#include "df/building_tablest.h"
#include "df/building_coffinst.h"
#include "df/building_farmplotst.h"
#include "df/building_furnacest.h"
#include "df/building_tradedepotst.h"
#include "df/building_shopst.h"
#include "df/building_doorst.h"
#include "df/building_floodgatest.h"
#include "df/building_boxst.h"
#include "df/building_weaponrackst.h"
#include "df/building_armorstandst.h"
#include "df/building_workshopst.h"
#include "df/building_cabinetst.h"
#include "df/building_statuest.h"
#include "df/building_window_glassst.h"
#include "df/building_window_gemst.h"
#include "df/building_wellst.h"
#include "df/building_bridgest.h"
#include "df/building_road_dirtst.h"
#include "df/building_road_pavedst.h"
#include "df/building_siegeenginest.h"
#include "df/building_trapst.h"
#include "df/building_animaltrapst.h"
#include "df/building_supportst.h"
#include "df/building_archerytargetst.h"
#include "df/building_chainst.h"
#include "df/building_cagest.h"
#include "df/building_stockpilest.h"
#include "df/building_civzonest.h"
#include "df/building_weaponst.h"
#include "df/building_wagonst.h"
#include "df/building_screw_pumpst.h"
#include "df/building_constructionst.h"
#include "df/building_hatchst.h"
#include "df/building_grate_wallst.h"
#include "df/building_grate_floorst.h"
#include "df/building_bars_verticalst.h"
#include "df/building_bars_floorst.h"
#include "df/building_gear_assemblyst.h"
#include "df/building_axle_horizontalst.h"
#include "df/building_axle_verticalst.h"
#include "df/building_water_wheelst.h"
#include "df/building_windmillst.h"
#include "df/building_traction_benchst.h"
#include "df/building_slabst.h"
#include "df/building_nestst.h"
#include "df/building_nest_boxst.h"
#include "df/building_hivest.h"
#include "df/building_rollersst.h"

#include "df/building_def_furnacest.h"
#include "df/building_def_workshopst.h"
#include "df/world.h"
#include "df/machine.h"


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
        bld->set_name(ENUM_ATTR_STR(building_type, name, bt));
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
                bld->set_id(ENUM_KEY_STR(building_type, bt) + "/" + ENUM_KEY_STR(furnace_type, st));
                bld->set_name(ENUM_ATTR_STR(furnace_type, name, st));

                if (st == furnace_type::Custom)
                {
                    for (size_t i = 0; i < df::global::world->raws.buildings.furnaces.size(); i++)
                    {
                        auto cust = df::global::world->raws.buildings.furnaces[i];

                        bld = out->add_building_list();
                        bld->mutable_building_type()->set_building_type(bt);
                        bld->mutable_building_type()->set_building_subtype(st);
                        bld->mutable_building_type()->set_building_custom(cust->id);
                        bld->set_id(ENUM_KEY_STR(building_type, bt) + "/" + cust->code);
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
                bld->set_id(ENUM_KEY_STR(building_type, bt) + "/" + ENUM_KEY_STR(shop_type, st));
                bld->set_name(ENUM_KEY_STR(shop_type, st));
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
                bld->set_id(ENUM_KEY_STR(building_type, bt) + "/" + ENUM_KEY_STR(workshop_type, st));
                bld->set_name(ENUM_ATTR_STR(workshop_type, name, st));

                if (st == workshop_type::Custom)
                {
                    for (size_t i = 0; i < df::global::world->raws.buildings.workshops.size(); i++)
                    {
                        auto cust = df::global::world->raws.buildings.workshops[i];

                        bld = out->add_building_list();
                        bld->mutable_building_type()->set_building_type(bt);
                        bld->mutable_building_type()->set_building_subtype(st);
                        bld->mutable_building_type()->set_building_custom(cust->id);
                        bld->set_id(ENUM_KEY_STR(building_type, bt) + "/" + cust->code);
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
                bld->set_id(ENUM_KEY_STR(building_type, bt) + "/" + ENUM_KEY_STR(siegeengine_type, st));
                bld->set_name(ENUM_KEY_STR(siegeengine_type, st));
            }
            break;
        case df::enums::building_type::Trap:
            FOR_ENUM_ITEMS(trap_type, st)
            {
                bld = out->add_building_list();
                bld->mutable_building_type()->set_building_type(bt);
                bld->mutable_building_type()->set_building_subtype(st);
                bld->mutable_building_type()->set_building_custom(-1);
                bld->set_id(ENUM_KEY_STR(building_type, bt) + "/" + ENUM_KEY_STR(trap_type, st));
                bld->set_name(ENUM_KEY_STR(trap_type, st));
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
                bld->set_id(ENUM_KEY_STR(building_type, bt) + "/" + ENUM_KEY_STR(civzone_type, st));
                bld->set_name(ENUM_KEY_STR(civzone_type, st));
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
                bld->set_id(ENUM_KEY_STR(building_type, bt) + "/" + ENUM_KEY_STR(construction_type, st));
                bld->set_name(ENUM_KEY_STR(construction_type, st));
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
    remote_build->set_index(local_build->id);
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
    if (local_build->room.width > 0 && local_build->room.height > 0 && local_build->room.extents != nullptr)
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

    //Add building-specific info
    switch (type)
    {
    case df::enums::building_type::NONE:
    {
        auto actual = virtual_cast<df::building_actual>(local_build);
        if (actual)
        {
        }
        break;
    }
    case df::enums::building_type::Chair:
    {
        auto actual = strict_virtual_cast<df::building_chairst>(local_build);
        if (actual)
        {
        }
        break;
    }
    case df::enums::building_type::Bed:
    {
        auto actual = strict_virtual_cast<df::building_bedst>(local_build);
        if (actual)
        {
        }
        break;
    }
    case df::enums::building_type::Table:
    {
        auto actual = strict_virtual_cast<df::building_tablest>(local_build);
        if (actual)
        {
        }
        break;
    }
    case df::enums::building_type::Coffin:
    {
        auto actual = strict_virtual_cast<df::building_coffinst>(local_build);
        if (actual)
        {
        }
        break;
    }
    case df::enums::building_type::FarmPlot:
    {
        auto actual = strict_virtual_cast<df::building_farmplotst>(local_build);
        if (actual)
        {
        }
        break;
    }
    case df::enums::building_type::Furnace:
    {
        auto actual = strict_virtual_cast<df::building_furnacest>(local_build);
        if (actual)
        {
        }
        break;
    }
    case df::enums::building_type::TradeDepot:
    {
        auto actual = strict_virtual_cast<df::building_tradedepotst>(local_build);
        if (actual)
        {
        }
        break;
    }
    case df::enums::building_type::Shop:
    {
        auto actual = strict_virtual_cast<df::building_shopst>(local_build);
        if (actual)
        {
        }
        break;
    }
    case df::enums::building_type::Door:
    {
        auto actual = strict_virtual_cast<df::building_doorst>(local_build);
        if (actual)
        {
            if (actual->door_flags.bits.closed)
                remote_build->set_active(1);
            else
                remote_build->set_active(0);
        }
        break;
    }
    case df::enums::building_type::Floodgate:
    {
        auto actual = strict_virtual_cast<df::building_floodgatest>(local_build);
        if (actual)
        {
            if (actual->gate_flags.bits.closed)
                remote_build->set_active(1);
            else
                remote_build->set_active(0);
        }
        break;
    }
    case df::enums::building_type::Box:
    {
        auto actual = strict_virtual_cast<df::building_boxst>(local_build);
        if (actual)
        {
        }
        break;
    }
    case df::enums::building_type::Weaponrack:
    {
        auto actual = strict_virtual_cast<df::building_weaponrackst>(local_build);
        if (actual)
        {
        }
        break;
    }
    case df::enums::building_type::Armorstand:
    {
        auto actual = strict_virtual_cast<df::building_armorstandst>(local_build);
        if (actual)
        {
        }
        break;
    }
    case df::enums::building_type::Workshop:
    {
        auto actual = strict_virtual_cast<df::building_workshopst>(local_build);
        if (actual && actual->machine.machine_id >= 0)
        {
            auto mach = df::machine::find(actual->machine.machine_id);
            remote_build->set_active(mach->flags.bits.active);
        }
        break;
    }
    case df::enums::building_type::Cabinet:
    {
        auto actual = strict_virtual_cast<df::building_cabinetst>(local_build);
        if (actual)
        {
        }
        break;
    }
    case df::enums::building_type::Statue:
    {
        auto actual = strict_virtual_cast<df::building_statuest>(local_build);
        if (actual)
        {
        }
        break;
    }
    case df::enums::building_type::WindowGlass:
    {
        auto actual = strict_virtual_cast<df::building_window_glassst>(local_build);
        if (actual)
        {
        }
        break;
    }
    case df::enums::building_type::WindowGem:
    {
        auto actual = strict_virtual_cast<df::building_window_gemst>(local_build);
        if (actual)
        {
        }
        break;
    }
    case df::enums::building_type::Well:
    {
        auto actual = strict_virtual_cast<df::building_wellst>(local_build);
        if (actual)
        {
            remote_build->set_active(actual->bucket_z);
        }
        break;
    }
    case df::enums::building_type::Bridge:
    {
        auto actual = strict_virtual_cast<df::building_bridgest>(local_build);
        if (actual)
        {
            auto direction = actual->direction;
            switch (direction)
            {
            case df::building_bridgest::Retracting:
                remote_build->set_direction(NONE);
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
            if (actual->gate_flags.bits.closed)
                remote_build->set_active(1);
            else
                remote_build->set_active(0);
        }
        break;
    }
    case df::enums::building_type::RoadDirt:
    {
        auto actual = strict_virtual_cast<df::building_road_dirtst>(local_build);
        if (actual)
        {
        }
        break;
    }
    case df::enums::building_type::RoadPaved:
    {
        auto actual = strict_virtual_cast<df::building_road_pavedst>(local_build);
        if (actual)
        {
        }
        break;
    }
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
        break;
    }
    case df::enums::building_type::Trap:
    {
        auto actual = strict_virtual_cast<df::building_trapst>(local_build);
        if (actual)
        {
            remote_build->set_active(actual->state);
        }
        break;
    }
    case df::enums::building_type::AnimalTrap:
    {
        auto actual = strict_virtual_cast<df::building_animaltrapst>(local_build);
        if (actual)
        {
        }
        break;
    }
    case df::enums::building_type::Support:
    {
        auto actual = strict_virtual_cast<df::building_supportst>(local_build);
        if (actual)
        {
            remote_build->set_active(actual->support_flags.bits.triggered);
        }
        break;
    }
    case df::enums::building_type::ArcheryTarget:
    {
        //FIXME: Need to decode archery targets again.
        auto actual = strict_virtual_cast<df::building_archerytargetst>(local_build);
        if (actual)
        {
            for (size_t i = 0; i < actual->relations.size(); i++)
            {
                if (actual->relations[i]->getType() != df::enums::building_type::Civzone)
                    continue;
                auto zone = strict_virtual_cast<df::building_civzonest>(actual->relations[i]);
                if (!zone)
                    continue;
                if (zone->type != df::civzone_type::ArcheryRange)
                    continue;
                if(zone->zone_settings.archery.dir_x < 0)
                    remote_build->set_direction(EAST);
                else if(zone->zone_settings.archery.dir_x > 0)
                    remote_build->set_direction(WEST);
                else if (zone->zone_settings.archery.dir_y < 0)
                    remote_build->set_direction(SOUTH);
                else if (zone->zone_settings.archery.dir_y > 0)
                    remote_build->set_direction(NORTH);
                break;
            }
        }
        break;
    }
    case df::enums::building_type::Chain:
    {
        auto actual = strict_virtual_cast<df::building_chainst>(local_build);
        if (actual)
        {
            remote_build->set_active(actual->chain_flags.bits.triggered);
        }
        break;
    }
    case df::enums::building_type::Cage:
    {
        auto actual = strict_virtual_cast<df::building_cagest>(local_build);
        if (actual)
        {
            remote_build->set_active(actual->cage_flags.bits.triggered);
        }
        break;
    }
    case df::enums::building_type::Stockpile:
    {
        auto actual = strict_virtual_cast<df::building_stockpilest>(local_build);
        if (actual)
        {
        }
        break;
    }
    case df::enums::building_type::Civzone:
    {
        auto actual = strict_virtual_cast<df::building_civzonest>(local_build);
        if (actual)
        {
        }
        break;
    }
    case df::enums::building_type::Weapon:
    {
        auto actual = strict_virtual_cast<df::building_weaponst>(local_build);
        if (actual)
        {
            if (actual->gate_flags.bits.closed)
                remote_build->set_active(1);
            else
                remote_build->set_active(0);
        }
        break;
    }
    case df::enums::building_type::Wagon:
    {
        auto actual = strict_virtual_cast<df::building_wagonst>(local_build);
        if (actual)
        {
        }
        break;
    }
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
            if (actual->machine.machine_id >= 0)
            {
                auto mach = df::machine::find(actual->machine.machine_id);
                remote_build->set_active(mach->flags.bits.active);
            }
        }
    }
    break;
    case df::enums::building_type::Construction:
    {
        auto actual = strict_virtual_cast<df::building_constructionst>(local_build);
        if (actual)
        {
        }
        break;
    }
    case df::enums::building_type::Hatch:
    {
        auto actual = strict_virtual_cast<df::building_hatchst>(local_build);
        if (actual)
        {
            if (actual->door_flags.bits.closed)
                remote_build->set_active(1);
            else
                remote_build->set_active(0);
        }
        break;
    }
    case df::enums::building_type::GrateWall:
    {
        auto actual = strict_virtual_cast<df::building_grate_wallst>(local_build);
        if (actual)
        {
            if (actual->gate_flags.bits.closed)
                remote_build->set_active(1);
            else
                remote_build->set_active(0);
        }
        break;
    }
    case df::enums::building_type::GrateFloor:
    {
        auto actual = strict_virtual_cast<df::building_grate_floorst>(local_build);
        if (actual)
        {
            if (actual->gate_flags.bits.closed)
                remote_build->set_active(1);
            else
                remote_build->set_active(0);
        }
        break;
    }
    case df::enums::building_type::BarsVertical:
    {
        auto actual = strict_virtual_cast<df::building_bars_verticalst>(local_build);
        if (actual)
        {
            if (actual->gate_flags.bits.closed)
                remote_build->set_active(1);
            else
                remote_build->set_active(0);
        }
        break;
    }
    case df::enums::building_type::BarsFloor:
    {
        auto actual = strict_virtual_cast<df::building_bars_floorst>(local_build);
        if (actual)
        {
            if (actual->gate_flags.bits.closed)
                remote_build->set_active(1);
            else
                remote_build->set_active(0);
        }
        break;
    }
    case df::enums::building_type::GearAssembly:
    {
        auto actual = strict_virtual_cast<df::building_gear_assemblyst>(local_build);
        if (actual)
        {
            if (actual->machine.machine_id >= 0)
            {
                auto mach = df::machine::find(actual->machine.machine_id);
                remote_build->set_active(mach->flags.bits.active);
            }
        }
        break;
    }
    case df::enums::building_type::AxleHorizontal:
    {
        auto actual = strict_virtual_cast<df::building_axle_horizontalst>(local_build);
        if (actual)
        {
            if (actual->is_vertical)
                remote_build->set_direction(NORTH);
            else
                remote_build->set_direction(EAST);
            if (actual->machine.machine_id >= 0)
            {
                auto mach = df::machine::find(actual->machine.machine_id);
                remote_build->set_active(mach->flags.bits.active);
            }
        }
    }
    break;
    case df::enums::building_type::AxleVertical:
    {
        auto actual = strict_virtual_cast<df::building_axle_verticalst>(local_build);
        if (actual)
        {
            if (actual->machine.machine_id >= 0)
            {
                auto mach = df::machine::find(actual->machine.machine_id);
                remote_build->set_active(mach->flags.bits.active);
            }
        }
        break;
    }
    case df::enums::building_type::WaterWheel:
    {
        auto actual = strict_virtual_cast<df::building_water_wheelst>(local_build);
        if (actual)
        {
            if (actual->is_vertical)
                remote_build->set_direction(NORTH);
            else
                remote_build->set_direction(EAST);
            if (actual->machine.machine_id >= 0)
            {
                auto mach = df::machine::find(actual->machine.machine_id);
                remote_build->set_active(mach->flags.bits.active);
            }
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
            if (actual->machine.machine_id >= 0)
            {
                auto mach = df::machine::find(actual->machine.machine_id);
                remote_build->set_active(mach->flags.bits.active);
            }
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
            if (actual->machine.machine_id >= 0)
            {
                auto mach = df::machine::find(actual->machine.machine_id);
                remote_build->set_active(mach->flags.bits.active);
            }
        }
    }
    break;
    default:
        break;
    }
}
