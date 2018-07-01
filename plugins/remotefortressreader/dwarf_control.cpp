#include "dwarf_control.h"
#include "DataDefs.h"
#include "df_version_int.h"

#include "df/build_req_choice_genst.h"
#include "df/build_req_choice_specst.h"
#include "df/build_req_choicest.h"
#include "df/job.h"
#include "df/job_list_link.h"
#include "df/interface_button_construction_building_selectorst.h"
#include "df/interface_button_construction_category_selectorst.h"
#include "df/ui.h"
#include "df/ui_build_selector.h"
#include "df/ui_sidebar_menus.h"
#include "df/viewscreen.h"
#include "df/world.h"

#include "modules/Buildings.h"
#include "modules/Gui.h"
#include "modules/Job.h"
#include "modules/MapCache.h"
#include "modules/Maps.h"
#include "modules/World.h"

#include <queue>

using namespace DFHack;
using namespace RemoteFortressReader;
using namespace df::enums;
using namespace Gui;

extern std::queue<interface_key::interface_key> keyQueue;

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
    auto menus = df::global::ui_sidebar_menus;
    auto build_selector = df::global::ui_build_selector;
    if (build_selector->building_type == -1)
        for (int i = 0; i < menus->building.choices_visible.size(); i++)
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
        for (int i = 0; i < build_selector->errors.size(); i++)
        {
            if (build_selector->errors[i])
                send_selector->add_errors(*build_selector->errors[i]);
        }
        for (int i = 0; i < build_selector->choices.size(); i++)
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
    }
}

command_result GetSideMenu(DFHack::color_ostream &stream, const dfproto::EmptyMessage *in, DwarfControl::SidebarState *out)
{
    auto ui = df::global::ui;
    out->set_mode((proto::enums::ui_sidebar_mode::ui_sidebar_mode)ui->main.mode);
    auto mode = ui->main.mode;
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
    return CR_OK;
}

command_result SetSideMenu(DFHack::color_ostream &stream, const DwarfControl::SidebarCommand *in)
{
    auto ui = df::global::ui;
    if (in->has_mode())
    {
        ui_sidebar_mode::ui_sidebar_mode set_mode = (ui_sidebar_mode::ui_sidebar_mode)in->mode();
        if (ui->main.mode != set_mode)
        {
            ui->main.mode = ui_sidebar_mode::Default;
            switch (set_mode)
            {
            case ui_sidebar_mode::Build:
                keyQueue.push(interface_key::D_BUILDING);
                break;
            default:
                ui->main.mode = set_mode;
                break;
            }
        }
    }
    switch (ui->main.mode)
    {
    case ui_sidebar_mode::Build:
        if (in->has_action())
        {
            if (in->has_menu_index())
                df::global::ui_sidebar_menus->building.cursor = in->menu_index();
            else
                df::global::ui_sidebar_menus->building.cursor = 0;
            break;
        }
    default:
        break;
    }
    auto viewScreen = getCurViewscreen();
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
    return CR_OK;
}
