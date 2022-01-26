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
#include "df/building_civzonest.h"
#include "df/building_constructionst.h"
#include "df/building_furnacest.h"
#include "df/building_rollersst.h"
#include "df/building_screw_pumpst.h"
#include "df/building_siegeenginest.h"
#include "df/building_trapst.h"
#include "df/building_water_wheelst.h"
#include "df/building_workshopst.h"
#include "df/world.h"

using std::endl;
using std::ofstream;
using std::pair;
using std::map;
using std::string;
using std::vector;
using namespace DFHack;

DFHACK_PLUGIN("blueprint");
REQUIRE_GLOBAL(world);

struct blueprint_options {
    // whether to display help
    bool help = false;

    // starting tile coordinate of the translation area (if not set then all
    // coordinates are set to -30000)
    df::coord start;

    // output file format. this could be an enum if we set up the boilerplate
    // for it.
    string format;

    // offset and comment to write in the quickfort start() modeline marker
    // if not set, coordinates are set to 0 and the comment will be empty
    df::coord2d playback_start = df::coord2d(0, 0);
    string playback_start_comment;

    // file splitting strategy. this could be an enum if we set up the
    // boilerplate for it.
    string split_strategy;

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
    bool track = false;
    bool build = false;
    bool place = false;
    bool zone  = false;
    bool query = false;

    static struct_identity _identity;
};
static const struct_field_info blueprint_options_fields[] = {
    { struct_field_info::PRIMITIVE, "help",                   offsetof(blueprint_options, help),                  &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::SUBSTRUCT, "start",                  offsetof(blueprint_options, start),                 &df::coord::_identity,                   0, 0 },
    { struct_field_info::PRIMITIVE, "format",                 offsetof(blueprint_options, format),                 df::identity_traits<string>::get(),     0, 0 },
    { struct_field_info::SUBSTRUCT, "playback_start",         offsetof(blueprint_options, playback_start),        &df::coord2d::_identity,                 0, 0 },
    { struct_field_info::PRIMITIVE, "playback_start_comment", offsetof(blueprint_options, playback_start_comment), df::identity_traits<string>::get(),     0, 0 },
    { struct_field_info::PRIMITIVE, "split_strategy",         offsetof(blueprint_options, split_strategy),         df::identity_traits<string>::get(),     0, 0 },
    { struct_field_info::PRIMITIVE, "width",                  offsetof(blueprint_options, width),                 &df::identity_traits<int32_t>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "height",                 offsetof(blueprint_options, height),                &df::identity_traits<int32_t>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "depth",                  offsetof(blueprint_options, depth),                 &df::identity_traits<int32_t>::identity, 0, 0 },
    { struct_field_info::PRIMITIVE, "name",                   offsetof(blueprint_options, name),                   df::identity_traits<string>::get(),     0, 0 },
    { struct_field_info::PRIMITIVE, "auto_phase",             offsetof(blueprint_options, auto_phase),            &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::PRIMITIVE, "dig",                    offsetof(blueprint_options, dig),                   &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::PRIMITIVE, "track",                  offsetof(blueprint_options, track),                 &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::PRIMITIVE, "build",                  offsetof(blueprint_options, build),                 &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::PRIMITIVE, "place",                  offsetof(blueprint_options, place),                 &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::PRIMITIVE, "zone",                   offsetof(blueprint_options, zone),                  &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::PRIMITIVE, "query",                  offsetof(blueprint_options, query),                 &df::identity_traits<bool>::identity,    0, 0 },
    { struct_field_info::END }
};
struct_identity blueprint_options::_identity(sizeof(blueprint_options), &df::allocator_fn<blueprint_options>, NULL, "blueprint_options", NULL, blueprint_options_fields);

command_result blueprint(color_ostream &, vector<string> &);

DFhackCExport command_result plugin_init(color_ostream &, vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand("blueprint", "Record the structure of a live game map in a quickfort blueprint file", blueprint, false));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &) {
    return CR_OK;
}

struct tile_context {
    bool pretty = false;
    df::building* b = NULL;
};

// We use const char * throughout this code instead of std::string to avoid
// having to allocate memory for all the small string literals. This
// significantly speeds up processing and allows us to handle very large maps
// (e.g. 16x16 embarks) without running out of memory. This cache provides a
// mechanism for storing dynamically created strings so their memory stays
// allocated until we write out the blueprints at the end.
// If NULL is passed as the str, the cache is cleared.
static const char * cache(const char *str) {
    // this local static assumes that no two blueprints are being generated at
    // the same time, which is currently ensured by the higher-level DFHack
    // command handling code. if this assumption ever becomes untrue, we'll
    // need to protect the cache with thread synchronization primitives.
    static std::set<string> _cache;
    if (!str) {
        _cache.clear();
        return NULL;
    }
    return _cache.emplace(str).first->c_str();
}

// Convenience wrapper for std::string.
static const char * cache(const string &str) {
    return cache(str.c_str());
}

// Convenience wrapper for std::ostringstream.
static const char * cache(std::ostringstream &str) {
    return cache(str.str());
}

static const char * get_tile_dig(const df::coord &pos, const tile_context &) {
    df::tiletype *tt = Maps::getTileType(pos);
    if (!tt)
        return NULL;

    switch (tileShape(*tt))
    {
    case tiletype_shape::EMPTY:
    case tiletype_shape::RAMP_TOP:
        return "h";
    case tiletype_shape::FLOOR:
    case tiletype_shape::BOULDER:
    case tiletype_shape::PEBBLES:
    case tiletype_shape::BROOK_TOP:
        return "d";
    case tiletype_shape::FORTIFICATION:
        return "F";
    case tiletype_shape::STAIR_UP:
        return "u";
    case tiletype_shape::STAIR_DOWN:
        return "j";
    case tiletype_shape::STAIR_UPDOWN:
        return "i";
    case tiletype_shape::RAMP:
        return "r";
    case tiletype_shape::WALL:
    default:
        return NULL;
    }
}

static const char * get_track_str(const char *prefix, df::tiletype tt) {
    TileDirection tdir = tileDirection(tt);

    string dir;
    if (tdir.north) dir += "N";
    if (tdir.south) dir += "S";
    if (tdir.east)  dir += "E";
    if (tdir.west)  dir += "W";

    return cache(prefix + dir);
}

static const char * get_tile_track(const df::coord &pos, const tile_context &) {
    df::tiletype *tt = Maps::getTileType(pos);
    if (!tt)
        return NULL;

    switch (tileShape(*tt))
    {
    case tiletype_shape::FLOOR:
        if (tileSpecial(*tt) == tiletype_special::TRACK)
            return get_track_str("track", *tt);
        break;
    case tiletype_shape::RAMP:
        if (tileSpecial(*tt) == tiletype_special::TRACK)
            return get_track_str("trackramp", *tt);
        break;
    default:
        break;
    }

    return NULL;
}

static pair<uint32_t, uint32_t> get_building_size(const df::building *b) {
    return pair<uint32_t, uint32_t>(b->x2 - b->x1 + 1, b->y2 - b->y1 + 1);
}

static const char * if_pretty(const tile_context &ctx, const char *c) {
    return ctx.pretty ? c : NULL;
}

static bool is_rectangular(const df::building *bld) {
    const df::building_extents &room = bld->room;
    if (!room.extents)
        return true;
    for (int32_t y = 0; y < room.height; ++y) {
        for (int32_t x = 0; x < room.width; ++x) {
            if (!room.extents[y * room.width + x])
                return false;
        }
    }
    return true;
}

static bool is_rectangular(const tile_context &ctx) {
    return is_rectangular(ctx.b);
}

static const char * do_block_building(const tile_context &ctx, const char *s,
                                      bool at_target_pos,
                                      bool *add_size = NULL) {
    if(!at_target_pos) {
        return if_pretty(ctx, "`");
    }
    if (add_size)
        *add_size = true;
    return s;
}

static const char * do_extent_building(const tile_context &ctx, const char *s,
                                       bool at_target_pos,
                                       bool *add_size = NULL) {
    // use expansion syntax for rectangular or one-tile buildings
    if (is_rectangular(ctx))
        return do_block_building(ctx, s, at_target_pos, add_size);

    return s;
}

static const char * get_bridge_str(df::building *b) {
    df::building_bridgest *bridge = virtual_cast<df::building_bridgest>(b);
    if (!bridge)
        return "g";

    switch(bridge->direction) {
    case df::building_bridgest::T_direction::Retracting: return "gs";
    case df::building_bridgest::T_direction::Left:       return "ga";
    case df::building_bridgest::T_direction::Right:      return "gd";
    case df::building_bridgest::T_direction::Up:         return "gw";
    case df::building_bridgest::T_direction::Down:       return "gx";
    default:
        return "g";
    }
}

static const char * get_siege_str(df::building *b) {
    df::building_siegeenginest *se =
            virtual_cast<df::building_siegeenginest>(b);
    return !se || se->type == df::siegeengine_type::Catapult ? "ic" : "ib";
}

static const char * get_workshop_str(df::building *b) {
    df::building_workshopst *ws = virtual_cast<df::building_workshopst>(b);
    if (!ws)
        return "~";

    switch (ws->type) {
    case workshop_type::Leatherworks:     return "we";
    case workshop_type::Quern:            return "wq";
    case workshop_type::Millstone:        return "wM";
    case workshop_type::Loom:             return "wo";
    case workshop_type::Clothiers:        return "wk";
    case workshop_type::Bowyers:          return "wb";
    case workshop_type::Carpenters:       return "wc";
    case workshop_type::MetalsmithsForge: return "wf";
    case workshop_type::MagmaForge:       return "wv";
    case workshop_type::Jewelers:         return "wj";
    case workshop_type::Masons:           return "wm";
    case workshop_type::Butchers:         return "wu";
    case workshop_type::Tanners:          return "wn";
    case workshop_type::Craftsdwarfs:     return "wr";
    case workshop_type::Siege:            return "ws";
    case workshop_type::Mechanics:        return "wt";
    case workshop_type::Still:            return "wl";
    case workshop_type::Farmers:          return "ww";
    case workshop_type::Kitchen:          return "wz";
    case workshop_type::Fishery:          return "wh";
    case workshop_type::Ashery:           return "wy";
    case workshop_type::Dyers:            return "wd";
    case workshop_type::Kennels:          return "k";
    case workshop_type::Custom:
    {
        int32_t custom = b->getCustomType();
        if (custom == 0) return "wS";
        if (custom == 1) return "wp";
    }
    // fallthrough
    case workshop_type::Tool:
    default:
        return "~";
    }
}

static const char * get_furnace_str(df::building *b) {
    df::building_furnacest *furnace = virtual_cast<df::building_furnacest>(b);
    if (!furnace)
        return "~";

    switch (furnace->type) {
    case furnace_type::WoodFurnace:       return "ew";
    case furnace_type::Smelter:           return "es";
    case furnace_type::GlassFurnace:      return "eg";
    case furnace_type::Kiln:              return "ek";
    case furnace_type::MagmaSmelter:      return "el";
    case furnace_type::MagmaGlassFurnace: return "ea";
    case furnace_type::MagmaKiln:         return "en";
    case furnace_type::Custom:
    default:
        return "~";
    }
}

static const char * get_construction_str(df::building *b) {
    df::building_constructionst *cons =
            virtual_cast<df::building_constructionst>(b);
    if (!cons)
        return "~";

    switch (cons->type) {
    case construction_type::Fortification: return "CF";
    case construction_type::Wall:          return "Cw";
    case construction_type::Floor:         return "Cf";
    case construction_type::UpStair:       return "Cu";
    case construction_type::DownStair:     return "Cd";
    case construction_type::UpDownStair:   return "Cx";
    case construction_type::Ramp:          return "Cr";
    case construction_type::TrackN:        return "trackN";
    case construction_type::TrackS:        return "trackS";
    case construction_type::TrackE:        return "trackE";
    case construction_type::TrackW:        return "trackW";
    case construction_type::TrackNS:       return "trackNS";
    case construction_type::TrackNE:       return "trackNE";
    case construction_type::TrackNW:       return "trackNW";
    case construction_type::TrackSE:       return "trackSE";
    case construction_type::TrackSW:       return "trackSW";
    case construction_type::TrackEW:       return "trackEW";
    case construction_type::TrackNSE:      return "trackNSE";
    case construction_type::TrackNSW:      return "trackNSW";
    case construction_type::TrackNEW:      return "trackNEW";
    case construction_type::TrackSEW:      return "trackSEW";
    case construction_type::TrackNSEW:     return "trackNSEW";
    case construction_type::TrackRampN:    return "trackrampN";
    case construction_type::TrackRampS:    return "trackrampS";
    case construction_type::TrackRampE:    return "trackrampE";
    case construction_type::TrackRampW:    return "trackrampW";
    case construction_type::TrackRampNS:   return "trackrampNS";
    case construction_type::TrackRampNE:   return "trackrampNE";
    case construction_type::TrackRampNW:   return "trackrampNW";
    case construction_type::TrackRampSE:   return "trackrampSE";
    case construction_type::TrackRampSW:   return "trackrampSW";
    case construction_type::TrackRampEW:   return "trackrampEW";
    case construction_type::TrackRampNSE:  return "trackrampNSE";
    case construction_type::TrackRampNSW:  return "trackrampNSW";
    case construction_type::TrackRampNEW:  return "trackrampNEW";
    case construction_type::TrackRampSEW:  return "trackrampSEW";
    case construction_type::TrackRampNSEW: return "trackrampNSEW";
    case construction_type::NONE:
    default:
        return "~";
    }
}

static const char * get_trap_str(df::building *b) {
    df::building_trapst *trap = virtual_cast<df::building_trapst>(b);
    if (!trap)
        return "~";

    switch (trap->trap_type) {
    case trap_type::StoneFallTrap: return "Ts";
    case trap_type::WeaponTrap:    return "Tw";
    case trap_type::Lever:         return "Tl";
    case trap_type::PressurePlate: return "Tp";
    case trap_type::CageTrap:      return "Tc";
    case trap_type::TrackStop:
        {
            std::ostringstream buf;
            buf << "CS";
            if (trap->use_dump) {
                if (trap->dump_x_shift == 0) {
                    buf << "d";
                    if (trap->dump_y_shift > 0)
                        buf << "d";
                } else {
                    buf << "ddd";
                    if (trap->dump_x_shift < 0)
                        buf << "d";
                }
            }

            // each case falls through and is additive
            switch (trap->friction) {
            case 10:    buf << "a";
            case 50:    buf << "a";
            case 500:   buf << "a";
            case 10000: buf << "a";
            }
            return cache(buf);
        }
    default:
        return "~";
    }
}

static const char * get_screw_pump_str(df::building *b) {
    df::building_screw_pumpst *sp = virtual_cast<df::building_screw_pumpst>(b);
    if (!sp)
        return "~";

    switch (sp->direction)
    {
    case screw_pump_direction::FromNorth: return "Msu";
    case screw_pump_direction::FromEast:  return "Msk";
    case screw_pump_direction::FromSouth: return "Msm";
    case screw_pump_direction::FromWest:  return "Msh";
    default:
        return "~";
    }
}

static const char * get_water_wheel_str(df::building *b) {
    df::building_water_wheelst *ww =
            virtual_cast<df::building_water_wheelst>(b);
    if (!ww)
        return "~";

    return ww->is_vertical ? "Mw" : "Mws";
}

static const char * get_axle_str(df::building *b) {
    df::building_axle_horizontalst *ah =
            virtual_cast<df::building_axle_horizontalst>(b);
    if (!ah)
        return "~";

    return ah->is_vertical ? "Mhs" : "Mh";
}

static const char * add_speed_suffix(df::building_rollersst *r,
                                     const char *prefix) {
    int32_t speed = r->speed;
    if (speed >= 50000) return prefix;
    string sprefix(prefix);
    if (speed >= 40000) return cache(sprefix + "q");
    if (speed >= 30000) return cache(sprefix + "qq");
    if (speed >= 20000) return cache(sprefix + "qqq");
    return cache(sprefix + "qqqq");
}

static const char * get_roller_str(df::building *b) {
    df::building_rollersst *r = virtual_cast<df::building_rollersst>(b);
    if (!r)
        return "~";

    switch (r->direction) {
    case screw_pump_direction::FromNorth: return add_speed_suffix(r, "Mr");
    case screw_pump_direction::FromEast:  return add_speed_suffix(r, "Mrs");
    case screw_pump_direction::FromSouth: return add_speed_suffix(r, "Mrss");
    case screw_pump_direction::FromWest:  return add_speed_suffix(r, "Mrsss");
    default:
        return "~";
    }
}

static const char * get_build_keys(const df::coord &pos,
                                   const tile_context &ctx,
                                   bool &add_size) {
    bool at_nw_corner = static_cast<int32_t>(pos.x) == ctx.b->x1
                            && static_cast<int32_t>(pos.y) == ctx.b->y1;
    bool at_se_corner = static_cast<int32_t>(pos.x) == ctx.b->x2
                            && static_cast<int32_t>(pos.y) == ctx.b->y2;
    bool at_center = static_cast<int32_t>(pos.x) == ctx.b->centerx
                            && static_cast<int32_t>(pos.y) == ctx.b->centery;

    switch(ctx.b->getType()) {
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
    case building_type::FarmPlot:
        return do_extent_building(ctx, "p", at_nw_corner, &add_size);
    case building_type::Weaponrack:
        return "r";
    case building_type::Statue:
        return "s";
    case building_type::Table:
        return "t";
    case building_type::Coffin:
        return "n";
    case building_type::RoadPaved:
        return do_extent_building(ctx, "o", at_nw_corner, &add_size);
    case building_type::RoadDirt:
        return do_extent_building(ctx, "O", at_nw_corner, &add_size);
    case building_type::Bridge:
        return do_block_building(ctx, get_bridge_str(ctx.b), at_nw_corner,
                          &add_size);
    case building_type::Well:
        return "l";
    case building_type::SiegeEngine:
        return do_block_building(ctx, get_siege_str(ctx.b), at_center);
    case building_type::Workshop:
        return do_block_building(ctx, get_workshop_str(ctx.b), at_center);
    case building_type::Furnace:
        return do_block_building(ctx, get_furnace_str(ctx.b), at_center);
    case building_type::WindowGlass:
        return "y";
    case building_type::WindowGem:
        return "Y";
    case building_type::Construction:
        return get_construction_str(ctx.b);
    case building_type::Shop:
        return do_block_building(ctx, "z", at_center);
    case building_type::AnimalTrap:
        return "m";
    case building_type::Chain:
        return "v";
    case building_type::Cage:
        return "j";
    case building_type::TradeDepot:
        return do_block_building(ctx, "D", at_center);
    case building_type::Trap:
        return get_trap_str(ctx.b);
    case building_type::Weapon:
        return "TS";
    case building_type::ScrewPump:
        return do_block_building(ctx, get_screw_pump_str(ctx.b), at_se_corner);
    case building_type::WaterWheel:
        return do_block_building(ctx, get_water_wheel_str(ctx.b), at_center);
    case building_type::Windmill:
        return do_block_building(ctx, "Mm", at_center);
    case building_type::GearAssembly:
        return "Mg";
    case building_type::AxleHorizontal:
        return do_block_building(ctx, get_axle_str(ctx.b), at_nw_corner,
                                 &add_size);
    case building_type::AxleVertical:
        return "Mv";
    case building_type::Rollers:
        return do_block_building(ctx, get_roller_str(ctx.b), at_nw_corner,
                                 &add_size);
    case building_type::Support:
        return "S";
    case building_type::ArcheryTarget:
        return "A";
    case building_type::TractionBench:
        return "R";
    case building_type::Hatch:
        return "H";
    case building_type::Slab:
        return "~s";
    case building_type::NestBox:
        return "N";
    case building_type::Hive:
        return "~h";
    case building_type::GrateWall:
        return "W";
    case building_type::GrateFloor:
        return "G";
    case building_type::BarsVertical:
        return "B";
    case building_type::BarsFloor:
        return "~b";
    case building_type::Bookcase:
        return "~c";
    case building_type::DisplayFurniture:
        return "F";
    case building_type::OfferingPlace:
        return "~a";
    default:
        return "~";
    }
}

// returns "~" if keys is NULL; otherwise returns the keys with the building
// dimensions in the expansion syntax
static const char * add_expansion_syntax(const df::building *bld,
                                         const char *keys) {
    if (!keys)
        return "~";
    std::ostringstream s;
    pair<uint32_t, uint32_t> size = get_building_size(bld);
    s << keys << "(" << size.first << "x" << size.second << ")";
    return cache(s);
}

static const char * add_expansion_syntax(const tile_context &ctx,
                                         const char *keys) {
    return add_expansion_syntax(ctx.b, keys);
}

static const char * get_tile_build(const df::coord &pos,
                                   const tile_context &ctx) {
    if (!ctx.b || ctx.b->getType() == building_type::Stockpile) {
        return NULL;
    }

    bool add_size = false;
    const char *keys = get_build_keys(pos, ctx, add_size);

    if (!add_size)
        return keys;
    return add_expansion_syntax(ctx, keys);
}

static const char * get_place_keys(const tile_context &ctx) {
    df::building_stockpilest* sp =
            virtual_cast<df::building_stockpilest>(ctx.b);
    if (!sp) {
        return NULL;
    }

    string keys;
    df::stockpile_group_set &flags = sp->settings.flags;
    if (flags.bits.animals) keys += 'a';
    if (flags.bits.food) keys += 'f';
    if (flags.bits.furniture) keys += 'u';
    if (flags.bits.coins) keys += 'n';
    if (flags.bits.corpses) keys += 'y';
    if (flags.bits.refuse) keys += 'r';
    if (flags.bits.stone) keys += 's';
    if (flags.bits.wood) keys += 'w';
    if (flags.bits.gems) keys += 'e';
    if (flags.bits.bars_blocks) keys += 'b';
    if (flags.bits.cloth) keys += 'h';
    if (flags.bits.leather) keys += 'l';
    if (flags.bits.ammo) keys += 'z';
    if (flags.bits.sheet) keys += 'S';
    if (flags.bits.finished_goods) keys += 'g';
    if (flags.bits.weapons) keys += 'p';
    if (flags.bits.armor) keys += 'd';

    if (keys.empty())
        return "c";
    return cache(keys);
}

static bool is_single_tile(const tile_context &ctx) {
    return ctx.b->x1 == ctx.b->x2 && ctx.b->y1 == ctx.b->y2;
}

static const char * get_tile_place(const df::coord &pos,
                                   const tile_context &ctx) {
    if (!ctx.b || ctx.b->getType() != building_type::Stockpile)
        return NULL;

    if (!is_rectangular(ctx) || is_single_tile(ctx))
        return get_place_keys(ctx);

    if (ctx.b->x1 != static_cast<int32_t>(pos.x)
            || ctx.b->y1 != static_cast<int32_t>(pos.y)) {
        return if_pretty(ctx, "`");
    }

    return add_expansion_syntax(ctx, get_place_keys(ctx));
}

static bool hospital_maximums_eq(const df::hospital_supplies &a,
                                 const df::hospital_supplies &b) {
    return a.max_thread == b.max_thread &&
            a.max_cloth == b.max_cloth &&
            a.max_splints == b.max_splints &&
            a.max_crutches == b.max_crutches &&
            a.max_plaster == b.max_plaster &&
            a.max_buckets == b.max_buckets &&
            a.max_soap == b.max_soap;
}

static const char * get_zone_keys(const df::building_civzonest *zone) {
    static const uint32_t DEFAULT_GATHER_FLAGS =
            df::building_civzonest::T_gather_flags::mask_pick_trees |
            df::building_civzonest::T_gather_flags::mask_pick_shrubs |
            df::building_civzonest::T_gather_flags::mask_gather_fallen;
    static const df::hospital_supplies DEFAULT_HOSPITAL;

    std::ostringstream keys;
    const df::building_civzonest::T_zone_flags &flags = zone->zone_flags;

    // inverted logic for Active since it's on by default
    if (!flags.bits.active) keys << 'a';

    // in UI order
    if (flags.bits.water_source) keys << 'w';
    if (flags.bits.fishing) keys << 'f';
    if (flags.bits.gather) {
        keys << 'g';
        if (zone->gather_flags.whole != DEFAULT_GATHER_FLAGS) {
            keys << 'G';
            // logic is inverted since they're all on by default
            if (!zone->gather_flags.bits.pick_trees) keys << 't';
            if (!zone->gather_flags.bits.pick_shrubs) keys << 's';
            if (!zone->gather_flags.bits.gather_fallen) keys << 'f';
            keys << '^';
        }
    }
    if (flags.bits.garbage_dump) keys << 'd';
    if (flags.bits.pen_pasture) keys << 'n';
    if (flags.bits.pit_pond) {
        keys << 'p';
        if (zone->pit_flags.bits.is_pond)
            keys << "Pf^";
    }
    if (flags.bits.sand) keys << 's';
    if (flags.bits.clay) keys << 'c';
    if (flags.bits.meeting_area) keys << 'm';
    if (flags.bits.hospital) {
        keys << 'h';
        const df::hospital_supplies &hospital = zone->hospital;
        if (!hospital_maximums_eq(hospital, DEFAULT_HOSPITAL)) {
            keys << "H{hospital";
            if (hospital.max_thread != DEFAULT_HOSPITAL.max_thread)
                keys << " thread=" << hospital.max_thread;
            if (hospital.max_cloth != DEFAULT_HOSPITAL.max_cloth)
                keys << " cloth=" << hospital.max_cloth;
            if (hospital.max_splints != DEFAULT_HOSPITAL.max_splints)
                keys << " splints=" << hospital.max_splints;
            if (hospital.max_crutches != DEFAULT_HOSPITAL.max_crutches)
                keys << " crutches=" << hospital.max_crutches;
            if (hospital.max_plaster != DEFAULT_HOSPITAL.max_plaster)
                keys << " plaster=" << hospital.max_plaster;
            if (hospital.max_buckets != DEFAULT_HOSPITAL.max_buckets)
                keys << " buckets=" << hospital.max_buckets;
            if (hospital.max_soap != DEFAULT_HOSPITAL.max_soap)
                keys << " soap=" << hospital.max_soap;
            keys << "}^";
        }
    }
    if (flags.bits.animal_training) keys << 't';

    string keys_str = keys.str();

    // there is no way to represent an active, but empty zone in quickfort
    if (keys_str.empty())
        return NULL;

    // remove final '^' character if there is one
    if (keys_str.back() == '^')
        keys_str.pop_back();

    return cache(keys_str);
}

static const char * get_tile_zone(const df::coord &pos,
                                  const tile_context &ctx) {
    vector<df::building_civzonest*> civzones;
    if (!Buildings::findCivzonesAt(&civzones, pos))
        return NULL;

    // we only have one "zone" blueprint, so use the "topmost" zone (that is,
    // the one that is highlighted when the cursor is over this tile).
    // overlapping zones are outside the scope of this plugin, I think.
    df::building_civzonest *zone = civzones.back();

    if (!is_rectangular(zone))
        return get_zone_keys(zone);

    if (zone->x1 != static_cast<int32_t>(pos.x)
            || zone->y1 != static_cast<int32_t>(pos.y)) {
        return if_pretty(ctx, "`");
    }

    return add_expansion_syntax(zone, get_zone_keys(zone));
}

static const char * get_tile_query(const df::coord &, const tile_context &ctx) {
    if (!ctx.b || !ctx.b->is_room)
        return NULL;
    return "r+";
}

static bool create_output_dir(color_ostream &out,
                              const blueprint_options &opts) {
    string basename = "blueprints/" + opts.name;
    size_t last_slash = basename.find_last_of("/");
    string parent_path = basename.substr(0, last_slash);

    // create output directory if it doesn't already exist
    if (!Filesystem::mkdir_recursive(parent_path)) {
        out.printerr("could not create output directory: '%s'\n",
                     parent_path.c_str());
        return false;
    }
    return true;
}

static bool get_filename(string &fname,
                         color_ostream &out,
                         blueprint_options opts, // copy because we can't const
                         const string &phase) {
    auto L = Lua::Core::State;
    Lua::StackUnwinder top(L);

    if (!lua_checkstack(L, 3) ||
        !Lua::PushModulePublic(
            out, L, "plugins.blueprint", "get_filename")) {
        out.printerr("Failed to load blueprint Lua code\n");
        return false;
    }

    Lua::Push(L, &opts);
    Lua::Push(L, phase);

    if (!Lua::SafeCall(out, L, 2, 1)) {
        out.printerr("Failed Lua call to get_filename\n");
        return false;
    }

    const char *s = lua_tostring(L, -1);
    if (!s) {
        out.printerr("Failed to retrieve filename from get_filename\n");
        return false;
    }

    fname = s;
    return true;
}

typedef vector<const char *> bp_row;     // index is x coordinate
typedef map<int16_t, bp_row> bp_area;    // key is y coordinate
typedef map<int16_t, bp_area> bp_volume; // key is z coordinate

typedef const char * (get_tile_fn)(const df::coord &pos,
                                   const tile_context &ctx);
typedef void (init_ctx_fn)(const df::coord &pos, tile_context &ctx);

struct blueprint_processor {
    bp_volume mapdata;
    const string mode;
    const string phase;
    const bool force_create;
    get_tile_fn * const get_tile;
    init_ctx_fn * const init_ctx;
    blueprint_processor(const string &mode, const string &phase,
                        bool force_create, get_tile_fn *get_tile,
                        init_ctx_fn *init_ctx)
        : mode(mode), phase(phase), force_create(force_create),
          get_tile(get_tile), init_ctx(init_ctx) { }
};

static void write_minimal(ofstream &ofile, const blueprint_options &opts,
                          const bp_volume &mapdata) {
    if (mapdata.begin() == mapdata.end())
        return;

    const string z_key = opts.depth > 0 ? "#<" : "#>";

    int16_t zprev = 0;
    for (auto area : mapdata) {
        for ( ; zprev < area.first; ++zprev)
            ofile << z_key << endl;
        int16_t yprev = 0;
        for (auto row : area.second) {
            for ( ; yprev < row.first; ++yprev)
                ofile << endl;
            size_t xprev = 0;
            auto &tiles = row.second;
            size_t rowsize = tiles.size();
            for (size_t x = 0; x < rowsize; ++x) {
                if (!tiles[x])
                    continue;
                for ( ; xprev < x; ++xprev)
                    ofile << ",";
                ofile << tiles[x];
            }
        }
        ofile << endl;
    }
}

static void write_pretty(ofstream &ofile, const blueprint_options &opts,
                         const bp_volume &mapdata) {
    const string z_key = opts.depth > 0 ? "#<" : "#>";

    int16_t absdepth = abs(opts.depth);
    for (int16_t z = 0; z < absdepth; ++z) {
        const bp_area *area = NULL;
        if (mapdata.count(z))
            area = &mapdata.at(z);
        for (int16_t y = 0; y < opts.height; ++y) {
            const bp_row *row = NULL;
            if (area && area->count(y))
                row = &area->at(y);
            for (int16_t x = 0; x < opts.width; ++x) {
                const char *tile = NULL;
                if (row)
                    tile = row->at(x);
                ofile << (tile ? tile : " ") << ",";
            }
            ofile << "#" << endl;
        }
        if (z < absdepth - 1)
            ofile << z_key << endl;
    }
}

static string get_modeline(const blueprint_options &opts, const string &mode,
                           const string &phase) {
    std::ostringstream modeline;
    modeline << "#" << mode << " label(" << phase << ")";
    if (opts.playback_start.x > 0) {
        modeline << " start(" << opts.playback_start.x
                << ";" << opts.playback_start.y;
        if (!opts.playback_start_comment.empty()) {
            modeline << ";" << opts.playback_start_comment;
        }
        modeline << ")";
    }

    return modeline.str();
}

static bool write_blueprint(color_ostream &out,
                            std::map<string, ofstream*> &output_files,
                            const blueprint_options &opts,
                            const blueprint_processor &processor,
                            bool pretty) {
    string fname;
    if (!get_filename(fname, out, opts, processor.phase))
        return false;
    if (!output_files.count(fname))
        output_files[fname] = new ofstream(fname, ofstream::trunc);

    ofstream &ofile = *output_files[fname];
    ofile << get_modeline(opts, processor.mode, processor.phase) << endl;

    if (pretty)
        write_pretty(ofile, opts, processor.mapdata);
    else
        write_minimal(ofile, opts, processor.mapdata);

    return true;
}

static void ensure_building(const df::coord &pos, tile_context &ctx) {
    if (ctx.b)
        return;
    ctx.b = Buildings::findAtTile(pos);
}

static void add_processor(vector<blueprint_processor> &processors,
                          const blueprint_options &opts, const char *mode,
                          const char *phase, bool require_phase,
                          get_tile_fn * const get_tile,
                          init_ctx_fn * const init_ctx = NULL) {
    if (opts.auto_phase || require_phase)
        processors.push_back(blueprint_processor(mode, phase, require_phase,
                                                 get_tile, init_ctx));
}

static bool do_transform(color_ostream &out,
                         const df::coord &start, const df::coord &end,
                         const blueprint_options &opts,
                         vector<string> &filenames) {
    // empty map instances to pass to emplace() below
    static const bp_area EMPTY_AREA;
    static const bp_row EMPTY_ROW;

    vector<blueprint_processor> processors;

    add_processor(processors, opts, "dig", "dig", opts.dig, get_tile_dig);
    add_processor(processors, opts, "dig", "track", opts.track, get_tile_track);
    add_processor(processors, opts, "build", "build", opts.build,
                  get_tile_build, ensure_building);
    add_processor(processors, opts, "place", "place", opts.place,
                  get_tile_place, ensure_building);
    add_processor(processors, opts, "zone", "zone", opts.zone, get_tile_zone);
    add_processor(processors, opts, "query", "query", opts.query,
                  get_tile_query, ensure_building);

    if (processors.empty()) {
        out.printerr("no phases requested! nothing to do!\n");
        return false;
    }

    if (!create_output_dir(out, opts))
        return false;

    const bool pretty = opts.format != "minimal";
    const int32_t z_inc = start.z < end.z ? 1 : -1;
    for (int32_t z = start.z; z != end.z; z += z_inc) {
        for (int32_t y = start.y; y < end.y; y++) {
            for (int32_t x = start.x; x < end.x; x++) {
                df::coord pos(x, y, z);
                tile_context ctx;
                ctx.pretty = pretty;
                for (blueprint_processor &processor : processors) {
                    if (processor.init_ctx)
                        processor.init_ctx(pos, ctx);
                    const char *tile_str = processor.get_tile(pos, ctx);
                    if (tile_str) {
                        // ensure our z-index is in the order we want to write
                        auto area = processor.mapdata.emplace(abs(z - start.z),
                                                              EMPTY_AREA);
                        auto row = area.first->second.emplace(y - start.y,
                                                              EMPTY_ROW);
                        auto &tiles = row.first->second;
                        if (row.second)
                            tiles.resize(opts.width);
                        tiles[x - start.x] = tile_str;
                    }
                }
            }
        }
    }

    std::map<string, ofstream*> output_files;
    for (blueprint_processor &processor : processors) {
        if (processor.mapdata.empty() && !processor.force_create)
            continue;
        if (!write_blueprint(out, output_files, opts, processor, pretty))
            break;
    }

    for (auto &it : output_files) {
        filenames.push_back(it.first);
        it.second->close();
        delete(it.second);
    }

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
            out, L, "plugins.blueprint", "parse_commandline")) {
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

static void print_help(color_ostream &out) {
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
                         vector<string> &files) {
    CoreSuspender suspend;

    if (parameters.size() >= 1 && parameters[0] == "gui") {
        std::ostringstream command;
        command << "gui/blueprint";
        for (size_t i = 1; i < parameters.size(); ++i) {
            command << " " << parameters[i];
        }
        string command_str = command.str();
        out.print("launching %s\n", command_str.c_str());

        Core::getInstance().setHotkeyCmd(command_str);
        return CR_OK;
    }

    blueprint_options options;
    if (!get_options(out, options, parameters) || options.help) {
        print_help(out);
        return options.help;
    }

    if (!Maps::IsValid()) {
        out.printerr("Map is not available!\n");
        return false;
    }

    // start coordinates can come from either the commandline or the map cursor
    df::coord start(options.start);
    if (start.x == -30000) {
        if (!Gui::getCursorCoords(start)) {
            out.printerr("Can't get cursor coords! Make sure you specify the"
                    " --cursor parameter or have an active cursor in DF.\n");
            return false;
        }
    }
    if (!Maps::isValidTilePos(start)) {
        out.printerr("Invalid start position: %d,%d,%d\n",
                     start.x, start.y, start.z);
        return false;
    }

    // end coords are one beyond the last processed coordinate. note that
    // options.depth can be negative.
    df::coord end(start.x + options.width, start.y + options.height,
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

    bool ok = do_transform(out, start, end, options, files);
    cache(NULL);
    return ok;
}

// entrypoint when called from Lua. returns the names of the generated files
static int run(lua_State *L) {
    int argc = lua_gettop(L);
    vector<string> argv;

    for (int i = 1; i <= argc; ++i) {
        const char *s = lua_tostring(L, i);
        if (s == NULL)
            luaL_error(L, "all parameters must be strings");
        argv.push_back(s);
    }

    vector<string> files;
    color_ostream *out = Lua::GetOutput(L);
    if (!out)
        out = &Core::getInstance().getConsole();
    if (do_blueprint(*out, argv, files)) {
        Lua::PushVector(L, files);
        return 1;
    }

    return 0;
}

command_result blueprint(color_ostream &out, vector<string> &parameters) {
    vector<string> files;
    if (do_blueprint(out, parameters, files)) {
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
