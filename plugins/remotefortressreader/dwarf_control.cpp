#include "dwarf_control.h"
#include "DataDefs.h"
#include "df_version_int.h"

#include "df/job.h"
#include "df/job_list_link.h"
#include "df/ui.h"
#include "df/world.h"

#include "modules/Buildings.h"
#include "modules/Job.h"
#include "modules/MapCache.h"
#include "modules/Maps.h"
#include "modules/World.h"

using namespace DFHack;
using namespace RemoteFortressReader;


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
            case df::enums::job_type::CarveFortification:
            case df::enums::job_type::DetailWall:
            case df::enums::job_type::DetailFloor:
            case df::enums::job_type::Dig:
            case df::enums::job_type::CarveUpwardStaircase:
            case df::enums::job_type::CarveDownwardStaircase:
            case df::enums::job_type::CarveUpDownStaircase:
            case df::enums::job_type::CarveRamp:
            case df::enums::job_type::DigChannel:
            case df::enums::job_type::FellTree:
            case df::enums::job_type::GatherPlants:
            case df::enums::job_type::RemoveConstruction:
            case df::enums::job_type::CarveTrack:
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

command_result GetSideMenu(DFHack::color_ostream &stream, const dfproto::EmptyMessage *in, DwarfControl::SidebarState *out)
{
    auto ui = df::global::ui;
    out->set_mode((proto::enums::ui_sidebar_mode::ui_sidebar_mode)ui->main.mode);
    return CR_OK;
}
