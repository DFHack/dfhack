#include "Core.h"
#include "Console.h"
#include "DataDefs.h"
#include "DataFuncs.h"
#include "Error.h"
#include "Export.h"
#include "LuaTools.h"
#include "PluginManager.h"

#include "modules/Burrows.h"
#include "modules/EventManager.h"
#include "modules/Gui.h"
#include "modules/Job.h"
#include "modules/MapCache.h"
#include "modules/Maps.h"
#include "modules/Persistent.h"
#include "modules/Units.h"
#include "modules/World.h"

#include "df/block_burrow.h"
#include "df/burrow.h"
#include "df/job.h"
#include "df/job_list_link.h"
#include "df/map_block.h"
#include "df/ui.h"
#include "df/unit.h"
#include "df/world.h"

#include "MiscUtils.h"
#include "TileTypes.h"

#include <iostream>
#include <stdlib.h>

#include "jsoncpp.h"

using std::vector;
using std::string;
using std::endl;
using std::cerr;
using namespace DFHack;
using namespace df::enums;
using namespace dfproto;

DFHACK_PLUGIN("burrows");
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(gamemode);

DFHACK_PLUGIN_IS_ENABLED(active);

static const int32_t persist_version=1;
static void load_config(color_ostream& out);
static void save_config(color_ostream& out);
static void on_presave_callback(color_ostream& out, void* nothing) {
    save_config(out);
}

static bool auto_grow = false;
static std::vector<int> grow_burrows;

/*
 * State change tracking.
 */

static int name_burrow_id = -1;

/*
 * Config and processing
 */

static std::map<std::string,int> name_lookup;

/*
 * Initialization.
 */

static command_result burrow(color_ostream &out, vector <string> & parameters);

static void init_map(color_ostream &out);
static void deinit_map(color_ostream &out);

struct DigJob {
    int id;
    df::job_type job;
    df::coord pos;
    df::tiletype old_tile;
};

static int next_job_id_save = 0;
static std::map<int,DigJob> diggers;

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "burrow", "Miscellaneous burrow control.", burrow, false,
        "  burrow enable options...\n"
        "  burrow disable options...\n"
        "    Enable or disable features of the plugin.\n"
        "    See below for a list and explanation.\n"
        "  burrow clear-units burrow burrow...\n"
        "  burrow clear-tiles burrow burrow...\n"
        "    Removes all units or tiles from the burrows.\n"
        "  burrow set-units target-burrow src-burrow...\n"
        "  burrow add-units target-burrow src-burrow...\n"
        "  burrow remove-units target-burrow src-burrow...\n"
        "    Adds or removes units in source burrows to/from the target\n"
        "    burrow. Set is equivalent to clear and add.\n"
        "  burrow set-tiles target-burrow src-burrow...\n"
        "  burrow add-tiles target-burrow src-burrow...\n"
        "  burrow remove-tiles target-burrow src-burrow...\n"
        "    Adds or removes tiles in source burrows to/from the target\n"
        "    burrow. In place of a source burrow it is possible to use\n"
        "    one of the following keywords:\n"
        "      ABOVE_GROUND, SUBTERRANEAN, INSIDE, OUTSIDE,\n"
        "      LIGHT, DARK, HIDDEN, REVEALED\n"
        "Implemented features:\n"
        "  auto-grow\n"
        "    When a wall inside a burrow with a name ending in '+' is dug\n"
        "    out, the burrow is extended to newly-revealed adjacent walls.\n"
        "    This final '+' may be omitted in burrow name args of commands above.\n"
        "   Note: Digging 1-wide corridors with the miner inside the burrow is SLOW.\n"
    ));

    if (Core::getInstance().isMapLoaded())
        init_map(out);
    if ( DFHack::Core::getInstance().isWorldLoaded() ) {
        load_config(out);
    }

    EventManager::EventHandler handler(on_presave_callback, 1);
    EventManager::registerListener(EventManager::EventType::PRESAVE, handler, plugin_self);

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    if ( DFHack::Core::getInstance().isWorldLoaded() ) {
        save_config(out);
    }
    deinit_map(out);

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
    case SC_MAP_LOADED:
        load_config(out);
        deinit_map(out);
        if (gamemode &&
            *gamemode == game_mode::DWARF)
            init_map(out);
        break;
    case SC_MAP_UNLOADED:
        //deinit_map(out);
        break;
    default:
        break;
    }

    return CR_OK;
}

static void handle_burrow_rename(color_ostream &out, df::burrow *burrow);

DEFINE_LUA_EVENT_1(onBurrowRename, handle_burrow_rename, df::burrow*);

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
            onBurrowRename(out, burrow);
    }
}

static void handle_dig_complete(color_ostream &out, df::job_type job, df::coord pos,
                                df::tiletype old_tile, df::tiletype new_tile, df::unit *worker);

DEFINE_LUA_EVENT_5(onDigComplete, handle_dig_complete,
                   df::job_type, df::coord, df::tiletype, df::tiletype, df::unit*);

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
                    onDigComplete(out, it->second.job, pos, it->second.old_tile, new_tile, worker);
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

DFhackCExport command_result plugin_onupdate(color_ostream &out)
{
    if (!active)
        return CR_OK;

    detect_burrow_renames(out);

    if (auto_grow)
        detect_digging(out);

    return CR_OK;
}

static void parse_names()
{
    auto &list = ui->burrows.list;

    grow_burrows.clear();
    name_lookup.clear();

    for (size_t i = 0; i < list.size(); i++)
    {
        auto burrow = list[i];

        std::string name = burrow->name;

        if (!name.empty())
        {
            name_lookup[name] = burrow->id;

            if (name[name.size()-1] == '+')
            {
                grow_burrows.push_back(burrow->id);
                name.resize(name.size()-1);
            }

            if (!name.empty())
                name_lookup[name] = burrow->id;
        }
    }
}

static void reset_tracking()
{
    diggers.clear();
    next_job_id_save = 0;
}

static void init_map(color_ostream &out)
{
    auto config = World::GetPersistentData("burrows/config");
    if (config.isValid())
    {
        auto_grow = !!(config.ival(0) & 1);
    }

    parse_names();
    name_burrow_id = -1;

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
    auto rv = World::GetPersistentData("burrows/config", &created);
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
        reset_tracking();
}

static void handle_burrow_rename(color_ostream &out, df::burrow *burrow)
{
    parse_names();
}

static void add_to_burrows(std::vector<df::burrow*> &burrows, df::coord pos)
{
    for (size_t i = 0; i < burrows.size(); i++)
        Burrows::setAssignedTile(burrows[i], pos, true);
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
                                df::tiletype old_tile, df::tiletype new_tile, df::unit *worker)
{
    if (!isWalkable(new_tile))
        return;

    std::vector<df::burrow*> to_grow;

    for (size_t i = 0; i < grow_burrows.size(); i++)
    {
        auto b = df::burrow::find(grow_burrows[i]);
        if (b && Burrows::isAssignedTile(b, pos))
            to_grow.push_back(b);
    }

    //out.print("%d to grow.\n", to_grow.size());

    if (to_grow.empty())
        return;

    MapExtras::MapCache mc;
    bool changed = false;

    if (!isWalkable(old_tile))
    {
        changed = true;
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
        changed = true;
        add_to_burrows(to_grow, pos-df::coord(0,0,1));

        if (tileShape(new_tile) == tiletype_shape::RAMP_TOP)
        {
            add_walls_to_burrows(out, to_grow, mc,
                                 pos+df::coord(-1,-1,-1), pos+df::coord(1,1,-1));
        }
    }

    if (changed && worker && !worker->job.current_job)
        Job::checkDesignationsNow();
}

static void renameBurrow(color_ostream &out, df::burrow *burrow, std::string name)
{
    CHECK_NULL_POINTER(burrow);

    // The event makes this absolutely necessary
    CoreSuspender suspend;

    burrow->name = name;
    onBurrowRename(out, burrow);
}

static df::burrow *findByName(color_ostream &out, std::string name, bool silent = false)
{
    int id = -1;
    if (name_lookup.count(name))
        id = name_lookup[name];
    auto rv = df::burrow::find(id);
    if (!rv && !silent)
        out.printerr("Burrow not found: '%s'\n", name.c_str());
    return rv;
}

static void copyUnits(df::burrow *target, df::burrow *source, bool enable)
{
    CHECK_NULL_POINTER(target);
    CHECK_NULL_POINTER(source);

    if (source == target)
    {
        if (!enable)
            Burrows::clearUnits(target);

        return;
    }

    for (size_t i = 0; i < source->units.size(); i++)
    {
        auto unit = df::unit::find(source->units[i]);

        if (unit)
            Burrows::setAssignedUnit(target, unit, enable);
    }
}

static void copyTiles(df::burrow *target, df::burrow *source, bool enable)
{
    CHECK_NULL_POINTER(target);
    CHECK_NULL_POINTER(source);

    if (source == target)
    {
        if (!enable)
            Burrows::clearTiles(target);

        return;
    }

    std::vector<df::map_block*> pvec;
    Burrows::listBlocks(&pvec, source);

    for (size_t i = 0; i < pvec.size(); i++)
    {
        auto block = pvec[i];
        auto smask = Burrows::getBlockMask(source, block);
        if (!smask)
            continue;

        auto tmask = Burrows::getBlockMask(target, block, enable);
        if (!tmask)
            continue;

        if (enable)
        {
            for (int j = 0; j < 16; j++)
                tmask->tile_bitmask[j] |= smask->tile_bitmask[j];
        }
        else
        {
            for (int j = 0; j < 16; j++)
                tmask->tile_bitmask[j] &= ~smask->tile_bitmask[j];

            if (!tmask->has_assignments())
                Burrows::deleteBlockMask(target, block, tmask);
        }
    }
}

static void setTilesByDesignation(df::burrow *target, df::tile_designation d_mask,
                                  df::tile_designation d_value, bool enable)
{
    CHECK_NULL_POINTER(target);

    auto &blocks = world->map.map_blocks;

    for (size_t i = 0; i < blocks.size(); i++)
    {
        auto block = blocks[i];
        df::block_burrow *mask = NULL;

        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                if ((block->designation[x][y].whole & d_mask.whole) != d_value.whole)
                    continue;

                if (!mask)
                    mask = Burrows::getBlockMask(target, block, enable);
                if (!mask)
                    goto next_block;

                mask->setassignment(x, y, enable);
            }
        }

        if (mask && !enable && !mask->has_assignments())
            Burrows::deleteBlockMask(target, block, mask);

    next_block:;
    }
}

static bool setTilesByKeyword(df::burrow *target, std::string name, bool enable)
{
    CHECK_NULL_POINTER(target);

    df::tile_designation mask(0);
    df::tile_designation value(0);

    if (name == "ABOVE_GROUND")
        mask.bits.subterranean = true;
    else if (name == "SUBTERRANEAN")
        mask.bits.subterranean = value.bits.subterranean = true;
    else if (name == "LIGHT")
        mask.bits.light = value.bits.light = true;
    else if (name == "DARK")
        mask.bits.light = true;
    else if (name == "OUTSIDE")
        mask.bits.outside = value.bits.outside = true;
    else if (name == "INSIDE")
        mask.bits.outside = true;
    else if (name == "HIDDEN")
        mask.bits.hidden = value.bits.hidden = true;
    else if (name == "REVEALED")
        mask.bits.hidden = true;
    else
        return false;

    setTilesByDesignation(target, mask, value, enable);
    return true;
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(renameBurrow),
    DFHACK_LUA_FUNCTION(findByName),
    DFHACK_LUA_FUNCTION(copyUnits),
    DFHACK_LUA_FUNCTION(copyTiles),
    DFHACK_LUA_FUNCTION(setTilesByKeyword),
    DFHACK_LUA_END
};

DFHACK_PLUGIN_LUA_EVENTS {
    DFHACK_LUA_EVENT(onBurrowRename),
    DFHACK_LUA_EVENT(onDigComplete),
    DFHACK_LUA_END
};

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

        for (size_t i = 1; i < parameters.size(); i++)
        {
            string &option = parameters[i];

            if (option == "auto-grow")
                enable_auto_grow(out, state);
            else
                return CR_WRONG_USAGE;
        }
    }
    else if (cmd == "clear-units")
    {
        if (parameters.size() < 2)
            return CR_WRONG_USAGE;

        for (size_t i = 1; i < parameters.size(); i++)
        {
            auto target = findByName(out, parameters[i]);
            if (!target)
                return CR_WRONG_USAGE;

            Burrows::clearUnits(target);
        }
    }
    else if (cmd == "set-units" || cmd == "add-units" || cmd == "remove-units")
    {
        if (parameters.size() < 3)
            return CR_WRONG_USAGE;

        auto target = findByName(out, parameters[1]);
        if (!target)
            return CR_WRONG_USAGE;

        if (cmd == "set-units")
            Burrows::clearUnits(target);

        bool enable = (cmd != "remove-units");

        for (size_t i = 2; i < parameters.size(); i++)
        {
            auto source = findByName(out, parameters[i]);
            if (!source)
                return CR_WRONG_USAGE;

            copyUnits(target, source, enable);
        }
    }
    else if (cmd == "clear-tiles")
    {
        if (parameters.size() < 2)
            return CR_WRONG_USAGE;

        for (size_t i = 1; i < parameters.size(); i++)
        {
            auto target = findByName(out, parameters[i]);
            if (!target)
                return CR_WRONG_USAGE;

            Burrows::clearTiles(target);
        }
    }
    else if (cmd == "set-tiles" || cmd == "add-tiles" || cmd == "remove-tiles")
    {
        if (parameters.size() < 3)
            return CR_WRONG_USAGE;

        auto target = findByName(out, parameters[1]);
        if (!target)
            return CR_WRONG_USAGE;

        if (cmd == "set-tiles")
            Burrows::clearTiles(target);

        bool enable = (cmd != "remove-tiles");

        for (size_t i = 2; i < parameters.size(); i++)
        {
            if (setTilesByKeyword(target, parameters[i], enable))
                continue;

            auto source = findByName(out, parameters[i]);
            if (!source)
                return CR_WRONG_USAGE;

            copyTiles(target, source, enable);
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

void load_config(color_ostream& out) {
    Json::Value& p = Persistent::get("burrows");
    int32_t version = p["version"].isInt() ? p["version"].asInt() : 0;
    if ( version == 0 ) {
        auto config = World::GetPersistentData("burrows/config");
        auto_grow = config.isValid() && config.ival(0);
        World::DeletePersistentData(config);
    } else if ( version == 1 ) {
        auto_grow = p["auto_grow"].isBool() ? p["auto_grow"].asBool() : false;
    } else {
        cerr << __FILE__ << ":" << __LINE__ << ": Unrecognized version: " << version << endl;
        exit(1);
    }
    if ( auto_grow )
        enable_auto_grow(out,true);
}

void save_config(color_ostream& out) {
    Json::Value& p = Persistent::get("burrows");
    p["version"] = persist_version;
    p["auto_grow"] = auto_grow;
}
