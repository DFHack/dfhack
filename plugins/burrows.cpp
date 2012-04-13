#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/Gui.h"
#include "modules/Job.h"
#include "modules/Maps.h"
#include "modules/MapCache.h"
#include "modules/World.h"
#include "TileTypes.h"

#include "DataDefs.h"
#include "df/ui.h"
#include "df/world.h"
#include "df/unit.h"
#include "df/burrow.h"
#include "df/map_block.h"
#include "df/job.h"
#include "df/job_list_link.h"

#include "MiscUtils.h"

#include <stdlib.h>

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;
using namespace df::enums;
using namespace dfproto;

using df::global::ui;
using df::global::world;

/*
 * Initialization.
 */

static command_result burrow(color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("burrows");

static void init_map(color_ostream &out);
static void deinit_map(color_ostream &out);

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "burrow", "Miscellaneous burrow control.", burrow, false,
        "  burrow enable options...\n"
        "  burrow disable options...\n"
        "    Enable or disable features of the plugin.\n"
        "    See below for a list and explanation.\n"
        "Implemented features:\n"
        "  auto-grow\n"
        "    When a wall inside a burrow with a name ending in '+' is dug\n"
        "    out, the burrow is extended to newly-revealed adjacent walls.\n"
        "    Digging 1-wide corridors with the miner inside the burrow is SLOW.\n"
    ));

    if (Core::getInstance().isMapLoaded())
        init_map(out);

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    deinit_map(out);

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        deinit_map(out);
        if (df::global::game_mode &&
            *df::global::game_mode == GAMEMODE_DWARF)
            init_map(out);
        break;
    case SC_MAP_UNLOADED:
        deinit_map(out);
        break;
    default:
        break;
    }

    return CR_OK;
}

/*
 * State change tracking.
 */

static int name_burrow_id = -1;

static void handle_burrow_rename(color_ostream &out, df::burrow *burrow);

static void detect_burrow_renames(color_ostream &out)
{
    if (ui->main.mode == ui_sidebar_mode::Burrows &&
        ui->burrows.in_edit_name_mode &&
        ui->burrows.sel_id >= 0)
    {
        name_burrow_id = ui->burrows.sel_id;
    }
    else if (name_burrow_id >= 0)
    {
        auto burrow = df::burrow::find(name_burrow_id);
        name_burrow_id = -1;
        if (burrow)
            handle_burrow_rename(out, burrow);
    }
}

struct DigJob {
    int id;
    df::job_type job;
    df::coord pos;
    df::tiletype old_tile;
};

static int next_job_id_save = 0;
static std::map<int,DigJob> diggers;

static void handle_dig_complete(color_ostream &out, df::job_type job, df::coord pos,
                                df::tiletype old_tile, df::tiletype new_tile);

static void detect_digging(color_ostream &out)
{
    for (auto it = diggers.begin(); it != diggers.end();)
    {
        auto worker = df::unit::find(it->first);

        if (!worker || !worker->job.current_job ||
            worker->job.current_job->id != it->second.id)
        {
            //out.print("Dig job %d expired.\n", it->second.id);

            df::coord pos = it->second.pos;

            if (auto block = Maps::getTileBlock(pos))
            {
                df::tiletype new_tile = block->tiletype[pos.x&15][pos.y&15];

                //out.print("Tile %d -> %d\n", it->second.old_tile, new_tile);

                if (new_tile != it->second.old_tile)
                {
                    handle_dig_complete(out, it->second.job, pos, it->second.old_tile, new_tile);

                    //if (worker && !worker->job.current_job)
                    //    worker->counters.think_counter = worker->counters.job_counter = 0;
                }
            }

            auto cur = it; ++it; diggers.erase(cur);
        }
        else
            ++it;
    }

    std::vector<df::job*> jvec;

    if (Job::listNewlyCreated(&jvec, &next_job_id_save))
    {
        for (size_t i = 0; i < jvec.size(); i++)
        {
            auto job = jvec[i];
            auto type = ENUM_ATTR(job_type, type, job->job_type);
            if (type != job_type_class::Digging)
                continue;

            auto worker = Job::getWorker(job);
            if (!worker)
                continue;

            df::coord pos = job->pos;
            auto block = Maps::getTileBlock(pos);
            if (!block)
                continue;

            auto &info = diggers[worker->id];

            //out.print("New dig job %d.\n", job->id);

            info.id = job->id;
            info.job = job->job_type;
            info.pos = pos;
            info.old_tile = block->tiletype[pos.x&15][pos.y&15];
        }
    }
}

static bool active = false;
static bool auto_grow = false;
static std::vector<int> grow_burrows;

DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
    if (!active || !auto_grow)
        return CR_OK;

    detect_burrow_renames(out);
    detect_digging(out);
    return CR_OK;
}

/*
 * Config and processing
 */

static void parse_names()
{
    auto &list = ui->burrows.list;

    grow_burrows.clear();

    for (size_t i = 0; i < list.size(); i++)
    {
        auto burrow = list[i];

        if (!burrow->name.empty() && burrow->name[burrow->name.size()-1] == '+')
            grow_burrows.push_back(burrow->id);
    }
}

static void reset_tracking()
{
    name_burrow_id = -1;
    diggers.clear();
    next_job_id_save = 0;
}

static void init_map(color_ostream &out)
{
    auto config = Core::getInstance().getWorld()->GetPersistentData("burrows/config");
    if (config.isValid())
    {
        auto_grow = !!(config.ival(0) & 1);
    }

    parse_names();
    reset_tracking();
    active = true;

    if (auto_grow && !grow_burrows.empty())
        out.print("Auto-growing %d burrows.\n", grow_burrows.size());
}

static void deinit_map(color_ostream &out)
{
    active = false;
    auto_grow = false;
    reset_tracking();
}

static PersistentDataItem create_config(color_ostream &out)
{
    bool created;
    auto rv = Core::getInstance().getWorld()->GetPersistentData("burrows/config", &created);
    if (created && rv.isValid())
        rv.ival(0) = 0;
    if (!rv.isValid())
        out.printerr("Could not write configuration.");
    return rv;
}

static void enable_auto_grow(color_ostream &out, bool enable)
{
    if (enable == auto_grow)
        return;

    auto config = create_config(out);
    if (!config.isValid())
        return;

    if (enable)
        config.ival(0) |= 1;
    else
        config.ival(0) &= ~1;

    auto_grow = enable;

    if (enable)
    {
        parse_names();
        reset_tracking();
    }
}

static void handle_burrow_rename(color_ostream &out, df::burrow *burrow)
{
    parse_names();
}

static void add_to_burrows(std::vector<df::burrow*> &burrows, df::coord pos)
{
    for (size_t i = 0; i < burrows.size(); i++)
        Maps::setBurrowTile(burrows[i], pos, true);
}

static void add_walls_to_burrows(color_ostream &out, std::vector<df::burrow*> &burrows,
                                MapExtras::MapCache &mc, df::coord pos1, df::coord pos2)
{
    for (int x = pos1.x; x <= pos2.x; x++)
    {
        for (int y = pos1.y; y <= pos2.y; y++)
        {
            for (int z = pos1.z; z <= pos2.z; z++)
            {
                df::coord pos(x,y,z);

                auto tile = mc.tiletypeAt(pos);

                if (isWallTerrain(tile))
                    add_to_burrows(burrows, pos);
            }
        }
    }
}

static void handle_dig_complete(color_ostream &out, df::job_type job, df::coord pos,
                                df::tiletype old_tile, df::tiletype new_tile)
{
    if (!isWalkable(new_tile))
        return;

    std::vector<df::burrow*> to_grow;

    for (size_t i = 0; i < grow_burrows.size(); i++)
    {
        auto b = df::burrow::find(grow_burrows[i]);
        if (b && Maps::isBurrowTile(b, pos))
            to_grow.push_back(b);
    }

    //out.print("%d to grow.\n", to_grow.size());

    if (to_grow.empty())
        return;

    MapExtras::MapCache mc;

    if (!isWalkable(old_tile))
    {
        add_walls_to_burrows(out, to_grow, mc, pos+df::coord(-1,-1,0), pos+df::coord(1,1,0));

        if (isWalkableUp(new_tile))
            add_to_burrows(to_grow, pos+df::coord(0,0,1));

        if (tileShape(new_tile) == tiletype_shape::RAMP)
        {
            add_walls_to_burrows(out, to_grow, mc,
                                 pos+df::coord(-1,-1,1), pos+df::coord(1,1,1));
        }
    }

    if (LowPassable(new_tile) && !LowPassable(old_tile))
    {
        add_to_burrows(to_grow, pos-df::coord(0,0,1));

        if (tileShape(new_tile) == tiletype_shape::RAMP_TOP)
        {
            add_walls_to_burrows(out, to_grow, mc,
                                 pos+df::coord(-1,-1,-1), pos+df::coord(1,1,-1));
        }
    }
}

static command_result burrow(color_ostream &out, vector <string> &parameters)
{
    CoreSuspender suspend;

    if (!active)
    {
        out.printerr("The plugin cannot be used without map.\n");
        return CR_FAILURE;
    }

    string cmd;
    if (!parameters.empty())
        cmd = parameters[0];

    if (cmd == "enable" || cmd == "disable")
    {
        if (parameters.size() < 2)
            return CR_WRONG_USAGE;

        bool state = (cmd == "enable");

        for (int i = 1; i < parameters.size(); i++)
        {
            string &option = parameters[i];

            if (option == "auto-grow")
                enable_auto_grow(out, state);
            else
                return CR_WRONG_USAGE;
        }
    }
    else
    {
        if (!parameters.empty() && cmd != "?")
            out.printerr("Invalid command: %s\n", cmd.c_str());
        return CR_WRONG_USAGE;
    }

    return CR_OK;
}
