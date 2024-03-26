#include "Debug.h"
#include "LuaTools.h"
#include "MemAccess.h"
#include "PluginManager.h"

#include "modules/EventManager.h"
#include "modules/Gui.h"
#include "modules/MapCache.h"
#include "modules/Job.h"
#include "modules/Persistence.h"
#include "modules/Screen.h"
#include "modules/Textures.h"
#include "modules/World.h"

#include "df/block_square_event_designation_priorityst.h"
#include "df/gamest.h"
#include "df/job_list_link.h"
#include "df/map_block.h"
#include "df/world.h"

#include <vector>
#include <cstdio>
#include <cstdlib>
#include <stack>
#include <string>
#include <cmath>
#include <memory>

using std::vector;
using std::string;
using std::stack;
using namespace DFHack;
using namespace df::enums;

command_result digv (color_ostream &out, vector <string> & parameters);
command_result digvx (color_ostream &out, vector <string> & parameters);
command_result digl (color_ostream &out, vector <string> & parameters);
command_result diglx (color_ostream &out, vector <string> & parameters);
command_result digexp (color_ostream &out, vector <string> & parameters);
command_result digcircle (color_ostream &out, vector <string> & parameters);
command_result digtype (color_ostream &out, vector <string> & parameters);

DFHACK_PLUGIN("dig");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(game);
REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(window_x);
REQUIRE_GLOBAL(window_y);
REQUIRE_GLOBAL(window_z);

namespace DFHack {
    DBG_DECLARE(dig, log, DebugCategory::LINFO);
}

static const string WARM_CONFIG_KEY = string(plugin_name) + "/warmdig";
static const string DAMP_CONFIG_KEY = string(plugin_name) + "/dampdig";
static PersistentDataItem warm_config, damp_config;

static void unhide_surrounding_tagged_tiles(color_ostream& out, void* ptr);

static const int32_t CYCLE_TICKS = 1223; // a prime number that's about a day
static int32_t cycle_timestamp = 0;  // world->frame_counter at last cycle

static vector<TexposHandle> textures;

static bool is_painting_warm = false;
static bool is_painting_damp = false;

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands) {
    textures = Textures::loadTileset("hack/data/art/damp_dig_map.png", 32, 32, true);

    commands.push_back(PluginCommand(
        "digv",
        "Dig a whole vein.",
        digv,
        Gui::cursor_hotkey));
    commands.push_back(PluginCommand(
        "digvx",
        "Dig a whole vein, following through z-levels.",
        digvx,
        Gui::cursor_hotkey));
   commands.push_back(PluginCommand(
        "digl",
        "Dig layerstone.",
        digl,
        Gui::cursor_hotkey));
    commands.push_back(PluginCommand(
        "diglx",
        "Dig layerstone, following through z-levels.",
        diglx,
        Gui::cursor_hotkey));
    commands.push_back(PluginCommand(
        "digexp",
        "Select or designate an exploratory pattern.",
        digexp));
    commands.push_back(PluginCommand(
        "digcircle",
        "Dig designate a circle (filled or hollow)",
        digcircle));
    commands.push_back(PluginCommand(
        "digtype",
        "Dig all veins of a given type.",
        digtype,Gui::cursor_hotkey));
    return CR_OK;
}

static void do_enable(bool enable) {
    if (enable != is_enabled) {
        is_enabled = enable;
        DEBUG(log).print("%s\n", is_enabled ? "enabled" : "disabled");
        if (enable) {
            EventManager::registerListener(EventManager::EventType::JOB_STARTED,
                EventManager::EventHandler(unhide_surrounding_tagged_tiles, 0), plugin_self);
        } else {
            EventManager::unregisterAll(plugin_self);
        }
    }
    else {
        DEBUG(log).print("%s, but already %s; no action\n",
            is_enabled ? "enabled" : "disabled", is_enabled ? "enabled" : "disabled");
    }
}

DFhackCExport command_result plugin_load_site_data(color_ostream &out) {
    cycle_timestamp = 0;
    is_painting_warm = false;
    is_painting_damp = false;

    warm_config = World::GetPersistentSiteData(WARM_CONFIG_KEY);
    damp_config = World::GetPersistentSiteData(DAMP_CONFIG_KEY);

    if (!warm_config.isValid()) {
        DEBUG(log,out).print("no warm dig config found in this save; initializing\n");
        warm_config = World::AddPersistentSiteData(WARM_CONFIG_KEY);
    }
    if (!damp_config.isValid()) {
        DEBUG(log,out).print("no damp dig config found in this save; initializing\n");
        damp_config = World::AddPersistentSiteData(DAMP_CONFIG_KEY);
    }

    for (auto & block : world->map.map_blocks) {
        auto warm_mask = World::getPersistentTilemask(warm_config, block);
        auto damp_mask = World::getPersistentTilemask(damp_config, block);
        if (warm_mask || damp_mask) {
            do_enable(true);
            break;
        }
    }

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == DFHack::SC_WORLD_UNLOADED) {
        is_painting_warm = false;
        is_painting_damp = false;
        do_enable(false);
    }
    return CR_OK;
}

static void fill_dig_jobs(std::unordered_map<df::coord, df::job *> &dig_jobs) {
    df::job_list_link *link = world->jobs.list.next;
    for (; link; link = link->next) {
        auto job = link->item;
        auto type = ENUM_ATTR(job_type, type, job->job_type);
        if (type != job_type_class::Digging)
            continue;
        dig_jobs.emplace(job->pos, job);
    }
}

// scrub no-longer designated tiles
static void do_cycle(color_ostream &out) {
    cycle_timestamp = world->frame_counter;

    std::unordered_map<df::coord, df::job *> dig_jobs;
    fill_dig_jobs(dig_jobs);

    bool has_assignment = false;
    uint32_t scrubbed = 0;
    for (auto & block : world->map.map_blocks) {
        auto warm_mask = World::getPersistentTilemask(warm_config, block);
        auto damp_mask = World::getPersistentTilemask(damp_config, block);
        if (!warm_mask && !damp_mask)
            continue;

        bool used = false;
        const auto & block_pos = block->map_pos;
        for (uint32_t x = 0; x < 16; x++) for (uint32_t y = 0; y < 16; y++) {
            if ((!warm_mask || !warm_mask->getassignment(x, y)) &&
                (!damp_mask || !damp_mask->getassignment(x, y)))
            {
                continue;
            }

            if (!block->designation[x][y].bits.dig &&
                !dig_jobs.contains(block_pos + df::coord(x, y, 0)))
            {
                if (warm_mask) warm_mask->setassignment(x, y, false);
                if (damp_mask) damp_mask->setassignment(x, y, false);
                ++scrubbed;
            } else {
                has_assignment = true;
                used = true;
            }
        }

        if (!used) {
            // delete the tile masks so we can skip over this block in the future
            World::deletePersistentTilemask(warm_config, block);
            World::deletePersistentTilemask(damp_config, block);
        }
    }

    DEBUG(log,out).print("scrubbed %u tagged tiles\n", scrubbed);

    if (!has_assignment){
        DEBUG(log,out).print("no more active tagged tiles; disabling\n");
        do_enable(false);
    }
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (is_enabled && world->frame_counter - cycle_timestamp >= CYCLE_TICKS)
        do_cycle(out);
    return CR_OK;
}

/////////////////////////////////////////////////////
// tile property detectors
//

static bool is_warm(const df::coord &pos) {
    auto block = Maps::getTileBlock(pos);
    if (!block)
        return false;
    return block->temperature_1[pos.x&15][pos.y&15] >= 10075;
}

static bool is_wall(const df::coord &pos) {
    df::tiletype *tt = Maps::getTileType(pos);
    return tt && tileShape(*tt) == df::tiletype_shape::WALL;
}

static bool is_rough_wall(int16_t x, int16_t y, int16_t z) {
    df::tiletype *tt = Maps::getTileType(x, y, z);
    if (!tt)
        return false;

    return tileShape(*tt) == df::tiletype_shape::WALL &&
        tileSpecial(*tt) != df::tiletype_special::SMOOTH;
}

static bool is_smooth_wall(const df::coord &pos) {
    df::tiletype *tt = Maps::getTileType(pos);
    return tt && tileSpecial(*tt) == df::tiletype_special::SMOOTH
                && tileShape(*tt) == df::tiletype_shape::WALL;
}

static bool is_aquifer(int16_t x, int16_t y, int16_t z, df::tile_designation *des = NULL) {
    if (!des)
        des = Maps::getTileDesignation(x, y, z);
    if (!des)
        return false;
    return des->bits.water_table && is_rough_wall(x, y, z);
}

static bool is_aquifer(const df::coord &pos, df::tile_designation *des = NULL) {
    return is_aquifer(pos.x, pos.y, pos.z, des);
}

static bool is_heavy_aquifer(int16_t x, int16_t y, int16_t z, df::map_block *block = NULL) {
    if (!block)
        block = Maps::getTileBlock(x, y, z);
    if (!block || !block->flags.bits.has_aquifer)
        return false;

    auto occ = Maps::getTileOccupancy(x, y, z);
    if (!occ)
        return false;

    return occ->bits.heavy_aquifer;
}

static bool is_heavy_aquifer(const df::coord &pos, df::map_block *block = NULL) {
    return is_heavy_aquifer(pos.x, pos.y, pos.z, block);
}

static bool is_wet(int16_t x, int16_t y, int16_t z) {
    auto des = Maps::getTileDesignation(x, y, z);
    if (!des)
        return false;
    if (des->bits.liquid_type == df::tile_liquid::Water && des->bits.flow_size >= 1)
        return true;
    if (is_aquifer(x, y, z, des))
        return true;
    return false;
}

static bool is_damp(const df::coord &pos) {
    return is_wet(pos.x-1, pos.y-1, pos.z) ||
        is_wet(pos.x, pos.y-1, pos.z) ||
        is_wet(pos.x+1, pos.y-1, pos.z) ||
        is_wet(pos.x-1, pos.y, pos.z) ||
        is_wet(pos.x+1, pos.y, pos.z) ||
        is_wet(pos.x-1, pos.y+1, pos.z) ||
        is_wet(pos.x, pos.y+1, pos.z) ||
        is_wet(pos.x+1, pos.y+1, pos.z) ||
        is_wet(pos.x, pos.y, pos.z+1);
}

/////////////////////////////////////////////////////
// event handlers
//

static void propagate_if_material_match(color_ostream& out, MapExtras::MapCache & mc,
    int16_t mat, bool warm, bool damp, const df::coord & pos)
{
    auto block = Maps::getTileBlock(pos);
    if (!block)
        return;

    INFO(log,out).print("testing adjacent tile at (%d,%d,%d), mat:%d",
        pos.x, pos.y, pos.z, mc.veinMaterialAt(pos));

    if (mat != mc.veinMaterialAt(pos))
        return;

    auto des = Maps::getTileDesignation(pos);
    auto occ = Maps::getTileOccupancy(pos);
    if (!des || !occ)
        return;

    des->bits.dig = df::tile_dig_designation::Default;
    occ->bits.dig_auto = true;

    if (warm) {
        if (auto warm_mask = World::getPersistentTilemask(warm_config, block, true))
            warm_mask->setassignment(pos, true);
    }
    if (damp) {
        if (auto damp_mask = World::getPersistentTilemask(damp_config, block, true))
            damp_mask->setassignment(pos, true);
    }
}

static void do_autodig(color_ostream& out, bool warm, bool damp, const df::coord & pos) {
    MapExtras::MapCache mc;
    int16_t mat = mc.veinMaterialAt(pos);
    if (mat == -1)
        return;

    DEBUG(log,out).print("processing autodig tile at (%d,%d,%d), warm:%d, damp:%d, mat:%d\n",
        pos.x, pos.y, pos.z, warm, damp, mat);

    propagate_if_material_match(out, mc, mat, warm, damp, pos + df::coord(-1, -1, 0));
    propagate_if_material_match(out, mc, mat, warm, damp, pos + df::coord( 0, -1, 0));
    propagate_if_material_match(out, mc, mat, warm, damp, pos + df::coord( 1, -1, 0));
    propagate_if_material_match(out, mc, mat, warm, damp, pos + df::coord(-1,  0, 0));
    propagate_if_material_match(out, mc, mat, warm, damp, pos + df::coord( 1,  0, 0));
    propagate_if_material_match(out, mc, mat, warm, damp, pos + df::coord(-1,  1, 0));
    propagate_if_material_match(out, mc, mat, warm, damp, pos + df::coord( 0,  1, 0));
    propagate_if_material_match(out, mc, mat, warm, damp, pos + df::coord( 1,  1, 0));
}

static void process_taken_dig_job(color_ostream& out, const df::coord & pos) {
    auto block = Maps::getTileBlock(pos);
    if (!block)
        return;

    bool warm = false, damp = false;
    if (auto warm_mask = World::getPersistentTilemask(warm_config, block)) {
        warm = warm_mask->getassignment(pos);
        warm_mask->setassignment(pos, false);
    }
    if (auto damp_mask = World::getPersistentTilemask(damp_config, block)) {
        damp = damp_mask->getassignment(pos);
        damp_mask->setassignment(pos, false);
    }

    auto occ = Maps::getTileOccupancy(pos);
    if (!occ || !occ->bits.dig_auto || (!warm && !damp))
        return;

    do_autodig(out, warm, damp, pos);
}

static void unhide_tagged(color_ostream& out, const df::coord & pos) {
    auto block = Maps::getTileBlock(pos);
    if (!block)
        return;
    if (auto warm_mask = World::getPersistentTilemask(warm_config, block)) {
        TRACE(log,out).print("testing tile at (%d,%d,%d); mask:%d, warm:%d\n", pos.x, pos.y, pos.z,
            warm_mask->getassignment(pos), is_warm(pos));
        if (warm_mask->getassignment(pos) && is_warm(pos)) {
            DEBUG(log,out).print("revealing warm dig tile at (%d,%d,%d)\n", pos.x, pos.y, pos.z);
            block->designation[pos.x&15][pos.y&15].bits.hidden = false;
        }
    }
    if (auto damp_mask = World::getPersistentTilemask(damp_config, block)) {
        TRACE(log,out).print("testing tile at (%d,%d,%d); mask:%d, damp:%d\n", pos.x, pos.y, pos.z,
            damp_mask->getassignment(pos), is_damp(pos));
        if (damp_mask->getassignment(pos) && is_damp(pos)) {
            DEBUG(log,out).print("revealing damp dig tile at (%d,%d,%d)\n", pos.x, pos.y, pos.z);
            block->designation[pos.x&15][pos.y&15].bits.hidden = false;
        }
    }
}

static void unhide_surrounding_tagged_tiles(color_ostream& out, void* job_ptr) {
    df::job *job = (df::job *)job_ptr;
    auto type = ENUM_ATTR(job_type, type, job->job_type);
    if (type != job_type_class::Digging)
        return;

    const auto & pos = job->pos;
    TRACE(log,out).print("handing dig job at (%d,%d,%d)\n", pos.x, pos.y, pos.z);

    process_taken_dig_job(out, pos);

    unhide_tagged(out, pos + df::coord(-1, -1, 0));
    unhide_tagged(out, pos + df::coord( 0, -1, 0));
    unhide_tagged(out, pos + df::coord( 1, -1, 0));
    unhide_tagged(out, pos + df::coord(-1,  0, 0));
    unhide_tagged(out, pos + df::coord( 1,  0, 0));
    unhide_tagged(out, pos + df::coord(-1,  1, 0));
    unhide_tagged(out, pos + df::coord( 0,  1, 0));
    unhide_tagged(out, pos + df::coord( 1,  1, 0));

    switch (job->job_type) {
    case df::job_type::CarveUpDownStaircase:
        unhide_tagged(out, pos + df::coord(0, 0, -1));
        unhide_tagged(out, pos + df::coord(0, 0,  1));
        break;
    case df::job_type::CarveDownwardStaircase:
        unhide_tagged(out, pos + df::coord(0, 0, -1));
        break;
    case df::job_type::CarveUpwardStaircase:
        unhide_tagged(out, pos + df::coord(0, 0, 1));
        break;
    case df::job_type::DigChannel:
        unhide_tagged(out, pos + df::coord(-1, -1, -1));
        unhide_tagged(out, pos + df::coord( 0, -1, -1));
        unhide_tagged(out, pos + df::coord( 1, -1, -1));
        unhide_tagged(out, pos + df::coord(-1,  0, -1));
        unhide_tagged(out, pos + df::coord( 1,  0, -1));
        unhide_tagged(out, pos + df::coord(-1,  1, -1));
        unhide_tagged(out, pos + df::coord( 0,  1, -1));
        unhide_tagged(out, pos + df::coord( 1,  1, -1));
        break;
    case df::job_type::CarveRamp:
        unhide_tagged(out, pos + df::coord(-1, -1, 1));
        unhide_tagged(out, pos + df::coord( 0, -1, 1));
        unhide_tagged(out, pos + df::coord( 1, -1, 1));
        unhide_tagged(out, pos + df::coord(-1,  0, 1));
        unhide_tagged(out, pos + df::coord( 1,  0, 1));
        unhide_tagged(out, pos + df::coord(-1,  1, 1));
        unhide_tagged(out, pos + df::coord( 0,  1, 1));
        unhide_tagged(out, pos + df::coord( 1,  1, 1));
        break;
    default:
        break;
    }
}

/////////////////////////////////////////////////////
// commands
//

template <class T>
bool from_string(T& t,
    const std::string& s,
    std::ios_base& (*f)(std::ios_base&))
{
    std::istringstream iss(s);
    return !(iss >> f >> t).fail();
}

enum circle_what
{
    circle_set,
    circle_unset,
    circle_invert,
};

bool dig (MapExtras::MapCache & MCache,
    circle_what what,
    df::tile_dig_designation type,
    int32_t priority,
    int32_t x, int32_t y, int32_t z,
    int x_max, int y_max
    )
{
    DFCoord at (x,y,z);
    auto b = MCache.BlockAt(at/16);
    if(!b || !b->is_valid())
        return false;
    if(x == 0 || x == x_max * 16 - 1)
    {
        //out.print("not digging map border\n");
        return false;
    }
    if(y == 0 || y == y_max * 16 - 1)
    {
        //out.print("not digging map border\n");
        return false;
    }
    df::tiletype tt = MCache.tiletypeAt(at);
    df::tile_designation des = MCache.designationAt(at);
    // could be potentially used to locate hidden constructions?
    if(tileMaterial(tt) == tiletype_material::CONSTRUCTION && !des.bits.hidden)
        return false;
    df::tiletype_shape ts = tileShape(tt);
    if (ts == tiletype_shape::EMPTY && !des.bits.hidden)
        return false;
    if(!des.bits.hidden)
    {
        do
        {
            df::tiletype_shape_basic tsb = ENUM_ATTR(tiletype_shape, basic_shape, ts);
            if(tsb == tiletype_shape_basic::Wall)
            {
                std::cerr << "allowing tt" << (int)tt << ", is wall\n";
                break;
            }
            if (tsb == tiletype_shape_basic::Floor
                && (type == tile_dig_designation::DownStair || type == tile_dig_designation::Channel)
                && ts != tiletype_shape::BRANCH
                && ts != tiletype_shape::TRUNK_BRANCH
                && ts != tiletype_shape::TWIG
                )
            {
                std::cerr << "allowing tt" << (int)tt << ", is floor\n";
                break;
            }
            if (tsb == tiletype_shape_basic::Stair && type == tile_dig_designation::Channel )
                break;
            return false;
        }
        while(0);
    }
    switch(what)
    {
    case circle_set:
        if(des.bits.dig == tile_dig_designation::No)
        {
            des.bits.dig = type;
        }
        break;
    case circle_unset:
        if (des.bits.dig != tile_dig_designation::No)
        {
            des.bits.dig = tile_dig_designation::No;
        }
        break;
    case circle_invert:
        if(des.bits.dig == tile_dig_designation::No)
        {
            des.bits.dig = type;
        }
        else
        {
            des.bits.dig = tile_dig_designation::No;
        }
        break;
    }
    std::cerr << "allowing tt" << (int)tt << "\n";
    MCache.setDesignationAt(at,des,priority);
    return true;
};

bool lineX (MapExtras::MapCache & MCache,
    circle_what what,
    df::tile_dig_designation type,
    int32_t priority,
    int32_t y1, int32_t y2, int32_t x, int32_t z,
    int x_max, int y_max
    )
{
    for(int32_t y = y1; y <= y2; y++)
    {
        dig(MCache, what, type, priority, x,y,z, x_max, y_max);
    }
    return true;
};

bool lineY (MapExtras::MapCache & MCache,
    circle_what what,
    df::tile_dig_designation type,
    int32_t priority,
    int32_t x1, int32_t x2, int32_t y, int32_t z,
    int x_max, int y_max
    )
{
    for(int32_t x = x1; x <= x2; x++)
    {
        dig(MCache, what, type, priority, x,y,z, x_max, y_max);
    }
    return true;
};

int32_t parse_priority(color_ostream &out, vector<string> &parameters)
{
    int32_t default_priority = game->main_interface.designation.priority;

    for (auto it = parameters.begin(); it != parameters.end(); ++it)
    {
        const string &s = *it;
        if (s.substr(0, 2) == "p=" || s.substr(0, 2) == "-p")
        {
            if (s.size() >= 3)
            {
                auto priority = int32_t(1000 * atof(s.c_str() + 2));
                parameters.erase(it);
                return priority;
            }
            else if (it + 1 != parameters.end())
            {
                auto priority = int32_t(1000 * atof((*(it + 1)).c_str()));
                parameters.erase(it, it + 2);
                return priority;
            }
            else
            {
                out.printerr("invalid priority specified; reverting to %i\n", default_priority);
                break;
            }
        }
    }

    return default_priority;
}

string forward_priority(color_ostream &out, vector<string> &parameters)
{
    return string("-p") + int_to_string(parse_priority(out, parameters) / 1000);
}

command_result digcircle (color_ostream &out, vector <string> & parameters)
{
    static bool filled = false;
    static circle_what what = circle_set;
    static df::tile_dig_designation type = tile_dig_designation::Default;
    static int diameter = 0;
    auto saved_d = diameter;
    bool force_help = false;
    int32_t priority = parse_priority(out, parameters);

    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            force_help = true;
        }
        else if(parameters[i] == "hollow")
        {
            filled = false;
        }
        else if(parameters[i] == "filled")
        {
            filled = true;
        }
        else if(parameters[i] == "set")
        {
            what = circle_set;
        }
        else if(parameters[i] == "unset")
        {
            what = circle_unset;
        }
        else if(parameters[i] == "invert")
        {
            what = circle_invert;
        }
        else if(parameters[i] == "dig")
        {
            type = tile_dig_designation::Default;
        }
        else if(parameters[i] == "ramp")
        {
            type = tile_dig_designation::Ramp;
        }
        else if(parameters[i] == "dstair")
        {
            type = tile_dig_designation::DownStair;
        }
        else if(parameters[i] == "ustair")
        {
            type = tile_dig_designation::UpStair;
        }
        else if(parameters[i] == "xstair")
        {
            type = tile_dig_designation::UpDownStair;
        }
        else if(parameters[i] == "chan")
        {
            type = tile_dig_designation::Channel;
        }
        else if (!from_string(diameter,parameters[i], std::dec))
        {
            diameter = saved_d;
        }
    }
    if(diameter < 0)
        diameter = -diameter;
    if(force_help || diameter == 0)
    {
        out.print(
            "A command for easy designation of filled and hollow circles.\n"
            "\n"
            "Options:\n"
            " hollow = Set the circle to hollow (default)\n"
            " filled = Set the circle to filled\n"
            "\n"
            "    set = set designation\n"
            "  unset = unset current designation\n"
            " invert = invert current designation\n"
            "\n"
            "    dig = normal digging\n"
            "   ramp = ramp digging\n"
            " ustair = staircase up\n"
            " dstair = staircase down\n"
            " xstair = staircase up/down\n"
            "   chan = dig channel\n"
            "\n"
            "      # = diameter in tiles (default = 0)\n"
            "   -p # = designation priority (default = 4)\n"
            "\n"
            "After you have set the options, the command called with no options\n"
            "repeats with the last selected parameters:\n"
            "'digcircle filled 3' = Dig a filled circle with diameter = 3.\n"
            "'digcircle' = Do it again.\n"
            );
        return CR_OK;
    }
    int32_t cx, cy, cz;
    CoreSuspender suspend;
    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    uint32_t x_max, y_max, z_max;
    Maps::getSize(x_max,y_max,z_max);

    MapExtras::MapCache MCache;
    if(!Gui::getCursorCoords(cx,cy,cz) || cx == -30000)
    {
        out.printerr("Can't get the cursor coords...\n");
        return CR_FAILURE;
    }
    int r = diameter / 2;
    int iter;
    bool adjust;
    if(diameter % 2)
    {
        // paint center
        if(filled)
        {
            lineY(MCache, what, type, priority, cx - r, cx + r, cy, cz, x_max, y_max);
        }
        else
        {
            dig(MCache, what, type, priority, cx - r, cy, cz, x_max, y_max);
            dig(MCache, what, type, priority, cx + r, cy, cz, x_max, y_max);
        }
        adjust = false;
        iter = 2;
    }
    else
    {
        adjust = true;
        iter = 1;
    }
    int lastwhole = r;
    for(; iter <= diameter - 1; iter +=2)
    {
        // top, bottom coords
        int top = cy - ((iter + 1) / 2) + adjust;
        int bottom = cy + ((iter + 1) / 2);
        // see where the current 'line' intersects the circle
        double val = std::sqrt(double(diameter*diameter - iter*iter));
        // adjust for circles with odd diameter
        if(!adjust)
            val -= 1;
        // map the found value to the DF grid
        double whole;
        double fraction = std::modf(val / 2.0, & whole);
        if (fraction > 0.5)
            whole += 1.0;
        int right = cx + whole;
        int left = cx - whole + adjust;
        int diff = lastwhole - whole;
        // paint
        if(filled || iter == diameter - 1)
        {
            lineY(MCache, what, type, priority, left, right, top, cz, x_max, y_max);
            lineY(MCache, what, type, priority, left, right, bottom, cz, x_max, y_max);
        }
        else
        {
            dig(MCache, what, type, priority, left, top, cz, x_max, y_max);
            dig(MCache, what, type, priority, left, bottom, cz, x_max, y_max);
            dig(MCache, what, type, priority, right, top, cz, x_max, y_max);
            dig(MCache, what, type, priority, right, bottom, cz, x_max, y_max);
        }
        if(!filled && diff > 1)
        {
            int lright = cx + lastwhole;
            int lleft = cx - lastwhole + adjust;
            lineY(MCache, what, type, priority, lleft + 1, left - 1, top + 1 , cz, x_max, y_max);
            lineY(MCache, what, type, priority, right + 1, lright - 1, top + 1 , cz, x_max, y_max);
            lineY(MCache, what, type, priority, lleft + 1, left - 1, bottom - 1 , cz, x_max, y_max);
            lineY(MCache, what, type, priority, right + 1, lright - 1, bottom - 1 , cz, x_max, y_max);
        }
        lastwhole = whole;
    }
    MCache.WriteAll();
    return CR_OK;
}
typedef char digmask[16][16];

static digmask diag5[5] =
{
    {
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
    },
    {
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
    },
    {
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
    },
    {
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
    },
    {
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
    },
};

static digmask diag5r[5] =
{
    {
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
    },
    {
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
    },
    {
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
    },
    {
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
    },
    {
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
        {0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0},
        {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
    },
};

static digmask ladder[3] =
{
    {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0},
        {1,0,1,1,0,1,1,0,1,1,0,1,1,0,1,1},
        {0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0},
        {1,0,1,1,0,1,1,0,1,1,0,1,1,0,1,1},
        {0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0},
        {1,0,1,1,0,1,1,0,1,1,0,1,1,0,1,1},
        {0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0},
        {1,0,1,1,0,1,1,0,1,1,0,1,1,0,1,1},
        {0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0},
    },
    {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0},
        {0,1,1,0,1,1,0,1,1,0,1,1,0,1,1,0},
        {0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0},
        {0,1,1,0,1,1,0,1,1,0,1,1,0,1,1,0},
        {0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0},
        {0,1,1,0,1,1,0,1,1,0,1,1,0,1,1,0},
        {0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0},
        {0,1,1,0,1,1,0,1,1,0,1,1,0,1,1,0},
        {0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1},
    },
    {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0},
        {1,1,0,1,1,0,1,1,0,1,1,0,1,1,0,1},
        {0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0},
        {1,1,0,1,1,0,1,1,0,1,1,0,1,1,0,1},
        {0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0},
        {1,1,0,1,1,0,1,1,0,1,1,0,1,1,0,1},
        {0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0},
        {1,1,0,1,1,0,1,1,0,1,1,0,1,1,0,1},
        {0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0},
    },
};

static digmask ladderr[3] =
{
    {
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
    },
    {
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
    },
    {
        {0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0},
        {0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0},
    },
};

static digmask all_tiles =
{
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
};

static digmask cross =
{
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0},
    {0,0,0,1,0,0,0,1,1,0,0,0,1,0,0,0},
    {0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0},
    {0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0},
    {0,0,0,1,0,0,0,1,1,0,0,0,1,0,0,0},
    {0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};
enum explo_how
{
    EXPLO_NOTHING,
    EXPLO_DIAG5,
    EXPLO_DIAG5R,
    EXPLO_LADDER,
    EXPLO_LADDERR,
    EXPLO_CLEAR,
    EXPLO_CROSS,
};

enum explo_what
{
    EXPLO_ALL,
    EXPLO_HIDDEN,
    EXPLO_DESIGNATED,
};

bool stamp_pattern (uint32_t bx, uint32_t by, int z_level,
    digmask & dm, explo_how how, explo_what what,
    int x_max, int y_max
    )
{
    df::map_block * bl = Maps::getBlock(bx,by,z_level);
    if(!bl)
        return false;
    int x = 0,mx = 16;
    if(bx == 0)
        x = 1;
    if(int(bx) == x_max - 1)
        mx = 15;
    for(; x < mx; x++)
    {
        int y = 0,my = 16;
        if(by == 0)
            y = 1;
        if(int(by) == y_max - 1)
            my = 15;
        for(; y < my; y++)
        {
            df::tile_designation & des = bl->designation[x][y];
            df::tiletype tt = bl->tiletype[x][y];
            // could be potentially used to locate hidden constructions?
            if(tileMaterial(tt) == tiletype_material::CONSTRUCTION && !des.bits.hidden)
                continue;
            if(!isWallTerrain(tt) && !des.bits.hidden)
                continue;
            if(how == EXPLO_CLEAR)
            {
                des.bits.dig = tile_dig_designation::No;
                continue;
            }
            if(dm[y][x])
            {
                if(what == EXPLO_ALL
                    || (des.bits.dig == tile_dig_designation::Default && what == EXPLO_DESIGNATED)
                    || (des.bits.hidden && what == EXPLO_HIDDEN))
                {
                    des.bits.dig = tile_dig_designation::Default;
                }
            }
            else if(what == EXPLO_DESIGNATED)
            {
                des.bits.dig = tile_dig_designation::No;
            }
        }
    }
    bl->flags.bits.designated = true;
    return true;
};

command_result digexp (color_ostream &out, vector <string> & parameters)
{
    bool force_help = false;
    static explo_how how = EXPLO_NOTHING;
    static explo_what what = EXPLO_HIDDEN;
    int32_t priority = parse_priority(out, parameters);

    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i] == "help" || parameters[i] == "?")
        {
            force_help = true;
        }
        else if(parameters[i] == "all")
        {
            what = EXPLO_ALL;
        }
        else if(parameters[i] == "hidden")
        {
            what = EXPLO_HIDDEN;
        }
        else if(parameters[i] == "designated")
        {
            what = EXPLO_DESIGNATED;
        }
        else if(parameters[i] == "diag5")
        {
            how = EXPLO_DIAG5;
        }
        else if(parameters[i] == "diag5r")
        {
            how = EXPLO_DIAG5R;
        }
        else if(parameters[i] == "clear")
        {
            how = EXPLO_CLEAR;
        }
        else if(parameters[i] == "ladder")
        {
            how = EXPLO_LADDER;
        }
        else if(parameters[i] == "ladderr")
        {
            how = EXPLO_LADDERR;
        }
        else if(parameters[i] == "cross")
        {
            how = EXPLO_CROSS;
        }
    }
    if(force_help || how == EXPLO_NOTHING)
    {
        out.print(
            "This command can be used for exploratory mining.\n"
            "http://dwarffortresswiki.org/Exploratory_mining\n"
            "\n"
            "There are two variables that can be set: pattern and filter.\n"
            "Patterns:\n"
            "  diag5 = diagonals separated by 5 tiles\n"
            " diag5r = diag5 rotated 90 degrees\n"
            " ladder = A 'ladder' pattern\n"
            "ladderr = ladder rotated 90 degrees\n"
            "  clear = Just remove all dig designations\n"
            "  cross = A cross, exactly in the middle of the map.\n"
            "Filters:\n"
            " all        = designate whole z-level\n"
            " hidden     = designate only hidden tiles of z-level (default)\n"
            " designated = Take current designation and apply pattern to it.\n"
            "\n"
            "After you have a pattern set, you can use 'expdig' to apply it:\n"
            "'digexp diag5 hidden' = set filter to hidden, pattern to diag5.\n"
            "'digexp' = apply the pattern with filter.\n"
            );
        return CR_OK;
    }
    CoreSuspender suspend;
    uint32_t x_max, y_max, z_max;
    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }
    Maps::getSize(x_max,y_max,z_max);
    int32_t xzzz,yzzz,z_level;
    if(!Gui::getViewCoords(xzzz,yzzz,z_level))
    {
        out.printerr("Can't get view coords...\n");
        return CR_FAILURE;
    }
    if(how == EXPLO_DIAG5)
    {
        int which;
        for(uint32_t x = 0; x < x_max; x++)
        {
            for(uint32_t y = 0 ; y < y_max; y++)
            {
                which = (4*x + y) % 5;
                stamp_pattern(x,y_max - 1 - y, z_level, diag5[which],
                    how, what, x_max, y_max);
            }
        }
    }
    else if(how == EXPLO_DIAG5R)
    {
        int which;
        for(uint32_t x = 0; x < x_max; x++)
        {
            for(uint32_t y = 0 ; y < y_max; y++)
            {
                which = (4*x + 1000-y) % 5;
                stamp_pattern(x,y_max - 1 - y, z_level, diag5r[which],
                    how, what, x_max, y_max);
            }
        }
    }
    else if(how == EXPLO_LADDER)
    {
        int which;
        for(uint32_t x = 0; x < x_max; x++)
        {
            which = x % 3;
            for(uint32_t y = 0 ; y < y_max; y++)
            {
                stamp_pattern(x, y, z_level, ladder[which],
                    how, what, x_max, y_max);
            }
        }
    }
    else if(how == EXPLO_LADDERR)
    {
        int which;
        for(uint32_t y = 0 ; y < y_max; y++)
        {
            which = y % 3;
            for(uint32_t x = 0; x < x_max; x++)
            {
                stamp_pattern(x, y, z_level, ladderr[which],
                    how, what, x_max, y_max);
            }
        }
    }
    else if(how == EXPLO_CROSS)
    {
        // middle + recentering for the image
        int xmid = x_max * 8 - 8;
        int ymid = y_max * 8 - 8;
        MapExtras::MapCache mx;
        for(int x = 0; x < 16; x++)
            for(int y = 0; y < 16; y++)
            {
                DFCoord pos(xmid+x,ymid+y,z_level);
                df::tiletype tt = mx.tiletypeAt(pos);
                if(tt == tiletype::Void)
                    continue;
                df::tile_designation des = mx.designationAt(pos);
                if(tileMaterial(tt) == tiletype_material::CONSTRUCTION && !des.bits.hidden)
                    continue;
                if(!isWallTerrain(tt) && !des.bits.hidden)
                    continue;
                if(cross[y][x])
                {
                    des.bits.dig = tile_dig_designation::Default;
                    mx.setDesignationAt(pos,des,priority);
                }
            }
        mx.WriteAll();
    }
    else for(uint32_t x = 0; x < x_max; x++)
    {
        for(uint32_t y = 0 ; y < y_max; y++)
        {
            stamp_pattern(x, y, z_level, all_tiles,
                how, what, x_max, y_max);
        }
    }
    return CR_OK;
}

command_result digvx (color_ostream &out, vector <string> & parameters)
{
    // HOTKEY COMMAND: CORE ALREADY SUSPENDED
    vector <string> lol;
    lol.push_back("x");
    lol.push_back(forward_priority(out, parameters));
    return digv(out,lol);
}

command_result digv (color_ostream &out, vector <string> & parameters)
{
    // HOTKEY COMMAND: CORE ALREADY SUSPENDED
    uint32_t x_max,y_max,z_max;
    bool updown = false;
    int32_t priority = parse_priority(out, parameters);

    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters.size() && parameters[0]=="x")
            updown = true;
        else
            return CR_WRONG_USAGE;
    }

    auto &con = out;

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    int32_t cx, cy, cz;
    Maps::getSize(x_max,y_max,z_max);
    uint32_t tx_max = x_max * 16;
    uint32_t ty_max = y_max * 16;
    Gui::getCursorCoords(cx,cy,cz);
    while(cx == -30000)
    {
        con.printerr("Cursor is not active. Point the cursor at a vein.\n");
        return CR_FAILURE;
    }
    DFHack::DFCoord xy ((uint32_t)cx,(uint32_t)cy,cz);
    if(xy.x == 0 || xy.x == int32_t(tx_max) - 1 || xy.y == 0 || xy.y == int32_t(ty_max) - 1)
    {
        con.printerr("I won't dig the borders. That would be cheating!\n");
        return CR_FAILURE;
    }
    std::unique_ptr<MapExtras::MapCache> MCache = std::make_unique<MapExtras::MapCache>();
    df::tile_designation des = MCache->designationAt(xy);
    df::tiletype tt = MCache->tiletypeAt(xy);
    int16_t veinmat = MCache->veinMaterialAt(xy);
    if( veinmat == -1 )
    {
        con.printerr("This tile is not a vein.\n");
        return CR_FAILURE;
    }
    con.print("%d/%d/%d tiletype: %d, veinmat: %d, designation: 0x%x ... DIGGING!\n", cx,cy,cz, tt, veinmat, des.whole);
    stack <DFHack::DFCoord> flood;
    flood.push(xy);

    while( !flood.empty() )
    {
        DFHack::DFCoord current = flood.top();
        flood.pop();
        if (MCache->tagAt(current))
            continue;
        int16_t vmat2 = MCache->veinMaterialAt(current);
        tt = MCache->tiletypeAt(current);
        if(!DFHack::isWallTerrain(tt))
            continue;
        if(vmat2!=veinmat)
            continue;

        // found a good tile, dig+unset material
        df::tile_designation des = MCache->designationAt(current);
        df::tile_designation des_minus;
        df::tile_designation des_plus;
        des_plus.whole = des_minus.whole = 0;
        int16_t vmat_minus = -1;
        int16_t vmat_plus = -1;
        bool below = 0;
        bool above = 0;
        if(updown)
        {
            if(MCache->testCoord(current-1))
            {
                below = 1;
                des_minus = MCache->designationAt(current-1);
                vmat_minus = MCache->veinMaterialAt(current-1);
            }

            if(MCache->testCoord(current+1))
            {
                above = 1;
                des_plus = MCache->designationAt(current+1);
                vmat_plus = MCache->veinMaterialAt(current+1);
            }
        }
        if(MCache->testCoord(current))
        {
            MCache->setTagAt(current, 1);

            if(current.x < int32_t(tx_max) - 2)
            {
                flood.push(DFHack::DFCoord(current.x + 1, current.y, current.z));
                if(current.y < int32_t(ty_max) - 2)
                {
                    flood.push(DFHack::DFCoord(current.x + 1, current.y + 1,current.z));
                    flood.push(DFHack::DFCoord(current.x, current.y + 1,current.z));
                }
                if(current.y > 1)
                {
                    flood.push(DFHack::DFCoord(current.x + 1, current.y - 1,current.z));
                    flood.push(DFHack::DFCoord(current.x, current.y - 1,current.z));
                }
            }
            if(current.x > 1)
            {
                flood.push(DFHack::DFCoord(current.x - 1, current.y,current.z));
                if(current.y < int32_t(ty_max) - 2)
                {
                    flood.push(DFHack::DFCoord(current.x - 1, current.y + 1,current.z));
                    flood.push(DFHack::DFCoord(current.x, current.y + 1,current.z));
                }
                if(current.y > 1)
                {
                    flood.push(DFHack::DFCoord(current.x - 1, current.y - 1,current.z));
                    flood.push(DFHack::DFCoord(current.x, current.y - 1,current.z));
                }
            }
            if(updown)
            {
                if(current.z > 0 && below && vmat_minus == vmat2)
                {
                    flood.push(current-1);

                    if(des_minus.bits.dig == tile_dig_designation::DownStair)
                        des_minus.bits.dig = tile_dig_designation::UpDownStair;
                    else
                        des_minus.bits.dig = tile_dig_designation::UpStair;
                    MCache->setDesignationAt(current-1,des_minus,priority);

                    des.bits.dig = tile_dig_designation::DownStair;
                }
                if(current.z < int32_t(z_max) - 1 && above && vmat_plus == vmat2)
                {
                    flood.push(current+ 1);

                    if(des_plus.bits.dig == tile_dig_designation::UpStair)
                        des_plus.bits.dig = tile_dig_designation::UpDownStair;
                    else
                        des_plus.bits.dig = tile_dig_designation::DownStair;
                    MCache->setDesignationAt(current+1,des_plus,priority);

                    if(des.bits.dig == tile_dig_designation::DownStair)
                        des.bits.dig = tile_dig_designation::UpDownStair;
                    else
                        des.bits.dig = tile_dig_designation::UpStair;
                }
            }
            if(des.bits.dig == tile_dig_designation::No)
                des.bits.dig = tile_dig_designation::Default;
            MCache->setDesignationAt(current,des,priority);
        }
    }
    MCache->WriteAll();
    return CR_OK;
}

command_result diglx (color_ostream &out, vector <string> & parameters)
{
    // HOTKEY COMMAND: CORE ALREADY SUSPENDED
    vector <string> lol;
    lol.push_back("x");
    lol.push_back(forward_priority(out, parameters));
    return digl(out,lol);
}

// TODO:
// digl and digv share the longish floodfill code and only use different conditions
// to check if a tile should be marked for digging or not.
// to make the plugin a bit smaller and cleaner a main execute method would be nice
// (doing the floodfill stuff and do the checks dependin on whether called in
// "vein" or "layer" mode)
command_result digl (color_ostream &out, vector <string> & parameters)
{
    // HOTKEY COMMAND: CORE ALREADY SUSPENDED
    uint32_t x_max,y_max,z_max;
    bool updown = false;
    bool undo = false;
    int32_t priority = parse_priority(out, parameters);

    for(size_t i = 0; i < parameters.size();i++)
    {
        if(parameters[i]=="x")
        {
            out << "This might take a while for huge layers..." << std::endl;
            updown = true;
        }
        else if(parameters[i]=="undo")
        {
            out << "Removing dig designation." << std::endl;
            undo = true;
        }
        else
            return CR_WRONG_USAGE;
    }

    auto &con = out;

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    int32_t cx, cy, cz;
    Maps::getSize(x_max,y_max,z_max);
    uint32_t tx_max = x_max * 16;
    uint32_t ty_max = y_max * 16;
    Gui::getCursorCoords(cx,cy,cz);
    while(cx == -30000)
    {
        con.printerr("Cursor is not active. Point the cursor at a vein.\n");
        return CR_FAILURE;
    }
    DFHack::DFCoord xy ((uint32_t)cx,(uint32_t)cy,cz);
    if(xy.x == 0 || xy.x == int32_t(tx_max) - 1 || xy.y == 0 || xy.y == int32_t(ty_max) - 1)
    {
        con.printerr("I won't dig the borders. That would be cheating!\n");
        return CR_FAILURE;
    }
    std::unique_ptr<MapExtras::MapCache> MCache = std::make_unique<MapExtras::MapCache>();
    df::tile_designation des = MCache->designationAt(xy);
    df::tiletype tt = MCache->tiletypeAt(xy);
    int16_t veinmat = MCache->veinMaterialAt(xy);
    int16_t basemat = MCache->layerMaterialAt(xy);
    if( veinmat != -1 )
    {
        con.printerr("This is a vein. Use digv instead!\n");
        return CR_FAILURE;
    }
    con.print("%d/%d/%d tiletype: %d, basemat: %d, designation: 0x%x ... DIGGING!\n", cx,cy,cz, tt, basemat, des.whole);
    stack <DFHack::DFCoord> flood;
    flood.push(xy);

    while( !flood.empty() )
    {
        DFHack::DFCoord current = flood.top();
        flood.pop();
        if (MCache->tagAt(current))
            continue;
        int16_t vmat2 = MCache->veinMaterialAt(current);
        int16_t bmat2 = MCache->layerMaterialAt(current);
        tt = MCache->tiletypeAt(current);

        if(!DFHack::isWallTerrain(tt))
            continue;
        if(vmat2!=-1)
            continue;
        if(bmat2!=basemat)
            continue;

        // don't dig out LAVA_STONE or MAGMA (semi-molten rock) accidentally
        if(    tileMaterial(tt)!=tiletype_material::STONE
            && tileMaterial(tt)!=tiletype_material::SOIL)
            continue;

        // found a good tile, dig+unset material
        df::tile_designation des = MCache->designationAt(current);
        df::tile_designation des_minus;
        df::tile_designation des_plus;
        des_plus.whole = des_minus.whole = 0;
        int16_t vmat_minus = -1;
        int16_t vmat_plus = -1;
        int16_t bmat_minus = -1;
        int16_t bmat_plus = -1;
        df::tiletype tt_minus;
        df::tiletype tt_plus;

        if(MCache->testCoord(current))
        {
            MCache->setTagAt(current, 1);
            if(current.x < int32_t(tx_max) - 2)
            {
                flood.push(DFHack::DFCoord(current.x + 1, current.y, current.z));
                if(current.y < int32_t(ty_max) - 2)
                {
                    flood.push(DFHack::DFCoord(current.x + 1, current.y + 1, current.z));
                    flood.push(DFHack::DFCoord(current.x, current.y + 1, current.z));
                }
                if(current.y > 1)
                {
                    flood.push(DFHack::DFCoord(current.x + 1, current.y - 1, current.z));
                    flood.push(DFHack::DFCoord(current.x, current.y - 1, current.z));
                }
            }
            if(current.x > 1)
            {
                flood.push(DFHack::DFCoord(current.x - 1, current.y, current.z));
                if(current.y < int32_t(ty_max) - 2)
                {
                    flood.push(DFHack::DFCoord(current.x - 1, current.y + 1, current.z));
                    flood.push(DFHack::DFCoord(current.x, current.y + 1, current.z));
                }
                if(current.y > 1)
                {
                    flood.push(DFHack::DFCoord(current.x - 1, current.y - 1, current.z));
                    flood.push(DFHack::DFCoord(current.x, current.y - 1, current.z));
                }
            }
            if(updown)
            {
                bool below = 0;
                bool above = 0;
                if(MCache->testCoord(current-1))
                {
                    //below = 1;
                    des_minus = MCache->designationAt(current-1);
                    vmat_minus = MCache->veinMaterialAt(current-1);
                    bmat_minus = MCache->layerMaterialAt(current-1);
                    tt_minus = MCache->tiletypeAt(current-1);
                    if (   tileMaterial(tt_minus)==tiletype_material::STONE
                        || tileMaterial(tt_minus)==tiletype_material::SOIL)
                        below = 1;
                }
                if(MCache->testCoord(current+1))
                {
                    //above = 1;
                    des_plus = MCache->designationAt(current+1);
                    vmat_plus = MCache->veinMaterialAt(current+1);
                    bmat_plus = MCache->layerMaterialAt(current+1);
                    tt_plus = MCache->tiletypeAt(current+1);
                    if (   tileMaterial(tt_plus)==tiletype_material::STONE
                        || tileMaterial(tt_plus)==tiletype_material::SOIL)
                        above = 1;
                }
                if(current.z > 0 && below && vmat_minus == -1 && bmat_minus == basemat)
                {
                    flood.push(current-1);

                    if(des_minus.bits.dig == tile_dig_designation::DownStair)
                        des_minus.bits.dig = tile_dig_designation::UpDownStair;
                    else
                        des_minus.bits.dig = tile_dig_designation::UpStair;
                    // undo mode: clear designation
                    if(undo)
                        des_minus.bits.dig = tile_dig_designation::No;
                    MCache->setDesignationAt(current-1,des_minus,priority);

                    des.bits.dig = tile_dig_designation::DownStair;
                }
                if(current.z < int32_t(z_max) - 1 && above && vmat_plus == -1 && bmat_plus == basemat)
                {
                    flood.push(current+ 1);

                    if(des_plus.bits.dig == tile_dig_designation::UpStair)
                        des_plus.bits.dig = tile_dig_designation::UpDownStair;
                    else
                        des_plus.bits.dig = tile_dig_designation::DownStair;
                    // undo mode: clear designation
                    if(undo)
                        des_plus.bits.dig = tile_dig_designation::No;
                    MCache->setDesignationAt(current+1,des_plus,priority);

                    if(des.bits.dig == tile_dig_designation::DownStair)
                        des.bits.dig = tile_dig_designation::UpDownStair;
                    else
                        des.bits.dig = tile_dig_designation::UpStair;
                }
            }
            if(des.bits.dig == tile_dig_designation::No)
                des.bits.dig = tile_dig_designation::Default;
            // undo mode: clear designation
            if(undo)
                des.bits.dig = tile_dig_designation::No;
            MCache->setDesignationAt(current,des,priority);
        }
    }
    MCache->WriteAll();
    return CR_OK;
}

command_result digtype (color_ostream &out, vector <string> & parameters)
{
    //mostly copy-pasted from digv
    int32_t priority = parse_priority(out, parameters);
    CoreSuspender suspend;

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return CR_FAILURE;
    }

    uint32_t zMin = 0;
    uint32_t xMax,yMax,zMax;
    Maps::getSize(xMax,yMax,zMax);

    bool hidden = false;
    bool automine = true;

    int32_t targetDigType = -1;
    for (string parameter : parameters) {
        if ( parameter == "clear" )
            targetDigType = tile_dig_designation::No;
        else if ( parameter == "dig" )
            targetDigType = tile_dig_designation::Default;
        else if ( parameter == "updown" )
            targetDigType = tile_dig_designation::UpDownStair;
        else if ( parameter == "channel" )
            targetDigType = tile_dig_designation::Channel;
        else if ( parameter == "ramp" )
            targetDigType = tile_dig_designation::Ramp;
        else if ( parameter == "down" )
            targetDigType = tile_dig_designation::DownStair;
        else if ( parameter == "up" )
            targetDigType = tile_dig_designation::UpStair;
        else if ( parameter == "-z" || parameter == "--cur-zlevel" )
            {zMax = *window_z + 1; zMin = *window_z;}
        else if ( parameter == "--zdown" || parameter == "-d")
            zMax = *window_z + 1;
        else if ( parameter == "--zup" || parameter == "-u")
            zMin = *window_z;
        else if ( parameter == "--hidden" || parameter == "-h")
            hidden = true;
        else if ( parameter == "--no-auto" || parameter == "-a" )
            automine = false;
        else
        {
            out.printerr("Invalid parameter: '%s'.\n", parameter.c_str());
            return CR_FAILURE;
        }
    }

    int32_t cx, cy, cz;
    uint32_t tileXMax = xMax * 16;
    uint32_t tileYMax = yMax * 16;
    Gui::getCursorCoords(cx,cy,cz);
    if (cx == -30000)
    {
        out.printerr("Cursor is not active. Point the cursor at a vein.\n");
        return CR_FAILURE;
    }
    DFHack::DFCoord xy ((uint32_t)cx,(uint32_t)cy,cz);
    std::unique_ptr<MapExtras::MapCache> mCache = std::make_unique<MapExtras::MapCache>();
    df::tile_designation baseDes = mCache->designationAt(xy);

    if (baseDes.bits.hidden && !hidden) {
        out.printerr("Cursor is pointing at a hidden tile. Point the cursor at a visible tile when using the --hidden option.\n");
        return CR_FAILURE;
    }

    df::tile_occupancy baseOcc = mCache->occupancyAt(xy);

    df::tiletype tt = mCache->tiletypeAt(xy);
    int16_t veinmat = mCache->veinMaterialAt(xy);
    if( veinmat == -1 )
    {
        out.printerr("This tile is not a vein.\n");
        return CR_FAILURE;
    }
    out.print("(%d,%d,%d) tiletype: %d, veinmat: %d, designation: 0x%x ... DIGGING!\n", cx,cy,cz, tt, veinmat, baseDes.whole);

    if ( targetDigType != -1 )
    {
        baseDes.bits.dig = (tile_dig_designation::tile_dig_designation)targetDigType;
    }
    else
    {
        if ( baseDes.bits.dig == tile_dig_designation::No )
        {
            baseDes.bits.dig = tile_dig_designation::Default;
        }
    }
    // Auto dig only works on default dig designation. Setting dig_auto for any other designation
    // prevents dwarves from digging that tile at all.
    if (baseDes.bits.dig == tile_dig_designation::Default && automine) baseOcc.bits.dig_auto = true;
    else baseOcc.bits.dig_auto = false;

    for( uint32_t z = zMin; z < zMax; z++ )
    {
        for( uint32_t x = 1; x < tileXMax-1; x++ )
        {
            for( uint32_t y = 1; y < tileYMax-1; y++ )
            {
                DFHack::DFCoord current(x,y,z);
                int16_t vmat2 = mCache->veinMaterialAt(current);
                if ( vmat2 != veinmat )
                    continue;
                tt = mCache->tiletypeAt(current);
                if (!DFHack::isWallTerrain(tt))
                    continue;
                if (tileMaterial(tt) != df::enums::tiletype_material::MINERAL)
                    continue;

                //designate it for digging
                if ( !mCache->testCoord(current) )
                {
                    out.printerr("testCoord failed at (%d,%d,%d)\n", x, y, z);
                    return CR_FAILURE;
                }

                df::tile_designation designation = mCache->designationAt(current);

                if (designation.bits.hidden && !hidden) continue;

                df::tile_occupancy occupancy = mCache->occupancyAt(current);
                designation.bits.dig = baseDes.bits.dig;
                occupancy.bits.dig_auto = baseOcc.bits.dig_auto;
                mCache->setDesignationAt(current, designation, priority);
                mCache->setOccupancyAt(current, occupancy);
            }
        }
    }

    mCache->WriteAll();
    return CR_OK;
}

/////////////////////////////////////////////////////
// Lua API
//

static void setWarmPaintEnabled(color_ostream &out, bool val) {
    is_painting_warm = val;
}

static bool getWarmPaintEnabled(color_ostream &out) {
    return is_painting_warm;
}

static void setDampPaintEnabled(color_ostream &out, bool val) {
    is_painting_damp = val;
}

static bool getDampPaintEnabled(color_ostream &out) {
    return is_painting_damp;
}

static void mark_cur_level(color_ostream &out, PersistentDataItem &config) {
    std::unordered_map<df::coord, df::job *> dig_jobs;
    fill_dig_jobs(dig_jobs);

    std::unordered_set<df::job *> z_jobs;

    bool did_set_assignment = false;
    const int z = *window_z;
    for (auto & block : world->map.map_blocks) {
        if (block->map_pos.z != z)
            continue;

        const auto & block_pos = block->map_pos;
        df::tile_bitmask *mask = NULL;
        for (uint32_t x = 0; x < 16; x++) for (uint32_t y = 0; y < 16; y++) {
            df::coord pos = block_pos + df::coord(x, y, 0);
            if (dig_jobs.contains(pos)) {
                z_jobs.emplace(dig_jobs[pos]);
                continue;
            }

            if (!block->designation[x][y].bits.dig)
                continue;

            if (!mask)
                mask = World::getPersistentTilemask(config, block, true);
            if (!mask) {
                WARN(log,out).print("unable to allocate tile bitmask\n");
                return;
            }

            mask->setassignment(x, y, true);
            did_set_assignment = true;
        }
    }

    for (auto job : z_jobs)
        unhide_surrounding_tagged_tiles(out, job);

    if (did_set_assignment)
        do_enable(true);
}

static void markCurLevelWarmDig(color_ostream &out) {
    mark_cur_level(out, warm_config);
}

static void markCurLevelDampDig(color_ostream &out) {
    mark_cur_level(out, damp_config);
}

static void update_tile_mask(const df::coord & pos, std::unordered_map<df::coord, df::job *> & dig_jobs) {
    auto block = Maps::getTileBlock(pos);
    auto des = Maps::getTileDesignation(pos);
    if (!block || !des)
        return;

    if (des->bits.dig == df::tile_dig_designation::No && !dig_jobs.contains(pos)) {
        if (auto warm_mask = World::getPersistentTilemask(warm_config, block))
            warm_mask->setassignment(pos, false);
        if (auto damp_mask = World::getPersistentTilemask(damp_config, block))
            damp_mask->setassignment(pos, false);
    } else {
        if (is_painting_warm)
            if (auto warm_mask = World::getPersistentTilemask(warm_config, block, true)) {
                warm_mask->setassignment(pos, true);
                do_enable(true);
            }
        if (is_painting_damp)
            if (auto damp_mask = World::getPersistentTilemask(damp_config, block, true)) {
                damp_mask->setassignment(pos, true);
                do_enable(true);
            }
    }
}

static int registerWarmDampTile(lua_State *L) {
    TRACE(log).print("entering registerWarmDampTile\n");
    df::coord pos;
    Lua::CheckDFAssign(L, &pos, 1);

    std::unordered_map<df::coord, df::job *> dig_jobs;
    fill_dig_jobs(dig_jobs);

    update_tile_mask(pos, dig_jobs);
    return 0;
}

static bool get_int_field(lua_State *L, int idx, const char *name, int16_t *dest) {
    lua_getfield(L, idx, name);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return false;
    }
    *dest = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return true;
}

static bool get_bounds(lua_State *L, int idx, df::coord &pos1, df::coord &pos2) {
    return get_int_field(L, idx, "x1", &pos1.x) &&
        get_int_field(L, idx, "y1", &pos1.y) &&
        get_int_field(L, idx, "z1", &pos1.z) &&
        get_int_field(L, idx, "x2", &pos2.x) &&
        get_int_field(L, idx, "y2", &pos2.y) &&
        get_int_field(L, idx, "z2", &pos2.z);
}

static int registerWarmDampBox(lua_State *L) {
    TRACE(log).print("entering registerWarmDampBox\n");
    df::coord pos_start, pos_end;
    if (!get_bounds(L, 1, pos_start, pos_end)) {
        luaL_argerror(L, 1, "invalid box bounds");
        return 0;
    }

    std::unordered_map<df::coord, df::job *> dig_jobs;
    fill_dig_jobs(dig_jobs);

    for (int32_t z = pos_start.z; z <= pos_end.z; ++z)
        for (int32_t y = pos_start.y; y <= pos_end.y; ++y)
            for (int32_t x = pos_start.x; x <= pos_end.x; ++x)
                update_tile_mask(df::coord(x, y, z), dig_jobs);

    return 0;
}

static void bump_layers(const Screen::Pen &pen, int x, int y) {
    Screen::Pen signpost_pen = Screen::readTile(x, y, true, &df::graphic_viewportst::screentexpos_signpost);
    Screen::Pen desig_pen = Screen::readTile(x, y, true, &df::graphic_viewportst::screentexpos_designation);
    if (signpost_pen.valid())
        Screen::paintTile(signpost_pen, x, y, true, &df::graphic_viewportst::screentexpos_background_two);
    if (desig_pen.valid())
        Screen::paintTile(desig_pen, x, y, true, &df::graphic_viewportst::screentexpos_signpost);
    Screen::paintTile(pen, x, y, true, &df::graphic_viewportst::screentexpos_designation);
}

static bool blink(int delay) {
    return (Core::getInstance().p->getTickCount()/delay) % 2 == 0;
}

static void paintScreenWarmDamp(bool show_hidden = false) {
    TRACE(log).print("entering paintScreenDampWarm\n");

    static Screen::Pen empty_pen;

    int warm_texpos = 0, damp_texpos = 0;
    Screen::findGraphicsTile("MINING_INDICATORS", 1, 0, &warm_texpos);
    Screen::findGraphicsTile("MINING_INDICATORS", 0, 0, &damp_texpos);
    Screen::Pen warm_pen, damp_pen;
    warm_pen.tile = warm_texpos;
    damp_pen.tile = damp_texpos;

    long warm_dig_texpos = Textures::getTexposByHandle(textures[1]);
    long damp_dig_texpos = Textures::getTexposByHandle(textures[2]);
    long light_aq_texpos = Textures::getTexposByHandle(textures[7]);
    long heavy_aq_texpos = Textures::getTexposByHandle(textures[8]);
    Screen::Pen warm_dig_pen, damp_dig_pen, light_aq_pen, heavy_aq_pen;
    warm_dig_pen.tile = warm_dig_texpos;
    damp_dig_pen.tile = damp_dig_texpos;
    light_aq_pen.tile = light_aq_texpos;
    heavy_aq_pen.tile = heavy_aq_texpos;

    auto dims = Gui::getDwarfmodeViewDims().map();
    for (int y = dims.first.y; y <= dims.second.y; ++y) {
        for (int x = dims.first.x; x <= dims.second.x; ++x) {
            df::coord pos(*window_x + x, *window_y + y, *window_z);

            auto block = Maps::getTileBlock(pos);
            if (!block)
                continue;

            if (Screen::inGraphicsMode()) {
                if (auto warm_mask = World::getPersistentTilemask(warm_config, block)) {
                    if (warm_mask->getassignment(pos))
                        bump_layers(warm_dig_pen, x, y);
                }
                if (auto damp_mask = World::getPersistentTilemask(damp_config, block)) {
                    if (damp_mask->getassignment(pos))
                        bump_layers(damp_dig_pen, x, y);
                }
            }

            if (!show_hidden && !Maps::isTileVisible(pos)) {
                TRACE(log).print("skipping hidden tile\n");
                continue;
            }

            if (Screen::inGraphicsMode()) {
                Screen::Pen *pen = NULL;
                if (is_warm(pos) && is_wall(pos)) {
                    pen = &warm_pen;
                } else if (is_aquifer(pos)) {
                    pen = is_heavy_aquifer(pos, block) ? &heavy_aq_pen : &light_aq_pen;
                } else if (is_wall(pos) && is_damp(pos)) {
                    pen = &damp_pen;
                }
                if (pen) {
                    int existing_tile = Screen::readTile(x, y, true).tile;
                    if (existing_tile == damp_texpos)
                        Screen::paintTile(empty_pen, x, y, true);
                    bump_layers(*pen, x, y);
                }
            } else {
                TRACE(log).print("scanning map tile at (%d, %d, %d) screen offset (%d, %d)\n",
                    pos.x, pos.y, pos.z, x, y);

                auto des = Maps::getTileDesignation(pos);
                if (des && des->bits.dig != df::tile_dig_designation::No) {
                    if (blink(1000))
                        continue;
                }

                Screen::Pen cur_tile = Screen::readTile(x, y, true);
                if (!cur_tile.valid()) {
                    DEBUG(log).print("cannot read tile at offset %d, %d\n", x, y);
                    continue;
                }

                int color = COLOR_BLACK;

                if (auto warm_mask = World::getPersistentTilemask(warm_config, block)) {
                    if (warm_mask->getassignment(pos) && blink(500)) {
                        color = COLOR_LIGHTRED;
                        auto damp_mask = World::getPersistentTilemask(damp_config, block);
                        if (damp_mask && damp_mask->getassignment(pos) && blink(2000))
                            color = COLOR_BLUE;
                    }
                }
                if (color == COLOR_BLACK) {
                    if (auto damp_mask = World::getPersistentTilemask(damp_config, block)) {
                        if (damp_mask->getassignment(pos) && blink(500))
                            color = COLOR_BLUE;
                    }
                }
                if (color == COLOR_BLACK && is_warm(pos)) {
                    color = COLOR_RED;
                }
                if (color == COLOR_BLACK && is_damp(pos)) {
                    if (!is_aquifer(pos, des) || blink(is_heavy_aquifer(pos, block) ? 500 : 2000))
                        color = COLOR_LIGHTBLUE;
                }

                if (color == COLOR_BLACK) {
                    TRACE(log).print("no need to modify tile; skipping\n");
                    continue;
                }

                // this will also change the color of the cursor or any selection box that overlaps
                // the tile. this is intentional since it makes the UI more readable. it will also change
                // the color of any UI elements (e.g. info sheets) that happen to overlap the map
                // on that tile. this is undesirable, but unavoidable, since we can't know which tiles
                // the UI is overwriting.
                if (cur_tile.fg && cur_tile.ch != ' ') {
                    cur_tile.fg = color;
                    cur_tile.bg = 0;
                } else {
                    cur_tile.fg = 0;
                    cur_tile.bg = color;
                }

                cur_tile.bold = false;

                if (cur_tile.tile)
                    cur_tile.tile_mode = Screen::Pen::CharColor;

                Screen::paintTile(cur_tile, x, y, true);
            }
        }
    }
}

struct designation{
    df::coord pos;
    df::tile_designation td;
    df::tile_occupancy to;
    designation() = default;
    designation(const df::coord &c, const df::tile_designation &td, const df::tile_occupancy &to) : pos(c), td(td), to(to) {}

    bool operator==(const designation &rhs) const {
        return pos == rhs.pos;
    }

    bool operator!=(const designation &rhs) const {
        return !(rhs == *this);
    }
};

namespace std {
    template<>
    struct hash<designation> {
        std::size_t operator()(const designation &c) const {
            std::hash<df::coord> hash_coord;
            return hash_coord(c.pos);
        }
    };
}

class Designations {
private:
    std::unordered_map<df::coord, designation> designations;
public:
    Designations() {
        df::job_list_link *link = world->jobs.list.next;
        for (; link; link = link->next) {
            df::job *job = link->item;

            if(!job || !Maps::isValidTilePos(job->pos))
                continue;

            df::tile_designation td;
            df::tile_occupancy to;
            bool keep_if_taken = false;

            switch (job->job_type) {
                case df::job_type::SmoothWall:
                case df::job_type::SmoothFloor:
                    keep_if_taken = true;
                    // fallthrough
                case df::job_type::CarveFortification:
                    td.bits.smooth = 1;
                    break;
                case df::job_type::DetailWall:
                case df::job_type::DetailFloor:
                    td.bits.smooth = 2;
                    break;
                case job_type::CarveTrack:
                    to.bits.carve_track_north = (job->item_category.whole >> 18) & 1;
                    to.bits.carve_track_south = (job->item_category.whole >> 19) & 1;
                    to.bits.carve_track_west = (job->item_category.whole >> 20) & 1;
                    to.bits.carve_track_east = (job->item_category.whole >> 21) & 1;
                    break;
                default:
                    continue;
            }
            if (keep_if_taken || !Job::getWorker(job))
                designations.emplace(job->pos, designation(job->pos, td, to));
        }
    }

    // get from job; if no job, then fall back to querying map
    designation get(const df::coord &pos) const {
        if (designations.count(pos)) {
            return designations.at(pos);
        }
        auto pdes = Maps::getTileDesignation(pos);
        auto pocc = Maps::getTileOccupancy(pos);
        if (!pdes || !pocc)
            return {};
        return designation(pos, *pdes, *pocc);
    }
};

static bool is_designated_for_smoothing(const designation &designation) {
    return designation.td.bits.smooth == 1;
}

static bool is_designated_for_engraving(const designation &designation) {
    return designation.td.bits.smooth == 2;
}

static bool is_designated_for_track_carving(const designation &designation) {
    const df::tile_occupancy &occ = designation.to;
    return occ.bits.carve_track_east || occ.bits.carve_track_north || occ.bits.carve_track_south || occ.bits.carve_track_west;
}

static char get_track_char(const designation &designation) {
    const df::tile_occupancy &occ = designation.to;
    if (occ.bits.carve_track_east && occ.bits.carve_track_north && occ.bits.carve_track_south && occ.bits.carve_track_west)
        return (char)0xCE; // NSEW
    if (occ.bits.carve_track_east && occ.bits.carve_track_north && occ.bits.carve_track_south)
        return (char)0xCC; // NSE
    if (occ.bits.carve_track_east && occ.bits.carve_track_north && occ.bits.carve_track_west)
        return (char)0xCA; // NEW
    if (occ.bits.carve_track_east && occ.bits.carve_track_south && occ.bits.carve_track_west)
        return (char)0xCB; // SEW
    if (occ.bits.carve_track_north && occ.bits.carve_track_south && occ.bits.carve_track_west)
        return (char)0xB9; // NSW
    if (occ.bits.carve_track_north && occ.bits.carve_track_south)
        return (char)0xBA; // NS
    if (occ.bits.carve_track_east && occ.bits.carve_track_west)
        return (char)0xCD; // EW
    if (occ.bits.carve_track_east && occ.bits.carve_track_north)
        return (char)0xC8; // NE
    if (occ.bits.carve_track_north && occ.bits.carve_track_west)
        return (char)0xBC; // NW
    if (occ.bits.carve_track_east && occ.bits.carve_track_south)
        return (char)0xC9; // SE
    if (occ.bits.carve_track_south && occ.bits.carve_track_west)
        return (char)0xBB; // SW
    if (occ.bits.carve_track_north)
        return (char)0xD0; // N
    if (occ.bits.carve_track_south)
        return (char)0xD2; // S
    if (occ.bits.carve_track_east)
        return (char)0xC6; // E
    if (occ.bits.carve_track_west)
        return (char)0xB5; // W
    return (char)0xC5; // single line cross; should never happen
}

static char get_tile_char(const df::coord &pos, char desig_char, bool draw_priority) {
    if (!draw_priority)
        return desig_char;

    std::vector<df::block_square_event_designation_priorityst *> priorities;
    Maps::SortBlockEvents(Maps::getTileBlock(pos), NULL, NULL, NULL, NULL, NULL, NULL, NULL, &priorities);
    if (priorities.empty())
        return desig_char;
    switch (priorities[0]->priority[pos.x % 16][pos.y % 16] / 1000) {
    case 1: return '1';
    case 2: return '2';
    case 3: return '3';
    case 4: return '4';
    case 5: return '5';
    case 6: return '6';
    case 7: return '7';
    default:
        return '4';
    }
}

static void paintScreenCarve() {
    TRACE(log).print("entering paintScreenCarve\n");

    if (Screen::inGraphicsMode() || blink(500))
        return;

    Designations designations;
    bool draw_priority = blink(1000);

    auto dims = Gui::getDwarfmodeViewDims().map();
    for (int y = dims.first.y; y <= dims.second.y; ++y) {
        for (int x = dims.first.x; x <= dims.second.x; ++x) {
            df::coord map_pos(*window_x + x, *window_y + y, *window_z);

            if (!Maps::isValidTilePos(map_pos))
                continue;

            if (!Maps::isTileVisible(map_pos)) {
                TRACE(log).print("skipping hidden tile\n");
                continue;
            }

            TRACE(log).print("scanning map tile at (%d, %d, %d) screen offset (%d, %d)\n",
                map_pos.x, map_pos.y, map_pos.z, x, y);

            Screen::Pen cur_tile;
            cur_tile.fg = COLOR_DARKGREY;

            auto des = designations.get(map_pos);

            if (is_designated_for_smoothing(des)) {
                if (is_smooth_wall(map_pos))
                    cur_tile.ch = get_tile_char(map_pos, (char)206, draw_priority); // octothorpe, indicating a fortification designation
                else
                    cur_tile.ch = get_tile_char(map_pos, (char)219, draw_priority); // solid block, indicating a smoothing designation
            }
            else if (is_designated_for_engraving(des)) {
                cur_tile.ch = get_tile_char(map_pos, (char)10, draw_priority); // solid block with a circle on it
            }
            else if (is_designated_for_track_carving(des)) {
                cur_tile.ch = get_tile_char(map_pos, get_track_char(des), draw_priority); // directional track
            }
            else {
                TRACE(log).print("skipping tile with no carving designation\n");
                continue;
            }

            Screen::paintTile(cur_tile, x, y, true);
        }
    }
}

DFHACK_PLUGIN_LUA_COMMANDS{
    DFHACK_LUA_COMMAND(registerWarmDampTile),
    DFHACK_LUA_COMMAND(registerWarmDampBox),
    DFHACK_LUA_END,
};

DFHACK_PLUGIN_LUA_FUNCTIONS{
    DFHACK_LUA_FUNCTION(setWarmPaintEnabled),
    DFHACK_LUA_FUNCTION(getWarmPaintEnabled),
    DFHACK_LUA_FUNCTION(setDampPaintEnabled),
    DFHACK_LUA_FUNCTION(getDampPaintEnabled),
    DFHACK_LUA_FUNCTION(markCurLevelWarmDig),
    DFHACK_LUA_FUNCTION(markCurLevelDampDig),
    DFHACK_LUA_FUNCTION(paintScreenWarmDamp),
    DFHACK_LUA_FUNCTION(paintScreenCarve),
    DFHACK_LUA_END
};
