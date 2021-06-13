/**
 * Translates a region of tiles specified by the cursor and arguments/prompts
 * into a series of blueprint files suitable for replay via quickfort.
 *
 * Written by cdombroski.
 */

#include <algorithm>
#include <sstream>

#include "Console.h"
#include "DataDefs.h"
#include "DataFuncs.h"
#include "DataIdentity.h"
#include "LuaTools.h"
#include "PluginManager.h"
#include "TileTypes.h"

#include "modules/Buildings.h"
#include "modules/Filesystem.h"
#include "modules/Gui.h"

#include "df/building_axle_horizontalst.h"
#include "df/building_bridgest.h"
#include "df/building_constructionst.h"
#include "df/building_furnacest.h"
#include "df/building_rollersst.h"
#include "df/building_screw_pumpst.h"
#include "df/building_siegeenginest.h"
#include "df/building_trapst.h"
#include "df/building_water_wheelst.h"
#include "df/building_workshopst.h"
#include "df/world.h"

using std::string;
using std::endl;
using std::vector;
using std::ofstream;
using std::pair;
using namespace DFHack;

DFHACK_PLUGIN("blueprint");
REQUIRE_GLOBAL(world);

struct blueprint_options {
    // whether to display help
    bool help = false;

    // starting tile coordinate of the translation area (if not set then all
    // coordinates are set to -30000)
    df::coord start;

    // dimensions of translation area. width and height are guaranteed to be
    // greater than 0. depth can be positive or negative, but not zero.
    int32_t width  = 0;
    int32_t height = 0;
    int32_t depth  = 0;

    // base name to use for generated files
    string name;

    // whether to autodetect which phases to output
    bool auto_phase = false;

    // if not autodetecting, which phases to output
    bool dig   = false;
    bool build = false;
    bool place = false;
    bool query = false;

    static struct_identity _identity;
};
static const struct_field_info blueprint_options_fields[] = {
    { struct_field_info::PRIMITIVE, "help",       offsetof(blueprint_options, help),       &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::SUBSTRUCT, "start",      offsetof(blueprint_options, start),      &df::coord::_identity,                   0, 0 },
    { struct_field_info::PRIMITIVE, "width",      offsetof(blueprint_options, width),      &df::identity_traits<int32_t>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "height",     offsetof(blueprint_options, height),     &df::identity_traits<int32_t>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "depth",      offsetof(blueprint_options, depth),      &df::identity_traits<int32_t>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "name",       offsetof(blueprint_options, name),        df::identity_traits<string>::get(),     0, 0 },
    { struct_field_info::PRIMITIVE, "auto_phase", offsetof(blueprint_options, auto_phase), &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::PRIMITIVE, "dig",        offsetof(blueprint_options, dig),        &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::PRIMITIVE, "build",      offsetof(blueprint_options, build),      &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::PRIMITIVE, "place",      offsetof(blueprint_options, place),      &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::PRIMITIVE, "query",      offsetof(blueprint_options, query),      &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::END }
};
struct_identity blueprint_options::_identity(sizeof(blueprint_options), &df::allocator_fn<blueprint_options>, NULL, "blueprint_options", NULL, blueprint_options_fields);

command_result blueprint(color_ostream &, vector<string> &);

DFhackCExport command_result plugin_init(color_ostream &, vector<PluginCommand> &commands)
{
    commands.push_back(PluginCommand("blueprint", "Record the structure of a live game map in a quickfort blueprint", blueprint, false));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &)
{
    return CR_OK;
}

static pair<uint32_t, uint32_t> get_building_size(df::building* b)
{
    return pair<uint32_t, uint32_t>(b->x2 - b->x1 + 1, b->y2 - b->y1 + 1);
}

static char get_tile_dig(int32_t x, int32_t y, int32_t z)
{
    df::tiletype *tt = Maps::getTileType(x, y, z);
    df::tiletype_shape ts = tileShape(tt ? *tt : tiletype::Void);
    switch (ts)
    {
    case tiletype_shape::EMPTY:
    case tiletype_shape::RAMP_TOP:
        return 'h';
    case tiletype_shape::FLOOR:
    case tiletype_shape::BOULDER:
    case tiletype_shape::PEBBLES:
    case tiletype_shape::BROOK_TOP:
        return 'd';
    case tiletype_shape::FORTIFICATION:
        return 'F';
    case tiletype_shape::STAIR_UP:
        return 'u';
    case tiletype_shape::STAIR_DOWN:
        return 'j';
    case tiletype_shape::STAIR_UPDOWN:
        return 'i';
    case tiletype_shape::RAMP:
        return 'r';
    default:
        return ' ';
    }
}

static string get_tile_build(uint32_t x, uint32_t y, df::building* b)
{
    if (! b)
        return " ";
    bool at_nw_corner = int32_t(x) == b->x1 && int32_t(y) == b->y1;
    bool at_se_corner = int32_t(x) == b->x2 && int32_t(y) == b->y2;
    bool at_center = int32_t(x) == b->centerx && int32_t(y) == b->centery;
    pair<uint32_t, uint32_t> size = get_building_size(b);
    stringstream out;// = stringstream();
    switch(b->getType())
    {
    case building_type::Armorstand:
        return "a";
    case building_type::Bed:
        return "b";
    case building_type::Chair:
        return "c";
    case building_type::Door:
        return "d";
    case building_type::Floodgate:
        return "x";
    case building_type::Cabinet:
        return "f";
    case building_type::Box:
        return "h";
        //case building_type::Kennel is missing
    case building_type::FarmPlot:
        if(!at_nw_corner)
            return "`";
        out << "p(" << size.first << "x" << size.second << ")";
        return out.str();
    case building_type::Weaponrack:
        return "r";
    case building_type::Statue:
        return "s";
    case building_type::Table:
        return "t";
    case building_type::RoadPaved:
        if(! at_nw_corner)
            return "`";
        out << "o(" << size.first << "x" << size.second << ")";
        return out.str();
    case building_type::RoadDirt:
        if(! at_nw_corner)
            return "`";
        out << "O(" << size.first << "x" << size.second << ")";
        return out.str();
    case building_type::Bridge:
        if(! at_nw_corner)
            return "`";
        switch(((df::building_bridgest*) b)->direction)
        {
        case df::building_bridgest::T_direction::Down:
            out << "gx";
            break;
        case df::building_bridgest::T_direction::Left:
            out << "ga";
            break;
        case df::building_bridgest::T_direction::Up:
            out << "gw";
            break;
        case df::building_bridgest::T_direction::Right:
            out << "gd";
            break;
        case df::building_bridgest::T_direction::Retracting:
            out << "gs";
            break;
        }
        out << "(" << size.first << "x" << size.second << ")";
        return out.str();
    case building_type::Well:
        return "l";
    case building_type::SiegeEngine:
        if (! at_center)
            return "`";
        return ((df::building_siegeenginest*) b)->type == df::siegeengine_type::Ballista ? "ib" : "ic";
    case building_type::Workshop:
        if (! at_center)
            return "`";
        switch (((df::building_workshopst*) b)->type)
        {
        case workshop_type::Leatherworks:
            return "we";
        case workshop_type::Quern:
            return "wq";
        case workshop_type::Millstone:
            return "wM";
        case workshop_type::Loom:
            return "wo";
        case workshop_type::Clothiers:
            return "wk";
        case workshop_type::Bowyers:
            return "wb";
        case workshop_type::Carpenters:
            return "wc";
        case workshop_type::MetalsmithsForge:
            return "wf";
        case workshop_type::MagmaForge:
            return "wv";
        case workshop_type::Jewelers:
            return "wj";
        case workshop_type::Masons:
            return "wm";
        case workshop_type::Butchers:
            return "wu";
        case workshop_type::Tanners:
            return "wn";
        case workshop_type::Craftsdwarfs:
            return "wr";
        case workshop_type::Siege:
            return "ws";
        case workshop_type::Mechanics:
            return "wt";
        case workshop_type::Still:
            return "wl";
        case workshop_type::Farmers:
            return "ww";
        case workshop_type::Kitchen:
            return "wz";
        case workshop_type::Fishery:
            return "wh";
        case workshop_type::Ashery:
            return "wy";
        case workshop_type::Dyers:
            return "wd";
        case workshop_type::Kennels:
            return "k";
        case workshop_type::Custom:
        case workshop_type::Tool:
            //can't do anything with custom workshop
            return "`";
        }
    case building_type::Furnace:
        if (! at_center)
            return "`";
        switch (((df::building_furnacest*) b)->type)
        {
        case furnace_type::WoodFurnace:
            return "ew";
        case furnace_type::Smelter:
            return "es";
        case furnace_type::GlassFurnace:
            return "eg";
        case furnace_type::Kiln:
            return "ek";
        case furnace_type::MagmaSmelter:
            return "el";
        case furnace_type::MagmaGlassFurnace:
            return "ea";
        case furnace_type::MagmaKiln:
            return "en";
        case furnace_type::Custom:
            //can't do anything with custom furnace
            return "`";
        }
    case building_type::WindowGlass:
        return "y";
    case building_type::WindowGem:
        return "Y";
    case building_type::Construction:
        switch (((df::building_constructionst*) b)->type)
        {
        case construction_type::NONE:
            return "`";
        case construction_type::Fortification:
            return "CF";
        case construction_type::Wall:
            return "CW";
        case construction_type::Floor:
            return "Cf";
        case construction_type::UpStair:
            return "Cu";
        case construction_type::DownStair:
            return "Cj";
        case construction_type::UpDownStair:
            return "Cx";
        case construction_type::Ramp:
            return "Cr";
        case construction_type::TrackN:
            return "trackN";
        case construction_type::TrackS:
            return "trackS";
        case construction_type::TrackE:
            return "trackE";
        case construction_type::TrackW:
            return "trackW";
        case construction_type::TrackNS:
            return "trackNS";
        case construction_type::TrackNE:
            return "trackNE";
        case construction_type::TrackNW:
            return "trackNW";
        case construction_type::TrackSE:
            return "trackSE";
        case construction_type::TrackSW:
            return "trackSW";
        case construction_type::TrackEW:
            return "trackEW";
        case construction_type::TrackNSE:
            return "trackNSE";
        case construction_type::TrackNSW:
            return "trackNSW";
        case construction_type::TrackNEW:
            return "trackNEW";
        case construction_type::TrackSEW:
            return "trackSEW";
        case construction_type::TrackNSEW:
            return "trackNSEW";
        case construction_type::TrackRampN:
            return "trackrampN";
        case construction_type::TrackRampS:
            return "trackrampS";
        case construction_type::TrackRampE:
            return "trackrampE";
        case construction_type::TrackRampW:
            return "trackrampW";
        case construction_type::TrackRampNS:
            return "trackrampNS";
        case construction_type::TrackRampNE:
            return "trackrampNE";
        case construction_type::TrackRampNW:
            return "trackrampNW";
        case construction_type::TrackRampSE:
            return "trackrampSE";
        case construction_type::TrackRampSW:
            return "trackrampSW";
        case construction_type::TrackRampEW:
            return "trackrampEW";
        case construction_type::TrackRampNSE:
            return "trackrampNSE";
        case construction_type::TrackRampNSW:
            return "trackrampNSW";
        case construction_type::TrackRampNEW:
            return "trackrampNEW";
        case construction_type::TrackRampSEW:
            return "trackrampSEW";
        case construction_type::TrackRampNSEW:
            return "trackrampNSEW";
        }
    case building_type::Shop:
        if (! at_center)
            return "`";
        return "z";
    case building_type::AnimalTrap:
        return "m";
    case building_type::Chain:
        return "v";
    case building_type::Cage:
        return "j";
    case building_type::TradeDepot:
        if (! at_center)
            return "`";
        return "D";
    case building_type::Trap:
        switch (((df::building_trapst*) b)->trap_type)
        {
        case trap_type::StoneFallTrap:
            return "Ts";
        case trap_type::WeaponTrap:
            return "Tw";
        case trap_type::Lever:
            return "Tl";
        case trap_type::PressurePlate:
            return "Tp";
        case trap_type::CageTrap:
            return "Tc";
        case trap_type::TrackStop:
            df::building_trapst* ts = (df::building_trapst*) b;
            out << "CS";
            if (ts->use_dump)
            {
                if (ts->dump_x_shift == 0)
                {
                    if (ts->dump_y_shift > 0)
                        out << "dd";
                    else
                        out << "d";
                }
                else
                {
                    if (ts->dump_x_shift > 0)
                        out << "ddd";
                    else
                        out << "dddd";
                }
            }
            switch (ts->friction)
            {
            case 10:
                out << "a";
            case 50:
                out << "a";
            case 500:
                out << "a";
            case 10000:
                out << "a";
            }
            return out.str();
        }
    case building_type::ScrewPump:
        if (! at_se_corner) //screw pumps anchor at bottom/right
            return "`";
        switch (((df::building_screw_pumpst*) b)->direction)
        {
        case screw_pump_direction::FromNorth:
            return "Msu";
        case screw_pump_direction::FromEast:
            return "Msk";
        case screw_pump_direction::FromSouth:
            return "Msm";
        case screw_pump_direction::FromWest:
            return "Msh";
        }
    case building_type::WaterWheel:
        if (! at_center)
            return "`";
        //s swaps orientation which defaults to vertical
        return ((df::building_water_wheelst*) b)->is_vertical ? "Mw" : "Mws";
    case building_type::Windmill:
        if (! at_center)
            return "`";
        return "Mm";
    case building_type::GearAssembly:
        return "Mg";
    case building_type::AxleHorizontal:
        if (! at_nw_corner) //a guess based on how constructions work
            return "`";
        //same as water wheel but reversed
        out << "Mh" << (((df::building_axle_horizontalst*) b)->is_vertical ? "s" : "")
            << "(" << size.first << "x" << size.second << ")";
        return out.str();
    case building_type::AxleVertical:
        return "Mv";
    case building_type::Rollers:
        if (! at_nw_corner)
            return "`";
        out << "Mr";
        switch (((df::building_rollersst*) b)->direction)
        {
        case screw_pump_direction::FromNorth:
            break;
        case screw_pump_direction::FromEast:
            out << "s";
        case screw_pump_direction::FromSouth:
            out << "s";
        case screw_pump_direction::FromWest:
            out << "s";
        }
        out << "(" << size.first << "x" << size.second << ")";
        return out.str();
    case building_type::Support:
        return "S";
    case building_type::ArcheryTarget:
        return "A";
    case building_type::TractionBench:
        return "R";
    case building_type::Hatch:
        return "H";
    case building_type::Slab:
        //how to mine alt key?!?
        //alt+s
        return " ";
    case building_type::NestBox:
        return "N";
    case building_type::Hive:
        //alt+h
        return " ";
    case building_type::GrateWall:
        return "W";
    case building_type::GrateFloor:
        return "G";
    case building_type::BarsVertical:
        return "B";
    case building_type::BarsFloor:
        //alt+b
        return " ";
    default:
        return " ";
    }
}

static string get_tile_place(uint32_t x, uint32_t y, df::building* b)
{
    if (! b || b->getType() != building_type::Stockpile)
        return " ";
    if (b->x1 != int32_t(x) || b->y1 != int32_t(y))
        return "`";
    pair<uint32_t, uint32_t> size = get_building_size(b);
    df::building_stockpilest* sp = (df::building_stockpilest*) b;
    stringstream out;// = stringstream();
    switch (sp->settings.flags.whole)
    {
    case df::stockpile_group_set::mask_animals:
        out << "a";
        break;
    case df::stockpile_group_set::mask_food:
        out << "f";
        break;
    case df::stockpile_group_set::mask_furniture:
        out << "u";
        break;
    case df::stockpile_group_set::mask_corpses:
        out << "y";
        break;
    case df::stockpile_group_set::mask_refuse:
        out << "r";
        break;
    case df::stockpile_group_set::mask_wood:
        out << "w";
        break;
    case df::stockpile_group_set::mask_stone:
        out << "s";
        break;
    case df::stockpile_group_set::mask_gems:
        out << "e";
        break;
    case df::stockpile_group_set::mask_bars_blocks:
        out << "b";
        break;
    case df::stockpile_group_set::mask_cloth:
        out << "h";
        break;
    case df::stockpile_group_set::mask_leather:
        out << "l";
        break;
    case df::stockpile_group_set::mask_ammo:
        out << "z";
        break;
    case df::stockpile_group_set::mask_coins:
        out << "n";
        break;
    case df::stockpile_group_set::mask_finished_goods:
        out << "g";
        break;
    case df::stockpile_group_set::mask_weapons:
        out << "p";
        break;
    case df::stockpile_group_set::mask_armor:
        out << "d";
        break;
    default: //multiple stockpile type
        return "`";
    }
    out << "("<< size.first << "x" << size.second << ")";
    return out.str();
}

static string get_tile_query(df::building* b)
{
    if (b && b->is_room)
        return "r+";
    return " ";
}

// returns filename
static string init_stream(ofstream &out, string basename, string target)
{
    std::ostringstream out_path;
    out_path << basename << "-" << target << ".csv";
    string path = out_path.str();
    out.open(path, ofstream::trunc);
    out << "#" << target << endl;
    return path;
}

static bool do_transform(const DFCoord &start, const DFCoord &end,
                         const blueprint_options &options,
                         vector<string> &files,
                         std::ostringstream &err)
{
    ofstream dig, build, place, query;

    string basename = "blueprints/" + options.name;
    size_t last_slash = basename.find_last_of("/");
    string parent_path = basename.substr(0, last_slash);

    // create output directory if it doesn't already exist
    std::error_code ec;
    if (!Filesystem::mkdir_recursive(parent_path))
    {
        err << "could not create output directory: '" << parent_path << "'";
        return false;
    }

    if (options.auto_phase || options.dig)
    {
        files.push_back(init_stream(dig, basename, "dig"));
    }
    if (options.auto_phase || options.build)
    {
        files.push_back(init_stream(build, basename, "build"));
    }
    if (options.auto_phase || options.place)
    {
        files.push_back(init_stream(place, basename, "place"));
    }
    if (options.auto_phase || options.query)
    {
        files.push_back(init_stream(query, basename, "query"));
    }

    const int32_t z_inc = start.z < end.z ? 1 : -1;
    const string z_key = start.z < end.z ? "#<" : "#>";
    for (int32_t z = start.z; z != end.z; z += z_inc)
    {
        for (int32_t y = start.y; y < end.y; y++)
        {
            for (int32_t x = start.x; x < end.x; x++)
            {
                df::building* b = Buildings::findAtTile(DFCoord(x, y, z));
                if (options.auto_phase || options.query)
                    query << get_tile_query(b) << ',';
                if (options.auto_phase || options.place)
                    place << get_tile_place(x, y, b) << ',';
                if (options.auto_phase || options.build)
                    build << get_tile_build(x, y, b) << ',';
                if (options.auto_phase || options.dig)
                    dig << get_tile_dig(x, y, z) << ',';
            }
            if (options.auto_phase || options.query)
                query << "#" << endl;
            if (options.auto_phase || options.place)
                place << "#" << endl;
            if (options.auto_phase || options.build)
                build << "#" << endl;
            if (options.auto_phase || options.dig)
                dig << "#" << endl;
        }
        if (z != end.z - z_inc)
        {
            if (options.auto_phase || options.query)
                query << z_key << endl;
            if (options.auto_phase || options.place)
                place << z_key << endl;
            if (options.auto_phase || options.build)
                build << z_key << endl;
            if (options.auto_phase || options.dig)
                dig << z_key << endl;
        }
    }
    if (options.auto_phase || options.query)
        query.close();
    if (options.auto_phase || options.place)
        place.close();
    if (options.auto_phase || options.build)
        build.close();
    if (options.auto_phase || options.dig)
        dig.close();

    return true;
}

static bool get_options(color_ostream &out,
                        blueprint_options &opts,
                        const vector<string> &parameters)
{
    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!lua_checkstack(L, parameters.size() + 2) ||
        !Lua::PushModulePublic(
            out, L, "plugins.blueprint", "parse_commandline"))
    {
        out.printerr("Failed to load blueprint Lua code\n");
        return false;
    }

    Lua::Push(L, &opts);

    for (const string &param : parameters)
        Lua::Push(L, param);

    if (!Lua::SafeCall(out, L, parameters.size() + 1, 0))
        return false;

    return true;
}

static void print_help(color_ostream &out)
{
    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!lua_checkstack(L, 1) ||
        !Lua::PushModulePublic(out, L, "plugins.blueprint", "print_help") ||
        !Lua::SafeCall(out, L, 0, 0))
    {
        out.printerr("Failed to load blueprint Lua code\n");
    }
}

// returns whether blueprint generation was successful. populates files with the
// names of the files that were generated
static bool do_blueprint(color_ostream &out,
                         const vector<string> &parameters,
                         vector<string> &files)
{
    CoreSuspender suspend;

    if (parameters.size() >= 1 && parameters[0] == "gui")
    {
        std::ostringstream command;
        command << "gui/blueprint";
        for (const string &param : parameters)
        {
            command << " " << param;
        }
        string command_str = command.str();
        out.print("launching %s\n", command_str.c_str());

        Core::getInstance().setHotkeyCmd(command_str);
        return CR_OK;
    }

    blueprint_options options;
    if (!get_options(out, options, parameters) || options.help)
    {
        print_help(out);
        return options.help;
    }

    if (!Maps::IsValid())
    {
        out.printerr("Map is not available!\n");
        return false;
    }

    // start coordinates can come from either the commandline or the map cursor
    DFCoord start(options.start);
    if (start.x == -30000)
    {
        if (!Gui::getCursorCoords(start))
        {
            out.printerr("Can't get cursor coords! Make sure you specify the"
                    " --cursor parameter or have an active cursor in DF.\n");
            return false;
        }
    }
    if (!Maps::isValidTilePos(start))
    {
        out.printerr("Invalid start position: %d,%d,%d\n",
                     start.x, start.y, start.z);
        return false;
    }

    // end coords are one beyond the last processed coordinate. note that
    // options.depth can be negative.
    DFCoord end(start.x + options.width, start.y + options.height,
                start.z + options.depth);

    // crop end coordinate to map bounds. we've already verified that start is
    // a valid coordinate, and width, height, and depth are non-zero, so our
    // final area is always going to be at least 1x1x1.
    df::world::T_map &map = df::global::world->map;
    if (end.x > map.x_count)
        end.x = map.x_count;
    if (end.y > map.y_count)
        end.y = map.y_count;
    if (end.z > map.z_count)
        end.z = map.z_count;
    if (end.z < -1)
        end.z = -1;

    std::ostringstream err;
    if (!do_transform(start, end, options, files, err))
    {
        out.printerr("%s\n", err.str().c_str());
        return false;
    }
    return true;
}

// entrypoint when called from Lua. returns the names of the generated files
static int run(lua_State *L)
{
    int argc = lua_gettop(L);
    vector<string> argv;

    for (int i = 1; i <= argc; ++i)
    {
        const char *s = lua_tostring(L, i);
        if (s == NULL)
            luaL_error(L, "all parameters must be strings");
        argv.push_back(s);
    }

    vector<string> files;
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    if (do_blueprint(*out, argv, files))
    {
        Lua::PushVector(L, files);
        return 1;
    }

    return 0;
}

command_result blueprint(color_ostream &out, vector<string> &parameters)
{
    vector<string> files;
    if (do_blueprint(out, parameters, files))
    {
        out.print("Generated blueprint file(s):\n");
        for (string &fname : files)
            out.print("  %s\n", fname.c_str());
        return CR_OK;
    }
    return CR_FAILURE;
}

DFHACK_PLUGIN_LUA_COMMANDS {
    DFHACK_LUA_COMMAND(run),
    DFHACK_LUA_END
};
