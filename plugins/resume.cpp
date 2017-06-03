#include <string>
#include <vector>
#include <algorithm>

#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>
#include <VTableInterpose.h>


// DF data structure definition headers
#include "DataDefs.h"
#include "MiscUtils.h"
#include "Types.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/world.h"
#include "df/building_constructionst.h"
#include "df/building.h"
#include "df/job.h"
#include "df/job_item.h"

#include "modules/Gui.h"
#include "modules/Screen.h"
#include "modules/Buildings.h"
#include "modules/Maps.h"

#include "modules/World.h"

#include "uicommon.h"

using std::map;
using std::string;
using std::vector;

using namespace DFHack;
using namespace df::enums;

DFHACK_PLUGIN("resume");
#define PLUGIN_VERSION 0.2

REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(world);

#ifndef HAVE_NULLPTR
#define nullptr 0L
#endif

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

df::job *get_suspended_job(df::building *bld)
{
    if (bld->getBuildStage() != 0)
        return nullptr;

    if (bld->jobs.size() == 0)
        return nullptr;

    auto job = bld->jobs[0];
    if (job->flags.bits.suspend)
        return job;

    return nullptr;
}

struct SuspendedBuilding
{
    df::building *bld;
    df::coord pos;
    bool was_resumed;
    bool is_planned;

    SuspendedBuilding(df::building *bld_) : bld(bld_), was_resumed(false), is_planned(false)
    {
        pos = df::coord(bld->centerx, bld->centery, bld->z);
    }

    bool isValid()
    {
        return bld && Buildings::findAtTile(pos) == bld && get_suspended_job(bld);
    }
};

DFHACK_PLUGIN_IS_ENABLED(enabled);
static bool buildings_scanned = false;
static vector<SuspendedBuilding> suspended_buildings, resumed_buildings;

void scan_for_suspended_buildings()
{
    if (buildings_scanned)
        return;

    for (auto b = world->buildings.all.begin(); b != world->buildings.all.end(); b++)
    {
        auto bld = *b;
        auto job = get_suspended_job(bld);
        if (job)
        {
            SuspendedBuilding sb(bld);
            sb.is_planned = job->job_items.size() == 1 && job->job_items[0]->item_type == item_type::NONE;

            auto it = resumed_buildings.begin();

            for (; it != resumed_buildings.end(); ++it)
                if (it->bld == bld) break;

            sb.was_resumed = it != resumed_buildings.end();

            suspended_buildings.push_back(sb);
        }
    }

    buildings_scanned = true;
}

void show_suspended_buildings()
{
    int32_t vx, vy, vz;
    if (!Gui::getViewCoords(vx, vy, vz))
        return;

    auto dims = Gui::getDwarfmodeViewDims();
    int left_margin = vx + dims.map_x2;
    int bottom_margin = vy + dims.map_y2 - 1;

    for (auto sb = suspended_buildings.begin(); sb != suspended_buildings.end();)
    {
        if (!sb->isValid())
        {
            sb = suspended_buildings.erase(sb);
            continue;
        }

        if (sb->bld->z == vz && sb->bld->centerx >= vx && sb->bld->centerx <= left_margin &&
            sb->bld->centery >= vy && sb->bld->centery <= bottom_margin)
        {
            int x = sb->bld->centerx - vx + 1;
            int y = sb->bld->centery - vy + 1;
            auto color = COLOR_YELLOW;
            if (sb->is_planned)
                color = COLOR_GREEN;
            else if (sb->was_resumed)
                color = COLOR_RED;

            OutputString(color, x, y, "X", false, 0, 0, true /* map */);
        }

        sb++;
    }
}

void clear_scanned()
{
    buildings_scanned = false;
    suspended_buildings.clear();
}

void resume_suspended_buildings(color_ostream &out)
{
    out << "Resuming all buildings." << endl;

    for (auto isb = resumed_buildings.begin(); isb != resumed_buildings.end();)
    {
        if (isb->isValid())
        {
            isb++;
            continue;
        }

        isb = resumed_buildings.erase(isb);
    }

    scan_for_suspended_buildings();
    for (auto sb = suspended_buildings.begin(); sb != suspended_buildings.end(); sb++)
    {
        if (sb->is_planned)
            continue;

        resumed_buildings.push_back(*sb);
        sb->bld->jobs[0]->flags.bits.suspend = false;
    }

    clear_scanned();

    out << resumed_buildings.size() << " buildings resumed" << endl;
}


//START Viewscreen Hook
struct resume_hook : public df::viewscreen_dwarfmodest
{
    //START UI Methods
    typedef df::viewscreen_dwarfmodest interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        INTERPOSE_NEXT(render)();

        if (enabled && DFHack::World::ReadPauseState() && ui->main.mode == ui_sidebar_mode::Default)
        {
            scan_for_suspended_buildings();
            show_suspended_buildings();
        }
        else
        {
            clear_scanned();
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(resume_hook, render);

DFhackCExport command_result plugin_enable ( color_ostream &out, bool enable)
{
    if (!gps)
        return CR_FAILURE;

    if (enabled != enable)
    {
        clear_scanned();

        if (!INTERPOSE_HOOK(resume_hook, render).apply(enable))
            return CR_FAILURE;

        enabled = enable;
    }

    return CR_OK;
}

static command_result resume_cmd(color_ostream &out, vector <string> & parameters)
{
    bool show_help = false;
    if (parameters.empty())
    {
        show_help = true;
    }
    else
    {
        auto cmd = parameters[0][0];
        if (cmd == 'v')
        {
            out << "Resume" << endl << "Version: " << PLUGIN_VERSION << endl;
        }
        else if (cmd == 's')
        {
            plugin_enable(out, true);
            out << "Overlay enabled" << endl;
        }
        else if (cmd == 'h')
        {
            plugin_enable(out, false);
            out << "Overlay disabled" << endl;
        }
        else if (cmd == 'a')
        {
            resume_suspended_buildings(out);
        }
        else
        {
            show_help = true;
        }
    }

    if (show_help)
        return CR_WRONG_USAGE;

    return CR_OK;
}

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(
        PluginCommand(
        "resume", "Display and easily resume suspended constructions",
        resume_cmd, false,
        "resume show\n"
        "  Show overlay when paused:\n"
        "    Yellow: Suspended construction\n"
        "    Red: Suspended after resume attempt, possibly stuck\n"
        "    Green: Planned building waiting for materials\n"
        "resume hide\n"
        "  Hide overlay\n"
        "resume all\n"
        "  Resume all suspended building constructions\n"
        ));

    return CR_OK;
}


DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        suspended_buildings.clear();
        resumed_buildings.clear();
        break;
    default:
        break;
    }

    return CR_OK;
}
