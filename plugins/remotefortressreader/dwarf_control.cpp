
#include "dwarf_control.h"
#include "DataDefs.h"
#include "df_version_int.h"

#include "df/build_req_choice_genst.h"
#include "df/build_req_choice_specst.h"
#include "df/build_req_choicest.h"
#include "df/building_def.h"
#include "df/building_def_furnacest.h"
#include "df/building_def_workshopst.h"
#include "df/job.h"
#include "df/job_list_link.h"
#include "df/plotinfost.h"
#include "df/buildreq.h"
#include "df/gamest.h"
#include "df/viewscreen.h"
#include "df/world.h"

#include "modules/Buildings.h"
#include "modules/Gui.h"
#include "modules/Job.h"
#include "modules/MapCache.h"
#include "modules/Maps.h"
#include "modules/World.h"

#include "MiscUtils.h"

#include <queue>

using namespace DFHack;
using namespace RemoteFortressReader;
using namespace df::enums;
using namespace Gui;
using namespace df::global;

extern std::queue<interface_key::interface_key> keyQueue;

void GetBuildingSize(
    int16_t type,
    int16_t subtype,
    int16_t custom,
    int16_t &rad_x_low,
    int16_t &rad_y_low,
    int16_t &rad_x_high,
    int16_t &rad_y_high
)
{
    rad_x_low = 0;
    rad_y_low = 0;
    rad_x_high = 0;
    rad_y_high = 0;
    df::building_def* customBuilding = 0;
    switch (type)
    {
    case building_type::FarmPlot:
    case building_type::Bridge:
    case building_type::RoadDirt:
    case building_type::RoadPaved:
    case building_type::Stockpile:
    case building_type::Civzone:
    case building_type::ScrewPump:
    case building_type::Construction:
    case building_type::AxleHorizontal:
    case building_type::WaterWheel:
    case building_type::Rollers:
    {
        bool widthOdd = world->building_width % 2;
        rad_x_low = world->building_width / 2;
        if(widthOdd)
            rad_x_high = world->building_width / 2;
        else
            rad_x_high = (world->building_width / 2) - 1;

        bool heightOdd = world->building_height % 2;
        rad_y_low = world->building_height / 2;
        if (heightOdd)
            rad_y_high = world->building_height / 2;
        else
            rad_y_high = (world->building_height / 2) - 1;
        }
        return;
    case building_type::Furnace:
        if (subtype != furnace_type::Custom)
        {
            rad_x_low = rad_y_low = rad_x_high = rad_y_high = 1;
            return;
        }
        customBuilding = world->raws.buildings.furnaces[custom];
        break;
    case building_type::TradeDepot:
    case building_type::Shop:
        rad_x_low = rad_y_low = rad_x_high = rad_y_high = 2;
        return;
    case building_type::Workshop:
        switch (subtype)
        {
        case workshop_type::Carpenters:
        case workshop_type::Farmers:
        case workshop_type::Masons:
        case workshop_type::Craftsdwarfs:
        case workshop_type::Jewelers:
        case workshop_type::MetalsmithsForge:
        case workshop_type::MagmaForge:
        case workshop_type::Bowyers:
        case workshop_type::Mechanics:
        case workshop_type::Butchers:
        case workshop_type::Leatherworks:
        case workshop_type::Tanners:
        case workshop_type::Clothiers:
        case workshop_type::Fishery:
        case workshop_type::Still:
        case workshop_type::Loom:
        case workshop_type::Kitchen:
        case workshop_type::Ashery:
        case workshop_type::Dyers:
            rad_x_low = rad_y_low = rad_x_high = rad_y_high = 1;
            return;
        case workshop_type::Siege:
        case workshop_type::Kennels:
            rad_x_low = rad_y_low = rad_x_high = rad_y_high = 2;
            return;
        case workshop_type::Custom:
            customBuilding = world->raws.buildings.workshops[custom];
            break;
        default:
            return;
        }
        break;
    case building_type::SiegeEngine:
    case building_type::Wagon:
    case building_type::Windmill:
        rad_x_low = rad_y_low = rad_x_high = rad_y_high = 1;
        return;
    default:
        return;
    }
    if (customBuilding)
    {
        rad_x_low = customBuilding->workloc_x;
        rad_y_low = customBuilding->workloc_y;
        rad_x_high = customBuilding->dim_x - rad_x_low - 1;
        rad_y_high = customBuilding->dim_y - rad_y_low - 1;
        return;
    }
}

command_result SendDigCommand(color_ostream &stream, const DigCommand *in)
{
    MapExtras::MapCache mc;

    for (int i = 0; i < in->locations_size(); i++)
    {
        auto pos = in->locations(i);
        auto des = mc.designationAt(DFCoord(pos.x(), pos.y(), pos.z()));
        switch (in->designation())
        {
        case NO_DIG:
            des.bits.dig = tile_dig_designation::No;
            break;
        case DEFAULT_DIG:
            des.bits.dig = tile_dig_designation::Default;
            break;
        case UP_DOWN_STAIR_DIG:
            des.bits.dig = tile_dig_designation::UpDownStair;
            break;
        case CHANNEL_DIG:
            des.bits.dig = tile_dig_designation::Channel;
            break;
        case RAMP_DIG:
            des.bits.dig = tile_dig_designation::Ramp;
            break;
        case DOWN_STAIR_DIG:
            des.bits.dig = tile_dig_designation::DownStair;
            break;
        case UP_STAIR_DIG:
            des.bits.dig = tile_dig_designation::UpStair;
            break;
        default:
            break;
        }
        mc.setDesignationAt(DFCoord(pos.x(), pos.y(), pos.z()), des);

#if DF_VERSION_INT >= 43005
        //remove and job postings related.
        for (df::job_list_link * listing = &(df::global::world->jobs.list); listing != NULL; listing = listing->next)
        {
            if (listing->item == NULL)
                continue;
            auto type = listing->item->job_type;
            switch (type)
            {
            case job_type::CarveFortification:
            case job_type::DetailWall:
            case job_type::DetailFloor:
            case job_type::Dig:
            case job_type::CarveUpwardStaircase:
            case job_type::CarveDownwardStaircase:
            case job_type::CarveUpDownStaircase:
            case job_type::CarveRamp:
            case job_type::DigChannel:
            case job_type::FellTree:
            case job_type::GatherPlants:
            case job_type::RemoveConstruction:
            case job_type::CarveTrack:
            {
                if (listing->item->pos == DFCoord(pos.x(), pos.y(), pos.z()))
                {
                    Job::removeJob(listing->item);
                    goto JOB_FOUND;
                }
                break;
            }
            default:
                continue;
            }
        }
    JOB_FOUND:
        continue;
#endif
    }

    mc.WriteAll();
    return CR_OK;
}

command_result SetPauseState(color_ostream &stream, const SingleBool *in)
{
    DFHack::World::SetPauseState(in->value());
    return CR_OK;
}

void CopyBuildMenu(DwarfControl::SidebarState * out)
{
    /* Removed in v50 because the new menu is completely changed and we may not even want to do it the same way anymore.
    auto menus = df::global::game;
    auto build_selector = df::global::ui_build_selector;
    if (build_selector->building_type == -1)
        for (size_t i = 0; i < menus->building.choices_visible.size(); i++)
        {
            auto menu_item = menus->building.choices_visible[i];
            auto send_item = out->add_menu_items();
            STRICT_VIRTUAL_CAST_VAR(building, df::interface_button_construction_building_selectorst, menu_item);
            if (building)
            {
                auto send_bld = send_item->mutable_building_type();
                send_bld->set_building_type(building->building_type);
                send_bld->set_building_subtype(building->building_subtype);
                send_bld->set_building_custom(building->custom_type);
                send_item->set_existing_count(building->existing_count);
            }
            STRICT_VIRTUAL_CAST_VAR(sub_category, df::interface_button_construction_category_selectorst, menu_item);
            if (sub_category)
            {
                send_item->set_build_category((DwarfControl::BuildCategory)sub_category->category_id);
            }
        }
    else
    {
        auto send_selector = out->mutable_build_selector();
        auto send_bld = send_selector->mutable_building_type();
        send_bld->set_building_type(build_selector->building_type);
        send_bld->set_building_subtype(build_selector->building_subtype);
        send_bld->set_building_custom(build_selector->custom_type);
        send_selector->set_stage((DwarfControl::BuildSelectorStage)build_selector->stage);
        for (size_t i = 0; i < build_selector->errors.size(); i++)
        {
            if (build_selector->errors[i])
                send_selector->add_errors(*build_selector->errors[i]);
        }
        for (size_t i = 0; i < build_selector->choices.size(); i++)
        {
            auto choice = build_selector->choices[i];
            auto send_choice = send_selector->add_choices();
            send_choice->set_distance(choice->distance);
            std::string name;
            choice->getName(&name);
            send_choice->set_name(name);
            send_choice->set_num_candidates(choice->getNumCandidates());
            send_choice->set_used_count(choice->getUsedCount());
        }
        int16_t x_low, y_low, x_high, y_high;
        GetBuildingSize(build_selector->building_type, build_selector->building_subtype, build_selector->custom_type, x_low, y_low, x_high, y_high);
        send_selector->set_radius_x_low(x_low);
        send_selector->set_radius_y_low(y_low);
        send_selector->set_radius_x_high(x_high);
        send_selector->set_radius_y_high(y_high);
        if (build_selector->stage >= 1)
        {
            auto send_cursor = send_selector->mutable_cursor();
            send_cursor->set_x(cursor->x);
            send_cursor->set_y(cursor->y);
            send_cursor->set_z(cursor->z);
        }

        for (int y = 0; y < (y_low + y_high + 1); y++)
            for (int x = 0; x < (x_low + x_high + 1); x++)
            {
                send_selector->add_tiles(build_selector->tiles[x][y]);
            }
    }
    */
}

command_result GetSideMenu(DFHack::color_ostream &stream, const dfproto::EmptyMessage *in, DwarfControl::SidebarState *out)
{
    /* Removed in v50 because the new menu is completely changed and we may not even want to do it the same way anymore.
    auto plotinfo = df::global::plotinfo;
    out->set_mode((proto::enums::ui_sidebar_mode::ui_sidebar_mode)plotinfo->main.mode);
    auto mode = plotinfo->main.mode;
    switch (mode)
    {
    case ui_sidebar_mode::Default:
        break;
    case ui_sidebar_mode::Squads:
        break;
    case ui_sidebar_mode::DesignateMine:
        break;
    case ui_sidebar_mode::DesignateRemoveRamps:
        break;
    case ui_sidebar_mode::DesignateUpStair:
        break;
    case ui_sidebar_mode::DesignateDownStair:
        break;
    case ui_sidebar_mode::DesignateUpDownStair:
        break;
    case ui_sidebar_mode::DesignateUpRamp:
        break;
    case ui_sidebar_mode::DesignateChannel:
        break;
    case ui_sidebar_mode::DesignateGatherPlants:
        break;
    case ui_sidebar_mode::DesignateRemoveDesignation:
        break;
    case ui_sidebar_mode::DesignateSmooth:
        break;
    case ui_sidebar_mode::DesignateCarveTrack:
        break;
    case ui_sidebar_mode::DesignateEngrave:
        break;
    case ui_sidebar_mode::DesignateCarveFortification:
        break;
    case ui_sidebar_mode::Stockpiles:
        break;
    case ui_sidebar_mode::Build:
        CopyBuildMenu(out);
        break;
    case ui_sidebar_mode::QueryBuilding:
        break;
    case ui_sidebar_mode::Orders:
        break;
    case ui_sidebar_mode::OrdersForbid:
        break;
    case ui_sidebar_mode::OrdersRefuse:
        break;
    case ui_sidebar_mode::OrdersWorkshop:
        break;
    case ui_sidebar_mode::OrdersZone:
        break;
    case ui_sidebar_mode::BuildingItems:
        break;
    case ui_sidebar_mode::ViewUnits:
        break;
    case ui_sidebar_mode::LookAround:
        break;
    case ui_sidebar_mode::DesignateItemsClaim:
        break;
    case ui_sidebar_mode::DesignateItemsForbid:
        break;
    case ui_sidebar_mode::DesignateItemsMelt:
        break;
    case ui_sidebar_mode::DesignateItemsUnmelt:
        break;
    case ui_sidebar_mode::DesignateItemsDump:
        break;
    case ui_sidebar_mode::DesignateItemsUndump:
        break;
    case ui_sidebar_mode::DesignateItemsHide:
        break;
    case ui_sidebar_mode::DesignateItemsUnhide:
        break;
    case ui_sidebar_mode::DesignateChopTrees:
        break;
    case ui_sidebar_mode::DesignateToggleEngravings:
        break;
    case ui_sidebar_mode::DesignateToggleMarker:
        break;
    case ui_sidebar_mode::Hotkeys:
        break;
    case ui_sidebar_mode::DesignateTrafficHigh:
        break;
    case ui_sidebar_mode::DesignateTrafficNormal:
        break;
    case ui_sidebar_mode::DesignateTrafficLow:
        break;
    case ui_sidebar_mode::DesignateTrafficRestricted:
        break;
    case ui_sidebar_mode::Zones:
        break;
    case ui_sidebar_mode::ZonesPenInfo:
        break;
    case ui_sidebar_mode::ZonesPitInfo:
        break;
    case ui_sidebar_mode::ZonesHospitalInfo:
        break;
    case ui_sidebar_mode::ZonesGatherInfo:
        break;
    case ui_sidebar_mode::DesignateRemoveConstruction:
        break;
    case ui_sidebar_mode::DepotAccess:
        break;
    case ui_sidebar_mode::NotesPoints:
        break;
    case ui_sidebar_mode::NotesRoutes:
        break;
    case ui_sidebar_mode::Burrows:
        break;
    case ui_sidebar_mode::Hauling:
        break;
    case ui_sidebar_mode::ArenaWeather:
        break;
    case ui_sidebar_mode::ArenaTrees:
        break;
    default:
        break;
    }
    */
    return CR_OK;
}

command_result SetSideMenu(DFHack::color_ostream &stream, const DwarfControl::SidebarCommand *in)
{
    /* Removed in v50 because the new menu is completely changed and we may not even want to do it the same way anymore.
    auto plotinfo = df::global::plotinfo;
    if (in->has_mode())
    {
        ui_sidebar_mode::ui_sidebar_mode set_mode = (ui_sidebar_mode::ui_sidebar_mode)in->mode();
        if (plotinfo->main.mode != set_mode)
        {
            plotinfo->main.mode = ui_sidebar_mode::Default;
            switch (set_mode)
            {
            case ui_sidebar_mode::Build:
                keyQueue.push(interface_key::D_BUILDING);
                break;
            default:
                plotinfo->main.mode = set_mode;
                break;
            }
        }
    }
    switch (plotinfo->main.mode)
    {
    case ui_sidebar_mode::Build:
        if (in->has_action())
        {
            int index = 0;
            if (in->has_menu_index())
                index = in->menu_index();
            if(ui_build_selector->building_type == -1)
                df::global::game->building.cursor = index;
            if (ui_build_selector->stage == 2)
            {
                ui_build_selector->sel_index = index;
            }
        }
        if (ui_build_selector->stage == 1)
        {
            if (in->has_selection_coord())
            {
                df::global::cursor->x = in->selection_coord().x();
                df::global::cursor->y = in->selection_coord().y();
                df::global::cursor->z = in->selection_coord().z();
                getCurViewscreen()->feed_key(interface_key::CURSOR_LEFT);
                getCurViewscreen()->feed_key(interface_key::CURSOR_RIGHT);
            }
        }
        break;
    default:
        break;
    }
    if (in->has_action())
    {
        switch (in->action())
        {
        case DwarfControl::MenuSelect:
            keyQueue.push(interface_key::SELECT);
            break;
        case DwarfControl::MenuCancel:
            keyQueue.push(interface_key::LEAVESCREEN);
            break;
        default:
            break;
        }
    }
    */
    return CR_OK;
}
