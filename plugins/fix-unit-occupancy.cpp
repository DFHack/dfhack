#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/Units.h"
#include "modules/Translation.h"
#include "modules/World.h"

#include "df/map_block.h"
#include "df/unit.h"
#include "df/world.h"

using namespace DFHack;

DFHACK_PLUGIN("fix-unit-occupancy");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);
REQUIRE_GLOBAL(cursor);
REQUIRE_GLOBAL(world);

static int run_interval = 1200; // daily

inline float getClock()
{
    return (float)clock() / (float)CLOCKS_PER_SEC;
}

static std::string get_unit_description(df::unit *unit)
{
    if (!unit)
        return "";
    std::string desc;
    auto name = Units::getVisibleName(unit);
    if (name->has_name)
        desc = Translation::TranslateName(name, false);
    desc += (desc.size() ? ", " : "") + Units::getProfessionName(unit); // Check animal type too

    return desc;
}

df::unit *findUnit(int x, int y, int z)
{
    for (auto u = world->units.active.begin(); u != world->units.active.end(); ++u)
    {
        if ((**u).pos.x == x && (**u).pos.y == y && (**u).pos.z == z)
            return *u;
    }
    return NULL;
}

struct uo_opts {
    bool dry_run;
    bool use_cursor;
    uo_opts (bool dry_run = false, bool use_cursor = false)
            :dry_run(dry_run), use_cursor(use_cursor)
    {}
};

command_result cmd_fix_unit_occupancy (color_ostream &out, std::vector <std::string> & parameters);

unsigned fix_unit_occupancy (color_ostream &out, uo_opts &opts)
{
    if (!Core::getInstance().isWorldLoaded())
        return 0;

    if (opts.use_cursor && cursor->x < 0)
        out.printerr("No cursor\n");

    unsigned count = 0;
    float time1 = getClock();
    for (size_t i = 0; i < world->map.map_blocks.size(); i++)
    {
        df::map_block *block = world->map.map_blocks[i];
        int map_z = block->map_pos.z;
        if (opts.use_cursor && (map_z != cursor->z || block->map_pos.y != (cursor->y / 16) * 16 || block->map_pos.x != (cursor->x / 16) * 16))
            continue;
        for (int x = 0; x < 16; x++)
        {
            int map_x = x + block->map_pos.x;
            for (int y = 0; y < 16; y++)
            {
                if (block->designation[x][y].bits.hidden)
                    continue;
                int map_y = y + block->map_pos.y;
                if (opts.use_cursor && (map_x != cursor->x || map_y != cursor->y))
                    continue;
                bool cur_occupancy = block->occupancy[x][y].bits.unit;
                bool fixed_occupancy = cur_occupancy;
                df::unit *cur_unit = findUnit(map_x, map_y, map_z);
                if (cur_occupancy && !cur_unit)
                {
                    out.print("%sFixing occupancy at (%i, %i, %i) - no unit found\n",
                        opts.dry_run ? "(Dry run) " : "",
                        map_x, map_y, map_z);
                    fixed_occupancy = false;
                }
                // else if (!cur_occupancy && cur_unit)
                // {
                //     out.printerr("Unit found at (%i, %i, %i): %s\n", map_x, map_y, map_z, get_unit_description(cur_unit).c_str());
                //     fixed_occupancy = true;
                // }
                if (cur_occupancy != fixed_occupancy && !opts.dry_run)
                {
                    ++count;
                    block->occupancy[x][y].bits.unit = fixed_occupancy;
                }
            }
        }
    }
    float time2 = getClock();
    std::cerr << "fix-unit-occupancy: elapsed time: " << time2 - time1 << " secs" << endl;
    if (count)
        out << "Fixed occupancy of " << count << " tiles [fix-unit-occupancy]" << endl;
    return count;
}

unsigned fix_unit_occupancy (color_ostream &out)
{
    uo_opts tmp;
    return fix_unit_occupancy(out, tmp);
}

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "fix-unit-occupancy",
        "Fix unit occupancy issues such as phantom 'creature blocking site' messages (bug 3499)",
        cmd_fix_unit_occupancy,
        false, //allow non-interactive use
        "  enable fix-unit-occupancy: Enable the plugin\n"
        "  disable fix-unit-occupancy fix-unit-occupancy: Disable the plugin\n"
        "  fix-unit-occupancy: Run the plugin immediately. Available options:\n"
        "    -h|here|cursor: Only operate on the tile at the cursor\n"
        "    -n|dry|dry-run: Do not write changes to map\n"
        "  fix-unit-occupancy interval X: Run the plugin every X ticks (when enabled).\n"
        "    Default is 1200, or 1 day. Ticks are only counted when the game is unpaused.\n"

    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    return CR_OK;
}

DFhackCExport command_result plugin_enable (color_ostream &out, bool enable)
{
    is_enabled = enable;
    if (is_enabled)
        fix_unit_occupancy(out);
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate (color_ostream &out)
{
    static unsigned tick = UINT_MAX;
    static decltype(world->frame_counter) old_world_frame = 0;
    if (is_enabled && World::isFortressMode())
    {
        // only increment tick when the world has changed
        if (old_world_frame != world->frame_counter)
        {
            old_world_frame = world->frame_counter;
            tick++;
        }
        if (tick > run_interval)
        {
            tick = 0;
            fix_unit_occupancy(out);
        }
    }
    else
    {
        tick = INT_MAX;
        old_world_frame = 0;
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange (color_ostream &out, state_change_event evt)
{
    if (evt == SC_MAP_LOADED && is_enabled && World::isFortressMode())
        fix_unit_occupancy(out);
    return CR_OK;
}

command_result cmd_fix_unit_occupancy (color_ostream &out, std::vector <std::string> & parameters)
{
    CoreSuspender suspend;
    uo_opts opts;
    bool ok = true;

    if (parameters.size() >= 1 && (parameters[0] == "-i" || parameters[0].find("interval") != std::string::npos))
    {
        if (parameters.size() >= 2)
        {
            int new_interval = atoi(parameters[1].c_str());
            if (new_interval < 100)
            {
                out.printerr("Invalid interval - minimum is 100 ticks\n");
                return CR_WRONG_USAGE;
            }
            run_interval = new_interval;
            if (!is_enabled)
                out << "note: Plugin not enabled (use `enable fix-unit-occupancy` to enable)" << endl;
            return CR_OK;
        }
        else
            return CR_WRONG_USAGE;
    }

    for (auto opt = parameters.begin(); opt != parameters.end(); ++opt)
    {
        if (*opt == "-n" || opt->find("dry") != std::string::npos)
            opts.dry_run = true;
        else if (*opt == "-h" || opt->find("cursor") != std::string::npos || opt->find("here") != std::string::npos)
            opts.use_cursor = true;
        else if (opt->find("enable") != std::string::npos)
            plugin_enable(out, true);
        else if (opt->find("disable") != std::string::npos)
            plugin_enable(out, false);
        else
        {
            out.printerr("Unknown parameter: %s\n", opt->c_str());
            ok = false;
        }
    }
    if (!ok)
        return CR_WRONG_USAGE;

    fix_unit_occupancy(out, opts);

    return CR_OK;
}
