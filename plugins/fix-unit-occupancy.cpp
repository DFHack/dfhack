#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"

#include "modules/Maps.h"
#include "modules/Units.h"
#include "modules/Translation.h"
#include "modules/World.h"

#include "df/creature_raw.h"
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

struct uo_buf {
    uint32_t dim_x, dim_y, dim_z;
    size_t size;
    uint8_t *buf;
    uo_buf() : size(0), buf(NULL)
    { }
    ~uo_buf()
    {
        if (buf)
            free(buf);
    }
    void resize()
    {
        Maps::getSize(dim_x, dim_y, dim_z);
        dim_x *= 16;
        dim_y *= 16;
        size = dim_x * dim_y * dim_z;
        buf = (uint8_t*)realloc(buf, size);
        clear();
    }
    inline void clear()
    {
        memset(buf, 0, size);
    }
    inline size_t offset (uint32_t x, uint32_t y, uint32_t z)
    {
        return (dim_x * dim_y * z) + (dim_x * y) + x;
    }
    inline uint8_t get (uint32_t x, uint32_t y, uint32_t z)
    {
        size_t off = offset(x, y, z);
        if (off < size)
            return buf[off];
        return 0;
    }
    inline void set (uint32_t x, uint32_t y, uint32_t z, uint8_t val)
    {
        size_t off = offset(x, y, z);
        if (off < size)
            buf[off] = val;
    }
    inline void get_coords (size_t off, uint32_t &x, uint32_t &y, uint32_t &z)
    {
        x = off % dim_x;
        y = (off / dim_x) % dim_y;
        z = off / (dim_x * dim_y);
    }
};

static uo_buf uo_buffer;

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

    if (!World::isFortressMode() && !opts.use_cursor)
    {
        out.printerr("Can only scan entire map in fortress mode\n");
        return 0;
    }

    if (opts.use_cursor && cursor->x < 0)
    {
        out.printerr("No cursor\n");
        return 0;
    }

    uo_buffer.resize();
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
                if (block->occupancy[x][y].bits.unit)
                    uo_buffer.set(map_x, map_y, map_z, 1);
            }
        }
    }

    for (auto it = world->units.active.begin(); it != world->units.active.end(); ++it)
    {
        df::unit *u = *it;
        if (!u || u->flags1.bits.caged || u->pos.x < 0)
            continue;
        df::creature_raw *craw = df::creature_raw::find(u->race);
        int unit_extents = (craw && craw->flags.is_set(df::creature_raw_flags::EQUIPMENT_WAGON)) ? 1 : 0;
        for (int16_t x = u->pos.x - unit_extents; x <= u->pos.x + unit_extents; ++x)
        {
            for (int16_t y = u->pos.y - unit_extents; y <= u->pos.y + unit_extents; ++y)
            {
                uo_buffer.set(x, y, u->pos.z, 0);
            }
        }
    }

    for (size_t i = 0; i < uo_buffer.size; i++)
    {
        if (uo_buffer.buf[i])
        {
            uint32_t x, y, z;
            uo_buffer.get_coords(i, x, y, z);
            out.print("(%u, %u, %u) - no unit found\n", x, y, z);
            ++count;
            if (!opts.dry_run)
            {
                df::map_block *b = Maps::getTileBlock(x, y, z);
                b->occupancy[x % 16][y % 16].bits.unit = false;
            }
        }
    }

    float time2 = getClock();
    std::cerr << "fix-unit-occupancy: elapsed time: " << time2 - time1 << " secs" << endl;
    if (count)
        out << (opts.dry_run ? "[dry run] " : "") << "Fixed occupancy of " << count << " tiles [fix-unit-occupancy]" << endl;
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

    unsigned count = fix_unit_occupancy(out, opts);
    if (!count)
        out << "No occupancy issues found." << endl;

    return CR_OK;
}
